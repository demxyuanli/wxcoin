#pragma once
#include <memory>
#include <vector>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include "EdgeTypes.h"
class TopoDS_Shape;
struct TriangleMesh;

class EdgeComponent {
public:
    EdgeDisplayFlags edgeFlags;

    EdgeComponent();
    ~EdgeComponent();

    void extractOriginalEdges(const TopoDS_Shape& shape);
    void extractFeatureEdges(const TopoDS_Shape& shape, double featureAngle, double minLength, bool onlyConvex, bool onlyConcave);
    void extractMeshEdges(const TriangleMesh& mesh);
    void generateAllEdgeNodes();

    SoSeparator* getEdgeNode(EdgeType type);

    void setEdgeDisplayType(EdgeType type, bool show);
    bool isEdgeDisplayTypeEnabled(EdgeType type) const;
    void updateEdgeDisplay(SoSeparator* parentNode);

    void generateHighlightEdgeNode();
    void generateNormalLineNode(const TriangleMesh& mesh, double length);
    void generateFaceNormalLineNode(const TriangleMesh& mesh, double length);

private:
    SoSeparator* originalEdgeNode = nullptr;
    SoSeparator* featureEdgeNode = nullptr;
    SoSeparator* meshEdgeNode = nullptr;
    SoSeparator* highlightEdgeNode = nullptr;
    SoSeparator* normalLineNode = nullptr;
    SoSeparator* faceNormalLineNode = nullptr;

}; 