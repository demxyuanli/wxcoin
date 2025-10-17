#include "ZoomController.h"
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <wx/log.h>
#include <algorithm>
#include <cmath>

//==============================================================================
// ZoomLevel Implementation
//==============================================================================

ZoomLevel::ZoomLevel(float scale, const wxString& name, const wxString& description)
    : m_scale(scale)
    , m_name(name)
    , m_description(description)
{
}

//==============================================================================
// ZoomController Implementation
//==============================================================================

ZoomController::ZoomController()
    : m_camera(nullptr)
    , m_zoomMode(CONTINUOUS)
    , m_minZoomScale(0.01f)
    , m_maxZoomScale(100.0f)
    , m_baseScale(1.0f)
    , m_baseDistance(5.0f)
    , m_baseHeight(10.0f)
    , m_basePosition(0, 0, 5)
{
    // Create default zoom levels
    addZoomLevel(0.1f, "10%", "Very zoomed out");
    addZoomLevel(0.25f, "25%", "Zoomed out");
    addZoomLevel(0.5f, "50%", "Half size");
    addZoomLevel(1.0f, "100%", "Actual size");
    addZoomLevel(2.0f, "200%", "Double size");
    addZoomLevel(4.0f, "400%", "Quadruple size");
    addZoomLevel(8.0f, "800%", "Very zoomed in");
}

void ZoomController::addZoomLevel(float scale, const wxString& name, const wxString& description) {
    // Remove existing level with same scale
    removeZoomLevel(scale);

    // Add new level
    m_zoomLevels.emplace_back(scale, name, description);

    // Sort levels by scale
    sortZoomLevels();
}

void ZoomController::removeZoomLevel(float scale) {
    auto it = std::find_if(m_zoomLevels.begin(), m_zoomLevels.end(),
        [scale](const ZoomLevel& level) { return std::abs(level.getScale() - scale) < 0.001f; });

    if (it != m_zoomLevels.end()) {
        m_zoomLevels.erase(it);
    }
}

void ZoomController::clearZoomLevels() {
    m_zoomLevels.clear();
}

bool ZoomController::zoomIn(float factor) {
    if (!m_camera) return false;

    float currentScale = getCurrentZoomScale();

    if (m_zoomMode == DISCRETE) {
        // Find next higher zoom level
        size_t currentLevel = findNearestZoomLevel(currentScale);
        if (currentLevel < m_zoomLevels.size() - 1) {
            return zoomToLevel(currentLevel + 1);
        }
        return false; // Already at maximum zoom
    } else {
        // Continuous zoom
        float adaptiveFactor = calculateAdaptiveSpeed(currentScale, ZOOM_IN);
        float newScale = currentScale * adaptiveFactor;

        newScale = std::min(newScale, m_maxZoomScale);

        if (newScale != currentScale) {
            return zoomTo(newScale);
        }
    }

    return false;
}

bool ZoomController::zoomOut(float factor) {
    if (!m_camera) return false;

    float currentScale = getCurrentZoomScale();

    if (m_zoomMode == DISCRETE) {
        // Find next lower zoom level
        size_t currentLevel = findNearestZoomLevel(currentScale);
        if (currentLevel > 0) {
            return zoomToLevel(currentLevel - 1);
        }
        return false; // Already at minimum zoom
    } else {
        // Continuous zoom
        float adaptiveFactor = calculateAdaptiveSpeed(currentScale, ZOOM_OUT);
        float newScale = currentScale * adaptiveFactor;

        newScale = std::max(newScale, m_minZoomScale);

        if (newScale != currentScale) {
            return zoomTo(newScale);
        }
    }

    return false;
}

bool ZoomController::zoomTo(float targetScale) {
    if (!m_camera) return false;

    // Clamp to limits
    targetScale = std::max(m_minZoomScale, std::min(m_maxZoomScale, targetScale));

    updateCameraZoom(targetScale);
    notifyZoomChanged(targetScale);

    return true;
}

bool ZoomController::zoomToLevel(size_t levelIndex) {
    if (levelIndex >= m_zoomLevels.size()) return false;

    float targetScale = m_zoomLevels[levelIndex].getScale();
    return zoomTo(targetScale);
}

bool ZoomController::zoomReset() {
    return zoomTo(m_baseScale);
}

float ZoomController::getCurrentZoomScale() const {
    if (!m_camera) return m_baseScale;

    return calculateZoomScale();
}

size_t ZoomController::getCurrentZoomLevel() const {
    if (m_zoomLevels.empty()) return 0;

    float currentScale = getCurrentZoomScale();
    return findNearestZoomLevel(currentScale);
}

wxString ZoomController::getCurrentZoomLevelName() const {
    size_t levelIndex = getCurrentZoomLevel();
    if (levelIndex < m_zoomLevels.size()) {
        return m_zoomLevels[levelIndex].getName();
    }
    return wxString::Format("%.0f%%", getCurrentZoomScale() * 100);
}

void ZoomController::setZoomLimits(float minScale, float maxScale) {
    m_minZoomScale = std::max(0.001f, minScale);
    m_maxZoomScale = std::max(minScale, maxScale);
}

void ZoomController::getZoomLimits(float& minScale, float& maxScale) const {
    minScale = m_minZoomScale;
    maxScale = m_maxZoomScale;
}

float ZoomController::calculateZoomScale() const {
    if (!m_camera) return m_baseScale;

    if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        // For perspective camera, use distance from origin
        SbVec3f position = m_camera->position.getValue();
        float distance = position.length();

        // Assuming base distance corresponds to base scale
        return m_baseScale * (5.0f / distance); // 5.0 is default camera distance
    } else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
        // For orthographic camera, use height
        SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
        float height = orthoCam->height.getValue();

        // Assuming base height corresponds to base scale
        return m_baseScale * (10.0f / height); // 10.0 is default ortho height
    }

    return m_baseScale;
}

float ZoomController::calculateAdaptiveSpeed(float currentScale, ZoomDirection direction) const {
    // Adaptive zoom speed based on current zoom level
    // Zoom slower when very zoomed in/out to provide more precision

    float baseFactor = (direction == ZOOM_IN) ? 1.2f : 0.833f;

    if (direction == ZOOM_IN) {
        // When zoomed in, slow down for more precision
        if (currentScale > 10.0f) {
            baseFactor = 1.1f;
        } else if (currentScale > 5.0f) {
            baseFactor = 1.15f;
        }
    } else {
        // When zoomed out, slow down for more precision
        if (currentScale < 0.1f) {
            baseFactor = 0.9f;
        } else if (currentScale < 0.5f) {
            baseFactor = 0.85f;
        }
    }

    return baseFactor;
}

void ZoomController::updateCameraZoom(float newScale) {
    if (!m_camera) return;

    // Store initial state on first zoom operation
    if (m_baseScale == 1.0f && newScale != 1.0f) {
        // This is the first zoom operation, store current state as base
        if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
            SoPerspectiveCamera* perspCam = static_cast<SoPerspectiveCamera*>(m_camera);
            m_baseDistance = perspCam->focalDistance.getValue();
            m_basePosition = m_camera->position.getValue();
        } else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
            SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
            m_baseHeight = orthoCam->height.getValue();
        }
        m_baseScale = 1.0f;
    }

    if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        // Perspective camera: adjust focal distance
        SoPerspectiveCamera* perspCam = static_cast<SoPerspectiveCamera*>(m_camera);
        float newFocalDistance = m_baseDistance / newScale;
        perspCam->focalDistance.setValue(newFocalDistance);

        // Adjust position to maintain view direction but change distance
        SbVec3f direction = m_basePosition;
        if (direction.length() > 0) {
            direction.normalize();
            SbVec3f newPosition = direction * (m_baseDistance / newScale);
            m_camera->position.setValue(newPosition);
        }
    } else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
        // Orthographic camera: adjust height
        SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
        float newHeight = m_baseHeight / newScale;
        orthoCam->height.setValue(newHeight);
    }

    // Mark camera as modified for Open Inventor
    if (m_camera) {
        m_camera->touch();
    }

    // Trigger view refresh after camera modification
    if (m_viewRefreshCallback) {
        m_viewRefreshCallback();
    }
}

void ZoomController::notifyZoomChanged(float newScale) {
    // Notify zoom changed
    if (m_zoomChangedCallback) {
        m_zoomChangedCallback(newScale);
    }

    // Check if crossed zoom level boundary
    if (m_zoomMode == HYBRID && !m_zoomLevels.empty()) {
        size_t nearestLevel = findNearestZoomLevel(newScale);
        static size_t lastNotifiedLevel = SIZE_MAX;

        if (nearestLevel != lastNotifiedLevel && m_zoomLevelChangedCallback) {
            m_zoomLevelChangedCallback(nearestLevel, m_zoomLevels[nearestLevel].getName());
            lastNotifiedLevel = nearestLevel;
        }
    }
}

void ZoomController::sortZoomLevels() {
    std::sort(m_zoomLevels.begin(), m_zoomLevels.end());
}

size_t ZoomController::findNearestZoomLevel(float scale) const {
    if (m_zoomLevels.empty()) return 0;

    // Find the level with scale closest to the given scale
    size_t nearestIndex = 0;
    float minDiff = std::abs(m_zoomLevels[0].getScale() - scale);

    for (size_t i = 1; i < m_zoomLevels.size(); ++i) {
        float diff = std::abs(m_zoomLevels[i].getScale() - scale);
        if (diff < minDiff) {
            minDiff = diff;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

//==============================================================================
// ZoomManager Implementation
//==============================================================================

ZoomManager& ZoomManager::getInstance() {
    static ZoomManager instance;
    return instance;
}

ZoomManager::ZoomManager() {
    m_controller = std::make_shared<ZoomController>();
    createDefaultZoomLevels();
}

void ZoomManager::createDefaultZoomLevels() {
    if (!m_controller) return;

    // Clear existing levels
    m_controller->clearZoomLevels();

    // Add common zoom levels
    m_controller->addZoomLevel(0.05f, "5%", "Extreme zoom out");
    m_controller->addZoomLevel(0.1f, "10%", "Very zoomed out");
    m_controller->addZoomLevel(0.25f, "25%", "Zoomed out");
    m_controller->addZoomLevel(0.5f, "50%", "Half size");
    m_controller->addZoomLevel(0.75f, "75%", "Three quarters");
    m_controller->addZoomLevel(1.0f, "100%", "Actual size");
    m_controller->addZoomLevel(1.5f, "150%", "One and a half");
    m_controller->addZoomLevel(2.0f, "200%", "Double size");
    m_controller->addZoomLevel(3.0f, "300%", "Triple size");
    m_controller->addZoomLevel(4.0f, "400%", "Quadruple size");
    m_controller->addZoomLevel(5.0f, "500%", "Five times");
    m_controller->addZoomLevel(8.0f, "800%", "Eight times");
    m_controller->addZoomLevel(10.0f, "1000%", "Ten times");
}
