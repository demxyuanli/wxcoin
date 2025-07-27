#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include <memory>
#include <Inventor/nodes/SoSeparator.h>

// Custom deleter for singleton objects (does nothing)
struct SingletonDeleter {
    template<typename T>
    void operator()(T*) const {
        // Do nothing - singleton objects manage their own lifetime
    }
};

namespace RenderingToolkitAPI {
    
    static bool s_initialized = false;
    static std::unique_ptr<RenderManager, SingletonDeleter> s_manager;
    static std::unique_ptr<RenderPluginManager, SingletonDeleter> s_pluginManager;
    
    bool initialize(const std::string& config) {
        if (s_initialized) {
            LOG_WRN_S("Rendering toolkit already initialized");
            return true;
        }
        
        try {
            // Create managers using singleton instances
            s_manager = std::unique_ptr<RenderManager, SingletonDeleter>(&RenderManager::getInstance());
            s_pluginManager = std::unique_ptr<RenderPluginManager, SingletonDeleter>(&RenderPluginManager::getInstance());
            
            // Register default components
            auto opencascadeProcessor = std::make_unique<OpenCASCADEProcessor>();
            s_manager->registerGeometryProcessor("OpenCASCADE", std::move(opencascadeProcessor));
            
            auto coin3dBackend = std::make_unique<Coin3DBackendImpl>();
            s_manager->registerRenderBackend("Coin3D", std::move(coin3dBackend));
            
            // Set defaults
            s_manager->setDefaultGeometryProcessor("OpenCASCADE");
            s_manager->setDefaultRenderBackend("Coin3D");
            
            s_initialized = true;
            LOG_INF_S("Rendering toolkit initialized successfully");
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERR_S("Failed to initialize rendering toolkit: " + std::string(e.what()));
            return false;
        }
    }
    
    void shutdown() {
        if (!s_initialized) {
            return;
        }
        
        s_manager.reset();
        s_pluginManager.reset();
        s_initialized = false;
        
        LOG_INF_S("Rendering toolkit shutdown complete");
    }
    
    RenderManager& getManager() {
        if (!s_initialized || !s_manager) {
            throw std::runtime_error("Rendering toolkit not initialized");
        }
        return *s_manager;
    }
    
    RenderConfig& getConfig() {
        return RenderConfig::getInstance();
    }
    
    RenderPluginManager& getPluginManager() {
        if (!s_initialized || !s_pluginManager) {
            throw std::runtime_error("Rendering toolkit not initialized");
        }
        return *s_pluginManager;
    }
    
    SoSeparatorPtr createSceneNode(const TriangleMesh& mesh, 
                                  bool selected,
                                  const std::string& backendName) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return nullptr;
        }
        
        auto backend = s_manager->getRenderBackend(backendName);
        if (!backend) {
            LOG_ERR_S("Rendering backend not found: " + backendName);
            return nullptr;
        }
        
        // Use default material properties when no custom material is specified
        Quantity_Color defaultDiffuse(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        Quantity_Color defaultAmbient(0.2, 0.2, 0.2, Quantity_TOC_RGB);
        Quantity_Color defaultSpecular(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        Quantity_Color defaultEmissive(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        return backend->createSceneNode(mesh, selected, defaultDiffuse, defaultAmbient, defaultSpecular, defaultEmissive, 0.5, 0.0);
    }
    
    SoSeparatorPtr createSceneNode(const TopoDS_Shape& shape,
                                  const MeshParameters& params,
                                  bool selected,
                                  const std::string& processorName,
                                  const std::string& backendName) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return nullptr;
        }
        
        auto backend = s_manager->getRenderBackend(backendName);
        if (!backend) {
            LOG_ERR_S("Rendering backend not found: " + backendName);
            return nullptr;
        }
        
        return backend->createSceneNode(shape, params, selected);
    }
    
    int loadPlugins(const std::string& directory) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return 0;
        }
        
        return s_pluginManager->loadPluginsFromDirectory(directory);
    }
    
    std::vector<std::string> getAvailableGeometryProcessors() {
        if (!s_initialized) {
            return {};
        }
        
        return s_manager->getAvailableGeometryProcessors();
    }
    
    std::vector<std::string> getAvailableRenderBackends() {
        if (!s_initialized) {
            return {};
        }
        
        return s_manager->getAvailableRenderBackends();
    }
    
    std::string getVersion() {
        return "1.0.0";
    }
    
    bool isInitialized() {
        return s_initialized;
    }

    // Culling system methods
    void updateCulling(const void* camera) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return;
        }
        
        s_manager->updateCulling(camera);
    }

    bool shouldRenderShape(const TopoDS_Shape& shape) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return true;
        }
        
        return s_manager->shouldRenderShape(shape);
    }

    void addOccluder(const TopoDS_Shape& shape, void* sceneNode) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return;
        }
        
        s_manager->addOccluder(shape, static_cast<SoSeparator*>(sceneNode));
    }

    void removeOccluder(const TopoDS_Shape& shape) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return;
        }
        
        s_manager->removeOccluder(shape);
    }

    void setFrustumCullingEnabled(bool enabled) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return;
        }
        
        s_manager->setFrustumCullingEnabled(enabled);
    }

    void setOcclusionCullingEnabled(bool enabled) {
        if (!s_initialized) {
            LOG_ERR_S("Rendering toolkit not initialized");
            return;
        }
        
        s_manager->setOcclusionCullingEnabled(enabled);
    }

    std::string getCullingStats() {
        if (!s_initialized) {
            return "Rendering toolkit not initialized";
        }
        
        return s_manager->getCullingStats();
    }
} 