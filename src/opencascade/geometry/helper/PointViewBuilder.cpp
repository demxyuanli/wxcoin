#include "geometry/helper/PointViewBuilder.h"
#include "OCCMeshConverter.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoScale.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>

PointViewBuilder::PointViewBuilder() {
}

PointViewBuilder::~PointViewBuilder() {
}

void PointViewBuilder::createPointViewRepresentation(SoSeparator* coinNode,
                                                      const TopoDS_Shape& shape,
                                                      const MeshParameters& params,
                                                      const DisplaySettings& displaySettings) {
    if (!coinNode) {
        return;
    }
    
    try {
        OCCMeshConverter::MeshParameters occParams;
        occParams.deflection = params.deflection;
        occParams.angularDeflection = params.angularDeflection;
        occParams.relative = params.relative;
        occParams.inParallel = params.inParallel;

        TriangleMesh mesh = OCCMeshConverter::convertToMesh(shape, occParams);

        if (mesh.vertices.empty()) {
            return;
        }

        SoSeparator* pointViewSep = new SoSeparator();
        pointViewSep->renderCaching.setValue(SoSeparator::OFF);
        pointViewSep->boundingBoxCaching.setValue(SoSeparator::OFF);
        pointViewSep->pickCulling.setValue(SoSeparator::OFF);

        SoMaterial* pointMaterial = new SoMaterial();
        Standard_Real r, g, b;
        displaySettings.pointViewColor.Values(r, g, b, Quantity_TOC_RGB);
        pointMaterial->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        pointMaterial->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        pointViewSep->addChild(pointMaterial);

        SoDrawStyle* pointStyle = new SoDrawStyle();
        pointStyle->pointSize.setValue(static_cast<float>(displaySettings.pointViewSize));
        pointViewSep->addChild(pointStyle);

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

        pointViewSep->addChild(coords);

        SoPointSet* pointSet = new SoPointSet();
        pointSet->numPoints.setValue(static_cast<int>(mesh.vertices.size()));

        if (displaySettings.pointViewShape == 1) {
            SoSeparator* circleSep = new SoSeparator();
            circleSep->renderCaching.setValue(SoSeparator::OFF);
            circleSep->boundingBoxCaching.setValue(SoSeparator::OFF);
            circleSep->pickCulling.setValue(SoSeparator::OFF);
            circleSep->addChild(pointMaterial);
            
            for (size_t i = 0; i < mesh.vertices.size(); ++i) {
                const gp_Pnt& vertex = mesh.vertices[i];
                
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
            
            pointViewSep->addChild(circleSep);
        } else if (displaySettings.pointViewShape == 2) {
            SoSeparator* triangleSep = new SoSeparator();
            triangleSep->renderCaching.setValue(SoSeparator::OFF);
            triangleSep->boundingBoxCaching.setValue(SoSeparator::OFF);
            triangleSep->pickCulling.setValue(SoSeparator::OFF);
            triangleSep->addChild(pointMaterial);
            
            for (size_t i = 0; i < mesh.vertices.size(); ++i) {
                const gp_Pnt& vertex = mesh.vertices[i];
                
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
            
            pointViewSep->addChild(triangleSep);
        } else {
            pointViewSep->addChild(pointSet);
        }

        pointViewSep->ref();
        coinNode->addChild(pointViewSep);

    } catch (const std::exception& e) {
    }
}

