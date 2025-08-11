#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <opencascade/TopoDS_Shape.hxx>
#include <opencascade/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"
#include "rendering/GeometryProcessor.h"

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

    // Apply appearance (color and width) to a specific edge node type if it exists
    void applyAppearanceToEdgeNode(EdgeType type, const Quantity_Color& color, double width);

    void generateHighlightEdgeNode();
    void generateNormalLineNode(const TriangleMesh& mesh, double length);
    void generateFaceNormalLineNode(const TriangleMesh& mesh, double length);
    void generateSilhouetteEdgeNode(const TopoDS_Shape& shape, const gp_Pnt& cameraPos);
    void clearSilhouetteEdgeNode();  // New: clear silhouette edge node

private:
    SoSeparator* originalEdgeNode = nullptr;
    SoSeparator* featureEdgeNode = nullptr;
    SoSeparator* meshEdgeNode = nullptr;
    SoSeparator* highlightEdgeNode = nullptr;
    SoSeparator* normalLineNode = nullptr;
    SoSeparator* faceNormalLineNode = nullptr;
    SoSeparator* silhouetteEdgeNode = nullptr;  // New: silhouette edge node

public:
    // Friend class to allow OCCViewer access to silhouetteEdgeNode
    friend class OCCViewer;
}; 