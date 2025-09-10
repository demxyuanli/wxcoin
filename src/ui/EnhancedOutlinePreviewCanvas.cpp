#include "ui/EnhancedOutlinePreviewCanvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"

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
#include <Inventor/nodes/SoTorus.h>
#include <Inventor/nodes/SoText3.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SbViewportRegion.h>

#include <GL/gl.h>
#include <cmath>
#include <chrono>
#include <algorithm>

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
EVT_TIMER(wxID_ANY, EnhancedOutlinePreviewCanvas::onTimer)
END_EVENT_TABLE()

EnhancedOutlinePreviewCanvas::EnhancedOutlinePreviewCanvas(wxWindow* parent, wxWindowID id, 
                                                         const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, nullptr, pos, size, wxWANTS_CHARS) {
    
    // Create OpenGL context
    m_glContext = new wxGLContext(this);
    
    // Create performance timer
    m_performanceTimer = new wxTimer(this);
    m_performanceTimer->Start(1000); // Update every second
    
    // Initialize performance tracking
    m_frameTimes.reserve(1000);
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    
    // Initialize the scene
    initializeScene();
    
    // Force initial paint
    Refresh(false);
}

EnhancedOutlinePreviewCanvas::~EnhancedOutlinePreviewCanvas() {
    delete m_glContext;
    delete m_performanceTimer;
    
    if (m_sceneRoot) {
        m_sceneRoot->unref();
    }
    
    // Clean up preview models
    for (auto& pair : m_previewModels) {
        if (pair.second) {
            pair.second->unref();
        }
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
    
    // Add lighting
    SoDirectionalLight* light1 = new SoDirectionalLight;
    light1->direction.setValue(-0.5f, -0.5f, -0.7f);
    light1->intensity = 0.8f;
    m_sceneRoot->addChild(light1);
    
    SoDirectionalLight* light2 = new SoDirectionalLight;
    light2->direction.setValue(0.5f, 0.5f, 0.7f);
    light2->intensity = 0.4f;
    m_sceneRoot->addChild(light2);
    
    // Create model root
    m_modelRoot = new SoSeparator;
    m_sceneRoot->addChild(m_modelRoot);
    
    // Create preview models
    createBasicModels();
    createAdvancedModels();
    
    // Set default active model
    if (!m_previewModels.empty()) {
        m_activeModel = m_previewModels.begin()->first;
    }
    
    // Initialize outline pass
    m_outlinePass = std::make_unique<EnhancedOutlinePass>(m_sceneManager, m_modelRoot);
    m_outlinePass->setParams(m_outlineParams);
    m_outlinePass->setSelectionConfig(m_selectionConfig);
    m_outlinePass->setEnabled(m_previewEnabled);
    
    m_initialized = true;
    m_needsRedraw = true;
}

void EnhancedOutlinePreviewCanvas::createBasicModels() {
    // Create a cube
    {
        SoSeparator* cubeSep = new SoSeparator;
        cubeSep->ref();
        
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(-3.0f, 3.0f, 0);
        cubeSep->addChild(transform);
        
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
        
        m_previewModels["Cube"] = cubeSep;
    }
    
    // Create a sphere
    {
        SoSeparator* sphereSep = new SoSeparator;
        sphereSep->ref();
        
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(3.0f, 3.0f, 0);
        sphereSep->addChild(transform);
        
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
        
        m_previewModels["Sphere"] = sphereSep;
    }
    
    // Create a cylinder
    {
        SoSeparator* cylSep = new SoSeparator;
        cylSep->ref();
        
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(-3.0f, -3.0f, 0);
        cylSep->addChild(transform);
        
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
        
        m_previewModels["Cylinder"] = cylSep;
    }
    
    // Create a cone
    {
        SoSeparator* coneSep = new SoSeparator;
        coneSep->ref();
        
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(3.0f, -3.0f, 0);
        coneSep->addChild(transform);
        
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
        
        m_previewModels["Cone"] = coneSep;
    }
}

void EnhancedOutlinePreviewCanvas::createAdvancedModels() {
    // Create a torus
    {
        SoSeparator* torusSep = new SoSeparator;
        torusSep->ref();
        
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(0.0f, 0.0f, 0);
        torusSep->addChild(transform);
        
        SoMaterial* material = new SoMaterial;
        material->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                      m_geomColor.Green() / 255.0f,
                                      m_geomColor.Blue() / 255.0f);
        material->specularColor.setValue(1.0f, 1.0f, 1.0f);
        material->shininess = 0.8f;
        torusSep->addChild(material);
        
        SoTorus* torus = new SoTorus;
        torus->majorRadius = 2.0f;
        torus->minorRadius = 0.5f;
        torusSep->addChild(torus);
        
        m_previewModels["Torus"] = torusSep;
    }
    
    // Create a complex model (multiple objects)
    {
        SoSeparator* complexSep = new SoSeparator;
        complexSep->ref();
        
        // Add rotation for animation
        SoRotationXYZ* rotation = new SoRotationXYZ;
        rotation->axis = SoRotationXYZ::Y;
        rotation->angle = 0.0f;
        complexSep->addChild(rotation);
        
        // Multiple small objects
        for (int i = 0; i < 5; i++) {
            SoSeparator* objSep = new SoSeparator;
            
            SoTransform* transform = new SoTransform;
            float angle = i * 2.0f * M_PI / 5.0f;
            transform->translation.setValue(cos(angle) * 2.0f, sin(angle) * 2.0f, 0);
            objSep->addChild(transform);
            
            SoMaterial* material = new SoMaterial;
            material->diffuseColor.setValue(m_geomColor.Red() / 255.0f,
                                          m_geomColor.Green() / 255.0f,
                                          m_geomColor.Blue() / 255.0f);
            objSep->addChild(material);
            
            if (i % 2 == 0) {
                SoCube* cube = new SoCube;
                cube->width = 0.8f;
                cube->height = 0.8f;
                cube->depth = 0.8f;
                objSep->addChild(cube);
            } else {
                SoSphere* sphere = new SoSphere;
                sphere->radius = 0.4f;
                objSep->addChild(sphere);
            }
            
            complexSep->addChild(objSep);
        }
        
        m_previewModels["Complex"] = complexSep;
    }
}

void EnhancedOutlinePreviewCanvas::updateOutlineParams(const EnhancedOutlineParams& params) {
    m_outlineParams = params;
    
    if (m_outlinePass) {
        m_outlinePass->setParams(params);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

EnhancedOutlineParams EnhancedOutlinePreviewCanvas::getOutlineParams() const {
    return m_outlineParams;
}

void EnhancedOutlinePreviewCanvas::updateSelectionConfig(const SelectionOutlineConfig& config) {
    m_selectionConfig = config;
    
    if (m_outlinePass) {
        m_outlinePass->setSelectionConfig(config);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

SelectionOutlineConfig EnhancedOutlinePreviewCanvas::getSelectionConfig() const {
    return m_selectionConfig;
}

void EnhancedOutlinePreviewCanvas::setDebugMode(OutlineDebugMode mode) {
    m_debugMode = mode;
    
    if (m_outlinePass) {
        m_outlinePass->setDebugMode(mode);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

OutlineDebugMode EnhancedOutlinePreviewCanvas::getDebugMode() const {
    return m_debugMode;
}

void EnhancedOutlinePreviewCanvas::setPerformanceMode(bool enabled) {
    m_performanceMode = enabled;
    m_qualityMode = false;
    
    if (m_outlinePass) {
        m_outlinePass->setDownsampleFactor(2);
        m_outlinePass->setEarlyCullingEnabled(true);
        m_outlinePass->setMultiSampleEnabled(false);
    }
    
    m_performanceInfo.performanceMode = "Performance";
    m_performanceInfo.isOptimized = true;
}

void EnhancedOutlinePreviewCanvas::setQualityMode(bool enabled) {
    m_qualityMode = enabled;
    m_performanceMode = false;
    
    if (m_outlinePass) {
        m_outlinePass->setDownsampleFactor(1);
        m_outlinePass->setEarlyCullingEnabled(false);
        m_outlinePass->setMultiSampleEnabled(true);
    }
    
    m_performanceInfo.performanceMode = "Quality";
    m_performanceInfo.isOptimized = false;
}

void EnhancedOutlinePreviewCanvas::setBalancedMode() {
    m_performanceMode = false;
    m_qualityMode = false;
    
    if (m_outlinePass) {
        m_outlinePass->setDownsampleFactor(1);
        m_outlinePass->setEarlyCullingEnabled(true);
        m_outlinePass->setMultiSampleEnabled(false);
    }
    
    m_performanceInfo.performanceMode = "Balanced";
    m_performanceInfo.isOptimized = true;
}

void EnhancedOutlinePreviewCanvas::setBackgroundColor(const wxColour& color) {
    m_bgColor = color;
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setOutlineColor(const wxColour& color) {
    m_outlineColor = color;
    
    if (m_outlinePass) {
        EnhancedOutlineParams params = m_outlinePass->getParams();
        params.outlineColor[0] = color.Red() / 255.0f;
        params.outlineColor[1] = color.Green() / 255.0f;
        params.outlineColor[2] = color.Blue() / 255.0f;
        m_outlinePass->setParams(params);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setGlowColor(const wxColour& color) {
    m_glowColor = color;
    
    if (m_outlinePass) {
        EnhancedOutlineParams params = m_outlinePass->getParams();
        params.glowColor[0] = color.Red() / 255.0f;
        params.glowColor[1] = color.Green() / 255.0f;
        params.glowColor[2] = color.Blue() / 255.0f;
        m_outlinePass->setParams(params);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setGeometryColor(const wxColour& color) {
    m_geomColor = color;
    
    // Update all model materials
    for (auto& pair : m_previewModels) {
        SoSeparator* model = pair.second;
        for (int i = 0; i < model->getNumChildren(); i++) {
            if (model->getChild(i)->isOfType(SoMaterial::getClassTypeId())) {
                SoMaterial* material = static_cast<SoMaterial*>(model->getChild(i));
                material->diffuseColor.setValue(color.Red() / 255.0f,
                                              color.Green() / 255.0f,
                                              color.Blue() / 255.0f);
                break;
            }
        }
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setHoverColor(const wxColour& color) {
    m_hoverColor = color;
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setSelectionColor(const wxColour& color) {
    m_selectionColor = color;
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setSelectedObjects(const std::vector<int>& objects) {
    m_selectedObjects = objects;
    
    if (m_outlinePass) {
        // Update selection state in outline pass
        // This would need to be implemented based on your object ID system
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setHoveredObject(int objectId) {
    m_hoveredObject = objectId;
    
    if (m_outlinePass) {
        m_outlinePass->setHoveredObject(objectId);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::clearSelection() {
    m_selectedObjects.clear();
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::clearHover() {
    m_hoveredObject = -1;
    
    if (m_outlinePass) {
        m_outlinePass->clearHover();
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

EnhancedOutlinePreviewCanvas::PerformanceInfo EnhancedOutlinePreviewCanvas::getPerformanceInfo() const {
    return m_performanceInfo;
}

void EnhancedOutlinePreviewCanvas::setPreviewEnabled(bool enabled) {
    m_previewEnabled = enabled;
    
    if (m_outlinePass) {
        m_outlinePass->setEnabled(enabled);
    }
    
    m_needsRedraw = true;
    Refresh(false);
}

void EnhancedOutlinePreviewCanvas::setAutoRotate(bool enabled) {
    m_autoRotate = enabled;
}

void EnhancedOutlinePreviewCanvas::setRotationSpeed(float speed) {
    m_rotationSpeed = speed;
}

void EnhancedOutlinePreviewCanvas::addPreviewModel(const wxString& name, SoSeparator* model) {
    if (model) {
        model->ref();
        m_previewModels[name] = model;
        
        // Add to scene if it's the active model
        if (m_activeModel == name) {
            m_modelRoot->addChild(model);
        }
        
        m_needsRedraw = true;
        Refresh(false);
    }
}

void EnhancedOutlinePreviewCanvas::removePreviewModel(const wxString& name) {
    auto it = m_previewModels.find(name);
    if (it != m_previewModels.end()) {
        if (it->second) {
            it->second->unref();
        }
        m_previewModels.erase(it);
        
        // Switch to another model if this was active
        if (m_activeModel == name && !m_previewModels.empty()) {
            m_activeModel = m_previewModels.begin()->first;
        }
        
        m_needsRedraw = true;
        Refresh(false);
    }
}

void EnhancedOutlinePreviewCanvas::setActiveModel(const wxString& name) {
    if (m_previewModels.find(name) != m_previewModels.end()) {
        m_activeModel = name;
        
        // Clear current models and add the active one
        m_modelRoot->removeAllChildren();
        
        // Add rotation for animation
        SoRotationXYZ* rotation = new SoRotationXYZ;
        rotation->axis = SoRotationXYZ::Y;
        rotation->angle = 0.0f;
        m_modelRoot->addChild(rotation);
        
        // Add the active model
        m_modelRoot->addChild(m_previewModels[name]);
        
        m_needsRedraw = true;
        Refresh(false);
    }
}

wxArrayString EnhancedOutlinePreviewCanvas::getAvailableModels() const {
    wxArrayString models;
    for (const auto& pair : m_previewModels) {
        models.Add(pair.first);
    }
    return models;
}

void EnhancedOutlinePreviewCanvas::onPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    
    if (!m_initialized) {
        initializeScene();
    }
    
    render();
}

void EnhancedOutlinePreviewCanvas::onSize(wxSizeEvent& event) {
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
        int prevHovered = m_hoveredObject;
        m_hoveredObject = getObjectAtPosition(event.GetPosition());
        
        if (prevHovered != m_hoveredObject) {
            setHoveredObject(m_hoveredObject);
        }
    }
    else if (event.LeftDClick()) {
        // Toggle selection on double click
        int objectId = getObjectAtPosition(event.GetPosition());
        if (objectId >= 0) {
            auto it = std::find(m_selectedObjects.begin(), m_selectedObjects.end(), objectId);
            if (it != m_selectedObjects.end()) {
                m_selectedObjects.erase(it);
            } else {
                m_selectedObjects.push_back(objectId);
            }
            updateObjectSelection();
        }
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
    
    // Auto rotation
    if (m_autoRotate && m_modelRoot && m_modelRoot->getNumChildren() > 0) {
        for (int i = 0; i < m_modelRoot->getNumChildren(); i++) {
            if (m_modelRoot->getChild(i)->isOfType(SoRotationXYZ::getClassTypeId())) {
                SoRotationXYZ* rotation = static_cast<SoRotationXYZ*>(m_modelRoot->getChild(i));
                rotation->angle = rotation->angle.getValue() + m_rotationSpeed * 0.01f;
                m_needsRedraw = true;
                Refresh(false);
                break;
            }
        }
    }
}

void EnhancedOutlinePreviewCanvas::onTimer(wxTimerEvent& event) {
    updatePerformanceStats();
}

void EnhancedOutlinePreviewCanvas::render() {
    if (!m_glContext || !m_sceneRoot) return;
    
    auto frameStart = std::chrono::high_resolution_clock::now();
    
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
    
    // Render the scene
    renderAction.apply(m_sceneRoot);
    
    SwapBuffers();
    
    // Update performance stats
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
    m_performanceInfo.frameTime = frameDuration.count() / 1000.0f;
    
    m_needsRedraw = false;
}

void EnhancedOutlinePreviewCanvas::updatePerformanceStats() {
    if (m_performanceInfo.frameTime > 0) {
        m_frameTimes.push_back(m_performanceInfo.frameTime);
        
        // Keep only recent frames
        if (m_frameTimes.size() > 100) {
            m_frameTimes.erase(m_frameTimes.begin());
        }
        
        // Calculate average
        if (!m_frameTimes.empty()) {
            float sum = 0.0f;
            for (float time : m_frameTimes) {
                sum += time;
            }
            m_performanceInfo.averageFrameTime = sum / m_frameTimes.size();
        }
        
        m_performanceInfo.frameCount = m_frameTimes.size();
    }
}

void EnhancedOutlinePreviewCanvas::logPerformanceInfo() {
    LOG_INF(("Preview Performance - Frame Time: " + std::to_string(m_performanceInfo.frameTime) + 
            "ms, Average: " + std::to_string(m_performanceInfo.averageFrameTime) + 
            "ms, Mode: " + m_performanceInfo.performanceMode).c_str(), 
            "EnhancedOutlinePreviewCanvas");
}

int EnhancedOutlinePreviewCanvas::getObjectAtPosition(const wxPoint& pos) {
    // Simple position-based detection for demo
    wxSize size = GetClientSize();
    int halfWidth = size.GetWidth() / 2;
    int halfHeight = size.GetHeight() / 2;
    
    // Determine which quadrant the mouse is in
    if (pos.x < halfWidth && pos.y < halfHeight) {
        return 1; // Top-left
    } else if (pos.x >= halfWidth && pos.y < halfHeight) {
        return 2; // Top-right
    } else if (pos.x < halfWidth && pos.y >= halfHeight) {
        return 3; // Bottom-left
    } else {
        return 4; // Bottom-right
    }
}

void EnhancedOutlinePreviewCanvas::updateObjectSelection() {
    // Update selection state in outline pass
    if (m_outlinePass) {
        // This would need to be implemented based on your object ID system
        // For now, just trigger a redraw
        m_needsRedraw = true;
        Refresh(false);
    }
}

void EnhancedOutlinePreviewCanvas::simulateSelectionState() {
    // Simulate selection state for preview
    // This could be used to show different outline intensities
    // based on selection state
}