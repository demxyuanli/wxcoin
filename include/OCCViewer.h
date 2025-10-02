#pragma once

#include "OCCGeometry.h"
#include "rendering/GeometryProcessor.h"
#include "ViewRefreshManager.h"
#include <vector>
#include <memory>
#include <wx/wx.h>
#include <wx/timer.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"
#include "edges/EdgeDisplayManager.h"
#include "viewer/interfaces/IGeometryApi.h"
#include "viewer/interfaces/ISelectionApi.h"
#include "viewer/interfaces/IRenderModesApi.h"
#include "viewer/interfaces/IMeshControlApi.h"
#include "viewer/interfaces/ILODApi.h"
#include "viewer/interfaces/IEdgeDisplayApi.h"
#include "viewer/interfaces/ISliceApi.h"
#include "viewer/interfaces/IExplodeApi.h"
#include "viewer/interfaces/IViewApi.h"
#include "viewer/interfaces/IOutlineApi.h"
#include "viewer/OutlineDisplayManager.h"
#include "viewer/ImageOutlinePass.h"
#include <unordered_map>
#include <atomic>
#include <thread>
#include "viewer/ExplodeTypes.h"

// Forward declarations
class OCCGeometry;
class SoSeparator;
class SoClipPlane;
class SoTransform;
class SceneManager;
class SliceController; // forward declaration for slice feature
class ExplodeController; // forward declaration for explode feature
class LODController; // forward declaration for LOD
class PickingService; // forward declaration for picking
class SelectionManager; // selection and visibility
class ObjectTreeSync;   // object tree synchronization
class GeometryRepository; // storage wrapper
class SceneAttachmentService; // attach/detach helpers
class ViewUpdateService; // view/canvas refresh
class MeshParameterController; // mesh params + remesh
class MeshingService; // apply + regenerate
class HoverSilhouetteManager; // hover silhouette
class BatchOperationManager; // batch updates

/**
 * @brief OpenCASCADE viewer integration
 *
 * Manages OpenCASCADE geometry objects display in 3D scene
 */
class OCCViewer : public wxEvtHandler,
	public IGeometryApi,
	public ISelectionApi,
	public IRenderModesApi,
	public IMeshControlApi,
	public ILODApi,
	public IEdgeDisplayApi,
	public ISliceApi,
	public IExplodeApi,
	public IViewApi,
	public IOutlineApi
{
public:
	explicit OCCViewer(SceneManager* sceneManager);
	~OCCViewer();

	// Type alias to keep backward compatibility with existing OCCViewer::ExplodeMode references
	using ExplodeMode = ::ExplodeMode;

	// Geometry management
	void addGeometry(std::shared_ptr<OCCGeometry> geometry) override;
	void removeGeometry(std::shared_ptr<OCCGeometry> geometry) override;
	void removeGeometry(const std::string& name) override;
	void clearAll() override;
	std::shared_ptr<OCCGeometry> findGeometry(const std::string& name) override;
	std::vector<std::shared_ptr<OCCGeometry>> getAllGeometry() const override;
	std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometries() const override;

	// Batch operations for performance optimization
	void beginBatchOperation();
	void endBatchOperation();
	bool isBatchOperationActive() const;

	// Batch geometry addition for better performance
	void addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) override;

	// Deferred ObjectTree updates
	void updateObjectTreeDeferred() override;

	// View manipulation
	void setGeometryVisible(const std::string& name, bool visible) override;
	void setGeometrySelected(const std::string& name, bool selected) override;
	void setGeometryColor(const std::string& name, const Quantity_Color& color) override;
	void setGeometryTransparency(const std::string& name, double transparency) override;
	void hideAll() override;
	void showAll() override;
	void selectAll() override;
	void deselectAll() override;
	void setAllColor(const Quantity_Color& color);
	void fitAll() override;
	void fitGeometry(const std::string& name) override;

	// Picking
	std::shared_ptr<OCCGeometry> pickGeometry(int x, int y);

	// Display modes
	void setWireframeMode(bool wireframe) override;
	void setShowEdges(bool showEdges) override;
	void setAntiAliasing(bool enabled) override;
	bool isWireframeMode() const override;
	bool isShowEdges() const override;
	bool isShowNormals() const;

	// Mesh quality
	void setMeshDeflection(double deflection, bool remesh = true) override;
	double getMeshDeflection() const override;
	void setAngularDeflection(double deflection, bool remesh = true) override;
	double getAngularDeflection() const override;

	// LOD (Level of Detail) control
	void setLODEnabled(bool enabled) override;
	bool isLODEnabled() const override;
	void setLODRoughDeflection(double deflection) override;
	double getLODRoughDeflection() const override;
	void setLODFineDeflection(double deflection) override;
	double getLODFineDeflection() const override;
	void setLODTransitionTime(int milliseconds) override;
	int getLODTransitionTime() const override;
	void setLODMode(bool roughMode) override;
	bool isLODRoughMode() const override;
	void startLODInteraction() override;

	// Subdivision surface control
	void setSubdivisionEnabled(bool enabled) override;
	bool isSubdivisionEnabled() const override;
	void setSubdivisionLevel(int level) override;
	int getSubdivisionLevel() const override;
	void setSubdivisionMethod(int method) override;
	int getSubdivisionMethod() const override;
	void setSubdivisionCreaseAngle(double angle) override;
	double getSubdivisionCreaseAngle() const override;

	// Mesh smoothing control
	void setSmoothingEnabled(bool enabled) override;
	bool isSmoothingEnabled() const override;
	void setSmoothingMethod(int method) override;
	int getSmoothingMethod() const override;
	void setSmoothingIterations(int iterations) override;
	int getSmoothingIterations() const override;
	void setSmoothingStrength(double strength) override;
	double getSmoothingStrength() const override;
	void setSmoothingCreaseAngle(double angle) override;
	double getSmoothingCreaseAngle() const override;

	// Advanced tessellation control
	void setTessellationMethod(int method) override;
	int getTessellationMethod() const override;
	void setTessellationQuality(int quality) override;
	int getTessellationQuality() const override;
	void setFeaturePreservation(double preservation) override;
	double getFeaturePreservation() const override;
	void setParallelProcessing(bool enabled) override;
	bool isParallelProcessing() const override;
	void setAdaptiveMeshing(bool enabled) override;
	bool isAdaptiveMeshing() const override;

	// Callbacks
	void onSelectionChanged();
	void onGeometryChanged(std::shared_ptr<OCCGeometry> geometry);

	// Normals display
	void setShowNormals(bool showNormals);
	void setNormalLength(double length);
	void setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect);
	void updateNormalsDisplay();
	
	// Enhanced normal consistency tools
	void setNormalConsistencyMode(bool enabled);
	bool isNormalConsistencyModeEnabled() const;
	void setNormalDebugMode(bool enabled);
	bool isNormalDebugModeEnabled() const;
	void refreshNormalDisplay();
	void toggleNormalDisplay();

	// View refresh
	void requestViewRefresh() override;

	// Advanced geometry creation - NEW
	std::shared_ptr<OCCGeometry> addGeometryWithAdvancedRendering(const TopoDS_Shape& shape, const std::string& name);
	std::shared_ptr<OCCGeometry> addBezierCurve(const std::vector<gp_Pnt>& controlPoints, const std::string& name);
	std::shared_ptr<OCCGeometry> addBezierSurface(const std::vector<std::vector<gp_Pnt>>& controlPoints, const std::string& name);
	std::shared_ptr<OCCGeometry> addBSplineCurve(const std::vector<gp_Pnt>& poles, const std::vector<double>& weights, const std::string& name);

	// Upgrade existing geometries - NEW
	void upgradeGeometryToAdvanced(const std::string& name);
	void upgradeAllGeometriesToAdvanced();

	// Mesh quality validation and debugging
	void validateMeshParameters();
	void logCurrentMeshSettings();
	void compareMeshQuality(const std::string& geometryName);
	std::string getMeshQualityReport() const;
	void exportMeshStatistics(const std::string& filename);
	bool verifyParameterApplication(const std::string& parameterName, double expectedValue);

	// Real-time parameter monitoring
	void enableParameterMonitoring(bool enabled);
	bool isParameterMonitoringEnabled() const;
	void logParameterChange(const std::string& parameterName, double oldValue, double newValue);

	// Force mesh regeneration
	void remeshAllGeometries();

	// Removed legacy display flag APIs (now handled per-geometry and via EdgeDisplayManager)

	// View behavior controls
	void setPreserveViewOnAdd(bool preserve) override { m_preserveViewOnAdd = preserve; }
	bool isPreserveViewOnAdd() const override { return m_preserveViewOnAdd; }

	EdgeDisplayFlags globalEdgeFlags;
	void setShowOriginalEdges(bool show) override;
	void setOriginalEdgesParameters(double samplingDensity, double minLength, bool showLinesOnly, const wxColour& color, double width);
	void setShowFeatureEdges(bool show) override;
	void setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave) override;
	void setShowMeshEdges(bool show) override;
	void setShowHighlightEdges(bool show) override;
	void setShowNormalLines(bool show) override;
	void setShowFaceNormalLines(bool show) override;
	// Outline API
	void setOutlineEnabled(bool enabled) override { if (m_outlineManager) m_outlineManager->setEnabled(enabled); }
	bool isOutlineEnabled() const override { return m_outlineManager ? m_outlineManager->isEnabled() : false; }
	void refreshOutlineAll() override { if (m_outlineManager) m_outlineManager->updateAll(); }
	ImageOutlineParams getOutlineParams() const { return m_outlineManager ? m_outlineManager->getParams() : ImageOutlineParams{}; }
	void setOutlineParams(const ImageOutlineParams& p) { if (m_outlineManager) m_outlineManager->setParams(p); }
	void toggleEdgeType(EdgeType type, bool show) override;
	bool isEdgeTypeEnabled(EdgeType type) const override;
	void updateAllEdgeDisplays() override;
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly) override;
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, int style, bool edgesOnly);

	// Slice (clipping plane) control
	void setSliceEnabled(bool enabled) override;
	bool isSliceEnabled() const override;
	void setSlicePlane(const SbVec3f& normal, float offset) override;
	void moveSliceAlongNormal(float delta) override;
	SbVec3f getSliceNormal() const override;
	float getSliceOffset() const override;

	// Assembly explode view
	void setExplodeEnabled(bool enabled, double factor = 1.0) override;
	bool isExplodeEnabled() const override { return m_explodeEnabled; }
	void setExplodeParams(ExplodeMode mode, double factor) override;
	void getExplodeParams(ExplodeMode& mode, double& factor) const override { mode = m_explodeMode; factor = m_explodeFactor; }
	void setExplodeParamsAdvanced(const ExplodeParams& params) override;
	ExplodeParams getExplodeParamsAdvanced() const override { return m_explodeParams; }

	// Feature edges status/progress (delegated to manager)
	struct FeatureEdgeParams { double angleDeg{ 15.0 }; double minLength{ 0.005 }; bool onlyConvex{ false }; bool onlyConcave{ false }; };
	bool isFeatureEdgeGenerationRunning() const { return m_edgeDisplayManager && m_edgeDisplayManager->isFeatureEdgeGenerationRunning(); }
	int getFeatureEdgeProgress() const { return m_edgeDisplayManager ? m_edgeDisplayManager->getFeatureEdgeProgress() : 0; }
	bool hasFeatureEdgeCache() const { return m_edgeDisplayManager ? m_edgeDisplayManager->hasFeatureEdgeCache() : false; }
	FeatureEdgeParams getLastFeatureEdgeParams() const { return m_lastFeatureParams; }

	gp_Pnt getCameraPosition() const override;
	SoSeparator* getRootSeparator() const { return m_occRoot; }

	// Hover silhouette API (screen-space driven)
	void updateHoverSilhouetteAt(const wxPoint& screenPos);

private:
	void initializeViewer();
	void onLODTimer();
	void createNormalVisualization(std::shared_ptr<OCCGeometry> geometry);
	static bool approximatelyEqual(double a, double b, double eps = 1e-6) { return std::abs(a - b) <= eps; }
	void invalidateFeatureEdgeCache();

	SceneManager* m_sceneManager;
	SoSeparator* m_occRoot;
	SoSeparator* m_normalRoot;

	std::vector<std::shared_ptr<OCCGeometry>> m_geometries;
	std::vector<std::shared_ptr<OCCGeometry>> m_selectedGeometries;

	bool m_wireframeMode;
	bool m_shadingMode;
	bool m_showEdges;
	bool m_antiAliasing;

	MeshParameters m_meshParams;

	// LOD settings (controller-backed)
	bool m_lodEnabled;
	std::unique_ptr<LODController> m_lodController;

	// Subdivision settings
	bool m_subdivisionEnabled;
	int m_subdivisionLevel;
	int m_subdivisionMethod;
	double m_subdivisionCreaseAngle;

	// Smoothing settings
	bool m_smoothingEnabled;
	int m_smoothingMethod;
	int m_smoothingIterations;
	double m_smoothingStrength;
	double m_smoothingCreaseAngle;

	// Advanced tessellation settings
	int m_tessellationMethod;
	int m_tessellationQuality;
	double m_featurePreservation;
	bool m_parallelProcessing;
	bool m_adaptiveMeshing;

	// Normal display settings
	bool m_showNormals;
	double m_normalLength;
	Quantity_Color m_correctNormalColor;
	Quantity_Color m_incorrectNormalColor;
	
	// Normal consistency settings
	bool m_normalConsistencyMode;
	bool m_normalDebugMode;

	// Original edges parameters
	double m_originalEdgesSamplingDensity = 80.0;
	double m_originalEdgesMinLength = 0.01;
	bool m_originalEdgesShowLinesOnly = false;
	wxColour m_originalEdgesColor = wxColour(255, 255, 255);
	double m_originalEdgesWidth = 1.0;

	Quantity_Color m_defaultColor;
	double m_defaultTransparency;

	// Batch operation state
	bool m_batchOperationActive;
	bool m_needsViewRefresh;
	bool m_preserveViewOnAdd{ true };
	std::unique_ptr<BatchOperationManager> m_batchManager;

	// Performance optimization
	bool m_meshRegenerationNeeded;
	MeshParameters m_lastMeshParams;

	// Deferred ObjectTree updates
	std::vector<std::shared_ptr<OCCGeometry>> m_pendingObjectTreeUpdates;

	// Parameter monitoring
	bool m_parameterMonitoringEnabled;

	std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>> m_nodeToGeom;
	std::shared_ptr<OCCGeometry> pickGeometryAtScreen(const wxPoint& screenPos);
	void setHoveredSilhouette(std::shared_ptr<OCCGeometry> geometry);
	std::unique_ptr<HoverSilhouetteManager> m_hoverManager;

	// Explode state (controller-backed)
	bool m_explodeEnabled{ false };
	double m_explodeFactor{ 1.0 };
	ExplodeMode m_explodeMode{ ExplodeMode::Radial };
	ExplodeParams m_explodeParams{};
	void applyExplode();
	void clearExplode();

	// Slice controller (encapsulated)
	std::unique_ptr<SliceController> m_sliceController;
	std::unique_ptr<ExplodeController> m_explodeController;
	std::unique_ptr<PickingService> m_pickingService;
	std::unique_ptr<SelectionManager> m_selectionManager;
	std::unique_ptr<ObjectTreeSync> m_objectTreeSync;
	std::unique_ptr<GeometryRepository> m_geometryRepo;
	std::unique_ptr<SceneAttachmentService> m_sceneAttach;
	std::unique_ptr<ViewUpdateService> m_viewUpdater;
	std::unique_ptr<MeshingService> m_meshingService;
	std::unique_ptr<MeshParameterController> m_meshController;
	std::unique_ptr<OutlineDisplayManager> m_outlineManager;
	std::unique_ptr<class SelectionOutlineManager> m_selectionOutline;

	// Legacy draw helpers removed; edge/normal rendering is handled elsewhere

	// Feature edge parameters (cache/progress moved to manager)
	FeatureEdgeParams m_lastFeatureParams;

	// New manager to centralize edge display
	std::unique_ptr<EdgeDisplayManager> m_edgeDisplayManager;
};