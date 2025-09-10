#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "EdgeTypes.h"
#include "OCCGeometry.h"

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
	void setShowFeatureEdges(bool show, const MeshParameters& meshParams);
	void setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave, const MeshParameters& meshParams);
	void setShowMeshEdges(bool show, const MeshParameters& meshParams);
	void setShowHighlightEdges(bool show, const MeshParameters& meshParams);
	void setShowNormalLines(bool show, const MeshParameters& meshParams);
	void setShowFaceNormalLines(bool show, const MeshParameters& meshParams);

	// Update
	void updateAll(const MeshParameters& meshParams);

	// Feature edge generation
	void startAsyncFeatureEdgeGeneration(double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave, const MeshParameters& meshParams);
	bool isFeatureEdgeGenerationRunning() const { return m_featureEdgeRunning.load(); }
	int getFeatureEdgeProgress() const { return m_featureEdgeProgress.load(); }
	bool hasFeatureEdgeCache() const { return m_featureCacheValid; }

	struct FeatureEdgeParams { double angleDeg{ 15.0 }; double minLength{ 0.005 }; bool onlyConvex{ false }; bool onlyConcave{ false }; };
	FeatureEdgeParams getLastFeatureEdgeParams() const { return m_lastFeatureParams; }
	void invalidateFeatureEdgeCache();

	// Appearance
	void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly);
	
	// Original edges parameters
	void setOriginalEdgesParameters(double samplingDensity, double minLength, bool showLinesOnly, const Quantity_Color& color, double width);

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
	
	// Original edges parameters
	struct OriginalEdgeParams { 
		double samplingDensity{ 80.0 }; 
		double minLength{ 0.01 }; 
		bool showLinesOnly{ false }; 
		Quantity_Color color{ 1.0, 0.0, 0.0, Quantity_TOC_RGB }; // Default red
		double width{ 1.0 }; 
	};
	OriginalEdgeParams m_originalEdgeParams{};
};
