#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "ui/OutlinePreviewCanvas.h"
#include "SceneManager.h"
#include "viewer/ImageOutlinePass.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoRotationXYZ.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include <GL/gl.h>

BEGIN_EVENT_TABLE(OutlinePreviewCanvas, wxGLCanvas)
EVT_PAINT(OutlinePreviewCanvas::onPaint)
EVT_SIZE(OutlinePreviewCanvas::onSize)
EVT_ERASE_BACKGROUND(OutlinePreviewCanvas::onEraseBackground)
EVT_LEFT_DOWN(OutlinePreviewCanvas::onMouseEvent)
EVT_LEFT_UP(OutlinePreviewCanvas::onMouseEvent)
EVT_MOTION(OutlinePreviewCanvas::onMouseEvent)
EVT_IDLE(OutlinePreviewCanvas::onIdle)
END_EVENT_TABLE()

static const int s_attribs[] = {
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_DEPTH_SIZE, 24,
    WX_GL_STENCIL_SIZE, 8,
    0
};

OutlinePreviewCanvas::OutlinePreviewCanvas(wxWindow* parent, wxWindowID id,
                                         const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, s_attribs, pos, size, 
                wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE) {
    
    // Set minimum size for preview
    SetMinSize(wxSize(300, 300));
}

OutlinePreviewCanvas::~OutlinePreviewCanvas() {
    if (m_glContext) {
        SetCurrent(*m_glContext);
        
        // Clean up scene graph
        if (m_sceneRoot) {
            m_sceneRoot->unref();
        }
        
        delete m_glContext;
    }
}

void OutlinePreviewCanvas::initializeScene() {
    if (m_initialized) return;
    
    // Create OpenGL context
    m_glContext = new wxGLContext(this);
    SetCurrent(*m_glContext);
    
    // Create scene graph
    m_sceneRoot = new SoSeparator;
    m_sceneRoot->ref();
    
    // Add camera
    m_camera = new SoPerspectiveCamera;
    m_camera->position.setValue(5.0f, 5.0f, 5.0f);
    m_camera->pointAt(SbVec3f(0, 0, 0));
    m_camera->nearDistance = 0.1f;
    m_camera->farDistance = 100.0f;
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
    
    // Note: We use a simplified outline rendering approach for the preview
    // This uses wireframe rendering with polygon offset to simulate outline effect
    
    m_initialized = true;
    m_needsRedraw = true;
}

void OutlinePreviewCanvas::createBasicModels() {
    // Material for all objects
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.7f, 0.7f, 0.7f);
    material->specularColor.setValue(1.0f, 1.0f, 1.0f);
    material->shininess = 0.8f;
    
    // Create a cube
    {
        SoSeparator* cubeSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(-2.0f, 0, 0);
        cubeSep->addChild(transform);
        cubeSep->addChild(material);
        SoCube* cube = new SoCube;
        cube->width = 1.5f;
        cube->height = 1.5f;
        cube->depth = 1.5f;
        cubeSep->addChild(cube);
        m_modelRoot->addChild(cubeSep);
    }
    
    // Create a sphere
    {
        SoSeparator* sphereSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(0, 0, 0);
        sphereSep->addChild(transform);
        sphereSep->addChild(material);
        SoSphere* sphere = new SoSphere;
        sphere->radius = 0.8f;
        sphereSep->addChild(sphere);
        m_modelRoot->addChild(sphereSep);
    }
    
    // Create a cylinder
    {
        SoSeparator* cylSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(2.0f, 0, 0);
        cylSep->addChild(transform);
        cylSep->addChild(material);
        SoCylinder* cylinder = new SoCylinder;
        cylinder->radius = 0.6f;
        cylinder->height = 1.8f;
        cylSep->addChild(cylinder);
        m_modelRoot->addChild(cylSep);
    }
    
    // Create a cone
    {
        SoSeparator* coneSep = new SoSeparator;
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(0, 0, -2.0f);
        coneSep->addChild(transform);
        coneSep->addChild(material);
        SoCone* cone = new SoCone;
        cone->bottomRadius = 0.7f;
        cone->height = 1.5f;
        coneSep->addChild(cone);
        m_modelRoot->addChild(coneSep);
    }
    
    // Add rotation animation
    SoRotationXYZ* rotation = new SoRotationXYZ;
    rotation->axis = SoRotationXYZ::Y;
    rotation->angle = 0.0f;
    m_modelRoot->insertChild(rotation, 0);
}

void OutlinePreviewCanvas::updateOutlineParams(const ImageOutlineParams& params) {
    m_outlineParams = params;
    if (m_outlinePass) {
        m_outlinePass->setParams(params);
        m_needsRedraw = true;
        Refresh(false);
    }
}

ImageOutlineParams OutlinePreviewCanvas::getOutlineParams() const {
    return m_outlineParams;
}

void OutlinePreviewCanvas::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    if (!m_initialized) {
        initializeScene();
    }
    
    render();
}

void OutlinePreviewCanvas::onSize(wxSizeEvent& event) {
    if (m_glContext && m_camera) {
        SetCurrent(*m_glContext);
        
        wxSize size = GetClientSize();
        glViewport(0, 0, size.GetWidth(), size.GetHeight());
        
        // Update camera aspect ratio
        SoPerspectiveCamera* perspCam = static_cast<SoPerspectiveCamera*>(m_camera);
        if (perspCam && size.GetHeight() > 0) {
            perspCam->aspectRatio = float(size.GetWidth()) / float(size.GetHeight());
        }
        
        m_needsRedraw = true;
    }
    
    event.Skip();
}

void OutlinePreviewCanvas::onEraseBackground(wxEraseEvent& event) {
    // Do nothing to avoid flicker
}

void OutlinePreviewCanvas::onMouseEvent(wxMouseEvent& event) {
    if (event.LeftDown()) {
        m_mouseDown = true;
        m_lastMousePos = event.GetPosition();
        CaptureMouse();
    } else if (event.LeftUp()) {
        m_mouseDown = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    } else if (event.Dragging() && m_mouseDown) {
        wxPoint currentPos = event.GetPosition();
        float dx = (currentPos.x - m_lastMousePos.x) * 0.01f;
        float dy = (currentPos.y - m_lastMousePos.y) * 0.01f;
        
        // Rotate models
        if (m_modelRoot && m_modelRoot->getNumChildren() > 0) {
            SoRotationXYZ* rotation = static_cast<SoRotationXYZ*>(m_modelRoot->getChild(0));
            if (rotation) {
                rotation->angle = rotation->angle.getValue() + dx;
                m_needsRedraw = true;
            }
        }
        
        m_lastMousePos = currentPos;
    }
}

void OutlinePreviewCanvas::onIdle(wxIdleEvent& event) {
    if (m_needsRedraw) {
        Refresh(false);
        event.RequestMore();
    }
}

void OutlinePreviewCanvas::render() {
    if (!m_glContext || !m_sceneRoot) return;
    
    SetCurrent(*m_glContext);
    
    // Clear background
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Get viewport size
    wxSize size = GetClientSize();
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    
    // Render scene with outline effect simulation
    // First pass: render with thick lines for outline effect
    if (m_outlineEnabled && m_outlineParams.edgeIntensity > 0.01f) {
        glEnable(GL_LINE_SMOOTH);
        glLineWidth(m_outlineParams.thickness * 2.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        
        // Set outline color based on intensity
        float intensity = m_outlineParams.edgeIntensity;
        glColor3f(0.0f, 0.0f, 0.0f); // Black outline
        
        SoGLRenderAction outlineAction(viewport);
        outlineAction.apply(m_modelRoot);
        
        glDisable(GL_POLYGON_OFFSET_LINE);
    }
    
    // Second pass: render filled objects
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthFunc(GL_LESS);
    
    SoGLRenderAction renderAction(viewport);
    renderAction.apply(m_sceneRoot);
    
    // Restore defaults
    glLineWidth(1.0f);
    glDisable(GL_LINE_SMOOTH);
    
    // Swap buffers
    SwapBuffers();
    
    m_needsRedraw = false;
}