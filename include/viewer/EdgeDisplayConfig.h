#pragma once

/**
 * @brief Unified edge display configuration
 *
 * This struct provides a high-level configuration for all edge display types,
 * replacing the need for individual setter calls.
 */
struct EdgeDisplayConfig {
    bool showOriginalEdges = false;
    bool showFeatureEdges = false;
    bool showMeshEdges = false;
    bool showNormalLines = false;
    bool showFaceNormalLines = false;
    bool showHighlightEdges = false;
    bool showIntersectionNodes = false;

    // Default constructor
    EdgeDisplayConfig() = default;

    // Constructor with all parameters
    EdgeDisplayConfig(bool original, bool feature, bool mesh, bool normal, bool faceNormal, bool highlight, bool intersection)
        : showOriginalEdges(original)
        , showFeatureEdges(feature)
        , showMeshEdges(mesh)
        , showNormalLines(normal)
        , showFaceNormalLines(faceNormal)
        , showHighlightEdges(highlight)
        , showIntersectionNodes(intersection)
    {}

    // Comparison operators
    bool operator==(const EdgeDisplayConfig& other) const {
        return showOriginalEdges == other.showOriginalEdges &&
               showFeatureEdges == other.showFeatureEdges &&
               showMeshEdges == other.showMeshEdges &&
               showNormalLines == other.showNormalLines &&
               showFaceNormalLines == other.showFaceNormalLines &&
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
