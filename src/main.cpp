#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include "NavigationStyle.h"

class MyGLCanvas : public wxGLCanvas
{
public:
    MyGLCanvas(wxWindow* parent);
    ~MyGLCanvas();

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouse(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);

    void InitCoin3D();
    void InitGL();
    void Render();
    void RenderBackground();

    wxGLContext* m_context;
    SoSeparator* m_root;
    SoPerspectiveCamera* m_camera;
    CustomNavigationStyle* m_navStyle;

    bool m_isDragging;
    wxPoint m_lastPos;
    bool m_isRotating;
    bool m_isPanning;
    bool m_initialized;
};

// OpenGL attributes for smooth rendering (multisampling enabled via wxWidgets)
static int attribList[] = {
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_DEPTH_SIZE, 16,
    WX_GL_SAMPLES, 4, // Anti-aliasing
    WX_GL_SAMPLE_BUFFERS, 1,
    0
};

MyGLCanvas::MyGLCanvas(wxWindow* parent)
    : wxGLCanvas(parent, wxID_ANY,
        attribList,
        wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE),
    m_context(nullptr), m_root(nullptr), m_camera(nullptr), m_navStyle(nullptr),
    m_isDragging(false), m_isRotating(false), m_isPanning(false), m_initialized(false)
{
    m_context = new wxGLContext(this);
    if (!m_context->IsOK()) {
        wxLogError("Failed to create OpenGL context");
        return;
    }

    SetCurrent(*m_context);
    InitGL();
    InitCoin3D();
    m_initialized = true;

    // Bind event handlers
    Bind(wxEVT_PAINT, &MyGLCanvas::OnPaint, this);
    Bind(wxEVT_SIZE, &MyGLCanvas::OnSize, this);
    Bind(wxEVT_LEFT_DOWN, &MyGLCanvas::OnMouse, this);
    Bind(wxEVT_LEFT_UP, &MyGLCanvas::OnMouse, this);
    Bind(wxEVT_MIDDLE_DOWN, &MyGLCanvas::OnMouse, this);
    Bind(wxEVT_MIDDLE_UP, &MyGLCanvas::OnMouse, this);
    Bind(wxEVT_RIGHT_DOWN, &MyGLCanvas::OnMouse, this);
    Bind(wxEVT_RIGHT_UP, &MyGLCanvas::OnMouse, this);
    Bind(wxEVT_MOTION, &MyGLCanvas::OnMouse, this);
    Bind(wxEVT_MOUSEWHEEL, &MyGLCanvas::OnMouseWheel, this);
}

void MyGLCanvas::InitGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1); // Second light for balanced illumination
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH); // Smooth shading for geometry
}

MyGLCanvas::~MyGLCanvas()
{
    if (m_root) m_root->unref();
    delete m_context;
    delete m_navStyle;
}

void MyGLCanvas::InitCoin3D()
{
    SoDB::init();

    m_root = new SoSeparator;
    m_root->ref();

    // Camera setup
    m_camera = new SoPerspectiveCamera;
    m_camera->position.setValue(0, 0, 5);
    m_camera->nearDistance = 0.1f;
    m_camera->farDistance = 100.0f;
    m_root->addChild(m_camera);

    // Lighting (two directional lights to mimic FreeCAD's illumination)
    SoDirectionalLight* light1 = new SoDirectionalLight;
    light1->direction.setValue(-1, -1, -1);
    light1->intensity.setValue(0.8f); // Higher intensity to compensate for no ambient light
    light1->color.setValue(1.0f, 1.0f, 1.0f);
    m_root->addChild(light1);

    SoDirectionalLight* light2 = new SoDirectionalLight;
    light2->direction.setValue(1, 1, 1); // Opposite direction for balanced lighting
    light2->intensity.setValue(0.6f);
    light2->color.setValue(1.0f, 1.0f, 1.0f);
    m_root->addChild(light2);

    // Material for the sphere (FreeCAD-like appearance)
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.8f, 0.2f, 0.2f); // Red diffuse
    material->specularColor.setValue(0.5f, 0.5f, 0.5f); // Specular highlight
    material->shininess.setValue(0.4f); // Moderate shininess
    m_root->addChild(material);

    // Sphere geometry
    SoSphere* sphere = new SoSphere;
    sphere->radius.setValue(1.0f); // Radius matches torus outer radius for similar scale
    m_root->addChild(sphere);

    // Coordinate system (X, Y, Z axes, FreeCAD-style)
    SoSeparator* axes = new SoSeparator;
    SoCoordinate3* axesCoords = new SoCoordinate3;
    axesCoords->point.set1Value(0, 0, 0, 0);
    axesCoords->point.set1Value(1, 1, 0, 0); // X axis
    axesCoords->point.set1Value(2, 0, 0, 0);
    axesCoords->point.set1Value(3, 0, 1, 0); // Y axis
    axesCoords->point.set1Value(4, 0, 0, 0);
    axesCoords->point.set1Value(5, 0, 0, 1); // Z axis

    // X axis (red)
    SoBaseColor* xColor = new SoBaseColor;
    xColor->rgb.setValue(1, 0, 0);
    axes->addChild(xColor);
    SoLineSet* xAxis = new SoLineSet;
    xAxis->numVertices.setValue(2);
    axes->addChild(xAxis);

    // Y axis (green)
    SoBaseColor* yColor = new SoBaseColor;
    yColor->rgb.setValue(0, 1, 0);
    axes->addChild(yColor);
    SoLineSet* yAxis = new SoLineSet;
    yAxis->numVertices.setValue(2);
    yAxis->startIndex.setValue(2);
    axes->addChild(yAxis);

    // Z axis (blue)
    SoBaseColor* zColor = new SoBaseColor;
    zColor->rgb.setValue(0, 0, 1);
    axes->addChild(zColor);
    SoLineSet* zAxis = new SoLineSet;
    zAxis->numVertices.setValue(2);
    zAxis->startIndex.setValue(4);
    axes->addChild(zAxis);

    m_root->addChild(axes);

    // Navigation style (synchronized with primary light)
    m_navStyle = new CustomNavigationStyle(m_camera, light1);
}

void MyGLCanvas::RenderBackground()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Gradient background (mimics FreeCAD's 3D view)
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.4f, 0.4f); // Top: light gray
    glVertex2f(-1, 1);
    glVertex2f(1, 1);
    glColor3f(0.2f, 0.2f, 0.2f); // Bottom: dark gray
    glVertex2f(1, -1);
    glVertex2f(-1, -1);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void MyGLCanvas::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    if (!IsShown()) return;
    SetCurrent(*m_context);
    Render();
    SwapBuffers();
}

void MyGLCanvas::OnSize(wxSizeEvent&)
{
    wxSize size = GetClientSize();
    if (size.x > 0 && size.y > 0) {
        SetCurrent(*m_context);
        glViewport(0, 0, size.x, size.y);
        m_camera->aspectRatio = static_cast<float>(size.x) / size.y;
        Refresh();
    }
}

void MyGLCanvas::OnMouse(wxMouseEvent& event)
{
    if (!m_initialized) return;

    wxPoint pos = event.GetPosition();
    wxSize size = GetClientSize();

    // Normalize mouse position (-1 to 1)
    SbVec2f normPos(
        (2.0f * pos.x / size.x) - 1.0f,
        1.0f - (2.0f * pos.y / size.y)
    );

    if (event.LeftDown()) {
        m_isDragging = true;
        m_isRotating = true;
        m_lastPos = pos;
        CaptureMouse();
    }
    else if (event.RightDown()) {
        m_isDragging = true;
        m_isPanning = true;
        m_lastPos = pos;
        CaptureMouse();
    }
    else if (event.LeftUp() || event.RightUp()) {
        m_isDragging = false;
        m_isRotating = false;
        m_isPanning = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
    else if (event.Dragging() && m_isDragging) {
        wxPoint currPos = event.GetPosition();
        SbVec2f currNormPos(
            (2.0f * currPos.x / size.x) - 1.0f,
            1.0f - (2.0f * currPos.y / size.y)
        );
        SbVec2f lastNormPos(
            (2.0f * m_lastPos.x / size.x) - 1.0f,
            1.0f - (2.0f * m_lastPos.y / size.y)
        );
        SbVec2f delta = currNormPos - lastNormPos;

        if (m_isRotating) {
            m_navStyle->rotateCamera(delta);
        }
        else if (m_isPanning) {
            m_navStyle->panCamera(delta);
        }
        m_lastPos = currPos;
        Refresh();
    }
}

void MyGLCanvas::OnMouseWheel(wxMouseEvent& event)
{
    if (!m_initialized) return;

    float delta = event.GetWheelRotation() / 120.0f;
    m_navStyle->zoomCamera(delta);
    Refresh();
}

void MyGLCanvas::Render()
{
    if (!m_initialized) return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render FreeCAD-like gradient background
    RenderBackground();

    // Render Coin3D scene
    SbViewportRegion viewport(GetClientSize().x, GetClientSize().y);
    SoGLRenderAction renderAction(viewport);
    renderAction.apply(m_root);
}

class MyFrame : public wxFrame
{
public:
    MyFrame() : wxFrame(nullptr, wxID_ANY, "FreeCAD-Like 3D Navigation", wxDefaultPosition, wxSize(800, 600))
    {
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        MyGLCanvas* canvas = new MyGLCanvas(this);
        sizer->Add(canvas, 1, wxEXPAND);
        SetSizer(sizer);
    }
};

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {
        SetDllDirectory(L"lib"); // Ensure Coin3D/wxWidgets DLLs are found
        MyFrame* frame = new MyFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);