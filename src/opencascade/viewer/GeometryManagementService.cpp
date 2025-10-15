#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/GeometryManagementService.h"
#include "SceneManager.h"
#include "viewer/GeometryRepository.h"
#include "viewer/SceneAttachmentService.h"
#include "viewer/ObjectTreeSync.h"
#include "viewer/SelectionManager.h"
#include "viewer/ViewUpdateService.h"
#include "Canvas.h"
#include "ObjectTreePanel.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>

GeometryManagementService::GeometryManagementService(
    SceneManager* sceneManager,
    std::vector<std::shared_ptr<OCCGeometry>>* geometries,
    std::vector<std::shared_ptr<OCCGeometry>>* selectedGeometries
)
    : m_sceneManager(sceneManager)
    , m_geometries(geometries)
    , m_selectedGeometries(selectedGeometries)
    , m_geometryRepo(nullptr)
    , m_sceneAttach(nullptr)
    , m_objectTreeSync(nullptr)
    , m_selectionManager(nullptr)
    , m_viewUpdater(nullptr)
    , m_batchMode(false)
{
}

GeometryManagementService::~GeometryManagementService()
{
}

void GeometryManagementService::setServices(
    GeometryRepository* repo,
    SceneAttachmentService* attach,
    ObjectTreeSync* treeSync,
    SelectionManager* selectionMgr,
    ViewUpdateService* viewUpdater
) {
    m_geometryRepo = repo;
    m_sceneAttach = attach;
    m_objectTreeSync = treeSync;
    m_selectionManager = selectionMgr;
    m_viewUpdater = viewUpdater;
}

bool GeometryManagementService::addGeometry(std::shared_ptr<OCCGeometry> geometry, bool batchMode) {
    if (!geometry) {
        LOG_ERR_S("Attempted to add null geometry to GeometryManagementService");
        return false;
    }

    // Check for existing geometry
    if (m_geometryRepo && m_geometryRepo->existsByName(geometry->getName())) {
        LOG_WRN_S("Geometry with name '" + geometry->getName() + "' already exists");
        return false;
    }

    // Store geometry
    if (m_geometryRepo) {
        m_geometryRepo->add(geometry);
    }
    m_geometries->push_back(geometry);

    // Attach to scene
    attachGeometryToScene(geometry);

    // Update object tree
    if (m_objectTreeSync) {
        m_objectTreeSync->addGeometry(geometry, batchMode || m_batchMode);
    }

    // Rebuild selection accelerator
    rebuildSelectionAccelerator();

    LOG_INF_S("Added geometry: " + geometry->getName());
    return true;
}

bool GeometryManagementService::removeGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry || !m_geometries) return false;

    // First verify the geometry exists in the list
    auto it = std::find(m_geometries->begin(), m_geometries->end(), geometry);
    if (it == m_geometries->end()) {
        LOG_WRN_S("Geometry not found: " + geometry->getName());
        return false;
    }

    // Save the geometry name for logging
    std::string geomName = geometry->getName();

    // Remove from selected geometries if present
    if (m_selectedGeometries) {
        auto selectedIt = std::find(m_selectedGeometries->begin(), m_selectedGeometries->end(), geometry);
        if (selectedIt != m_selectedGeometries->end()) {
            m_selectedGeometries->erase(selectedIt);
        }
    }

    // Detach from scene BEFORE erasing from vector
    detachGeometryFromScene(geometry);

    // Remove from object tree BEFORE erasing from vector
    if (m_objectTreeSync) {
        m_objectTreeSync->removeGeometry(geometry);
    }

    // Remove from repository BEFORE erasing from vector
    if (m_geometryRepo) {
        m_geometryRepo->remove(geometry);
    }

    // NOW safe to remove from main list - re-find iterator to be absolutely sure it's valid
    it = std::find(m_geometries->begin(), m_geometries->end(), geometry);
    if (it != m_geometries->end()) {
        m_geometries->erase(it);
    } else {
        LOG_ERR_S("Iterator became invalid during removal operations");
        return false;
    }

    // Rebuild selection accelerator
    rebuildSelectionAccelerator();

    LOG_INF_S("Removed geometry: " + geomName);
    return true;
}

bool GeometryManagementService::removeGeometry(const std::string& name) {
    auto geometry = findGeometry(name);
    if (geometry) {
        return removeGeometry(geometry);
    }
    return false;
}

void GeometryManagementService::clearAll() {
    m_selectedGeometries->clear();

    if (m_geometryRepo) {
        m_geometryRepo->clear();
    }

    if (m_sceneAttach) {
        m_sceneAttach->detachAll();
    }

    // Clear selection accelerator
    rebuildSelectionAccelerator();

    LOG_INF_S("Cleared all geometries");
}

std::shared_ptr<OCCGeometry> GeometryManagementService::findGeometry(const std::string& name) const {
    if (m_geometryRepo) {
        return m_geometryRepo->findByName(name);
    }
    return nullptr;
}

const std::vector<std::shared_ptr<OCCGeometry>>& GeometryManagementService::getAllGeometries() const {
    return *m_geometries;
}

const std::vector<std::shared_ptr<OCCGeometry>>& GeometryManagementService::getSelectedGeometries() const {
    return *m_selectedGeometries;
}

void GeometryManagementService::setGeometryVisible(const std::string& name, bool visible) {
    if (m_selectionManager) {
        m_selectionManager->setGeometryVisible(name, visible);
    }

    // Ensure scene attachment reflects visibility
    auto geometry = findGeometry(name);
    if (geometry) {
        SoSeparator* coinNode = geometry->getCoinNode();
        if (coinNode && m_sceneManager && m_sceneManager->getObjectRoot()) {
            SoSeparator* root = m_sceneManager->getObjectRoot();
            int idx = root->findChild(coinNode);
            if (visible) {
                if (idx < 0) {
                    root->addChild(coinNode);
                }
            } else {
                if (idx >= 0) {
                    root->removeChild(idx);
                }
            }
        }

        // Update object tree display name
        updateGeometryInTree(geometry);
    }

    if (m_viewUpdater) {
        m_viewUpdater->requestGeometryChanged(true);
    }
}

void GeometryManagementService::setGeometrySelected(const std::string& name, bool selected) {
    if (m_selectionManager) {
        m_selectionManager->setGeometrySelected(name, selected);
    }
}

void GeometryManagementService::setGeometryColor(const std::string& name, const Quantity_Color& color) {
    if (m_selectionManager) {
        m_selectionManager->setGeometryColor(name, color);
    }
}

void GeometryManagementService::setGeometryTransparency(const std::string& name, double transparency) {
    if (m_selectionManager) {
        m_selectionManager->setGeometryTransparency(name, transparency);
    }
}

void GeometryManagementService::addGeometriesBatch(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
    if (geometries.empty()) return;

    LOG_INF_S("Starting batch addition of " + std::to_string(geometries.size()) + " geometries");

    // Pre-allocate space
    m_geometries->reserve(m_geometries->size() + geometries.size());

    for (const auto& geometry : geometries) {
        if (!geometry) {
            LOG_ERR_S("Attempted to add null geometry in batch operation");
            continue;
        }

        // Quick validation
        auto it = std::find_if(m_geometries->begin(), m_geometries->end(),
            [&](const std::shared_ptr<OCCGeometry>& g) {
                return g->getName() == geometry->getName();
            });

        if (it != m_geometries->end()) {
            LOG_WRN_S("Geometry with name '" + geometry->getName() + "' already exists (skipping in batch)");
            continue;
        }

        // Store geometry
        m_geometries->push_back(geometry);
        if (m_geometryRepo) {
            m_geometryRepo->add(geometry);
        }

        // Add to object tree sync (batch mode)
        if (m_objectTreeSync) {
            m_objectTreeSync->addGeometry(geometry, true); // true = batch mode
        }

        // Collect Coin3D node for attachment
        SoSeparator* coinNode = geometry->getCoinNode();
        if (coinNode && m_sceneAttach) {
            m_sceneAttach->attach(geometry);
        }
    }

    // Rebuild selection accelerator
    rebuildSelectionAccelerator();

    LOG_INF_S("Batch geometry addition completed");
}

void GeometryManagementService::attachGeometryToScene(std::shared_ptr<OCCGeometry> geometry) {
    if (geometry && m_sceneAttach) {
        m_sceneAttach->attach(geometry);
    }
}

void GeometryManagementService::detachGeometryFromScene(std::shared_ptr<OCCGeometry> geometry) {
    if (geometry && m_sceneAttach) {
        m_sceneAttach->detach(geometry);
    }
}

void GeometryManagementService::updateGeometryInTree(std::shared_ptr<OCCGeometry> geometry) {
    if (geometry && m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        if (canvas && canvas->getObjectTreePanel()) {
            canvas->getObjectTreePanel()->updateOCCGeometryName(geometry);
        }
    }
}

void GeometryManagementService::rebuildSelectionAccelerator() {
    // This will be handled by the parent OCCViewer through SelectionAcceleratorService
    // We just provide a hook for it
}
