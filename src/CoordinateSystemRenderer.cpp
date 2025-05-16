#include "CoordinateSystemRenderer.h"
#include "Logger.h"
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoTransform.h>

const float CoordinateSystemRenderer::COORD_PLANE_SIZE = 4.0f;
const float CoordinateSystemRenderer::COORD_PLANE_TRANSPARENCY = 1.0f;

CoordinateSystemRenderer::CoordinateSystemRenderer(SoSeparator* objectRoot)
    : m_objectRoot(objectRoot)
{
    LOG_INF("CoordinateSystemRenderer initializing");
    createCoordinateSystem();
}

CoordinateSystemRenderer::~CoordinateSystemRenderer() {
    LOG_INF("CoordinateSystemRenderer destroying");
}

void CoordinateSystemRenderer::createCoordinateSystem() {
    SoSeparator* coordSystemSep = new SoSeparator;
    SoTransform* originTransform = new SoTransform;
    originTransform->translation.setValue(0.0f, 0.0f, 0.0f);
    originTransform->rotation.setValue(SbRotation::identity());
    originTransform->scaleFactor.setValue(1.0f, 1.0f, 1.0f);
    coordSystemSep->addChild(originTransform);

    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    coordSystemSep->addChild(hints);

    SoDrawStyle* globalStyle = new SoDrawStyle;
    globalStyle->linePattern = 0xFFFF;
    globalStyle->lineWidth = 1.0f;
    globalStyle->pointSize = 1.0f;
    coordSystemSep->addChild(globalStyle);

    // X plane (YZ plane)
    SoSeparator* xPlaneSep = new SoSeparator;
    SoMaterial* xMaterial = new SoMaterial;
    xMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    xMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
    xPlaneSep->addChild(xMaterial);

    SoDrawStyle* xDrawStyle = new SoDrawStyle;
    xDrawStyle->style = SoDrawStyle::FILLED;
    xPlaneSep->addChild(xDrawStyle);

    SoFaceSet* xFaceSet = new SoFaceSet;
    SoVertexProperty* xVertices = new SoVertexProperty;
    float s = COORD_PLANE_SIZE / 2.0f;
    xVertices->vertex.set1Value(0, SbVec3f(0.0f, -s, -s));
    xVertices->vertex.set1Value(1, SbVec3f(0.0f, s, -s));
    xVertices->vertex.set1Value(2, SbVec3f(0.0f, s, s));
    xVertices->vertex.set1Value(3, SbVec3f(0.0f, -s, s));
    xFaceSet->vertexProperty = xVertices;
    xFaceSet->numVertices.set1Value(0, 4);
    xPlaneSep->addChild(xFaceSet);

    SoSeparator* xLineSep = new SoSeparator;
    SoMaterial* xLineMaterial = new SoMaterial;
    xLineMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    xLineMaterial->transparency.setValue(0.0f);
    xLineSep->addChild(xLineMaterial);

    SoDrawStyle* xLineStyle = new SoDrawStyle;
    xLineStyle->style = SoDrawStyle::LINES;
    xLineStyle->lineWidth = 1.0f;
    xLineSep->addChild(xLineStyle);

    SoIndexedLineSet* xLines = new SoIndexedLineSet;
    xLines->vertexProperty = xVertices;
    xLines->coordIndex.set1Value(0, 0);
    xLines->coordIndex.set1Value(1, 1);
    xLines->coordIndex.set1Value(2, 2);
    xLines->coordIndex.set1Value(3, 3);
    xLines->coordIndex.set1Value(4, 0);
    xLines->coordIndex.set1Value(5, -1);
    xLineSep->addChild(xLines);
    xPlaneSep->addChild(xLineSep);
    coordSystemSep->addChild(xPlaneSep);

    // Y plane (XZ plane)
    SoSeparator* yPlaneSep = new SoSeparator;
    SoMaterial* yMaterial = new SoMaterial;
    yMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    yMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
    yPlaneSep->addChild(yMaterial);

    SoDrawStyle* yDrawStyle = new SoDrawStyle;
    yDrawStyle->style = SoDrawStyle::FILLED;
    yPlaneSep->addChild(yDrawStyle);

    SoFaceSet* yFaceSet = new SoFaceSet;
    SoVertexProperty* yVertices = new SoVertexProperty;
    yVertices->vertex.set1Value(0, SbVec3f(-s, 0.0f, -s));
    yVertices->vertex.set1Value(1, SbVec3f(s, 0.0f, -s));
    yVertices->vertex.set1Value(2, SbVec3f(s, 0.0f, s));
    yVertices->vertex.set1Value(3, SbVec3f(-s, 0.0f, s));
    yFaceSet->vertexProperty = yVertices;
    yFaceSet->numVertices.set1Value(0, 4);
    yPlaneSep->addChild(yFaceSet);

    SoSeparator* yLineSep = new SoSeparator;
    SoMaterial* yLineMaterial = new SoMaterial;
    yLineMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    yLineMaterial->transparency.setValue(0.0f);
    yLineSep->addChild(yLineMaterial);

    SoDrawStyle* yLineStyle = new SoDrawStyle;
    yLineStyle->style = SoDrawStyle::LINES;
    yLineStyle->lineWidth = 1.0f;
    yLineSep->addChild(yLineStyle);

    SoIndexedLineSet* yLines = new SoIndexedLineSet;
    yLines->vertexProperty = yVertices;
    yLines->coordIndex.set1Value(0, 0);
    yLines->coordIndex.set1Value(1, 1);
    yLines->coordIndex.set1Value(2, 2);
    yLines->coordIndex.set1Value(3, 3);
    yLines->coordIndex.set1Value(4, 0);
    yLines->coordIndex.set1Value(5, -1);
    yLineSep->addChild(yLines);
    yPlaneSep->addChild(yLineSep);
    coordSystemSep->addChild(yPlaneSep);

    // Z plane (XY plane)
    SoSeparator* zPlaneSep = new SoSeparator;
    SoMaterial* zMaterial = new SoMaterial;
    zMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    zMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
    zPlaneSep->addChild(zMaterial);

    SoDrawStyle* zDrawStyle = new SoDrawStyle;
    zDrawStyle->style = SoDrawStyle::FILLED;
    zPlaneSep->addChild(zDrawStyle);

    SoFaceSet* zFaceSet = new SoFaceSet;
    SoVertexProperty* zVertices = new SoVertexProperty;
    zVertices->vertex.set1Value(0, SbVec3f(-s, -s, 0.0f));
    zVertices->vertex.set1Value(1, SbVec3f(s, -s, 0.0f));
    zVertices->vertex.set1Value(2, SbVec3f(s, s, 0.0f));
    zVertices->vertex.set1Value(3, SbVec3f(-s, s, 0.0f));
    zFaceSet->vertexProperty = zVertices;
    zFaceSet->numVertices.set1Value(0, 4);
    zPlaneSep->addChild(zFaceSet);

    SoSeparator* zLineSep = new SoSeparator;
    SoMaterial* zLineMaterial = new SoMaterial;
    zLineMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    zLineMaterial->transparency.setValue(0.0f);
    zLineSep->addChild(zLineMaterial);

    SoDrawStyle* zLineStyle = new SoDrawStyle;
    zLineStyle->style = SoDrawStyle::LINES;
    zLineStyle->lineWidth = 1.0f;
    zLineSep->addChild(zLineStyle);

    SoIndexedLineSet* zLines = new SoIndexedLineSet;
    zLines->vertexProperty = zVertices;
    zLines->coordIndex.set1Value(0, 0);
    zLines->coordIndex.set1Value(1, 1);
    zLines->coordIndex.set1Value(2, 2);
    zLines->coordIndex.set1Value(3, 3);
    zLines->coordIndex.set1Value(4, 0);
    zLines->coordIndex.set1Value(5, -1);
    zLineSep->addChild(zLines);
    zPlaneSep->addChild(zLineSep);
    coordSystemSep->addChild(zPlaneSep);

    // X axis
    SoSeparator* xAxisSep = new SoSeparator;
    SoMaterial* xAxisMaterial = new SoMaterial;
    xAxisMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    xAxisMaterial->transparency.setValue(0.0f);
    xAxisSep->addChild(xAxisMaterial);

    SoDrawStyle* xAxisStyle = new SoDrawStyle;
    xAxisStyle->lineWidth = 1.0f;
    xAxisSep->addChild(xAxisStyle);

    SoCoordinate3* xAxisCoords = new SoCoordinate3;
    xAxisCoords->point.set1Value(0, SbVec3f(-s, 0.0f, 0.0f));
    xAxisCoords->point.set1Value(1, SbVec3f(s, 0.0f, 0.0f));
    xAxisSep->addChild(xAxisCoords);

    SoLineSet* xAxisLine = new SoLineSet;
    xAxisLine->numVertices.setValue(2);
    xAxisSep->addChild(xAxisLine);
    coordSystemSep->addChild(xAxisSep);

    // Y axis
    SoSeparator* yAxisSep = new SoSeparator;
    SoMaterial* yAxisMaterial = new SoMaterial;
    yAxisMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    yAxisMaterial->transparency.setValue(0.0f);
    yAxisSep->addChild(yAxisMaterial);

    SoDrawStyle* yAxisStyle = new SoDrawStyle;
    yAxisStyle->lineWidth = 1.0f;
    yAxisSep->addChild(yAxisStyle);

    SoCoordinate3* yAxisCoords = new SoCoordinate3;
    yAxisCoords->point.set1Value(0, SbVec3f(0.0f, -s, 0.0f));
    yAxisCoords->point.set1Value(1, SbVec3f(0.0f, s, 0.0f));
    yAxisSep->addChild(yAxisCoords);

    SoLineSet* yAxisLine = new SoLineSet;
    yAxisLine->numVertices.setValue(2);
    yAxisSep->addChild(yAxisLine);
    coordSystemSep->addChild(yAxisSep);

    // Z axis
    SoSeparator* zAxisSep = new SoSeparator;
    SoMaterial* zAxisMaterial = new SoMaterial;
    zAxisMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    zAxisMaterial->transparency.setValue(0.0f);
    zAxisSep->addChild(zAxisMaterial);

    SoDrawStyle* zAxisStyle = new SoDrawStyle;
    zAxisStyle->lineWidth = 1.0f;
    zAxisSep->addChild(zAxisStyle);

    SoCoordinate3* zAxisCoords = new SoCoordinate3;
    zAxisCoords->point.set1Value(0, SbVec3f(0.0f, 0.0f, -s));
    zAxisCoords->point.set1Value(1, SbVec3f(0.0f, 0.0f, s));
    zAxisSep->addChild(zAxisCoords);

    SoLineSet* zAxisLine = new SoLineSet;
    zAxisLine->numVertices.setValue(2);
    zAxisSep->addChild(zAxisLine);
    coordSystemSep->addChild(zAxisSep);

    m_objectRoot->addChild(coordSystemSep);
}