#pragma once

#include <wx/event.h>
#include <Inventor/SbLinear.h>
#include <vector>
#include <functional>
#include <memory>

class SoCamera;

class ZoomLevel {
public:
    ZoomLevel(float scale, const wxString& name, const wxString& description = wxEmptyString);
    ~ZoomLevel() = default;

    float getScale() const { return m_scale; }
    const wxString& getName() const { return m_name; }
    const wxString& getDescription() const { return m_description; }

    // Comparison operators for sorting
    bool operator<(const ZoomLevel& other) const { return m_scale < other.m_scale; }
    bool operator==(const ZoomLevel& other) const { return m_scale == other.m_scale; }

private:
    float m_scale;
    wxString m_name;
    wxString m_description;
};

class ZoomController {
public:
    enum ZoomMode {
        CONTINUOUS,     // Continuous zoom (original behavior)
        DISCRETE,       // Snap to predefined zoom levels
        HYBRID          // Continuous with level hints
    };

    enum ZoomDirection {
        ZOOM_IN,
        ZOOM_OUT,
        ZOOM_RESET
    };

    ZoomController();
    ~ZoomController() = default;

    // Setup
    void setCamera(SoCamera* camera) { m_camera = camera; }
    void setZoomMode(ZoomMode mode) { m_zoomMode = mode; }
    ZoomMode getZoomMode() const { return m_zoomMode; }

    // Zoom level management
    void addZoomLevel(float scale, const wxString& name, const wxString& description = wxEmptyString);
    void removeZoomLevel(float scale);
    void clearZoomLevels();
    const std::vector<ZoomLevel>& getZoomLevels() const { return m_zoomLevels; }

    // Zoom operations
    bool zoomIn(float factor = 1.2f);
    bool zoomOut(float factor = 0.833f);
    bool zoomTo(float targetScale);
    bool zoomToLevel(size_t levelIndex);
    bool zoomReset();

    // Current state
    float getCurrentZoomScale() const;
    size_t getCurrentZoomLevel() const;
    wxString getCurrentZoomLevelName() const;

    // Zoom constraints
    void setZoomLimits(float minScale, float maxScale);
    void getZoomLimits(float& minScale, float& maxScale) const;

    // Callbacks
    void setZoomChangedCallback(std::function<void(float)> callback) { m_zoomChangedCallback = callback; }
    void setZoomLevelChangedCallback(std::function<void(size_t, const wxString&)> callback) {
        m_zoomLevelChangedCallback = callback;
    }
    void setViewRefreshCallback(std::function<void()> callback) { m_viewRefreshCallback = callback; }

private:
    // Zoom calculation
    float calculateZoomScale() const;
    float calculateAdaptiveSpeed(float currentScale, ZoomDirection direction) const;
    void updateCameraZoom(float newScale);
    void notifyZoomChanged(float newScale);

    // Level management
    void sortZoomLevels();
    size_t findNearestZoomLevel(float scale) const;

    SoCamera* m_camera;
    ZoomMode m_zoomMode;

    // Zoom levels (sorted by scale, ascending)
    std::vector<ZoomLevel> m_zoomLevels;

    // Zoom limits
    float m_minZoomScale;
    float m_maxZoomScale;

    // Current state
    float m_baseScale;  // Scale at zoom reset

    // Base camera parameters (stored on first zoom)
    float m_baseDistance;    // Base focal distance for perspective camera
    float m_baseHeight;      // Base height for orthographic camera
    SbVec3f m_basePosition;  // Base position for perspective camera

    // Callbacks
    std::function<void(float)> m_zoomChangedCallback;
    std::function<void(size_t, const wxString&)> m_zoomLevelChangedCallback;
    std::function<void()> m_viewRefreshCallback;
};

//==============================================================================
// ZoomManager - High-level zoom control with UI integration
//==============================================================================

class ZoomManager {
public:
    static ZoomManager& getInstance();

    // Quick zoom operations
    void zoomIn() { if (m_controller) m_controller->zoomIn(); }
    void zoomOut() { if (m_controller) m_controller->zoomOut(); }
    void zoomReset() { if (m_controller) m_controller->zoomReset(); }
    void zoomToLevel(size_t level) { if (m_controller) m_controller->zoomToLevel(level); }

    // Setup
    void setController(std::shared_ptr<ZoomController> controller) { m_controller = controller; }
    ZoomController* getController() const { return m_controller.get(); }

    // View refresh callback
    void setViewRefreshCallback(std::function<void()> callback) {
        if (m_controller) {
            m_controller->setViewRefreshCallback(callback);
        }
    }

    // Default zoom levels
    void createDefaultZoomLevels();

private:
    ZoomManager();
    ~ZoomManager() = default;

    std::shared_ptr<ZoomController> m_controller;
};
