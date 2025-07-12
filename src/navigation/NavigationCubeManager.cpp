#include "NavigationCubeManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include "CuteNavCube.h"
#include "DPIManager.h"
#include "NavigationCubeConfigDialog.h"
#include <wx/wx.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>

void NavigationCubeManager::Layout::update(int newX_logical, int newY_logical, int newSize_logical,
    const wxSize& windowSize_logical, float dpiScale)
{
    size = (std::max)(80, (std::min)(newSize_logical, windowSize_logical.x / 2));
    size = (std::max)(80, (std::min)(size, windowSize_logical.y / 2));
    x = (std::max)(0, (std::min)(newX_logical, windowSize_logical.x - size));
    y = (std::max)(0, (std::min)(newY_logical, windowSize_logical.y - size));
}

NavigationCubeManager::NavigationCubeManager(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas), m_sceneManager(sceneManager), m_isEnabled(true)
{
    LOG_INF_S("NavigationCubeManager: Initializing");
    initCube();
}

NavigationCubeManager::~NavigationCubeManager()
{
    LOG_INF_S("NavigationCubeManager: Destroying");
}

void NavigationCubeManager::initCube() {
    if (!m_isEnabled) return;

    try {
        auto cubeCallback = [this](const std::string& view) {
            m_sceneManager->setView(view);
            m_canvas->Refresh(true);
        };

        wxSize clientSize = m_canvas->GetClientSize();
        float dpiScale = m_canvas->getDPIScale();

        m_navCube = std::make_unique<CuteNavCube>(cubeCallback, dpiScale, clientSize.x, clientSize.y);
        m_navCube->setRotationChangedCallback([this]() {
            syncMainCameraToCube();
            m_canvas->Refresh(true);
        });
        LOG_INF_S("NavigationCubeManager: Navigation cube (CuteNavCube) initialized");

        if (clientSize.x > 0 && clientSize.y > 0) {
            m_cubeLayout.size = 200;
            m_cubeLayout.update(clientSize.x - m_cubeLayout.size - m_marginx,
                m_marginy,
                m_cubeLayout.size, clientSize, dpiScale);
            LOG_INF_S("NavigationCubeManager: Initialized navigation cube position: x=" + std::to_string(m_cubeLayout.x) +
                ", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size));
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("NavigationCubeManager: Failed to initialize navigation cube: " + std::string(e.what()));
        m_canvas->showErrorDialog("Failed to initialize navigation cube.");
        m_navCube.reset();
        m_isEnabled = false;
    }
}

void NavigationCubeManager::render() {
    if (m_navCube && m_isEnabled) {
        syncCubeCameraToMain();

        float dpiScale = m_canvas->getDPIScale();
        int cubeX = static_cast<int>(m_cubeLayout.x * dpiScale);
        int cubeY = static_cast<int>(m_cubeLayout.y * dpiScale);
        m_navCube->render(cubeX, cubeY, wxSize(m_cubeLayout.size, m_cubeLayout.size));
        //LOG_DBG("NavigationCubeManager: Rendering navigation cube: x=" + std::to_string(m_cubeLayout.x) +
        //    ", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size) +
        //    ", dpiScale=" + std::to_string(dpiScale));
    }
}

bool NavigationCubeManager::handleMouseEvent(wxMouseEvent& event) {
    if (!m_navCube || !m_isEnabled) {
        return false;
    }

    float dpiScale = m_canvas->getDPIScale();
    float x = event.GetX() / dpiScale;
    float y = event.GetY() / dpiScale;

    if (x >= m_cubeLayout.x && x < (m_cubeLayout.x + m_cubeLayout.size) &&
        y >= m_cubeLayout.y && y < (m_cubeLayout.y + m_cubeLayout.size)) {

        wxMouseEvent cubeEvent(event);
        cubeEvent.m_x = static_cast<int>((x - m_cubeLayout.x) * dpiScale);
        cubeEvent.m_y = static_cast<int>((y - m_cubeLayout.y) * dpiScale);

        int scaled_cube_dimension = static_cast<int>(m_cubeLayout.size * dpiScale);
        wxSize cube_viewport_scaled_size(scaled_cube_dimension, scaled_cube_dimension);

        if (event.GetEventType() == wxEVT_LEFT_DOWN ||
            event.GetEventType() == wxEVT_LEFT_UP ||
            event.GetEventType() == wxEVT_MOTION) {
            m_navCube->handleMouseEvent(cubeEvent, cube_viewport_scaled_size);
            m_canvas->Refresh(true);
            return true; // Event handled
        }
    }
    return false; // Event not handled
}

void NavigationCubeManager::handleSizeChange() {
    wxSize size = m_canvas->GetClientSize();
    float dpiScale = m_canvas->getDPIScale();
    
    m_cubeLayout.update(size.x - m_cubeLayout.size - m_marginx,
        m_marginy,
        m_cubeLayout.size, size, dpiScale);

    if (m_navCube) {
        m_navCube->setWindowSize(size.x, size.y);
    }
}

void NavigationCubeManager::handleDPIChange() {
    auto& dpiManager = DPIManager::getInstance();
    m_marginx = dpiManager.getScaledSize(20);
    m_marginy = dpiManager.getScaledSize(20);
    
    // The cube should handle its own DPI-related texture updates.
    // We just ensure it's redrawn and its position is updated on next size event.
    m_canvas->Refresh(true);

    LOG_DBG_S("NavigationCubeManager: Handled DPI change, margins updated.");
}

void NavigationCubeManager::setEnabled(bool enabled) {
    m_isEnabled = enabled;
    if (enabled && !m_navCube) {
        initCube();
    }
    if (m_navCube) {
        m_navCube->setEnabled(enabled);
    }
    m_canvas->Refresh(true);
}

bool NavigationCubeManager::isEnabled() const {
    return m_isEnabled && m_navCube && m_navCube->isEnabled();
}

void NavigationCubeManager::setRect(int x, int y, int size) {
    if (size < 50 || x < 0 || y < 0) {
        LOG_WRN_S("NavigationCubeManager::setRect: Invalid parameters: x=" + std::to_string(x) +
            ", y=" + std::to_string(y) + ", size=" + std::to_string(size));
        return;
    }
    wxSize clientSize = m_canvas->GetClientSize();
    float dpiScale = m_canvas->getDPIScale();
    m_cubeLayout.update(x, y, size, clientSize, dpiScale);
    m_canvas->Refresh(true);
    LOG_INF_S("NavigationCubeManager::setRect: Set navigation cube rect: x=" + std::to_string(m_cubeLayout.x) +
        ", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size) +
        ", dpiScale=" + std::to_string(dpiScale));
}

void NavigationCubeManager::setColor(const wxColour& color) {
    if (!m_navCube) {
        LOG_WRN_S("NavigationCubeManager::setColor: Skipped: nav cube not created");
        return;
    }
    LOG_INF_S("NavigationCubeManager::setColor: Set navigation cube color to R=" + std::to_string(color.GetRed()) +
        ", G=" + std::to_string(color.GetGreen()) + ", B=" + std::to_string(color.GetBlue()));
    m_canvas->Refresh(true);
}

void NavigationCubeManager::setViewportSize(int size) {
    if (!m_navCube) {
        LOG_WRN_S("NavigationCubeManager::setViewportSize: Skipped: nav cube not created");
        return;
    }
    if (size < 50) {
        LOG_WRN_S("NavigationCubeManager::setViewportSize: Invalid size: " + std::to_string(size));
        return;
    }
    //LOG_INF_S("NavigationCubeManager::setViewportSize: Set navigation cube viewport size to " + std::to_string(size));
    m_canvas->Refresh(true);
}

void NavigationCubeManager::syncCubeCameraToMain() {
    if (!m_navCube || !m_sceneManager) {
        LOG_WRN_S("NavigationCubeManager::syncCubeCameraToMain: Skipped: components missing");
        return;
    }

    SoCamera* mainCamera = m_sceneManager->getCamera();
    if (mainCamera) {
        SbRotation mainOrient = mainCamera->orientation.getValue();
        float navDistance = 5.0f; 
        SbVec3f srcVec(0, 0, -1);
        SbVec3f mainCamViewVector;
        mainOrient.multVec(srcVec, mainCamViewVector);
        SbVec3f navCubeCamPos = -mainCamViewVector * navDistance;

        m_navCube->setCameraPosition(navCubeCamPos);
        m_navCube->setCameraOrientation(mainOrient);

        //LOG_DBG("NavigationCubeManager::syncCubeCameraToMain: Synced navigation cube camera based on main camera orientation.");
    }
}

void NavigationCubeManager::syncMainCameraToCube() {
    if (!m_navCube || !m_sceneManager) {
        LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: Skipped: components missing");
        return;
    }

    SoCamera* navCamera = m_navCube->getCamera();
    if (!navCamera) {
        LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: Navigation cube camera is null.");
        return;
    }

    SoCamera* mainCamera = m_sceneManager->getCamera();
    if (!mainCamera) {
        LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: Main scene camera is null.");
        return;
    }

    SbVec3f mainCamCurrentPos = mainCamera->position.getValue();
    float mainCamDistanceToOrigin = mainCamCurrentPos.length();

    if (mainCamDistanceToOrigin < 1e-3) {
        mainCamDistanceToOrigin = 10.0f;
        LOG_DBG_S("NavigationCubeManager::syncMainCameraToCube: Main camera at origin, using default distance for orbit.");
    }

    SbVec3f navCamPos = navCamera->position.getValue();
    SbRotation navCamOrient = navCamera->orientation.getValue();

    SbVec3f newMainCamDir = navCamPos;
    if (newMainCamDir.normalize() == 0.0f) {
         LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: NavCam position is origin, cannot determine direction.");
        return;
    }
    
    SbVec3f newMainCamPos = newMainCamDir * mainCamDistanceToOrigin;
    mainCamera->position.setValue(newMainCamPos);
    mainCamera->orientation.setValue(navCamOrient);

    //LOG_DBG("NavigationCubeManager::syncMainCameraToCube: Synced main camera to orbit origin, pos: (" +
    //    std::to_string(newMainCamPos[0]) + ", " + std::to_string(newMainCamPos[1]) + ", " + std::to_string(newMainCamPos[2]) +
    //    "), dist: " + std::to_string(mainCamDistanceToOrigin));
}

void NavigationCubeManager::showConfigDialog() {
    wxSize clientSize = m_canvas->GetClientSize();
    float dpiScale = m_canvas->getDPIScale();
    int maxX = static_cast<int>(clientSize.x / dpiScale);
    int maxY = static_cast<int>(clientSize.y / dpiScale);

    NavigationCubeConfigDialog dialog(m_canvas, m_cubeLayout.x, m_cubeLayout.y, m_cubeLayout.size, m_cubeLayout.size, wxColour(255, 255, 255), maxX, maxY);
    if (dialog.ShowModal() == wxID_OK) {
        int newX = dialog.GetX();
        int newY = dialog.GetY();
        int newSize = dialog.GetSize();
        int newViewportSize = dialog.GetViewportSize();
        wxColour newColor = dialog.GetColor();

        setRect(newX, newY, newSize);
        setViewportSize(newViewportSize);
        setColor(newColor);
        LOG_INF_S("NavigationCubeManager::showConfigDialog: Applied new navigation cube settings: x=" +
            std::to_string(newX) + ", y=" + std::to_string(newY) + ", size=" + std::to_string(newSize) +
            ", viewportSize=" + std::to_string(newViewportSize));
    }
} 
