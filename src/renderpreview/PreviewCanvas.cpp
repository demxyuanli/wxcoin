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
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoSpotLight.h>
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

void PreviewCanvas::createLightIndicator(SoLight* light, int lightIndex, const std::string& lightName, SoSeparator* container, const SbVec3f& lightPosition)
{
    if (!light || !container) {
        LOG_WRN_S("PreviewCanvas::createLightIndicator: Invalid light or container");
        return;
    }
    
    // Create a visual indicator for the light
    SoSeparator* indicator = new SoSeparator();
    if (!indicator) {
        LOG_ERR_S("PreviewCanvas::createLightIndicator: Failed to create indicator separator");
        return;
    }
    
    // Get light properties
    SbColor lightColor = light->color.getValue();
    float lightIntensity = light->intensity.getValue();
    
    // Calculate proper position and direction based on light type
    SbVec3f indicatorPosition = lightPosition;
    SbVec3f lightDirection(0.0f, 0.0f, -1.0f); // Default direction
    
    if (light->isOfType(SoDirectionalLight::getClassTypeId())) {
        SoDirectionalLight* dirLight = static_cast<SoDirectionalLight*>(light);
        if (dirLight) {
            lightDirection = dirLight->direction.getValue();
            // For directional lights, position the indicator at a reasonable distance from origin
            // based on the light direction
            indicatorPosition = lightDirection * -8.0f; // 8 units away from origin in light direction
        }
    } else if (light->isOfType(SoSpotLight::getClassTypeId())) {
        SoSpotLight* spotLight = static_cast<SoSpotLight*>(light);
        if (spotLight) {
            lightDirection = spotLight->direction.getValue();
            // For spot lights, use the actual position
            indicatorPosition = lightPosition;
        }
    } else if (light->isOfType(SoPointLight::getClassTypeId())) {
        SoPointLight* pointLight = static_cast<SoPointLight*>(light);
        if (pointLight) {
            // For point lights, use the actual position
            indicatorPosition = lightPosition;
            // Point lights don't have a specific direction, so we'll show a sphere without direction line
        }
    }
    
    // Create transform to position the indicator
    SoTransform* transform = new SoTransform();
    if (transform) {
        transform->translation.setValue(indicatorPosition);
        indicator->addChild(transform);
    }
    
    // Create material for the indicator
    SoMaterial* indicatorMaterial = new SoMaterial();
    if (indicatorMaterial) {
        indicatorMaterial->diffuseColor.setValue(lightColor);
        indicatorMaterial->emissiveColor.setValue(lightColor * 0.8f); // Make it glow
        indicatorMaterial->ambientColor.setValue(lightColor * 0.3f);
        indicatorMaterial->transparency.setValue(0.2f); // Slight transparency
        indicator->addChild(indicatorMaterial);
    }
    
    // Create light source sphere (size based on intensity)
    SoSphere* lightSphere = new SoSphere();
    if (lightSphere) {
        float sphereRadius = 0.2f + lightIntensity * 0.3f; // Size varies with intensity
        lightSphere->radius.setValue(sphereRadius);
        indicator->addChild(lightSphere);
    }
    
    // Create direction line for directional and spot lights
    if (light->isOfType(SoDirectionalLight::getClassTypeId()) || light->isOfType(SoSpotLight::getClassTypeId())) {
        SoSeparator* lineGroup = new SoSeparator();
        if (lineGroup) {
            // Line material (same color as light but more opaque)
            SoMaterial* lineMaterial = new SoMaterial();
            if (lineMaterial) {
                lineMaterial->diffuseColor.setValue(lightColor);
                lineMaterial->ambientColor.setValue(lightColor * 0.5f);
                lineMaterial->emissiveColor.setValue(lightColor * 0.3f);
                lineGroup->addChild(lineMaterial);
            }
            
            // Create line coordinates
            SoCoordinate3* lineCoords = new SoCoordinate3();
            if (lineCoords) {
                SbVec3f startPos = SbVec3f(0.0f, 0.0f, 0.0f); // Start from light sphere center
                SbVec3f endPos = SbVec3f(0.0f, 0.0f, 0.0f); // End at scene origin
                
                // Calculate line length based on intensity
                float lineLength = 1.0f + lightIntensity * 2.0f;
                
                // For directional lights, calculate the direction to origin
                if (light->isOfType(SoDirectionalLight::getClassTypeId())) {
                    // Calculate direction from light position to origin
                    SbVec3f lightPos = indicatorPosition;
                    SbVec3f origin = SbVec3f(0.0f, 0.0f, 0.0f);
                    SbVec3f directionToOrigin = origin - lightPos;
                    directionToOrigin.normalize();
                    
                    // Use calculated line length
                    endPos = directionToOrigin * lineLength;
                } else {
                    // For spot lights, use the light direction
                    SbVec3f lightDir = lightDirection;
                    endPos = lightDir * lineLength;
                }
                
                lineCoords->point.set1Value(0, startPos);
                lineCoords->point.set1Value(1, endPos);
                lineGroup->addChild(lineCoords);
            }
            
            // Create line set
            SoLineSet* lineSet = new SoLineSet();
            if (lineSet) {
                lineSet->numVertices.setValue(2);
                lineGroup->addChild(lineSet);
            }
            
            indicator->addChild(lineGroup);
        }
    }
    
    // Create light name indicator (first letter of light name) at the end of the line
    if (lightIndex >= 0) {
        // Create a small name indicator
        SoSeparator* nameGroup = new SoSeparator();
        if (nameGroup) {
            // Calculate line end position for letter placement
            SbVec3f lineEndPos;
            float lineLength = 1.0f + lightIntensity * 2.0f;
            
            if (light->isOfType(SoDirectionalLight::getClassTypeId())) {
                // Calculate direction from light position to origin
                SbVec3f lightPos = indicatorPosition;
                SbVec3f origin = SbVec3f(0.0f, 0.0f, 0.0f);
                SbVec3f directionToOrigin = origin - lightPos;
                directionToOrigin.normalize();
                lineEndPos = directionToOrigin * lineLength;
            } else if (light->isOfType(SoSpotLight::getClassTypeId())) {
                lineEndPos = lightDirection * lineLength;
            } else {
                lineEndPos = SbVec3f(0.0f, 0.0f, 0.0f);
            }
            
            // Position letter at the end of the line
            SoTransform* nameTransform = new SoTransform();
            if (nameTransform) {
                nameTransform->translation.setValue(lineEndPos);
                nameGroup->addChild(nameTransform);
            }
            
            // Name material (black text)
            SoMaterial* nameMaterial = new SoMaterial();
            if (nameMaterial) {
                nameMaterial->diffuseColor.setValue(SbColor(0.0f, 0.0f, 0.0f)); // Black
                nameMaterial->emissiveColor.setValue(SbColor(0.0f, 0.0f, 0.0f)); // No glow
                nameGroup->addChild(nameMaterial);
            }
            
            // Create text node for the first letter
            SoText2* nameText = new SoText2();
            if (nameText) {
                // Extract first letter from light name
                std::string firstLetter = "L"; // Default to "L" for Light
                if (!lightName.empty()) {
                    firstLetter = std::string(1, lightName[0]);
                }
                
                nameText->string.setValue(firstLetter.c_str());
                nameText->justification.setValue(SoText2::CENTER);
                nameText->spacing.setValue(1.0f);
                nameGroup->addChild(nameText);
            }
            
            indicator->addChild(nameGroup);
        }
    }
    
    // Add indicator to container
    container->addChild(indicator);
}

void PreviewCanvas::createLightIndicator()
{
    LOG_INF_S("PreviewCanvas::createLightIndicator: Creating visual indicators for all lights");

    if (!m_objectRoot) {
        LOG_WRN_S("PreviewCanvas::createLightIndicator: Object root not available");
        return;
    }

    m_lightIndicator = new SoSeparator;
    if (!m_lightIndicator) {
        LOG_ERR_S("PreviewCanvas::createLightIndicator: Failed to create light indicator separator");
        return;
    }
    m_lightIndicator->ref();

    // Create indicators for each light
    // Main light (front)
    if (m_light) {
        // Calculate position for directional light
        SbVec3f lightDirection = m_light->direction.getValue();
        SbVec3f indicatorPosition = lightDirection * -8.0f; // 8 units away from origin in light direction
        
        createLightIndicator(m_light, 0, "Main Light", m_lightIndicator, indicatorPosition);
    }

    // Find and create indicators for other lights
    if (!m_sceneRoot) {
        LOG_WRN_S("PreviewCanvas::createLightIndicator: Scene root not available");
        return;
    }

    SoSearchAction searchAction;
    searchAction.setType(SoDirectionalLight::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_sceneRoot);

    int lightIndex = 1;
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (!path) {
            LOG_WRN_S("PreviewCanvas::createLightIndicator: Null path encountered, skipping");
            continue;
        }
        
        SoNode* tailNode = path->getTail();
        if (!tailNode) {
            LOG_WRN_S("PreviewCanvas::createLightIndicator: Null tail node, skipping");
            continue;
        }
        
        if (!tailNode->isOfType(SoDirectionalLight::getClassTypeId())) {
            LOG_WRN_S("PreviewCanvas::createLightIndicator: Tail node is not a directional light, skipping");
            continue;
        }
        
        SoDirectionalLight* light = static_cast<SoDirectionalLight*>(tailNode);
        if (!light) {
            LOG_WRN_S("PreviewCanvas::createLightIndicator: Failed to cast to directional light, skipping");
            continue;
        }
        
        // Skip the main light as it's already handled
        if (light != m_light) {
            // Calculate position for directional light
            SbVec3f lightDirection = light->direction.getValue();
            SbVec3f indicatorPosition = lightDirection * -8.0f; // 8 units away from origin in light direction
            
            createLightIndicator(light, lightIndex++, "Light " + std::to_string(lightIndex), m_lightIndicator, indicatorPosition);
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
        if (m_objectRoot) {
            int indicatorIndex = m_objectRoot->findChild(m_lightIndicator);
            if (indicatorIndex >= 0) {
                m_objectRoot->removeChild(indicatorIndex);
            } else {
                LOG_WRN_S("PreviewCanvas::updateLightIndicator: Light indicator not found in object root");
            }
        } else {
            LOG_WRN_S("PreviewCanvas::updateLightIndicator: Object root not available");
        }
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
    
    if (!m_lightMaterial) {
        LOG_WRN_S("PreviewCanvas::updateLighting: No light material available");
        return;
    }
    
    if (!m_sceneRoot) {
        LOG_WRN_S("PreviewCanvas::updateLighting: No scene root available");
        return;
    }
    
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
        if (!path) {
            LOG_WRN_S("PreviewCanvas::updateLighting: Null path encountered, skipping");
            continue;
        }
        
        SoNode* tailNode = path->getTail();
        if (!tailNode) {
            LOG_WRN_S("PreviewCanvas::updateLighting: Null tail node, skipping");
            continue;
        }
        
        if (!tailNode->isOfType(SoDirectionalLight::getClassTypeId())) {
            LOG_WRN_S("PreviewCanvas::updateLighting: Tail node is not a directional light, skipping");
            continue;
        }
        
        SoDirectionalLight* light = static_cast<SoDirectionalLight*>(tailNode);
        if (!light) {
            LOG_WRN_S("PreviewCanvas::updateLighting: Failed to cast to directional light, skipping");
            continue;
        }
        
        // Update light color
        light->color.setValue(SbColor(r, g, b));
        
        // Update light intensity (scale based on original intensity)
        float originalIntensity = light->intensity.getValue();
        float scaledIntensity = originalIntensity * intensity;
        light->intensity.setValue(scaledIntensity);
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
    } else {
        LOG_WRN_S("PreviewCanvas::updateMaterial: Sphere object or its Coin node not available");
    }
    
    if (m_occCone && m_occCone->getCoinNode()) {
        updateObjectMaterial(m_occCone->getCoinNode(), ambient, diffuse, specular, shininess, transparency);
    } else {
        LOG_WRN_S("PreviewCanvas::updateMaterial: Cone object or its Coin node not available");
    }
    
    if (m_occBox && m_occBox->getCoinNode()) {
        updateObjectMaterial(m_occBox->getCoinNode(), ambient, diffuse, specular, shininess, transparency);
    } else {
        LOG_WRN_S("PreviewCanvas::updateMaterial: Box object or its Coin node not available");
    }
    
    render(true);
}

void PreviewCanvas::updateObjectMaterial(SoNode* node, float ambient, float diffuse, float specular, float shininess, float transparency)
{
    if (!node) {
        LOG_WRN_S("PreviewCanvas::updateObjectMaterial: Null node provided");
        return;
    }
    
    // Use SoSearchAction to find material nodes in the object's scene graph
    SoSearchAction searchAction;
    searchAction.setType(SoMaterial::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(node);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (!path) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterial: Null path encountered, skipping");
            continue;
        }
        
        SoNode* tailNode = path->getTail();
        if (!tailNode) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterial: Null tail node, skipping");
            continue;
        }
        
        if (!tailNode->isOfType(SoMaterial::getClassTypeId())) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterial: Tail node is not a material, skipping");
            continue;
        }
        
        SoMaterial* material = static_cast<SoMaterial*>(tailNode);
        if (!material) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterial: Failed to cast to material, skipping");
            continue;
        }
        
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

void PreviewCanvas::updateMultiLighting(const std::vector<RenderLightSettings>& lights)
{
    LOG_INF_S("PreviewCanvas::updateMultiLighting: Updating multiple lights");
    
    if (!m_lightMaterial) {
        LOG_WRN_S("PreviewCanvas::updateMultiLighting: No light material available");
        return;
    }
    
    if (!m_sceneRoot) {
        LOG_WRN_S("PreviewCanvas::updateMultiLighting: No scene root available");
        return;
    }
    
    // Clear ALL existing lights including the main light
    // Use a safer approach to clear lights
    clearAllLights();
    
    // Reset main light pointer since we removed it
    m_light = nullptr;
    
    // Clear existing light indicators container
    if (m_lightIndicatorsContainer) {
        // Check if the container is still a valid child of scene root
        int containerIndex = m_sceneRoot->findChild(m_lightIndicatorsContainer);
        if (containerIndex >= 0) {
            m_sceneRoot->removeChild(containerIndex);
        }
        m_lightIndicatorsContainer = nullptr;
    }
    
    // Clear main light indicator
    if (m_lightIndicator) {
        // Check if the indicator is still a valid child of object root
        if (m_objectRoot) {
            int indicatorIndex = m_objectRoot->findChild(m_lightIndicator);
            if (indicatorIndex >= 0) {
                m_objectRoot->removeChild(indicatorIndex);
            }
        }
        m_lightIndicator->unref();
        m_lightIndicator = nullptr;
    }
    
    // Create new light indicators container
    m_lightIndicatorsContainer = new SoSeparator();
    if (m_lightIndicatorsContainer) {
        m_sceneRoot->addChild(m_lightIndicatorsContainer);
    }
    
    // Add new lights and create indicators
    for (size_t i = 0; i < lights.size(); ++i) {
        const auto& lightSettings = lights[i];
        if (lightSettings.enabled) {
            // Create light based on type
            SoLight* newLight = createLightByType(lightSettings);
            if (!newLight) {
                LOG_ERR_S("PreviewCanvas::updateMultiLighting: Failed to create light of type: " + lightSettings.type);
                continue;
            }
            
            // Set common light properties
            float r = lightSettings.color.Red() / 255.0f;
            float g = lightSettings.color.Green() / 255.0f;
            float b = lightSettings.color.Blue() / 255.0f;
            
            newLight->color.setValue(SbColor(r, g, b));
            newLight->intensity.setValue(static_cast<float>(lightSettings.intensity));
            
            // Create light group and add to scene
            SoSeparator* lightGroup = new SoSeparator();
            if (!lightGroup) {
                LOG_ERR_S("PreviewCanvas::updateMultiLighting: Failed to create light group");
                newLight->unref();
                continue;
            }
            
            // For directional lights, we need a transform for positioning
            // For point and spot lights, position is handled by the light itself
            if (lightSettings.type == "directional") {
                SoTransform* lightTransform = new SoTransform();
                if (lightTransform) {
                    SbVec3f position(static_cast<float>(lightSettings.positionX),
                                    static_cast<float>(lightSettings.positionY),
                                    static_cast<float>(lightSettings.positionZ));
                    lightTransform->translation.setValue(position);
                    lightGroup->addChild(lightTransform);
                }
            }
            
            lightGroup->addChild(newLight);
            m_sceneRoot->addChild(lightGroup);
            
            // Create light indicator
            SbVec3f lightPosition(static_cast<float>(lightSettings.positionX),
                                 static_cast<float>(lightSettings.positionY),
                                 static_cast<float>(lightSettings.positionZ));
            createLightIndicator(newLight, static_cast<int>(i), lightSettings.name, m_lightIndicatorsContainer, lightPosition);
        }
    }
    
    // Update material based on all lights for better multi-light support
    if (!lights.empty() && m_lightMaterial) {
        // Calculate combined lighting from all enabled lights
        float totalR = 0.0f, totalG = 0.0f, totalB = 0.0f;
        float totalIntensity = 0.0f;
        
        for (const auto& light : lights) {
            if (light.enabled) {
                float r = light.color.Red() / 255.0f;
                float g = light.color.Green() / 255.0f;
                float b = light.color.Blue() / 255.0f;
                float intensity = static_cast<float>(light.intensity);
                
                totalR += r * intensity;
                totalG += g * intensity;
                totalB += b * intensity;
                totalIntensity += intensity;
            }
        }
        
        // Normalize the combined color
        if (totalIntensity > 0.0f) {
            totalR /= totalIntensity;
            totalG /= totalIntensity;
            totalB /= totalIntensity;
        }
        
        // Set material properties based on combined lighting
        m_lightMaterial->ambientColor.setValue(SbColor(totalR * 0.2f, totalG * 0.2f, totalB * 0.2f));
        m_lightMaterial->diffuseColor.setValue(SbColor(totalR * 0.8f, totalG * 0.8f, totalB * 0.8f));
        m_lightMaterial->specularColor.setValue(SbColor(totalR * 0.6f, totalG * 0.6f, totalB * 0.6f));
        
        // Update geometry object materials to respond to lighting changes
        updateGeometryMaterialsForLighting(totalR, totalG, totalB, totalIntensity);
    }
    
    render(true);
}

SoLight* PreviewCanvas::createLightByType(const RenderLightSettings& lightSettings)
{
    LOG_INF_S("PreviewCanvas::createLightByType: Creating light of type: " + lightSettings.type);
    
    if (lightSettings.type == "directional") {
        SoDirectionalLight* light = new SoDirectionalLight();
        if (!light) {
            LOG_ERR_S("PreviewCanvas::createLightByType: Failed to create directional light");
            return nullptr;
        }
        
        // Set direction for directional light
        SbVec3f direction(static_cast<float>(lightSettings.directionX),
                          static_cast<float>(lightSettings.directionY),
                          static_cast<float>(lightSettings.directionZ));
        direction.normalize();
        light->direction.setValue(direction);
        
        LOG_INF_S("PreviewCanvas::createLightByType: Created directional light with direction (" + 
                  std::to_string(direction[0]) + ", " + std::to_string(direction[1]) + ", " + std::to_string(direction[2]) + ")");
        return light;
    }
    else if (lightSettings.type == "point") {
        SoPointLight* light = new SoPointLight();
        if (!light) {
            LOG_ERR_S("PreviewCanvas::createLightByType: Failed to create point light");
            return nullptr;
        }
        
        // Set position for point light
        SbVec3f position(static_cast<float>(lightSettings.positionX),
                         static_cast<float>(lightSettings.positionY),
                         static_cast<float>(lightSettings.positionZ));
        light->location.setValue(position);
        
        LOG_INF_S("PreviewCanvas::createLightByType: Created point light at position (" + 
                  std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]) + ")");
        return light;
    }
    else if (lightSettings.type == "spot") {
        SoSpotLight* light = new SoSpotLight();
        if (!light) {
            LOG_ERR_S("PreviewCanvas::createLightByType: Failed to create spot light");
            return nullptr;
        }
        
        // Set position for spot light
        SbVec3f position(static_cast<float>(lightSettings.positionX),
                         static_cast<float>(lightSettings.positionY),
                         static_cast<float>(lightSettings.positionZ));
        light->location.setValue(position);
        
        // Set direction for spot light
        SbVec3f direction(static_cast<float>(lightSettings.directionX),
                          static_cast<float>(lightSettings.directionY),
                          static_cast<float>(lightSettings.directionZ));
        direction.normalize();
        light->direction.setValue(direction);
        
        // Set spot angle (convert from degrees to radians)
        float spotAngle = static_cast<float>(lightSettings.spotAngle) * M_PI / 180.0f;
        light->cutOffAngle.setValue(spotAngle);
        
        // Set spot exponent
        light->dropOffRate.setValue(static_cast<float>(lightSettings.spotExponent));
        
        LOG_INF_S("PreviewCanvas::createLightByType: Created spot light at position (" + 
                  std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]) + 
                  ") with angle " + std::to_string(lightSettings.spotAngle) + " degrees");
        return light;
    }
    else {
        LOG_ERR_S("PreviewCanvas::createLightByType: Unknown light type: " + lightSettings.type);
        return nullptr;
    }
}

void PreviewCanvas::updateGeometryMaterialsForLighting(float lightR, float lightG, float lightB, float totalIntensity)
{
    LOG_INF_S("PreviewCanvas::updateGeometryMaterialsForLighting: Updating geometry materials for lighting");
    
    // Update sphere material (red base color)
    if (m_occSphere && m_occSphere->getCoinNode()) {
        updateObjectMaterialForLighting(m_occSphere->getCoinNode(), 
            SbColor(1.0f, 0.3f, 0.3f), // Base red color
            lightR, lightG, lightB, totalIntensity);
    } else {
        LOG_WRN_S("PreviewCanvas::updateGeometryMaterialsForLighting: Sphere object or its Coin node not available");
    }
    
    // Update cone material (green base color)
    if (m_occCone && m_occCone->getCoinNode()) {
        updateObjectMaterialForLighting(m_occCone->getCoinNode(), 
            SbColor(0.3f, 1.0f, 0.3f), // Base green color
            lightR, lightG, lightB, totalIntensity);
    } else {
        LOG_WRN_S("PreviewCanvas::updateGeometryMaterialsForLighting: Cone object or its Coin node not available");
    }
    
    // Update box material (blue base color)
    if (m_occBox && m_occBox->getCoinNode()) {
        updateObjectMaterialForLighting(m_occBox->getCoinNode(), 
            SbColor(0.3f, 0.3f, 1.0f), // Base blue color
            lightR, lightG, lightB, totalIntensity);
    } else {
        LOG_WRN_S("PreviewCanvas::updateGeometryMaterialsForLighting: Box object or its Coin node not available");
    }
}

void PreviewCanvas::clearAllLights()
{
    LOG_INF_S("PreviewCanvas::clearAllLights: Safely clearing all lights");
    
    if (!m_sceneRoot) {
        LOG_WRN_S("PreviewCanvas::clearAllLights: No scene root available");
        return;
    }
    
    // Store paths to lights before removing them to avoid iterator invalidation
    std::vector<SoFullPath*> lightPaths;
    
    // Find all directional lights
    SoSearchAction searchAction;
    searchAction.setType(SoDirectionalLight::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_sceneRoot);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (path && path->getTail() && path->getTail()->isOfType(SoDirectionalLight::getClassTypeId())) {
            lightPaths.push_back(path);
        }
    }
    
    // Find all point lights
    searchAction.setType(SoPointLight::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_sceneRoot);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (path && path->getTail() && path->getTail()->isOfType(SoPointLight::getClassTypeId())) {
            lightPaths.push_back(path);
        }
    }
    
    // Find all spot lights
    searchAction.setType(SoSpotLight::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_sceneRoot);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (path && path->getTail() && path->getTail()->isOfType(SoSpotLight::getClassTypeId())) {
            lightPaths.push_back(path);
        }
    }
    
    // Now safely remove all found lights
    for (SoFullPath* path : lightPaths) {
        if (!path) {
            LOG_WRN_S("PreviewCanvas::clearAllLights: Null path encountered, skipping");
            continue;
        }
        
        if (path->getLength() < 2) {
            LOG_WRN_S("PreviewCanvas::clearAllLights: Invalid path length, skipping");
            continue;
        }
        
        SoNode* lightNode = path->getTail();
        if (!lightNode) {
            LOG_WRN_S("PreviewCanvas::clearAllLights: Null light node, skipping");
            continue;
        }
        
        // Check if the light node is still valid
        if (lightNode->getRefCount() <= 0) {
            LOG_WRN_S("PreviewCanvas::clearAllLights: Light node has invalid ref count, skipping");
            continue;
        }
        
        SoNode* parentNode = path->getNode(path->getLength() - 2);
        if (!parentNode) {
            LOG_WRN_S("PreviewCanvas::clearAllLights: Null parent node, skipping");
            continue;
        }
        
        if (!parentNode->isOfType(SoSeparator::getClassTypeId())) {
            LOG_WRN_S("PreviewCanvas::clearAllLights: Parent is not a separator, skipping");
            continue;
        }
        
        SoSeparator* parent = static_cast<SoSeparator*>(parentNode);
        
        // Check if the light is still a child of the parent before removing
        int childIndex = parent->findChild(lightNode);
        if (childIndex >= 0) {
            LOG_INF_S("PreviewCanvas::clearAllLights: Removing light at index " + std::to_string(childIndex));
            
            // Additional safety check: verify the child at this index is still the expected light
            SoNode* childAtIndex = parent->getChild(childIndex);
            if (childAtIndex == lightNode) {
                parent->removeChild(childIndex);
            } else {
                LOG_WRN_S("PreviewCanvas::clearAllLights: Child at index does not match expected light, skipping removal");
            }
        } else {
            LOG_WRN_S("PreviewCanvas::clearAllLights: Light not found in parent, skipping removal");
        }
    }
    
    // Clear the paths vector to prevent memory leaks
    lightPaths.clear();
    
    LOG_INF_S("PreviewCanvas::clearAllLights: All lights cleared successfully");
}

void PreviewCanvas::updateObjectMaterialForLighting(SoNode* node, const SbColor& baseColor, 
                                                   float lightR, float lightG, float lightB, float totalIntensity)
{
    if (!node) {
        LOG_WRN_S("PreviewCanvas::updateObjectMaterialForLighting: Null node provided");
        return;
    }
    
    // Use SoSearchAction to find material nodes in the object's scene graph
    SoSearchAction searchAction;
    searchAction.setType(SoMaterial::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(node);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (!path) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterialForLighting: Null path encountered, skipping");
            continue;
        }
        
        SoNode* tailNode = path->getTail();
        if (!tailNode) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterialForLighting: Null tail node, skipping");
            continue;
        }
        
        if (!tailNode->isOfType(SoMaterial::getClassTypeId())) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterialForLighting: Tail node is not a material, skipping");
            continue;
        }
        
        SoMaterial* material = static_cast<SoMaterial*>(tailNode);
        if (!material) {
            LOG_WRN_S("PreviewCanvas::updateObjectMaterialForLighting: Failed to cast to material, skipping");
            continue;
        }
        
        // Calculate lighting-adjusted colors
        float ambientR = baseColor[0] * lightR * 0.3f;
        float ambientG = baseColor[1] * lightG * 0.3f;
        float ambientB = baseColor[2] * lightB * 0.3f;
        
        float diffuseR = baseColor[0] * lightR * 0.8f;
        float diffuseG = baseColor[1] * lightG * 0.8f;
        float diffuseB = baseColor[2] * lightB * 0.8f;
        
        float specularR = lightR * 0.6f;
        float specularG = lightG * 0.6f;
        float specularB = lightB * 0.6f;
        
        // Apply intensity scaling
        float intensityScale = std::min(totalIntensity / 3.0f, 2.0f); // Cap at 2x intensity
        
        material->ambientColor.setValue(SbColor(ambientR * intensityScale, ambientG * intensityScale, ambientB * intensityScale));
        material->diffuseColor.setValue(SbColor(diffuseR * intensityScale, diffuseG * intensityScale, diffuseB * intensityScale));
        material->specularColor.setValue(SbColor(specularR * intensityScale, specularG * intensityScale, specularB * intensityScale));
    }
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