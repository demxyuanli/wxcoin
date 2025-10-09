#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <opencascade/TopoDS_Shape.hxx>
#include <opencascade/TopoDS_Edge.hxx>
#include <opencascade/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"
#include "rendering/GeometryProcessor.h"
#include <mutex>

class EdgeComponent {
public:
    EdgeDisplayFlags edgeFlags;

    EdgeComponent();
    ~EdgeComponent();

    void extractOriginalEdges(const TopoDS_Shape& shape, double samplingDensity = 80.0, double minLength = 0.01, bool showLinesOnly = false, const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB), double width = 1.0,
		bool highlightIntersectionNodes = false, const Quantity_Color& intersectionNodeColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), double intersectionNodeSize = 3.0);
    void extractFeatureEdges(const TopoDS_Shape& shape, double featureAngle, double minLength, bool onlyConvex, bool onlyConcave);
    void extractMeshEdges(const TriangleMesh& mesh);
    void generateAllEdgeNodes();

    SoSeparator* getEdgeNode(EdgeType type);

    void setEdgeDisplayType(EdgeType type, bool show);
    bool isEdgeDisplayTypeEnabled(EdgeType type) const;
    void updateEdgeDisplay(SoSeparator* parentNode);

    // Apply appearance (color, width and style) to a specific edge node type if it exists
    void applyAppearanceToEdgeNode(EdgeType type, const Quantity_Color& color, double width, int style = 0);

    void generateHighlightEdgeNode();
    void generateNormalLineNode(const TriangleMesh& mesh, double length);
    void generateFaceNormalLineNode(const TriangleMesh& mesh, double length);
    void generateSilhouetteEdgeNode(const TopoDS_Shape& shape, const gp_Pnt& cameraPos);
    void clearSilhouetteEdgeNode();  // New: clear silhouette edge node
    void generateIntersectionNodesNode(const std::vector<gp_Pnt>& intersectionPoints, const Quantity_Color& color, double size);

private:
    void findEdgeIntersections(const TopoDS_Shape& shape, std::vector<gp_Pnt>& intersectionPoints);
    void findEdgeIntersectionsFromEdges(const std::vector<class TopoDS_Edge>& edges, std::vector<gp_Pnt>& intersectionPoints);
    void findEdgeIntersectionsSimple(const std::vector<class TopoDS_Edge>& edges, std::vector<gp_Pnt>& intersectionPoints);
    double computeMinDistanceBetweenCurves(const struct EdgeData& data1, const struct EdgeData& data2);
    gp_Pnt computeIntersectionPoint(const struct EdgeData& data1, const struct EdgeData& data2);
    SoSeparator* originalEdgeNode = nullptr;
    SoSeparator* featureEdgeNode = nullptr;
    SoSeparator* meshEdgeNode = nullptr;
    SoSeparator* highlightEdgeNode = nullptr;
    SoSeparator* normalLineNode = nullptr;
    SoSeparator* faceNormalLineNode = nullptr;
    SoSeparator* silhouetteEdgeNode = nullptr;  // New: silhouette edge node
    SoSeparator* intersectionNodesNode = nullptr;  // New: intersection nodes node
    mutable std::mutex m_nodeMutex;
    
    std::unique_ptr<class EdgeExtractor> m_extractor;
    std::unique_ptr<class EdgeRenderer> m_renderer;

public:
    // Friend class to allow OCCViewer access to silhouetteEdgeNode
    friend class OCCViewer;
}; 