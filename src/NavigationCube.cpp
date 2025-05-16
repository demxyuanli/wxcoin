#include "NavigationCube.h"
#include "Canvas.h"
#include "SceneManager.h"
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include "Logger.h"

NavigationCube::NavigationCube(Canvas* canvas)
    : m_canvas(canvas)
    , m_root(new SoSeparator)
    , m_orthoCamera(new SoOrthographicCamera)
    , m_enabled(true)
{
    m_root->ref();
    initialize();
}

NavigationCube::~NavigationCube() {
    m_root->unref();
}

void NavigationCube::initialize() {
    setupGeometry();
    setupInteraction();

    // Define standard view directions (direction, up vector)
    m_viewDirections["Top"] = { SbVec3f(0, 0, -1), SbVec3f(0, 1, 0) };
    m_viewDirections["Bottom"] = { SbVec3f(0, 0, 1), SbVec3f(0, 1, 0) };
    m_viewDirections["Front"] = { SbVec3f(0, -1, 0), SbVec3f(0, 0, 1) };
    m_viewDirections["Back"] = { SbVec3f(0, 1, 0), SbVec3f(0, 0, 1) };
    m_viewDirections["Left"] = { SbVec3f(-1, 0, 0), SbVec3f(0, 0, 1) };
    m_viewDirections["Right"] = { SbVec3f(1, 0, 0), SbVec3f(0, 0, 1) };
    m_viewDirections["Iso1"] = { SbVec3f(1, 1, 1), SbVec3f(0, 0, 1) }; // Isometric view
}

void NavigationCube::setupGeometry() {
    // Orthographic camera for 2D overlay
    m_orthoCamera->viewportMapping = SoOrthographicCamera::ADJUST_CAMERA;
    m_orthoCamera->position.setValue(0, 0, 5);
    m_orthoCamera->nearDistance = 0.1f;
    m_orthoCamera->farDistance = 10.0f;
    m_root->addChild(m_orthoCamera);

    // Position cube in top-right corner (normalized viewport coordinates)
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(0.8f, 0.8f, 0); // Adjust for top-right
    m_root->addChild(transform);

    // Cube geometry
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f); // Light gray
    material->transparency.setValue(0.4f);
    m_root->addChild(material);

    SoCube* cube = new SoCube;
    cube->width = 0.2f;
    cube->height = 0.2f;
    cube->depth = 0.2f;
    m_root->addChild(cube);

    // Labels for faces
    const char* labels[] = { "Top", "Front", "Right" };
    SbVec3f labelPositions[] = { SbVec3f(0, 0, 0.15f), SbVec3f(0, -0.15f, 0), SbVec3f(0.15f, 0, 0) };
    for (int i = 0; i < 3; ++i) {
        SoSeparator* labelSep = new SoSeparator;
        SoTranslation* trans = new SoTranslation;
        trans->translation = labelPositions[i];
        SoText2* text = new SoText2;
        text->string = labels[i];
        labelSep->addChild(trans);
        labelSep->addChild(text);
        m_root->addChild(labelSep);
    }
}

void NavigationCube::setupInteraction() {
    // Interaction handled via InputManager
}

void NavigationCube::handleMouseClick(const wxMouseEvent& event, const wxSize& viewportSize) {
    if (!this->m_enabled || event.GetEventType() != wxEVT_LEFT_DOWN) return;

    std::string region = this->pickRegion(SbVec2s(event.GetX(), viewportSize.y - event.GetY()), viewportSize);
    if (!region.empty()) {
        this->switchToView(region);
        m_canvas->Refresh(true);
    }
}

std::string NavigationCube::pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize) {
    SoRayPickAction pickAction(SbViewportRegion(viewportSize.x, viewportSize.y));
    pickAction.setPoint(mousePos);
    pickAction.apply(this->m_root);

    SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
    if (!pickedPoint) return "";

    // Simplified: Map picked geometry to region (extend with face detection logic)
    return "Top"; // TODO: Implement proper face/edge/corner mapping
}

void NavigationCube::switchToView(const std::string& region) {
    auto it = m_viewDirections.find(region);
    if (it == m_viewDirections.end()) return;

    SoCamera* camera = m_canvas->getCamera();
    if (!camera) {
        LOG_ERR("No camera available for view switch");
        return;
    }

    const auto& [direction, up] = it->second;
    SbVec3f currentPos = camera->position.getValue();
    float focalDistance = camera->focalDistance.getValue();

    // Set orientation to align with direction and up vector
    SbRotation rotation(SbVec3f(0, 0, -1), direction);
    camera->orientation.setValue(rotation);
    camera->position.setValue(currentPos + direction * focalDistance);
    camera->focalDistance.setValue(focalDistance);

    LOG_INF("Switched to view: " + region);
}

void NavigationCube::setEnabled(bool enabled) {
    this->m_enabled = enabled;
    this->m_root->enableNotify(enabled);
    m_canvas->Refresh(true);
}