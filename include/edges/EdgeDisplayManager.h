#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "EdgeTypes.h"
#include "OCCGeometry.h"
#include "edges/ModularEdgeComponent.h"
#include "rendering/GeometryProcessor.h"

class SceneManager;
struct MeshParameters;

class EdgeDisplayManager {
public:
	EdgeDisplayManager(SceneManager* sceneManager,
		std::vector<std::shared_ptr<OCCGeometry>>* geometries);

	// Flags
	const EdgeDisplayFlags& getFlags() const { return m_flags; }
	void setFlags(const EdgeDisplayFlags& flags) { m_flags = flags; }

	// Toggling APIs
	void toggleEdgeType(EdgeType type, bool show, const MeshParameters& meshParams);
	void setShowOriginalEdges(bool show, const MeshParameters& meshParams);
	
	// CRITICAL FEATURE: Show original edges only for selected objects (performance optimization)
	void setShowOriginalEdgesForSelectedOnly(bool selectedOnly, const MeshParameters& meshParams);
	bool isShowOriginalEdgesForSelectedOnly() const { return m_showOriginalEdgesForSelectedOnly; }
	
	// Outline/contour edges (fast mode) - silhouette = outline = contour (unified naming convention)
	void setShowSilhouetteEdgesOnly(bool silhouetteOnly, const MeshParameters& meshParams);
	bool isShowSilhouetteEdgesOnly() const { return m_showSilhouetteEdgesOnly; }
	
	// CRITICAL FIX: Async original edge extraction (following FreeCAD's CoinThread approach)
	// Start asynchronous extraction in background thread, create nodes on main thread
	void startAsyncOriginalEdgeExtraction(double samplingDensity, double minLength, bool showLinesOnly,
		const Quantity_Color& color, double width, const Quantity_Color& intersectionNodeColor,
		double intersectionNodeSize, IntersectionNodeShape intersectionNodeShape,
		const MeshParameters& meshParams,
		std::function<void(bool, const std::string&)> onComplete = nullptr);
	
	// Check async extraction status
	bool isOriginalEdgeExtractionRunning() const { return m_originalEdgeRunning.load(); }
	int getOriginalEdgeExtractionProgress() const { return m_originalEdgeProgress.load(); }
	bool hasOriginalEdgeCache() const { return m_originalEdgeCacheValid; }
	void invalidateOriginalEdgeCache() { m_originalEdgeCacheValid = false; }
	
	void extractOriginalEdgesOnly(double samplingDensity, double minLength, bool showLinesOnly,
		const Quantity_Color& color, double width, const Quantity_Color& intersectionNodeColor,
		double intersectionNodeSize, IntersectionNodeShape intersectionNodeShape,
		std::function<void(bool, const std::string&)> onComplete = nullptr);
	void setShowFeatureEdges(bool show, const MeshParameters& meshParams);
	void setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave, const MeshParameters& meshParams);
	void setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave, const MeshParameters& meshParams,
		const Quantity_Color& color, double width);
	void setShowMeshEdges(bool show, const MeshParameters& meshParams);
	void setShowHighlightEdges(bool show, const MeshParameters& meshParams);
	void setShowVerticeNormals(bool show, const MeshParameters& meshParams);
	void setShowFaceNormals(bool show, const MeshParameters& meshParams);
	void setShowIntersectionNodes(bool show, const MeshParameters& meshParams);

	// Update
	void updateAll(const MeshParameters& meshParams, bool forceMeshRegeneration = false);

	// Edge component switching (for migration)
	void setUseModularEdgeComponent(bool useModular);
	bool isUsingModularEdgeComponent() const;

	// Feature edge generation
	void startAsyncFeatureEdgeGeneration(double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave, const MeshParameters& meshParams);
	bool isFeatureEdgeGenerationRunning() const { return m_featureEdgeRunning.load(); }
	int getFeatureEdgeProgress() const { return m_featureEdgeProgress.load(); }
	bool hasFeatureEdgeCache() const { return m_featureCacheValid; }

	// Async intersection computation
	void computeIntersectionsAsync(
		double tolerance,
		class IAsyncEngine* engine,
		std::function<void(size_t totalPoints, bool success)> onComplete,
		std::function<void(int progress, const std::string& message)> onProgress = nullptr);
	
	bool isIntersectionComputationRunning() const { return m_intersectionRunning.load(); }
	void cancelIntersectionComputation();
	int getIntersectionProgress() const { return m_intersectionProgress.load(); }

	struct FeatureEdgeParams { double angleDeg{ 15.0 }; double minLength{ 0.005 }; bool onlyConvex{ false }; bool onlyConcave{ false }; };
	FeatureEdgeParams getLastFeatureEdgeParams() const { return m_lastFeatureParams; }
	void invalidateFeatureEdgeCache();

	struct FeatureEdgeAppearance {
		Quantity_Color color{ 1.0, 0.0, 0.0, Quantity_TOC_RGB }; // Default red
		double width{ 2.0 };
		int style{ 0 }; // 0=Solid, 1=Dashed, 2=Dotted, 3=Dash-Dot
		bool edgesOnly{ false };
	};

	struct WireframeAppearance {
		Quantity_Color color{ 0.0, 0.0, 0.0, Quantity_TOC_RGB }; // Default black
		double width{ 1.0 };
		int style{ 0 }; // 0=Solid, 1=Dashed, 2=Dotted, 3=Dash-Dot
		bool showOnlyNew{ false };
	};

	struct MeshEdgeAppearance {
		Quantity_Color color{ 0.0, 0.0, 1.0, Quantity_TOC_RGB }; // Default blue
		double width{ 1.0 };
		int style{ 0 }; // 0=Solid, 1=Dashed, 2=Dotted, 3=Dash-Dot
		bool showOnlyNew{ false };
	};

	// Appearance
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly);
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly, const MeshParameters& meshParams);
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, int style, bool edgesOnly);

	// Wireframe appearance
	void applyWireframeAppearance(const Quantity_Color& color, double width, int style, bool showOnlyNew);
	void setWireframeAppearance(const WireframeAppearance& appearance);

	// Mesh edges appearance
	void applyMeshEdgeAppearance(const Quantity_Color& color, double width, int style, bool showOnlyNew);
	void setMeshEdgeAppearance(const MeshEdgeAppearance& appearance);
	
	// Original edges parameters
	void setOriginalEdgesParameters(double samplingDensity, double minLength, bool showLinesOnly, const Quantity_Color& color, double width,
		bool highlightIntersectionNodes = false, const Quantity_Color& intersectionNodeColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), double intersectionNodeSize = 3.0, IntersectionNodeShape intersectionNodeShape = IntersectionNodeShape::Point);

private:
	SceneManager* m_sceneManager{ nullptr };
	std::vector<std::shared_ptr<OCCGeometry>>* m_geometries{ nullptr };
	EdgeDisplayFlags m_flags{};

	// Async feature generation state
	std::atomic<bool> m_featureEdgeRunning{ false };
	std::atomic<int> m_featureEdgeProgress{ 0 };
	std::thread m_featureEdgeThread;
	FeatureEdgeParams m_lastFeatureParams{};
	bool m_featureCacheValid{ false };
	FeatureEdgeAppearance m_featureEdgeAppearance{};

	// CRITICAL FIX: Async original edge extraction (following FreeCAD's CoinThread approach)
	// This prevents UI blocking and GL context crashes for large models
	std::atomic<bool> m_originalEdgeRunning{ false };
	std::atomic<int> m_originalEdgeProgress{ 0 };
	std::thread m_originalEdgeThread;
	bool m_originalEdgeCacheValid{ false };

	// Async intersection computation state
	std::atomic<bool> m_intersectionRunning{ false };
	std::atomic<int> m_intersectionProgress{ 0 };

	// Wireframe and mesh edge appearance
	WireframeAppearance m_wireframeAppearance{};
	MeshEdgeAppearance m_meshEdgeAppearance{};

	// Original edges parameters
	struct OriginalEdgeParams {
		double samplingDensity{ 80.0 };
		double minLength{ 0.01 };
		bool showLinesOnly{ false };
		Quantity_Color color{ 1.0, 0.0, 0.0, Quantity_TOC_RGB }; // Default red
		double width{ 1.0 };
		bool highlightIntersectionNodes{ false };
		Quantity_Color intersectionNodeColor{ 1.0, 0.0, 0.0, Quantity_TOC_RGB }; // Default red
		double intersectionNodeSize{ 3.0 };
		IntersectionNodeShape intersectionNodeShape{ IntersectionNodeShape::Point };
	};
	OriginalEdgeParams m_originalEdgeParams{};
	MeshParameters m_lastOriginalMeshParams{};
	
	// CRITICAL FEATURES: Advanced display modes
	bool m_showOriginalEdgesForSelectedOnly{ false };  // Show edges only for selected objects
	bool m_showSilhouetteEdgesOnly{ false };            // Show only outline/contour edges (fast mode, silhouette = outline = contour)
};
