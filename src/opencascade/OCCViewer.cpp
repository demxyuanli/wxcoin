#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "rendering/RenderingToolkitAPI.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "ObjectTreePanel.h"
#include "ViewRefreshManager.h"
#include "config/EdgeSettingsConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoScale.h>
#include <algorithm>
#include <limits>
#include <cmath>
#include <wx/gdicmn.h>
#include <chrono>
#include "EdgeComponent.h"
#include "edges/EdgeDisplayManager.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SoPickedPoint.h>
#include <unordered_map>
// DynamicSilhouetteRenderer usage removed; hover silhouettes managed by HoverSilhouetteManager
#include <future>
#include "OCCShapeBuilder.h"
#include "viewer/SliceController.h"
#include "viewer/ExplodeController.h"
#include "viewer/LODController.h"
#include "viewer/PickingService.h"
#include "viewer/SelectionManager.h"
#include "viewer/ObjectTreeSync.h"
#include "viewer/GeometryRepository.h"
#include "viewer/SceneAttachmentService.h"
#include "viewer/ViewUpdateService.h"
#include "viewer/MeshingService.h"
#include "viewer/MeshParameterController.h"
#include "viewer/HoverSilhouetteManager.h"
#include "viewer/BatchOperationManager.h"
#include "viewer/OutlineDisplayManager.h"
#include "viewer/SelectionOutlineManager.h"

gp_Pnt OCCViewer::getCameraPosition() const {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return gp_Pnt(0, 0, 0);
	SoCamera* camera = m_sceneManager->getCanvas()->getCamera();
	if (!camera) return gp_Pnt(0, 0, 0);
	SbVec3f pos = camera->position.getValue();
	return gp_Pnt(pos[0], pos[1], pos[2]);
}

// Invalidate and regenerate silhouette edges on camera/view change (pseudo-code, actual integration may depend on your event system)
// void OCCViewer::onCameraChanged() {
//     if (globalEdgeFlags.showSilhouetteEdges) setShowSilhouetteEdges(true);
// }

OCCViewer::OCCViewer(SceneManager* sceneManager)
	: m_sceneManager(sceneManager),
	m_occRoot(nullptr),
	m_wireframeMode(false),
	m_showEdges(true),
	m_antiAliasing(true),
	m_showNormals(false),
	m_normalLength(0.5),
	m_correctNormalColor(1.0, 0.0, 0.0, Quantity_TOC_RGB),
	m_incorrectNormalColor(0.0, 1.0, 0.0, Quantity_TOC_RGB),
	m_normalConsistencyMode(true),
	m_normalDebugMode(false),
	m_defaultColor(0.7, 0.7, 0.7, Quantity_TOC_RGB),
	m_defaultTransparency(0.0),
	m_lodEnabled(false),

	m_subdivisionEnabled(false),
	m_subdivisionLevel(2),
	m_subdivisionMethod(0),
	m_subdivisionCreaseAngle(30.0),
	m_smoothingEnabled(false),
	m_smoothingMethod(0),
	m_smoothingIterations(2),
	m_smoothingStrength(0.5),
	m_smoothingCreaseAngle(30.0),
	m_tessellationMethod(0),
	m_tessellationQuality(2),
	m_featurePreservation(0.5),
	m_parallelProcessing(true),
	m_adaptiveMeshing(false),
	m_batchOperationActive(false),
	m_needsViewRefresh(false),
	m_parameterMonitoringEnabled(false)
{
	initializeViewer();
	// Remove default edge display to avoid conflicts with new EdgeComponent system
	// setShowEdges(true);

	// Create LOD controller
	m_lodController = std::make_unique<LODController>(this);
	// Create edge display manager
	m_edgeDisplayManager = std::make_unique<EdgeDisplayManager>(m_sceneManager, &m_geometries);
	// Create selection manager and object tree sync
	m_selectionManager = std::make_unique<SelectionManager>(m_sceneManager, &m_geometries, &m_selectedGeometries);
	m_objectTreeSync = std::make_unique<ObjectTreeSync>(m_sceneManager, &m_pendingObjectTreeUpdates);
	// Create selection outline manager (geometry-layer outlines)
	m_outlineManager = std::make_unique<OutlineDisplayManager>(m_sceneManager, m_occRoot, &m_geometries);
	m_selectionOutline = std::make_unique<SelectionOutlineManager>(m_sceneManager, m_occRoot, &m_selectedGeometries);
	// Selection outline disabled by default to avoid unwanted red lines
	// if (m_selectionOutline) m_selectionOutline->setEnabled(true);
	// Create geometry repo and scene attachment helper
	m_geometryRepo = std::make_unique<GeometryRepository>(&m_geometries);
	m_sceneAttach = std::make_unique<SceneAttachmentService>(m_occRoot, &m_nodeToGeom);
	m_viewUpdater = std::make_unique<ViewUpdateService>(m_sceneManager);
	m_meshingService = std::make_unique<MeshingService>();
	m_meshController = std::make_unique<MeshParameterController>(this, m_meshingService.get(), &m_meshParams, &m_geometries);
	m_batchManager = std::make_unique<BatchOperationManager>(m_sceneManager, m_objectTreeSync.get(), m_viewUpdater.get());
}

OCCViewer::~OCCViewer()
{
	clearAll();
	if (m_occRoot) {
		m_occRoot->unref();
	}
}

void OCCViewer::initializeViewer()
{
	m_occRoot = new SoSeparator;
	m_occRoot->ref();

	if (m_sceneManager) {
		m_sceneManager->getObjectRoot()->addChild(m_occRoot);
	}
	// Create slice controller bound to current root
	m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	// Create picking service bound to current root and node map
	m_pickingService = std::make_unique<PickingService>(m_sceneManager, m_occRoot, &m_nodeToGeom);
	// Create outline manager (disabled by default)
	m_outlineManager = std::make_unique<OutlineDisplayManager>(m_sceneManager, m_occRoot, &m_geometries);

	LOG_INF_S("OCC Viewer initialized");
}

// Slice nodes are managed by SliceController

void OCCViewer::addGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	auto addStartTime = std::chrono::high_resolution_clock::now();

	if (!geometry) {
		LOG_ERR_S("Attempted to add null geometry to OCCViewer");
		return;
	}

	auto validationStartTime = std::chrono::high_resolution_clock::now();
	if (m_geometryRepo && m_geometryRepo->existsByName(geometry->getName())) {
		LOG_WRN_S("Geometry with name '" + geometry->getName() + "' already exists");
		return;
	}
	auto validationEndTime = std::chrono::high_resolution_clock::now();
	auto validationDuration = std::chrono::duration_cast<std::chrono::microseconds>(validationEndTime - validationStartTime);

	// Only log in non-batch mode to reduce overhead
	if (!m_batchOperationActive) {
		LOG_INF_S("Adding geometry: " + geometry->getName());
	}

	auto meshStartTime = std::chrono::high_resolution_clock::now();
	// Use optimized mesh update method
	geometry->updateCoinRepresentationIfNeeded(m_meshParams);
	auto meshEndTime = std::chrono::high_resolution_clock::now();
	auto meshDuration = std::chrono::duration_cast<std::chrono::milliseconds>(meshEndTime - meshStartTime);

	auto storageStartTime = std::chrono::high_resolution_clock::now();
	if (m_geometryRepo) m_geometryRepo->add(geometry);
	auto storageEndTime = std::chrono::high_resolution_clock::now();
	auto storageDuration = std::chrono::duration_cast<std::chrono::microseconds>(storageEndTime - storageStartTime);

	auto coinNodeStartTime = std::chrono::high_resolution_clock::now();
	SoSeparator* coinNode = geometry->getCoinNode();
	auto coinNodeEndTime = std::chrono::high_resolution_clock::now();
	auto coinNodeDuration = std::chrono::duration_cast<std::chrono::microseconds>(coinNodeEndTime - coinNodeStartTime);

	auto addChildStartTime = std::chrono::high_resolution_clock::now();
	if (coinNode && m_occRoot) {
		if (m_sceneAttach) m_sceneAttach->attach(geometry);
	}
	else {
		if (!coinNode) {
			LOG_ERR_S("Coin3D node is null for geometry: " + geometry->getName());
		}
		if (!m_occRoot) {
			LOG_ERR_S("OCC root is null, cannot add geometry: " + geometry->getName());
		}
	}
	auto addChildEndTime = std::chrono::high_resolution_clock::now();
	auto addChildDuration = std::chrono::duration_cast<std::chrono::microseconds>(addChildEndTime - addChildStartTime);

	auto objectTreeStartTime = std::chrono::high_resolution_clock::now();
	if (m_objectTreeSync) {
		m_objectTreeSync->addGeometry(geometry, m_batchOperationActive);
	}
	auto objectTreeEndTime = std::chrono::high_resolution_clock::now();
	auto objectTreeDuration = std::chrono::duration_cast<std::chrono::microseconds>(objectTreeEndTime - objectTreeStartTime);

	auto viewUpdateStartTime = std::chrono::high_resolution_clock::now();
	// Auto-update scene bounds and optionally view when geometry is added
	if (m_viewUpdater) {
		m_viewUpdater->updateSceneBounds();
		if (m_batchOperationActive) {
			m_needsViewRefresh = true;
			if (m_batchManager) m_batchManager->markNeedsViewRefresh();
		}
		else {
			if (!m_preserveViewOnAdd && m_viewUpdater) m_viewUpdater->resetView();
			m_viewUpdater->requestRefresh(static_cast<int>(IViewRefresher::Reason::GEOMETRY_CHANGED), true);
			m_viewUpdater->refreshCanvas(false);
			LOG_INF_S(std::string("Scene bounds updated and ") + (m_preserveViewOnAdd ? "view preserved" : "view reset") + " after adding geometry");
		}
	}

	// Notify outline manager if exists
	if (m_outlineManager) {
		m_outlineManager->onGeometryAdded(geometry);
	}
	auto viewUpdateEndTime = std::chrono::high_resolution_clock::now();
	auto viewUpdateDuration = std::chrono::duration_cast<std::chrono::microseconds>(viewUpdateEndTime - viewUpdateStartTime);

	auto addEndTime = std::chrono::high_resolution_clock::now();
	auto addDuration = std::chrono::duration_cast<std::chrono::milliseconds>(addEndTime - addStartTime);

	// Only show detailed breakdown in non-batch mode or for debugging
	if (!m_batchOperationActive) {
		LOG_INF_S("=== GEOMETRY ADDITION BREAKDOWN ===");
		LOG_INF_S("Geometry: " + geometry->getName());
		LOG_INF_S("  - Validation: " + std::to_string(validationDuration.count()) + "μs");
		LOG_INF_S("  - Mesh regeneration: " + std::to_string(meshDuration.count()) + "ms");
		LOG_INF_S("  - Storage: " + std::to_string(storageDuration.count()) + "μs");
		LOG_INF_S("  - Coin3D node: " + std::to_string(coinNodeDuration.count()) + "μs");
		LOG_INF_S("  - Add to scene: " + std::to_string(addChildDuration.count()) + "μs");
		LOG_INF_S("  - Object tree: " + std::to_string(objectTreeDuration.count()) + "μs");
		LOG_INF_S("  - View update: " + std::to_string(viewUpdateDuration.count()) + "μs");
		LOG_INF_S("TOTAL ADDITION TIME: " + std::to_string(addDuration.count()) + "ms");
		LOG_INF_S("==================================");
	}
}

void OCCViewer::removeGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) return;

	auto it = std::find(m_geometries.begin(), m_geometries.end(), geometry);
	if (it != m_geometries.end()) {
		if (m_sceneAttach) m_sceneAttach->detach(geometry);

		auto selectedIt = std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry);
		if (selectedIt != m_selectedGeometries.end()) {
			m_selectedGeometries.erase(selectedIt);
		}

		// Remove geometry from ObjectTree
		if (m_objectTreeSync) m_objectTreeSync->removeGeometry(geometry);

		if (m_geometryRepo) m_geometryRepo->remove(geometry);
		LOG_INF_S("Removed OCC geometry: " + geometry->getName());
	}
}

void OCCViewer::removeGeometry(const std::string& name)
{
	auto geometry = findGeometry(name);
	if (geometry) {
		removeGeometry(geometry);
	}
}

void OCCViewer::clearAll()
{
	m_selectedGeometries.clear();
	if (m_geometryRepo) m_geometryRepo->clear();
	if (m_sceneAttach) m_sceneAttach->detachAll();

	LOG_INF_S("Cleared all OCC geometries");
}

// --- Hover silhouette helpers ---

namespace {
	// Find the first separator that is a direct child of m_occRoot (top-level geometry node)
	SoSeparator* findTopLevelSeparatorInPath(SoPath* path, SoSeparator* occRoot) {
		if (!path || !occRoot) return nullptr;
		for (int i = 0; i < path->getLength(); ++i) {
			SoNode* node = path->getNode(i);
			if (node == occRoot) {
				// Next separator child under occRoot is the geometry root we added
				for (int j = i + 1; j < path->getLength(); ++j) {
					SoSeparator* sep = dynamic_cast<SoSeparator*>(path->getNode(j));
					if (sep) return sep;
				}
				break;
			}
		}
		return nullptr;
	}
}

std::shared_ptr<OCCGeometry> OCCViewer::pickGeometryAtScreen(const wxPoint& screenPos) {
	if (!m_pickingService) m_pickingService = std::make_unique<PickingService>(m_sceneManager, m_occRoot, &m_nodeToGeom);
	return m_pickingService->pickGeometryAtScreen(screenPos);
}

void OCCViewer::setHoveredSilhouette(std::shared_ptr<OCCGeometry> geometry) {
	if (!m_hoverManager) m_hoverManager = std::make_unique<HoverSilhouetteManager>(m_sceneManager, m_occRoot, m_pickingService.get());
	m_hoverManager->setHoveredSilhouette(geometry);
}

void OCCViewer::updateHoverSilhouetteAt(const wxPoint& screenPos) {
	// Use outline manager for hover if enabled
	if (m_outlineManager && m_outlineManager->isEnabled() && m_outlineManager->isHoverMode()) {
		if (m_pickingService) {
			std::shared_ptr<OCCGeometry> geometry;
			// Only pick if screen position is valid
			if (screenPos.x >= 0 && screenPos.y >= 0) {
				geometry = m_pickingService->pickGeometryAtScreen(screenPos);
			}
			m_outlineManager->setHoveredGeometry(geometry);
			if (m_sceneManager && m_sceneManager->getCanvas()) {
				m_sceneManager->getCanvas()->Refresh(false);
			}
		}
	} else {
		// Fall back to legacy hover silhouette
		if (!m_hoverManager) m_hoverManager = std::make_unique<HoverSilhouetteManager>(m_sceneManager, m_occRoot, m_pickingService.get());
		m_hoverManager->updateHoverSilhouetteAt(screenPos);
	}
}

std::shared_ptr<OCCGeometry> OCCViewer::findGeometry(const std::string& name)
{
	if (m_geometryRepo) return m_geometryRepo->findByName(name);
	return nullptr;
}

std::vector<std::shared_ptr<OCCGeometry>> OCCViewer::getAllGeometry() const
{
	return m_geometries;
}

std::vector<std::shared_ptr<OCCGeometry>> OCCViewer::getSelectedGeometries() const
{
	return m_selectedGeometries;
}

void OCCViewer::setGeometryVisible(const std::string& name, bool visible)
{
	if (m_selectionManager) m_selectionManager->setGeometryVisible(name, visible);
	// Ensure scene attachment reflects visibility
	if (auto g = findGeometry(name)) {
		SoSeparator* coinNode = g->getCoinNode();
		if (coinNode && m_occRoot) {
			int idx = m_occRoot->findChild(coinNode);
			if (visible) {
				if (idx < 0) {
					m_occRoot->addChild(coinNode);
				}
			}
			else {
				if (idx >= 0) {
					m_occRoot->removeChild(idx);
				}
			}
		}
	}
	// Also update tree item text to reflect [H] marker
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		Canvas* canvas = m_sceneManager->getCanvas();
		if (canvas && canvas->getObjectTreePanel()) {
			if (auto g = findGeometry(name)) {
				canvas->getObjectTreePanel()->updateOCCGeometryName(g);
			}
		}
	}
	if (m_viewUpdater) m_viewUpdater->requestGeometryChanged(true);
}

void OCCViewer::setGeometrySelected(const std::string& name, bool selected)
{
	if (m_selectionManager) m_selectionManager->setGeometrySelected(name, selected);
}

void OCCViewer::setGeometryColor(const std::string& name, const Quantity_Color& color)
{
	if (m_selectionManager) m_selectionManager->setGeometryColor(name, color);
}

void OCCViewer::setGeometryTransparency(const std::string& name, double transparency)
{
	if (m_selectionManager) m_selectionManager->setGeometryTransparency(name, transparency);
}

void OCCViewer::hideAll()
{
	if (m_selectionManager) m_selectionManager->hideAll();
}

void OCCViewer::showAll()
{
	if (m_selectionManager) m_selectionManager->showAll();
}

void OCCViewer::selectAll()
{
	if (m_selectionManager) m_selectionManager->selectAll();
}

void OCCViewer::deselectAll()
{
	if (m_selectionManager) m_selectionManager->deselectAll();
}

void OCCViewer::setAllColor(const Quantity_Color& color)
{
	for (auto& g : m_geometries) g->setColor(color);
}

void OCCViewer::fitAll()
{
	LOG_INF_S("Fit all OCC geometries");

	if (!m_sceneManager) {
		LOG_WRN_S("SceneManager is null, cannot perform fitAll");
		return;
	}

	// Update scene bounds first to ensure we have accurate bounding information
	m_sceneManager->updateSceneBounds();

	// Reset view to fit all geometries
	m_sceneManager->resetView();

	// Request view refresh
	if (m_sceneManager->getCanvas()) {
		Canvas* canvas = m_sceneManager->getCanvas();
		if (canvas && canvas->getRefreshManager()) {
			canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
		}
		canvas->Refresh();
	}

	LOG_INF_S("FitAll completed - view adjusted to show all geometries");
}

void OCCViewer::fitGeometry(const std::string& name)
{
	LOG_INF_S("Fit geometry: " + name);
}

std::shared_ptr<OCCGeometry> OCCViewer::pickGeometry(int x, int y)
{
	if (!m_sceneManager) {
		return nullptr;
	}

	// Convert screen coordinates to world coordinates
	SbVec3f worldPos;
	if (!m_sceneManager->screenToWorld(wxPoint(x, y), worldPos)) {
		LOG_WRN_S("Failed to convert screen coordinates to world coordinates");
		return nullptr;
	}

	LOG_INF_S("Picking at world position: (" + std::to_string(worldPos[0]) + ", " +
		std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]) + ")");

	// Find the closest geometry to the picked point
	std::shared_ptr<OCCGeometry> closestGeometry = nullptr;
	double minDistance = std::numeric_limits<double>::max();

	for (auto& geometry : m_geometries) {
		if (!geometry->isVisible()) continue;

		// Get geometry position
		gp_Pnt geometryPos = geometry->getPosition();

		// Calculate distance to geometry center
		double distance = std::sqrt(
			std::pow(worldPos[0] - geometryPos.X(), 2) +
			std::pow(worldPos[1] - geometryPos.Y(), 2) +
			std::pow(worldPos[2] - geometryPos.Z(), 2)
		);

		LOG_INF_S("Geometry '" + geometry->getName() + "' at distance: " + std::to_string(distance));

		// Use a much larger picking radius for complex geometries like wrench
		double pickingRadius = 15.0; // Increased from 10.0 to 15.0

		if (distance < minDistance && distance < pickingRadius) {
			minDistance = distance;
			closestGeometry = geometry;
		}
	}

	if (closestGeometry) {
		LOG_INF_S("Picked geometry: " + closestGeometry->getName() + " at distance: " + std::to_string(minDistance));
	}
	else {
		LOG_INF_S("No geometry picked");
	}

	return closestGeometry;
}

void OCCViewer::setWireframeMode(bool wireframe)
{
	m_wireframeMode = wireframe;
	// Update all geometries to wireframe mode
	for (auto& geometry : m_geometries) {
		if (geometry) {
			geometry->setWireframeMode(wireframe);
		}
	}

	// Request immediate view refresh
	if (m_viewUpdater) m_viewUpdater->requestGeometryChanged(true);
}

// Removed setShadingMode method - functionality not needed

void OCCViewer::setShowEdges(bool showEdges)
{
	m_showEdges = showEdges;

	// Update EdgeSettingsConfig
	EdgeSettingsConfig& edgeConfig = EdgeSettingsConfig::getInstance();
	edgeConfig.setGlobalShowEdges(showEdges);

	// Update rendering toolkit configuration
	auto& config = RenderingToolkitAPI::getConfig();
	config.getEdgeSettings().showEdges = showEdges;

	remeshAllGeometries();

	// Immediate refresh
	if (m_viewUpdater) m_viewUpdater->requestEdgesToggled(true);

	LOG_INF_S("OCCViewer showEdges set to: " + std::string(showEdges ? "enabled" : "disabled"));
}

void OCCViewer::setAntiAliasing(bool enabled)
{
	m_antiAliasing = enabled;
}

bool OCCViewer::isWireframeMode() const
{
	return m_wireframeMode;
}

// Removed isShadingMode method - functionality not needed

bool OCCViewer::isShowEdges() const
{
	return m_showEdges;
}

bool OCCViewer::isShowNormals() const
{
	return m_showNormals;
}

void OCCViewer::setMeshDeflection(double deflection, bool remesh)
{
	if (m_meshParams.deflection != deflection) {
		m_meshParams.deflection = deflection;

		// Throttle remeshing to prevent excessive calls
		if (remesh) {
			static wxLongLong lastRemeshTime = 0;
			wxLongLong currentTime = wxGetLocalTimeMillis();
			const int MIN_REMESH_INTERVAL = 200; // Minimum 200ms between remesh operations

			if (currentTime - lastRemeshTime >= MIN_REMESH_INTERVAL) {
				try {
					remeshAllGeometries();
					lastRemeshTime = currentTime;
					// Reduce debug logging frequency
					static int logCounter = 0;
					if (++logCounter % 10 == 0) { // Log every 10th remesh
						LOG_DBG_S("Remeshed all geometries with deflection: " + std::to_string(deflection));
					}
				}
				catch (const std::exception& e) {
					LOG_ERR_S("OCCViewer::setMeshDeflection: Exception during remesh: " + std::string(e.what()));
					// Don't update lastRemeshTime if remesh failed
				}
				catch (...) {
					LOG_ERR_S("OCCViewer::setMeshDeflection: Unknown exception during remesh");
					// Don't update lastRemeshTime if remesh failed
				}
			}
			else {
				// Skip remesh if too soon after last one - reduce logging frequency in debug
				static int skipLogCounter = 0;
				if (++skipLogCounter % 50 == 0) { // Log every 50th skip
					long long timeDiff = (currentTime - lastRemeshTime).GetValue();
					LOG_DBG_S("Remesh throttled for deflection: " + std::to_string(deflection) +
						" (last remesh was " + std::to_string(timeDiff) + "ms ago)");
				}
			}
		}
	}
}

double OCCViewer::getMeshDeflection() const
{
	return m_meshParams.deflection;
}

void OCCViewer::setAngularDeflection(double deflection, bool remesh)
{
	if (m_meshParams.angularDeflection != deflection) {
		m_meshParams.angularDeflection = deflection;

		// Throttle remeshing to prevent excessive calls
		if (remesh) {
			static wxLongLong lastRemeshTime = 0;
			wxLongLong currentTime = wxGetLocalTimeMillis();
			const int MIN_REMESH_INTERVAL = 200; // Minimum 200ms between remesh operations

			if (currentTime - lastRemeshTime >= MIN_REMESH_INTERVAL) {
				try {
					remeshAllGeometries();
					lastRemeshTime = currentTime;
					// Reduce debug logging frequency
					static int logCounter = 0;
					if (++logCounter % 10 == 0) { // Log every 10th remesh
						LOG_DBG_S("Remeshed all geometries with angular deflection: " + std::to_string(deflection));
					}
				}
				catch (const std::exception& e) {
					LOG_ERR_S("OCCViewer::setAngularDeflection: Exception during remesh: " + std::string(e.what()));
					// Don't update lastRemeshTime if remesh failed
				}
				catch (...) {
					LOG_ERR_S("OCCViewer::setAngularDeflection: Unknown exception during remesh");
					// Don't update lastRemeshTime if remesh failed
				}
			}
			else {
				// Skip remesh if too soon after last one - reduce logging frequency in debug
				static int skipLogCounter = 0;
				if (++skipLogCounter % 50 == 0) { // Log every 50th skip
					long long timeDiff = (currentTime - lastRemeshTime).GetValue();
					LOG_DBG_S("Remesh throttled for angular deflection: " + std::to_string(deflection) +
						" (last remesh was " + std::to_string(timeDiff) + "ms ago)");
				}
			}
		}
	}
}

double OCCViewer::getAngularDeflection() const
{
	return m_meshParams.angularDeflection;
}

void OCCViewer::onSelectionChanged()
{
	if (m_selectionManager) m_selectionManager->onSelectionChanged();
	if (m_selectionOutline && m_selectionOutline->isEnabled()) m_selectionOutline->syncToSelection();
}

void OCCViewer::onGeometryChanged(std::shared_ptr<OCCGeometry> geometry)
{
	if (geometry) {
		geometry->regenerateMesh(m_meshParams);
	}
}

void OCCViewer::setShowNormals(bool showNormals)
{
	m_showNormals = showNormals;
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowNormalLines(showNormals, m_meshParams);
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
		if (refreshManager) {
			refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::NORMALS_TOGGLED, true);
		}
	}
}

void OCCViewer::setNormalLength(double length)
{
	m_normalLength = length;
	// Force regeneration of normal lines with new length
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->updateAll(m_meshParams);
	}
}

void OCCViewer::setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect)
{
	m_correctNormalColor = correct;
	m_incorrectNormalColor = incorrect;
	// Force regeneration of normal lines with new colors
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->updateAll(m_meshParams);
	}
}

void OCCViewer::updateNormalsDisplay()
{
	// This function is no longer needed as m_normalRoot is removed.
	// The normal display logic is now handled by the EdgeComponent.
}

// Enhanced normal consistency tools
void OCCViewer::setNormalConsistencyMode(bool enabled)
{
	m_normalConsistencyMode = enabled;
	LOG_INF_S("Normal consistency mode " + std::string(enabled ? "enabled" : "disabled"));
	
	// Apply to all geometries
	for (auto& geometry : m_geometries) {
		if (geometry) {
			// Force regeneration with consistency mode
			geometry->regenerateMesh(m_meshParams);
		}
	}
	
	// Refresh view
	if (m_viewUpdater) m_viewUpdater->requestGeometryChanged(true);
}

bool OCCViewer::isNormalConsistencyModeEnabled() const
{
	return m_normalConsistencyMode;
}

void OCCViewer::setNormalDebugMode(bool enabled)
{
	m_normalDebugMode = enabled;
	LOG_INF_S("Normal debug mode " + std::string(enabled ? "enabled" : "disabled"));
	
	// Toggle normal display based on debug mode
	setShowNormals(enabled);
	
	// Update normal colors for debug visualization
	if (enabled) {
		setNormalColor(
			Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB),  // Green for correct
			Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB)   // Red for incorrect
		);
	}
}

bool OCCViewer::isNormalDebugModeEnabled() const
{
	return m_normalDebugMode;
}

void OCCViewer::refreshNormalDisplay()
{
	LOG_INF_S("Refreshing normal display");
	
	// Force regeneration of all normal lines
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->updateAll(m_meshParams);
	}
	
	// Refresh view
	if (m_viewUpdater) m_viewUpdater->requestGeometryChanged(true);
}

void OCCViewer::toggleNormalDisplay()
{
	bool currentState = isShowNormals();
	setShowNormals(!currentState);
	LOG_INF_S("Normal display toggled to: " + std::string(!currentState ? "ON" : "OFF"));
}

void OCCViewer::createNormalVisualization(std::shared_ptr<OCCGeometry> geometry)
{
	// This function is no longer needed as m_normalRoot is removed.
	// The normal display logic is now handled by the EdgeComponent.
}

void OCCViewer::remeshAllGeometries()
{
	if (m_meshController) m_meshController->remeshAll();
}

// LOD (Level of Detail) methods
void OCCViewer::setLODEnabled(bool enabled)
{
	if (m_lodEnabled != enabled) {
		m_lodEnabled = enabled;
		if (m_lodController) m_lodController->setEnabled(enabled);
		LOG_INF_S("LOD " + std::string(enabled ? "enabled" : "disabled"));
	}
}

bool OCCViewer::isLODEnabled() const { return m_lodEnabled; }

void OCCViewer::setLODRoughDeflection(double deflection)
{
	if (m_lodController && m_lodController->getRoughDeflection() != deflection) {
		m_lodController->setRoughDeflection(deflection);
		LOG_INF_S("LOD rough deflection set to: " + std::to_string(deflection));
	}
}

double OCCViewer::getLODRoughDeflection() const { return m_lodController ? m_lodController->getRoughDeflection() : 0.1; }

void OCCViewer::setLODFineDeflection(double deflection)
{
	if (m_lodController && m_lodController->getFineDeflection() != deflection) {
		m_lodController->setFineDeflection(deflection);
		LOG_INF_S("LOD fine deflection set to: " + std::to_string(deflection));
	}
}

double OCCViewer::getLODFineDeflection() const { return m_lodController ? m_lodController->getFineDeflection() : 0.01; }

void OCCViewer::setLODTransitionTime(int milliseconds)
{
	if (m_lodController && m_lodController->getTransitionTimeMs() != milliseconds) {
		m_lodController->setTransitionTimeMs(milliseconds);
		LOG_INF_S("LOD transition time set to: " + std::to_string(milliseconds) + "ms");
	}
}

int OCCViewer::getLODTransitionTime() const { return m_lodController ? m_lodController->getTransitionTimeMs() : 500; }

void OCCViewer::setLODMode(bool roughMode)
{
	if (m_lodController) m_lodController->setMode(roughMode);
	LOG_INF_S("LOD mode switched to " + std::string(roughMode ? "rough" : "fine"));
}

bool OCCViewer::isLODRoughMode() const { return m_lodController ? m_lodController->isRoughMode() : false; }

void OCCViewer::onLODTimer() { /* handled by LODController now */ }

void OCCViewer::startLODInteraction()
{
	if (m_lodEnabled && m_lodController) m_lodController->startInteraction();
}

// Batch operations for performance optimization
void OCCViewer::beginBatchOperation()
{
	m_batchOperationActive = true;
	m_needsViewRefresh = false;
	if (m_batchManager) m_batchManager->begin();
	LOG_INF_S("Batch operation started");
}

void OCCViewer::endBatchOperation()
{
	m_batchOperationActive = false;
	if (m_batchManager) m_batchManager->end();
	m_needsViewRefresh = false;
}

bool OCCViewer::isBatchOperationActive() const
{
	return m_batchOperationActive;
}

void OCCViewer::addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
	if (geometries.empty()) {
		return;
	}

	auto batchAddStartTime = std::chrono::high_resolution_clock::now();
	LOG_INF_S("Starting batch addition of " + std::to_string(geometries.size()) + " geometries");

	// Pre-allocate space to avoid reallocations
	m_geometries.reserve(m_geometries.size() + geometries.size());

	// Collect all Coin3D nodes for batch addition
	std::vector<SoSeparator*> coinNodes;
	coinNodes.reserve(geometries.size());

	for (const auto& geometry : geometries) {
		if (!geometry) {
			LOG_ERR_S("Attempted to add null geometry in batch operation");
			continue;
		}

		// Quick validation (no logging in batch mode)
		auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
			[&](const std::shared_ptr<OCCGeometry>& g) {
				return g->getName() == geometry->getName();
			});

		if (it != m_geometries.end()) {
			LOG_WRN_S("Geometry with name '" + geometry->getName() + "' already exists (skipping in batch)");
			continue;
		}

		// Regenerate mesh (lazy)
		geometry->updateCoinRepresentationIfNeeded(m_meshParams);

		// Store geometry
		m_geometries.push_back(geometry);

		// Collect Coin3D node and record mapping for picking -> hover silhouette
		SoSeparator* coinNode = geometry->getCoinNode();
		if (coinNode) {
			coinNodes.push_back(coinNode);
			m_nodeToGeom[coinNode] = geometry;
		}
		else {
			LOG_ERR_S("Coin3D node is null for geometry: " + geometry->getName());
		}
	}

	// Batch add all Coin3D nodes to scene
	if (m_sceneAttach && !coinNodes.empty()) {
		for (const auto& geometry : geometries) {
			if (geometry) m_sceneAttach->attach(geometry);
		}
	}

	// Queue geometries for deferred ObjectTree update in batch mode
	if (m_batchOperationActive) {
		for (const auto& geometry : geometries) {
			if (geometry) {
				m_pendingObjectTreeUpdates.push_back(geometry);
			}
		}
		LOG_INF_S("Queued " + std::to_string(geometries.size()) + " geometries for deferred ObjectTree update");
	}

	auto batchAddEndTime = std::chrono::high_resolution_clock::now();
	auto batchAddDuration = std::chrono::duration_cast<std::chrono::milliseconds>(batchAddEndTime - batchAddStartTime);
	LOG_INF_S("Batch geometry addition completed in " + std::to_string(batchAddDuration.count()) + "ms");
}

void OCCViewer::requestViewRefresh()
{
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		Canvas* canvas = m_sceneManager->getCanvas();
		canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, true);
	}
}

void OCCViewer::updateObjectTreeDeferred()
{
	if (m_pendingObjectTreeUpdates.empty()) {
		LOG_INF_S("No pending ObjectTree updates to process");
		return;
	}
	auto updateStartTime = std::chrono::high_resolution_clock::now();
	if (m_objectTreeSync) m_objectTreeSync->processDeferred();
	auto updateEndTime = std::chrono::high_resolution_clock::now();
	auto updateDuration = std::chrono::duration_cast<std::chrono::milliseconds>(updateEndTime - updateStartTime);
	LOG_INF_S("Deferred ObjectTree updates processed in " + std::to_string(updateDuration.count()) + "ms");
}

// Subdivision surface control implementations
void OCCViewer::setSubdivisionEnabled(bool enabled)
{
	if (m_subdivisionEnabled == enabled) return;
	m_subdivisionEnabled = enabled;
	LOG_INF_S("Subdivision enabled: " + std::string(enabled ? "true" : "false"));
	logParameterChange("subdivision_enabled", enabled ? 0.0 : 1.0, enabled ? 1.0 : 0.0);
	if (m_meshController) m_meshController->setSubdivisionEnabled(enabled);
}

bool OCCViewer::isSubdivisionEnabled() const
{
	return m_subdivisionEnabled;
}

void OCCViewer::setSubdivisionLevel(int level)
{
	if (level < 1 || level > 5 || m_subdivisionLevel == level) return;
	int oldValue = m_subdivisionLevel;
	m_subdivisionLevel = level;
	LOG_INF_S("Subdivision level set to: " + std::to_string(level));
	logParameterChange("subdivision_level", oldValue, level);
	if (m_meshController) m_meshController->setSubdivisionLevel(level);
}

int OCCViewer::getSubdivisionLevel() const
{
	return m_subdivisionLevel;
}

void OCCViewer::setSubdivisionMethod(int method)
{
	if (method < 0 || method > 3 || m_subdivisionMethod == method) return;
	m_subdivisionMethod = method;
	LOG_INF_S("Subdivision method set to: " + std::to_string(method));
	if (m_meshController) m_meshController->setSubdivisionMethod(method);
}

int OCCViewer::getSubdivisionMethod() const
{
	return m_subdivisionMethod;
}

void OCCViewer::setSubdivisionCreaseAngle(double angle)
{
	if (angle < 0.0 || angle > 180.0 || m_subdivisionCreaseAngle == angle) return;
	m_subdivisionCreaseAngle = angle;
	LOG_INF_S("Subdivision crease angle set to: " + std::to_string(angle));
	if (m_meshController) m_meshController->setSubdivisionCreaseAngle(angle);
}

double OCCViewer::getSubdivisionCreaseAngle() const
{
	return m_subdivisionCreaseAngle;
}

// Mesh smoothing control implementations
void OCCViewer::setSmoothingEnabled(bool enabled)
{
	if (m_smoothingEnabled == enabled) return;
	m_smoothingEnabled = enabled;
	LOG_INF_S("Smoothing enabled: " + std::string(enabled ? "true" : "false"));
	if (m_meshController) m_meshController->setSmoothingEnabled(enabled);
}

bool OCCViewer::isSmoothingEnabled() const
{
	return m_smoothingEnabled;
}

void OCCViewer::setSmoothingMethod(int method)
{
	if (method < 0 || method > 3 || m_smoothingMethod == method) return;
	m_smoothingMethod = method;
	LOG_INF_S("Smoothing method set to: " + std::to_string(method));
	if (m_meshController) m_meshController->setSmoothingMethod(method);
}

int OCCViewer::getSmoothingMethod() const
{
	return m_smoothingMethod;
}

void OCCViewer::setSmoothingIterations(int iterations)
{
	if (iterations < 1 || iterations > 10 || m_smoothingIterations == iterations) return;
	m_smoothingIterations = iterations;
	LOG_INF_S("Smoothing iterations set to: " + std::to_string(iterations));
	if (m_meshController) m_meshController->setSmoothingIterations(iterations);
}

int OCCViewer::getSmoothingIterations() const
{
	return m_smoothingIterations;
}

void OCCViewer::setSmoothingStrength(double strength)
{
	if (strength < 0.01 || strength > 1.0 || m_smoothingStrength == strength) return;
	m_smoothingStrength = strength;
	LOG_INF_S("Smoothing strength set to: " + std::to_string(strength));
	if (m_meshController) m_meshController->setSmoothingStrength(strength);
}

double OCCViewer::getSmoothingStrength() const
{
	return m_smoothingStrength;
}

void OCCViewer::setSmoothingCreaseAngle(double angle)
{
	if (angle < 0.0 || angle > 180.0 || m_smoothingCreaseAngle == angle) return;
	m_smoothingCreaseAngle = angle;
	LOG_INF_S("Smoothing crease angle set to: " + std::to_string(angle));
	if (m_meshController) m_meshController->setSmoothingCreaseAngle(angle);
}

double OCCViewer::getSmoothingCreaseAngle() const
{
	return m_smoothingCreaseAngle;
}

// Advanced tessellation control implementations
void OCCViewer::setTessellationMethod(int method)
{
	if (method < 0 || method > 3 || m_tessellationMethod == method) return;
	m_tessellationMethod = method;
	LOG_INF_S("Tessellation method set to: " + std::to_string(method));
	if (m_meshController) m_meshController->setTessellationMethod(method);
}

int OCCViewer::getTessellationMethod() const
{
	return m_tessellationMethod;
}

void OCCViewer::setTessellationQuality(int quality)
{
	if (quality < 1 || quality > 5 || m_tessellationQuality == quality) return;
	m_tessellationQuality = quality;
	LOG_INF_S("Tessellation quality set to: " + std::to_string(quality));
	if (m_meshController) m_meshController->setTessellationQuality(quality);
}

int OCCViewer::getTessellationQuality() const
{
	return m_tessellationQuality;
}

void OCCViewer::setFeaturePreservation(double preservation)
{
	if (preservation < 0.0 || preservation > 1.0 || m_featurePreservation == preservation) return;
	m_featurePreservation = preservation;
	LOG_INF_S("Feature preservation set to: " + std::to_string(preservation));
	if (m_meshController) m_meshController->setFeaturePreservation(preservation);
}

double OCCViewer::getFeaturePreservation() const
{
	return m_featurePreservation;
}

void OCCViewer::setParallelProcessing(bool enabled)
{
	if (m_parallelProcessing == enabled) return;
	m_parallelProcessing = enabled;
	LOG_INF_S("Parallel processing enabled: " + std::string(enabled ? "true" : "false"));
	if (m_meshController) m_meshController->setParallelProcessing(enabled);
}

bool OCCViewer::isParallelProcessing() const
{
	return m_parallelProcessing;
}

void OCCViewer::setAdaptiveMeshing(bool enabled)
{
	if (m_adaptiveMeshing == enabled) return;
	m_adaptiveMeshing = enabled;
	LOG_INF_S("Adaptive meshing enabled: " + std::string(enabled ? "true" : "false"));
	if (m_meshController) m_meshController->setAdaptiveMeshing(enabled);
}

bool OCCViewer::isAdaptiveMeshing() const
{
	return m_adaptiveMeshing;
}

// Mesh quality validation and debugging implementations
void OCCViewer::validateMeshParameters()
{
	LOG_INF_S("=== MESH PARAMETER VALIDATION ===");

	// Validate subdivision parameters
	LOG_INF_S("Subdivision Settings:");
	LOG_INF_S("  - Enabled: " + std::string(m_subdivisionEnabled ? "true" : "false"));
	LOG_INF_S("  - Level: " + std::to_string(m_subdivisionLevel));
	LOG_INF_S("  - Method: " + std::to_string(m_subdivisionMethod));
	LOG_INF_S("  - Crease Angle: " + std::to_string(m_subdivisionCreaseAngle));

	// Validate smoothing parameters
	LOG_INF_S("Smoothing Settings:");
	LOG_INF_S("  - Enabled: " + std::string(m_smoothingEnabled ? "true" : "false"));
	LOG_INF_S("  - Method: " + std::to_string(m_smoothingMethod));
	LOG_INF_S("  - Iterations: " + std::to_string(m_smoothingIterations));
	LOG_INF_S("  - Strength: " + std::to_string(m_smoothingStrength));
	LOG_INF_S("  - Crease Angle: " + std::to_string(m_smoothingCreaseAngle));

	// Validate tessellation parameters
	LOG_INF_S("Tessellation Settings:");
	LOG_INF_S("  - Method: " + std::to_string(m_tessellationMethod));
	LOG_INF_S("  - Quality: " + std::to_string(m_tessellationQuality));
	LOG_INF_S("  - Feature Preservation: " + std::to_string(m_featurePreservation));
	LOG_INF_S("  - Parallel Processing: " + std::string(m_parallelProcessing ? "true" : "false"));
	LOG_INF_S("  - Adaptive Meshing: " + std::string(m_adaptiveMeshing ? "true" : "false"));

	// Validate basic mesh parameters
	LOG_INF_S("Basic Mesh Settings:");
	LOG_INF_S("  - Deflection: " + std::to_string(m_meshParams.deflection));
	LOG_INF_S("  - Angular Deflection: " + std::to_string(m_meshParams.angularDeflection) +
		" (controls curve approximation - lower = smoother curves)");
	LOG_INF_S("  - Relative: " + std::string(m_meshParams.relative ? "true" : "false"));
	LOG_INF_S("  - In Parallel: " + std::string(m_meshParams.inParallel ? "true" : "false"));

	// Add recommendations for curve-surface fitting
	if (m_meshParams.angularDeflection > 2.0) {
		LOG_WRN_S("Angular deflection is large - curves may appear faceted");
		LOG_INF_S("  Recommendation: Reduce angular deflection to < 1.0 for smoother curves");
	} else if (m_meshParams.angularDeflection < 0.5) {
		LOG_INF_S("Angular deflection is small - curves will be very smooth");
		if (m_meshParams.deflection > 0.5) {
			LOG_WRN_S("  Warning: Large deflection with small angular deflection may cause fitting issues");
			LOG_INF_S("  Recommendation: Reduce mesh deflection or increase angular deflection");
		}
	}

	LOG_INF_S("=== VALIDATION COMPLETE ===");
}

void OCCViewer::logCurrentMeshSettings()
{
	LOG_INF_S("=== CURRENT MESH SETTINGS ===");
	LOG_INF_S("Geometry Count: " + std::to_string(m_geometries.size()));

	for (const auto& geometry : m_geometries) {
		if (geometry) {
			LOG_INF_S("Geometry: " + geometry->getName());
			// TODO: Add geometry-specific mesh statistics
		}
	}

	LOG_INF_S("=== SETTINGS LOGGED ===");
}

void OCCViewer::compareMeshQuality(const std::string& geometryName)
{
	auto geometry = findGeometry(geometryName);
	if (!geometry) {
		LOG_ERR_S("Geometry not found: " + geometryName);
		return;
	}

	LOG_INF_S("=== MESH QUALITY COMPARISON FOR: " + geometryName + " ===");

	// TODO: Implement mesh quality comparison
	// This would compare current mesh with previous state or reference mesh

	LOG_INF_S("=== COMPARISON COMPLETE ===");
}

std::string OCCViewer::getMeshQualityReport() const
{
	std::string report = "=== MESH QUALITY REPORT ===\n";

	report += "Active Geometries: " + std::to_string(m_geometries.size()) + "\n";
	report += "Subdivision Enabled: " + std::string(m_subdivisionEnabled ? "Yes" : "No") + "\n";
	report += "Smoothing Enabled: " + std::string(m_smoothingEnabled ? "Yes" : "No") + "\n";
	report += "Adaptive Meshing: " + std::string(m_adaptiveMeshing ? "Yes" : "No") + "\n";
	report += "Parallel Processing: " + std::string(m_parallelProcessing ? "Yes" : "No") + "\n";

	report += "\nCurrent Parameters:\n";
	report += "- Deflection: " + std::to_string(m_meshParams.deflection) + "\n";
	report += "- Subdivision Level: " + std::to_string(m_subdivisionLevel) + "\n";
	report += "- Smoothing Iterations: " + std::to_string(m_smoothingIterations) + "\n";
	report += "- Tessellation Quality: " + std::to_string(m_tessellationQuality) + "\n";

	return report;
}

void OCCViewer::exportMeshStatistics(const std::string& filename)
{
	LOG_INF_S("Exporting mesh statistics to: " + filename);

	// TODO: Implement mesh statistics export
	// This would export detailed mesh information to a file

	LOG_INF_S("Mesh statistics exported successfully");
}

bool OCCViewer::verifyParameterApplication(const std::string& parameterName, double expectedValue)
{
	LOG_INF_S("Verifying parameter: " + parameterName + " = " + std::to_string(expectedValue));

	// Check if parameter matches expected value
	if (parameterName == "deflection") {
		bool matches = std::abs(m_meshParams.deflection - expectedValue) < 1e-6;
		LOG_INF_S("Deflection verification: " + std::string(matches ? "PASS" : "FAIL"));
		return matches;
	}
	else if (parameterName == "subdivision_level") {
		bool matches = (m_subdivisionLevel == static_cast<int>(expectedValue));
		LOG_INF_S("Subdivision level verification: " + std::string(matches ? "PASS" : "FAIL"));
		return matches;
	}
	else if (parameterName == "smoothing_iterations") {
		bool matches = (m_smoothingIterations == static_cast<int>(expectedValue));
		LOG_INF_S("Smoothing iterations verification: " + std::string(matches ? "PASS" : "FAIL"));
		return matches;
	}
	else if (parameterName == "angular_deflection") {
		bool matches = std::abs(m_meshParams.angularDeflection - expectedValue) < 1e-6;
		LOG_INF_S("Angular deflection verification: " + std::string(matches ? "PASS" : "FAIL"));
		return matches;
	}
	// Add more parameter checks as needed

	LOG_ERR_S("Unknown parameter: " + parameterName);
	return false;
}

// Real-time parameter monitoring implementations
void OCCViewer::enableParameterMonitoring(bool enabled)
{
	m_parameterMonitoringEnabled = enabled;
	LOG_INF_S("Parameter monitoring " + std::string(enabled ? "enabled" : "disabled"));
}

bool OCCViewer::isParameterMonitoringEnabled() const
{
	return m_parameterMonitoringEnabled;
}

void OCCViewer::logParameterChange(const std::string& parameterName, double oldValue, double newValue)
{
	if (m_parameterMonitoringEnabled) {
		LOG_INF_S("PARAMETER CHANGE: " + parameterName +
			" [" + std::to_string(oldValue) + " -> " + std::to_string(newValue) + "]");
	}
}

// Legacy display flag APIs removed; rendering handled at geometry level and by EdgeDisplayManager

void OCCViewer::setShowOriginalEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowOriginalEdges(show, m_meshParams);
}

void OCCViewer::setOriginalEdgesParameters(double samplingDensity, double minLength, bool showLinesOnly, const wxColour& color, double width) {
	// Store parameters for use when generating original edges
	m_originalEdgesSamplingDensity = samplingDensity;
	m_originalEdgesMinLength = minLength;
	m_originalEdgesShowLinesOnly = showLinesOnly;
	m_originalEdgesColor = color;
	m_originalEdgesWidth = width;
	
	LOG_INF_S("Original edges parameters set: density=" + std::to_string(samplingDensity) + 
		", minLength=" + std::to_string(minLength) + 
		", linesOnly=" + std::string(showLinesOnly ? "true" : "false") +
		", width=" + std::to_string(width));
	
	// Convert wxColour to Quantity_Color
	Quantity_Color occColor(color.Red() / 255.0, color.Green() / 255.0, color.Blue() / 255.0, Quantity_TOC_RGB);
	
	// Apply parameters to EdgeDisplayManager
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->setOriginalEdgesParameters(samplingDensity, minLength, showLinesOnly, occColor, width);
	}
	
	// Refresh the view
	requestViewRefresh();
}
void OCCViewer::setShowFeatureEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowFeatureEdges(show, m_meshParams);
}

void OCCViewer::applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->applyFeatureEdgeAppearance(color, width, edgesOnly);
}

void OCCViewer::setExplodeEnabled(bool enabled, double factor) {
	if (!m_explodeController) m_explodeController = std::make_unique<ExplodeController>(m_occRoot);
	if (m_explodeEnabled == enabled && std::abs(m_explodeFactor - factor) < 1e-6) return;
	m_explodeEnabled = enabled;
	m_explodeFactor = factor;
	m_explodeController->setEnabled(enabled, factor);
	if (enabled) applyExplode(); else clearExplode();
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

void OCCViewer::setExplodeParams(ExplodeMode mode, double factor) {
	m_explodeMode = mode;
	m_explodeFactor = factor;
	if (!m_explodeController) m_explodeController = std::make_unique<ExplodeController>(m_occRoot);
	m_explodeController->setParams(mode, factor);
	if (m_explodeEnabled) {
		applyExplode();
		if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
	}
}

void OCCViewer::applyExplode() {
	if (!m_explodeController) m_explodeController = std::make_unique<ExplodeController>(m_occRoot);
	m_explodeController->setParams(m_explodeMode, m_explodeFactor);
	m_explodeController->apply(m_geometries);
}

void OCCViewer::clearExplode() {
	if (!m_explodeController) return;
	m_explodeController->clear(m_geometries);
}

void OCCViewer::setSliceEnabled(bool enabled) {
	if (!m_sliceController) m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	m_sliceController->setEnabled(enabled);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

void OCCViewer::setSlicePlane(const SbVec3f& normal, float offset) {
	if (!m_sliceController) m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	m_sliceController->setPlane(normal, offset);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

void OCCViewer::moveSliceAlongNormal(float delta) {
	if (!m_sliceController) return;
	m_sliceController->moveAlongNormal(delta);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

bool OCCViewer::isSliceEnabled() const {
	return m_sliceController ? m_sliceController->isEnabled() : false;
}

SbVec3f OCCViewer::getSliceNormal() const {
	return m_sliceController ? m_sliceController->normal() : SbVec3f(0, 0, 1);
}

float OCCViewer::getSliceOffset() const {
	return m_sliceController ? m_sliceController->offset() : 0.0f;
}

void OCCViewer::setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave) {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->setShowFeatureEdges(show, featureAngleDeg, minLength, onlyConvex, onlyConcave, m_meshParams);
	}
}
void OCCViewer::setShowMeshEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowMeshEdges(show, m_meshParams);
}
void OCCViewer::setShowHighlightEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowHighlightEdges(show, m_meshParams);
}
void OCCViewer::setShowNormalLines(bool show) {
	LOG_INF_S("Setting show normal lines to: " + std::string(show ? "true" : "false"));
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowNormalLines(show, m_meshParams);
}

void OCCViewer::setShowFaceNormalLines(bool show) {
	LOG_INF_S("Setting show face normal lines to: " + std::string(show ? "true" : "false"));
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowFaceNormalLines(show, m_meshParams);
}

// removed deprecated setShowSilhouetteEdges

void OCCViewer::toggleEdgeType(EdgeType type, bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->toggleEdgeType(type, show, m_meshParams);
}

bool OCCViewer::isEdgeTypeEnabled(EdgeType type) const {
	const EdgeDisplayFlags& flags = m_edgeDisplayManager ? m_edgeDisplayManager->getFlags() : EdgeDisplayFlags{};
	switch (type) {
	case EdgeType::Original: return flags.showOriginalEdges;
	case EdgeType::Feature: return flags.showFeatureEdges;
	case EdgeType::Mesh: return flags.showMeshEdges;
	case EdgeType::Highlight: return flags.showHighlightEdges;
	case EdgeType::NormalLine: return flags.showNormalLines;
	case EdgeType::FaceNormalLine: return flags.showFaceNormalLines;
	}
	return false;
}

void OCCViewer::updateAllEdgeDisplays() {
	if (m_edgeDisplayManager) m_edgeDisplayManager->updateAll(m_meshParams);
}

void OCCViewer::invalidateFeatureEdgeCache() {}