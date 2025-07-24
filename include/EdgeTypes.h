#pragma once

// 边线类型枚举
enum class EdgeType {
    Original,
    Feature,
    Mesh,
    Highlight,
    NormalLine,
    FaceNormalLine
};

// 边线显示状态结构体
struct EdgeDisplayFlags {
    bool showOriginalEdges = false;
    bool showFeatureEdges = false;
    bool showMeshEdges = false;
    bool showHighlightEdges = false;
    bool showNormalLines = false;
    bool showFaceNormalLines = false;
}; 