#include "RefreshCommand.h"
#include "Canvas.h"
#include "OCCViewer.h"
#include "SceneManager.h"
#include "ViewRefreshManager.h"
#include "ObjectTreePanel.h"
#include "OCCMeshConverter.h"
#include "logger/Logger.h"

// RefreshCommand base class implementation
RefreshCommand::RefreshCommand(cmd::CommandType type, const RefreshTarget& target)
    : m_commandType(type), m_target(target)
{
}

std::string RefreshCommand::getDescription() const
{
    std::string desc = "Refresh ";
    switch (m_commandType) {
        case cmd::CommandType::RefreshView:
            desc += "View";
            break;
        case cmd::CommandType::RefreshScene:
            desc += "Scene";
            break;
        case cmd::CommandType::RefreshObject:
            desc += "Object";
            break;
        case cmd::CommandType::RefreshMaterial:
            desc += "Material";
            break;
        case cmd::CommandType::RefreshGeometry:
            desc += "Geometry";
            break;
        case cmd::CommandType::RefreshUI:
            desc += "UI";
            break;
        default:
            desc += "Unknown";
            break;
    }
    
    if (!m_target.objectId.empty()) {
        desc += " (" + m_target.objectId + ")";
    }
    
    return desc;
}

// RefreshViewCommand implementation
RefreshViewCommand::RefreshViewCommand(const RefreshTarget& target)
    : RefreshCommand(cmd::CommandType::RefreshView, target), m_canvas(nullptr)
{
}

void RefreshViewCommand::execute()
{
    if (!m_canvas) {
        LOG_WRN_S("RefreshViewCommand: Canvas is null");
        return;
    }
    
    LOG_INF_S("Executing RefreshViewCommand" + 
              (m_target.objectId.empty() ? "" : " for object: " + m_target.objectId));
    
    ViewRefreshManager* refreshManager = m_canvas->getRefreshManager();
    if (refreshManager) {
        ViewRefreshManager::RefreshReason reason = ViewRefreshManager::RefreshReason::MANUAL_REQUEST;
        
        // Map component type to specific refresh reason
        if (m_target.componentType == "material") {
            reason = ViewRefreshManager::RefreshReason::MATERIAL_CHANGED;
        } else if (m_target.componentType == "geometry") {
            reason = ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED;
        } else if (m_target.componentType == "selection") {
            reason = ViewRefreshManager::RefreshReason::SELECTION_CHANGED;
        } else if (m_target.componentType == "camera") {
            reason = ViewRefreshManager::RefreshReason::CAMERA_MOVED;
        }
        
        refreshManager->requestRefresh(reason, m_target.immediate);
    } else {
        // Fallback to direct refresh
        m_canvas->Refresh();
        LOG_WRN_S("RefreshViewCommand: Using fallback direct refresh");
    }
}

// RefreshSceneCommand implementation
RefreshSceneCommand::RefreshSceneCommand(const RefreshTarget& target)
    : RefreshCommand(cmd::CommandType::RefreshScene, target), m_sceneManager(nullptr)
{
}

void RefreshSceneCommand::execute()
{
    if (!m_sceneManager) {
        LOG_WRN_S("RefreshSceneCommand: SceneManager is null");
        return;
    }
    
    LOG_INF_S("Executing RefreshSceneCommand" + 
              (m_target.objectId.empty() ? "" : " for object: " + m_target.objectId));
    
    // Update scene bounds
    m_sceneManager->updateSceneBounds();
    
    // Request view refresh through the refresh manager
    Canvas* canvas = m_sceneManager->getCanvas();
    if (canvas && canvas->getRefreshManager()) {
        ViewRefreshManager::RefreshReason reason = ViewRefreshManager::RefreshReason::SCENE_CHANGED;
        canvas->getRefreshManager()->requestRefresh(reason, m_target.immediate);
    }
}

// RefreshObjectCommand implementation
RefreshObjectCommand::RefreshObjectCommand(const RefreshTarget& target)
    : RefreshCommand(cmd::CommandType::RefreshObject, target), m_occViewer(nullptr)
{
}

void RefreshObjectCommand::execute()
{
    if (!m_occViewer) {
        LOG_WRN_S("RefreshObjectCommand: OCCViewer is null");
        return;
    }
    
    LOG_INF_S("Executing RefreshObjectCommand" + 
              (m_target.objectId.empty() ? "" : " for object: " + m_target.objectId));
    
    if (!m_target.objectId.empty()) {
        // Refresh specific object
        auto geometry = m_occViewer->findGeometry(m_target.objectId);
        if (geometry) {
            geometry->regenerateMesh(OCCMeshConverter::MeshParameters());
            LOG_INF_S("Refreshed object: " + m_target.objectId);
        } else {
            LOG_WRN_S("Object not found: " + m_target.objectId);
        }
    } else {
        // Refresh all objects
        m_occViewer->remeshAllGeometries();
        LOG_INF_S("Refreshed all objects");
    }
    
    // Request view refresh
    m_occViewer->requestViewRefresh();
}

// RefreshMaterialCommand implementation
RefreshMaterialCommand::RefreshMaterialCommand(const RefreshTarget& target)
    : RefreshCommand(cmd::CommandType::RefreshMaterial, target), m_occViewer(nullptr)
{
}

void RefreshMaterialCommand::execute()
{
    if (!m_occViewer) {
        LOG_WRN_S("RefreshMaterialCommand: OCCViewer is null");
        return;
    }
    
    LOG_INF_S("Executing RefreshMaterialCommand" + 
              (m_target.objectId.empty() ? "" : " for object: " + m_target.objectId));
    
    auto geometries = m_occViewer->getAllGeometry();
    for (auto& geometry : geometries) {
        if (m_target.objectId.empty() || geometry->getName() == m_target.objectId) {
            // Force material update by marking Coin representation as needing update
            geometry->getCoinNode(); // This will rebuild the representation if needed
        }
    }
    
    // Request view refresh with material change reason
    m_occViewer->requestViewRefresh();
}

// RefreshGeometryCommand implementation
RefreshGeometryCommand::RefreshGeometryCommand(const RefreshTarget& target)
    : RefreshCommand(cmd::CommandType::RefreshGeometry, target), m_occViewer(nullptr)
{
}

void RefreshGeometryCommand::execute()
{
    if (!m_occViewer) {
        LOG_WRN_S("RefreshGeometryCommand: OCCViewer is null");
        return;
    }
    
    LOG_INF_S("Executing RefreshGeometryCommand" + 
              (m_target.objectId.empty() ? "" : " for object: " + m_target.objectId));
    
    if (!m_target.objectId.empty()) {
        // Refresh specific geometry
        auto geometry = m_occViewer->findGeometry(m_target.objectId);
        if (geometry) {
            geometry->regenerateMesh(OCCMeshConverter::MeshParameters());
            LOG_INF_S("Regenerated geometry mesh: " + m_target.objectId);
        } else {
            LOG_WRN_S("Geometry not found: " + m_target.objectId);
        }
    } else {
        // Refresh all geometries
        m_occViewer->remeshAllGeometries();
        LOG_INF_S("Regenerated all geometry meshes");
    }
    
    // Request view refresh
    m_occViewer->requestViewRefresh();
}

// RefreshUICommand implementation
RefreshUICommand::RefreshUICommand(const RefreshTarget& target)
    : RefreshCommand(cmd::CommandType::RefreshUI, target), m_canvas(nullptr)
{
}

void RefreshUICommand::execute()
{
    if (!m_canvas) {
        LOG_WRN_S("RefreshUICommand: Canvas is null");
        return;
    }
    
    LOG_INF_S("Executing RefreshUICommand" + 
              (m_target.componentType.empty() ? "" : " for component: " + m_target.componentType));
    
    if (m_target.componentType == "objecttree") {
        // Refresh object tree panel
        auto* objectTreePanel = m_canvas->getObjectTreePanel();
        if (objectTreePanel) {
            objectTreePanel->updateTreeSelectionFromViewer();
            LOG_INF_S("Updated ObjectTreePanel");
        }
    } else if (m_target.componentType == "properties") {
        // Refresh property panel - currently not implemented in Canvas
        LOG_INF_S("PropertyPanel refresh requested but not available");
    } else {
        // Refresh entire UI
        m_canvas->Refresh();
        
        // Refresh child panels
        auto* objectTreePanel = m_canvas->getObjectTreePanel();
        if (objectTreePanel) {
            objectTreePanel->Refresh();
        }
        
        // PropertyPanel not currently available in Canvas
        // TODO: Add PropertyPanel support when implemented
        
        LOG_INF_S("Refreshed entire UI");
    }
}

// RefreshCommandFactory implementation
std::shared_ptr<RefreshCommand> RefreshCommandFactory::createCommand(
    cmd::CommandType type, const RefreshTarget& target)
{
    switch (type) {
        case cmd::CommandType::RefreshView:
            return std::make_shared<RefreshViewCommand>(target);
        case cmd::CommandType::RefreshScene:
            return std::make_shared<RefreshSceneCommand>(target);
        case cmd::CommandType::RefreshObject:
            return std::make_shared<RefreshObjectCommand>(target);
        case cmd::CommandType::RefreshMaterial:
            return std::make_shared<RefreshMaterialCommand>(target);
        case cmd::CommandType::RefreshGeometry:
            return std::make_shared<RefreshGeometryCommand>(target);
        case cmd::CommandType::RefreshUI:
            return std::make_shared<RefreshUICommand>(target);
        default:
            LOG_WRN_S("Unknown refresh command type");
            return nullptr;
    }
}

std::shared_ptr<RefreshCommand> RefreshCommandFactory::createCommand(
    const std::string& commandString, const std::unordered_map<std::string, std::string>& parameters)
{
    try {
        cmd::CommandType type = cmd::from_string(commandString);
        RefreshTarget target = parseTarget(parameters);
        return createCommand(type, target);
    } catch (...) {
        // Handle potential static map access issues during shutdown
        LOG_WRN_S("Static map access error during command creation");
        return nullptr;
    }
}

RefreshTarget RefreshCommandFactory::parseTarget(const std::unordered_map<std::string, std::string>& parameters)
{
    RefreshTarget target;
    
    auto objectIdIt = parameters.find("objectId");
    if (objectIdIt != parameters.end()) {
        target.objectId = objectIdIt->second;
    }
    
    auto componentTypeIt = parameters.find("componentType");
    if (componentTypeIt != parameters.end()) {
        target.componentType = componentTypeIt->second;
    }
    
    auto immediateIt = parameters.find("immediate");
    if (immediateIt != parameters.end()) {
        target.immediate = (immediateIt->second == "true" || immediateIt->second == "1");
    }
    
    return target;
} 