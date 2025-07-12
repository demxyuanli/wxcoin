#include "MultiViewportManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "NavigationCubeManager.h"
#include "DPIManager.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>

#include <Inventor/nodes/SoNode.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/lists/SoPickedPointList.h>
#include <Inventor/SoPickedPoint.h>
#include <GL/gl.h>
#include <cmath>
#include <vector>

MultiViewportManager::MultiViewportManager(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_navigationCubeManager(nullptr)
    , m_cubeOutlineRoot(nullptr)
    , m_cubeOutlineCamera(nullptr)
    , m_coordinateSystemRoot(nullptr)
    , m_coordinateSystemCamera(nullptr)
    , m_transparentBackground(true)
    , m_margin(20)
    , m_dpiScale(1.0f)
    , m_initialized(false) {
    LOG_INF_S("MultiViewportManager: Initializing");
    
    if (!m_canvas) {
        LOG_ERR_S("MultiViewportManager: Canvas is null");
    }
    if (!m_sceneManager) {
        LOG_ERR_S("MultiViewportManager: SceneManager is null");
    }
    
    initializeViewports();
    LOG_INF_S("MultiViewportManager: Initialization completed");
}

MultiViewportManager::~MultiViewportManager() {
    LOG_INF_S("MultiViewportManager: Destroyed");
    
    if (m_cubeOutlineRoot) {
        m_cubeOutlineRoot->unref();
    }
    
    if (m_coordinateSystemRoot) {
        m_coordinateSystemRoot->unref();
    }
}

void MultiViewportManager::initializeViewports() {
    auto& dpiManager = DPIManager::getInstance();
    m_dpiScale = dpiManager.getDPIScale();
    m_margin = dpiManager.getScaledSize(20);
    
    // Initialize viewport layouts (will be updated in handleSizeChange)
    m_viewports[VIEWPORT_NAVIGATION_CUBE] = ViewportInfo(0, 0, 100, 100, true);
    m_viewports[VIEWPORT_CUBE_OUTLINE] = ViewportInfo(0, 0, 300, 300, true);
    m_viewports[VIEWPORT_COORDINATE_SYSTEM] = ViewportInfo(0, 0, 100, 100, true);
}

void MultiViewportManager::createEquilateralTriangle(float x, float y, float angleRad) {
    SoSeparator* triSep = new SoSeparator;
    
    // Enable picking for this shape
    SoPickStyle* pickStyle = new SoPickStyle;
    pickStyle->style.setValue(SoPickStyle::SHAPE);
    triSep->addChild(pickStyle);
    
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
    
    // Determine triangle name based on position
    std::string triangleName;
    if (y > 0) {
        triangleName = "Top Triangle";
    } else if (y < 0) {
        triangleName = "Bottom Triangle";
    } else if (x < 0) {
        triangleName = "Left Triangle";
    } else {
        triangleName = "Right Triangle";
    }
    
    // Set the name on the root separator for easier identification
    triSep->setName(triangleName.c_str());
    
    triSep->addChild(faceSet);
    
    // Store as composite shape
    CompositeShape compositeShape(triSep, triangleName);
    m_compositeShapes.push_back(compositeShape);

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
    
    // Debug: Print all composite shapes
    LOG_INF_S("Created composite shapes:");
    for (const auto& compositeShape : m_compositeShapes) {
        LOG_INF_S("  " + compositeShape.shapeName);
    }
}



void MultiViewportManager::createCurvedArrow(int dir, float scale) {
    SoSeparator* arrowSep = new SoSeparator;
    
    // Enable picking for this shape
    SoPickStyle* pickStyle = new SoPickStyle;
    pickStyle->style.setValue(SoPickStyle::SHAPE);
    arrowSep->addChild(pickStyle);
    
    // Determine arrow name based on direction
    std::string arrowName;
    switch (dir) {
        case -1: arrowName = "Top Left Arrow"; break;
        case 1: arrowName = "Top Right Arrow"; break;
        case -2: arrowName = "Bottom Left Arrow"; break;
        case 2: arrowName = "Bottom Right Arrow"; break;
        default: arrowName = "Unknown Arrow"; break;
    }
    
    // Set the name on the root separator for easier identification
    arrowSep->setName(arrowName.c_str());
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
        headFace->setName("Top Arrow Head");
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
        arcLine->setName("Bottom Arrow Line");
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
        headFace->setName("Bottom Arrow Head");
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
        arcLine->setName("Left Arrow Line");
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
        headFace->setName("Left Arrow Head");
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
        arcLine->setName("Right Arrow Line");
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
    headFace->setName("Right Arrow Head");
    headSep->addChild(headFace);
    arrowSep->addChild(headSep);
    }
    
    // Store as composite shape
    CompositeShape compositeShape(arrowSep, arrowName);
    m_compositeShapes.push_back(compositeShape);
    
    m_cubeOutlineRoot->addChild(arrowSep);
}

void MultiViewportManager::createTopRightCircle(float scale) {
    SoSeparator* sphereSep = new SoSeparator;
    
    // Enable picking for this shape
    SoPickStyle* pickStyle = new SoPickStyle;
    pickStyle->style.setValue(SoPickStyle::SHAPE);
    sphereSep->addChild(pickStyle);
    
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
    
    // Set the name on the root separator for easier identification
    sphereSep->setName("Sphere");
    
    // Store as composite shape
    CompositeShape compositeShape(sphereSep, "Sphere");
    m_compositeShapes.push_back(compositeShape);
    
    m_cubeOutlineRoot->addChild(sphereSep);
}

void MultiViewportManager::createSmallCube(float scale) {
    SoSeparator* cubeSep = new SoSeparator;

    // Enable picking for this shape
    SoPickStyle* pickStyle = new SoPickStyle;
    pickStyle->style.setValue(SoPickStyle::SHAPE);
    cubeSep->addChild(pickStyle);

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

    // Set the name on the root separator for easier identification
    cubeSep->setName("Cube");

    // Store as composite shape
    CompositeShape compositeShape(cubeSep, "Cube");
    m_compositeShapes.push_back(compositeShape);

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
        LOG_WRN_S("MultiViewportManager::render - Canvas or SceneManager is null");
        return;
    }
    
    // Initialize scene graphs on first render when GL context is active
    if (!m_initialized) {
        try {
            createCubeOutlineScene();
            createCoordinateSystemScene();
            m_initialized = true;
        } catch (const std::exception& e) {
            LOG_ERR_S("MultiViewportManager: Failed to initialize scene graphs: " + std::string(e.what()));
            return;
        }
    }
    

    
    if (m_viewports[VIEWPORT_CUBE_OUTLINE].enabled) {
        renderCubeOutline();
    }

    if (m_viewports[VIEWPORT_COORDINATE_SYSTEM].enabled) {
        renderCoordinateSystem();
    }

    if (m_viewports[VIEWPORT_NAVIGATION_CUBE].enabled && m_navigationCubeManager) {
        renderNavigationCube();
    }
}

void MultiViewportManager::renderNavigationCube() {
    if (m_navigationCubeManager) {
        // Render navigation cube first - it has its own positioning logic
        m_navigationCubeManager->render();
        
        // Note: The green border is temporarily disabled because NavigationCubeManager
        // has its own independent layout system that doesn't use MultiViewportManager's viewport.
        // The actual CuteNavCube position is controlled by NavigationCubeManager::m_cubeLayout,
        // not by m_viewports[VIEWPORT_NAVIGATION_CUBE].
        
        // TODO: Sync the two positioning systems if border visualization is needed
    }
}

void MultiViewportManager::renderCubeOutline() {

    if (!m_cubeOutlineRoot || !m_cubeOutlineCamera) {
        LOG_WRN_S("MultiViewportManager: Cube outline scene not initialized");
        return;
    }
    
    ViewportInfo& viewport = m_viewports[VIEWPORT_CUBE_OUTLINE];
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    wxSize canvasSize = m_canvas->GetClientSize();
    int yBottom = canvasSize.y - viewport.y - viewport.height;

    glEnable(GL_SCISSOR_TEST);
    glScissor(viewport.x, yBottom, viewport.width, viewport.height);
    if (!m_transparentBackground) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT);  
    }
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

    if (!m_coordinateSystemRoot || !m_coordinateSystemCamera) {
        LOG_WRN_S("MultiViewportManager: Coordinate system scene not initialized");
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
    if (!m_transparentBackground) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT);  
    }
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



void MultiViewportManager::handleSizeChange(const wxSize& canvasSize) {
    updateViewportLayouts(canvasSize);
}

void MultiViewportManager::updateViewportLayouts(const wxSize& canvasSize) {
    int margin = static_cast<int>(m_margin * m_dpiScale);
    
    // Navigation cube (top-right)
    int cubeSize = static_cast<int>(80 * m_dpiScale);
    m_viewports[VIEWPORT_NAVIGATION_CUBE] = ViewportInfo(
        canvasSize.x - cubeSize - margin,
        canvasSize.y - cubeSize - margin,
        cubeSize,
        cubeSize
    );
    
    int outlineSize = static_cast<int>(120 * m_dpiScale);
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
    wxSize canvasSize = m_canvas->GetClientSize();
    float x = event.GetX();
    float y_wx = event.GetY();
    float y = canvasSize.y - y_wx; 

    
    // Priority 1: Check navigation cube viewport first
    // Note: NavigationCubeManager uses wxWidgets coordinate system (top-left origin)
    if (m_navigationCubeManager) {
        bool handled = m_navigationCubeManager->handleMouseEvent(event);
        if (handled) {
            return true; // NavigationCubeManager handled the event
        } 
    } 

    // Priority 2: Check cube outline viewport only if mouse is NOT in navigation cube viewport
    if (m_viewports[VIEWPORT_CUBE_OUTLINE].enabled && m_cubeOutlineRoot) {
        ViewportInfo& outlineViewport = m_viewports[VIEWPORT_CUBE_OUTLINE];
        
        // Calculate the actual rendering position (convert to OpenGL coordinates)
        int actualYBottom = canvasSize.y - outlineViewport.y - outlineViewport.height;
        int actualYTop = actualYBottom + outlineViewport.height;
        
        
        bool xInRange = (x >= outlineViewport.x && x < (outlineViewport.x + outlineViewport.width));
        bool yInRange = (y >= actualYBottom && y < actualYTop);
        
        if (xInRange && yInRange) {

            // Convert gl coordinates to Open Inventor viewport-local coordinates (left-bottom origin)
            int localX = static_cast<int>(x) - outlineViewport.x;
            int localY = static_cast<int>(y) - actualYBottom;
            int pickY = localY;  // y is already in bottom-up coordinate system

            // Use viewport region matching the cube outline viewport only
            SbViewportRegion viewportRegion(outlineViewport.width, outlineViewport.height);
            SbVec2s pickPoint(localX, pickY);

            if (event.LeftDown()) {
                LOG_INF_S("Mouse left down in cube outline viewport at (" + std::to_string(x) + ", " + std::to_string(y) + ") [local: (" + std::to_string(localX) + ", " + std::to_string(localY) + ", pick: (" + std::to_string(pickPoint[0]) + ", " + std::to_string(pickPoint[1]) + ")]");

                SoRayPickAction pickAction(viewportRegion);
                pickAction.setPoint(pickPoint);
                pickAction.apply(m_cubeOutlineRoot);

                SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
                if (pickedPoint) {
                    SoPath* path = pickedPoint->getPath();
                    if (path) {
                        std::string clickedShape = findShapeNameFromPath(path);
                        LOG_INF_S(clickedShape + " clicked at position (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                    }

                }

                return true;
            }
            else if (event.LeftUp()) {
                return true;
            }
            else if (event.Moving()) {
                return true;
            }
            else {
                return true;
            }
        }
    }
    
    // Priority 3: Check coordinate system viewport if needed
    if (m_viewports[VIEWPORT_COORDINATE_SYSTEM].enabled) {
        ViewportInfo& coordViewport = m_viewports[VIEWPORT_COORDINATE_SYSTEM];
        if (x >= coordViewport.x && x < (coordViewport.x + coordViewport.width) &&
            y >= coordViewport.y && y < (coordViewport.y + coordViewport.height)) {
            // Handle coordinate system viewport events here if needed
            return true;
        }
    }
    
    // Mouse is not in any viewport
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



std::string MultiViewportManager::findShapeNameFromPath(SoPath* path) {
    if (!path) return "Unknown";
    
    // First, try to find a named separator in the path
    for (int i = 0; i < path->getLength(); i++) {
        SoNode* node = path->getNode(i);
        if (node && node->isOfType(SoSeparator::getClassTypeId())) {
            SbName name = node->getName();
            if (name != "") {
                return std::string(name.getString());
            }
        }
    }
    
    // If no named separator found, try to identify by composite shape
    for (const auto& compositeShape : m_compositeShapes) {
        // Check if any node in the path is the root node of this composite shape
        for (int i = 0; i < path->getLength(); i++) {
            SoNode* pathNode = path->getNode(i);
            if (pathNode == compositeShape.rootNode) {
                return compositeShape.shapeName;
            }
        }
    }
    
    // Fallback: try to identify by shape type
    for (int i = 0; i < path->getLength(); i++) {
        SoNode* node = path->getNode(i);
        if (node) {
            if (node->isOfType(SoSphere::getClassTypeId())) {
                return "Sphere";
            } else if (node->isOfType(SoCube::getClassTypeId())) {
                return "Cube";
            }
        }
    }
    
    // If we can't identify the shape, try to get more information
    LOG_DBG_S("Could not identify shape from path, path length: " + std::to_string(path->getLength()));
    for (int i = 0; i < path->getLength(); i++) {
        SoNode* node = path->getNode(i);
        if (node) {
            LOG_DBG_S("  Node " + std::to_string(i) + ": " + node->getTypeId().getName().getString());
        }
    }
    
    return "Unknown";
}
