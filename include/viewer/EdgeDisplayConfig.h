#pragma once

/**
 * @brief Unified edge display configuration
 *
 * This struct provides a high-level configuration for all edge display types,
 * replacing the need for individual setter calls.
 * 
 * Naming conventions aligned with unified terminology:
 * - showOriginalEdges: Original geometric edges (wireFrame equivalent)
 * - showFeatureEdges: Feature edges extracted based on angle/curvature
 * - showMeshEdges: Mesh edges from triangulation
 * - showVerticeNormals: Vertex normal lines (verticeNormals)
 * - showFaceNormals: Face normal lines (faceNormals)
 * - showHighlightEdges: Hover-highlighted edges (originalEdges or meshEdges)
 * - showIntersectionNodes: Edge intersection nodes
 * 
 * Note: Silhouette/outline/contour edges are handled separately via EdgeType::Silhouette
 */
struct EdgeDisplayConfig {
    bool showOriginalEdges = false;      // Original geometric edges (wireFrame)
    bool showFeatureEdges = false;       // Feature edges
    bool showMeshEdges = false;          // Mesh edges from triangulation
    bool showVerticeNormals = false;     // Vertex normal lines (verticeNormals)
    bool showFaceNormals = false;        // Face normal lines (faceNormals)
    bool showHighlightEdges = false;     // Hover-highlighted edges
    bool showIntersectionNodes = false;  // Edge intersection nodes

    // Default constructor
    EdgeDisplayConfig() = default;

    // Constructor with all parameters
    EdgeDisplayConfig(bool original, bool feature, bool mesh, bool verticeNormal, bool faceNormal, bool highlight, bool intersection)
        : showOriginalEdges(original)
        , showFeatureEdges(feature)
        , showMeshEdges(mesh)
        , showVerticeNormals(verticeNormal)
        , showFaceNormals(faceNormal)
        , showHighlightEdges(highlight)
        , showIntersectionNodes(intersection)
    {}

    // Comparison operators
    bool operator==(const EdgeDisplayConfig& other) const {
        return showOriginalEdges == other.showOriginalEdges &&
               showFeatureEdges == other.showFeatureEdges &&
               showMeshEdges == other.showMeshEdges &&
               showVerticeNormals == other.showVerticeNormals &&
               showFaceNormals == other.showFaceNormals &&
               showHighlightEdges == other.showHighlightEdges &&
               showIntersectionNodes == other.showIntersectionNodes;
    }

    bool operator!=(const EdgeDisplayConfig& other) const {
        return !(*this == other);
    }

    // Preset configurations
    static EdgeDisplayConfig createMinimal() {
        return EdgeDisplayConfig(false, false, false, false, false, false, false);
    }

    static EdgeDisplayConfig createStandard() {
        return EdgeDisplayConfig(true, true, false, false, false, false, false);
    }

    static EdgeDisplayConfig createFull() {
        return EdgeDisplayConfig(true, true, true, true, true, true, true);
    }

    static EdgeDisplayConfig createAnalysis() {
        return EdgeDisplayConfig(false, true, true, true, true, false, false);
    }
};
