#pragma once

// Edge type enumeration
enum class EdgeType {
    Original,
    Feature,
    Mesh,
    Highlight,
    NormalLine,
    FaceNormalLine,
    Silhouette  // New: silhouette edge type
};

// Edge display state structure
struct EdgeDisplayFlags {
    bool showOriginalEdges = false;
    bool showFeatureEdges = false;
    bool showMeshEdges = false;
    bool showHighlightEdges = false;
    bool showNormalLines = false;
    bool showFaceNormalLines = false;
    bool showSilhouetteEdges = false;  // New: silhouette edge display flag
}; 