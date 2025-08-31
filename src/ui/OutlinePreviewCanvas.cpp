#include "ui/OutlinePreviewCanvas.h"
#include "viewer/ImageOutlinePass2.h"

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
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoSceneTexture2.h>
#include <Inventor/nodes/SoShaderParameter.h>

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <GL/glx.h>
#endif

BEGIN_EVENT_TABLE(OutlinePreviewCanvas, wxGLCanvas)
EVT_PAINT(OutlinePreviewCanvas::onPaint)
EVT_SIZE(OutlinePreviewCanvas::onSize)
EVT_ERASE_BACKGROUND(OutlinePreviewCanvas::onEraseBackground)
EVT_LEFT_DOWN(OutlinePreviewCanvas::onMouseEvent)
EVT_LEFT_UP(OutlinePreviewCanvas::onMouseEvent)
EVT_MOTION(OutlinePreviewCanvas::onMouseEvent)
EVT_LEAVE_WINDOW(OutlinePreviewCanvas::onMouseEvent)
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
    if (m_glContext) {
        delete m_glContext;
    }
    
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
    
    // Create ImageOutlinePass2
    m_outlinePass = std::make_unique<ImageOutlinePass2>(this, m_modelRoot);
    m_outlinePass->setEnabled(m_outlineEnabled);
    m_outlinePass->setParams(m_outlineParams);
    
    m_initialized = true;
    m_needsRedraw = true;
}

void OutlinePreviewCanvas::createBasicModels() {
    // Add rotation node if it doesn't exist
    if (m_modelRoot->getNumChildren() == 0) {
        SoRotationXYZ* rotation = new SoRotationXYZ;
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
        sphere->radius = 0.8f;
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
        cylinder->radius = 0.6f;
        cylinder->height = 1.8f;
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
    
    if (m_outlinePass) {
        m_outlinePass->setParams(params);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

ImageOutlineParams OutlinePreviewCanvas::getOutlineParams() const {
    return m_outlineParams;
}

void OutlinePreviewCanvas::setOutlineEnabled(bool enabled) {
    m_outlineEnabled = enabled;
    
    if (m_outlinePass) {
        m_outlinePass->setEnabled(enabled);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

void OutlinePreviewCanvas::setGeometryColor(const wxColour& color) {
    m_geomColor = color;
    
    // Recreate models with new color
    if (m_modelRoot && m_initialized) {
        // Remove all children except the rotation node (first child)
        while (m_modelRoot->getNumChildren() > 1) {
            m_modelRoot->removeChild(1);
        }
        
        // Recreate basic models with new color
        createBasicModels();
    }
    
    m_needsRedraw = true;
}

void OutlinePreviewCanvas::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    if (!m_initialized) {
        initializeScene();
    }
    
    render();
}

void OutlinePreviewCanvas::onSize(wxSizeEvent& event) {
    if (m_camera) {
        wxSize size = GetClientSize();
        float aspect = (float)size.GetWidth() / (float)size.GetHeight();
        m_camera->aspectRatio = aspect;
    }
    
    m_needsRedraw = true;
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
        wxPoint pos = event.GetPosition();
        int dx = pos.x - m_lastMousePos.x;
        int dy = pos.y - m_lastMousePos.y;
        
        // Rotate the model
        if (m_modelRoot && m_modelRoot->getNumChildren() > 0) {
            SoNode* firstChild = m_modelRoot->getChild(0);
            if (firstChild->isOfType(SoRotationXYZ::getClassTypeId())) {
                SoRotationXYZ* rotation = (SoRotationXYZ*)firstChild;
                rotation->angle = rotation->angle.getValue() + dx * 0.01f;
            }
        }
        
        m_lastMousePos = pos;
        m_needsRedraw = true;
    } else if (event.GetEventType() == wxEVT_MOTION) {
        // Update hover state
        wxPoint screenPos = event.GetPosition();
        int newHoverIndex = getObjectAtPosition(screenPos);
        if (newHoverIndex != m_hoveredObjectIndex) {
            m_hoveredObjectIndex = newHoverIndex;
            m_needsRedraw = true;
        }
    } else if (event.GetEventType() == wxEVT_LEAVE_WINDOW) {
        // Mouse left the window
        if (m_hoveredObjectIndex != -1) {
            m_hoveredObjectIndex = -1;
            m_needsRedraw = true;
        }
    }
}

void OutlinePreviewCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    m_mouseDown = false;
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
    
    // Get viewport size
    wxSize size = GetClientSize();
    
    // Set background color
    glClearColor(m_bgColor.Red() / 255.0f, 
                m_bgColor.Green() / 255.0f, 
                m_bgColor.Blue() / 255.0f, 
                1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    
    // ImageOutlinePass2 will handle everything
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    SoGLRenderAction renderAction(viewport);
    renderAction.apply(m_sceneRoot);
    
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