#include "renderpreview/PreviewCanvas.h"
#include "logger/Logger.h"
#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/GeometryProcessor.h"
#include "config/RenderingConfig.h"
#include <wx/dcclient.h>
#include <memory>
#include <cmath>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>

const int PreviewCanvas::s_canvasAttribs[] = {
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_DEPTH_SIZE, 24,
    WX_GL_STENCIL_SIZE, 8,
    0 // Terminator
};

BEGIN_EVENT_TABLE(PreviewCanvas, wxGLCanvas)
EVT_PAINT(PreviewCanvas::onPaint)
EVT_SIZE(PreviewCanvas::onSize)
EVT_ERASE_BACKGROUND(PreviewCanvas::onEraseBackground)
EVT_LEFT_DOWN(PreviewCanvas::onMouseDown)
EVT_RIGHT_DOWN(PreviewCanvas::onMouseDown)
EVT_LEFT_UP(PreviewCanvas::onMouseUp)
EVT_RIGHT_UP(PreviewCanvas::onMouseUp)
EVT_MOTION(PreviewCanvas::onMouseMove)
EVT_MOUSEWHEEL(PreviewCanvas::onMouseWheel)
END_EVENT_TABLE()

PreviewCanvas::PreviewCanvas(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, s_canvasAttribs, pos, size, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS | wxBORDER_NONE)
    , m_sceneRoot(nullptr)
    , m_camera(nullptr)
    , m_light(nullptr)
    , m_objectRoot(nullptr)
    , m_lightMaterial(nullptr)
    , m_lightIndicator(nullptr)
    , m_glContext(nullptr)
    , m_initialized(false)
    , m_mouseDown(false)
    , m_lastMousePos(0, 0)
    , m_cameraDistance(15.0f)
    , m_cameraCenter(0.0f, 0.0f, 0.0f)
{
    LOG_INF_S("PreviewCanvas::PreviewCanvas: Initializing");
    SetName("PreviewCanvas");

    try {
        // Initialize Coin3D
        SoDB::init();
        SoInteraction::init();
        
        // Create OpenGL context
        m_glContext = new wxGLContext(this);
        
        // Initialize scene
        initializeScene();
        
        m_initialized = true;
        
        // Initial render
        render(false);
        
        Refresh(true);
        Update();
        LOG_INF_S("PreviewCanvas::PreviewCanvas: Initialized successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("PreviewCanvas::PreviewCanvas: Initialization failed: " + std::string(e.what()));
        throw;
    }
}

PreviewCanvas::~PreviewCanvas()
{
    LOG_INF_S("PreviewCanvas::~PreviewCanvas: Destroying");
    if (m_glContext) {
        delete m_glContext;
    }
}

void PreviewCanvas::initializeScene()
{
    LOG_INF_S("PreviewCanvas::initializeScene: Creating scene graph");
    
    // Create scene root
    m_sceneRoot = new SoSeparator;
    m_sceneRoot->ref();
    
    // Create camera first
    m_camera = new SoPerspectiveCamera;
    m_camera->ref();
    m_camera->position.setValue(8.0f, -8.0f, 8.0f);
    m_camera->nearDistance.setValue(0.001f);
    m_camera->farDistance.setValue(10000.0f);
    m_camera->focalDistance.setValue(13.86f);
    
    // Set camera orientation for 46-degree view
    SbVec3f viewDir(-8.0f, 8.0f, -8.0f);
    viewDir.normalize();
    SbVec3f defaultDir(0, 0, -1);
    SbRotation rotation(defaultDir, viewDir);
    m_camera->orientation.setValue(rotation);
    m_sceneRoot->addChild(m_camera);
    
    // Create lighting
    setupLighting();
    
    // Create object root
    m_objectRoot = new SoSeparator;
    m_objectRoot->ref();
    m_sceneRoot->addChild(m_objectRoot);
    
    // Create default scene
    createDefaultScene();
    
    LOG_INF_S("PreviewCanvas::initializeScene: Scene graph created successfully");
}

void PreviewCanvas::setupLighting()
{
    LOG_INF_S("PreviewCanvas::setupLighting: Setting up FreeCAD-style three-point lighting");
    
    // Set a light model to enable proper lighting calculation
    SoLightModel* lightModel = new SoLightModel;
    lightModel->model.setValue(SoLightModel::PHONG);
    m_sceneRoot->addChild(lightModel);
    
    // Create main directional light (top 45-degree light)
    m_light = new SoDirectionalLight;
    m_light->ref();
    m_light->direction.setValue(SbVec3f(0.0f, -0.707f, -0.707f)); // Light from top 45 degrees pointing down (Y=-0.707, Z=-0.707)
    m_light->intensity.setValue(1.0f);
    m_light->color.setValue(SbColor(1.0f, 1.0f, 1.0f)); // White light
    m_sceneRoot->addChild(m_light);
    
    // Create left fill light
    auto* leftLight = new SoDirectionalLight;
    leftLight->ref();
    leftLight->direction.setValue(SbVec3f(-1.0f, 0.0f, 0.0f)); // Light from left
    leftLight->intensity.setValue(0.6f);
    leftLight->color.setValue(SbColor(1.0f, 1.0f, 1.0f)); // White light
    m_sceneRoot->addChild(leftLight);
    
    // Create top rim light with increased intensity
    auto* topLight = new SoDirectionalLight;
    topLight->ref();
    topLight->direction.setValue(SbVec3f(0.0f, 1.0f, 0.0f)); // Light from top
    topLight->intensity.setValue(0.8f); // Increased from 0.4f to 0.8f
    topLight->color.setValue(SbColor(1.0f, 1.0f, 1.0f)); // White light
    m_sceneRoot->addChild(topLight);
    
    // Create light material for controlling light properties
    m_lightMaterial = new SoMaterial;
    m_lightMaterial->ref();
    m_lightMaterial->ambientColor.setValue(SbColor(0.4f, 0.4f, 0.4f)); // Increased ambient for better top illumination
    m_lightMaterial->diffuseColor.setValue(SbColor(0.8f, 0.8f, 0.8f));
    m_lightMaterial->specularColor.setValue(SbColor(0.5f, 0.5f, 0.5f));
    m_sceneRoot->addChild(m_lightMaterial);
    
    LOG_INF_S("PreviewCanvas::setupLighting: FreeCAD-style three-point lighting setup completed");
}

void PreviewCanvas::createDefaultScene()
{
    LOG_INF_S("PreviewCanvas::createDefaultScene: Creating default preview scene");
    
    // Create OCCGeometry objects for basic shapes
    createBasicGeometryObjects();
    LOG_INF_S("PreviewCanvas::createDefaultScene: OCCGeometry objects created");
    
    // Create checkerboard plane
    createCheckerboardPlane();
    LOG_INF_S("PreviewCanvas::createDefaultScene: Checkerboard plane created");
    
    // Create light indicator
    createLightIndicator();
    LOG_INF_S("PreviewCanvas::createDefaultScene: Light indicator created");
    
    // Create coordinate system
    createCoordinateSystem();
    LOG_INF_S("PreviewCanvas::createDefaultScene: Coordinate system created");
    
    // Create basic geometry objects
    createBasicGeometryObjects();
    LOG_INF_S("PreviewCanvas::createDefaultScene: Basic geometry objects created");
    
    // Set up camera for 46-degree view
    setupDefaultCamera();
    LOG_INF_S("PreviewCanvas::createDefaultScene: Camera setup completed");
    
    LOG_INF_S("PreviewCanvas::createDefaultScene: Default scene created successfully");
}

void PreviewCanvas::createCheckerboardPlane()
{
    auto* planeGroup = new SoSeparator;
    
    // Create checkerboard plane using indexed face set
    auto* coordinates = new SoCoordinate3;
    const int gridSize = 8; // 8x8 grid like chess board
    const float cellSize = 2.5f; // Cell size
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
    
    // Create checkerboard pattern with alternating colors
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            // Create separate face set for each cell to have different materials
            auto* cellGroup = new SoSeparator;
            
            // Determine if this cell should be light or dark (chess pattern)
            bool isLightCell = ((x + z) % 2) == 0;
            
            // Create material for this cell
            auto* cellMaterial = new SoMaterial;
            if (isLightCell) {
                // Light cell - light silver gray
                cellMaterial->diffuseColor.setValue(SbColor(0.9f, 0.9f, 0.92f));
                cellMaterial->ambientColor.setValue(SbColor(0.7f, 0.7f, 0.72f));
            } else {
                // Dark cell - darker gray
                cellMaterial->diffuseColor.setValue(SbColor(0.6f, 0.6f, 0.65f));
                cellMaterial->ambientColor.setValue(SbColor(0.4f, 0.4f, 0.45f));
            }
            cellMaterial->transparency.setValue(0.6f); // 60% transparency
            cellGroup->addChild(cellMaterial);
            
            // Create face set for this cell
    auto* faceSet = new SoIndexedFaceSet;
            int baseIndex = z * (gridSize + 1) + x;
            
            // Create face indices for this cell
            faceSet->coordIndex.set1Value(0, baseIndex);
            faceSet->coordIndex.set1Value(1, baseIndex + 1);
            faceSet->coordIndex.set1Value(2, baseIndex + gridSize + 2);
            faceSet->coordIndex.set1Value(3, baseIndex + gridSize + 1);
            faceSet->coordIndex.set1Value(4, -1); // End face
            
            cellGroup->addChild(faceSet);
            planeGroup->addChild(cellGroup);
        }
    }
    
    // Add grid lines
    auto* lineSet = new SoIndexedLineSet;
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int baseIndex = z * (gridSize + 1) + x;
            
            // Create line indices for grid
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex + 1);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), -1);
            
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex + gridSize + 1);
            lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), -1);
        }
    }
    
    // Add grid lines material
    auto* lineMaterial = new SoMaterial;
    lineMaterial->diffuseColor.setValue(SbColor(0.3f, 0.3f, 0.3f)); // Dark gray for grid lines
    planeGroup->addChild(lineMaterial);
    planeGroup->addChild(lineSet);
    
    m_objectRoot->addChild(planeGroup);
}



void PreviewCanvas::createBasicGeometryObjects()
{
    LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: Creating OCCGeometry objects");

    const double R = 2.5; // Radius of the arrangement
    const double h_sphere = 1.2; // y position for sphere to sit on plane
    const double h_cone = 1.25; // y position for cone to sit on plane
    const double h_box = 1.0; // y position for box to sit on plane

    gp_Pnt pos_sphere(0.0, h_sphere, R);
    gp_Pnt pos_cone(-R * sqrt(3.0) / 2.0, h_cone, -R / 2.0);
    gp_Pnt pos_box(R * sqrt(3.0) / 2.0, h_box, -R / 2.0);

    m_occSphere = std::make_unique<OCCSphere>("PreviewSphere", 1.2);
    m_occSphere->setPosition(pos_sphere);
    m_occSphere->setColor(Quantity_Color(1.0, 0.3, 0.3, Quantity_TOC_RGB)); // Bright red
    m_occSphere->setMaterialDiffuseColor(Quantity_Color(1.0, 0.3, 0.3, Quantity_TOC_RGB));
    m_occSphere->setMaterialAmbientColor(Quantity_Color(0.6, 0.2, 0.2, Quantity_TOC_RGB));
    m_occSphere->setMaterialSpecularColor(Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB));
    m_occSphere->setMaterialShininess(0.8);

    m_occCone = std::make_unique<OCCCone>("PreviewCone", 1.0, 0.0, 2.5); // bottomRadius, topRadius=0, height
    m_occCone->setPosition(pos_cone);
    m_occCone->setRotation(gp_Vec(1.0, 0.0, 0.0), -M_PI / 2.0); // Pointing up
    m_occCone->setColor(Quantity_Color(0.3, 1.0, 0.3, Quantity_TOC_RGB)); // Bright green
    m_occCone->setMaterialDiffuseColor(Quantity_Color(0.3, 1.0, 0.3, Quantity_TOC_RGB));
    m_occCone->setMaterialAmbientColor(Quantity_Color(0.2, 0.6, 0.2, Quantity_TOC_RGB));
    m_occCone->setMaterialSpecularColor(Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB));
    m_occCone->setMaterialShininess(0.6);

    m_occBox = std::make_unique<OCCBox>("PreviewBox", 2.0, 2.0, 2.0);
    m_occBox->setPosition(pos_box);
    m_occBox->setColor(Quantity_Color(0.3, 0.3, 1.0, Quantity_TOC_RGB)); // Bright blue
    m_occBox->setMaterialDiffuseColor(Quantity_Color(0.3, 0.3, 1.0, Quantity_TOC_RGB));
    m_occBox->setMaterialAmbientColor(Quantity_Color(0.2, 0.2, 0.6, Quantity_TOC_RGB));
    m_occBox->setMaterialSpecularColor(Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB));
    m_occBox->setMaterialShininess(0.7);

    // Build Coin3D representations for all OCCGeometry objects
    MeshParameters meshParams;
    meshParams.deflection = 0.01; // High quality mesh

    m_occSphere->buildCoinRepresentation(meshParams,
        Quantity_Color(1.0, 0.3, 0.3, Quantity_TOC_RGB),  // diffuse
        Quantity_Color(0.6, 0.2, 0.2, Quantity_TOC_RGB),  // ambient
        Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),  // specular
        Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),  // emissive
        0.8, 0.0); // shininess, transparency

    m_occCone->buildCoinRepresentation(meshParams,
        Quantity_Color(0.3, 1.0, 0.3, Quantity_TOC_RGB),  // diffuse
        Quantity_Color(0.2, 0.6, 0.2, Quantity_TOC_RGB),  // ambient
        Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),  // specular
        Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),  // emissive
        0.6, 0.0); // shininess, transparency

    m_occBox->buildCoinRepresentation(meshParams,
        Quantity_Color(0.3, 0.3, 1.0, Quantity_TOC_RGB),  // diffuse
        Quantity_Color(0.2, 0.2, 0.6, Quantity_TOC_RGB),  // ambient
        Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),  // specular
        Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),  // emissive
        0.7, 0.0); // shininess, transparency

    // Add Coin3D nodes to scene
    if (m_occSphere->getCoinNode()) {
        m_objectRoot->addChild(m_occSphere->getCoinNode());
    }
    if (m_occCone->getCoinNode()) {
        m_objectRoot->addChild(m_occCone->getCoinNode());
    }
    if (m_occBox->getCoinNode()) {
        m_objectRoot->addChild(m_occBox->getCoinNode());
    }

    LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: OCCGeometry objects created successfully");
}

void PreviewCanvas::createLightIndicator()
{
    LOG_INF_S("PreviewCanvas::createLightIndicator: Creating visual indicators for all lights");

    m_lightIndicator = new SoSeparator;
    m_lightIndicator->ref();

    // Helper lambda to create a single light indicator
    auto createLightIndicator = [](SoDirectionalLight* light, int lightIndex) -> SoSeparator* {
        if (!light) return nullptr;

        SoSeparator* indicatorSep = new SoSeparator;

        // Get light properties
        SbVec3f lightDir = light->direction.getValue();
        SbColor lightColor = light->color.getValue();
        float lightIntensity = light->intensity.getValue();

        // Position the indicator based on light direction
        SoTransform* positionTransform = new SoTransform;
        // Position indicator at a visible distance from origin
        SbVec3f indicatorPos = lightDir * -8.0f; // 8 units away from origin
        positionTransform->translation.setValue(indicatorPos);
        indicatorSep->addChild(positionTransform);

        // Create material for the light source sphere
        SoMaterial* sphereMaterial = new SoMaterial;
        sphereMaterial->diffuseColor.setValue(lightColor);
        sphereMaterial->ambientColor.setValue(lightColor * 0.3f);
        sphereMaterial->emissiveColor.setValue(lightColor * 0.6f); // Make it glow
        sphereMaterial->transparency.setValue(0.2f); // Slight transparency
        indicatorSep->addChild(sphereMaterial);

        // Create light source sphere (size based on intensity)
        SoSphere* lightSphere = new SoSphere;
        lightSphere->radius.setValue(0.3f + lightIntensity * 0.2f); // Size varies with intensity
        indicatorSep->addChild(lightSphere);

        // Create arrow to show light direction
        SoSeparator* arrowGroup = new SoSeparator;

        // Arrow material (same color as light but more opaque)
        SoMaterial* arrowMaterial = new SoMaterial;
        arrowMaterial->diffuseColor.setValue(lightColor);
        arrowMaterial->ambientColor.setValue(lightColor * 0.5f);
        arrowMaterial->emissiveColor.setValue(lightColor * 0.3f);
        arrowGroup->addChild(arrowMaterial);

        // Position arrow to point towards origin
        SoTransform* arrowTransform = new SoTransform;
        arrowTransform->translation.setValue(0.0f, 0.0f, 0.0f);
        
        // Calculate rotation to point arrow towards origin
        SbVec3f arrowDir = -lightDir; // Point towards origin
        SbVec3f defaultDir(0.0f, 0.0f, -1.0f); // Default arrow direction
        SbRotation rotation(defaultDir, arrowDir);
        arrowTransform->rotation.setValue(rotation);
        arrowGroup->addChild(arrowTransform);

        // Arrow shaft (cylinder)
        SoCylinder* arrowShaft = new SoCylinder;
        arrowShaft->radius.setValue(0.05f);
        arrowShaft->height.setValue(1.5f + lightIntensity * 1.0f); // Length varies with intensity
        arrowGroup->addChild(arrowShaft);

        // Arrow head (cone)
        SoCone* arrowHead = new SoCone;
        arrowHead->bottomRadius.setValue(0.12f);
        arrowHead->height.setValue(0.3f);
        
        SoTransform* headTransform = new SoTransform;
        headTransform->translation.setValue(0.0f, 0.0f, 0.75f + lightIntensity * 0.5f); // Position at end of shaft
        arrowGroup->addChild(headTransform);
        arrowGroup->addChild(arrowHead);

        indicatorSep->addChild(arrowGroup);

        // Add intensity text indicator (optional - could be implemented with SoText2)
        // For now, we'll use the sphere size and arrow length to indicate intensity

        return indicatorSep;
    };

    // Create indicators for each light
    // Main light (front)
    if (m_light) {
        SoSeparator* mainIndicator = createLightIndicator(m_light, 0);
        if (mainIndicator) {
            m_lightIndicator->addChild(mainIndicator);
        }
    }

    // Find and create indicators for other lights
    SoSearchAction searchAction;
    searchAction.setType(SoDirectionalLight::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_sceneRoot);

    int lightIndex = 1;
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (path && path->getTail()->isOfType(SoDirectionalLight::getClassTypeId())) {
            SoDirectionalLight* light = static_cast<SoDirectionalLight*>(path->getTail());
            // Skip the main light as it's already handled
            if (light != m_light) {
                SoSeparator* indicator = createLightIndicator(light, lightIndex++);
                if (indicator) {
                    m_lightIndicator->addChild(indicator);
                }
            }
        }
    }

    m_objectRoot->addChild(m_lightIndicator);
    LOG_INF_S("PreviewCanvas::createLightIndicator: Light indicators created successfully");
}

void PreviewCanvas::updateLightIndicator(const wxColour& color, float intensity)
{
    LOG_INF_S("PreviewCanvas::updateLightIndicator: Updating all light indicators");

    // The simplest and most robust way to reflect changes is to rebuild the indicators.
    if (m_lightIndicator) {
        m_objectRoot->removeChild(m_lightIndicator);
        // The node was ref'd, so it won't be deleted until unref'd.
        // Since we are replacing it, let's unref the old one.
        m_lightIndicator->unref();
        m_lightIndicator = nullptr;
    }

    // Re-create all indicators based on the current state of the lights in the scene graph.
    createLightIndicator();

    LOG_INF_S("PreviewCanvas::updateLightIndicator: Light indicators updated.");
}

void PreviewCanvas::createCoordinateSystem()
{
    LOG_INF_S("PreviewCanvas::createCoordinateSystem: Creating coordinate system");
    
    auto* coordGroup = new SoSeparator;
    
    // Create coordinate axes (X, Y, Z)
    // X-axis (Red)
    auto* xAxisGroup = new SoSeparator;
    auto* xMaterial = new SoMaterial;
    xMaterial->diffuseColor.setValue(SbColor(1.0f, 0.0f, 0.0f)); // Red
    xMaterial->emissiveColor.setValue(SbColor(0.3f, 0.0f, 0.0f)); // Glow effect
    xAxisGroup->addChild(xMaterial);
    
    auto* xCylinder = new SoCylinder;
    xCylinder->radius.setValue(0.05f);
    xCylinder->height.setValue(4.0f);
    xAxisGroup->addChild(xCylinder);
    
    // X-axis cone (arrow head)
    auto* xCone = new SoCone;
    xCone->bottomRadius.setValue(0.15f);
    xCone->height.setValue(0.3f);
    auto* xConeTransform = new SoTransform;
    xConeTransform->translation.setValue(SbVec3f(2.0f, 0.0f, 0.0f));
    xAxisGroup->addChild(xConeTransform);
    xAxisGroup->addChild(xCone);
    
    coordGroup->addChild(xAxisGroup);
    
    // Y-axis (Green)
    auto* yAxisGroup = new SoSeparator;
    auto* yMaterial = new SoMaterial;
    yMaterial->diffuseColor.setValue(SbColor(0.0f, 1.0f, 0.0f)); // Green
    yMaterial->emissiveColor.setValue(SbColor(0.0f, 0.3f, 0.0f)); // Glow effect
    yAxisGroup->addChild(yMaterial);
    
    auto* yCylinder = new SoCylinder;
    yCylinder->radius.setValue(0.05f);
    yCylinder->height.setValue(4.0f);
    auto* yTransform = new SoTransform;
    yTransform->rotation.setValue(SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), M_PI / 2.0f)); // Rotate 90 degrees around Z
    yAxisGroup->addChild(yTransform);
    yAxisGroup->addChild(yCylinder);
    
    // Y-axis cone (arrow head)
    auto* yCone = new SoCone;
    yCone->bottomRadius.setValue(0.15f);
    yCone->height.setValue(0.3f);
    auto* yConeTransform = new SoTransform;
    yConeTransform->translation.setValue(SbVec3f(0.0f, 2.0f, 0.0f));
    yConeTransform->rotation.setValue(SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), M_PI / 2.0f));
    yAxisGroup->addChild(yConeTransform);
    yAxisGroup->addChild(yCone);
    
    coordGroup->addChild(yAxisGroup);
    
    // Z-axis (Blue)
    auto* zAxisGroup = new SoSeparator;
    auto* zMaterial = new SoMaterial;
    zMaterial->diffuseColor.setValue(SbColor(0.0f, 0.0f, 1.0f)); // Blue
    zMaterial->emissiveColor.setValue(SbColor(0.0f, 0.0f, 0.3f)); // Glow effect
    zAxisGroup->addChild(zMaterial);
    
    auto* zCylinder = new SoCylinder;
    zCylinder->radius.setValue(0.05f);
    zCylinder->height.setValue(4.0f);
    auto* zTransform = new SoTransform;
    zTransform->rotation.setValue(SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), -M_PI / 2.0f)); // Rotate -90 degrees around X
    zAxisGroup->addChild(zTransform);
    zAxisGroup->addChild(zCylinder);
    
    // Z-axis cone (arrow head)
    auto* zCone = new SoCone;
    zCone->bottomRadius.setValue(0.15f);
    zCone->height.setValue(0.3f);
    auto* zConeTransform = new SoTransform;
    zConeTransform->translation.setValue(SbVec3f(0.0f, 0.0f, 2.0f));
    zConeTransform->rotation.setValue(SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), -M_PI / 2.0f));
    zAxisGroup->addChild(zConeTransform);
    zAxisGroup->addChild(zCone);
    
    coordGroup->addChild(zAxisGroup);
    
    // Add coordinate system to scene
    m_objectRoot->addChild(coordGroup);
    
    LOG_INF_S("PreviewCanvas::createCoordinateSystem: Coordinate system created successfully");
}

void PreviewCanvas::setupDefaultCamera()
{
    // Set up camera for 46-degree isometric view - adjusted to see all models
    m_camera->position.setValue(SbVec3f(12.0f, 8.0f, 12.0f));
    m_camera->pointAt(SbVec3f(0.0f, 0.0f, 0.0f));
    m_camera->focalDistance.setValue(15.0f);
    
    // Set perspective camera
    SoPerspectiveCamera* perspCamera = static_cast<SoPerspectiveCamera*>(m_camera);
    perspCamera->heightAngle.setValue(0.785398f); // 45 degrees
}

void PreviewCanvas::render(bool fastMode)
{
    if (!m_initialized || !m_glContext) {
        LOG_ERR_S("PreviewCanvas::render: Not initialized or no GL context");
        return;
    }
    
    if (!m_sceneRoot) {
        LOG_ERR_S("PreviewCanvas::render: No scene root");
        return;
    }
    
    SetCurrent(*m_glContext);
    
    // Set viewport
    wxSize size = GetClientSize();
    if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
        LOG_ERR_S("PreviewCanvas::render: Invalid viewport size");
        return;
    }
    
    // Update camera aspect ratio
    if (m_camera) {
        m_camera->aspectRatio.setValue(static_cast<float>(size.GetWidth()) / static_cast<float>(size.GetHeight()));
    }
    
    // Create viewport region
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    
    // Create render action with proper settings
    SoGLRenderAction renderAction(viewport);
    renderAction.setSmoothing(!fastMode);
    renderAction.setNumPasses(fastMode ? 1 : 2);
    renderAction.setTransparencyType(
        fastMode ? SoGLRenderAction::BLEND : SoGLRenderAction::SORTED_OBJECT_BLEND
    );
    
    // Set up OpenGL state like SceneManager does
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    
    // Set light blue background as per requirements
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // Light blue background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Reset OpenGL errors before rendering
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERR_S("Pre-render: OpenGL error: " + std::to_string(err));
    }
    
    // Reset OpenGL state to prevent errors
    glDisable(GL_TEXTURE_2D);
    
    // Render the scene
    renderAction.apply(m_sceneRoot);
    
    // Check for OpenGL errors after rendering
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERR_S("Post-render: OpenGL error: " + std::to_string(err));
    }
    
    SwapBuffers();
    
    LOG_INF_S("PreviewCanvas::render: Rendered successfully");
}

void PreviewCanvas::resetView()
{
    setupDefaultCamera();
    render(false);
}

void PreviewCanvas::updateLighting(float ambient, float diffuse, float specular, const wxColour& color, float intensity)
{
    LOG_INF_S("PreviewCanvas::updateLighting: Updating lighting properties");
    
    if (!m_lightMaterial) return;
    
    // Update light material
    float r = color.Red() / 255.0f;
    float g = color.Green() / 255.0f;
    float b = color.Blue() / 255.0f;
    
    m_lightMaterial->ambientColor.setValue(SbColor(r * ambient, g * ambient, b * ambient));
    m_lightMaterial->diffuseColor.setValue(SbColor(r * diffuse, g * diffuse, b * diffuse));
    m_lightMaterial->specularColor.setValue(SbColor(r * specular, g * specular, b * specular));
    
    // Update all directional lights in the scene
    SoSearchAction searchAction;
    searchAction.setType(SoDirectionalLight::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_sceneRoot);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (path && path->getTail()->isOfType(SoDirectionalLight::getClassTypeId())) {
            SoDirectionalLight* light = static_cast<SoDirectionalLight*>(path->getTail());
            
            // Update light color
            light->color.setValue(SbColor(r, g, b));
            
            // Update light intensity (scale based on original intensity)
            float originalIntensity = light->intensity.getValue();
            float scaledIntensity = originalIntensity * intensity;
            light->intensity.setValue(scaledIntensity);
        }
    }
    
    // Update light indicator
    updateLightIndicator(color, intensity);
    
    render(true);
}

void PreviewCanvas::updateMaterial(float ambient, float diffuse, float specular, float shininess, float transparency)
{
    LOG_INF_S("PreviewCanvas::updateMaterial: Updating material properties");
    
    // Update material properties for all geometry objects
    if (m_occSphere && m_occSphere->getCoinNode()) {
        updateObjectMaterial(m_occSphere->getCoinNode(), ambient, diffuse, specular, shininess, transparency);
    }
    
    if (m_occCone && m_occCone->getCoinNode()) {
        updateObjectMaterial(m_occCone->getCoinNode(), ambient, diffuse, specular, shininess, transparency);
    }
    
    if (m_occBox && m_occBox->getCoinNode()) {
        updateObjectMaterial(m_occBox->getCoinNode(), ambient, diffuse, specular, shininess, transparency);
    }
    
    render(true);
}

void PreviewCanvas::updateObjectMaterial(SoNode* node, float ambient, float diffuse, float specular, float shininess, float transparency)
{
    if (!node) return;
    
    // Use SoSearchAction to find material nodes in the object's scene graph
    SoSearchAction searchAction;
    searchAction.setType(SoMaterial::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(node);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (path && path->getTail()->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(path->getTail());
            
            // Update material properties directly
            // Use base colors for each object (red, green, blue)
            SbColor baseColor(0.8f, 0.8f, 0.8f); // Default gray color
            
            material->ambientColor.setValue(SbColor(baseColor[0] * ambient, baseColor[1] * ambient, baseColor[2] * ambient));
            material->diffuseColor.setValue(SbColor(baseColor[0] * diffuse, baseColor[1] * diffuse, baseColor[2] * diffuse));
            material->specularColor.setValue(SbColor(specular, specular, specular));
            material->shininess.setValue(shininess);
            material->transparency.setValue(transparency);
        }
    }
}

void PreviewCanvas::updateTexture(bool enabled, int mode, float scale)
{
    LOG_INF_S("PreviewCanvas::updateTexture: Updating texture settings");
    
    // For now, we'll just trigger a re-render
    // In a full implementation, this would apply textures to the geometry objects
    render(true);
}

void PreviewCanvas::updateAntiAliasing(int method, int msaaSamples, bool fxaaEnabled)
{
    LOG_INF_S("PreviewCanvas::updateAntiAliasing: Updating anti-aliasing settings");
    
    // For now, we'll just trigger a re-render
    // In a full implementation, this would configure MSAA and FXAA settings
    render(true);
}

void PreviewCanvas::onPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    render(false);
    event.Skip();
}

void PreviewCanvas::onSize(wxSizeEvent& event)
{
    // Handle resize
    event.Skip();
}

void PreviewCanvas::onEraseBackground(wxEraseEvent& event)
{
    // Do nothing to prevent flickering
}

void PreviewCanvas::onMouseDown(wxMouseEvent& event)
{
    m_mouseDown = true;
    m_lastMousePos = event.GetPosition();
    CaptureMouse();
    event.Skip();
}

void PreviewCanvas::onMouseUp(wxMouseEvent& event)
{
    m_mouseDown = false;
    if (HasCapture()) {
        ReleaseMouse();
    }
    event.Skip();
}

void PreviewCanvas::onMouseMove(wxMouseEvent& event)
{
    if (!m_mouseDown || !m_camera) {
        event.Skip();
        return;
    }

    wxPoint currentPos = event.GetPosition();
    int deltaX = currentPos.x - m_lastMousePos.x;
    
    // Only handle left mouse button for rotation
    if (event.LeftIsDown()) {
        // Calculate rotation angle based on mouse movement
        float rotationAngle = deltaX * 0.01f; // Sensitivity factor
        
        // Get current camera position
        SbVec3f cameraPos = m_camera->position.getValue();
        
        // Calculate vector from center to camera
        SbVec3f cameraVector = cameraPos - m_cameraCenter;
        
        // Create rotation around Z-axis
        SbRotation zRotation(SbVec3f(0.0f, 0.0f, 1.0f), rotationAngle);
        
        // Apply rotation to camera vector
        SbVec3f rotatedVector;
        zRotation.multVec(cameraVector, rotatedVector);
        
        // Set new camera position
        SbVec3f newPosition = m_cameraCenter + rotatedVector;
        m_camera->position.setValue(newPosition);
        
        // Make camera look at center
        m_camera->pointAt(m_cameraCenter);
        
        // Update last mouse position
        m_lastMousePos = currentPos;
        
        // Trigger re-render
        render(true);
    }
    
    event.Skip();
}

void PreviewCanvas::onMouseWheel(wxMouseEvent& event)
{
    if (!m_camera) {
        event.Skip();
        return;
    }
    
    // Handle zoom with mouse wheel
    int wheelRotation = event.GetWheelRotation();
    float zoomFactor = 1.0f;
    
    if (wheelRotation > 0) {
        zoomFactor = 1.1f; // Zoom in
    } else if (wheelRotation < 0) {
        zoomFactor = 0.9f; // Zoom out
    }
    
    // Get current camera position
    SbVec3f currentPos = m_camera->position.getValue();
    SbVec3f cameraVector = currentPos - m_cameraCenter;
    
    // Scale the camera distance
    cameraVector *= zoomFactor;
    m_cameraDistance = cameraVector.length();
    
    // Set new camera position
    SbVec3f newPosition = m_cameraCenter + cameraVector;
    m_camera->position.setValue(newPosition);
    
    // Make camera look at center
    m_camera->pointAt(m_cameraCenter);
    
    // Trigger re-render
    render(true);
    
    event.Skip();
} 