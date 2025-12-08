#include "viewer/modes/PointsMode.h"
#include "geometry/VertexExtractor.h"
#include "OCCMeshConverter.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoPointSet.h>

PointsMode::PointsMode() {
}

RenderingConfig::DisplayMode PointsMode::getModeType() const {
    return RenderingConfig::DisplayMode::Points;
}

int PointsMode::getSwitchChildIndex() const {
    return 0; // Points mode is child 0
}

bool PointsMode::requiresFaces() const {
    return false;
}

bool PointsMode::requiresEdges() const {
    return false;
}

SoSeparator* PointsMode::buildModeNode(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ModularEdgeComponent* modularEdgeComponent,
    VertexExtractor* vertexExtractor) {
    
    if (shape.IsNull()) {
        LOG_WRN_S("PointsMode::buildModeNode: shape is null");
        return nullptr;
    }
    
    // NOTE: vertexExtractor parameter is not used in this implementation
    // We directly use OCCMeshConverter to generate mesh
    // This check is kept for backward compatibility but vertexExtractor can be null

    SoSeparator* modeNode = new SoSeparator();
    modeNode->ref();
    modeNode->renderCaching.setValue(SoSeparator::OFF);
    modeNode->boundingBoxCaching.setValue(SoSeparator::OFF);
    modeNode->pickCulling.setValue(SoSeparator::OFF);

    // DrawStyle for points
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->style.setValue(SoDrawStyle::POINTS);
    // CRITICAL FIX: Ensure point size is at least 2.0 for visibility
    float pointSize = static_cast<float>(context.display.pointViewSize);
    if (pointSize < 2.0f) {
        pointSize = 2.0f;
        LOG_WRN_S("PointsMode::buildModeNode: pointSize too small (" + std::to_string(context.display.pointViewSize) + 
            "), using minimum 2.0 for visibility");
    }
    drawStyle->pointSize.setValue(pointSize);
    modeNode->addChild(drawStyle);
    LOG_INF_S("PointsMode::buildModeNode: Added DrawStyle with pointSize=" + std::to_string(pointSize));

    // Material for points
    SoMaterial* material = new SoMaterial();
    Standard_Real r, g, b;
    context.display.pointViewColor.Values(r, g, b, Quantity_TOC_RGB);
    material->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    material->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    modeNode->addChild(material);
    LOG_INF_S("PointsMode::buildModeNode: Added material with color (" + 
        std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ")");

    // Extract vertices and create point set
    try {
        OCCMeshConverter::MeshParameters occParams;
        occParams.deflection = params.deflection;
        occParams.angularDeflection = params.angularDeflection;
        occParams.relative = params.relative;
        occParams.inParallel = params.inParallel;

        TriangleMesh mesh = OCCMeshConverter::convertToMesh(shape, occParams);
        LOG_INF_S("PointsMode::buildModeNode: Generated mesh with " + std::to_string(mesh.vertices.size()) + " vertices");
        if (!mesh.vertices.empty()) {
            SoCoordinate3* coords = new SoCoordinate3();
            coords->point.setNum(static_cast<int>(mesh.vertices.size()));
            SbVec3f* points = new SbVec3f[mesh.vertices.size()];
            for (size_t i = 0; i < mesh.vertices.size(); ++i) {
                const gp_Pnt& vertex = mesh.vertices[i];
                points[i].setValue(
                    static_cast<float>(vertex.X()),
                    static_cast<float>(vertex.Y()),
                    static_cast<float>(vertex.Z())
                );
            }
            coords->point.setValues(0, static_cast<int>(mesh.vertices.size()), points);
            delete[] points;
            modeNode->addChild(coords);
            LOG_INF_S("PointsMode::buildModeNode: Added SoCoordinate3 with " + std::to_string(mesh.vertices.size()) + " points");

            SoPointSet* pointSet = new SoPointSet();
            pointSet->numPoints.setValue(static_cast<int>(mesh.vertices.size()));
            modeNode->addChild(pointSet);
            LOG_INF_S("PointsMode::buildModeNode: Added SoPointSet with " + std::to_string(mesh.vertices.size()) + " points");
        } else {
            LOG_WRN_S("PointsMode::buildModeNode: Mesh is empty, no points to display");
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in PointsMode::buildModeNode: " + std::string(e.what()));
    }

    return modeNode;
}


