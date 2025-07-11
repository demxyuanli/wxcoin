#include "NavigationCube.h"
#include "DPIManager.h"
#include "DPIAwareRendering.h"
#include <algorithm>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLinear.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/time.h>
#include <cmath>
#include <vector>
#include "logger/Logger.h"

std::map<std::string, std::shared_ptr<NavigationCube::TextureData>> NavigationCube::s_textureCache;

NavigationCube::NavigationCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight)
    : m_root(new SoSeparator)
    , m_orthoCamera(new SoOrthographicCamera)
    , m_cameraTransform(new SoTransform)
    , m_enabled(true)
    , m_dpiScale(dpiScale)
    , m_viewChangeCallback(viewChangeCallback)
    , m_isDragging(false)
    , m_lastMousePos(0, 0)
    , m_rotationX(0.0f)
    , m_rotationY(0.0f)
    , m_lastDragTime(0)
    , m_windowWidth(windowWidth)
    , m_windowHeight(windowHeight)
{
    m_root->ref();
    initialize();
}

NavigationCube::~NavigationCube() {
    m_root->unref();
}

void NavigationCube::initialize() {
    setupGeometry();

    m_faceToView = {
        { "F", "Front" },
        { "B", "Back" },
        { "L", "Left" },
        { "R", "Right" },
        { "T", "Top" },
        { "D", "Bottom" }
    };
}

bool NavigationCube::generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height) {
    wxBitmap bitmap(width, height, 32);
    wxMemoryDC dc;
    dc.SelectObject(bitmap); // Explicitly select bitmap
    if (!dc.IsOk()) {
        //LOG_ERR_S("NavigationCube::generateFaceTexture: Failed to create wxMemoryDC for texture: " + text);
        // Fallback: Fill with white
        for (int i = 0; i < width * height * 4; i += 4) {
            imageData[i] = 255; // R
            imageData[i + 1] = 255; // G
            imageData[i + 2] = 255; // B
            imageData[i + 3] = 255; // A
        }
        //LOG_INF_S("NavigationCube::generateFaceTexture: Fallback to white texture for: " + text);
        return true;
    }

    dc.SetBackground(wxColour(255, 255, 255, 255)); // Opaque white background
    dc.Clear();

    // Use DPI manager for high-quality font rendering
    auto& dpiManager = DPIManager::getInstance();
    wxFont font = dpiManager.getScaledFont(16, "Arial", true, false);
    dc.SetFont(font);
    dc.SetTextForeground(wxColour(255, 0, 0)); // Red text

    wxSize textSize = dc.GetTextExtent(text);
    int x = (width - textSize.GetWidth()) / 2;
    int y = (height - textSize.GetHeight()) / 2;
    dc.DrawText(text, x, y);

    // Validate bitmap content
    wxImage image = bitmap.ConvertToImage();
    if (!image.IsOk()) {
        LOG_ERR_S("NavigationCube::generateFaceTexture: Failed to convert bitmap to image for texture: " + text);
        // Fallback: Fill with white
        for (int i = 0; i < width * height * 4; i += 4) {
            imageData[i] = 255; // R
            imageData[i + 1] = 255; // G
            imageData[i + 2] = 255; // B
            imageData[i + 3] = 255; // A
        }
        LOG_INF_S("NavigationCube::generateFaceTexture: Fallback to white texture for: " + text);
        return true;
    }

    image.InitAlpha(); // Ensure alpha channel
    unsigned char* rgb = image.GetData();
    unsigned char* alpha = image.GetAlpha();

    // Set alpha to 255 for all pixels (opaque texture)
    for (int i = 0; i < width * height; ++i) {
        alpha[i] = 255;
    }

    // Copy to imageData (RGBA) and validate
    bool hasValidPixels = false;
    for (int i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
        imageData[i] = rgb[j];     // R
        imageData[i + 1] = rgb[j + 1]; // G
        imageData[i + 2] = rgb[j + 2]; // B
        imageData[i + 3] = alpha[j / 3]; // A
        if (imageData[i] != 0 || imageData[i + 1] != 0 || imageData[i + 2] != 0) {
            hasValidPixels = true;
        }
    }

    // Debug: Log sample pixels
    LOG_INF("NavigationCube::generateFaceTexture: Background pixel (0,0) RGBA=" +
        std::to_string(imageData[0]) + "," +
        std::to_string(imageData[1]) + "," +
        std::to_string(imageData[2]) + "," +
        std::to_string(imageData[3]), "NavigationCube");
    int textPixelIdx = (width * height / 2) * 4; // Middle of texture for text
    LOG_INF("NavigationCube::generateFaceTexture: Text pixel (center) RGBA=" +
        std::to_string(imageData[textPixelIdx]) + "," +
        std::to_string(imageData[textPixelIdx + 1]) + "," +
        std::to_string(imageData[textPixelIdx + 2]) + "," +
        std::to_string(imageData[textPixelIdx + 3]),"NavigationCube");
    // Log additional pixels
    for (int i = 0; i < 20; i += 4) {
        LOG_INF("NavigationCube::generateFaceTexture: Debug pixel " + std::to_string(i / 4) + " RGBA=" +
            std::to_string(imageData[i]) + "," +
            std::to_string(imageData[i + 1]) + "," +
            std::to_string(imageData[i + 2]) + "," +
            std::to_string(imageData[i + 3]), "NavigationCube");
    }

    if (!hasValidPixels) {
        // Fallback: Fill with white
        for (int i = 0; i < width * height * 4; i += 4) {
            imageData[i] = 255; // R
            imageData[i + 1] = 255; // G
            imageData[i + 2] = 255; // B
            imageData[i + 3] = 255; // A
        }
    }

    return true;
}

void NavigationCube::setupGeometry() {
    m_orthoCamera->viewportMapping = SoOrthographicCamera::ADJUST_CAMERA;
    m_orthoCamera->nearDistance = 0.1f;
    m_orthoCamera->farDistance = 10.0f;
    m_orthoCamera->position.setValue(0.0f, 0.0f, 5.0f);
    m_orthoCamera->orientation.setValue(SbRotation::identity());
    m_root->addChild(m_orthoCamera);

    m_root->addChild(m_cameraTransform);
    updateCameraRotation();

    SoEnvironment* env = new SoEnvironment;
    env->ambientColor.setValue(1.0f, 1.0f, 1.0f);
    env->ambientIntensity.setValue(1.0f);
    m_root->addChild(env);

    SoDirectionalLight* mainLight = new SoDirectionalLight;
    mainLight->direction.setValue(0.5f, 0.5f, -1.0f);
    mainLight->intensity.setValue(0.8f);
    mainLight->color.setValue(1.0f, 1.0f, 1.0f);
    mainLight->on.setValue(true);
    m_root->addChild(mainLight);

    SoDirectionalLight* fillLight = new SoDirectionalLight;
    fillLight->direction.setValue(-0.3f, -0.3f, -0.5f);
    fillLight->intensity.setValue(0.8f);
    fillLight->color.setValue(0.9f, 0.9f, 1.0f);
    fillLight->on.setValue(true);
    m_root->addChild(fillLight);

    SoSeparator* cubeAssembly = new SoSeparator; // Parent for all faces

    SoMaterial* material = new SoMaterial;
    material->ambientColor.setValue(0.4f, 0.4f, 0.4f);
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    material->specularColor.setValue(0.6f, 0.6f, 0.6f);
    material->shininess.setValue(0.5f);
    material->emissiveColor.setValue(0.1f, 0.1f, 0.1f);
    cubeAssembly->addChild(material);

    SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
    texCoords->point.setValues(0, 4, new SbVec2f[4]{
        SbVec2f(0, 1), SbVec2f(1, 1), SbVec2f(1, 0), SbVec2f(0, 0)
    });
    cubeAssembly->addChild(texCoords); // Global texture coordinates for faces

    // Use DPI manager for optimal texture size
    auto& dpiManager = DPIManager::getInstance();
    int texSize = dpiManager.getScaledTextureSize(128);

    struct FaceDef {
        std::string description; // e.g., "Front Face"
        SbVec3f vertices[4];
        std::string textureKey;  // "F", "B", "T", "D", "L", "R"
    };

    float s = 0.5f; // half size of the cube

    std::vector<FaceDef> facesData = {
        {"Top Face (+Z)", {SbVec3f(-s, -s, s), SbVec3f(s, -s, s), SbVec3f(s, s, s), SbVec3f(-s, s, s)}, "T"},
        {"Bottom Face (-Z)",  {SbVec3f(s, -s, -s), SbVec3f(-s, -s, -s), SbVec3f(-s, s, -s), SbVec3f(s, s, -s)}, "D"},
        {"Right Face (-X)",  {SbVec3f(-s, -s, -s), SbVec3f(-s, -s, s), SbVec3f(-s, s, s), SbVec3f(-s, s, -s)}, "R"},
        {"Left Face (+X)", {SbVec3f(s, -s, s), SbVec3f(s, -s, -s), SbVec3f(s, s, -s), SbVec3f(s, s, s)}, "L"},
        {"Front Face (+Y)",   {SbVec3f(-s, s, s), SbVec3f(s, s, s), SbVec3f(s, s, -s), SbVec3f(-s, s, -s)}, "F"},
        {"Back Face (-Y)",{SbVec3f(-s, -s, -s), SbVec3f(s, -s, -s), SbVec3f(s, -s, s), SbVec3f(-s, -s, s)}, "B"}
    };
    
    bool applyTextures = true; 

    for (const auto& faceDef : facesData) {
        SoSeparator* faceSep = new SoSeparator;
        faceSep->setName(SbName(faceDef.textureKey.c_str())); // Name separator for picking, e.g., "F"

        SoTexture2* texture = new SoTexture2;
        
        // Use DPI manager for high-quality texture generation and caching
        auto textureInfo = dpiManager.getOrCreateScaledTexture(
            faceDef.textureKey, 
            128, // base texture size
            [&](unsigned char* data, int w, int h) -> bool {
                return generateFaceTexture(faceDef.textureKey, data, w, h);
            }
        );
        
        if (textureInfo) {
            texture->image.setValue(SbVec2s(textureInfo->width, textureInfo->height), 
                                  textureInfo->channels, textureInfo->data.get());
        } else {
            continue;
        }
        // texture->setName(faceDef.textureKey.c_str()); // Texture node itself can also be named if needed
        texture->model = SoTexture2::MODULATE;
        if (applyTextures) {
            faceSep->addChild(texture);
        }

        SoCoordinate3* coords = new SoCoordinate3;
        coords->point.setValues(0, 4, faceDef.vertices);
        faceSep->addChild(coords);

        SoFaceSet* faceSet = new SoFaceSet;
        faceSet->numVertices.setValue(4); // Single quad
        faceSep->addChild(faceSet);
        
        cubeAssembly->addChild(faceSep);
    }
    
    m_root->addChild(cubeAssembly);


    // Edges (remain as before, as a separate SoCube with LINES draw style)
    SoSeparator* edgeSep = new SoSeparator;
    SoDrawStyle* drawStyle = DPIAwareRendering::createDPIAwareGeometryStyle(2.0f, false);
    drawStyle->style = SoDrawStyle::LINES;
    edgeSep->addChild(drawStyle);

    SoMaterial* edgeMaterial = new SoMaterial;
    edgeMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f); 
    edgeMaterial->specularColor.setValue(1.0f, 1.0f, 1.0f);
    edgeMaterial->shininess.setValue(0.2f);
    edgeSep->addChild(edgeMaterial);

    SoCube* edgeCube = new SoCube; // This cube is for edges only
    edgeCube->width.setValue(1.0f);
    edgeCube->height.setValue(1.0f);
    edgeCube->depth.setValue(1.0f);
    edgeCube->setName("NavCubeEdges");
    edgeSep->addChild(edgeCube);
    m_root->addChild(edgeSep); // Add edges to the main root
}

void NavigationCube::updateCameraRotation() {
    float distance = 5.0f;
    float radX = m_rotationX * M_PI / 180.0f;
    float radY = m_rotationY * M_PI / 180.0f;

    float x = distance * sin(radY) * cos(radX);
    float y = distance * sin(radX);
    float z = distance * cos(radY) * cos(radX);

    m_orthoCamera->position.setValue(x, y, z);
    m_orthoCamera->pointAt(SbVec3f(0, 0, 0));
}

std::string NavigationCube::pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize) {
    SoRayPickAction pickAction(SbViewportRegion(viewportSize.x, viewportSize.y));
    pickAction.setPoint(mousePos);
    // pickAction.setRadius(1.0f); // Optional: Small radius for easier picking of edges/corners
    pickAction.apply(m_root);

    SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
    if (!pickedPoint) {
        return "";
    }

    SoPath* pickedPath = pickedPoint->getPath();
    if (!pickedPath || pickedPath->getLength() == 0) {
        return "";
    }

    // Iterate from the picked primitive up the path to find a named SoSeparator ("F", "B", "L", "R", "T", "D")
    for (int i = pickedPath->getLength() - 1; i >= 0; --i) {
        SoNode* currentNode = pickedPath->getNode(i);
        // Check if the node is an SoSeparator and has a name
        if (currentNode && currentNode->isOfType(SoSeparator::getClassTypeId()) && currentNode->getName().getLength() > 0) {
            std::string nameStr = currentNode->getName().getString();
            auto it = m_faceToView.find(nameStr); // m_faceToView maps "F" to "Front", etc.
            if (it != m_faceToView.end()) {
                return it->second; // Return "Front", "Back", etc.
            }
        }
    }
    
    // Fallback or specific handling for edges if NavCubeEdges is picked
    SoNode* pickedPrimitive = pickedPath->getTail();
    if (pickedPrimitive && pickedPrimitive->getName() == "NavCubeEdges") {
        // Here you could add logic to determine which edge or corner was hit,
        // possibly using pickedPoint->getNormal(), though it might be complex.
        // For now, clicking an edge won't trigger a view change via this path.
    }

    return "";
}

void NavigationCube::handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize) {
    if (!m_enabled) return;

    static float dragThreshold = 5.0f;
    static SbVec2s dragStartPos(0, 0);

    float x = static_cast<float>(event.GetX());
    float y = static_cast<float>(event.GetY());
    SbVec2s currentPos(static_cast<short>(x), static_cast<short>(y));


    if (event.GetEventType() == wxEVT_LEFT_DOWN) {
        m_isDragging = true;
        m_lastMousePos = currentPos;
        dragStartPos = m_lastMousePos;

        SbVec3f camPos = m_orthoCamera->position.getValue();
        float distance = camPos.length();
        m_rotationX = asinf(camPos[1] / distance) * 180.0f / M_PI;
        m_rotationY = atan2f(camPos[0], camPos[2]) * 180.0f / M_PI;

    }
    else if (event.GetEventType() == wxEVT_LEFT_UP && m_isDragging) {
        m_isDragging = false;
        SbVec2s delta = currentPos - dragStartPos;
        float distance = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
        if (distance < dragThreshold) {
            SbVec2s pickPos(static_cast<short>(x), static_cast<short>(viewportSize.y - y));
            std::string region = pickRegion(pickPos, viewportSize);
            if (!region.empty() && m_viewChangeCallback) {
                m_viewChangeCallback(region);
            }
        }
    }
    else if (event.GetEventType() == wxEVT_MOTION && m_isDragging) {
        wxLongLong currentTime = wxGetLocalTimeMillis();
        if (currentTime - m_lastDragTime < 16) return;
        m_lastDragTime = currentTime;

        SbVec2s delta = currentPos - m_lastMousePos;

        float sensitivity = 1.0f;
        m_rotationY += delta[0] * sensitivity;
        m_rotationX -= delta[1] * sensitivity;

        m_rotationX = std::max(-89.0f, std::min(89.0f, m_rotationX));

        updateCameraRotation();
        m_lastMousePos = currentPos;


        if (m_rotationChangedCallback) {
            m_rotationChangedCallback();
        }
    }
}

void NavigationCube::render(int x, int y, const wxSize& size) {
    SbViewportRegion viewport;
    viewport.setWindowSize(SbVec2s(static_cast<short>(m_windowWidth), static_cast<short>(m_windowHeight)));
    viewport.setViewportPixels(x, y, static_cast<int>(size.x * m_dpiScale), static_cast<int>(size.y * m_dpiScale));
    SoGLRenderAction renderAction(viewport);
    renderAction.setSmoothing(true);
    renderAction.setNumPasses(1);
    renderAction.setTransparencyType(SoGLRenderAction::BLEND);
    renderAction.apply(m_root);
}

void NavigationCube::setEnabled(bool enabled) {
    m_enabled = enabled;
    m_root->enableNotify(enabled);
}

void NavigationCube::setCameraPosition(const SbVec3f& position) {
    if (m_orthoCamera) {
        m_orthoCamera->position.setValue(position);
    }
}

void NavigationCube::setCameraOrientation(const SbRotation& orientation) {
    if (m_orthoCamera) {
        m_orthoCamera->orientation.setValue(orientation);
    }
}
