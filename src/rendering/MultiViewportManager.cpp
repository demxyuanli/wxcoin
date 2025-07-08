#include "MultiViewportManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "NavigationCubeManager.h"
#include "DPIManager.h"
#include "Logger.h"
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>  
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoSphere.h>
#include <GL/gl.h>
#include <cmath>  

MultiViewportManager::MultiViewportManager(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_navigationCubeManager(nullptr)
    , m_cubeOutlineRoot(nullptr)
    , m_coordinateSystemRoot(nullptr)
    , m_cubeOutlineCamera(nullptr)
    , m_coordinateSystemCamera(nullptr)
    , m_margin(10)
    , m_dpiScale(1.0f)
    , m_initialized(false) 
{
    LOG_INF("MultiViewportManager: Initializing");
    initializeViewports();
}

MultiViewportManager::~MultiViewportManager() {
    if (m_cubeOutlineRoot) {
        m_cubeOutlineRoot->unref();
    }
    if (m_coordinateSystemRoot) {
        m_coordinateSystemRoot->unref();
    }
    LOG_INF("MultiViewportManager: Destroyed");
}

void MultiViewportManager::initializeViewports() {
    auto& dpiManager = DPIManager::getInstance();
    m_dpiScale = dpiManager.getDPIScale();
    m_margin = dpiManager.getScaledSize(20);
    
    // Initialize viewport layouts (will be updated in handleSizeChange)
    m_viewports[VIEWPORT_NAVIGATION_CUBE] = ViewportInfo(0, 0, 120, 120, true);
    m_viewports[VIEWPORT_CUBE_OUTLINE] = ViewportInfo(0, 0, 200, 200, true);
    m_viewports[VIEWPORT_COORDINATE_SYSTEM] = ViewportInfo(0, 0, 100, 100, true);
}

void MultiViewportManager::createEquilateralTriangle(float x, float y, float angleRad) {
    SoSeparator* triSep = new SoSeparator;
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
    triSep->addChild(material);

    SoTransform* transform = new SoTransform;
    transform->translation.setValue(x, y, 0);
    transform->rotation.setValue(SbVec3f(0, 0, 1), angleRad);
    triSep->addChild(transform);

    float a = 1.0f;
    float h = a * sqrt(3.0f) / 2.0f;
    SbVec3f tri[3] = {
        SbVec3f(0, 2.0f * h / 3.0f, 0),
        SbVec3f(-a / 2.0f, -h / 3.0f, 0),
        SbVec3f(a / 2.0f, -h / 3.0f, 0)
    };
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setValues(0, 3, tri);
    triSep->addChild(coords);

    SoFaceSet* faceSet = new SoFaceSet;
    faceSet->numVertices.setValue(3);
    triSep->addChild(faceSet);

    m_cubeOutlineRoot->addChild(triSep);
}

void MultiViewportManager::createCubeOutlineScene() {
    m_cubeOutlineRoot = new SoSeparator;
    m_cubeOutlineRoot->ref();
    m_cubeOutlineCamera = new SoOrthographicCamera;
    m_cubeOutlineCamera->position.setValue(0, 0, 5);
    m_cubeOutlineCamera->orientation.setValue(SbRotation::identity());
    m_cubeOutlineCamera->height.setValue(6.0f);
    m_cubeOutlineRoot->addChild(m_cubeOutlineCamera);
    SoDirectionalLight* light = new SoDirectionalLight;
    light->direction.setValue(0, 0, -1);
    m_cubeOutlineRoot->addChild(light);
    float scale = 0.95f;
    createEquilateralTriangle(0, 2.7f * scale, 0);        
    createEquilateralTriangle(0, -2.7f * scale, M_PI);   
    createEquilateralTriangle(-2.7f * scale, 0, M_PI/2);  
    createEquilateralTriangle(2.7f * scale, 0, -M_PI/2);  
    createCurvedArrow(-1, scale); 
    createCurvedArrow(1, scale); 
    createCurvedArrow(-2, scale); 
    createCurvedArrow(2, scale); 
    createTopRightCircle(scale);
    createSmallCube(scale);
}

void MultiViewportManager::createNavigationShapes() {
}


void addOutline(SoSeparator* parent, const std::vector<SbVec3f>& points, bool closed = true) {
    SoMaterial* lineMat = new SoMaterial;
    lineMat->diffuseColor.setValue(0, 0, 0);
    parent->addChild(lineMat);
    SoDrawStyle* style = new SoDrawStyle;
    style->lineWidth = 2;
    parent->addChild(style);
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setValues(0, points.size(), points.data());
    parent->addChild(coords);
    SoLineSet* line = new SoLineSet;
    if (closed) line->numVertices.setValue(points.size());
    else line->numVertices.setValue(points.size());
    parent->addChild(line);
}

void MultiViewportManager::createTopArrow() {
    SoSeparator* arrowSep = new SoSeparator;
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
    arrowSep->addChild(material);
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(0, 2.7f, 0);
    arrowSep->addChild(transform);
    SbVec3f pts[3] = {
        SbVec3f(-0.5f, -0.3f, 0), SbVec3f(0.5f, -0.3f, 0), SbVec3f(0.0f, 0.5f, 0)
    };
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setValues(0, 3, pts);
    arrowSep->addChild(coords);
    SoFaceSet* faceSet = new SoFaceSet;
    faceSet->numVertices.setValue(3);
    arrowSep->addChild(faceSet);
    m_cubeOutlineRoot->addChild(arrowSep);
}

void MultiViewportManager::createBottomTriangle() {
    SoSeparator* triangleSep = new SoSeparator;
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
    triangleSep->addChild(material);
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(0, -2.7f, 0);
    triangleSep->addChild(transform);
    SbVec3f pts[3] = {
        SbVec3f(-0.5f, 0.3f, 0), SbVec3f(0.5f, 0.3f, 0), SbVec3f(0.0f, -0.5f, 0)
    };
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setValues(0, 3, pts);
    triangleSep->addChild(coords);
    SoFaceSet* faceSet = new SoFaceSet;
    faceSet->numVertices.setValue(3);
    triangleSep->addChild(faceSet);
    m_cubeOutlineRoot->addChild(triangleSep);
}

void MultiViewportManager::createSideTriangle(int dir) {
    SoSeparator* triSep = new SoSeparator;
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
    triSep->addChild(material);
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(2.8f * dir, 0, 0);
    triSep->addChild(transform);
    SbVec3f pts[3];
    if (dir < 0) {
        pts[0] = SbVec3f(0.3f, -0.4f, 0); pts[1] = SbVec3f(0.3f, 0.4f, 0); pts[2] = SbVec3f(-0.3f, 0.0f, 0);
    } else {
        pts[0] = SbVec3f(-0.3f, -0.4f, 0); pts[1] = SbVec3f(-0.3f, 0.4f, 0); pts[2] = SbVec3f(0.3f, 0.0f, 0);
    }
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setValues(0, 3, pts);
    triSep->addChild(coords);
    SoFaceSet* faceSet = new SoFaceSet;
    faceSet->numVertices.setValue(3);
    triSep->addChild(faceSet);
    m_cubeOutlineRoot->addChild(triSep);
}

void MultiViewportManager::createCurvedArrow(int dir, float scale) {
    SoSeparator* arrowSep = new SoSeparator;
    if (dir == -1) {
        float radius = 2.7f * scale;
        float startAngle = 110.0f * M_PI / 180.0f; 
        float endAngle = 145.0f * M_PI / 180.0f;   
        int numSegments = 24;
        std::vector<SbVec3f> arcPoints;
        for (int i = 0; i <= numSegments; ++i) {
            float t = float(i) / numSegments;
            float angle = startAngle + (endAngle - startAngle) * t;
            arcPoints.push_back(SbVec3f(radius * cos(angle), radius * sin(angle), 0));
        }
        SoCoordinate3* arcCoords = new SoCoordinate3;
        arcCoords->point.setValues(0, arcPoints.size(), arcPoints.data());
        arrowSep->addChild(arcCoords);
        SoDrawStyle* arcStyle = new SoDrawStyle;
        arcStyle->lineWidth = 6.0f;
        arrowSep->addChild(arcStyle);
        SoLineSet* arcLine = new SoLineSet;
        arcLine->numVertices.setValue(arcPoints.size());
        arrowSep->addChild(arcLine);
        float ex = radius * cos(endAngle);
        float ey = radius * sin(endAngle);
        float tx = -sin(endAngle); 
        float ty = cos(endAngle);
        float nx = cos(endAngle);  
        float ny = sin(endAngle);
        float arrowLength = 0.8f * scale;
        float arrowWidth = 0.8f * scale;
        SbVec3f tip(ex + tx * arrowLength, ey + ty * arrowLength, 0);
        SbVec3f left(ex + nx * (arrowWidth/2), ey + ny * (arrowWidth/2), 0);
        SbVec3f right(ex - nx * (arrowWidth/2), ey - ny * (arrowWidth/2), 0);
        SbVec3f tri[3] = { right, left, tip };
        SoSeparator* headSep = new SoSeparator;
        SoMaterial* headMat = new SoMaterial;
        headMat->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
        headSep->addChild(headMat);
        SoCoordinate3* headCoords = new SoCoordinate3;
        headCoords->point.setValues(0, 3, tri);
        headSep->addChild(headCoords);
        SoFaceSet* headFace = new SoFaceSet;
        headFace->numVertices.setValue(3);
        headSep->addChild(headFace);
        arrowSep->addChild(headSep);
    } else if (dir == 1) {
        float radius = 2.7f * scale;
        float startAngle = 70.0f * M_PI / 180.0f;  
        float endAngle = 35.0f * M_PI / 180.0f;    
        int numSegments = 24;
        std::vector<SbVec3f> arcPoints;
        for (int i = 0; i <= numSegments; ++i) {
            float t = float(i) / numSegments;
            float angle = startAngle + (endAngle - startAngle) * t;
            arcPoints.push_back(SbVec3f(radius * cos(angle), radius * sin(angle), 0));
        }
        SoCoordinate3* arcCoords = new SoCoordinate3;
        arcCoords->point.setValues(0, arcPoints.size(), arcPoints.data());
        arrowSep->addChild(arcCoords);
        SoDrawStyle* arcStyle = new SoDrawStyle;
        arcStyle->lineWidth = 6.0f;
        arrowSep->addChild(arcStyle);
        SoLineSet* arcLine = new SoLineSet;
        arcLine->numVertices.setValue(arcPoints.size());
        arrowSep->addChild(arcLine);
        float ex = radius * cos(endAngle);
        float ey = radius * sin(endAngle);
        float tx = sin(endAngle); 
        float ty = -cos(endAngle);
        float nx = cos(endAngle);  
        float ny = sin(endAngle);
        float arrowLength = 0.8f * scale;
        float arrowWidth = 0.8f * scale;
        SbVec3f tip(ex + tx * arrowLength, ey + ty * arrowLength, 0);
        SbVec3f left(ex + nx * (arrowWidth/2), ey + ny * (arrowWidth/2), 0);
        SbVec3f right(ex - nx * (arrowWidth/2), ey - ny * (arrowWidth/2), 0);
        SbVec3f tri[3] = { left, right, tip };
        SoSeparator* headSep = new SoSeparator;
        SoMaterial* headMat = new SoMaterial;
        headMat->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
        headSep->addChild(headMat);
        SoCoordinate3* headCoords = new SoCoordinate3;
        headCoords->point.setValues(0, 3, tri);
        headSep->addChild(headCoords);
        SoFaceSet* headFace = new SoFaceSet;
        headFace->numVertices.setValue(3);
        headSep->addChild(headFace);
        arrowSep->addChild(headSep);
    }
    else if (dir == -2) {
        float radius = 2.7f * scale;
        float startAngle = 250.0f * M_PI / 180.0f;
        float endAngle = 215.0f * M_PI / 180.0f;
        int numSegments = 24;
        std::vector<SbVec3f> arcPoints;
        for (int i = 0; i <= numSegments; ++i) {
            float t = float(i) / numSegments;
            float angle = startAngle + (endAngle - startAngle) * t;
            arcPoints.push_back(SbVec3f(radius * cos(angle), radius * sin(angle), 0));
        }
        SoCoordinate3* arcCoords = new SoCoordinate3;
        arcCoords->point.setValues(0, arcPoints.size(), arcPoints.data());
        arrowSep->addChild(arcCoords);
        SoDrawStyle* arcStyle = new SoDrawStyle;
        arcStyle->lineWidth = 6.0f;
        arrowSep->addChild(arcStyle);
        SoLineSet* arcLine = new SoLineSet;
        arcLine->numVertices.setValue(arcPoints.size());
        arrowSep->addChild(arcLine);
        float ex = radius * cos(endAngle);
        float ey = radius * sin(endAngle);
        float tx = sin(endAngle);
        float ty = -cos(endAngle);
        float nx = cos(endAngle);
        float ny = sin(endAngle);
        float arrowLength = 0.8f * scale;
        float arrowWidth = 0.8f * scale;
        SbVec3f tip(ex + tx * arrowLength, ey + ty * arrowLength, 0);
        SbVec3f left(ex + nx * (arrowWidth / 2), ey + ny * (arrowWidth / 2), 0);
        SbVec3f right(ex - nx * (arrowWidth / 2), ey - ny * (arrowWidth / 2), 0);
        SbVec3f tri[3] = { left, right, tip };
        SoSeparator* headSep = new SoSeparator;
        SoMaterial* headMat = new SoMaterial;
        headMat->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
        headSep->addChild(headMat);
        SoCoordinate3* headCoords = new SoCoordinate3;
        headCoords->point.setValues(0, 3, tri);
        headSep->addChild(headCoords);
        SoFaceSet* headFace = new SoFaceSet;
        headFace->numVertices.setValue(3);
        headSep->addChild(headFace);
        arrowSep->addChild(headSep);
    }
    else if (dir == 2) {
        float radius = 2.7f * scale;
        float startAngle = 290.0f * M_PI / 180.0f;
        float endAngle = 325.0f * M_PI / 180.0f;
        int numSegments = 24;
        std::vector<SbVec3f> arcPoints;
        for (int i = 0; i <= numSegments; ++i) {
            float t = float(i) / numSegments;
            float angle = startAngle + (endAngle - startAngle) * t;
            arcPoints.push_back(SbVec3f(radius * cos(angle), radius * sin(angle), 0));
        }
        SoCoordinate3* arcCoords = new SoCoordinate3;
        arcCoords->point.setValues(0, arcPoints.size(), arcPoints.data());
        arrowSep->addChild(arcCoords);
        SoDrawStyle* arcStyle = new SoDrawStyle;
        arcStyle->lineWidth = 6.0f;
        arrowSep->addChild(arcStyle);
        SoLineSet* arcLine = new SoLineSet;
        arcLine->numVertices.setValue(arcPoints.size());
        arrowSep->addChild(arcLine);
        float ex = radius * cos(endAngle);
        float ey = radius * sin(endAngle);
        float tx = -sin(endAngle);
        float ty = cos(endAngle);
        float nx = cos(endAngle);
        float ny = sin(endAngle);
        float arrowLength = 0.8f * scale;
        float arrowWidth = 0.8f * scale;
        SbVec3f tip(ex + tx * arrowLength, ey + ty * arrowLength, 0);
        SbVec3f left(ex + nx * (arrowWidth / 2), ey + ny * (arrowWidth / 2), 0);
        SbVec3f right(ex - nx * (arrowWidth / 2), ey - ny * (arrowWidth / 2), 0);
        SbVec3f tri[3] = { right, left, tip };
    SoSeparator* headSep = new SoSeparator;
    SoMaterial* headMat = new SoMaterial;
    headMat->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
    headSep->addChild(headMat);
    SoCoordinate3* headCoords = new SoCoordinate3;
    headCoords->point.setValues(0, 3, tri);
    headSep->addChild(headCoords);
    SoFaceSet* headFace = new SoFaceSet;
    headFace->numVertices.setValue(3);
    headSep->addChild(headFace);
    arrowSep->addChild(headSep);
    }
    m_cubeOutlineRoot->addChild(arrowSep);
}

void MultiViewportManager::createTopRightCircle(float scale) {
    SoSeparator* sphereSep = new SoSeparator;
    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(0.8f, 1.0f, 0.8f);
    sphereSep->addChild(mat);
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(2.5f * scale, 2.5f * scale, 0);
    transform->scaleFactor.setValue(0.5f * scale, 0.5f * scale, 0.5f * scale);
    sphereSep->addChild(transform);
    SoSphere* sphere = new SoSphere;
    sphere->radius = 1.0f * scale;
    sphereSep->addChild(sphere);
    m_cubeOutlineRoot->addChild(sphereSep);
}

void MultiViewportManager::createSmallCube(float scale) {
    SoSeparator* cubeSep = new SoSeparator;

    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(0.8f, 1.0f, 0.8f);
    cubeSep->addChild(mat);

    SoTransform* transform = new SoTransform;
    transform->translation.setValue(2.5f * scale, -2.5f * scale, 0);
    transform->scaleFactor.setValue(0.6f * scale, 0.6f * scale, 0.6f * scale);
    transform->rotation.setValue(SbRotation(SbVec3f(0,1,0), M_PI/4) * SbRotation(SbVec3f(1,0,0), M_PI/6));
    cubeSep->addChild(transform);

    SoCube* cube = new SoCube;
    cube->width = 1.0f * scale;
    cube->height = 1.0f * scale;
    cube->depth = 1.0f * scale;
    cubeSep->addChild(cube);

    SoMaterial* edgeMat = new SoMaterial;
    edgeMat->diffuseColor.setValue(0, 0, 0);
    cubeSep->addChild(edgeMat);

    SoCoordinate3* edgeCoords = new SoCoordinate3;
    SbVec3f verts[8] = {
        SbVec3f(-0.5f * scale, -0.5f * scale, -0.5f * scale),
        SbVec3f( 0.5f * scale, -0.5f * scale, -0.5f * scale),
        SbVec3f( 0.5f * scale,  0.5f * scale, -0.5f * scale),
        SbVec3f(-0.5f * scale,  0.5f * scale, -0.5f * scale),
        SbVec3f(-0.5f * scale, -0.5f * scale,  0.5f * scale),
        SbVec3f( 0.5f * scale, -0.5f * scale,  0.5f * scale),
        SbVec3f( 0.5f * scale,  0.5f * scale,  0.5f * scale),
        SbVec3f(-0.5f * scale,  0.5f * scale,  0.5f * scale)
    };
    edgeCoords->point.setValues(0, 8, verts);
    cubeSep->addChild(edgeCoords);

    SoIndexedLineSet* edgeLines = new SoIndexedLineSet;
    int32_t edgeIdx[] = {
        0,1, 1,2, 2,3, 3,0, // bottom
        4,5, 5,6, 6,7, 7,4, // top
        0,4, 1,5, 2,6, 3,7, // sides
        SO_END_LINE_INDEX
    };
    edgeLines->coordIndex.setValues(0, 25, edgeIdx);
    cubeSep->addChild(edgeLines);

    m_cubeOutlineRoot->addChild(cubeSep);
}

void MultiViewportManager::createCoordinateSystemScene() {
    m_coordinateSystemRoot = new SoSeparator;
    m_coordinateSystemRoot->ref();
    
    // Create orthographic camera
    m_coordinateSystemCamera = new SoOrthographicCamera;
    m_coordinateSystemCamera->position.setValue(0, 0, 5);
    m_coordinateSystemCamera->orientation.setValue(SbRotation::identity());
    m_coordinateSystemCamera->height.setValue(3.0f);
    m_coordinateSystemRoot->addChild(m_coordinateSystemCamera);
    
    // Add lighting
    SoDirectionalLight* light = new SoDirectionalLight;
    light->direction.setValue(0, 0, -1);
    m_coordinateSystemRoot->addChild(light);
    
    int coordSize = 80;
    if (m_viewports[VIEWPORT_COORDINATE_SYSTEM].width > 0) {
        coordSize = m_viewports[VIEWPORT_COORDINATE_SYSTEM].width;
    }
    float axisLength = (coordSize * 0.42f) / 30.0f; 

    // Create coordinate axes
    SoSeparator* axesSep = new SoSeparator;
    // X axis
    SoSeparator* xAxisSep = new SoSeparator;
    SoMaterial* xMaterial = new SoMaterial;
    xMaterial->diffuseColor.setValue(1.0f, 0.2f, 0.2f);
    xMaterial->emissiveColor.setValue(0.3f, 0.0f, 0.0f);
    xAxisSep->addChild(xMaterial);
    SoCoordinate3* xCoords = new SoCoordinate3;
    xCoords->point.setValues(0, 2, new SbVec3f[2] {
        SbVec3f(0, 0, 0), SbVec3f(axisLength, 0, 0)
    });
    xAxisSep->addChild(xCoords);
    SoLineSet* xLine = new SoLineSet;
    xLine->numVertices.setValue(2);
    xAxisSep->addChild(xLine);
    // X label
    SoTranslation* xTrans = new SoTranslation;
    xTrans->translation.setValue(axisLength + 0.2f, 0, 0);
    xAxisSep->addChild(xTrans);
    SoText2* xText = new SoText2;
    xText->string.setValue("X");
    xAxisSep->addChild(xText);
    axesSep->addChild(xAxisSep);
    // Y axis
    SoSeparator* yAxisSep = new SoSeparator;
    SoMaterial* yMaterial = new SoMaterial;
    yMaterial->diffuseColor.setValue(0.2f, 1.0f, 0.2f);
    yMaterial->emissiveColor.setValue(0.0f, 0.3f, 0.0f);
    yAxisSep->addChild(yMaterial);
    SoCoordinate3* yCoords = new SoCoordinate3;
    yCoords->point.setValues(0, 2, new SbVec3f[2] {
        SbVec3f(0, 0, 0), SbVec3f(0, axisLength, 0)
    });
    yAxisSep->addChild(yCoords);
    SoLineSet* yLine = new SoLineSet;
    yLine->numVertices.setValue(2);
    yAxisSep->addChild(yLine);
    // Y label
    SoTranslation* yTrans = new SoTranslation;
    yTrans->translation.setValue(0, axisLength + 0.2f, 0);
    yAxisSep->addChild(yTrans);
    SoText2* yText = new SoText2;
    yText->string.setValue("Y");
    yAxisSep->addChild(yText);
    axesSep->addChild(yAxisSep);
    // Z axis
    SoSeparator* zAxisSep = new SoSeparator;
    SoMaterial* zMaterial = new SoMaterial;
    zMaterial->diffuseColor.setValue(0.2f, 0.2f, 1.0f);
    zMaterial->emissiveColor.setValue(0.0f, 0.0f, 0.3f);
    zAxisSep->addChild(zMaterial);
    SoCoordinate3* zCoords = new SoCoordinate3;
    zCoords->point.setValues(0, 2, new SbVec3f[2] {
        SbVec3f(0, 0, 0), SbVec3f(0, 0, axisLength)
    });
    zAxisSep->addChild(zCoords);
    SoLineSet* zLine = new SoLineSet;
    zLine->numVertices.setValue(2);
    zAxisSep->addChild(zLine);
    // Z label
    SoTranslation* zTrans = new SoTranslation;
    zTrans->translation.setValue(0, 0, axisLength + 0.2f);
    zAxisSep->addChild(zTrans);
    SoText2* zText = new SoText2;
    zText->string.setValue("Z");
    zAxisSep->addChild(zText);
    axesSep->addChild(zAxisSep);
    m_coordinateSystemRoot->addChild(axesSep);
}

void MultiViewportManager::render() {
    if (!m_canvas || !m_sceneManager) {
        return;
    }
    
    // Initialize scene graphs on first render when GL context is active
    if (!m_initialized) {
        try {
            createCubeOutlineScene();
            createCoordinateSystemScene();
            m_initialized = true;
            LOG_INF("MultiViewportManager: Scene graphs initialized successfully");
        } catch (const std::exception& e) {
            LOG_ERR("MultiViewportManager: Failed to initialize scene graphs: " + std::string(e.what()));
            return;
        }
    }
    
    // Log viewport status for debugging
    LOG_DBG("MultiViewportManager: render() called - Canvas size: " + 
            std::to_string(m_canvas->GetClientSize().x) + "x" + 
            std::to_string(m_canvas->GetClientSize().y));
    
    if (m_viewports[VIEWPORT_CUBE_OUTLINE].enabled) {
        LOG_DBG("MultiViewportManager: Cube outline viewport - x:" + 
                std::to_string(m_viewports[VIEWPORT_CUBE_OUTLINE].x) + 
                " y:" + std::to_string(m_viewports[VIEWPORT_CUBE_OUTLINE].y) + 
                " w:" + std::to_string(m_viewports[VIEWPORT_CUBE_OUTLINE].width) + 
                " h:" + std::to_string(m_viewports[VIEWPORT_CUBE_OUTLINE].height));
        renderCubeOutline();
    }

    if (m_viewports[VIEWPORT_COORDINATE_SYSTEM].enabled) {
        LOG_DBG("MultiViewportManager: Coordinate system viewport - x:" + 
                std::to_string(m_viewports[VIEWPORT_COORDINATE_SYSTEM].x) + 
                " y:" + std::to_string(m_viewports[VIEWPORT_COORDINATE_SYSTEM].y) + 
                " w:" + std::to_string(m_viewports[VIEWPORT_COORDINATE_SYSTEM].width) + 
                " h:" + std::to_string(m_viewports[VIEWPORT_COORDINATE_SYSTEM].height));
        renderCoordinateSystem();
    }

    if (m_viewports[VIEWPORT_NAVIGATION_CUBE].enabled && m_navigationCubeManager) {
        LOG_DBG("MultiViewportManager: Rendering navigation cube");
        renderNavigationCube();
    }
}

void MultiViewportManager::renderNavigationCube() {
    if (m_navigationCubeManager) {
        m_navigationCubeManager->render();
    }
}

void MultiViewportManager::renderCubeOutline() {
    LOG_DBG("MultiViewportManager: Rendering cube outline viewport");

    if (!m_cubeOutlineRoot || !m_cubeOutlineCamera) {
        LOG_WRN("MultiViewportManager: Cube outline scene not initialized");
        return;
    }
    
    ViewportInfo& viewport = m_viewports[VIEWPORT_CUBE_OUTLINE];
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    wxSize canvasSize = m_canvas->GetClientSize();
    int yBottom = canvasSize.y - viewport.y - viewport.height;

    glEnable(GL_SCISSOR_TEST);
    glScissor(viewport.x, yBottom, viewport.width, viewport.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    SbViewportRegion viewportRegion;
    viewportRegion.setWindowSize(SbVec2s(canvasSize.x, canvasSize.y));
    viewportRegion.setViewportPixels(viewport.x, yBottom, viewport.width, viewport.height);

    SoGLRenderAction renderAction(viewportRegion);
    renderAction.setSmoothing(true);
    renderAction.setTransparencyType(SoGLRenderAction::BLEND);

    renderAction.apply(m_cubeOutlineRoot);

    glPopMatrix();
    glPopAttrib();
}

void MultiViewportManager::renderCoordinateSystem() {
    LOG_DBG("MultiViewportManager: Rendering coordinate system viewport");

    if (!m_coordinateSystemRoot || !m_coordinateSystemCamera) {
        LOG_WRN("MultiViewportManager: Coordinate system scene not initialized");
        return;
    }

    syncCoordinateSystemCameraToMain();

    ViewportInfo& viewport = m_viewports[VIEWPORT_COORDINATE_SYSTEM];
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    wxSize canvasSize = m_canvas->GetClientSize();
    int yBottom = canvasSize.y - viewport.y - viewport.height;

    glEnable(GL_SCISSOR_TEST);
    glScissor(viewport.x, yBottom, viewport.width, viewport.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    SbViewportRegion viewportRegion;
    viewportRegion.setWindowSize(SbVec2s(canvasSize.x, canvasSize.y));
    viewportRegion.setViewportPixels(viewport.x, yBottom, viewport.width, viewport.height);

    SoGLRenderAction renderAction(viewportRegion);
    renderAction.setSmoothing(true);
    renderAction.setTransparencyType(SoGLRenderAction::BLEND);

    renderAction.apply(m_coordinateSystemRoot);

    glPopMatrix();
    glPopAttrib();
}

void MultiViewportManager::syncCameraWithMain(SoCamera* targetCamera) {
    if (!targetCamera || !m_sceneManager) {
        return;
    }
    
    SoCamera* mainCamera = m_sceneManager->getCamera();
    if (mainCamera) {
        targetCamera->orientation.setValue(mainCamera->orientation.getValue());
    }
}

void MultiViewportManager::handleSizeChange(const wxSize& canvasSize) {
    updateViewportLayouts(canvasSize);
}

void MultiViewportManager::updateViewportLayouts(const wxSize& canvasSize) {
    int margin = static_cast<int>(m_margin * m_dpiScale);
    
    // Navigation cube (top-right)
    int cubeSize = static_cast<int>(100 * m_dpiScale);
    m_viewports[VIEWPORT_NAVIGATION_CUBE] = ViewportInfo(
        canvasSize.x - cubeSize - margin,
        canvasSize.y - cubeSize - margin,
        cubeSize,
        cubeSize
    );
    
    int outlineSize = static_cast<int>(100 * m_dpiScale);
    m_viewports[VIEWPORT_CUBE_OUTLINE] = ViewportInfo(
        canvasSize.x - outlineSize - margin, 
        margin,                             
        outlineSize,
        outlineSize
    );
    
    // Coordinate system (bottom-right)
    int coordSize = static_cast<int>(80 * m_dpiScale);
    m_viewports[VIEWPORT_COORDINATE_SYSTEM] = ViewportInfo(
        canvasSize.x - coordSize - margin,
        canvasSize.y - coordSize - margin,
        coordSize,
        coordSize
    );
}

void MultiViewportManager::handleDPIChange() {
    auto& dpiManager = DPIManager::getInstance();
    m_dpiScale = dpiManager.getDPIScale();
    m_margin = dpiManager.getScaledSize(20);
}

bool MultiViewportManager::handleMouseEvent(wxMouseEvent& event) {
    // Check if mouse is in any viewport and handle accordingly
    float x = event.GetX();
    float y = event.GetY();
    
    // Check navigation cube viewport
    if (m_viewports[VIEWPORT_NAVIGATION_CUBE].enabled && m_navigationCubeManager) {
        ViewportInfo& viewport = m_viewports[VIEWPORT_NAVIGATION_CUBE];
        if (x >= viewport.x && x < (viewport.x + viewport.width) &&
            y >= viewport.y && y < (viewport.y + viewport.height)) {
            return m_navigationCubeManager->handleMouseEvent(event);
        }
    }
    
    // Add handling for other viewports if needed
    
    return false;
}

void MultiViewportManager::setNavigationCubeManager(NavigationCubeManager* manager) {
    m_navigationCubeManager = manager;
}

void MultiViewportManager::setViewportEnabled(ViewportType type, bool enabled) {
    if (type >= 0 && type < VIEWPORT_COUNT) {
        m_viewports[type].enabled = enabled;
    }
}

bool MultiViewportManager::isViewportEnabled(ViewportType type) const {
    if (type >= 0 && type < VIEWPORT_COUNT) {
        return m_viewports[type].enabled;
    }
    return false;
}

void MultiViewportManager::setViewportRect(ViewportType type, int x, int y, int width, int height) {
    if (type >= 0 && type < VIEWPORT_COUNT) {
        m_viewports[type] = ViewportInfo(x, y, width, height, m_viewports[type].enabled);
    }
}

ViewportInfo MultiViewportManager::getViewportInfo(ViewportType type) const {
    if (type >= 0 && type < VIEWPORT_COUNT) {
        return m_viewports[type];
    }
    return ViewportInfo();
}

void MultiViewportManager::syncCoordinateSystemCameraToMain() {
    if (!m_coordinateSystemCamera || !m_sceneManager) return;
    SoCamera* mainCamera = m_sceneManager->getCamera();
    if (mainCamera) {
        SbRotation mainOrient = mainCamera->orientation.getValue();
        float distance = 5.0f; 
        SbVec3f srcVec(0, 0, -1);
        SbVec3f viewVec;
        mainOrient.multVec(srcVec, viewVec);
        SbVec3f camPos = -viewVec * distance;

        m_coordinateSystemCamera->position.setValue(camPos);
        m_coordinateSystemCamera->orientation.setValue(mainOrient);
    }
}