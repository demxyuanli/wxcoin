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
#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <sstream>
// #include <wx/log.h>  // Removed: no logging needed
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenGL function pointers
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM2FPROC glUniform2f = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

// Platform specific includes for extension loading
#ifndef _WIN32
    #include <GL/glx.h>
#endif

// Function to load OpenGL extensions
void loadOpenGLExtensions() {
#ifdef _WIN32
    #define GET_PROC(name) name = (decltype(name))wglGetProcAddress(#name); \
                          if (!name) name = (decltype(name))wglGetProcAddress(#name"ARB"); \
                          if (!name) name = (decltype(name))wglGetProcAddress(#name"EXT")
#else
    #define GET_PROC(name) name = (decltype(name))glXGetProcAddress((const GLubyte*)#name); \
                          if (!name) name = (decltype(name))glXGetProcAddress((const GLubyte*)#name"ARB"); \
                          if (!name) name = (decltype(name))glXGetProcAddress((const GLubyte*)#name"EXT")
#endif
    
    GET_PROC(glGenFramebuffers);
    GET_PROC(glDeleteFramebuffers);
    GET_PROC(glBindFramebuffer);
    GET_PROC(glFramebufferTexture2D);
    GET_PROC(glCheckFramebufferStatus);
    GET_PROC(glCreateShader);
    GET_PROC(glDeleteShader);
    GET_PROC(glShaderSource);
    GET_PROC(glCompileShader);
    GET_PROC(glGetShaderiv);
    GET_PROC(glGetShaderInfoLog);
    GET_PROC(glCreateProgram);
    GET_PROC(glDeleteProgram);
    GET_PROC(glAttachShader);
    GET_PROC(glLinkProgram);
    GET_PROC(glGetProgramiv);
    GET_PROC(glGetProgramInfoLog);
    GET_PROC(glUseProgram);
    GET_PROC(glBindAttribLocation);
    GET_PROC(glGetUniformLocation);
    GET_PROC(glUniform1i);
    GET_PROC(glUniform1f);
    GET_PROC(glUniform2f);
    GET_PROC(glGenVertexArrays);
    GET_PROC(glDeleteVertexArrays);
    GET_PROC(glBindVertexArray);
    GET_PROC(glGenBuffers);
    GET_PROC(glDeleteBuffers);
    GET_PROC(glBindBuffer);
    GET_PROC(glBufferData);
    GET_PROC(glEnableVertexAttribArray);
    GET_PROC(glDisableVertexAttribArray);
    GET_PROC(glVertexAttribPointer);
    GET_PROC(glActiveTexture);
    
#undef GET_PROC
}

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
        
        // Clean up resources
        cleanupFBO();
        cleanupShaders();
        
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
    
    // Load OpenGL extensions
    static bool extensionsLoaded = false;
    if (!extensionsLoaded) {
        loadOpenGLExtensions();
        extensionsLoaded = true;
        
        // Check critical functions silently
        if (!glGenFramebuffers || !glBindFramebuffer) {
            // FBO functions not available
        }
        if (!glCreateShader || !glUseProgram) {
            // Shader functions not available
        }
    }
    
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
    
    // Initialize FBO and shaders for proper outline rendering
    initializeShaders();
    
    // FBO will be initialized in onSize event
    // But try to initialize now if we have a size
    wxSize size = GetClientSize();
    if (size.GetWidth() > 0 && size.GetHeight() > 0) {
        initializeFBO(size.GetWidth(), size.GetHeight());
    }
    
    // Check OpenGL version (silently)
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    
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
    m_needsRedraw = true;
    Refresh(false);
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
        
        // Reinitialize FBO with new size
        if (size.GetWidth() > 0 && size.GetHeight() > 0) {
            initializeFBO(size.GetWidth(), size.GetHeight());
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
    } else if (event.Moving()) {
        // Check which object is under mouse
        wxPoint pos = event.GetPosition();
        int oldHovered = m_hoveredObjectIndex;
        m_hoveredObjectIndex = getObjectAtPosition(pos);
        
        if (oldHovered != m_hoveredObjectIndex) {
            m_needsRedraw = true;
        }
    } else if (event.Leaving()) {
        // Mouse left the window
        if (m_hoveredObjectIndex != -1) {
            m_hoveredObjectIndex = -1;
            m_needsRedraw = true;
        }
    }
}

void OutlinePreviewCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    // Handle mouse capture lost
    m_mouseDown = false;
    // No need to call ReleaseMouse() here as the capture is already lost
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
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    
    // If FBO or shaders not ready, use simple rendering
    bool useFBO = m_fbo && m_normalShader && m_outlineShader && glGenFramebuffers && glUseProgram;
    
    // For now, disable FBO rendering to avoid issues
    // TODO: Fix FBO implementation
    useFBO = false;
    
    if (!useFBO) {
        // Simple fallback rendering
        glViewport(0, 0, size.GetWidth(), size.GetHeight());
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_DEPTH_TEST);
        
        // Outline rendering using silhouette edges
        if (m_outlineEnabled && m_outlineParams.edgeIntensity > 0.01f) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            
            // Step 1: Render scaled-up black/orange silhouettes
            glClear(GL_DEPTH_BUFFER_BIT);  // Clear depth to ensure outline is behind
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);  // Show back faces only
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_LIGHTING);
            
            // Use slight offset to avoid z-fighting
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
            
            // Render all objects slightly larger
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            
            // Scale from center of scene
            float scale = 1.0f + (m_outlineParams.thickness * 0.005f);
            glScalef(scale, scale, scale);
            
            // Check hover state
            bool hasHover = (m_hoveredObjectIndex >= 1 && m_hoveredObjectIndex <= 4);
            
            if (hasHover) {
                // Render non-hovered objects in black
                for (int i = 1; i <= 4 && m_modelRoot && i < m_modelRoot->getNumChildren(); i++) {
                    if (i == m_hoveredObjectIndex) continue;  // Skip hovered
                    
                    glColor3f(0.0f, 0.0f, 0.0f);  // Black
                    
                    // Create temp scene
                    SoSeparator* temp = new SoSeparator;
                    temp->ref();
                    
                    // Add transforms
                    if (m_modelRoot->getNumChildren() > 0 && 
                        m_modelRoot->getChild(0)->isOfType(SoRotationXYZ::getClassTypeId())) {
                        temp->addChild(m_modelRoot->getChild(0));
                    }
                    
                    SoTransform* trans = new SoTransform;
                    switch(i) {
                        case 1: trans->translation.setValue(-2.0f, 2.0f, 0.0f); break;
                        case 2: trans->translation.setValue(2.0f, 2.0f, 0.0f); break;
                        case 3: trans->translation.setValue(-2.0f, -2.0f, 0.0f); break;
                        case 4: trans->translation.setValue(2.0f, -2.0f, 0.0f); break;
                    }
                    temp->addChild(trans);
                    temp->addChild(m_modelRoot->getChild(i));
                    
                    SoGLRenderAction action(viewport);
                    action.apply(temp);
                    temp->unref();
                }
                
                // Render hovered object in orange
                if (m_hoveredObjectIndex <= m_modelRoot->getNumChildren()) {
                    glColor3f(1.0f, 0.5f, 0.0f);  // Orange
                    
                    SoSeparator* temp = new SoSeparator;
                    temp->ref();
                    
                    if (m_modelRoot->getChild(0)->isOfType(SoRotationXYZ::getClassTypeId())) {
                        temp->addChild(m_modelRoot->getChild(0));
                    }
                    
                    SoTransform* trans = new SoTransform;
                    switch(m_hoveredObjectIndex) {
                        case 1: trans->translation.setValue(-2.0f, 2.0f, 0.0f); break;
                        case 2: trans->translation.setValue(2.0f, 2.0f, 0.0f); break;
                        case 3: trans->translation.setValue(-2.0f, -2.0f, 0.0f); break;
                        case 4: trans->translation.setValue(2.0f, -2.0f, 0.0f); break;
                    }
                    temp->addChild(trans);
                    temp->addChild(m_modelRoot->getChild(m_hoveredObjectIndex));
                    
                    SoGLRenderAction action(viewport);
                    action.apply(temp);
                    temp->unref();
                }
            } else {
                // No hover - all black
                glColor3f(0.0f, 0.0f, 0.0f);
                SoGLRenderAction action(viewport);
                action.apply(m_modelRoot);
            }
            
            glPopMatrix();
            
            // Reset state
            glDisable(GL_POLYGON_OFFSET_FILL);
            glCullFace(GL_BACK);
            
            glPopAttrib();
        }
        
        // Step 2: Render the scene normally on top
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        SoGLRenderAction renderAction(viewport);
        renderAction.apply(m_sceneRoot);
        
        SwapBuffers();
        m_needsRedraw = false;
        return;
    }
    
    if (m_outlineEnabled && m_outlineParams.edgeIntensity > 0.01f) {
        // Pass 1: Render to FBO with normal shader
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        
        // Attach normal texture as color attachment
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_normalTexture, 0);
        
        glViewport(0, 0, m_fboWidth, m_fboHeight);
        glClearColor(0.5f, 0.5f, 1.0f, 1.0f); // Default normal pointing to camera
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Use normal shader
        glUseProgram(m_normalShader);
        
        // Render scene with normal shader
        SoGLRenderAction normalAction(viewport);
        normalAction.apply(m_sceneRoot);
        
        // Pass 2: Render scene normally to color attachment
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
        
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(0); // Use default shader
        
        // Render scene normally
        SoGLRenderAction colorAction(viewport);
        colorAction.apply(m_sceneRoot);
        
        // Pass 3: Apply outline post-processing
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, size.GetWidth(), size.GetHeight());
        
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Use outline shader
        glUseProgram(m_outlineShader);
        
        // Set uniforms
        glUniform1i(glGetUniformLocation(m_outlineShader, "uColorTexture"), 0);
        glUniform1i(glGetUniformLocation(m_outlineShader, "uNormalTexture"), 1);
        glUniform2f(glGetUniformLocation(m_outlineShader, "uResolution"), 
                    float(m_fboWidth), float(m_fboHeight));
        glUniform1f(glGetUniformLocation(m_outlineShader, "uThickness"), 
                    m_outlineParams.thickness);
        glUniform1f(glGetUniformLocation(m_outlineShader, "uIntensity"), 
                    m_outlineParams.edgeIntensity);
        
        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_normalTexture);
        
        // Render fullscreen quad
        if (m_quadVAO) {
            glBindVertexArray(m_quadVAO);
        } else {
            // Manual vertex attribute setup if VAO not supported
            glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        }
        
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        if (m_quadVAO) {
            glBindVertexArray(0);
        } else {
            glDisableVertexAttribArray(0);
        }
        
        // Cleanup
        glUseProgram(0);
        glActiveTexture(GL_TEXTURE0);
    } else {
        // No outline - just render normally
        glViewport(0, 0, size.GetWidth(), size.GetHeight());
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        SoGLRenderAction renderAction(viewport);
        renderAction.apply(m_sceneRoot);
    }
    
    // Swap buffers
    SwapBuffers();
    
    m_needsRedraw = false;
}

// Shader sources
static const char* g_normalVertexShader = R"GLSL(
varying vec3 vNormal;
varying vec3 vPosition;

void main() {
    vNormal = gl_Normal;
    vPosition = gl_Vertex.xyz;
    gl_Position = ftransform();
}
)GLSL";

static const char* g_normalFragmentShader = R"GLSL(
varying vec3 vNormal;
varying vec3 vPosition;

void main() {
    vec3 normal = normalize(vNormal);
    gl_FragColor = vec4(normal * 0.5 + 0.5, 1.0);
}
)GLSL";

static const char* g_outlineVertexShader = R"GLSL(
attribute vec2 aPosition;
varying vec2 vTexCoord;

void main() {
    vTexCoord = aPosition * 0.5 + 0.5;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)GLSL";

static const char* g_outlineFragmentShader = R"GLSL(
uniform sampler2D uColorTexture;
uniform sampler2D uNormalTexture;
uniform vec2 uResolution;
uniform float uThickness;
uniform float uIntensity;

varying vec2 vTexCoord;

void main() {
    vec2 texelSize = 1.0 / uResolution;
    vec4 color = texture2D(uColorTexture, vTexCoord);
    
    // Simple edge detection on normal texture
    vec2 offset = texelSize * uThickness;
    vec3 center = texture2D(uNormalTexture, vTexCoord).rgb;
    vec3 top = texture2D(uNormalTexture, vTexCoord + vec2(0.0, -offset.y)).rgb;
    vec3 right = texture2D(uNormalTexture, vTexCoord + vec2(offset.x, 0.0)).rgb;
    vec3 bottom = texture2D(uNormalTexture, vTexCoord + vec2(0.0, offset.y)).rgb;
    vec3 left = texture2D(uNormalTexture, vTexCoord + vec2(-offset.x, 0.0)).rgb;
    
    float d1 = length(center - top);
    float d2 = length(center - right);
    float d3 = length(center - bottom);
    float d4 = length(center - left);
    
    float edge = max(max(d1, d2), max(d3, d4)) * uIntensity * 2.0;
    edge = clamp(edge, 0.0, 1.0);
    
    // Apply outline
    vec3 outlineColor = vec3(0.0, 0.0, 0.0);
    gl_FragColor = vec4(mix(color.rgb, outlineColor, edge), 1.0);
}
)GLSL";

void OutlinePreviewCanvas::initializeFBO(int width, int height) {
    static int lastWidth = 0, lastHeight = 0;
    
    // Only track if size changed
    if (width != lastWidth || height != lastHeight) {
        // Size changed: %dx%d
        lastWidth = width;
        lastHeight = height;
    }
    
    // Clean up existing FBO if any
    cleanupFBO();
    
    m_fboWidth = width;
    m_fboHeight = height;
    
    // Check if FBO functions are available
    if (!glGenFramebuffers || !glBindFramebuffer) {
        // FBO functions not available
        return;
    }
    
    // Generate FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    
    // FBO created successfully
    
    // Color texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    
    // Depth texture
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    
    // Normal texture
    glGenTextures(1, &m_normalTexture);
    glBindTexture(GL_TEXTURE_2D, m_normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Check FBO completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // FBO creation failed
        cleanupFBO();
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OutlinePreviewCanvas::cleanupFBO() {
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthTexture) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
    if (m_normalTexture) {
        glDeleteTextures(1, &m_normalTexture);
        m_normalTexture = 0;
    }
}

void OutlinePreviewCanvas::initializeShaders() {
    // Initialize shaders
    
    // Create normal shader program
    m_normalShader = createShaderProgram(g_normalVertexShader, g_normalFragmentShader);
    if (!m_normalShader) {
        // Failed to create normal shader
    }
    
    // Create outline shader program
    m_outlineShader = createShaderProgram(g_outlineVertexShader, g_outlineFragmentShader);
    if (!m_outlineShader) {
        // Failed to create outline shader
    } else {
        // Bind attribute locations before linking
        if (glBindAttribLocation) {
            glUseProgram(m_outlineShader);
            glBindAttribLocation(m_outlineShader, 0, "aPosition");
            glLinkProgram(m_outlineShader);
            glUseProgram(0);
        }
    }
    
    // Create quad VAO for fullscreen rendering
    createQuadVAO();
}

void OutlinePreviewCanvas::cleanupShaders() {
    if (m_normalShader) {
        glDeleteProgram(m_normalShader);
        m_normalShader = 0;
    }
    if (m_outlineShader) {
        glDeleteProgram(m_outlineShader);
        m_outlineShader = 0;
    }
    if (m_quadVAO) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
}

unsigned int OutlinePreviewCanvas::compileShader(const char* source, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        // Shader compilation failed
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

unsigned int OutlinePreviewCanvas::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return 0;
    }
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        // Shader program linking failed
        glDeleteProgram(program);
        program = 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void OutlinePreviewCanvas::createQuadVAO() {
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    
    // Check if VAO is supported
    if (glGenVertexArrays && glBindVertexArray) {
        glGenVertexArrays(1, &m_quadVAO);
        glBindVertexArray(m_quadVAO);
        // VAO created
    } else {
        // VAO not supported, using VBO only
    }
    
    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    if (m_quadVAO) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
}

int OutlinePreviewCanvas::getObjectAtPosition(const wxPoint& pos) {
    if (!m_modelRoot || m_modelRoot->getNumChildren() < 2) {
        return -1;
    }
    
    // Simple hit test based on screen regions
    // Since we have 4 objects arranged in a 2x2 grid
    wxSize size = GetClientSize();
    int halfW = size.GetWidth() / 2;
    int halfH = size.GetHeight() / 2;
    
    // Determine which quadrant the mouse is in
    // Objects in m_modelRoot: 0=rotation, 1=cylinder, 2=sphere, 3=cube, 4=cone
    if (pos.y < halfH) {
        // Top half
        return (pos.x < halfW) ? 1 : 2;  // cylinder : sphere
    } else {
        // Bottom half
        return (pos.x < halfW) ? 3 : 4;  // cube : cone
    }
}