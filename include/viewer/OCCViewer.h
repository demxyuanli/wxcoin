#pragma once

#include "geometry/OCCGeometry.h"
#include "rendering/GeometryProcessor.h"
#include "ViewRefreshManager.h"
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
#include "viewer/ExplodeTypes.h"
#include <vector>
#include <memory>
#include <wx/wx.h>

// Forward declarations
class SceneManager;
class SoSeparator;
class ViewportController;
class RenderingController;
class LODController;
class SliceController;
class ExplodeController;
class PickingService;
class SelectionManager;
class ObjectTreeSync;
class GeometryRepository;
class SceneAttachmentService;
class ViewUpdateService;
class MeshingService;
class MeshParameterController;
class EdgeDisplayManager;
class HoverSilhouetteManager;
class BatchOperationManager;
class OutlineDisplayManager;
class SelectionOutlineManager;
class ImageOutlineParams;

/**
 * @brief OpenCASCADE viewer integration - Refactored modular version
 *
 * Main viewer class that delegates to specialized controllers and managers.
 * Uses composition pattern to organize complex functionality.
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

	// Type alias for backward compatibility
	using ExplodeMode = ::ExplodeMode;

	// ===== Geometry Management API (IGeometryApi) =====
	void addGeometry(std::shared_ptr<OCCGeometry> geometry) override;
	void removeGeometry(std::shared_ptr<OCCGeometry> geometry) override;
	void removeGeometry(const std::string& name) override;
	void clearAll() override;
	std::shared_ptr<OCCGeometry> findGeometry(const std::string& name) override;
	std::vector<std::shared_ptr<OCCGeometry>> getAllGeometry() const override;
	std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometries() const override;
	void addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) override;
	void updateObjectTreeDeferred() override;

	// ===== Selection and Visibility API (ISelectionApi) =====
	void setGeometryVisible(const std::string& name, bool visible) override;
	void setGeometrySelected(const std::string& name, bool selected) override;
	void setGeometryColor(const std::string& name, const Quantity_Color& color) override;
	void setGeometryTransparency(const std::string& name, double transparency) override;
	void hideAll() override;
	void showAll() override;
	void selectAll() override;
	void deselectAll() override;
	void setAllColor(const Quantity_Color& color);
	std::shared_ptr<OCCGeometry> pickGeometry(int x, int y);

	// ===== Rendering and Display Modes API (IRenderModesApi) =====
	void setWireframeMode(bool wireframe) override;
	void setShowEdges(bool showEdges) override;
	void setAntiAliasing(bool enabled) override;
	bool isWireframeMode() const override;
	bool isShowEdges() const override;
	bool isShowNormals() const;
	void setShowNormals(bool showNormals);
	void setNormalLength(double length);
	void setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect);
	void updateNormalsDisplay();
	void setNormalConsistencyMode(bool enabled);
	bool isNormalConsistencyModeEnabled() const;
	void setNormalDebugMode(bool enabled);
	bool isNormalDebugModeEnabled() const;
	void refreshNormalDisplay();
	void toggleNormalDisplay();

	// ===== View Control API (IViewApi) =====
	void fitAll() override;
	void fitGeometry(const std::string& name) override;
	void requestViewRefresh() override;
	void setPreserveViewOnAdd(bool preserve) override;
	bool isPreserveViewOnAdd() const override;
	gp_Pnt getCameraPosition() const override;

	// ===== Mesh Control API (IMeshControlApi) =====
	void setMeshDeflection(double deflection, bool remesh = true) override;
	double getMeshDeflection() const override;
	void setAngularDeflection(double deflection, bool remesh = true) override;
	double getAngularDeflection() const override;
	const MeshParameters& getMeshParameters() const;

	void setSubdivisionEnabled(bool enabled) override;
	bool isSubdivisionEnabled() const override;
	void setSubdivisionLevel(int level) override;
	int getSubdivisionLevel() const override;
	void setSubdivisionMethod(int method) override;
	int getSubdivisionMethod() const override;
	void setSubdivisionCreaseAngle(double angle) override;
	double getSubdivisionCreaseAngle() const override;

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

	void remeshAllGeometries();
	void validateMeshParameters();
	void logCurrentMeshSettings();

	// ===== LOD API (ILODApi) =====
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

	// ===== Edge Display API (IEdgeDisplayApi) =====
	EdgeDisplayFlags globalEdgeFlags;
	void setShowOriginalEdges(bool show) override;
	void setOriginalEdgesParameters(double samplingDensity, double minLength, bool showLinesOnly, 
		const wxColour& color, double width, bool highlightIntersectionNodes = false, 
		const wxColour& intersectionNodeColor = wxColour(255, 0, 0), double intersectionNodeSize = 3.0);
	void setShowFeatureEdges(bool show) override;
	void setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, 
		bool onlyConvex, bool onlyConcave) override;
	void setShowMeshEdges(bool show) override;
	void setShowHighlightEdges(bool show) override;
	void setShowNormalLines(bool show) override;
	void setShowFaceNormalLines(bool show) override;
	void toggleEdgeType(EdgeType type, bool show) override;
	bool isEdgeTypeEnabled(EdgeType type) const override;
	void updateAllEdgeDisplays() override;
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly) override;
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, int style, bool edgesOnly);

	// Feature edge status/progress
	struct FeatureEdgeParams { 
		double angleDeg{ 15.0 }; 
		double minLength{ 0.005 }; 
		bool onlyConvex{ false }; 
		bool onlyConcave{ false }; 
	};
	bool isFeatureEdgeGenerationRunning() const;
	int getFeatureEdgeProgress() const;
	bool hasFeatureEdgeCache() const;
	FeatureEdgeParams getLastFeatureEdgeParams() const { return m_lastFeatureParams; }

	// ===== Slice API (ISliceApi) =====
	void setSliceEnabled(bool enabled) override;
	bool isSliceEnabled() const override;
	void setSlicePlane(const SbVec3f& normal, float offset) override;
	void moveSliceAlongNormal(float delta) override;
	SbVec3f getSliceNormal() const override;
	float getSliceOffset() const override;

	// ===== Explode API (IExplodeApi) =====
	void setExplodeEnabled(bool enabled, double factor = 1.0) override;
	bool isExplodeEnabled() const override;
	void setExplodeParams(ExplodeMode mode, double factor) override;
	void getExplodeParams(ExplodeMode& mode, double& factor) const override;
	void setExplodeParamsAdvanced(const ExplodeParams& params) override;
	ExplodeParams getExplodeParamsAdvanced() const override;

	// ===== Outline API (IOutlineApi) =====
	void setOutlineEnabled(bool enabled) override;
	bool isOutlineEnabled() const override;
	void refreshOutlineAll() override;
	ImageOutlineParams getOutlineParams() const;
	void setOutlineParams(const ImageOutlineParams& p);

	// ===== Batch Operations =====
	void beginBatchOperation();
	void endBatchOperation();
	bool isBatchOperationActive() const;

	// ===== Callbacks =====
	void onSelectionChanged();
	void onGeometryChanged(std::shared_ptr<OCCGeometry> geometry);

	// ===== Advanced Geometry Creation =====
	std::shared_ptr<OCCGeometry> addGeometryWithAdvancedRendering(
		const TopoDS_Shape& shape, const std::string& name);
	std::shared_ptr<OCCGeometry> addBezierCurve(
		const std::vector<gp_Pnt>& controlPoints, const std::string& name);
	std::shared_ptr<OCCGeometry> addBezierSurface(
		const std::vector<std::vector<gp_Pnt>>& controlPoints, const std::string& name);
	std::shared_ptr<OCCGeometry> addBSplineCurve(
		const std::vector<gp_Pnt>& poles, const std::vector<double>& weights, const std::string& name);
	void upgradeGeometryToAdvanced(const std::string& name);
	void upgradeAllGeometriesToAdvanced();

	// ===== Hover Silhouette =====
	void updateHoverSilhouetteAt(const wxPoint& screenPos);

	// ===== Internal Access =====
	SoSeparator* getRootSeparator() const { return m_occRoot; }
	PickingService* getPickingService() const { return m_pickingService.get(); }

private:
	void initializeViewer();
	void createNormalVisualization(std::shared_ptr<OCCGeometry> geometry);
	void invalidateFeatureEdgeCache();

	SceneManager* m_sceneManager;
	SoSeparator* m_occRoot;
	SoSeparator* m_normalRoot;

	// Composition: Delegate to specialized controllers and managers
	std::unique_ptr<ViewportController> m_viewportController;
	std::unique_ptr<RenderingController> m_renderingController;
	std::unique_ptr<MeshParameterController> m_meshController;
	std::unique_ptr<LODController> m_lodController;
	std::unique_ptr<SliceController> m_sliceController;
	std::unique_ptr<ExplodeController> m_explodeController;
	std::unique_ptr<PickingService> m_pickingService;
	std::unique_ptr<SelectionManager> m_selectionManager;
	std::unique_ptr<ObjectTreeSync> m_objectTreeSync;
	std::unique_ptr<GeometryRepository> m_geometryRepo;
	std::unique_ptr<SceneAttachmentService> m_sceneAttach;
	std::unique_ptr<ViewUpdateService> m_viewUpdater;
	std::unique_ptr<MeshingService> m_meshingService;
	std::unique_ptr<EdgeDisplayManager> m_edgeDisplayManager;
	std::unique_ptr<OutlineDisplayManager> m_outlineManager;
	std::unique_ptr<SelectionOutlineManager> m_selectionOutline;
	std::unique_ptr<HoverSilhouetteManager> m_hoverManager;
	std::unique_ptr<BatchOperationManager> m_batchManager;

	// Explode state
	bool m_explodeEnabled;
	double m_explodeFactor;
	ExplodeMode m_explodeMode;
	ExplodeParams m_explodeParams;

	// Feature edge parameters
	FeatureEdgeParams m_lastFeatureParams;

	// Original edges parameters
	double m_originalEdgesSamplingDensity;
	double m_originalEdgesMinLength;
	bool m_originalEdgesShowLinesOnly;
	wxColour m_originalEdgesColor;
	double m_originalEdgesWidth;
	bool m_originalEdgesHighlightIntersectionNodes;
	wxColour m_originalEdgesIntersectionNodeColor;
	double m_originalEdgesIntersectionNodeSize;
};
