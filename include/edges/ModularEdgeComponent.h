#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <memory>
#include <mutex>
#include <functional>
#include <atomic>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"
#include "rendering/GeometryProcessor.h"
#include "edges/extractors/BaseEdgeExtractor.h"
#include "edges/renderers/BaseEdgeRenderer.h"
#include "edges/extractors/OriginalEdgeExtractor.h"
#include "edges/extractors/FeatureEdgeExtractor.h"
#include "edges/extractors/MeshEdgeExtractor.h"
#include "edges/extractors/SilhouetteEdgeExtractor.h"

// Forward declarations
class EdgeLODManager;
class EdgeProcessorFactory;

namespace async {
    class AsyncEdgeIntersectionComputer;
    class AsyncEngineIntegration;
}

/**
 * @brief Enumeration for intersection node display shapes
 */
enum class IntersectionNodeShape {
    Sphere,     // Traditional sphere (higher quality, slower)
    Point,      // Simple point (fastest performance)
    Cross,      // Cross shape made of lines (balanced performance/quality)
    Cube        // Simple cube (good balance)
};

/**
 * @brief Modular edge component using the new modular edge system
 *
 * This component uses specialized extractors and renderers for different
 * edge types, providing better separation of concerns and extensibility.
 */
class ModularEdgeComponent {
public:
    EdgeDisplayFlags edgeFlags;

    ModularEdgeComponent();
    ~ModularEdgeComponent();

    // Original edges
    void extractOriginalEdges(
        const TopoDS_Shape& shape,
        double samplingDensity = 80.0,
        double minLength = 0.01,
        bool showLinesOnly = false,
        const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),
        double width = 1.0,
        bool highlightIntersectionNodes = false,
        const Quantity_Color& intersectionNodeColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
        double intersectionNodeSize = 3.0,
        IntersectionNodeShape intersectionNodeShape = IntersectionNodeShape::Point);

    // Feature edges
    void extractFeatureEdges(
        const TopoDS_Shape& shape,
        double featureAngle = 15.0,
        double minLength = 0.005,
        bool onlyConvex = false,
        bool onlyConcave = false,
        const Quantity_Color& color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 2.0);

    // Mesh edges
    void extractMeshEdges(
        const TriangleMesh& mesh,
        const Quantity_Color& color = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 1.0);

    // Silhouette edges (following FreeCAD's approach)
    void extractSilhouetteEdges(
        const TopoDS_Shape& shape,
        const gp_Pnt& cameraPos,
        const Quantity_Color& color = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 1.0);

    // Generate all edge nodes
    void generateAllEdgeNodes();

    // Node access
    SoSeparator* getEdgeNode(EdgeType type);

    // Extractor access (for advanced operations)
    std::shared_ptr<BaseEdgeExtractor> getOriginalExtractor() { return m_originalExtractor; }

    // Display control
    void setEdgeDisplayType(EdgeType type, bool show);
    bool isEdgeDisplayTypeEnabled(EdgeType type) const;
    void updateOriginalEdgesDisplay(SoSeparator* parentNode);
    void updateIntersectionNodesDisplay(SoSeparator* parentNode);
    void updateEdgeDisplay(SoSeparator* parentNode);

    // Appearance control
    void applyAppearanceToEdgeNode(
        EdgeType type,
        const Quantity_Color& color,
        double width,
        int style = 0);

    // Special edge types
    void generateHighlightEdgeNode();
    void generateNormalLineNode(const TriangleMesh& mesh, double length);
    void generateFaceNormalLineNode(const TriangleMesh& mesh, double length);

    // Intersection nodes appearance update
    void updateIntersectionNodesAppearance(
        SoSeparator* node,
        const Quantity_Color& color,
        double size);

    // Intersection nodes
    SoSeparator* createIntersectionNodesNode(
        const std::vector<gp_Pnt>& intersectionPoints,
        const Quantity_Color& color,
        double size,
        IntersectionNodeShape shape = IntersectionNodeShape::Point);

    // Incremental intersection node API for progressive display
    void addSingleIntersectionNode(const gp_Pnt& point, const Quantity_Color& color, double size);
    void addBatchIntersectionNodes(const std::vector<gp_Pnt>& points, const Quantity_Color& color, double size);
    void clearIntersectionNodes();
    bool hasIntersectionNodes() const;

    // Async intersection computation
    void computeIntersectionsAsync(
        const TopoDS_Shape& shape,
        double tolerance,
        class IAsyncEngine* engine,
        std::function<void(const std::vector<gp_Pnt>&, bool, const std::string&)> onComplete,
        std::function<void(int, const std::string&)> onProgress = nullptr);
    
    void cancelIntersectionComputation();
    bool isComputingIntersections() const { return m_computingIntersections.load(); }

    // Cleanup
    void clearMeshEdgeNode();
    void clearSilhouetteEdgeNode();
    void clearEdgeNode(EdgeType type);

    // LOD management
    void setLODEnabled(bool enabled);
    bool isLODEnabled() const;
    void updateLODLevel(const gp_Pnt& cameraPos);
    void generateLODLevels(const TopoDS_Shape& shape, const gp_Pnt& cameraPos);

    // NEW: Cache-based edge rendering API (public methods)
    void extractAndCacheOriginalEdges(const TopoDS_Shape& shape, double samplingDensity, double minLength);
    SoSeparator* createNodeFromCachedEdges(const Quantity_Color& color, double width);

private:
    // Processors for different edge types
    std::shared_ptr<BaseEdgeExtractor> m_originalExtractor;
    std::shared_ptr<BaseEdgeExtractor> m_featureExtractor;
    std::shared_ptr<BaseEdgeExtractor> m_meshExtractor;
    std::shared_ptr<BaseEdgeExtractor> m_silhouetteExtractor;

    std::shared_ptr<BaseEdgeRenderer> m_originalRenderer;
    std::shared_ptr<BaseEdgeRenderer> m_featureRenderer;
    std::shared_ptr<BaseEdgeRenderer> m_meshRenderer;

    // LOD manager
    std::unique_ptr<EdgeLODManager> m_lodManager;

    // Edge nodes
    SoSeparator* originalEdgeNode = nullptr;
    SoSeparator* featureEdgeNode = nullptr;
    SoSeparator* meshEdgeNode = nullptr;
    SoSeparator* highlightEdgeNode = nullptr;
    SoSeparator* normalLineNode = nullptr;
    SoSeparator* faceNormalLineNode = nullptr;
    SoSeparator* silhouetteEdgeNode = nullptr;
    SoSeparator* intersectionNodesNode = nullptr;

    // Cached edge data (extracted at import time, rendered on demand)
    struct CachedEdgeData {
        std::vector<gp_Pnt> vertices;          // All edge vertices
        std::vector<std::pair<int, int>> segments; // Edge segments as vertex indices
        bool isValid = false;
        
        void clear() {
            vertices.clear();
            segments.clear();
            isValid = false;
        }
    };
    CachedEdgeData m_cachedOriginalEdges;
    mutable std::mutex m_cachedEdgesMutex;

    mutable std::mutex m_nodeMutex;

    // Async computation state
    std::unique_ptr<async::AsyncEdgeIntersectionComputer> m_asyncIntersectionComputer;
    std::atomic<bool> m_computingIntersections{false};

    // Helper methods
    void cleanupEdgeNode(SoSeparator*& node);
    void attachEdgeNodeToParent(SoSeparator* parentNode, SoSeparator* edgeNode);
};
