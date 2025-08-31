#include "ui/OutlinePreviewCanvas.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoRotationXYZ.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SbViewportRegion.h>

#include <GL/gl.h>
#include <cmath>
#include <wx/log.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BEGIN_EVENT_TABLE(OutlinePreviewCanvas, wxGLCanvas)
EVT_PAINT(OutlinePreviewCanvas::onPaint)
EVT_SIZE(OutlinePreviewCanvas::onSize)
EVT_ERASE_BACKGROUND(OutlinePreviewCanvas::onEraseBackground)
EVT_MOUSE_EVENTS(OutlinePreviewCanvas::onMouseEvent)
EVT_MOUSE_CAPTURE_LOST(OutlinePreviewCanvas::onMouseCaptureLost)
EVT_IDLE(OutlinePreviewCanvas::onIdle)
END_EVENT_TABLE()

OutlinePreviewCanvas::OutlinePreviewCanvas(wxWindow* parent, wxWindowID id, 
                                         const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, nullptr, pos, size, wxWANTS_CHARS) {
    
    // Create OpenGL context
    m_glContext = new wxGLContext(this);
    
    // Initialize the scene
    initializeScene();
    
    // Force initial paint to ensure OpenGL context is properly set up
    Refresh(false);
}

OutlinePreviewCanvas::~OutlinePreviewCanvas() {
    delete m_glContext;
    
    if (m_sceneRoot) {
        m_sceneRoot->unref();
    }
}

void OutlinePreviewCanvas::initializeScene() {
    if (!m_glContext) return;
    
    SetCurrent(*m_glContext);
    
    // Create scene root
    m_sceneRoot = new SoSeparator;
    m_sceneRoot->ref();
    
    // Create and setup camera
    m_camera = new SoPerspectiveCamera;
    m_camera->position.setValue(0.0f, 0.0f, 15.0f);
    m_camera->nearDistance = 0.1f;
    m_camera->farDistance = 1000.0f;
    m_sceneRoot->addChild(m_camera);
    
    // Add light
    SoDirectionalLight* light = new SoDirectionalLight;
    light->direction.setValue(-0.5f, -0.5f, -0.7f);
    m_sceneRoot->addChild(light);
    
    // Create model root
    m_modelRoot = new SoSeparator;
    m_sceneRoot->addChild(m_modelRoot);
    
    // Create basic models
    createBasicModels();
    
    m_initialized = true;
    m_needsRedraw = true;
}

void OutlinePreviewCanvas::createBasicModels() {
    // Clear existing models
    m_modelRoot->removeAllChildren();
    
    // Add rotation node for animation
    SoRotationXYZ* rotation = nullptr;
    for (int i = 0; i < m_modelRoot->getNumChildren(); i++) {
        if (m_modelRoot->getChild(i)->isOfType(SoRotationXYZ::getClassTypeId())) {
            rotation = static_cast<SoRotationXYZ*>(m_modelRoot->getChild(i));
            break;
        }
    }
    if (!rotation) {
        rotation = new SoRotationXYZ;
        rotation->axis = SoRotationXYZ::Y;
        rotation->angle = 0.0f;
        m_modelRoot->addChild(rotation);
    }
    
    // Create a cube
    {
        SoSeparator* cubeSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(-2.0f, 2.0f, 0);
        cubeSep->addChild(transform);
        
        // Material with configured color
        SoMaterial* cubeMaterial = new SoMaterial;
        cubeMaterial->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                          m_geomColor.Green() / 255.0f,
                                          m_geomColor.Blue() / 255.0f);
        cubeMaterial->specularColor.setValue(1.0f, 1.0f, 1.0f);
        cubeMaterial->shininess = 0.8f;
        cubeSep->addChild(cubeMaterial);
        
        SoCube* cube = new SoCube;
        cube->width = 2.0f;
        cube->height = 2.0f;
        cube->depth = 2.0f;
        cubeSep->addChild(cube);
        m_modelRoot->addChild(cubeSep);
    }
    
    // Create a sphere
    {
        SoSeparator* sphereSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(2.0f, 2.0f, 0);
        sphereSep->addChild(transform);
        
        // Material with configured color
        SoMaterial* sphereMaterial = new SoMaterial;
        sphereMaterial->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                            m_geomColor.Green() / 255.0f,
                                            m_geomColor.Blue() / 255.0f);
        sphereMaterial->specularColor.setValue(1.0f, 1.0f, 1.0f);
        sphereMaterial->shininess = 0.8f;
        sphereSep->addChild(sphereMaterial);
        
        SoSphere* sphere = new SoSphere;
        sphere->radius = 1.2f;
        sphereSep->addChild(sphere);
        m_modelRoot->addChild(sphereSep);
    }
    
    // Create a cylinder
    {
        SoSeparator* cylSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(-2.0f, -2.0f, 0);
        cylSep->addChild(transform);
        
        // Material with configured color
        SoMaterial* cylMaterial = new SoMaterial;
        cylMaterial->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                         m_geomColor.Green() / 255.0f,
                                         m_geomColor.Blue() / 255.0f);
        cylMaterial->specularColor.setValue(1.0f, 1.0f, 1.0f);
        cylMaterial->shininess = 0.8f;
        cylSep->addChild(cylMaterial);
        
        SoCylinder* cylinder = new SoCylinder;
        cylinder->radius = 0.8f;
        cylinder->height = 2.5f;
        cylSep->addChild(cylinder);
        m_modelRoot->addChild(cylSep);
    }
    
    // Create a cone
    {
        SoSeparator* coneSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(2.0f, -2.0f, 0);
        coneSep->addChild(transform);
        
        // Material with configured color
        SoMaterial* coneMaterial = new SoMaterial;
        coneMaterial->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                          m_geomColor.Green() / 255.0f,
                                          m_geomColor.Blue() / 255.0f);
        coneMaterial->specularColor.setValue(1.0f, 1.0f, 1.0f);
        coneMaterial->shininess = 0.8f;
        coneSep->addChild(coneMaterial);
        
        SoCone* cone = new SoCone;
        cone->bottomRadius = 0.7f;
        cone->height = 1.5f;
        coneSep->addChild(cone);
        m_modelRoot->addChild(coneSep);
    }
}

void OutlinePreviewCanvas::updateOutlineParams(const ImageOutlineParams& params) {
    m_outlineParams = params;
    m_needsRedraw = true;
    Refresh(false);
}

ImageOutlineParams OutlinePreviewCanvas::getOutlineParams() const {
    return m_outlineParams;
}

void OutlinePreviewCanvas::setOutlineEnabled(bool enabled) {
    m_outlineEnabled = enabled;
    m_needsRedraw = true;
    Refresh(false);
}

void OutlinePreviewCanvas::setGeometryColor(const wxColour& color) {
    m_geomColor = color;
    
    // Recreate models with new color
    createBasicModels();
    
    m_needsRedraw = true;
    Refresh(false);
}

void OutlinePreviewCanvas::onPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    
    if (!m_initialized) {
        initializeScene();
    }
    
    render();
}

void OutlinePreviewCanvas::onSize(wxSizeEvent& event) {
    // Update viewport
    if (m_camera) {
        wxSize size = GetClientSize();
        float aspect = (float)size.GetWidth() / (float)size.GetHeight();
        m_camera->aspectRatio = aspect;
    }
    
    m_needsRedraw = true;
    event.Skip();
}

void OutlinePreviewCanvas::onEraseBackground(wxEraseEvent& WXUNUSED(event)) {
    // Do nothing to avoid flicker
}

void OutlinePreviewCanvas::onMouseEvent(wxMouseEvent& event) {
    if (event.LeftDown()) {
        m_mouseDown = true;
        m_lastMousePos = event.GetPosition();
        CaptureMouse();
    }
    else if (event.LeftUp()) {
        m_mouseDown = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
    else if (event.Dragging() && m_mouseDown) {
        wxPoint delta = event.GetPosition() - m_lastMousePos;
        m_lastMousePos = event.GetPosition();
        
        // Rotate model
        if (m_modelRoot && m_modelRoot->getNumChildren() > 0) {
            // Find rotation node
            for (int i = 0; i < m_modelRoot->getNumChildren(); i++) {
                if (m_modelRoot->getChild(i)->isOfType(SoRotationXYZ::getClassTypeId())) {
                    SoRotationXYZ* rotation = static_cast<SoRotationXYZ*>(m_modelRoot->getChild(i));
                    rotation->angle = rotation->angle.getValue() + delta.x * 0.01f;
                    break;
                }
            }
        }
        
        m_needsRedraw = true;
        Refresh(false);
    }
    else if (event.Moving()) {
        // Update hover state
        int prevHovered = m_hoveredObjectIndex;
        m_hoveredObjectIndex = getObjectAtPosition(event.GetPosition());
        
        if (prevHovered != m_hoveredObjectIndex) {
            m_needsRedraw = true;
            Refresh(false);
        }
    }
}

void OutlinePreviewCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
    m_mouseDown = false;
}

void OutlinePreviewCanvas::onIdle(wxIdleEvent& event) {
    if (m_needsRedraw) {
        Refresh(false);
        m_needsRedraw = false;
    }
}

void OutlinePreviewCanvas::render() {
    if (!m_glContext || !m_sceneRoot) return;
    
    SetCurrent(*m_glContext);
    
    // Get viewport size
    wxSize size = GetClientSize();
    
    // Set background color
    glClearColor(m_bgColor.Red() / 255.0f, 
                m_bgColor.Green() / 255.0f, 
                m_bgColor.Blue() / 255.0f, 
                1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    
    // Set up viewport and render
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    SoGLRenderAction renderAction(viewport);
    
    // First pass: render the scene normally
    renderAction.apply(m_sceneRoot);
    
    // Second pass: render outline if enabled
    if (m_outlineEnabled && m_outlineParams.edgeIntensity > 0.01f) {
        // Use 2-pass silhouette rendering
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        
        // Pass 1: Render back faces, scaled up, in black
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_BACK, GL_FILL);
        glDisable(GL_LIGHTING);
        
        // Push matrix and scale
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        
        // Get the center of the scene
        glTranslatef(0, 0, 0);
        float scaleFactor = 1.0f + m_outlineParams.thickness * 0.02f;
        glScalef(scaleFactor, scaleFactor, scaleFactor);
        
        // Set outline color
        if (m_hoveredObjectIndex >= 0) {
            // Orange for hover
            glColor4f(m_hoverColor.Red() / 255.0f,
                     m_hoverColor.Green() / 255.0f,
                     m_hoverColor.Blue() / 255.0f,
                     m_outlineParams.edgeIntensity);
        } else {
            // Black for normal outline
            glColor4f(m_outlineColor.Red() / 255.0f,
                     m_outlineColor.Green() / 255.0f,
                     m_outlineColor.Blue() / 255.0f,
                     m_outlineParams.edgeIntensity);
        }
        
        // Render only the models (not the whole scene)
        renderAction.apply(m_modelRoot);
        
        glPopMatrix();
        glPopAttrib();
    }
    
    SwapBuffers();
    m_needsRedraw = false;
}

int OutlinePreviewCanvas::getObjectAtPosition(const wxPoint& pos) {
    // Simple position-based detection for demo
    wxSize size = GetClientSize();
    int halfWidth = size.GetWidth() / 2;
    int halfHeight = size.GetHeight() / 2;
    
    // Determine which quadrant the mouse is in
    if (pos.x < halfWidth && pos.y < halfHeight) {
        return 1; // Top-left: cube
    } else if (pos.x >= halfWidth && pos.y < halfHeight) {
        return 2; // Top-right: sphere
    } else if (pos.x < halfWidth && pos.y >= halfHeight) {
        return 3; // Bottom-left: cylinder
    } else {
        return 4; // Bottom-right: cone
    }
}