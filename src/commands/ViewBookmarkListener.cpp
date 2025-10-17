#include "ViewBookmarkListener.h"
#include "ViewBookmark.h"
#include "CameraAnimation.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/choice.h>
#include <unordered_set>

//==============================================================================
// ViewBookmarkListener Implementation
//==============================================================================

ViewBookmarkListener::ViewBookmarkListener()
    : m_camera(nullptr), m_canvas(nullptr)
{
}

CommandResult ViewBookmarkListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {

    cmd::CommandType cmdType = cmd::from_string(commandType);

    if (!m_camera) {
        return CommandResult(false, "No camera available for bookmark operations");
    }

    switch (cmdType) {
        case cmd::CommandType::ViewBookmarkSave:
            saveCurrentBookmark();
            return CommandResult(true, "Bookmark saved successfully");

        case cmd::CommandType::ViewBookmarkFront:
            animateToBookmark("Front");
            // Animation will handle view refresh through NavigationAnimator
            return CommandResult(true, "Animated to front view");

        case cmd::CommandType::ViewBookmarkBack:
            animateToBookmark("Back");
            // Animation will handle view refresh through NavigationAnimator
            return CommandResult(true, "Animated to back view");

        case cmd::CommandType::ViewBookmarkLeft:
            animateToBookmark("Left");
            // Animation will handle view refresh through NavigationAnimator
            return CommandResult(true, "Animated to left view");

        case cmd::CommandType::ViewBookmarkRight:
            animateToBookmark("Right");
            // Animation will handle view refresh through NavigationAnimator
            return CommandResult(true, "Animated to right view");

        case cmd::CommandType::ViewBookmarkTop:
            animateToBookmark("Top");
            // Animation will handle view refresh through NavigationAnimator
            return CommandResult(true, "Animated to top view");

        case cmd::CommandType::ViewBookmarkBottom:
            animateToBookmark("Bottom");
            // Animation will handle view refresh through NavigationAnimator
            return CommandResult(true, "Animated to bottom view");

        case cmd::CommandType::ViewBookmarkIsometric:
            animateToBookmark("Isometric");
            // Animation will handle view refresh through NavigationAnimator
            return CommandResult(true, "Animated to isometric view");

        case cmd::CommandType::ViewBookmarkManager:
            showBookmarkManager();
            return CommandResult(true, "Bookmark manager opened");

        default:
            return CommandResult(false, "Unknown bookmark command: " + commandType);
    }
}

bool ViewBookmarkListener::canHandleCommand(const std::string& commandType) const {
    cmd::CommandType cmdType = cmd::from_string(commandType);
    return cmdType == cmd::CommandType::ViewBookmarkSave ||
           cmdType == cmd::CommandType::ViewBookmarkFront ||
           cmdType == cmd::CommandType::ViewBookmarkBack ||
           cmdType == cmd::CommandType::ViewBookmarkLeft ||
           cmdType == cmd::CommandType::ViewBookmarkRight ||
           cmdType == cmd::CommandType::ViewBookmarkTop ||
           cmdType == cmd::CommandType::ViewBookmarkBottom ||
           cmdType == cmd::CommandType::ViewBookmarkIsometric ||
           cmdType == cmd::CommandType::ViewBookmarkManager;
}

void ViewBookmarkListener::saveCurrentBookmark() {
    wxString name = wxGetTextFromUser("Enter bookmark name:", "Save Bookmark",
                                     "Bookmark " + wxDateTime::Now().Format("%H%M%S"));

    if (name.IsEmpty()) {
        return;
    }

    auto& bm = ViewBookmarkManager::getInstance();
    if (bm.addBookmark(name, m_camera->position.getValue(), m_camera->orientation.getValue())) {
        wxMessageBox("Bookmark saved: " + name, "Success", wxOK | wxICON_INFORMATION);
    } else {
        wxMessageBox("Failed to save bookmark or name already exists", "Error", wxOK | wxICON_ERROR);
    }
}

void ViewBookmarkListener::restoreBookmark(const wxString& name) {
    auto& bm = ViewBookmarkManager::getInstance();
    auto bookmark = bm.getBookmark(name);

    if (bookmark) {
        m_camera->position.setValue(bookmark->getPosition());
        m_camera->orientation.setValue(bookmark->getRotation());
    }
}

void ViewBookmarkListener::animateToBookmark(const wxString& name) {
    // For standard views, use SceneManager's setView method which properly
    // calculates camera position based on scene bounds
    // For custom bookmarks, use NavigationAnimator

    static const std::unordered_set<wxString> standardViews = {
        "Front", "Back", "Left", "Right", "Top", "Bottom", "Isometric"
    };

    if (standardViews.count(name) > 0) {
        // Use SceneManager for standard views
        auto sceneManager = m_canvas ? m_canvas->getSceneManager() : nullptr;
        if (sceneManager) {
            sceneManager->setView(std::string(name.mb_str()));
            // setView already handles view refresh, no need to trigger again
        } else {
            LOG_ERR_S("ViewBookmarkListener: No scene manager available for standard view");
        }
    } else {
        // Use NavigationAnimator for custom bookmarks
        NavigationAnimator::getInstance().animateToBookmark(name, 1.0f);
    }
}

void ViewBookmarkListener::showBookmarkManager() {
    auto& bm = ViewBookmarkManager::getInstance();
    auto names = bm.getBookmarkNames();

    if (names.empty()) {
        wxMessageBox("No bookmarks available", "Bookmark Manager", wxOK | wxICON_INFORMATION);
        return;
    }

    wxArrayString choices;
    for (const auto& name : names) {
        choices.Add(name);
    }

    wxString selection = wxGetSingleChoice("Select bookmark to restore:",
                                          "Bookmark Manager", choices);

    if (!selection.IsEmpty()) {
        animateToBookmark(selection);
    }
}

void ViewBookmarkListener::triggerViewRefresh() {
    if (m_viewRefreshCallback) {
        m_viewRefreshCallback();
    }
}

//==============================================================================
// ViewBookmarkSaveListener Implementation
//==============================================================================

ViewBookmarkSaveListener::ViewBookmarkSaveListener()
    : m_camera(nullptr)
{
}

CommandResult ViewBookmarkSaveListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {

    cmd::CommandType cmdType = cmd::from_string(commandType);

    if (cmdType == cmd::CommandType::ViewBookmarkSave) {
        if (!m_camera) {
            return CommandResult(false, "No camera available for bookmark operations");
        }

        wxString name = wxGetTextFromUser("Enter bookmark name:", "Save Bookmark",
                                         "Bookmark " + wxDateTime::Now().Format("%H%M%S"));

        if (name.IsEmpty()) {
            return CommandResult(false, "Bookmark name is empty");
        }

        auto& bm = ViewBookmarkManager::getInstance();
        if (bm.addBookmark(name, m_camera->position.getValue(), m_camera->orientation.getValue())) {
            return CommandResult(true, "Bookmark saved: " + std::string(name.mb_str()));
        } else {
            return CommandResult(false, "Failed to save bookmark or name already exists");
        }
    }
    return CommandResult(false, "Unknown command for ViewBookmarkSaveListener: " + commandType);
}

bool ViewBookmarkSaveListener::canHandleCommand(const std::string& commandType) const {
    return cmd::from_string(commandType) == cmd::CommandType::ViewBookmarkSave;
}

//==============================================================================
// ViewBookmarkRestoreListener Implementation
//==============================================================================

ViewBookmarkRestoreListener::ViewBookmarkRestoreListener(const wxString& bookmarkName)
    : m_bookmarkName(bookmarkName)
    , m_camera(nullptr)
{
}

CommandResult ViewBookmarkRestoreListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {

    cmd::CommandType cmdType = cmd::from_string(commandType);

    if (cmdType == cmd::CommandType::ViewBookmarkRestore) {
        if (!m_camera) {
            return CommandResult(false, "No camera available for bookmark operations");
        }

        NavigationAnimator::getInstance().animateToBookmark(m_bookmarkName, 1.0f);
        return CommandResult(true, "Animated to bookmark: " + std::string(m_bookmarkName.mb_str()));
    }
    return CommandResult(false, "Unknown command for ViewBookmarkRestoreListener: " + commandType);
}

bool ViewBookmarkRestoreListener::canHandleCommand(const std::string& commandType) const {
    return cmd::from_string(commandType) == cmd::CommandType::ViewBookmarkRestore;
}
