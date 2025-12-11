#pragma once

/**
 * @brief Edge type enumeration
 * 
 * Naming conventions:
 * - Original: Geometric original edges (equivalent to wireFrame)
 * - Feature: Feature edges extracted based on angle/curvature
 * - Mesh: Mesh edges from triangulation
 * - Highlight: Hover-highlighted originalEdges or meshEdges
 * - VerticeNormal: Vertex normal lines (verticeNormals)
 * - FaceNormal: Triangle face normal lines (faceNormals)
 * - IntersectionNodes: Edge intersection nodes
 * - Silhouette: Outline/contour edges (silhouette = outline = contour)
 */
enum class EdgeType {
    Original,           // Original geometric edges (wireFrame equivalent)
    Feature,            // Feature edges
    Mesh,               // Mesh edges from triangulation
    Highlight,          // Hover-highlighted edges (originalEdges or meshEdges)
    VerticeNormal,      // Vertex normal lines (verticeNormals)
    FaceNormal,         // Face normal lines (faceNormals)
    IntersectionNodes,  // Edge intersection nodes
    Silhouette          // Outline/contour edges (silhouette = outline = contour)
};

/**
 * @brief Edge display state structure
 * 
 * Naming conventions aligned with unified terminology:
 * - showOriginalEdges: Show original geometric edges (wireFrame equivalent)
 * - showFeatureEdges: Show feature edges
 * - showMeshEdges: Show mesh edges from triangulation
 * - showHighlightEdges: Show hover-highlighted edges
 * - showVerticeNormals: Show vertex normal lines (verticeNormals)
 * - showFaceNormals: Show face normal lines (faceNormals)
 * - showSilhouetteEdges: Show outline/contour edges (silhouette = outline = contour)
 * - showIntersectionNodes: Show edge intersection nodes
 */
struct EdgeDisplayFlags {
    bool showOriginalEdges = false;      // Original geometric edges (wireFrame)
    bool showFeatureEdges = false;       // Feature edges
    bool showMeshEdges = false;          // Mesh edges from triangulation
    bool showHighlightEdges = false;     // Hover-highlighted edges
    bool showVerticeNormals = false;     // Vertex normal lines (verticeNormals)
    bool showFaceNormals = false;        // Face normal lines (faceNormals)
    bool showSilhouetteEdges = false;    // Outline/contour edges (silhouette = outline = contour)
    bool showIntersectionNodes = false;  // Edge intersection nodes
}; 