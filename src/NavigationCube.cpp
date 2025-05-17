#include "NavigationCube.h"
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
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLinear.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/time.h>
#include <cmath>
#include "Logger.h"

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
    if (!dc.IsOk()) {
        LOG_ERR("NavigationCube::generateFaceTexture: Failed to create wxMemoryDC for texture: " + text);
        return false;
    }

    dc.SetBackground(wxColour(255, 255, 255, 0));
    dc.Clear();

    int fontSize = static_cast<int>(16 * m_dpiScale);
    wxFont font(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    dc.SetFont(font);
    dc.SetTextForeground(*wxBLACK);

    wxSize textSize = dc.GetTextExtent(text);
    int x = (width - textSize.GetWidth()) / 2;
    int y = (height - textSize.GetHeight()) / 2;
    dc.DrawText(text, x, y);

    wxImage image = bitmap.ConvertToImage();
    if (!image.IsOk()) {
        LOG_ERR("NavigationCube::generateFaceTexture: Failed to convert bitmap to image for texture: " + text);
        return false;
    }
    image.InitAlpha();
    LOG_INF("NavigationCube::generateFaceTexture: Alpha channel enabled for " + text + ": " + std::to_string(image.HasAlpha()));

    unsigned char* src = image.GetData();
    bool hasAlpha = image.HasAlpha();
    unsigned char* alpha = hasAlpha ? image.GetAlpha() : nullptr;

    for (int i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
        imageData[i] = src[j];
        imageData[i + 1] = src[j + 1];
        imageData[i + 2] = src[j + 2];
        imageData[i + 3] = hasAlpha ? alpha[j / 3] : 255;
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
    LOG_INF("NavigationCube::setupGeometry: Added orthographic camera");

    m_root->addChild(m_cameraTransform);
    updateCameraRotation();

    SoEnvironment* env = new SoEnvironment;
    env->ambientColor.setValue(1.0f, 1.0f, 1.0f);
    env->ambientIntensity.setValue(0.3f);
    m_root->addChild(env);
    LOG_INF("NavigationCube::setupGeometry: Added environment light with intensity 0.3");

    SoDirectionalLight* mainLight = new SoDirectionalLight;
    mainLight->direction.setValue(0.5f, 0.5f, -1.0f);
    mainLight->intensity.setValue(0.8f);
    mainLight->color.setValue(1.0f, 1.0f, 1.0f);
    mainLight->on.setValue(true);
    m_root->addChild(mainLight);
    LOG_INF("NavigationCube::setupGeometry: Added main directional light from (0.5, 0.5, -1.0)");

    SoDirectionalLight* fillLight = new SoDirectionalLight;
    fillLight->direction.setValue(-0.3f, -0.3f, -0.5f);
    fillLight->intensity.setValue(0.4f);
    fillLight->color.setValue(0.9f, 0.9f, 1.0f);
    fillLight->on.setValue(true);
    m_root->addChild(fillLight);
    LOG_INF("NavigationCube::setupGeometry: Added fill directional light from (-0.3, -0.3, -0.5)");

    SoSeparator* cubeSep = new SoSeparator;
    SoMaterial* material = new SoMaterial;
    material->ambientColor.setValue(0.4f, 0.4f, 0.4f);
    // Debug: Use bright material to test rendering
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f); // Temporary white for debugging
    // material->diffuseColor.setValue(0.8f, 0.8f, 0.8f); // Original
    material->specularColor.setValue(0.6f, 0.6f, 0.6f);
    material->shininess.setValue(0.5f);
    material->emissiveColor.setValue(0.1f, 0.1f, 0.1f);
    cubeSep->addChild(material);
    LOG_INF("NavigationCube::setupGeometry: Added cube material");

    SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
    texCoords->point.setValues(0, 4, new SbVec2f[4]{
        SbVec2f(0, 0), SbVec2f(1, 0), SbVec2f(1, 1), SbVec2f(0, 1)
        });
    cubeSep->addChild(texCoords);

    int texSize = m_dpiScale > 1.5f ? 128 : 64;
    LOG_INF("NavigationCube::setupGeometry: Using texture size: " + std::to_string(texSize) + "x" + std::to_string(texSize));

    const char* faceNames[] = { "F", "B", "L", "R", "T", "D" };
    for (const auto& name : faceNames) {
        SoTexture2* texture = new SoTexture2;
        std::string cacheKey = std::string(name) + "_" + std::to_string(texSize);

        std::shared_ptr<TextureData> cachedTexture;
        auto it = s_textureCache.find(cacheKey);
        if (it != s_textureCache.end()) {
            cachedTexture = it->second;
            texture->image.setValue(SbVec2s(cachedTexture->width, cachedTexture->height), 4, cachedTexture->data);
            LOG_DBG("NavigationCube::setupGeometry: Using cached texture for: " + std::string(name));
        }
        else {
            unsigned char* imageData = new unsigned char[texSize * texSize * 4];
            if (!generateFaceTexture(name, imageData, texSize, texSize)) {
                LOG_ERR("NavigationCube::setupGeometry: Failed to generate texture for: " + std::string(name));
                memset(imageData, 0, texSize * texSize * 4);
                texture->image.setValue(SbVec2s(texSize, texSize), 4, imageData);
                LOG_INF("NavigationCube::setupGeometry: Applied default transparent texture for: " + std::string(name));
            }
            else {
                texture->image.setValue(SbVec2s(texSize, texSize), 4, imageData);
                s_textureCache[cacheKey] = std::make_shared<TextureData>(imageData, texSize, texSize);
                LOG_INF("NavigationCube::setupGeometry: Generated and cached texture for: " + std::string(name));
            }
        }

        texture->setName(name);
        // Debug: Comment out to disable textures and test material
        // cubeSep->addChild(texture);
    }

    SoCube* cube = new SoCube;
    cube->width.setValue(1.0f);
    cube->height.setValue(1.0f);
    cube->depth.setValue(1.0f);
    cube->setName("NavCube");
    cubeSep->addChild(cube);
    m_root->addChild(cubeSep);
    LOG_INF("NavigationCube::setupGeometry: Added cube geometry");

    SoSeparator* edgeSep = new SoSeparator;
    SoDrawStyle* drawStyle = new SoDrawStyle;
    drawStyle->style = SoDrawStyle::LINES;
    drawStyle->lineWidth = 1.0f;
    edgeSep->addChild(drawStyle);

    SoMaterial* edgeMaterial = new SoMaterial;
    edgeMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.8f);
    edgeMaterial->specularColor.setValue(0.5f, 0.5f, 1.0f);
    edgeMaterial->shininess.setValue(0.7f);
    edgeSep->addChild(edgeMaterial);
    LOG_INF("NavigationCube::setupGeometry: Added edge material");

    SoCube* edgeCube = new SoCube;
    edgeCube->width.setValue(1.0f);
    edgeCube->height.setValue(1.0f);
    edgeCube->depth.setValue(1.0f);
    edgeCube->setName("NavCubeEdges");
    edgeSep->addChild(edgeCube);
    m_root->addChild(edgeSep);
    LOG_INF("NavigationCube::setupGeometry: Added edge geometry");
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
    LOG_DBG("NavigationCube::updateCameraRotation: Camera position x=" + std::to_string(x) + ", y=" + std::to_string(y) + ", z=" + std::to_string(z));
}

std::string NavigationCube::pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize) {
    SoRayPickAction pickAction(SbViewportRegion(viewportSize.x, viewportSize.y));
    pickAction.setPoint(mousePos);
    pickAction.apply(m_root);

    SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
    if (!pickedPoint) {
        LOG_DBG("NavigationCube::pickRegion: No point picked at position (" +
            std::to_string(mousePos[0]) + ", " + std::to_string(mousePos[1]) + ")");
        return "";
    }

    SoNode* pickedNode = pickedPoint->getPath()->getTail();
    if (pickedNode && pickedNode->getName().getLength() > 0) {
        std::string name = pickedNode->getName().getString();
        if (m_faceToView.find(name) != m_faceToView.end()) {
            LOG_INF("NavigationCube::pickRegion: Picked node: " + name);
            return m_faceToView[name];
        }
    }

    if (pickedNode && (pickedNode->getName() == "NavCube" || pickedNode->getName() == "NavCubeEdges")) {
        SbVec3f normal = pickedPoint->getNormal();
        if (std::abs(normal[2] - 1.0f) < 0.1f) return m_faceToView["T"];
        if (std::abs(normal[2] + 1.0f) < 0.1f) return m_faceToView["D"];
        if (std::abs(normal[1] - 1.0f) < 0.1f) return m_faceToView["F"];
        if (std::abs(normal[1] + 1.0f) < 0.1f) return m_faceToView["B"];
        if (std::abs(normal[0] - 1.0f) < 0.1f) return m_faceToView["R"];
        if (std::abs(normal[0] + 1.0f) < 0.1f) return m_faceToView["L"];
    }

    LOG_DBG("NavigationCube::pickRegion: No valid **valid face picked");
    return "";
}

void NavigationCube::handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize) {
    if (!m_enabled) return;

    static float dragThreshold = 5.0f;
    static SbVec2s dragStartPos(0, 0);

    float x = static_cast<float>(event.GetX());
    float y = static_cast<float>(event.GetY());
    SbVec2s currentPos(static_cast<short>(x), static_cast<short>(y));

    LOG_DBG("NavigationCube::handleMouseEvent: Mouse at x=" + std::to_string(x) + ", y=" + std::to_string(y));

    if (event.GetEventType() == wxEVT_LEFT_DOWN) {
        m_isDragging = true;
        m_lastMousePos = currentPos;
        dragStartPos = m_lastMousePos;

        SbVec3f camPos = m_orthoCamera->position.getValue();
        float distance = camPos.length();
        m_rotationX = asinf(camPos[1] / distance) * 180.0f / M_PI;
        m_rotationY = atan2f(camPos[0], camPos[2]) * 180.0f / M_PI;

        LOG_INF("NavigationCube::handleMouseEvent: Drag started at x=" + std::to_string(m_lastMousePos[0]) +
            ", y=" + std::to_string(m_lastMousePos[1]) + ", rotationX=" + std::to_string(m_rotationX) +
            ", rotationY=" + std::to_string(m_rotationY));
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
                LOG_INF("NavigationCube::handleMouseEvent: Switched to view: " + region);
            }
        }
        LOG_INF("NavigationCube::handleMouseEvent: Drag ended");
    }
    else if (event.GetEventType() == wxEVT_MOTION && m_isDragging) {
        wxLongLong currentTime = wxGetLocalTimeMillis();
        if (currentTime - m_lastDragTime < 16) return;
        m_lastDragTime = currentTime;

        SbVec2s delta = currentPos - m_lastMousePos;

        float sensitivity = 0.5f;
        m_rotationY += delta[0] * sensitivity;
        m_rotationX -= delta[1] * sensitivity;

        m_rotationX = std::max(-89.0f, std::min(89.0f, m_rotationX));

        updateCameraRotation();
        m_lastMousePos = currentPos;

        LOG_DBG("NavigationCube::handleMouseEvent: Rotated: X=" + std::to_string(m_rotationX) +
            ", Y=" + std::to_string(m_rotationY) + ", deltaX=" + std::to_string(delta[0]) +
            ", deltaY=" + std::to_string(delta[1]));

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
        LOG_DBG("NavigationCube::setCameraPosition: Set camera position to x=" + std::to_string(position[0]) +
            ", y=" + std::to_string(position[1]) + ", z=" + std::to_string(position[2]));
    }
    else {
        LOG_WAR("NavigationCube::setCameraPosition: Camera not initialized");
    }
}

void NavigationCube::setCameraOrientation(const SbRotation& orientation) {
    if (m_orthoCamera) {
        m_orthoCamera->orientation.setValue(orientation);
        LOG_DBG("NavigationCube::setCameraOrientation: Set camera orientation");
    }
    else {
        LOG_WAR("NavigationCube::setCameraOrientation: Camera not initialized");
    }
}