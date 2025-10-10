#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "edges/EdgeDisplayManager.h"
#include "rendering/RenderingToolkitAPI.h"
#include "EdgeComponent.h"
#include "edges/EdgeGenerationService.h"
#include "edges/EdgeRenderApplier.h"
#include "logger/Logger.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "ViewRefreshManager.h"

EdgeDisplayManager::EdgeDisplayManager(SceneManager* sceneManager,
	std::vector<std::shared_ptr<OCCGeometry>>* geometries)
	: m_sceneManager(sceneManager), m_geometries(geometries) {
}

void EdgeDisplayManager::toggleEdgeType(EdgeType type, bool show, const MeshParameters& meshParams) {
	switch (type) {
	case EdgeType::Original: m_flags.showOriginalEdges = show; break;
	case EdgeType::Feature: m_flags.showFeatureEdges = show; break;
	case EdgeType::Mesh: m_flags.showMeshEdges = show; break;
	case EdgeType::Highlight: m_flags.showHighlightEdges = show; break;
	case EdgeType::NormalLine: m_flags.showNormalLines = show; break;
	case EdgeType::FaceNormalLine: m_flags.showFaceNormalLines = show; break;
	}
	updateAll(meshParams);
}

void EdgeDisplayManager::setShowOriginalEdges(bool show, const MeshParameters& meshParams) { m_flags.showOriginalEdges = show; updateAll(meshParams); }
void EdgeDisplayManager::setShowFeatureEdges(bool show, const MeshParameters& meshParams) {
	m_flags.showFeatureEdges = show;

	// When disabling feature edges, restore geometry faces visibility
	if (!show && m_geometries) {
		for (auto& g : *m_geometries) {
			if (g) {
				// Restore faces visibility when disabling feature edges
				g->setFacesVisible(true);
			}
		}
	}

	if (show && !m_featureCacheValid && !m_featureEdgeRunning.load()) {
		// Kick off async generation using last known params (or defaults)
		startAsyncFeatureEdgeGeneration(m_lastFeatureParams.angleDeg, m_lastFeatureParams.minLength, m_lastFeatureParams.onlyConvex, m_lastFeatureParams.onlyConcave, meshParams);
	}
	updateAll(meshParams);
}
void EdgeDisplayManager::setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave, const MeshParameters& meshParams) {
	m_flags.showFeatureEdges = show;

	// When disabling feature edges, restore geometry faces visibility
	if (!show && m_geometries) {
		for (auto& g : *m_geometries) {
			if (g) {
				// Restore faces visibility when disabling feature edges
				g->setFacesVisible(true);
			}
		}
	}

	if (show) {
		bool paramsChanged = m_lastFeatureParams.angleDeg != featureAngleDeg || m_lastFeatureParams.minLength != minLength || m_lastFeatureParams.onlyConvex != onlyConvex || m_lastFeatureParams.onlyConcave != onlyConcave;
		if (paramsChanged) {
			m_lastFeatureParams = { featureAngleDeg, minLength, onlyConvex, onlyConcave };
			invalidateFeatureEdgeCache();
		}
		if (!m_featureCacheValid && !m_featureEdgeRunning.load()) {
			startAsyncFeatureEdgeGeneration(m_lastFeatureParams.angleDeg, m_lastFeatureParams.minLength, m_lastFeatureParams.onlyConvex, m_lastFeatureParams.onlyConcave, meshParams);
		}
	}
	updateAll(meshParams);
}
void EdgeDisplayManager::setShowMeshEdges(bool show, const MeshParameters& meshParams) { m_flags.showMeshEdges = show; updateAll(meshParams); }
void EdgeDisplayManager::setShowHighlightEdges(bool show, const MeshParameters& meshParams) { m_flags.showHighlightEdges = show; updateAll(meshParams); }
void EdgeDisplayManager::setShowNormalLines(bool show, const MeshParameters& meshParams) { m_flags.showNormalLines = show; updateAll(meshParams); }
void EdgeDisplayManager::setShowFaceNormalLines(bool show, const MeshParameters& meshParams) { m_flags.showFaceNormalLines = show; updateAll(meshParams); }

void EdgeDisplayManager::setOriginalEdgesParameters(double samplingDensity, double minLength, bool showLinesOnly, const Quantity_Color& color, double width,
	bool highlightIntersectionNodes, const Quantity_Color& intersectionNodeColor, double intersectionNodeSize) {
	m_originalEdgeParams.samplingDensity = samplingDensity;
	m_originalEdgeParams.minLength = minLength;
	m_originalEdgeParams.showLinesOnly = showLinesOnly;
	m_originalEdgeParams.color = color;
	m_originalEdgeParams.width = width;
	m_originalEdgeParams.highlightIntersectionNodes = highlightIntersectionNodes;
	m_originalEdgeParams.intersectionNodeColor = intersectionNodeColor;
	m_originalEdgeParams.intersectionNodeSize = intersectionNodeSize;
}

void EdgeDisplayManager::updateAll(const MeshParameters& meshParams) {
	if (!m_geometries) return;
	EdgeGenerationService generator;
	EdgeRenderApplier applier;
	
	// Use the provided meshParams, but ensure it's the latest parameters
	// This helps maintain consistency between geometry faces and mesh edges
	MeshParameters currentParams = meshParams;
	
	for (auto& g : *m_geometries) {
		if (!g) continue;
		if (!g->edgeComponent) g->edgeComponent = std::make_unique<EdgeComponent>();
		g->edgeComponent->edgeFlags = m_flags;

		if (m_flags.showOriginalEdges) {
			generator.ensureOriginalEdges(g, m_originalEdgeParams.samplingDensity, m_originalEdgeParams.minLength, 
				m_originalEdgeParams.showLinesOnly, m_originalEdgeParams.color, m_originalEdgeParams.width,
				m_originalEdgeParams.highlightIntersectionNodes, m_originalEdgeParams.intersectionNodeColor, m_originalEdgeParams.intersectionNodeSize);
		}
		bool needMesh = false;
		if (m_flags.showMeshEdges && g->edgeComponent->getEdgeNode(EdgeType::Mesh) == nullptr) needMesh = true;
		// For vertex normals, we also need mesh data. Ensure we treat "show normals" as enabling normal lines.
		if (m_flags.showNormalLines && g->edgeComponent->getEdgeNode(EdgeType::NormalLine) == nullptr) needMesh = true;
		if (m_flags.showFaceNormalLines && g->edgeComponent->getEdgeNode(EdgeType::FaceNormalLine) == nullptr) needMesh = true;
		if (needMesh) {
			generator.ensureMeshDerivedEdges(g, currentParams, m_flags.showMeshEdges, m_flags.showNormalLines, m_flags.showFaceNormalLines);
		}
		if (m_flags.showFeatureEdges && m_featureCacheValid) {
			generator.ensureFeatureEdges(g, m_lastFeatureParams.angleDeg, m_lastFeatureParams.minLength, m_lastFeatureParams.onlyConvex, m_lastFeatureParams.onlyConcave);
		}
		applier.applyFlagsAndAttach(g, m_flags);
	}
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		m_sceneManager->getCanvas()->Refresh();
		if (auto* rm = m_sceneManager->getCanvas()->getRefreshManager()) rm->requestRefresh(ViewRefreshManager::RefreshReason::EDGES_TOGGLED, true);
	}
}

void EdgeDisplayManager::startAsyncFeatureEdgeGeneration(double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave, const MeshParameters& meshParams) {
	if (m_featureEdgeRunning.load() || !m_geometries) return;
	m_featureEdgeRunning = true;
	m_featureEdgeProgress = 0;
	if (m_featureEdgeThread.joinable()) m_featureEdgeThread.detach();
	m_lastFeatureParams = { featureAngleDeg, minLength, onlyConvex, onlyConcave };
	m_featureEdgeThread = std::thread([this, meshParams]() {
		const int total = static_cast<int>(m_geometries->size());
		int done = 0;
		for (auto& g : *m_geometries) {
			if (!g) continue;
			if (!g->edgeComponent) g->edgeComponent = std::make_unique<EdgeComponent>();
			if (m_flags.showFeatureEdges && g->edgeComponent->getEdgeNode(EdgeType::Feature) == nullptr) {
				// Worker thread: compute feature edge geometry only
				g->edgeComponent->extractFeatureEdges(g->getShape(), m_lastFeatureParams.angleDeg, m_lastFeatureParams.minLength, m_lastFeatureParams.onlyConvex, m_lastFeatureParams.onlyConcave);
			}
			done++;
			m_featureEdgeProgress = static_cast<int>(static_cast<double>(done) / std::max(1, total) * 100.0);
		}
		m_featureEdgeRunning = false;
		m_featureCacheValid = true;
		// Back to UI thread to apply appearance/attach and refresh
		if (m_sceneManager && m_sceneManager->getCanvas()) {
			m_sceneManager->getCanvas()->CallAfter([this, meshParams]() {
				updateAll(meshParams);
				// Apply stored appearance parameters after feature edges are generated
				if (m_flags.showFeatureEdges) {
					applyFeatureEdgeAppearance(m_featureEdgeAppearance.color, m_featureEdgeAppearance.width, m_featureEdgeAppearance.edgesOnly, meshParams);
				}
			});
		} else {
			updateAll(meshParams);
			// Apply stored appearance parameters after feature edges are generated
			if (m_flags.showFeatureEdges) {
				applyFeatureEdgeAppearance(m_featureEdgeAppearance.color, m_featureEdgeAppearance.width, m_featureEdgeAppearance.edgesOnly, meshParams);
			}
		}
		});
}

void EdgeDisplayManager::invalidateFeatureEdgeCache() {
	m_featureCacheValid = false;
}

void EdgeDisplayManager::applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly, const MeshParameters& meshParams) {
	applyFeatureEdgeAppearance(color, width, 0, edgesOnly); // Default to solid style
}

void EdgeDisplayManager::applyFeatureEdgeAppearance(const Quantity_Color& color, double width, int style, bool edgesOnly) {
	if (!m_geometries) return;

	// Store appearance parameters for later use
	m_featureEdgeAppearance.color = color;
	m_featureEdgeAppearance.width = width;
	m_featureEdgeAppearance.style = style;
	m_featureEdgeAppearance.edgesOnly = edgesOnly;

	for (auto& g : *m_geometries) {
		if (!g) continue;
		g->setEdgeColor(color);
		g->setEdgeWidth(width);
		g->setFacesVisible(!edgesOnly);
		if (g->edgeComponent) {
			g->edgeComponent->edgeFlags = m_flags;
			// If feature edge node exists, apply appearance immediately
			if (g->edgeComponent->getEdgeNode(EdgeType::Feature)) {
				g->edgeComponent->applyAppearanceToEdgeNode(EdgeType::Feature, color, width, style);
			} else if (m_flags.showFeatureEdges && !m_featureEdgeRunning.load()) {
				// If feature edges are enabled but node doesn't exist, ensure generation with current params
				// This handles the case where appearance is applied before feature edges are generated
				if (!m_featureCacheValid) {
					startAsyncFeatureEdgeGeneration(m_lastFeatureParams.angleDeg, m_lastFeatureParams.minLength,
						m_lastFeatureParams.onlyConvex, m_lastFeatureParams.onlyConcave, MeshParameters{});
				}
			}
			g->updateEdgeDisplay();
		}
	}
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		if (auto* rm = m_sceneManager->getCanvas()->getRefreshManager()) rm->requestRefresh(ViewRefreshManager::RefreshReason::RENDERING_CHANGED, true);
	}
}