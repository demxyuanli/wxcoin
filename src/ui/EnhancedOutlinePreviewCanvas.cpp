#include "ui/EnhancedOutlinePreviewCanvas.h"

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
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbMatrix.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <vector>
#include <wx/log.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BEGIN_EVENT_TABLE(EnhancedOutlinePreviewCanvas, wxGLCanvas)
EVT_PAINT(EnhancedOutlinePreviewCanvas::onPaint)
EVT_SIZE(EnhancedOutlinePreviewCanvas::onSize)
EVT_ERASE_BACKGROUND(EnhancedOutlinePreviewCanvas::onEraseBackground)
EVT_MOUSE_EVENTS(EnhancedOutlinePreviewCanvas::onMouseEvent)
EVT_MOUSE_CAPTURE_LOST(EnhancedOutlinePreviewCanvas::onMouseCaptureLost)
EVT_IDLE(EnhancedOutlinePreviewCanvas::onIdle)
END_EVENT_TABLE()

EnhancedOutlinePreviewCanvas::EnhancedOutlinePreviewCanvas(wxWindow* parent, wxWindowID id, 
                                         const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, nullptr, pos, size, wxWANTS_CHARS) {
    
    // Create OpenGL context
    m_glContext = new wxGLContext(this);
    
    // Initialize the scene
    initializeScene();
    
    // Force initial paint to ensure OpenGL context is properly set up
    Refresh(false);
}

EnhancedOutlinePreviewCanvas::~EnhancedOutlinePreviewCanvas() {
    delete m_glContext;
    
    if (m_sceneRoot) {
        m_sceneRoot->unref();
    }
}

void EnhancedOutlinePreviewCanvas::initializeScene() {
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
    
    // Add lights
    setupLighting();
    
    // Create model root
    m_modelRoot = new SoSeparator;
    m_sceneRoot->addChild(m_modelRoot);
    
    // Create outline root (for geometry-based outlines)
    m_outlineRoot = new SoSeparator;
    m_sceneRoot->addChild(m_outlineRoot);
    
    // Create basic models
    createBasicModels();
    
    m_initialized = true;
    m_needsRedraw = true;
}

void EnhancedOutlinePreviewCanvas::setupLighting() {
    // Add main directional light
    SoDirectionalLight* mainLight = new SoDirectionalLight;
    mainLight->direction.setValue(-0.5f, -0.5f, -0.7f);
    mainLight->intensity = 0.8f;
    m_sceneRoot->addChild(mainLight);
    
    // Add fill light
    SoDirectionalLight* fillLight = new SoDirectionalLight;
    fillLight->direction.setValue(0.5f, 0.2f, -0.3f);
    fillLight->intensity = 0.3f;
    fillLight->color.setValue(0.9f, 0.9f, 1.0f);
    m_sceneRoot->addChild(fillLight);
}

void EnhancedOutlinePreviewCanvas::createBasicModels() {
    // Clear existing models
    m_modelRoot->removeAllChildren();
    m_outlineRoot->removeAllChildren();
    
    // Add rotation node for animation
    m_rotationNode = new SoRotationXYZ;
    m_rotationNode->axis = SoRotationXYZ::Y;
    m_rotationNode->angle = 0.0f;
    m_modelRoot->addChild(m_rotationNode);
    
    // Create model separator
    SoSeparator* modelSep = new SoSeparator;
    m_modelRoot->addChild(modelSep);
    
    // Enable back face culling for models
    SoShapeHints* shapeHints = new SoShapeHints;
    shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapeHints->shapeType = SoShapeHints::SOLID;
    modelSep->addChild(shapeHints);
    
    // Create objects
    createCube(modelSep);
    createSphere(modelSep);
    createCylinder(modelSep);
    createCone(modelSep);
}

void EnhancedOutlinePreviewCanvas::createCube(SoSeparator* parent) {
    SoSeparator* cubeSep = new SoSeparator;
    
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(-3.0f, 2.0f, 0);
    cubeSep->addChild(transform);
    
    // Material
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                  m_geomColor.Green() / 255.0f,
                                  m_geomColor.Blue() / 255.0f);
    material->specularColor.setValue(1.0f, 1.0f, 1.0f);
    material->shininess = 0.8f;
    cubeSep->addChild(material);
    
    SoCube* cube = new SoCube;
    cube->width = 2.0f;
    cube->height = 2.0f;
    cube->depth = 2.0f;
    cubeSep->addChild(cube);
    
    parent->addChild(cubeSep);
}

void EnhancedOutlinePreviewCanvas::createSphere(SoSeparator* parent) {
    SoSeparator* sphereSep = new SoSeparator;
    
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(3.0f, 2.0f, 0);
    sphereSep->addChild(transform);
    
    // Material
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                  m_geomColor.Green() / 255.0f,
                                  m_geomColor.Blue() / 255.0f);
    material->specularColor.setValue(1.0f, 1.0f, 1.0f);
    material->shininess = 0.8f;
    sphereSep->addChild(material);
    
    SoSphere* sphere = new SoSphere;
    sphere->radius = 1.5f;
    sphereSep->addChild(sphere);
    
    parent->addChild(sphereSep);
}

void EnhancedOutlinePreviewCanvas::createCylinder(SoSeparator* parent) {
    SoSeparator* cylSep = new SoSeparator;
    
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(-3.0f, -2.0f, 0);
    cylSep->addChild(transform);
    
    // Material
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                  m_geomColor.Green() / 255.0f,
                                  m_geomColor.Blue() / 255.0f);
    material->specularColor.setValue(1.0f, 1.0f, 1.0f);
    material->shininess = 0.8f;
    cylSep->addChild(material);
    
    SoCylinder* cylinder = new SoCylinder;
    cylinder->radius = 1.0f;
    cylinder->height = 3.0f;
    cylSep->addChild(cylinder);
    
    parent->addChild(cylSep);
}

void EnhancedOutlinePreviewCanvas::createCone(SoSeparator* parent) {
    SoSeparator* coneSep = new SoSeparator;
    
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(3.0f, -2.0f, 0);
    coneSep->addChild(transform);
    
    // Material
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                  m_geomColor.Green() / 255.0f,
                                  m_geomColor.Blue() / 255.0f);
    material->specularColor.setValue(1.0f, 1.0f, 1.0f);
    material->shininess = 0.8f;
    coneSep->addChild(material);
    
    SoCone* cone = new SoCone;
    cone->bottomRadius = 1.2f;
    cone->height = 2.5f;
    coneSep->addChild(cone);
    
    parent->addChild(coneSep);
}

void EnhancedOutlinePreviewCanvas::updateOutlineParams(const EnhancedOutlineParams& params) {
    m_outlineParams = params;
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setOutlineMethod(OutlineMethod method) {
    m_outlineMethod = method;
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::onPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    
    if (!m_initialized) {
        initializeScene();
    }
    
    render();
}

void EnhancedOutlinePreviewCanvas::onSize(wxSizeEvent& event) {
    // Update viewport
    if (m_camera) {
        wxSize size = GetClientSize();
        float aspect = (float)size.GetWidth() / (float)size.GetHeight();
        m_camera->aspectRatio = aspect;
    }
    
    m_needsRedraw = true;
    event.Skip();
}

void EnhancedOutlinePreviewCanvas::onEraseBackground(wxEraseEvent& WXUNUSED(event)) {
    // Do nothing to avoid flicker
}

void EnhancedOutlinePreviewCanvas::onMouseEvent(wxMouseEvent& event) {
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
        if (m_rotationNode) {
            m_rotationNode->angle = m_rotationNode->angle.getValue() + delta.x * 0.01f;
        }
        
        m_needsRedraw = true;
        Refresh(false);
    }
}

void EnhancedOutlinePreviewCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
    m_mouseDown = false;
}

void EnhancedOutlinePreviewCanvas::onIdle(wxIdleEvent& event) {
    if (m_needsRedraw) {
        Refresh(false);
        m_needsRedraw = false;
    }
}

void EnhancedOutlinePreviewCanvas::render() {
    if (!m_glContext || !m_sceneRoot) return;
    
    SetCurrent(*m_glContext);
    
    // Get viewport size
    wxSize size = GetClientSize();
    
    // Set background color
    glClearColor(m_bgColor.Red() / 255.0f, 
                m_bgColor.Green() / 255.0f, 
                m_bgColor.Blue() / 255.0f, 
                1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    
    // Set up viewport
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    SoGLRenderAction renderAction(viewport);
    
    // Render based on selected method
    switch (m_outlineMethod) {
        case OutlineMethod::INVERTED_HULL:
            renderInvertedHull(renderAction);
            break;
        case OutlineMethod::SCREEN_SPACE:
            renderScreenSpace(renderAction);
            break;
        case OutlineMethod::GEOMETRY_SILHOUETTE:
            renderGeometrySilhouette(renderAction);
            break;
        case OutlineMethod::STENCIL_BUFFER:
            renderStencilBuffer(renderAction);
            break;
        case OutlineMethod::MULTI_PASS:
            renderMultiPass(renderAction);
            break;
        default:
            renderBasic(renderAction);
            break;
    }
    
    SwapBuffers();
    m_needsRedraw = false;
}

void EnhancedOutlinePreviewCanvas::renderBasic(SoGLRenderAction& action) {
    // Just render the scene normally
    action.apply(m_sceneRoot);
}

void EnhancedOutlinePreviewCanvas::renderInvertedHull(SoGLRenderAction& action) {
    if (!m_outlineEnabled || m_outlineParams.edgeIntensity < 0.01f) {
        renderBasic(action);
        return;
    }
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Pass 1: Render enlarged back faces in outline color
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_BACK, GL_FILL);
    glDisable(GL_LIGHTING);
    
    // Set outline color
    glColor4f(m_outlineColor.Red() / 255.0f,
             m_outlineColor.Green() / 255.0f,
             m_outlineColor.Blue() / 255.0f,
             m_outlineParams.edgeIntensity);
    
    // Push matrix and scale
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    // Scale based on thickness
    float scaleFactor = 1.0f + m_outlineParams.thickness * 0.02f;
    glScalef(scaleFactor, scaleFactor, scaleFactor);
    
    // Render only the models
    action.apply(m_modelRoot);
    
    glPopMatrix();
    
    // Pass 2: Render the scene normally
    glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    
    action.apply(m_sceneRoot);
    
    glPopAttrib();
}

void EnhancedOutlinePreviewCanvas::renderScreenSpace(SoGLRenderAction& action) {
    if (!m_outlineEnabled || m_outlineParams.edgeIntensity < 0.01f) {
        renderBasic(action);
        return;
    }
    
    wxSize size = GetClientSize();
    int width = size.GetWidth();
    int height = size.GetHeight();
    
    // Allocate buffers
    std::vector<float> depthBuffer(width * height);
    std::vector<unsigned char> colorBuffer(width * height * 4);
    
    // Pass 1: Render scene and capture depth
    action.apply(m_sceneRoot);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depthBuffer.data());
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, colorBuffer.data());
    
    // Pass 2: Edge detection and outline rendering
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Draw pixels with edge detection
    glBegin(GL_POINTS);
    
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            // Simple Sobel edge detection on depth
            float center = depthBuffer[y * width + x];
            float right = depthBuffer[y * width + (x + 1)];
            float bottom = depthBuffer[(y + 1) * width + x];
            
            float dx = right - center;
            float dy = bottom - center;
            float edge = sqrt(dx * dx + dy * dy);
            
            // Check if it's an edge
            if (edge > m_outlineParams.depthThreshold) {
                glColor4f(m_outlineColor.Red() / 255.0f,
                         m_outlineColor.Green() / 255.0f,
                         m_outlineColor.Blue() / 255.0f,
                         m_outlineParams.edgeIntensity);
                glVertex2i(x, y);
            }
        }
    }
    
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

void EnhancedOutlinePreviewCanvas::renderGeometrySilhouette(SoGLRenderAction& action) {
    // First render the scene normally
    action.apply(m_sceneRoot);
    
    if (!m_outlineEnabled || m_outlineParams.edgeIntensity < 0.01f) {
        return;
    }
    
    // Extract silhouette edges and render them
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Setup for line rendering
    glDisable(GL_LIGHTING);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(m_outlineParams.thickness);
    glColor4f(m_outlineColor.Red() / 255.0f,
             m_outlineColor.Green() / 255.0f,
             m_outlineColor.Blue() / 255.0f,
             m_outlineParams.edgeIntensity);
    
    // Enable polygon offset to avoid z-fighting
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    
    // Here we would extract and render silhouette edges
    // For demo purposes, we'll render wireframe as approximation
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    action.apply(m_modelRoot);
    
    glPopAttrib();
}

void EnhancedOutlinePreviewCanvas::renderStencilBuffer(SoGLRenderAction& action) {
    if (!m_outlineEnabled || m_outlineParams.edgeIntensity < 0.01f) {
        renderBasic(action);
        return;
    }
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Pass 1: Render objects to stencil buffer
    glEnable(GL_STENCIL_TEST);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    
    action.apply(m_modelRoot);
    
    // Pass 2: Render scaled objects where stencil is 0
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    
    glDisable(GL_LIGHTING);
    glColor4f(m_outlineColor.Red() / 255.0f,
             m_outlineColor.Green() / 255.0f,
             m_outlineColor.Blue() / 255.0f,
             m_outlineParams.edgeIntensity);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    float scaleFactor = 1.0f + m_outlineParams.thickness * 0.02f;
    glScalef(scaleFactor, scaleFactor, scaleFactor);
    
    action.apply(m_modelRoot);
    
    glPopMatrix();
    
    // Pass 3: Render scene normally
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_LIGHTING);
    
    action.apply(m_sceneRoot);
    
    glPopAttrib();
}

void EnhancedOutlinePreviewCanvas::renderMultiPass(SoGLRenderAction& action) {
    if (!m_outlineEnabled || m_outlineParams.edgeIntensity < 0.01f) {
        renderBasic(action);
        return;
    }
    
    // Combine multiple techniques for best quality
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Pass 1: Inverted hull for outer silhouette
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDepthFunc(GL_LEQUAL);
    
    glColor4f(m_outlineColor.Red() / 255.0f,
             m_outlineColor.Green() / 255.0f,
             m_outlineColor.Blue() / 255.0f,
             m_outlineParams.edgeIntensity * 0.7f);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    float scaleFactor = 1.0f + m_outlineParams.thickness * 0.015f;
    glScalef(scaleFactor, scaleFactor, scaleFactor);
    
    action.apply(m_modelRoot);
    glPopMatrix();
    
    // Pass 2: Wireframe for internal edges
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(m_outlineParams.thickness * 0.5f);
    glEnable(GL_LINE_SMOOTH);
    
    glColor4f(m_outlineColor.Red() / 255.0f,
             m_outlineColor.Green() / 255.0f,
             m_outlineColor.Blue() / 255.0f,
             m_outlineParams.edgeIntensity * 0.3f);
    
    action.apply(m_modelRoot);
    
    // Pass 3: Normal rendering
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    glDepthFunc(GL_LESS);
    
    action.apply(m_sceneRoot);
    
    glPopAttrib();
}