#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "OCCViewer.h"
#include "viewer/NormalDisplayService.h"
#include "viewer/RenderModeManager.h"
#include "viewer/GeometryManagementService.h"
#include "viewer/ViewOperationsService.h"
#include "viewer/GeometryFactoryService.h"
#include "viewer/ConfigurationManager.h"
#include "viewer/MeshQualityService.h"
#include "OCCGeometry.h"
#include "rendering/RenderingToolkitAPI.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "config/LightingConfig.h"
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
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <algorithm>
#include <limits>
#include <cmath>
#include <wx/gdicmn.h>
#include <chrono>
#include "edges/EdgeDisplayManager.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SoPickedPoint.h>
#include <unordered_map>
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
#include "viewer/SelectionAcceleratorService.h"
#include "viewer/MeshQualityValidator.h"

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
	m_normalRoot(nullptr),
	m_wireframeMode(false),
	m_shadingMode(false),
	m_showEdges(true),
	m_antiAliasing(true),
	m_defaultColor(0.7, 0.7, 0.7, Quantity_TOC_RGB),
	m_lastMeshParams{}  // Initialize to default values to avoid issues caused by random values
{
	initializeViewer();
	// Remove default edge display to avoid conflicts with new EdgeComponent system
	// setShowEdges(true);

	// Create services
	m_configurationManager = std::make_unique<ConfigurationManager>();
	m_normalDisplayService = std::make_unique<NormalDisplayService>();
	m_renderModeManager = std::make_unique<RenderModeManager>();
	m_geometryManagementService = std::make_unique<GeometryManagementService>(
		m_sceneManager, &m_geometries, &m_selectedGeometries);
	m_viewOperationsService = std::make_unique<ViewOperationsService>();
	m_geometryFactoryService = std::make_unique<GeometryFactoryService>();
	m_meshQualityService = std::make_unique<MeshQualityService>();
	m_asyncEngine = std::make_unique<async::AsyncEngineIntegration>();

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

	// Set up geometry management service dependencies
	if (m_geometryManagementService) {
		m_geometryManagementService->setServices(
			m_geometryRepo.get(),
			m_sceneAttach.get(),
			m_objectTreeSync.get(),
			m_selectionManager.get(),
			m_viewUpdater.get()
		);
	}

	// Set up view operations service
	if (m_viewOperationsService) {
		m_viewOperationsService->setBatchMode(m_batchOperationActive);
	}
	m_meshingService = std::make_unique<MeshingService>();
	m_meshController = std::make_unique<MeshParameterController>(this, m_meshingService.get(), &m_meshParams, &m_geometries);
	m_batchManager = std::make_unique<BatchOperationManager>(m_sceneManager, m_objectTreeSync.get(), m_viewUpdater.get());
	
	// Initialize selection accelerator service
	m_selectionAcceleratorService = std::make_unique<SelectionAcceleratorService>();
	
	// Initialize mesh quality validator
	m_qualityValidator = std::make_unique<MeshQualityValidator>();
	m_qualityValidator->setContext(&m_geometries, &m_meshParams);
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

}

void OCCViewer::addGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	auto addStartTime = std::chrono::high_resolution_clock::now();

	if (!geometry) {
		LOG_ERR_S("Attempted to add null geometry to OCCViewer");
		return;
	}

	// Update mesh representation first
	geometry->updateCoinRepresentationIfNeeded(m_meshParams);

	// Use geometry management service to add the geometry
	if (m_geometryManagementService) {
		if (m_geometryManagementService->addGeometry(geometry, m_batchOperationActive)) {
			// Notify outline manager
			if (m_outlineManager) {
				m_outlineManager->onGeometryAdded(geometry);
			}

			// Handle view updates
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
		}
	}
		}
	}

	auto addEndTime = std::chrono::high_resolution_clock::now();
	auto addDuration = std::chrono::duration_cast<std::chrono::milliseconds>(addEndTime - addStartTime);
}

void OCCViewer::removeGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (m_geometryManagementService) {
		m_geometryManagementService->removeGeometry(geometry);
		// Don't update slice geometries here - it will be updated when needed
	}
}

void OCCViewer::removeGeometry(const std::string& name)
{
	if (m_geometryManagementService) {
		m_geometryManagementService->removeGeometry(name);
		// Don't update slice geometries here - it will be updated when needed
	}
}

void OCCViewer::clearAll()
{
	if (m_geometryManagementService) {
		m_geometryManagementService->clearAll();
		// Don't update slice geometries here - it will be updated when needed
	}
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
	if (m_geometryManagementService) {
		return m_geometryManagementService->findGeometry(name);
	}
	return nullptr;
}

std::vector<std::shared_ptr<OCCGeometry>> OCCViewer::getAllGeometry() const
{
	if (m_geometryManagementService) {
		return m_geometryManagementService->getAllGeometries();
	}
	return std::vector<std::shared_ptr<OCCGeometry>>();
}

std::vector<std::shared_ptr<OCCGeometry>> OCCViewer::getSelectedGeometries() const
{
	if (m_geometryManagementService) {
		return m_geometryManagementService->getSelectedGeometries();
	}
	return std::vector<std::shared_ptr<OCCGeometry>>();
}

void OCCViewer::setGeometryVisible(const std::string& name, bool visible)
{
	if (m_geometryManagementService) {
		m_geometryManagementService->setGeometryVisible(name, visible);
	}
}

void OCCViewer::setGeometrySelected(const std::string& name, bool selected)
{
	if (m_geometryManagementService) {
		m_geometryManagementService->setGeometrySelected(name, selected);
	}
}

void OCCViewer::setGeometryColor(const std::string& name, const Quantity_Color& color)
{
	if (m_geometryManagementService) {
		m_geometryManagementService->setGeometryColor(name, color);
	}
}

void OCCViewer::setGeometryTransparency(const std::string& name, double transparency)
{
	if (m_geometryManagementService) {
		m_geometryManagementService->setGeometryTransparency(name, transparency);
	}
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
}

void OCCViewer::fitGeometry(const std::string& name)
{
	// Find the geometry and fit view to it
	auto geometry = findGeometry(name);
	if (geometry && m_sceneManager) {
		// This would typically zoom to the specific geometry bounds
		// For now, just log that this functionality exists
	}
}

std::shared_ptr<OCCGeometry> OCCViewer::pickGeometry(int x, int y)
{
	if (!m_sceneManager) {
		return nullptr;
	}

	// Convert screen coordinates to world coordinates and get ray
	SbVec3f worldPos;
	if (!m_sceneManager->screenToWorld(wxPoint(x, y), worldPos)) {
		return nullptr;
	}
	
	// Try to use SelectionAcceleratorService for fast picking
	if (m_selectionAcceleratorService && !m_geometries.empty()) {
		// Get camera for ray direction
		gp_Pnt cameraPos = getCameraPosition();
		gp_Pnt clickPos(worldPos[0], worldPos[1], worldPos[2]);
		gp_Vec rayDir(cameraPos, clickPos);
		
		if (rayDir.Magnitude() > 1e-7) {
			rayDir.Normalize();
			
			auto result = m_selectionAcceleratorService->pickByRay(cameraPos, rayDir, m_geometries);
			if (result) {
				return result;
			}
		}
	}

	// Fallback to distance-based picking
	if (m_selectionAcceleratorService) {
		return m_selectionAcceleratorService->pickByDistance(worldPos, m_geometries);
	}

	return nullptr;
}

void OCCViewer::setWireframeMode(bool wireframe)
{
	if (m_renderModeManager) {
		m_renderModeManager->setWireframeMode(wireframe, m_geometries);
	}

	// Request immediate view refresh
	if (m_viewUpdater) m_viewUpdater->requestGeometryChanged(true);
}

void OCCViewer::setShowEdges(bool showEdges)
{
	if (m_renderModeManager) {
		m_renderModeManager->setShowEdges(showEdges, m_edgeDisplayManager.get(), m_meshParams);
	}

	remeshAllGeometries();

	// Immediate refresh
	if (m_viewUpdater) m_viewUpdater->requestEdgesToggled(true);
}

void OCCViewer::setAntiAliasing(bool enabled)
{
	if (m_renderModeManager) {
		m_renderModeManager->setAntiAliasing(enabled);
	}
}

void OCCViewer::setDisplaySettings(const RenderingConfig::DisplaySettings& settings)
{
	// Check if display mode actually changed to avoid unnecessary rebuilds
	bool displayModeChanged = (m_displaySettings.displayMode != settings.displayMode);
	bool edgeSettingsChanged = (m_displaySettings.showEdges != settings.showEdges);
	bool pointViewSettingsChanged = (
		m_displaySettings.showPointView != settings.showPointView ||
		m_displaySettings.showSolidWithPointView != settings.showSolidWithPointView ||
		m_displaySettings.pointSize != settings.pointSize ||
		m_displaySettings.pointColor != settings.pointColor ||
		m_displaySettings.pointShape != settings.pointShape
	);

	m_displaySettings = settings;

	// Only apply changes if something actually changed
	if (displayModeChanged) {
		// Apply display mode to all geometries
		for (auto& geometry : m_geometries) {
			if (geometry) {
				geometry->setDisplayMode(settings.displayMode);
				// Only force rebuild if the display mode change requires it
				if (settings.displayMode == RenderingConfig::DisplayMode::NoShading ||
					settings.displayMode == RenderingConfig::DisplayMode::Points ||
					settings.displayMode == RenderingConfig::DisplayMode::Wireframe) {
					MeshParameters defaultParams;
					geometry->forceCoinRepresentationRebuild(defaultParams);
				}
			}
		}
	}

	// Apply edge display only if changed
	if (edgeSettingsChanged) {
		setShowEdges(settings.showEdges);
	}

	// Apply point view settings only if changed
	if (pointViewSettingsChanged) {
		for (auto& geometry : m_geometries) {
			if (geometry) {
				geometry->setShowPointView(settings.showPointView);
				geometry->setShowSolidWithPointView(settings.showSolidWithPointView);
				geometry->setPointViewSize(settings.pointSize);
				geometry->setPointViewColor(settings.pointColor);
				geometry->setPointViewShape(settings.pointShape);
			}
		}

		// Request view update
		if (m_viewUpdater) {
			m_viewUpdater->requestPointViewToggled(true);
		}
	}

	// Only request refresh if something actually changed
	if (displayModeChanged || edgeSettingsChanged || pointViewSettingsChanged) {
		// Special handling for NoShading mode - ensure lighting is updated
		if (displayModeChanged && settings.displayMode == RenderingConfig::DisplayMode::NoShading) {
			// Force lighting update to clear all lights for NoShading
			if (m_sceneManager) {
				// This will trigger the LightingConfig callback which will detect NoShading mode
				LightingConfig& lightingConfig = LightingConfig::getInstance();
				lightingConfig.applySettingsToScene();
			}
		}

		if (m_viewUpdater) {
			m_viewUpdater->requestRefresh(static_cast<int>(IViewRefresher::Reason::RENDERING_CHANGED), true);
		}
	}
}

const RenderingConfig::DisplaySettings& OCCViewer::getDisplaySettings() const
{
	return m_displaySettings;
}

bool OCCViewer::isPointViewEnabled() const
{
	return m_displaySettings.showPointView;
}

bool OCCViewer::isWireframeMode() const
{
	return m_renderModeManager ? m_renderModeManager->isWireframeMode() : false;
}

bool OCCViewer::isShowEdges() const
{
	return m_renderModeManager ? m_renderModeManager->isShowEdges() : true;
}

bool OCCViewer::isShowNormals() const
{
	return m_normalDisplayService ? m_normalDisplayService->isShowNormals() : false;
}

void OCCViewer::setMeshDeflection(double deflection, bool remesh)
{
	if (m_meshParams.deflection != deflection) {
		m_meshParams.deflection = deflection;
		if (remesh) {
			throttledRemesh("setMeshDeflection");
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
		if (remesh) {
			throttledRemesh("setAngularDeflection");
		}
	}
}

double OCCViewer::getAngularDeflection() const
{
	return m_meshParams.angularDeflection;
}

const MeshParameters& OCCViewer::getMeshParameters() const
{
	return m_meshParams;
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
	
	if (m_normalDisplayService && m_configurationManager) {
		// Sync configuration from ConfigurationManager
		auto& config = m_configurationManager->getNormalDisplayConfig();
		config.showNormals = showNormals;
		m_normalDisplayService->setNormalDisplayConfig(config);
		m_normalDisplayService->setShowNormals(showNormals, m_edgeDisplayManager.get(), m_meshParams);
		
	} else {
		LOG_WRN_S("OCCViewer::setShowNormals - m_normalDisplayService or m_configurationManager is null");
	}
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
		if (refreshManager) {
			refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::NORMALS_TOGGLED, true);
		}
	}
}

void OCCViewer::setNormalLength(double length)
{
	if (m_normalDisplayService && m_configurationManager) {
		auto& config = m_configurationManager->getNormalDisplayConfig();
		config.length = length;
		m_normalDisplayService->setNormalDisplayConfig(config);
		m_normalDisplayService->setNormalLength(length);
	// Force regeneration of normal lines with new length
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->updateAll(m_meshParams);
		}
	}
}

void OCCViewer::setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect)
{
	if (m_normalDisplayService) {
		m_normalDisplayService->setNormalColor(correct, incorrect);
	// Force regeneration of normal lines with new colors
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->updateAll(m_meshParams);
	}
}
}


// Enhanced normal consistency tools
void OCCViewer::setNormalConsistencyMode(bool enabled)
{
	if (m_normalDisplayService && m_configurationManager) {
		auto& config = m_configurationManager->getNormalDisplayConfig();
		config.consistencyMode = enabled;
		m_normalDisplayService->setNormalDisplayConfig(config);
		m_normalDisplayService->setNormalConsistencyMode(enabled);
	
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
}

bool OCCViewer::isNormalConsistencyModeEnabled() const
{
	return m_normalDisplayService ? m_normalDisplayService->isNormalConsistencyModeEnabled() : false;
}

void OCCViewer::setNormalDebugMode(bool enabled)
{
	if (m_normalDisplayService && m_configurationManager) {
		auto& config = m_configurationManager->getNormalDisplayConfig();
		config.debugMode = enabled;
		m_normalDisplayService->setNormalDisplayConfig(config);
		m_normalDisplayService->setNormalDebugMode(enabled);
	
	// Toggle normal display based on debug mode
	setShowNormals(enabled);
	}
}

bool OCCViewer::isNormalDebugModeEnabled() const
{
	return m_normalDisplayService ? m_normalDisplayService->isNormalDebugModeEnabled() : false;
}

void OCCViewer::refreshNormalDisplay()
{
	// Force regeneration of all normal lines
	if (m_normalDisplayService) {
		m_normalDisplayService->refreshNormalDisplay(m_edgeDisplayManager.get(), m_meshParams);
	}
	
	// Refresh view
	if (m_viewUpdater) m_viewUpdater->requestGeometryChanged(true);
}

void OCCViewer::toggleNormalDisplay()
{
	if (m_normalDisplayService) {
		m_normalDisplayService->toggleNormalDisplay(m_edgeDisplayManager.get(), m_meshParams);
}
}


void OCCViewer::remeshAllGeometries()
{
	if (m_meshController) {
		m_meshController->remeshAll();
	}
}

// LOD (Level of Detail) methods
void OCCViewer::setLODEnabled(bool enabled)
{
	if (m_lodEnabled != enabled) {
		m_lodEnabled = enabled;
		if (m_lodController) m_lodController->setEnabled(enabled);
	}
}

bool OCCViewer::isLODEnabled() const { return m_lodEnabled; }

void OCCViewer::setLODRoughDeflection(double deflection)
{
	if (m_lodController && m_lodController->getRoughDeflection() != deflection) {
		m_lodController->setRoughDeflection(deflection);
	}
}

double OCCViewer::getLODRoughDeflection() const { return m_lodController ? m_lodController->getRoughDeflection() : 0.1; }

void OCCViewer::setLODFineDeflection(double deflection)
{
	if (m_lodController && m_lodController->getFineDeflection() != deflection) {
		m_lodController->setFineDeflection(deflection);
	}
}

double OCCViewer::getLODFineDeflection() const { return m_lodController ? m_lodController->getFineDeflection() : 0.01; }

void OCCViewer::setLODTransitionTime(int milliseconds)
{
	if (m_lodController && m_lodController->getTransitionTimeMs() != milliseconds) {
		m_lodController->setTransitionTimeMs(milliseconds);
	}
}

int OCCViewer::getLODTransitionTime() const { return m_lodController ? m_lodController->getTransitionTimeMs() : 500; }

void OCCViewer::setLODMode(bool roughMode)
{
	if (m_lodController) m_lodController->setMode(roughMode);
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

		// Log geometry bounds for debugging
		if (!geometry->getShape().IsNull()) {
			Bnd_Box bbox;
			BRepBndLib::Add(geometry->getShape(), bbox);
			if (!bbox.IsVoid()) {
				double xmin, ymin, zmin, xmax, ymax, zmax;
				bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
			}
		}

		// Store geometry
		m_geometries.push_back(geometry);

		// Don't update slice geometries here - will be updated when slice is enabled

		// Add to object tree sync (batch mode)
		if (m_objectTreeSync) {
			m_objectTreeSync->addGeometry(geometry, true); // true = batch mode
		}

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
	}

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
		return;
	}
	if (m_objectTreeSync) m_objectTreeSync->processDeferred();
}

// Subdivision surface control implementations
void OCCViewer::setSubdivisionEnabled(bool enabled)
{
	if (m_configurationManager) {
		auto& config = m_configurationManager->getSubdivisionConfig();
		updateConfigAndNotify(config.enabled, enabled,
			[&]() { m_meshController->setSubdivisionEnabled(enabled); });
	}
}

bool OCCViewer::isSubdivisionEnabled() const
{
	return m_configurationManager ? m_configurationManager->getSubdivisionConfig().enabled : false;
}

void OCCViewer::setSubdivisionLevel(int level)
{
	if (m_configurationManager && level >= 1 && level <= 5) {
		auto& config = m_configurationManager->getSubdivisionConfig();
		updateConfigAndNotify(config.level, level,
			[&]() { m_meshController->setSubdivisionLevel(level); });
	}
}

int OCCViewer::getSubdivisionLevel() const
{
	return m_configurationManager ? m_configurationManager->getSubdivisionConfig().level : 1;
}

void OCCViewer::setSubdivisionMethod(int method)
{
	if (m_configurationManager && method >= 0 && method <= 3) {
		auto& config = m_configurationManager->getSubdivisionConfig();
		updateConfigAndNotify(config.method, method,
			[&]() { m_meshController->setSubdivisionMethod(method); });
	}
}

int OCCViewer::getSubdivisionMethod() const
{
	return m_configurationManager ? m_configurationManager->getSubdivisionConfig().method : 0;
}

void OCCViewer::setSubdivisionCreaseAngle(double angle)
{
	if (m_configurationManager && angle >= 0.0 && angle <= 180.0) {
		auto& config = m_configurationManager->getSubdivisionConfig();
		updateConfigAndNotify(config.creaseAngle, angle,
			[&]() { m_meshController->setSubdivisionCreaseAngle(angle); });
	}
}

double OCCViewer::getSubdivisionCreaseAngle() const
{
	return m_configurationManager ? m_configurationManager->getSubdivisionConfig().creaseAngle : 30.0;
}

// Mesh smoothing control implementations
void OCCViewer::setSmoothingEnabled(bool enabled)
{
	if (m_configurationManager) {
		auto& config = m_configurationManager->getSmoothingConfig();
		updateConfigAndNotify(config.enabled, enabled,
			[&]() { m_meshController->setSmoothingEnabled(enabled); });
	}
}

bool OCCViewer::isSmoothingEnabled() const
{
	return m_configurationManager ? m_configurationManager->getSmoothingConfig().enabled : false;
}

void OCCViewer::setSmoothingMethod(int method)
{
	if (m_configurationManager && method >= 0 && method <= 3) {
		auto& config = m_configurationManager->getSmoothingConfig();
		updateConfigAndNotify(config.method, method,
			[&]() { m_meshController->setSmoothingMethod(method); });
	}
}

int OCCViewer::getSmoothingMethod() const
{
	return m_configurationManager ? m_configurationManager->getSmoothingConfig().method : 0;
}

void OCCViewer::setSmoothingIterations(int iterations)
{
	if (m_configurationManager && iterations >= 1 && iterations <= 10) {
		auto& config = m_configurationManager->getSmoothingConfig();
		updateConfigAndNotify(config.iterations, iterations,
			[&]() { m_meshController->setSmoothingIterations(iterations); });
	}
}

int OCCViewer::getSmoothingIterations() const
{
	return m_configurationManager ? m_configurationManager->getSmoothingConfig().iterations : 3;
}

void OCCViewer::setSmoothingStrength(double strength)
{
	if (m_configurationManager && strength >= 0.01 && strength <= 1.0) {
		auto& config = m_configurationManager->getSmoothingConfig();
		updateConfigAndNotify(config.strength, strength,
			[&]() { m_meshController->setSmoothingStrength(strength); });
	}
}

double OCCViewer::getSmoothingStrength() const
{
	return m_configurationManager ? m_configurationManager->getSmoothingConfig().strength : 0.5;
}

void OCCViewer::setSmoothingCreaseAngle(double angle)
{
	if (m_configurationManager && angle >= 0.0 && angle <= 180.0) {
		auto& config = m_configurationManager->getSmoothingConfig();
		updateConfigAndNotify(config.creaseAngle, angle,
			[&]() { m_meshController->setSmoothingCreaseAngle(angle); });
	}
}

double OCCViewer::getSmoothingCreaseAngle() const
{
	return m_configurationManager ? m_configurationManager->getSmoothingConfig().creaseAngle : 30.0;
}

// Advanced tessellation control implementations
void OCCViewer::setTessellationMethod(int method)
{
	if (m_configurationManager && method >= 0 && method <= 3) {
		auto& config = m_configurationManager->getTessellationConfig();
		updateConfigAndNotify(config.method, method,
			[&]() { m_meshController->setTessellationMethod(method); });
	}
}

int OCCViewer::getTessellationMethod() const
{
	return m_configurationManager ? m_configurationManager->getTessellationConfig().method : 0;
}

void OCCViewer::setTessellationQuality(int quality)
{
	if (m_configurationManager && quality >= 1 && quality <= 5) {
		auto& config = m_configurationManager->getTessellationConfig();
		updateConfigAndNotify(config.quality, quality,
			[&]() { m_meshController->setTessellationQuality(quality); });
	}
}

int OCCViewer::getTessellationQuality() const
{
	return m_configurationManager ? m_configurationManager->getTessellationConfig().quality : 3;
}

void OCCViewer::setFeaturePreservation(double preservation)
{
	if (m_configurationManager && preservation >= 0.0 && preservation <= 1.0) {
		auto& config = m_configurationManager->getTessellationConfig();
		updateConfigAndNotify(config.featurePreservation, preservation,
			[&]() { m_meshController->setFeaturePreservation(preservation); });
	}
}

double OCCViewer::getFeaturePreservation() const
{
	return m_configurationManager ? m_configurationManager->getTessellationConfig().featurePreservation : 0.5;
}

void OCCViewer::setParallelProcessing(bool enabled)
{
	if (m_configurationManager) {
		auto& config = m_configurationManager->getTessellationConfig();
		updateConfigAndNotify(config.parallelProcessing, enabled,
			[&]() { m_meshController->setParallelProcessing(enabled); });
	}
}

bool OCCViewer::isParallelProcessing() const
{
	return m_configurationManager ? m_configurationManager->getTessellationConfig().parallelProcessing : false;
}

void OCCViewer::setAdaptiveMeshing(bool enabled)
{
	if (m_configurationManager) {
		auto& config = m_configurationManager->getTessellationConfig();
		updateConfigAndNotify(config.adaptiveMeshing, enabled,
			[&]() { m_meshController->setAdaptiveMeshing(enabled); });
	}
}

bool OCCViewer::isAdaptiveMeshing() const
{
	return m_configurationManager ? m_configurationManager->getTessellationConfig().adaptiveMeshing : false;
}

// Mesh quality validation and debugging (delegated to MeshQualityValidator)
void OCCViewer::validateMeshParameters()
{
	if (m_qualityValidator && m_configurationManager) {
		// Set configurations from ConfigurationManager to MeshQualityValidator
		const auto& subdivisionConfig = m_configurationManager->getSubdivisionConfig();
		const auto& smoothingConfig = m_configurationManager->getSmoothingConfig();
		const auto& tessellationConfig = m_configurationManager->getTessellationConfig();

		m_qualityValidator->setSubdivisionParams(subdivisionConfig.enabled, subdivisionConfig.level,
			subdivisionConfig.method, subdivisionConfig.creaseAngle);
		m_qualityValidator->setSmoothingParams(smoothingConfig.enabled, smoothingConfig.method,
			smoothingConfig.iterations, smoothingConfig.strength, smoothingConfig.creaseAngle);
		m_qualityValidator->setTessellationParams(tessellationConfig.method, tessellationConfig.quality,
			tessellationConfig.featurePreservation, tessellationConfig.parallelProcessing, tessellationConfig.adaptiveMeshing);

		m_qualityValidator->validateMeshParameters();
	}
}

void OCCViewer::logCurrentMeshSettings()
{
	if (m_meshQualityService) {
		m_meshQualityService->logCurrentMeshSettings();
	}
}

void OCCViewer::compareMeshQuality(const std::string& geometryName)
{
	if (m_qualityValidator) {
		m_qualityValidator->compareMeshQuality(geometryName);
	}
}

std::string OCCViewer::getMeshQualityReport() const
{
	if (m_meshQualityService) {
		return m_meshQualityService->getMeshQualityReport();
	}
	return "Mesh quality service not initialized";
}

void OCCViewer::exportMeshStatistics(const std::string& filename)
{
	if (m_meshQualityService) {
		m_meshQualityService->exportMeshStatistics(filename);
	}
}

bool OCCViewer::verifyParameterApplication(const std::string& parameterName, double expectedValue)
{
	if (m_qualityValidator) {
		return m_qualityValidator->verifyParameterApplication(parameterName, expectedValue);
	}
	return false;
}

// Real-time parameter monitoring (delegated to MeshQualityService)
void OCCViewer::enableParameterMonitoring(bool enabled)
{
	if (m_meshQualityService) {
		m_meshQualityService->enableParameterMonitoring(enabled);
	}
}

bool OCCViewer::isParameterMonitoringEnabled() const
{
	return m_meshQualityService ? m_meshQualityService->isParameterMonitoringEnabled() : false;
}

void OCCViewer::logParameterChange(const std::string& parameterName, double oldValue, double newValue)
{
	if (m_meshQualityService) {
		m_meshQualityService->logParameterChange(parameterName, oldValue, newValue);
	}
}

// Configuration management (delegated to ConfigurationManager)
void OCCViewer::loadDefaultConfigurations()
{
	if (m_configurationManager) {
		m_configurationManager->loadDefaultConfigurations();
	}
}

bool OCCViewer::loadConfigurationFromFile(const std::string& filename)
{
	if (m_configurationManager) {
		return m_configurationManager->loadConfigurationFromFile(filename);
	}
	return false;
}

bool OCCViewer::saveConfigurationToFile(const std::string& filename) const
{
	if (m_configurationManager) {
		return m_configurationManager->saveConfigurationToFile(filename);
	}
	return false;
}

bool OCCViewer::validateAllConfigurations() const
{
	if (m_configurationManager) {
		return m_configurationManager->validateAllConfigurations();
	}
	return false;
}

std::string OCCViewer::getConfigurationValidationErrors() const
{
	if (m_configurationManager) {
		return m_configurationManager->getValidationErrors();
	}
	return "Configuration manager not initialized";
}

void OCCViewer::applyQualityPreset(const std::string& presetName)
{
	if (m_configurationManager) {
		m_configurationManager->applyQualityPreset(presetName);
	}
}

void OCCViewer::applyPerformancePreset(const std::string& presetName)
{
	if (m_configurationManager) {
		m_configurationManager->applyPerformancePreset(presetName);
	}
}

std::vector<std::string> OCCViewer::getAvailableConfigurationPresets() const
{
	if (m_configurationManager) {
		return m_configurationManager->getAvailablePresets();
	}
	return std::vector<std::string>();
}

void OCCViewer::resetConfigurationsToDefaults()
{
	if (m_configurationManager) {
		m_configurationManager->resetToDefaults();
	}
}

std::string OCCViewer::exportConfigurationAsJson() const
{
	if (m_configurationManager) {
		return m_configurationManager->exportConfigurationAsJson();
	}
	return "{}";
}

bool OCCViewer::importConfigurationFromJson(const std::string& jsonString)
{
	if (m_configurationManager) {
		return m_configurationManager->importConfigurationFromJson(jsonString);
	}
	return false;
}

void OCCViewer::setShowOriginalEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowOriginalEdges(show, m_meshParams);
}

void OCCViewer::computeIntersectionsAsync(
	double tolerance,
	std::function<void(size_t totalPoints, bool success)> onComplete,
	std::function<void(int progress, const std::string& message)> onProgress)
{
	if (!m_edgeDisplayManager) {
		LOG_ERR_S("OCCViewer: EdgeDisplayManager not initialized");
		if (onComplete) {
			onComplete(0, false);
		}
		return;
	}

	auto* frame = dynamic_cast<wxFrame*>(wxTheApp->GetTopWindow());
	if (!frame) {
		LOG_ERR_S("OCCViewer: Cannot get main frame");
		if (onComplete) {
			onComplete(0, false);
		}
		return;
	}

	if (!m_asyncEngine) {
		LOG_ERR_S("OCCViewer: AsyncEngine not initialized");
		if (onComplete) {
			onComplete(0, false);
		}
		return;
	}

	m_edgeDisplayManager->computeIntersectionsAsync(
		tolerance,
		m_asyncEngine.get(),
		onComplete,
		onProgress
	);
}

bool OCCViewer::isIntersectionComputationRunning() const {
	return m_edgeDisplayManager ? m_edgeDisplayManager->isIntersectionComputationRunning() : false;
}

int OCCViewer::getIntersectionProgress() const {
	return m_edgeDisplayManager ? m_edgeDisplayManager->getIntersectionProgress() : 0;
}

void OCCViewer::cancelIntersectionComputation() {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->cancelIntersectionComputation();
	}
}

void OCCViewer::setOriginalEdgesParameters(double samplingDensity, double minLength, bool showLinesOnly, const wxColour& color, double width,
	bool highlightIntersectionNodes, const wxColour& intersectionNodeColor, double intersectionNodeSize, IntersectionNodeShape intersectionNodeShape) {
	// Store parameters in ConfigurationManager
	if (m_configurationManager) {
		auto& config = m_configurationManager->getOriginalEdgesConfig();
		updateConfigValue(config.samplingDensity, samplingDensity);
		updateConfigValue(config.minLength, minLength);
		updateConfigValue(config.showLinesOnly, showLinesOnly);
		updateConfigValue(config.color, color);
		updateConfigValue(config.width, width);
		updateConfigValue(config.highlightIntersectionNodes, highlightIntersectionNodes);
		updateConfigValue(config.intersectionNodeColor, intersectionNodeColor);
		updateConfigValue(config.intersectionNodeSize, intersectionNodeSize);
		// Note: intersectionNodeShape is not stored in config yet, could be added later
	
	// Convert wxColour to Quantity_Color
	Quantity_Color occColor(color.Red() / 255.0, color.Green() / 255.0, color.Blue() / 255.0, Quantity_TOC_RGB);
	Quantity_Color intersectionNodeOccColor(intersectionNodeColor.Red() / 255.0, intersectionNodeColor.Green() / 255.0, intersectionNodeColor.Blue() / 255.0, Quantity_TOC_RGB);
	
	// Apply parameters to EdgeDisplayManager
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->setOriginalEdgesParameters(samplingDensity, minLength, showLinesOnly, occColor, width,
			highlightIntersectionNodes, intersectionNodeOccColor, intersectionNodeSize, intersectionNodeShape);
	}
	
	// Refresh the view
	requestViewRefresh();
	}
}
void OCCViewer::setShowFeatureEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowFeatureEdges(show, m_meshParams);
}

void OCCViewer::applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->applyFeatureEdgeAppearance(color, width, edgesOnly, m_meshParams);
}

void OCCViewer::applyFeatureEdgeAppearance(const Quantity_Color& color, double width, int style, bool edgesOnly) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->applyFeatureEdgeAppearance(color, width, style, edgesOnly);
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
	m_explodeParams.primaryMode = mode;
	m_explodeParams.baseFactor = factor;
	if (!m_explodeController) m_explodeController = std::make_unique<ExplodeController>(m_occRoot);
	m_explodeController->setParams(mode, factor);
	if (m_explodeEnabled) {
		applyExplode();
		if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
	}
}

void OCCViewer::setExplodeParamsAdvanced(const ExplodeParams& params) {
	m_explodeParams = params;
	// Map to legacy fields for backward compatibility
	m_explodeMode = params.primaryMode;
	m_explodeFactor = params.baseFactor;
	if (!m_explodeController) m_explodeController = std::make_unique<ExplodeController>(m_occRoot);
	// Reuse simple controller params; controller will read advanced params via viewer when needed (future)
	m_explodeController->setParams(m_explodeMode, m_explodeFactor);
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
	
	// Update geometries list before enabling
	if (enabled) {
		updateSliceGeometries();
	}
	
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

void OCCViewer::setShowSectionContours(bool show) {
	if (!m_sliceController) m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	m_sliceController->setShowSectionContours(show);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

bool OCCViewer::isShowSectionContours() const {
	return m_sliceController ? m_sliceController->isShowSectionContours() : false;
}

void OCCViewer::setSlicePlaneColor(const SbVec3f& color) {
	if (!m_sliceController) m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	m_sliceController->setPlaneColor(color);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

const SbVec3f& OCCViewer::getSlicePlaneColor() const {
	static const SbVec3f defaultColor(0.7f, 0.95f, 0.7f); // Light green
	return m_sliceController ? m_sliceController->getPlaneColor() : defaultColor;
}

void OCCViewer::setSlicePlaneOpacity(float opacity) {
	if (!m_sliceController) m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	m_sliceController->setPlaneOpacity(opacity);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

float OCCViewer::getSlicePlaneOpacity() const {
	return m_sliceController ? m_sliceController->getPlaneOpacity() : 0.85f;
}

void OCCViewer::setSliceGeometries(const std::vector<OCCGeometry*>& geometries) {
	if (!m_sliceController) m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	m_sliceController->setGeometries(geometries);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh();
}

void OCCViewer::updateSliceGeometries() {
	if (m_sliceController) {
		std::vector<OCCGeometry*> geomPtrs;
		geomPtrs.reserve(m_geometries.size());
		for (const auto& geom : m_geometries) {
			if (geom) {  // Check for valid pointer
				geomPtrs.push_back(geom.get());
			}
		}
		m_sliceController->setGeometries(geomPtrs);
	}
}

bool OCCViewer::handleSliceMousePress(const SbVec2s* mousePos, const SbViewportRegion* vp) {
	return m_sliceController ? m_sliceController->handleMousePress(mousePos, vp) : false;
}

bool OCCViewer::handleSliceMouseMove(const SbVec2s* mousePos, const SbViewportRegion* vp) {
	return m_sliceController ? m_sliceController->handleMouseMove(mousePos, vp) : false;
}

bool OCCViewer::handleSliceMouseRelease(const SbVec2s* mousePos, const SbViewportRegion* vp) {
	return m_sliceController ? m_sliceController->handleMouseRelease(mousePos, vp) : false;
}

bool OCCViewer::isSliceInteracting() const {
	return m_sliceController ? m_sliceController->isInteracting() : false;
}

void OCCViewer::setSliceDragEnabled(bool enabled) {
	if (!m_sliceController) m_sliceController = std::make_unique<SliceController>(m_sceneManager, m_occRoot);
	m_sliceController->setDragEnabled(enabled);
}

bool OCCViewer::isSliceDragEnabled() const {
	return m_sliceController ? m_sliceController->isDragEnabled() : false;
}

void OCCViewer::setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave) {
	if (m_edgeDisplayManager) {
		// Use default color and width for backward compatibility
		Quantity_Color defaultColor(1.0, 0.0, 0.0, Quantity_TOC_RGB); // Red
		m_edgeDisplayManager->setShowFeatureEdges(show, featureAngleDeg, minLength, onlyConvex, onlyConcave, m_meshParams, defaultColor, 2.0);
	}
}

void OCCViewer::setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave,
	const Quantity_Color& color, double width) {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->setShowFeatureEdges(show, featureAngleDeg, minLength, onlyConvex, onlyConcave, m_meshParams, color, width);
	}
}
void OCCViewer::setShowMeshEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowMeshEdges(show, m_meshParams);
}
void OCCViewer::setShowHighlightEdges(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowHighlightEdges(show, m_meshParams);
}
void OCCViewer::setShowNormalLines(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowNormalLines(show, m_meshParams);
}

void OCCViewer::setShowFaceNormalLines(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowFaceNormalLines(show, m_meshParams);
}

void OCCViewer::setShowIntersectionNodes(bool show) {
	if (m_edgeDisplayManager) m_edgeDisplayManager->setShowIntersectionNodes(show, m_meshParams);
}

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
	case EdgeType::IntersectionNodes: return flags.showIntersectionNodes;
	}
	return false;
}

void OCCViewer::updateAllEdgeDisplays() {
	if (m_edgeDisplayManager) m_edgeDisplayManager->updateAll(m_meshParams);
}

void OCCViewer::forceRegenerateMeshDerivedEdges(const MeshParameters& meshParams) {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->updateAll(meshParams, true); // true = force mesh regeneration
	}
}

void OCCViewer::rebuildSelectionAccelerator()
{
	if (m_selectionAcceleratorService) {
		m_selectionAcceleratorService->rebuildFromGeometries(m_geometries);
	}
}

// Configuration synchronization helpers
template<typename T, typename Func>
void OCCViewer::updateConfigAndNotify(T& configValue, T newValue, Func&& meshControllerFunc)
{
	if (configValue == newValue) return;
	configValue = newValue;
	if (m_meshController) {
		meshControllerFunc();
	}
}

template<typename T>
void OCCViewer::updateConfigValue(T& configValue, T newValue)
{
	configValue = newValue;
}

// Throttled remeshing helper to avoid excessive remesh operations
void OCCViewer::throttledRemesh(const std::string& context)
{
	static wxLongLong lastRemeshTime = 0;
	wxLongLong currentTime = wxGetLocalTimeMillis();
	const int MIN_REMESH_INTERVAL = 200; // Minimum 200ms between remesh operations

	if (currentTime - lastRemeshTime >= MIN_REMESH_INTERVAL) {
		try {
			remeshAllGeometries();
			lastRemeshTime = currentTime;
			// Request view refresh after remeshing
			if (m_viewUpdater) {
				m_viewUpdater->requestGeometryChanged(true);
			}
		}
		catch (const std::exception& e) {
			LOG_ERR_S("OCCViewer::" + context + ": Exception during remesh: " + std::string(e.what()));
			// Don't update lastRemeshTime if remesh failed
		}
		catch (...) {
			LOG_ERR_S("OCCViewer::" + context + ": Unknown exception during remesh");
			// Don't update lastRemeshTime if remesh failed
		}
	}
}

// Simplified Edge Display APIs - unified facade interface
void OCCViewer::setEdgeDisplayMode(EdgeType type, bool show) {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->toggleEdgeType(type, show, m_meshParams);
	}
}

void OCCViewer::configureEdgeDisplay(const EdgeDisplayConfig& config) {
	// Apply configuration through individual setters for backward compatibility
	setShowOriginalEdges(config.showOriginalEdges);
	setShowFeatureEdges(config.showFeatureEdges);
	setShowMeshEdges(config.showMeshEdges);
	setShowNormalLines(config.showNormalLines);
	setShowFaceNormalLines(config.showFaceNormalLines);
	setShowHighlightEdges(config.showHighlightEdges);
	setShowIntersectionNodes(config.showIntersectionNodes);
}

const EdgeDisplayFlags& OCCViewer::getEdgeDisplayFlags() const {
	return m_edgeDisplayManager ? m_edgeDisplayManager->getFlags() : globalEdgeFlags;
}

// Advanced geometry creation - delegated to GeometryFactoryService
std::shared_ptr<OCCGeometry> OCCViewer::addGeometryWithAdvancedRendering(const TopoDS_Shape& shape, const std::string& name)
{
	if (m_geometryFactoryService) {
		auto geometry = m_geometryFactoryService->addGeometryWithAdvancedRendering(shape, name);
		if (geometry && m_geometryManagementService) {
			// Add to scene using geometry management service
			m_geometryManagementService->addGeometry(geometry, m_batchOperationActive);
		}
		return geometry;
	}
	return nullptr;
}

std::shared_ptr<OCCGeometry> OCCViewer::addBezierCurve(const std::vector<gp_Pnt>& controlPoints, const std::string& name)
{
	if (m_geometryFactoryService) {
		auto geometry = m_geometryFactoryService->addBezierCurve(controlPoints, name);
		if (geometry && m_geometryManagementService) {
			// Add to scene using geometry management service
			m_geometryManagementService->addGeometry(geometry, m_batchOperationActive);
		}
		return geometry;
	}
	return nullptr;
}

std::shared_ptr<OCCGeometry> OCCViewer::addBezierSurface(const std::vector<std::vector<gp_Pnt>>& controlPoints, const std::string& name)
{
	if (m_geometryFactoryService) {
		auto geometry = m_geometryFactoryService->addBezierSurface(controlPoints, name);
		if (geometry && m_geometryManagementService) {
			// Add to scene using geometry management service
			m_geometryManagementService->addGeometry(geometry, m_batchOperationActive);
		}
		return geometry;
	}
	return nullptr;
}

std::shared_ptr<OCCGeometry> OCCViewer::addBSplineCurve(const std::vector<gp_Pnt>& poles, const std::vector<double>& weights, const std::string& name)
{
	if (m_geometryFactoryService) {
		auto geometry = m_geometryFactoryService->addBSplineCurve(poles, weights, name);
		if (geometry && m_geometryManagementService) {
			// Add to scene using geometry management service
			m_geometryManagementService->addGeometry(geometry, m_batchOperationActive);
		}
		return geometry;
	}
	return nullptr;
}

// Wireframe appearance methods
void OCCViewer::applyWireframeAppearance(const Quantity_Color& color, double width, int style, bool showOnlyNew) {
	// Apply wireframe appearance to all geometries
	for (auto& g : m_geometries) {
		if (g) {
			g->setWireframeWidth(width);
			g->setWireframeColor(color);
			// Update the Coin3D material if the geometry is in wireframe mode
			if (g->isWireframeMode()) {
				g->updateWireframeMaterial(color);
			}
		}
	}

	// Store in edge display manager for reference
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->applyWireframeAppearance(color, width, style, showOnlyNew);
	}

	// Request view refresh
	if (m_viewUpdater) m_viewUpdater->requestGeometryChanged(true);
}

void OCCViewer::setWireframeAppearance(const EdgeDisplayManager::WireframeAppearance& appearance) {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->setWireframeAppearance(appearance);
	}
}

// Mesh edges appearance methods
void OCCViewer::applyMeshEdgeAppearance(const Quantity_Color& color, double width, int style, bool showOnlyNew) {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->applyMeshEdgeAppearance(color, width, style, showOnlyNew);
	}
}

void OCCViewer::setMeshEdgeAppearance(const EdgeDisplayManager::MeshEdgeAppearance& appearance) {
	if (m_edgeDisplayManager) {
		m_edgeDisplayManager->setMeshEdgeAppearance(appearance);
	}
}