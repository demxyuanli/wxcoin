#include "geometry/PointViewRenderer.h"
#include "geometry/VertexExtractor.h"
#include "config/RenderingConfig.h"
#include "geometry/GeometryRenderContext.h"
#include "OCCMeshConverter.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoScale.h>
#include <OpenCASCADE/gp_Pnt.hxx>

PointViewRenderer::PointViewRenderer()
{
}

PointViewRenderer::~PointViewRenderer()
{
}

SoSeparator* PointViewRenderer::createPointViewNode(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const DisplaySettings& displaySettings,
    VertexExtractor* vertexExtractor)
{
    try {
        std::vector<gp_Pnt> vertices;

        // Try to use vertex extractor first
        if (vertexExtractor) {
            vertices = vertexExtractor->getCachedVertices();
        }

        // If no cached vertices, generate mesh
        if (vertices.empty()) {
            OCCMeshConverter::MeshParameters occParams;
            occParams.deflection = params.deflection;
            occParams.angularDeflection = params.angularDeflection;
            occParams.relative = params.relative;
            occParams.inParallel = params.inParallel;

            TriangleMesh mesh = OCCMeshConverter::convertToMesh(shape, occParams);
            if (mesh.vertices.empty()) {
                return nullptr;
            }
            vertices = mesh.vertices;
        }

        // Create point view separator
        SoSeparator* pointViewSep = new SoSeparator();
        pointViewSep->renderCaching.setValue(SoSeparator::OFF);
        pointViewSep->boundingBoxCaching.setValue(SoSeparator::OFF);
        pointViewSep->pickCulling.setValue(SoSeparator::OFF);

        // Create material for points
        SoMaterial* pointMaterial = new SoMaterial();
        Standard_Real r, g, b;
        displaySettings.pointViewColor.Values(r, g, b, Quantity_TOC_RGB);
        pointMaterial->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        pointMaterial->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        pointViewSep->addChild(pointMaterial);

        // Create draw style for point size
        SoDrawStyle* pointStyle = new SoDrawStyle();
        pointStyle->pointSize.setValue(static_cast<float>(displaySettings.pointViewSize));
        pointViewSep->addChild(pointStyle);

        // Create coordinates
        SoCoordinate3* coords = new SoCoordinate3();
        coords->point.setNum(static_cast<int>(vertices.size()));

        SbVec3f* points = new SbVec3f[vertices.size()];
        for (size_t i = 0; i < vertices.size(); ++i) {
            const gp_Pnt& vertex = vertices[i];
            points[i].setValue(
                static_cast<float>(vertex.X()),
                static_cast<float>(vertex.Y()),
                static_cast<float>(vertex.Z())
            );
        }
        coords->point.setValues(0, static_cast<int>(vertices.size()), points);
        delete[] points;
        pointViewSep->addChild(coords);

        // Add shape-specific rendering based on pointShape
        if (displaySettings.pointViewShape == 1) {
            // Circle points
            SoSeparator* circleSep = createCirclePoints(vertices, displaySettings);
            pointViewSep->addChild(circleSep);
        } else if (displaySettings.pointViewShape == 2) {
            // Triangle points
            SoSeparator* triangleSep = createTrianglePoints(vertices, displaySettings);
            pointViewSep->addChild(triangleSep);
        } else {
            // Default: Square points (SoPointSet)
            SoPointSet* pointSet = new SoPointSet();
            pointSet->numPoints.setValue(static_cast<int>(vertices.size()));
            pointViewSep->addChild(pointSet);
        }

        return pointViewSep;
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in PointViewRenderer::createPointViewNode: " + std::string(e.what()));
        return nullptr;
    }
}

SoSeparator* PointViewRenderer::createCirclePoints(
    const std::vector<gp_Pnt>& vertices,
    const DisplaySettings& displaySettings)
{
    SoSeparator* circleSep = new SoSeparator();
    circleSep->renderCaching.setValue(SoSeparator::OFF);
    circleSep->boundingBoxCaching.setValue(SoSeparator::OFF);
    circleSep->pickCulling.setValue(SoSeparator::OFF);

    SoMaterial* pointMaterial = new SoMaterial();
    Standard_Real r, g, b;
    displaySettings.pointViewColor.Values(r, g, b, Quantity_TOC_RGB);
    pointMaterial->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    pointMaterial->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    circleSep->addChild(pointMaterial);

    for (size_t i = 0; i < vertices.size(); ++i) {
        const gp_Pnt& vertex = vertices[i];

        SoSeparator* sphereSep = new SoSeparator();
        sphereSep->renderCaching.setValue(SoSeparator::OFF);
        sphereSep->boundingBoxCaching.setValue(SoSeparator::OFF);
        sphereSep->pickCulling.setValue(SoSeparator::OFF);

        SoTranslation* translation = new SoTranslation();
        translation->translation.setValue(
            static_cast<float>(vertex.X()),
            static_cast<float>(vertex.Y()),
            static_cast<float>(vertex.Z())
        );
        sphereSep->addChild(translation);

        SoScale* scale = new SoScale();
        float scaleFactor = static_cast<float>(displaySettings.pointViewSize) / 10.0f;
        scale->scaleFactor.setValue(scaleFactor, scaleFactor, scaleFactor);
        sphereSep->addChild(scale);

        SoSphere* sphere = new SoSphere();
        sphereSep->addChild(sphere);

        circleSep->addChild(sphereSep);
    }

    return circleSep;
}

SoSeparator* PointViewRenderer::createTrianglePoints(
    const std::vector<gp_Pnt>& vertices,
    const DisplaySettings& displaySettings)
{
    SoSeparator* triangleSep = new SoSeparator();
    triangleSep->renderCaching.setValue(SoSeparator::OFF);
    triangleSep->boundingBoxCaching.setValue(SoSeparator::OFF);
    triangleSep->pickCulling.setValue(SoSeparator::OFF);

    SoMaterial* pointMaterial = new SoMaterial();
    Standard_Real r, g, b;
    displaySettings.pointViewColor.Values(r, g, b, Quantity_TOC_RGB);
    pointMaterial->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    pointMaterial->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    triangleSep->addChild(pointMaterial);

    for (size_t i = 0; i < vertices.size(); ++i) {
        const gp_Pnt& vertex = vertices[i];

        SoSeparator* coneSep = new SoSeparator();
        coneSep->renderCaching.setValue(SoSeparator::OFF);
        coneSep->boundingBoxCaching.setValue(SoSeparator::OFF);
        coneSep->pickCulling.setValue(SoSeparator::OFF);

        SoTranslation* translation = new SoTranslation();
        translation->translation.setValue(
            static_cast<float>(vertex.X()),
            static_cast<float>(vertex.Y()),
            static_cast<float>(vertex.Z())
        );
        coneSep->addChild(translation);

        SoScale* scale = new SoScale();
        float scaleFactor = static_cast<float>(displaySettings.pointViewSize) / 10.0f;
        scale->scaleFactor.setValue(scaleFactor, scaleFactor, scaleFactor);
        coneSep->addChild(scale);

        SoCone* cone = new SoCone();
        coneSep->addChild(cone);

        triangleSep->addChild(coneSep);
    }

    return triangleSep;
}

SoSeparator* PointViewRenderer::createSquarePoints(
    const std::vector<gp_Pnt>& vertices,
    const DisplaySettings& displaySettings)
{
    // This is handled directly in createPointViewNode using SoPointSet
    // This method is kept for consistency but not used
    return nullptr;
}

