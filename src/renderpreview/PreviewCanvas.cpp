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
    
    // Create main directional light (front light)
    m_light = new SoDirectionalLight;
    m_light->ref();
    m_light->direction.setValue(SbVec3f(0.0f, 0.0f, -1.0f)); // Light from front
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

    // Helper lambda to create a single arrow indicator for a light
    auto createArrowForLight = [](SoDirectionalLight* light) -> SoSeparator* {
        if (!light) return nullptr;

        SoSeparator* indicatorSep = new SoSeparator;

        // Group for the whole arrow, so we can position it
        SoSeparator* arrowGroup = new SoSeparator;

        // Transform to position the arrow far away, pointing towards the origin
        SoTransform* positionTransform = new SoTransform;
        SbVec3f lightDir = light->direction.getValue();
        positionTransform->translation.setValue(lightDir * -12.0f); // Position far out
        arrowGroup->addChild(positionTransform);

        // Material based on light color and intensity
        SoMaterial* material = new SoMaterial;
        SbColor color = light->color.getValue();
        material->diffuseColor.setValue(color);
        // Emissive color makes it glow, good for indicators
        material->emissiveColor.setValue(color * 0.4f);
        arrowGroup->addChild(material);

        // Transform to orient the arrow geometry
        SoTransform* orientationTransform = new SoTransform;
        SbVec3f defaultArrowDir(0.0f, 1.0f, 0.0f); // Assuming arrow model points up Y
        SbRotation rotation(defaultArrowDir, -lightDir);
        orientationTransform->rotation.setValue(rotation);
        arrowGroup->addChild(orientationTransform);

        // Arrow Geometry (Shaft and Head)
        // Shaft (a cylinder)
        SoCylinder* shaft = new SoCylinder;
        shaft->radius = 0.1f;
        // Scale height by intensity
        shaft->height.setValue(2.0f * light->intensity.getValue());
        arrowGroup->addChild(shaft);

        // Head (a cone)
        SoCone* head = new SoCone;
        head->bottomRadius = 0.2f;
        head->height = 0.5f;
        SoTransform* headTransform = new SoTransform;
        // Position at the end of the shaft
        headTransform->translation.setValue(0.0f, (shaft->height.getValue() / 2.0f) + (head->height.getValue() / 2.0f), 0.0f);
        arrowGroup->addChild(headTransform);
        arrowGroup->addChild(head);

        indicatorSep->addChild(arrowGroup);
        return indicatorSep;
    };

    // Use SoSearchAction to find all directional lights in the scene
    SoSearchAction searchAction;
    searchAction.setType(SoDirectionalLight::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_sceneRoot);

    int numLights = searchAction.getPaths().getLength();
    LOG_INF_S("Found " + std::to_string(numLights) + " directional lights to create indicators for.");

    for (int i = 0; i < numLights; ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (path && path->getTail()->isOfType(SoDirectionalLight::getClassTypeId())) {
            SoDirectionalLight* light = static_cast<SoDirectionalLight*>(path->getTail());
            SoSeparator* arrow = createArrowForLight(light);
            if (arrow) {
                m_lightIndicator->addChild(arrow);
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
    if (!m_lightMaterial || !m_light) return;
    
    // Update light material
    float r = color.Red() / 255.0f;
    float g = color.Green() / 255.0f;
    float b = color.Blue() / 255.0f;
    
    m_lightMaterial->ambientColor.setValue(SbColor(r * ambient, g * ambient, b * ambient));
    m_lightMaterial->diffuseColor.setValue(SbColor(r * diffuse, g * diffuse, b * diffuse));
    m_lightMaterial->specularColor.setValue(SbColor(r * specular, g * specular, b * specular));
    
    // Update light intensity
    m_light->intensity.setValue(intensity);
    
    // Update light indicator
    updateLightIndicator(color, intensity);
    
    render(true);
}

void PreviewCanvas::updateMaterial(float ambient, float diffuse, float specular, float shininess, float transparency)
{
    // Update material properties for all geometry objects
    // This would need to be implemented by updating the materials of individual objects
    // For now, we'll just trigger a re-render
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