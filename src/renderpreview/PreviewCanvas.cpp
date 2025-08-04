#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/LightManager.h"
#include "logger/Logger.h"
#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/GeometryProcessor.h"
#include "config/RenderingConfig.h"
#include <wx/dcclient.h>
#include <memory>
#include <cmath>
#include <map>
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
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>

// LightManager implementation will be moved to a separate file

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
    , m_objectRoot(nullptr)
    , m_lightMaterial(nullptr)
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
        
        // Setup default lighting using LightManager
        setupDefaultLighting();
        
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
    
    // Create object root
    m_objectRoot = new SoSeparator;
    m_objectRoot->ref();
    m_sceneRoot->addChild(m_objectRoot);
    
    // Set light model to enable proper lighting calculation (must be before lights)
    SoLightModel* lightModel = new SoLightModel;
    lightModel->model.setValue(SoLightModel::PHONG);
    m_sceneRoot->addChild(lightModel);
    
    // Initialize managers
    m_objectManager = std::make_unique<ObjectManager>(m_sceneRoot, m_objectRoot);
    m_lightManager = std::make_unique<LightManager>(m_sceneRoot, m_objectRoot);
    m_antiAliasingManager = std::make_unique<AntiAliasingManager>(this, m_glContext);
    m_renderingManager = std::make_unique<RenderingManager>(m_sceneRoot, this, m_glContext);
    
    // Create default scene
    createDefaultScene();
    
    LOG_INF_S("PreviewCanvas::initializeScene: Scene graph created successfully");
}

void PreviewCanvas::setupDefaultLighting()
{
    LOG_INF_S("PreviewCanvas::setupDefaultLighting: Setting up default three-point lighting using LightManager");
    
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::setupDefaultLighting: Light manager not initialized");
        return;
    }
    
    // Light model is already set in initializeScene()
    
    // Create default three-point lighting system using LightManager
    std::vector<RenderLightSettings> defaultLights;
    
    // Main directional light (top 45-degree light)
    RenderLightSettings mainLight;
    mainLight.name = "Main Light";
    mainLight.type = "directional";
    mainLight.directionX = 0.0f;
    mainLight.directionY = -0.707f;
    mainLight.directionZ = -0.707f;
    mainLight.intensity = 1.5f; // Increased default intensity from 1.0f to 1.5f
    mainLight.color = wxColour(255, 255, 255); // White light
    mainLight.enabled = true;
    defaultLights.push_back(mainLight);
    
    // Left fill light
    RenderLightSettings leftLight;
    leftLight.name = "Fill Light";
    leftLight.type = "directional";
    leftLight.directionX = -1.0f;
    leftLight.directionY = 0.0f;
    leftLight.directionZ = 0.0f;
    leftLight.intensity = 0.6f;
    leftLight.color = wxColour(255, 255, 255); // White light
    leftLight.enabled = true;
    defaultLights.push_back(leftLight);
    
    // Top rim light
    RenderLightSettings topLight;
    topLight.name = "Rim Light";
    topLight.type = "directional";
    topLight.directionX = 0.0f;
    topLight.directionY = 1.0f;
    topLight.directionZ = 0.0f;
    topLight.intensity = 0.8f;
    topLight.color = wxColour(255, 255, 255); // White light
    topLight.enabled = true;
    defaultLights.push_back(topLight);
    
    // Add ambient light to ensure basic visibility
    RenderLightSettings ambientLight;
    ambientLight.name = "Ambient Light";
    ambientLight.type = "directional";
    ambientLight.directionX = 0.0f;
    ambientLight.directionY = 0.0f;
    ambientLight.directionZ = -1.0f;
    ambientLight.intensity = 0.3f; // Low intensity ambient light
    ambientLight.color = wxColour(255, 255, 255);
    ambientLight.enabled = true;
    defaultLights.push_back(ambientLight);
    
    // Apply default lighting using LightManager
    m_lightManager->updateMultipleLights(defaultLights);
    
    // Create default material for scene objects
    m_lightMaterial = new SoMaterial;
    m_lightMaterial->ref();
    m_lightMaterial->ambientColor.setValue(SbColor(0.4f, 0.4f, 0.4f));
    m_lightMaterial->diffuseColor.setValue(SbColor(0.8f, 0.8f, 0.8f));
    m_lightMaterial->specularColor.setValue(SbColor(0.5f, 0.5f, 0.5f));
    m_sceneRoot->addChild(m_lightMaterial);
    
    LOG_INF_S("PreviewCanvas::setupDefaultLighting: Default three-point lighting setup completed using LightManager");
    LOG_INF_S("PreviewCanvas::setupDefaultLighting: Light count: " + std::to_string(m_lightManager->getLightCount()));
    
    // Debug: Check scene structure
    LOG_INF_S("PreviewCanvas::setupDefaultLighting: Scene root children count: " + std::to_string(m_sceneRoot->getNumChildren()));
    for (int i = 0; i < m_sceneRoot->getNumChildren(); ++i) {
        SoNode* child = m_sceneRoot->getChild(i);
        if (child->isOfType(SoDirectionalLight::getClassTypeId())) {
            LOG_INF_S("PreviewCanvas::setupDefaultLighting: Found directional light at index " + std::to_string(i));
        } else if (child->isOfType(SoSeparator::getClassTypeId())) {
            LOG_INF_S("PreviewCanvas::setupDefaultLighting: Found separator at index " + std::to_string(i));
        }
    }
    
    // Debug: Check light container
    if (m_lightManager) {
        auto lightIds = m_lightManager->getAllLightIds();
        LOG_INF_S("PreviewCanvas::setupDefaultLighting: Light IDs: " + std::to_string(lightIds.size()));
        for (int lightId : lightIds) {
            auto settings = m_lightManager->getLightSettings(lightId);
            LOG_INF_S("PreviewCanvas::setupDefaultLighting: Light " + std::to_string(lightId) + 
                     " - " + settings.name + " - enabled: " + std::to_string(settings.enabled) + 
                     " - intensity: " + std::to_string(settings.intensity));
        }
    }
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
    
    // Light indicators are managed by LightManager
    LOG_INF_S("PreviewCanvas::createDefaultScene: Light indicators managed by LightManager");
    
    // Create coordinate system
    createCoordinateSystem();
    LOG_INF_S("PreviewCanvas::createDefaultScene: Coordinate system created");
    
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
            coordinates->point.set1Value(z * (gridSize + 1) + x, SbVec3f(xPos, -1.0f, zPos));
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
    
    // Clear any existing objects first
    if (m_objectManager) {
        auto objectIds = m_objectManager->getAllObjectIds();
        for (int objectId : objectIds) {
            m_objectManager->removeObject(objectId);
        }
        LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: Cleared " + std::to_string(objectIds.size()) + " existing objects");
    }

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

    // Note: OCC geometry nodes will be managed by ObjectManager
    // Do not add them directly to scene to avoid duplication

    // Add OCCGeometry objects to ObjectManager for property management
    LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: ObjectManager pointer: " + std::string(m_objectManager ? "valid" : "null"));
    LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: Current object count: " + std::to_string(m_objectManager ? m_objectManager->getAllObjectIds().size() : 0));
    if (m_objectManager) {
        // Sphere object settings
        ObjectSettings sphereSettings;
        sphereSettings.objectType = ObjectType::SPHERE;
        sphereSettings.name = "Red Sphere";
        sphereSettings.position = SbVec3f(0.0f, 1.2f, 2.5f);
        sphereSettings.materialColor = wxColour(255, 77, 77); // Bright red
        sphereSettings.ambient = 0.6f;
        sphereSettings.diffuse = 1.0f;
        sphereSettings.specular = 1.0f;
        sphereSettings.shininess = 102.4f; // 0.8 * 128
        sphereSettings.transparency = 0.0f;
        int sphereId = m_objectManager->addObject(sphereSettings);
        m_objectManager->associateOCCGeometry(sphereId, m_occSphere.get());
        LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: Added sphere to ObjectManager with ID " + std::to_string(sphereId));

        // Cone object settings
        ObjectSettings coneSettings;
        coneSettings.objectType = ObjectType::CONE;
        coneSettings.name = "Green Cone";
        coneSettings.position = SbVec3f(-2.165f, 1.25f, -1.25f);
        coneSettings.materialColor = wxColour(77, 255, 77); // Bright green
        coneSettings.ambient = 0.2f;
        coneSettings.diffuse = 1.0f;
        coneSettings.specular = 1.0f;
        coneSettings.shininess = 76.8f; // 0.6 * 128
        coneSettings.transparency = 0.0f;
        int coneId = m_objectManager->addObject(coneSettings);
        m_objectManager->associateOCCGeometry(coneId, m_occCone.get());
        LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: Added cone to ObjectManager with ID " + std::to_string(coneId));

        // Box object settings
        ObjectSettings boxSettings;
        boxSettings.objectType = ObjectType::BOX;
        boxSettings.name = "Blue Box";
        boxSettings.position = SbVec3f(2.165f, 1.0f, -1.25f);
        boxSettings.materialColor = wxColour(77, 77, 255); // Bright blue
        boxSettings.ambient = 0.2f;
        boxSettings.diffuse = 0.3f;
        boxSettings.specular = 1.0f;
        boxSettings.shininess = 89.6f; // 0.7 * 128
        boxSettings.transparency = 0.0f;
        int boxId = m_objectManager->addObject(boxSettings);
        m_objectManager->associateOCCGeometry(boxId, m_occBox.get());
        LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: Added box to ObjectManager with ID " + std::to_string(boxId));
    }

    LOG_INF_S("PreviewCanvas::createBasicGeometryObjects: OCCGeometry objects created successfully");
}











void PreviewCanvas::createCoordinateSystem()
{
    LOG_INF_S("PreviewCanvas::createCoordinateSystem: Creating coordinate system");
    
    auto* coordGroup = new SoSeparator;
    
    // Create coordinate axes using SoLineSet (based on FreeCAD implementation)
    float axisLength = 4.0f;
    
    // X-axis (Red) - pointing right
    auto* xAxisGroup = new SoSeparator;
    auto* xMaterial = new SoMaterial;
    xMaterial->diffuseColor.setValue(SbColor(1.0f, 0.0f, 0.0f)); // Red
    xMaterial->emissiveColor.setValue(SbColor(0.3f, 0.0f, 0.0f)); // Glow effect
    xAxisGroup->addChild(xMaterial);
    
    auto* xCoords = new SoCoordinate3;
    xCoords->point.set1Value(0, SbVec3f(-axisLength, 0.0f, 0.0f));
    xCoords->point.set1Value(1, SbVec3f(axisLength, 0.0f, 0.0f));
    xAxisGroup->addChild(xCoords);
    
    auto* xLine = new SoLineSet;
    xLine->numVertices.setValue(2);
    xAxisGroup->addChild(xLine);
    
    coordGroup->addChild(xAxisGroup);
    
    // Y-axis (Green) - pointing up
    auto* yAxisGroup = new SoSeparator;
    auto* yMaterial = new SoMaterial;
    yMaterial->diffuseColor.setValue(SbColor(0.0f, 1.0f, 0.0f)); // Green
    yMaterial->emissiveColor.setValue(SbColor(0.0f, 0.3f, 0.0f)); // Glow effect
    yAxisGroup->addChild(yMaterial);
    
    auto* yCoords = new SoCoordinate3;
    yCoords->point.set1Value(0, SbVec3f(0.0f, -axisLength, 0.0f));
    yCoords->point.set1Value(1, SbVec3f(0.0f, axisLength, 0.0f));
    yAxisGroup->addChild(yCoords);
    
    auto* yLine = new SoLineSet;
    yLine->numVertices.setValue(2);
    yAxisGroup->addChild(yLine);
    
    coordGroup->addChild(yAxisGroup);
    
    // Z-axis (Blue) - pointing forward
    auto* zAxisGroup = new SoSeparator;
    auto* zMaterial = new SoMaterial;
    zMaterial->diffuseColor.setValue(SbColor(0.0f, 0.0f, 1.0f)); // Blue
    zMaterial->emissiveColor.setValue(SbColor(0.0f, 0.0f, 0.3f)); // Glow effect
    zAxisGroup->addChild(zMaterial);
    
    auto* zCoords = new SoCoordinate3;
    zCoords->point.set1Value(0, SbVec3f(0.0f, 0.0f, -axisLength));
    zCoords->point.set1Value(1, SbVec3f(0.0f, 0.0f, axisLength));
    zAxisGroup->addChild(zCoords);
    
    auto* zLine = new SoLineSet;
    zLine->numVertices.setValue(2);
    zAxisGroup->addChild(zLine);
    
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
    
    // Check if the GL context is still valid
    if (!m_glContext->IsOK()) {
        LOG_ERR_S("PreviewCanvas::render: GL context is not OK");
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
    
    // Create render action with proper settings for multi-light support
    SoGLRenderAction renderAction(viewport);
    renderAction.setSmoothing(!fastMode);
    renderAction.setNumPasses(fastMode ? 1 : 2);
    renderAction.setTransparencyType(
        fastMode ? SoGLRenderAction::BLEND : SoGLRenderAction::SORTED_OBJECT_BLEND
    );
    
    // Set up OpenGL state for proper lighting
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_LIGHTING); // Enable lighting
    glEnable(GL_NORMALIZE); // Enable normal normalization for proper lighting
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
    try {
    renderAction.apply(m_sceneRoot);
    } catch (const std::exception& e) {
        LOG_ERR_S("PreviewCanvas::render: Exception during rendering: " + std::string(e.what()));
        return;
    } catch (...) {
        LOG_ERR_S("PreviewCanvas::render: Unknown exception during rendering");
        return;
    }
    
    // Check for OpenGL errors after rendering
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERR_S("Post-render: OpenGL error: " + std::to_string(err));
    }
    
    SwapBuffers();
}

void PreviewCanvas::resetView()
{
    setupDefaultCamera();
    render(false);
}

void PreviewCanvas::updateLighting(float ambient, float diffuse, float specular, const wxColour& color, float intensity)
{
    LOG_INF_S("PreviewCanvas::updateLighting: Updating lighting properties using LightManager");
    
    if (!m_lightManager) {
        LOG_WRN_S("PreviewCanvas::updateLighting: Light manager not initialized");
        return;
    }
    
    if (!m_lightMaterial) {
        LOG_WRN_S("PreviewCanvas::updateLighting: No light material available");
        return;
    }
    
    // Update material properties
    float r = color.Red() / 255.0f;
    float g = color.Green() / 255.0f;
    float b = color.Blue() / 255.0f;
    
    m_lightMaterial->ambientColor.setValue(SbColor(r * ambient, g * ambient, b * ambient));
    m_lightMaterial->diffuseColor.setValue(SbColor(r * diffuse, g * diffuse, b * diffuse));
    m_lightMaterial->specularColor.setValue(SbColor(r * specular, g * specular, b * specular));
    
    // Get current lights and update their properties
    auto currentLights = m_lightManager->getAllLightSettings();
    std::vector<RenderLightSettings> updatedLights;
    
    for (auto& light : currentLights) {
        // Update light color and intensity
        light.color = color;
        light.intensity = light.intensity * intensity; // Scale existing intensity
        updatedLights.push_back(light);
    }
    
    // Apply updated lights through LightManager
    if (!updatedLights.empty()) {
        m_lightManager->updateMultipleLights(updatedLights);
    }
    
    render(true);
}

void PreviewCanvas::updateMaterial(float ambient, float diffuse, float specular, float shininess, float transparency)
{
    LOG_INF_S("PreviewCanvas::updateMaterial: Updating material properties using LightManager");
    
    if (!m_lightManager) {
        LOG_WRN_S("PreviewCanvas::updateMaterial: Light manager not initialized");
        return;
    }
    
    // Material updates are handled by LightManager
    m_lightManager->updateMaterialsForLighting();
    
    render(true);
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

// Unified light management interface methods
int PreviewCanvas::addLight(const RenderLightSettings& settings)
{
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::addLight: Light manager not initialized");
        return -1;
    }
    
    int lightId = m_lightManager->addLight(settings);
    if (lightId >= 0) {
        render(true);
    }
    return lightId;
}

bool PreviewCanvas::removeLight(int lightId)
{
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::removeLight: Light manager not initialized");
        return false;
    }
    
    bool success = m_lightManager->removeLight(lightId);
    if (success) {
        render(true);
    }
    return success;
}

bool PreviewCanvas::updateLight(int lightId, const RenderLightSettings& settings)
{
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::updateLight: Light manager not initialized");
        return false;
    }
    
    bool success = m_lightManager->updateLight(lightId, settings);
    if (success) {
        render(true);
    }
    return success;
}

void PreviewCanvas::updateMultipleLights(const std::vector<RenderLightSettings>& lights)
{
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::updateMultipleLights: Light manager not initialized");
        return;
    }
    
    m_lightManager->updateMultipleLights(lights);
    render(true);
}

void PreviewCanvas::updateMultiLighting(const std::vector<RenderLightSettings>& lights)
{
    // Legacy method - now delegates to updateMultipleLights
    updateMultipleLights(lights);
}

std::vector<RenderLightSettings> PreviewCanvas::getAllLights() const
{
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::getAllLights: Light manager not initialized");
        return {};
    }
    
    return m_lightManager->getAllLightSettings();
}

void PreviewCanvas::clearAllLights()
{
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::clearAllLights: Light manager not initialized");
        return;
    }
    
    m_lightManager->clearAllLights();
    render(true);
}

void PreviewCanvas::resetToDefaultLighting()
{
    LOG_INF_S("PreviewCanvas::resetToDefaultLighting: Resetting to default three-point lighting");
    
    if (!m_lightManager) {
        LOG_ERR_S("PreviewCanvas::resetToDefaultLighting: Light manager not initialized");
        return;
    }
    
    // Clear existing lights
    m_lightManager->clearAllLights();
    
    // Setup default lighting
    setupDefaultLighting();
    
    render(true);
}

bool PreviewCanvas::hasLights() const
{
    if (!m_lightManager) {
        return false;
    }
    
    return m_lightManager->getLightCount() > 0;
}









void PreviewCanvas::updateRenderingMode(int mode)
{
    // This method will handle different rendering modes
    // For now, we'll implement basic modes
    switch (mode) {
        case 0: // Solid
            // Default solid rendering
            break;
        case 1: // Wireframe
            // TODO: Implement wireframe rendering
            break;
        case 2: // Points
            // TODO: Implement point rendering
            break;
        case 3: // Hidden Line
            // TODO: Implement hidden line rendering
            break;
        case 4: // Shaded
            // Default shaded rendering
            break;
        default:
            // Default to solid rendering
            break;
    }
    
    // Trigger re-render
    render(false);
    
    LOG_INF_S("PreviewCanvas::updateRenderingMode: Rendering mode updated to " + std::to_string(mode));
}

// Anti-aliasing management interface
int PreviewCanvas::addAntiAliasingConfig(const AntiAliasingSettings& settings)
{
    if (!m_antiAliasingManager) {
        LOG_ERR_S("PreviewCanvas::addAntiAliasingConfig: Anti-aliasing manager not initialized");
        return -1;
    }
    
    int configId = m_antiAliasingManager->addConfiguration(settings);
    if (configId >= 0) {
        render(true);
    }
    return configId;
}

bool PreviewCanvas::removeAntiAliasingConfig(int configId)
{
    if (!m_antiAliasingManager) {
        LOG_ERR_S("PreviewCanvas::removeAntiAliasingConfig: Anti-aliasing manager not initialized");
        return false;
    }
    
    bool success = m_antiAliasingManager->removeConfiguration(configId);
    if (success) {
        render(true);
    }
    return success;
}

bool PreviewCanvas::updateAntiAliasingConfig(int configId, const AntiAliasingSettings& settings)
{
    if (!m_antiAliasingManager) {
        LOG_ERR_S("PreviewCanvas::updateAntiAliasingConfig: Anti-aliasing manager not initialized");
        return false;
    }
    
    bool success = m_antiAliasingManager->updateConfiguration(configId, settings);
    if (success) {
        render(true);
    }
    return success;
}

bool PreviewCanvas::setActiveAntiAliasingConfig(int configId)
{
    if (!m_antiAliasingManager) {
        LOG_ERR_S("PreviewCanvas::setActiveAntiAliasingConfig: Anti-aliasing manager not initialized");
        return false;
    }
    
    bool success = m_antiAliasingManager->setActiveConfiguration(configId);
    if (success) {
        render(true);
    }
    return success;
}

std::vector<AntiAliasingSettings> PreviewCanvas::getAllAntiAliasingConfigs() const
{
    if (!m_antiAliasingManager) {
        LOG_ERR_S("PreviewCanvas::getAllAntiAliasingConfigs: Anti-aliasing manager not initialized");
        return {};
    }
    
    return m_antiAliasingManager->getAllConfigurations();
}

// Rendering management interface
int PreviewCanvas::addRenderingConfig(const RenderingSettings& settings)
{
    if (!m_renderingManager) {
        LOG_ERR_S("PreviewCanvas::addRenderingConfig: Rendering manager not initialized");
        return -1;
    }
    
    int configId = m_renderingManager->addConfiguration(settings);
    if (configId >= 0) {
        render(true);
    }
    return configId;
}

bool PreviewCanvas::removeRenderingConfig(int configId)
{
    if (!m_renderingManager) {
        LOG_ERR_S("PreviewCanvas::removeRenderingConfig: Rendering manager not initialized");
        return false;
    }
    
    bool success = m_renderingManager->removeConfiguration(configId);
    if (success) {
        render(true);
    }
    return success;
}

bool PreviewCanvas::updateRenderingConfig(int configId, const RenderingSettings& settings)
{
    if (!m_renderingManager) {
        LOG_ERR_S("PreviewCanvas::updateRenderingConfig: Rendering manager not initialized");
        return false;
    }
    
    bool success = m_renderingManager->updateConfiguration(configId, settings);
    if (success) {
        render(true);
    }
    return success;
}

bool PreviewCanvas::setActiveRenderingConfig(int configId)
{
    if (!m_renderingManager) {
        LOG_ERR_S("PreviewCanvas::setActiveRenderingConfig: Rendering manager not initialized");
        return false;
    }
    
    bool success = m_renderingManager->setActiveConfiguration(configId);
    if (success) {
        render(true);
    }
    return success;
}

std::vector<RenderingSettings> PreviewCanvas::getAllRenderingConfigs() const
{
    if (!m_renderingManager) {
        LOG_ERR_S("PreviewCanvas::getAllRenderingConfigs: Rendering manager not initialized");
        return {};
    }
    
    return m_renderingManager->getAllConfigurations();
}

// Object management interface implementation
int PreviewCanvas::addObject(const ObjectSettings& settings)
{
    if (!m_objectManager) {
        LOG_ERR_S("PreviewCanvas::addObject: Object manager not initialized");
        return -1;
    }
    
    int objectId = m_objectManager->addObject(settings);
    if (objectId >= 0) {
        render(true);
    }
    return objectId;
}

bool PreviewCanvas::removeObject(int objectId)
{
    if (!m_objectManager) {
        LOG_ERR_S("PreviewCanvas::removeObject: Object manager not initialized");
        return false;
    }
    
    bool success = m_objectManager->removeObject(objectId);
    if (success) {
        render(true);
    }
    return success;
}

bool PreviewCanvas::updateObject(int objectId, const ObjectSettings& settings)
{
    if (!m_objectManager) {
        LOG_ERR_S("PreviewCanvas::updateObject: Object manager not initialized");
        return false;
    }
    
    bool success = m_objectManager->updateObject(objectId, settings);
    if (success) {
        render(true);
    }
    return success;
}

void PreviewCanvas::updateMultipleObjects(const std::vector<ObjectSettings>& objects)
{
    if (!m_objectManager) {
        LOG_ERR_S("PreviewCanvas::updateMultipleObjects: Object manager not initialized");
        return;
    }
    
    m_objectManager->updateMultipleObjects(objects);
    render(true);
}

void PreviewCanvas::clearAllObjects()
{
    if (!m_objectManager) {
        LOG_ERR_S("PreviewCanvas::clearAllObjects: Object manager not initialized");
        return;
    }
    
    m_objectManager->clearAllObjects();
    render(true);
}

std::vector<ObjectSettings> PreviewCanvas::getAllObjects() const
{
    if (!m_objectManager) {
        LOG_ERR_S("PreviewCanvas::getAllObjects: Object manager not initialized");
        return std::vector<ObjectSettings>();
    }
    
    return m_objectManager->getAllObjectSettings();
} 