#include "renderpreview/RenderPreviewCanvas.h"
#include "SceneManager.h"
#include "RenderingEngine.h"
#include "OCCViewer.h"
#include "GeometryFactory.h"
#include "logger/Logger.h"
#include <wx/dcclient.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>

BEGIN_EVENT_TABLE(RenderPreviewCanvas, Canvas)
EVT_PAINT(RenderPreviewCanvas::onPaint)
EVT_SIZE(RenderPreviewCanvas::onSize)
EVT_ERASE_BACKGROUND(RenderPreviewCanvas::onEraseBackground)
END_EVENT_TABLE()

RenderPreviewCanvas::RenderPreviewCanvas(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
    : Canvas(parent, id, pos, size)
{
    LOG_INF_S("RenderPreviewCanvas::RenderPreviewCanvas: Initializing");
    SetName("RenderPreviewCanvas");

    try {
        // Create default preview scene
        createDefaultPreviewScene();

        Refresh(true);
        Update();
        LOG_INF_S("RenderPreviewCanvas::RenderPreviewCanvas: Initialized successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderPreviewCanvas::RenderPreviewCanvas: Initialization failed: " + std::string(e.what()));
        throw;
    }
}

RenderPreviewCanvas::~RenderPreviewCanvas()
{
    LOG_INF_S("RenderPreviewCanvas::~RenderPreviewCanvas: Destroying");
}

void RenderPreviewCanvas::createDefaultPreviewScene()
{
    LOG_INF_S("RenderPreviewCanvas::createDefaultPreviewScene: Creating default preview scene");
    
    SceneManager* sceneManager = getSceneManager();
    if (!sceneManager) {
        LOG_ERR_S("RenderPreviewCanvas::createDefaultPreviewScene: SceneManager not available");
        return;
    }

    SoSeparator* objectRoot = sceneManager->getObjectRoot();
    if (!objectRoot) {
        LOG_ERR_S("RenderPreviewCanvas::createDefaultPreviewScene: Object root not available");
        return;
    }

    // Set light green background
    setLightGreenBackground();
    
    // Create checkerboard plane
    createCheckerboardPlane();
    
    // Create basic geometry objects
    createBasicGeometryObjects();
    
    // Set up camera for 46-degree view
    setupDefaultCamera();
    
    LOG_INF_S("RenderPreviewCanvas::createDefaultPreviewScene: Default scene created successfully");
}

void RenderPreviewCanvas::setLightGreenBackground()
{
    SceneManager* sceneManager = getSceneManager();
    if (!sceneManager) return;
    
    // Create a light green background material
    auto* backgroundMaterial = new SoMaterial;
    backgroundMaterial->diffuseColor.setValue(SbColor(0.8f, 0.9f, 0.8f)); // Light green
    backgroundMaterial->ambientColor.setValue(SbColor(0.6f, 0.7f, 0.6f));
    
    // Add to scene root
    SoSeparator* sceneRoot = sceneManager->getObjectRoot();
    if (sceneRoot) {
        sceneRoot->addChild(backgroundMaterial);
    }
}

void RenderPreviewCanvas::createCheckerboardPlane()
{
    SceneManager* sceneManager = getSceneManager();
    if (!sceneManager) return;
    
    SoSeparator* objectRoot = sceneManager->getObjectRoot();
    if (!objectRoot) return;
    
    auto* planeGroup = new SoSeparator;
    
    // Create checkerboard material
    auto* checkerMaterial = new SoMaterial;
    checkerMaterial->diffuseColor.setValue(SbColor(0.9f, 0.9f, 0.9f)); // Light gray
    checkerMaterial->ambientColor.setValue(SbColor(0.7f, 0.7f, 0.7f));
    planeGroup->addChild(checkerMaterial);
    
    // Create checkerboard plane using indexed face set
    auto* coordinates = new SoCoordinate3;
    const int gridSize = 10;
    const float cellSize = 2.0f;
    const float halfSize = (gridSize * cellSize) / 2.0f;
    
    // Generate vertices
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            float xPos = x * cellSize - halfSize;
            float zPos = z * cellSize - halfSize;
            coordinates->point.set1Value(z * (gridSize + 1) + x, SbVec3f(xPos, 0.0f, zPos));
        }
    }
    planeGroup->addChild(coordinates);
    
    // Create checkerboard pattern using indexed face set
    auto* faceSet = new SoIndexedFaceSet;
    auto* lineSet = new SoIndexedLineSet;
    
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int baseIndex = z * (gridSize + 1) + x;
            
            // Create face indices
            faceSet->coordIndex.set1Value(faceSet->coordIndex.getNum(), baseIndex);
            faceSet->coordIndex.set1Value(faceSet->coordIndex.getNum(), baseIndex + 1);
            faceSet->coordIndex.set1Value(faceSet->coordIndex.getNum(), baseIndex + gridSize + 2);
            faceSet->coordIndex.set1Value(faceSet->coordIndex.getNum(), baseIndex + gridSize + 1);
            faceSet->coordIndex.set1Value(faceSet->coordIndex.getNum(), -1); // End face
            
            // Create line indices for grid
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex + 1);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), -1);
            
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex + gridSize + 1);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), -1);
        }
    }
    
    planeGroup->addChild(faceSet);
    
    // Add grid lines
    auto* lineMaterial = new SoMaterial;
    lineMaterial->diffuseColor.setValue(SbColor(0.3f, 0.3f, 0.3f)); // Dark gray for grid lines
    planeGroup->addChild(lineMaterial);
    planeGroup->addChild(lineSet);
    
    objectRoot->addChild(planeGroup);
}

void RenderPreviewCanvas::createBasicGeometryObjects()
{
    SceneManager* sceneManager = getSceneManager();
    if (!sceneManager) return;
    
    SoSeparator* objectRoot = sceneManager->getObjectRoot();
    if (!objectRoot) return;
    
    // Create sphere
    auto* sphereGroup = new SoSeparator;
    auto* sphereTransform = new SoTransform;
    sphereTransform->translation.setValue(SbVec3f(-3.0f, 1.0f, 0.0f));
    sphereGroup->addChild(sphereTransform);
    
    auto* sphereMaterial = new SoMaterial;
    sphereMaterial->diffuseColor.setValue(SbColor(0.8f, 0.4f, 0.4f)); // Reddish
    sphereMaterial->ambientColor.setValue(SbColor(0.6f, 0.3f, 0.3f));
    sphereMaterial->specularColor.setValue(SbColor(1.0f, 1.0f, 1.0f));
    sphereMaterial->shininess.setValue(0.8f);
    sphereGroup->addChild(sphereMaterial);
    
    auto* sphere = new SoSphere;
    sphere->radius.setValue(1.0f);
    sphereGroup->addChild(sphere);
    
    objectRoot->addChild(sphereGroup);
    
    // Create cylinder
    auto* cylinderGroup = new SoSeparator;
    auto* cylinderTransform = new SoTransform;
    cylinderTransform->translation.setValue(SbVec3f(0.0f, 1.0f, 0.0f));
    cylinderGroup->addChild(cylinderTransform);
    
    auto* cylinderMaterial = new SoMaterial;
    cylinderMaterial->diffuseColor.setValue(SbColor(0.4f, 0.8f, 0.4f)); // Greenish
    cylinderMaterial->ambientColor.setValue(SbColor(0.3f, 0.6f, 0.3f));
    cylinderMaterial->specularColor.setValue(SbColor(1.0f, 1.0f, 1.0f));
    cylinderMaterial->shininess.setValue(0.6f);
    cylinderGroup->addChild(cylinderMaterial);
    
    auto* cylinder = new SoCylinder;
    cylinder->radius.setValue(0.8f);
    cylinder->height.setValue(2.0f);
    cylinderGroup->addChild(cylinder);
    
    objectRoot->addChild(cylinderGroup);
    
    // Create cube
    auto* cubeGroup = new SoSeparator;
    auto* cubeTransform = new SoTransform;
    cubeTransform->translation.setValue(SbVec3f(3.0f, 1.0f, 0.0f));
    cubeGroup->addChild(cubeTransform);
    
    auto* cubeMaterial = new SoMaterial;
    cubeMaterial->diffuseColor.setValue(SbColor(0.4f, 0.4f, 0.8f)); // Bluish
    cubeMaterial->ambientColor.setValue(SbColor(0.3f, 0.3f, 0.6f));
    cubeMaterial->specularColor.setValue(SbColor(1.0f, 1.0f, 1.0f));
    cubeMaterial->shininess.setValue(0.9f);
    cubeGroup->addChild(cubeMaterial);
    
    auto* cube = new SoCube;
    cube->width.setValue(1.5f);
    cube->height.setValue(1.5f);
    cube->depth.setValue(1.5f);
    cubeGroup->addChild(cube);
    
    objectRoot->addChild(cubeGroup);
}

void RenderPreviewCanvas::setupDefaultCamera()
{
    SceneManager* sceneManager = getSceneManager();
    if (!sceneManager) return;
    
    SoCamera* camera = sceneManager->getCamera();
    if (!camera) return;
    
    // Set up camera for 46-degree isometric view
    camera->position.setValue(SbVec3f(8.0f, 6.0f, 8.0f));
    camera->pointAt(SbVec3f(0.0f, 0.0f, 0.0f));
    camera->focalDistance.setValue(10.0f);
    
    // Set perspective camera
    if (camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        SoPerspectiveCamera* perspCamera = static_cast<SoPerspectiveCamera*>(camera);
        perspCamera->heightAngle.setValue(0.785398f); // 45 degrees
    }
}

void RenderPreviewCanvas::render(bool fastMode)
{
    Canvas::render(fastMode);
}

void RenderPreviewCanvas::resetView()
{
    Canvas::resetView();
    setupDefaultCamera();
    render(false);
}

void RenderPreviewCanvas::onPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    render(false);
    event.Skip();
}

void RenderPreviewCanvas::onSize(wxSizeEvent& event)
{
    // Let parent handle resize
    event.Skip();
}

void RenderPreviewCanvas::onEraseBackground(wxEraseEvent& event)
{
    // Do nothing to prevent flickering
}
