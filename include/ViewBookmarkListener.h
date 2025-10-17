#pragma once

#include "CommandListener.h"
#include "Canvas.h"
#include "CommandType.h"
#include "ViewBookmark.h"
#include "CameraAnimation.h"
#include <wx/string.h>
#include <Inventor/nodes/SoCamera.h>

class ViewBookmarkListener : public CommandListener {
public:
    ViewBookmarkListener();
    ~ViewBookmarkListener() override = default;

    void setCamera(SoCamera* camera, std::function<void()> viewRefreshCallback = nullptr) {
        m_camera = camera;
        m_viewRefreshCallback = viewRefreshCallback;
        // Set camera to NavigationAnimator for bookmark animations
        if (m_camera) {
            NavigationAnimator::getInstance().setCamera(m_camera);
            // Set view refresh callback for NavigationAnimator if provided
            if (viewRefreshCallback) {
                NavigationAnimator::getInstance().setViewRefreshCallback(viewRefreshCallback);
            }
        }
    }
    void setCanvas(Canvas* canvas) { m_canvas = canvas; }

    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ViewBookmarkListener"; }

private:
    SoCamera* m_camera;
    Canvas* m_canvas;
    std::function<void()> m_viewRefreshCallback;

    void saveCurrentBookmark();
    void restoreBookmark(const wxString& name);
    void animateToBookmark(const wxString& name);
    void showBookmarkManager();
    void triggerViewRefresh();
};

class ViewBookmarkSaveListener : public CommandListener {
public:
    ViewBookmarkSaveListener();
    ~ViewBookmarkSaveListener() override = default;

    void setCamera(SoCamera* camera) { m_camera = camera; }

    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ViewBookmarkSaveListener"; }

private:
    SoCamera* m_camera;
};

class ViewBookmarkRestoreListener : public CommandListener {
public:
    ViewBookmarkRestoreListener(const wxString& bookmarkName);
    ~ViewBookmarkRestoreListener() override = default;

    void setCamera(SoCamera* camera) { m_camera = camera; }

    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ViewBookmarkRestoreListener"; }

private:
    wxString m_bookmarkName;
    SoCamera* m_camera;
};
