#include "NavigationCube.h"
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDirectionalLight.h>
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

NavigationCube::NavigationCube(std::function<void(const std::string&)> viewChangeCallback)
    : m_root(new SoSeparator)
    , m_orthoCamera(new SoOrthographicCamera)
    , m_cameraTransform(new SoTransform)
    , m_enabled(true)
    , m_viewChangeCallback(viewChangeCallback)
    , m_isDragging(false)
    , m_lastMousePos(0, 0)
    , m_rotationX(0.0f)
    , m_rotationY(0.0f)
    , m_lastDragTime(0)
{
    m_root->ref();
    initialize();
}

NavigationCube::~NavigationCube() {
    m_root->unref();
}

void NavigationCube::initialize() {
    setupGeometry();

    // Map face names (texture labels) to view names
    m_faceToView = {
        { "F", "Front" },
        { "B", "Back" },
        { "L", "Left" },
        { "R", "Right" },
        { "T", "Top" },
        { "D", "Bottom" }
    };
}

void NavigationCube::generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height) {
    wxBitmap bitmap(width, height, 32);
    wxMemoryDC dc(bitmap);
    dc.SetBackground(wxColour(255, 255, 255, 0));
    dc.Clear();

    wxFont font(16, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    dc.SetFont(font);
    dc.SetTextForeground(*wxBLACK);

    wxSize textSize = dc.GetTextExtent(text);
    int x = (width - textSize.GetWidth()) / 2;
    int y = (height - textSize.GetHeight()) / 2;
    dc.DrawText(text, x, y);

    wxImage image = bitmap.ConvertToImage();
    unsigned char* src = image.GetData();
    bool hasAlpha = image.HasAlpha();
    unsigned char* alpha = hasAlpha ? image.GetAlpha() : nullptr;

    for (int i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
        imageData[i] = src[j];
        imageData[i + 1] = src[j + 1];
        imageData[i + 2] = src[j + 2];
        imageData[i + 3] = hasAlpha ? alpha[j / 3] : 255;
    }
}

void NavigationCube::setupGeometry() {
    m_orthoCamera->viewportMapping = SoOrthographicCamera::ADJUST_CAMERA;
    m_orthoCamera->nearDistance = 0.1f;
    m_orthoCamera->farDistance = 10.0f;
    m_root->addChild(m_orthoCamera);

    m_root->addChild(m_cameraTransform);
    updateCameraRotation();

    SoDirectionalLight* light = new SoDirectionalLight;
    light->direction.setValue(0, 0, -1);
    light->intensity.setValue(0.8f);
    light->color.setValue(1.0f, 1.0f, 1.0f);
    m_root->addChild(light);

    SoSeparator* cubeSep = new SoSeparator;
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    material->transparency.setValue(0.2f);
    cubeSep->addChild(material);

    SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
    texCoords->point.setValues(0, 4, new SbVec2f[4]{
        SbVec2f(0, 0), SbVec2f(1, 0), SbVec2f(1, 1), SbVec2f(0, 1)
        });
    cubeSep->addChild(texCoords);

    const char* faceNames[] = { "F", "B", "L", "R", "T", "D" };
    for (const auto& name : faceNames) {
        SoTexture2* texture = new SoTexture2;
        const int texSize = 64;
        unsigned char* imageData = new unsigned char[texSize * texSize * 4];
        generateFaceTexture(name, imageData, texSize, texSize);
        texture->image.setValue(SbVec2s(texSize, texSize), 4, imageData);
        texture->setName(name);
        cubeSep->addChild(texture);
        delete[] imageData; // Free memory to prevent leak
    }

    SoCube* cube = new SoCube;
    cube->width = 1.0f;
    cube->height = 1.0f;
    cube->depth = 1.0f;
    cube->setName("NavCube");
    cubeSep->addChild(cube);
    m_root->addChild(cubeSep);

    SoSeparator* edgeSep = new SoSeparator;
    SoDrawStyle* drawStyle = new SoDrawStyle;
    drawStyle->style = SoDrawStyle::LINES;
    drawStyle->lineWidth = 1.0f;
    edgeSep->addChild(drawStyle);

    SoMaterial* edgeMaterial = new SoMaterial;
    edgeMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.0f);
    edgeSep->addChild(edgeMaterial);

    SoCube* edgeCube = new SoCube;
    edgeCube->width = 1.0f;
    edgeCube->height = 1.0f;
    edgeCube->depth = 1.0f;
    edgeCube->setName("NavCubeEdges");
    edgeSep->addChild(edgeCube);
    m_root->addChild(edgeSep);
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
    pickAction.apply(m_root);

    SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
    if (!pickedPoint) return "";

    SoNode* pickedNode = pickedPoint->getPath()->getTail();
    if (pickedNode && pickedNode->getName().getLength() > 0) {
        std::string name = pickedNode->getName().getString();
        if (m_faceToView.find(name) != m_faceToView.end()) {
            LOG_INF("Picked node: " + name);
            return m_faceToView[name];
        }
    }

    if (pickedNode && (pickedNode->getName() == "NavCube" || pickedNode->getName() == "NavCubeEdges")) {
        SbVec3f normal = pickedPoint->getNormal();
        if (std::abs(normal[2] - 1.0f) < 0.1f) return m_faceToView["T"];    // Top
        if (std::abs(normal[2] + 1.0f) < 0.1f) return m_faceToView["D"];    // Bottom
        if (std::abs(normal[1] - 1.0f) < 0.1f) return m_faceToView["F"];    // Front
        if (std::abs(normal[1] + 1.0f) < 0.1f) return m_faceToView["B"];    // Back
        if (std::abs(normal[0] - 1.0f) < 0.1f) return m_faceToView["R"];    // Right
        if (std::abs(normal[0] + 1.0f) < 0.1f) return m_faceToView["L"];    // Left
    }

    return "";
}

void NavigationCube::handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize) {
    if (!m_enabled) return;

    static float dragThreshold = 5.0f;
    static SbVec2s dragStartPos(0, 0);

    if (event.GetEventType() == wxEVT_LEFT_DOWN) {
        m_isDragging = true;
        m_lastMousePos = SbVec2s(event.GetX(), viewportSize.y - event.GetY());
        dragStartPos = m_lastMousePos;
        LOG_INF("Navigation cube drag started");
    }
    else if (event.GetEventType() == wxEVT_LEFT_UP && m_isDragging) {
        m_isDragging = false;
        SbVec2s currentPos(event.GetX(), viewportSize.y - event.GetY());
        SbVec2s delta = currentPos - dragStartPos;
        float distance = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
        if (distance < dragThreshold) {
            std::string region = pickRegion(currentPos, viewportSize);
            if (!region.empty() && m_viewChangeCallback) {
                m_viewChangeCallback(region);
                LOG_INF("Navigation cube switched to view: " + region);
            }
        }
        LOG_INF("Navigation cube drag ended");
    }
    else if (event.GetEventType() == wxEVT_MOTION && m_isDragging) {
        wxLongLong currentTime = wxGetLocalTimeMillis();
        if (currentTime - m_lastDragTime < 16) return;
        m_lastDragTime = currentTime;

        SbVec2s currentPos(event.GetX(), viewportSize.y - event.GetY());
        SbVec2s delta = currentPos - m_lastMousePos;

        m_rotationY += delta[0] * 0.5f;
        m_rotationX += delta[1] * 0.5f;
        m_rotationX = std::max(-89.0f, std::min(89.0f, m_rotationX));

        updateCameraRotation();
        m_lastMousePos = currentPos;
        LOG_DBG("Navigation cube rotated: X=" + std::to_string(m_rotationX) + ", Y=" + std::to_string(m_rotationY));
    }
}

void NavigationCube::render(const wxSize& size) {
    SbViewportRegion viewport(size.x, size.y);
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