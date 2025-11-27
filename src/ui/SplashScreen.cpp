#include "SplashScreen.h"

#include "config/ConfigManager.h"
#include "logger/Logger.h"

#include <wx/dcbuffer.h>
#include <wx/dir.h>
#include <wx/frame.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/region.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>
#include <wx/filename.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <random>
#include <string>
#include <vector>

class SplashImagePanel : public wxPanel {
public:
    explicit SplashImagePanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetDoubleBuffered(true);
        Bind(wxEVT_PAINT, &SplashImagePanel::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &SplashImagePanel::OnEraseBackground, this);
    }

    void SetBitmap(const wxBitmap& bitmap) {
        m_bitmap = bitmap;
        SetMinSize(bitmap.GetSize());
        SetSize(bitmap.GetSize());
        Refresh();
    }

    const wxBitmap& GetBitmap() const { return m_bitmap; }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        // Note: Do not call dc.Clear() for transparent backgrounds on Windows
        // as it may fill transparent areas with black before the bitmap is drawn
        // The wxBG_STYLE_PAINT style and OnEraseBackground handler prevent background erasure
        if (m_bitmap.IsOk()) {
            dc.DrawBitmap(m_bitmap, 0, 0, true);
        }
    }

    void OnEraseBackground(wxEraseEvent& event) {
        event.Skip(false);  
    }

    wxBitmap m_bitmap;
};

namespace {

std::vector<wxString> splitList(const wxString& value) {
    std::vector<wxString> parts;
    wxStringTokenizer tokenizer(value, ",;", wxTOKEN_STRTOK);
    while (tokenizer.HasMoreTokens()) {
        wxString token = tokenizer.GetNextToken().Trim(true).Trim(false);
        if (!token.empty()) {
            parts.push_back(token);
        }
    }
    return parts;
}

wxFileName resolvePath(const wxString& value, bool expectDir) {
    wxFileName path(value);
    if (path.IsAbsolute()) {
        path.Normalize();
        return path;
    }

    std::vector<wxString> searchRoots;
    searchRoots.push_back(wxFileName::GetCwd());
    wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
    searchRoots.push_back(exePath.GetPath());
    wxFileName parentDir(exePath);
    if (parentDir.GetDirCount() > 0) {
        parentDir.RemoveLastDir();
        searchRoots.push_back(parentDir.GetPath());
        if (parentDir.GetDirCount() > 0) {
            wxFileName projectRoot(parentDir);
            projectRoot.RemoveLastDir();
            searchRoots.push_back(projectRoot.GetPath());
        }
    }

    for (const auto& root : searchRoots) {
        wxFileName candidate(value);
        candidate.MakeAbsolute(root);
        candidate.Normalize();
        if (expectDir ? wxDirExists(candidate.GetFullPath()) : wxFileExists(candidate.GetFullPath())) {
            return candidate;
        }
    }

    wxFileName fallback(value);
    fallback.MakeAbsolute(wxFileName::GetCwd());
    fallback.Normalize();
    return fallback;
}

} // namespace

SplashScreen::SplashScreen()
    : m_frame(nullptr)
    , m_panel(nullptr)
    , m_panelSizer(nullptr)
    , m_backgroundPanel(nullptr)
    , m_messageLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_finished(false)
    , m_nextMessageIndex(0)
    , m_configLoaded(false)
    , m_selectedBackgroundImage(wxEmptyString)
{
    static bool imageHandlersInitialized = false;
    if (!imageHandlersInitialized) {
        wxInitAllImageHandlers();
        imageHandlersInitialized = true;
    }

    ConfigManager& cm = ConfigManager::getInstance();
    bool configReady = cm.isInitialized();

    wxString title = configReady ? wxString::FromUTF8(cm.getString("SplashScreen", "Title", "CAD Navigator").c_str())
        : wxString::FromUTF8("CAD Navigator");
    wxString initialMessage = configReady ? wxString::FromUTF8(cm.getString("SplashScreen", "InitialMessage", "Starting services...").c_str())
        : wxString::FromUTF8("Starting services...");

    initializeFrame(title);
    if (configReady) {
        loadBackgroundImage();
        loadConfiguredMessages();
        m_configLoaded = true;
    }

    ShowMessage(initialMessage);
}

SplashScreen::~SplashScreen() {
    Finish();
}

void SplashScreen::ShowMessage(const wxString& message) {
    if (!m_frame || m_finished) {
        return;
    }
    m_messageLabel->SetLabel(message);
    int wrapWidth = m_frame->GetClientSize().GetWidth() - 60;
    if (m_backgroundPanel && m_backgroundPanel->GetBitmap().IsOk()) {
        wrapWidth = m_backgroundPanel->GetBitmap().GetWidth() - 40;
    }
    wrapWidth = std::max(wrapWidth, 200);
    m_messageLabel->Wrap(wrapWidth);
    m_frame->Layout();
    m_frame->Update();
    wxSafeYield(m_frame, true);
}

bool SplashScreen::ShowConfiguredMessage(const wxString& key) {
    ConfigManager& cm = ConfigManager::getInstance();
    if (!cm.isInitialized()) {
        return false;
    }

    std::string utf8 = cm.getString("SplashScreen", key.ToStdString(), "");
    if (utf8.empty()) {
        return false;
    }
    ShowMessage(wxString::FromUTF8(utf8.c_str()));
    return true;
}

bool SplashScreen::ShowNextConfiguredMessage() {
    if (m_nextMessageIndex >= m_configMessages.size()) {
        return false;
    }
    ShowMessage(m_configMessages[m_nextMessageIndex]);
    ++m_nextMessageIndex;
    return true;
}

void SplashScreen::ReloadFromConfig(size_t messagesAlreadyShown) {
    ConfigManager& cm = ConfigManager::getInstance();
    if (!cm.isInitialized()) {
        return;
    }

    // Don't reset selected background image - keep the same image throughout the application launch
    // This ensures only one random image is shown per launch, even if config is reloaded
    // Only reload the background if it hasn't been selected yet
    if (m_selectedBackgroundImage.IsEmpty()) {
    loadBackgroundImage();
    }

    loadConfiguredMessages();
    m_configLoaded = true;
    m_nextMessageIndex = std::min(messagesAlreadyShown, m_configMessages.size());
}

void SplashScreen::Finish() {
    if (m_finished) {
        return;
    }
    m_finished = true;
    if (m_frame) {
        m_frame->Hide();
        m_frame->Destroy();
        m_frame = nullptr;
    }
}

void SplashScreen::initializeFrame(const wxString& title) {
    const long frameStyle = wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxFRAME_SHAPED | wxBORDER_NONE;
    m_frame = new wxFrame(nullptr, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(480, 320), frameStyle);
    
    m_frame->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_frame->SetBackgroundColour(wxColour(0, 0, 0, 0));
    m_frame->SetDoubleBuffered(true);
    m_frame->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent& evt) { evt.Skip(false); });

    m_panel = new wxPanel(m_frame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_panel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_panel->SetBackgroundColour(wxColour(0, 0, 0, 0));
    m_panel->SetDoubleBuffered(true);
    m_panel->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent& evt) { evt.Skip(false); });

    m_panelSizer = new wxBoxSizer(wxVERTICAL);

    m_backgroundPanel = nullptr;

    m_messageLabel = new wxStaticText(m_panel, wxID_ANY, wxEmptyString);
    m_messageLabel->SetForegroundColour(wxColour(255, 255, 255));
    m_messageLabel->SetBackgroundColour(wxColour(0, 0, 0, 0));
    wxFont messageFont = m_messageLabel->GetFont();
    messageFont.SetPointSize(messageFont.GetPointSize() + 1);
    m_messageLabel->SetFont(messageFont);

    m_titleLabel = new wxStaticText(m_panel, wxID_ANY, title);
    m_titleLabel->SetForegroundColour(wxColour(255, 255, 255));
    m_titleLabel->SetBackgroundColour(wxColour(0, 0, 0, 0));
    wxFont titleFont = m_titleLabel->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 6);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_titleLabel->SetFont(titleFont);

    m_panel->SetSizer(m_panelSizer);
    m_panelSizer->Add(m_titleLabel, 0, wxLEFT | wxRIGHT | wxTOP, 20);
    m_panelSizer->AddSpacer(8);
    m_panelSizer->Add(m_messageLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 20);

    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(m_panel, 1, wxEXPAND);
    m_frame->SetSizer(frameSizer);

    m_frame->CentreOnScreen();
    m_frame->Show();
    wxSafeYield(m_frame, true);
}

void SplashScreen::loadConfiguredMessages() {
    ConfigManager& cm = ConfigManager::getInstance();
    if (!cm.isInitialized()) {
        return;
    }

    m_configMessages.clear();
    wxString messageList = wxString::FromUTF8(cm.getString("SplashScreen", "Messages", "").c_str());
    auto sequences = splitList(messageList);
    for (const auto& entry : sequences) {
        m_configMessages.push_back(entry);
    }
    m_nextMessageIndex = 0;
}

void SplashScreen::loadBackgroundImage() {
    ConfigManager& cm = ConfigManager::getInstance();
    if (!cm.isInitialized()) {
        return;
    }

    // If we already have a selected image, always reuse it to avoid showing different images
    // This ensures that during a single application launch, only one random image is shown
    if (!m_selectedBackgroundImage.IsEmpty()) {
        loadBackgroundImageFromPath(m_selectedBackgroundImage);
        return;
    }

    wxString imagesValue = wxString::FromUTF8(cm.getString("SplashScreen", "BackgroundImages", "").c_str());
    auto imageCandidates = splitList(imagesValue);

    wxString directoryValue = wxString::FromUTF8(cm.getString("SplashScreen", "BackgroundDirectory", "config/splashscreen").c_str());

    auto resolveCandidate = [&](const wxString& value, bool expectDir) -> wxFileName {
        wxFileName path(value);
        if (path.IsAbsolute()) {
            path.Normalize();
            return path;
        }

        std::vector<wxString> searchRoots;
        searchRoots.push_back(wxFileName::GetCwd());
        wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        exePath.Normalize();
        searchRoots.push_back(exePath.GetPath());

        wxFileName parentDir(exePath);
        for (int i = 0; i < 3 && parentDir.GetDirCount() > 0; ++i) {
            parentDir.RemoveLastDir();
            searchRoots.push_back(parentDir.GetPath());
        }

        for (const auto& root : searchRoots) {
            wxFileName candidate(value);
            candidate.MakeAbsolute(root);
            candidate.Normalize();
            if (expectDir ? wxDirExists(candidate.GetFullPath()) : wxFileExists(candidate.GetFullPath())) {
                return candidate;
            }
        }

        wxFileName fallback(value);
        fallback.MakeAbsolute(wxFileName::GetCwd());
        fallback.Normalize();
        return fallback;
    };

    struct BackgroundEntry {
        wxString normal;
        wxString hidpi;
    };

    std::map<wxString, BackgroundEntry> backgroundMap;

    auto registerCandidate = [&](const wxString& candidate) {
        wxFileName path = resolveCandidate(candidate, false);
        if (!wxFileExists(path.GetFullPath())) {
            return;
        }
        wxString baseName = path.GetName();
        bool isHiDpi = baseName.EndsWith("_2x");
        if (isHiDpi) {
            baseName = baseName.Mid(0, baseName.length() - 3);
        }
        BackgroundEntry& entry = backgroundMap[baseName];
        if (isHiDpi) {
            entry.hidpi = path.GetFullPath();
        } else {
            entry.normal = path.GetFullPath();
        }
    };

    for (const auto& candidate : imageCandidates) {
        registerCandidate(candidate);
    }

    if (backgroundMap.empty()) {
        wxFileName directoryPath = resolveCandidate(directoryValue, true);
        if (wxDirExists(directoryPath.GetFullPath())) {
            wxDir dir(directoryPath.GetFullPath());
            wxString filename;
            bool cont = dir.GetFirst(&filename, "*.png", wxDIR_FILES);
            while (cont) {
                registerCandidate(directoryPath.GetFullPath() + wxFILE_SEP_PATH + filename);
                cont = dir.GetNext(&filename);
            }
        }
    }

    std::vector<BackgroundEntry> backgrounds;
    backgrounds.reserve(backgroundMap.size());
    for (const auto& pair : backgroundMap) {
        if (!pair.second.normal.empty() || !pair.second.hidpi.empty()) {
            backgrounds.push_back(pair.second);
        }
    }

    if (backgrounds.empty()) {
        return;
    }

    double scaleFactor = 1.0;
#if wxCHECK_VERSION(3,1,5)
    if (m_frame) {
        scaleFactor = m_frame->GetContentScaleFactor();
    }
#endif

    // Use current time as additional seed for better randomness across launches
    auto now = std::chrono::high_resolution_clock::now();
    auto seed = static_cast<unsigned int>(now.time_since_epoch().count());

    std::mt19937 gen(seed);
    std::uniform_int_distribution<size_t> dist(0, backgrounds.size() - 1);
    const BackgroundEntry& entry = backgrounds[dist(gen)];

    wxString selected = entry.normal;
    if (scaleFactor > 1.5 && !entry.hidpi.empty()) {
        selected = entry.hidpi;
    } else if (selected.empty()) {
        selected = entry.hidpi;
    }

    if (selected.empty()) {
        return;
    }

    // Store the selected image path to avoid re-selection on config reload
    m_selectedBackgroundImage = selected;

    loadBackgroundImageFromPath(selected);
}

void SplashScreen::loadBackgroundImageFromPath(const wxString& imagePath) {
    wxImage image;
    if (!image.LoadFile(imagePath)) {
        LOG_WRN("SplashScreen", std::string("Failed to load splash background: ") + imagePath.ToStdString());
        return;
    }

    wxBitmap bitmap(image);
    if (!bitmap.IsOk()) {
        return;
    }

    if (!m_backgroundPanel) {
        m_panelSizer->Detach(m_titleLabel);
        m_panelSizer->Detach(m_messageLabel);

        m_backgroundPanel = new SplashImagePanel(m_panel);
        m_backgroundPanel->SetBackgroundColour(wxColour(0, 0, 0, 0));
        m_messageLabel->Reparent(m_backgroundPanel);
        m_titleLabel->Reparent(m_backgroundPanel);
        wxBoxSizer* overlaySizer = new wxBoxSizer(wxVERTICAL);
        overlaySizer->AddSpacer(60);
        overlaySizer->Add(m_titleLabel, 0, wxLEFT | wxRIGHT, 60);
        overlaySizer->AddSpacer(8);
        overlaySizer->Add(m_messageLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 60);
        m_backgroundPanel->SetSizer(overlaySizer);
        m_panelSizer->Insert(0, m_backgroundPanel, 0, wxALIGN_LEFT, 0);
    }

    m_backgroundPanel->SetBitmap(bitmap);

    int wrapWidth = bitmap.GetWidth() - 40;
    wrapWidth = std::max(wrapWidth, 200);
    m_messageLabel->Wrap(wrapWidth);

    m_panel->SetMinSize(wxSize(bitmap.GetWidth(), bitmap.GetHeight()));
    m_panelSizer->Layout();
    m_panel->Layout();

    m_frame->SetClientSize(m_panel->GetBestSize());

#ifdef __WXMSW__
    if (image.HasAlpha()) {
        const int w = image.GetWidth();
        const int h = image.GetHeight();
        const unsigned char* alpha = image.GetAlpha();
        wxRegion region;
        const int threshold = 90;
        for (int y = 0; y < h; ++y) {
            int start = -1;
            for (int x = 0; x < w; ++x) {
                bool opaque = alpha[y * w + x] >= threshold;
                if (opaque) {
                    if (start < 0) {
                        start = x;
                    }
                }
                else if (start >= 0) {
                    region.Union(start, y, x - start, 1);
                    start = -1;
                }
            }
            if (start >= 0) {
                region.Union(start, y, w - start, 1);
            }
        }
        if (!region.IsEmpty()) {
            m_frame->SetShape(region);
        }
    }
#elif defined(__WXGTK__)
    if (image.HasAlpha()) {
        wxRegion region(bitmap, wxColour(0, 0, 0));
        if (!region.IsEmpty()) {
            m_frame->SetShape(region);
        }
    }
#endif

    m_frame->CentreOnScreen();
}

int SplashScreen::GetBackgroundHeight() const {
    if (!m_backgroundPanel) {
        return 0;
    }
    const wxBitmap& bmp = m_backgroundPanel->GetBitmap();
    return bmp.IsOk() ? bmp.GetHeight() : 0;
}