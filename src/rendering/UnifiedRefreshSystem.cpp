#include "UnifiedRefreshSystem.h"
#include "Canvas.h"
#include "OCCViewer.h"
#include "SceneManager.h"
#include "CommandType.h"
#include "logger/Logger.h"

UnifiedRefreshSystem::UnifiedRefreshSystem(Canvas* canvas, OCCViewer* occViewer, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_occViewer(occViewer)
    , m_sceneManager(sceneManager)
    , m_commandDispatcher(nullptr)
    , m_initialized(false)
{
    // Create refresh command listener only if we have the required components
    // If any component is nullptr, we'll create the listener later via setComponents()
    if (canvas && sceneManager) {
        m_refreshListener = std::make_shared<RefreshCommandListener>(canvas, occViewer, sceneManager);
        LOG_INF_S("UnifiedRefreshSystem created with all components");
    } else {
        LOG_INF_S("UnifiedRefreshSystem created, components will be set later");
    }
}

UnifiedRefreshSystem::~UnifiedRefreshSystem()
{
    shutdown();
    try {
        // Avoid logging during shutdown to prevent crashes
        std::cout << "UnifiedRefreshSystem destroyed" << std::endl;
    } catch (...) {
        // Ignore any exceptions during shutdown
    }
}

void UnifiedRefreshSystem::initialize(CommandDispatcher* commandDispatcher)
{
    if (m_initialized) {
        LOG_WRN_S("UnifiedRefreshSystem already initialized");
        return;
    }

    if (!commandDispatcher) {
        LOG_ERR_S("UnifiedRefreshSystem: Command dispatcher is null");
        return;
    }

    m_commandDispatcher = commandDispatcher;

    // Register refresh command listener for all refresh command types if available
    if (m_refreshListener) {
        try {
            m_commandDispatcher->registerListener(cmd::CommandType::RefreshView, m_refreshListener);
            m_commandDispatcher->registerListener(cmd::CommandType::RefreshScene, m_refreshListener);
            m_commandDispatcher->registerListener(cmd::CommandType::RefreshObject, m_refreshListener);
            m_commandDispatcher->registerListener(cmd::CommandType::RefreshMaterial, m_refreshListener);
            m_commandDispatcher->registerListener(cmd::CommandType::RefreshGeometry, m_refreshListener);
            m_commandDispatcher->registerListener(cmd::CommandType::RefreshUI, m_refreshListener);
            LOG_INF_S("UnifiedRefreshSystem: Refresh listeners registered");
        } catch (...) {
            // Handle potential static map access issues during shutdown
            std::cout << "UnifiedRefreshSystem: Exception during listener registration (ignored)" << std::endl;
        }
    } else {
        LOG_INF_S("UnifiedRefreshSystem: Refresh listener not available yet, will register when components are set");
    }

    // Set command dispatcher in view refresh manager if available
    if (m_canvas && m_canvas->getRefreshManager()) {
        m_canvas->getRefreshManager()->setCommandDispatcher(commandDispatcher);
    }

    m_initialized = true;
    LOG_INF_S("UnifiedRefreshSystem initialized successfully");
}

void UnifiedRefreshSystem::shutdown()
{
    if (!m_initialized || !m_commandDispatcher) {
        return;
    }

    try {
        // Unregister all refresh command listeners
        // Use try-catch to handle potential static object destruction issues
        if (m_refreshListener) {
            // Use string literals directly to avoid accessing static maps during shutdown
            m_commandDispatcher->unregisterListener("REFRESH_VIEW", m_refreshListener);
            m_commandDispatcher->unregisterListener("REFRESH_SCENE", m_refreshListener);
            m_commandDispatcher->unregisterListener("REFRESH_OBJECT", m_refreshListener);
            m_commandDispatcher->unregisterListener("REFRESH_MATERIAL", m_refreshListener);
            m_commandDispatcher->unregisterListener("REFRESH_GEOMETRY", m_refreshListener);
            m_commandDispatcher->unregisterListener("REFRESH_UI", m_refreshListener);
        }
    } catch (...) {
        // Ignore exceptions during shutdown - static objects might be destroyed
        std::cout << "UnifiedRefreshSystem: Exception during listener unregistration (ignored)" << std::endl;
    }

    m_commandDispatcher = nullptr;
    m_initialized = false;
    
    try {
        LOG_INF_S("UnifiedRefreshSystem shutdown completed");
    } catch (...) {
        // Avoid logging during shutdown to prevent crashes
        std::cout << "UnifiedRefreshSystem shutdown completed" << std::endl;
    }
}

void UnifiedRefreshSystem::setOCCViewer(OCCViewer* occViewer)
{
    m_occViewer = occViewer;
    
    // Update the refresh listener with the new OCCViewer
    if (m_refreshListener) {
        m_refreshListener->setOCCViewer(occViewer);
    }
    
    LOG_INF_S("UnifiedRefreshSystem: OCCViewer updated");
}

void UnifiedRefreshSystem::setComponents(Canvas* canvas, OCCViewer* occViewer, SceneManager* sceneManager)
{
    m_canvas = canvas;
    m_occViewer = occViewer;
    m_sceneManager = sceneManager;
    
    // Create or update refresh listener if we have required components
    if (canvas && sceneManager) {
        if (!m_refreshListener) {
            m_refreshListener = std::make_shared<RefreshCommandListener>(canvas, occViewer, sceneManager);
            LOG_INF_S("UnifiedRefreshSystem: Refresh listener created");
            
            // Re-register with command dispatcher if already initialized
            if (m_initialized && m_commandDispatcher) {
                try {
                    m_commandDispatcher->registerListener(cmd::CommandType::RefreshView, m_refreshListener);
                    m_commandDispatcher->registerListener(cmd::CommandType::RefreshScene, m_refreshListener);
                    m_commandDispatcher->registerListener(cmd::CommandType::RefreshObject, m_refreshListener);
                    m_commandDispatcher->registerListener(cmd::CommandType::RefreshMaterial, m_refreshListener);
                    m_commandDispatcher->registerListener(cmd::CommandType::RefreshGeometry, m_refreshListener);
                    m_commandDispatcher->registerListener(cmd::CommandType::RefreshUI, m_refreshListener);
                    LOG_INF_S("UnifiedRefreshSystem: Refresh listener registered with command dispatcher");
                } catch (...) {
                    // Handle potential static map access issues during shutdown
                    std::cout << "UnifiedRefreshSystem: Exception during listener re-registration (ignored)" << std::endl;
                }
            }
        } else {
            // Update existing listener
            m_refreshListener->setOCCViewer(occViewer);
        }
    }
    
    LOG_INF_S("UnifiedRefreshSystem: Components updated");
}

void UnifiedRefreshSystem::setCanvas(Canvas* canvas)
{
    m_canvas = canvas;
    if (m_refreshListener) {
        // Note: RefreshCommandListener doesn't have setCanvas method,
        // so we need to recreate the listener if needed
        if (m_sceneManager) {
            setComponents(canvas, m_occViewer, m_sceneManager);
        }
    }
}

void UnifiedRefreshSystem::setSceneManager(SceneManager* sceneManager)
{
    m_sceneManager = sceneManager;
    if (m_refreshListener) {
        // Note: RefreshCommandListener doesn't have setSceneManager method,
        // so we need to recreate the listener if needed
        if (m_canvas) {
            setComponents(m_canvas, m_occViewer, sceneManager);
        }
    }
}

void UnifiedRefreshSystem::refreshView(const std::string& objectId, bool immediate)
{
    if (!m_initialized || !m_commandDispatcher) {
        LOG_WRN_S("UnifiedRefreshSystem not initialized, using direct refresh");
        directRefreshView();
        return;
    }

    try {
        auto params = createRefreshParams(objectId, "", immediate);
        auto result = m_commandDispatcher->dispatchCommand(cmd::CommandType::RefreshView, params);
        
        if (!result.success) {
            LOG_WRN_S("Failed to dispatch RefreshView command: " + result.message);
            directRefreshView();
        }
    } catch (...) {
        // Handle potential static map access issues during shutdown
        LOG_WRN_S("Exception during refreshView command dispatch, using direct refresh");
        directRefreshView();
    }
}

void UnifiedRefreshSystem::refreshScene(const std::string& objectId, bool immediate)
{
    if (!m_initialized || !m_commandDispatcher) {
        LOG_WRN_S("UnifiedRefreshSystem not initialized, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::SCENE_CHANGED);
        return;
    }

    try {
        auto params = createRefreshParams(objectId, "", immediate);
        auto result = m_commandDispatcher->dispatchCommand(cmd::CommandType::RefreshScene, params);
        
        if (!result.success) {
            LOG_WRN_S("Failed to dispatch RefreshScene command: " + result.message);
            directRefreshView(ViewRefreshManager::RefreshReason::SCENE_CHANGED);
        }
    } catch (...) {
        // Handle potential static map access issues during shutdown
        LOG_WRN_S("Exception during refreshScene command dispatch, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::SCENE_CHANGED);
    }
}

void UnifiedRefreshSystem::refreshObject(const std::string& objectId, bool immediate)
{
    if (!m_initialized || !m_commandDispatcher) {
        LOG_WRN_S("UnifiedRefreshSystem not initialized, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::OBJECT_CHANGED);
        return;
    }

    try {
        auto params = createRefreshParams(objectId, "", immediate);
        auto result = m_commandDispatcher->dispatchCommand(cmd::CommandType::RefreshObject, params);
        
        if (!result.success) {
            LOG_WRN_S("Failed to dispatch RefreshObject command: " + result.message);
            directRefreshView(ViewRefreshManager::RefreshReason::OBJECT_CHANGED);
    }
    } catch (...) {
        // Handle potential static map access issues during shutdown
        LOG_WRN_S("Exception during refreshObject command dispatch, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::OBJECT_CHANGED);
    }
}

void UnifiedRefreshSystem::refreshMaterial(const std::string& objectId, bool immediate)
{
    if (!m_initialized || !m_commandDispatcher) {
        LOG_WRN_S("UnifiedRefreshSystem not initialized, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED);
        return;
    }

    try {
        auto params = createRefreshParams(objectId, "material", immediate);
        auto result = m_commandDispatcher->dispatchCommand(cmd::CommandType::RefreshMaterial, params);
        
        if (!result.success) {
            LOG_WRN_S("Failed to dispatch RefreshMaterial command: " + result.message);
            directRefreshView(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED);
        }
    } catch (...) {
        // Handle potential static map access issues during shutdown
        LOG_WRN_S("Exception during refreshMaterial command dispatch, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED);
    }
}

void UnifiedRefreshSystem::refreshGeometry(const std::string& objectId, bool immediate)
{
    if (!m_initialized || !m_commandDispatcher) {
        LOG_WRN_S("UnifiedRefreshSystem not initialized, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED);
        return;
    }

    try {
        auto params = createRefreshParams(objectId, "geometry", immediate);
        auto result = m_commandDispatcher->dispatchCommand(cmd::CommandType::RefreshGeometry, params);
        
        if (!result.success) {
            LOG_WRN_S("Failed to dispatch RefreshGeometry command: " + result.message);
            directRefreshView(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED);
        }
    } catch (...) {
        // Handle potential static map access issues during shutdown
        LOG_WRN_S("Exception during refreshGeometry command dispatch, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED);
    }
}

void UnifiedRefreshSystem::refreshUI(const std::string& componentType, bool immediate)
{
    if (!m_initialized || !m_commandDispatcher) {
        LOG_WRN_S("UnifiedRefreshSystem not initialized, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::UI_CHANGED);
        return;
    }

    try {
        auto params = createRefreshParams("", componentType, immediate);
        auto result = m_commandDispatcher->dispatchCommand(cmd::CommandType::RefreshUI, params);
        
        if (!result.success) {
            LOG_WRN_S("Failed to dispatch RefreshUI command: " + result.message);
            directRefreshView(ViewRefreshManager::RefreshReason::UI_CHANGED);
        }
    } catch (...) {
        // Handle potential static map access issues during shutdown
        LOG_WRN_S("Exception during refreshUI command dispatch, using direct refresh");
        directRefreshView(ViewRefreshManager::RefreshReason::UI_CHANGED);
    }
}

void UnifiedRefreshSystem::directRefreshView(ViewRefreshManager::RefreshReason reason)
{
    if (m_canvas && m_canvas->getRefreshManager()) {
        m_canvas->getRefreshManager()->requestRefresh(reason, true);
    } else if (m_canvas) {
        m_canvas->Refresh();
    } else {
        LOG_WRN_S("UnifiedRefreshSystem: No canvas available for direct refresh");
    }
}

void UnifiedRefreshSystem::directRefreshAll()
{
    // Refresh scene bounds
    if (m_sceneManager) {
        m_sceneManager->updateSceneBounds();
    }

    // Refresh all geometries
    if (m_occViewer) {
        m_occViewer->remeshAllGeometries();
    }

    // Refresh view
    directRefreshView(ViewRefreshManager::RefreshReason::MANUAL_REQUEST);

    LOG_INF_S("UnifiedRefreshSystem: Direct refresh all completed");
}

std::unordered_map<std::string, std::string> UnifiedRefreshSystem::createRefreshParams(
    const std::string& objectId, const std::string& componentType, bool immediate)
{
    std::unordered_map<std::string, std::string> params;
    
    if (!objectId.empty()) {
        params["objectId"] = objectId;
    }
    
    if (!componentType.empty()) {
        params["componentType"] = componentType;
    }
    
    if (immediate) {
        params["immediate"] = "true";
    }
    
    return params;
} 