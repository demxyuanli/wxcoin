#include "ViewBookmark.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

//==============================================================================
// ViewBookmark Implementation
//==============================================================================

ViewBookmark::ViewBookmark()
    : m_name(wxEmptyString)
    , m_position(0.0f, 0.0f, 1.0f)
    , m_rotation(SbRotation::identity())
    , m_timestamp(wxDateTime::Now())
{
}

ViewBookmark::ViewBookmark(const wxString& name, const SbVec3f& position, const SbRotation& rotation)
    : m_name(name)
    , m_position(position)
    , m_rotation(rotation)
    , m_timestamp(wxDateTime::Now())
{
}

wxString ViewBookmark::toString() const {
    wxString str = wxString::Format("%s|%f,%f,%f|%f,%f,%f,%f|%s",
        m_name,
        m_position[0], m_position[1], m_position[2],
        m_rotation[0], m_rotation[1], m_rotation[2], m_rotation[3],
        m_timestamp.FormatISODate() + " " + m_timestamp.FormatISOTime());
    return str;
}

std::shared_ptr<ViewBookmark> ViewBookmark::fromString(const wxString& str) {
    wxStringTokenizer tokenizer(str, "|");
    if (tokenizer.CountTokens() < 4) {
        return nullptr;
    }

    wxString name = tokenizer.GetNextToken();
    wxString posStr = tokenizer.GetNextToken();
    wxString rotStr = tokenizer.GetNextToken();
    wxString timeStr = tokenizer.GetNextToken();

    // Parse position
    wxStringTokenizer posTokenizer(posStr, ",");
    if (posTokenizer.CountTokens() != 3) return nullptr;

    double px, py, pz;
    if (!posTokenizer.GetNextToken().ToDouble(&px) ||
        !posTokenizer.GetNextToken().ToDouble(&py) ||
        !posTokenizer.GetNextToken().ToDouble(&pz)) {
        return nullptr;
    }
    SbVec3f position(static_cast<float>(px), static_cast<float>(py), static_cast<float>(pz));

    // Parse rotation
    wxStringTokenizer rotTokenizer(rotStr, ",");
    if (rotTokenizer.CountTokens() != 4) return nullptr;

    double rx, ry, rz, rw;
    if (!rotTokenizer.GetNextToken().ToDouble(&rx) ||
        !rotTokenizer.GetNextToken().ToDouble(&ry) ||
        !rotTokenizer.GetNextToken().ToDouble(&rz) ||
        !rotTokenizer.GetNextToken().ToDouble(&rw)) {
        return nullptr;
    }
    SbRotation rotation(static_cast<float>(rx), static_cast<float>(ry),
                       static_cast<float>(rz), static_cast<float>(rw));

    // Parse timestamp
    wxDateTime timestamp;
    if (!timeStr.IsEmpty()) {
        wxString dateStr = timeStr.BeforeFirst(' ');
        wxString timeStrPart = timeStr.AfterFirst(' ');

        if (!timestamp.ParseISODate(dateStr) || !timestamp.ParseISOTime(timeStrPart)) {
            timestamp = wxDateTime::Now();
        }
    } else {
        timestamp = wxDateTime::Now();
    }

    auto bookmark = std::make_shared<ViewBookmark>(name, position, rotation);
    bookmark->m_timestamp = timestamp;
    return bookmark;
}

bool ViewBookmark::isValid() const {
    return !m_name.IsEmpty() && m_position.length() > 0.0f;
}

//==============================================================================
// ViewBookmarkManager Implementation
//==============================================================================

ViewBookmarkManager& ViewBookmarkManager::getInstance() {
    static ViewBookmarkManager instance;
    return instance;
}

ViewBookmarkManager::ViewBookmarkManager()
    : m_configPath(getDefaultConfigPath())
{
    // Try to load existing bookmarks
    if (!loadFromFile()) {
        // Create default bookmarks if loading fails
        createDefaultBookmarks();
    }
}

bool ViewBookmarkManager::addBookmark(const wxString& name, const SbVec3f& position, const SbRotation& rotation) {
    if (name.IsEmpty() || hasBookmark(name)) {
        return false;
    }

    auto bookmark = std::make_shared<ViewBookmark>(name, position, rotation);
    m_bookmarks.push_back(bookmark);
    sortBookmarksByTimestamp();
    return true;
}

bool ViewBookmarkManager::removeBookmark(const wxString& name) {
    auto it = std::find_if(m_bookmarks.begin(), m_bookmarks.end(),
        [&name](const std::shared_ptr<ViewBookmark>& bookmark) {
            return bookmark->getName() == name;
        });

    if (it != m_bookmarks.end()) {
        m_bookmarks.erase(it);
        return true;
    }
    return false;
}

bool ViewBookmarkManager::renameBookmark(const wxString& oldName, const wxString& newName) {
    if (newName.IsEmpty() || hasBookmark(newName)) {
        return false;
    }

    auto bookmark = getBookmark(oldName);
    if (!bookmark) {
        return false;
    }

    bookmark->setName(newName);
    bookmark->updateTimestamp();
    sortBookmarksByTimestamp();
    return true;
}

std::shared_ptr<ViewBookmark> ViewBookmarkManager::getBookmark(const wxString& name) const {
    auto it = std::find_if(m_bookmarks.begin(), m_bookmarks.end(),
        [&name](const std::shared_ptr<ViewBookmark>& bookmark) {
            return bookmark->getName() == name;
        });

    return (it != m_bookmarks.end()) ? *it : nullptr;
}

std::vector<wxString> ViewBookmarkManager::getBookmarkNames() const {
    std::vector<wxString> names;
    for (const auto& bookmark : m_bookmarks) {
        names.push_back(bookmark->getName());
    }
    return names;
}

bool ViewBookmarkManager::hasBookmark(const wxString& name) const {
    return getBookmark(name) != nullptr;
}

bool ViewBookmarkManager::saveToFile(const wxString& filename) {
    wxString actualFilename = filename.IsEmpty() ? m_configPath : filename;

    try {
        wxTextFile file(actualFilename);
        bool fileExists = wxFile::Exists(actualFilename);

        if (fileExists) {
            if (!file.Open(actualFilename)) {
                return false;
            }
            file.Clear();
        } else {
            file.Create(actualFilename);
        }

        // Write header
        file.AddLine("# View Bookmarks Configuration");
        file.AddLine("# Format: Name|Position(x,y,z)|Rotation(x,y,z,w)|Timestamp");
        file.AddLine("");

        // Write bookmarks
        for (const auto& bookmark : m_bookmarks) {
            if (bookmark && bookmark->isValid()) {
                file.AddLine(bookmark->toString());
            }
        }

        return file.Write() && file.Close();
    } catch (...) {
        return false;
    }
}

bool ViewBookmarkManager::loadFromFile(const wxString& filename) {
    wxString actualFilename = filename.IsEmpty() ? m_configPath : filename;

    if (!wxFile::Exists(actualFilename)) {
        return false;
    }

    try {
        wxTextFile file;
        if (!file.Open(actualFilename)) {
            return false;
        }

        m_bookmarks.clear();

        for (size_t i = 0; i < file.GetLineCount(); ++i) {
            wxString line = file.GetLine(i).Trim().Trim(false);

            // Skip comments and empty lines
            if (line.IsEmpty() || line.StartsWith("#")) {
                continue;
            }

            auto bookmark = ViewBookmark::fromString(line);
            if (bookmark && bookmark->isValid()) {
                m_bookmarks.push_back(bookmark);
            }
        }

        file.Close();
        sortBookmarksByTimestamp();
        return true;
    } catch (...) {
        return false;
    }
}

void ViewBookmarkManager::createDefaultBookmarks() {
    m_bookmarks.clear();

    // Standard views
    addBookmark("Front", SbVec3f(0, 0, 5), SbRotation(SbVec3f(1, 0, 0), 0));
    addBookmark("Back", SbVec3f(0, 0, -5), SbRotation(SbVec3f(1, 0, 0), M_PI));
    addBookmark("Left", SbVec3f(-5, 0, 0), SbRotation(SbVec3f(0, 1, 0), M_PI/2));
    addBookmark("Right", SbVec3f(5, 0, 0), SbRotation(SbVec3f(0, 1, 0), -M_PI/2));
    addBookmark("Top", SbVec3f(0, 5, 0), SbRotation(SbVec3f(1, 0, 0), -M_PI/2));
    addBookmark("Bottom", SbVec3f(0, -5, 0), SbRotation(SbVec3f(1, 0, 0), M_PI/2));

    // Isometric views
    addBookmark("Isometric", SbVec3f(5, 5, 5),
                SbRotation(SbVec3f(1, 1, 1), 2*M_PI/3));
}

wxString ViewBookmarkManager::getDefaultConfigPath() const {
    wxString configDir = wxStandardPaths::Get().GetUserDataDir();
    wxFileName::Mkdir(configDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    wxFileName configFile(configDir, "view_bookmarks.txt");
    return configFile.GetFullPath();
}

void ViewBookmarkManager::sortBookmarksByTimestamp() {
    std::sort(m_bookmarks.begin(), m_bookmarks.end(),
        [](const std::shared_ptr<ViewBookmark>& a, const std::shared_ptr<ViewBookmark>& b) {
            return a->getTimestamp() > b->getTimestamp(); // Newest first
        });
}

