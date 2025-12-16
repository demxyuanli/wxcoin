#include "ui/DisplayModePreviewCanvas.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
#include "geometry/helper/PointViewBuilder.h"
#include "edges/ModularEdgeComponent.h"
#include "EdgeTypes.h"
#include "rendering/RenderingToolkitAPI.h"
#include "OCCBrepConverter.h"
#include "OCCMeshConverter.h"
#include "logger/Logger.h"
#include <OpenCASCADE/Quantity_Color.hxx>
#include <wx/dcclient.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/SoDB.h>
#include <GL/gl.h>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SO_SWITCH_ALL
#define SO_SWITCH_ALL 0
#endif
#ifndef SO_SWITCH_NONE
#define SO_SWITCH_NONE -1
#endif

BEGIN_EVENT_TABLE(DisplayModePreviewCanvas, wxGLCanvas)
EVT_PAINT(DisplayModePreviewCanvas::onPaint)
EVT_SIZE(DisplayModePreviewCanvas::onSize)
EVT_ERASE_BACKGROUND(DisplayModePreviewCanvas::onEraseBackground)
EVT_LEFT_DOWN(DisplayModePreviewCanvas::onMouseEvent)
EVT_LEFT_UP(DisplayModePreviewCanvas::onMouseEvent)
EVT_MOTION(DisplayModePreviewCanvas::onMouseEvent)
EVT_MOUSEWHEEL(DisplayModePreviewCanvas::onMouseEvent)
END_EVENT_TABLE()

DisplayModePreviewCanvas::DisplayModePreviewCanvas(wxWindow* parent, wxWindowID id,
                                                   const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, nullptr, pos, size, wxWANTS_CHARS) {
    
    SoDB::init();
    
    m_glContext = new wxGLContext(this);
    m_edgeComponent = std::make_unique<ModularEdgeComponent>();
    m_pointViewBuilder = std::make_unique<PointViewBuilder>();
    initializeScene();
    Refresh(false);
}

DisplayModePreviewCanvas::~DisplayModePreviewCanvas() {
    delete m_glContext;
    delete m_mesh;
    
    if (m_sceneRoot) {
        m_sceneRoot->unref();
    }
    if (m_surfaceNode) {
        m_surfaceNode->unref();
    }
    if (m_edgesNode) {
        m_edgesNode->unref();
    }
    if (m_pointsNode) {
        m_pointsNode->unref();
    }
}

void DisplayModePreviewCanvas::initializeScene() {
    if (!m_glContext) return;
    
    SetCurrent(*m_glContext);
    
    m_sceneRoot = new SoSeparator;
    m_sceneRoot->ref();
    
    setupCamera();
    setupLighting();
    setupMaterial();
    createGeometry();
    
    GeometryRenderContext defaultContext;
    defaultContext.material.diffuseColor = Quantity_Color(0.6, 0.6, 0.7, Quantity_TOC_RGB);
    defaultContext.material.ambientColor = Quantity_Color(0.4, 0.4, 0.5, Quantity_TOC_RGB);
    defaultContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    defaultContext.material.shininess = 50.0;
    defaultContext.display.wireframeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    defaultContext.display.wireframeWidth = 1.0;
    
    m_currentConfig = DisplayModeConfigFactory::getConfig(RenderingConfig::DisplayMode::Solid, defaultContext);
    
    m_initialized = true;
    updateGeometryFromConfig(m_currentConfig);
    
    m_needsRedraw = true;
    
    CallAfter([this]() {
        performViewAll();
    });
}

void DisplayModePreviewCanvas::setupCamera() {
    m_camera = new SoPerspectiveCamera;
    
    float focalDist = 10.0f;
    
    SbRotation rotY(SbVec3f(0, 1, 0), M_PI / 4.0);
    SbRotation rotX(SbVec3f(1, 0, 0), asin(tan(M_PI / 6.0)));
    
    m_camera->orientation.setValue(rotY * rotX);
    
    SbVec3f zAxis;
    m_camera->orientation.getValue().multVec(SbVec3f(0, 0, 1), zAxis);
    m_camera->position.setValue(zAxis * focalDist);
    
    m_camera->nearDistance = 0.1f;
    m_camera->farDistance = 100.0f;
    m_camera->focalDistance = focalDist;
    
    m_sceneRoot->addChild(m_camera);
}

void DisplayModePreviewCanvas::setupLighting() {
    SoDirectionalLight* light = new SoDirectionalLight;
    SbVec3f lightDir;
    m_camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), lightDir);
    light->direction.setValue(lightDir);
    light->intensity.setValue(1.0f);
    m_sceneRoot->addChild(light);
    
    m_lightModel = new SoLightModel;
    m_sceneRoot->addChild(m_lightModel);
}

void DisplayModePreviewCanvas::setupMaterial() {
    m_material = new SoMaterial;
    m_sceneRoot->addChild(m_material);
    
    m_drawStyle = new SoDrawStyle;
    m_sceneRoot->addChild(m_drawStyle);
}

void DisplayModePreviewCanvas::createGeometry() {
    m_geometryRoot = new SoSeparator;
    m_geometryRoot->ref();
    
    m_surfaceNode = new SoSeparator;
    m_surfaceNode->ref();
    m_geometryRoot->addChild(m_surfaceNode);
    
    m_edgesNode = new SoSeparator;
    m_edgesNode->ref();
    
    m_pointsNode = new SoSeparator;
    m_pointsNode->ref();
    
    m_surfaceSwitch = new SoSwitch;
    m_surfaceSwitch->addChild(m_geometryRoot);
    m_sceneRoot->addChild(m_surfaceSwitch);
    
    m_edgesSwitch = new SoSwitch;
    m_edgesSwitch->addChild(m_edgesNode);
    m_sceneRoot->addChild(m_edgesSwitch);
    
    m_pointsSwitch = new SoSwitch;
    m_pointsSwitch->addChild(m_pointsNode);
    m_sceneRoot->addChild(m_pointsSwitch);
    
    wxString stepPath;
    std::vector<wxString> searchedPaths;
    
    wxFileName cwdStepFile(wxFileName::GetCwd(), "modpreview.stp");
    cwdStepFile.AppendDir("config");
    cwdStepFile.AppendDir("samples");
    wxString cwdPath = cwdStepFile.GetFullPath();
    searchedPaths.push_back(cwdPath);
    
    if (std::filesystem::exists(cwdPath.ToStdString())) {
        stepPath = cwdPath;
        LOG_INF_S("Found STEP file at: " + stepPath.ToStdString());
    } else {
        wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        wxFileName exeStepFile(exePath.GetPath(), "modpreview.stp");
        exeStepFile.AppendDir("config");
        exeStepFile.AppendDir("samples");
        wxString exePathStr = exeStepFile.GetFullPath();
        searchedPaths.push_back(exePathStr);
        
        if (std::filesystem::exists(exePathStr.ToStdString())) {
            stepPath = exePathStr;
            LOG_INF_S("Found STEP file at: " + stepPath.ToStdString());
        } else {
            wxFileName projectRoot = exePath;
            projectRoot.RemoveLastDir();
            projectRoot.RemoveLastDir();
            wxFileName projectStepFile(projectRoot.GetPath(), "modpreview.stp");
            projectStepFile.AppendDir("config");
            projectStepFile.AppendDir("samples");
            wxString projectPathStr = projectStepFile.GetFullPath();
            searchedPaths.push_back(projectPathStr);
            
            if (std::filesystem::exists(projectPathStr.ToStdString())) {
                stepPath = projectPathStr;
                LOG_INF_S("Found STEP file at: " + stepPath.ToStdString());
            }
        }
    }
    
    std::string stepPathStr = stepPath.ToStdString();
    
    if (stepPathStr.empty()) {
        LOG_ERR_S("STEP file not found. Searched locations:");
        for (size_t i = 0; i < searchedPaths.size(); ++i) {
            bool exists = std::filesystem::exists(searchedPaths[i].ToStdString());
            LOG_ERR_S("  " + std::to_string(i + 1) + ". " + searchedPaths[i].ToStdString() + (exists ? " [EXISTS]" : " [NOT FOUND]"));
        }
        return;
    }
    
    if (!std::filesystem::exists(stepPathStr)) {
        LOG_ERR_S("STEP file does not exist: " + stepPathStr);
        return;
    }
    
    LOG_INF_S("Attempting to load STEP file: " + stepPathStr);
    
    try {
        TopoDS_Shape shape = OCCBrepConverter::loadFromSTEP(stepPathStr);
        
        if (shape.IsNull()) {
            LOG_ERR_S("Failed to load STEP geometry: Shape is null");
            return;
        }
        
        m_shape = shape;
        
        LOG_INF_S("STEP file loaded successfully, converting to mesh...");
        MeshParameters meshParams;
        meshParams.deflection = 0.5;
        meshParams.angularDeflection = 0.5;
        
        auto& manager = RenderingToolkitAPI::getManager();
        auto processor = manager.getGeometryProcessor("OpenCASCADE");
        if (!processor) {
            LOG_ERR_S("OpenCASCADE geometry processor not available");
            return;
        }
        
        TriangleMesh mesh = processor->convertToMesh(shape, meshParams);
        if (mesh.vertices.empty()) {
            LOG_ERR_S("Failed to convert STEP geometry to mesh");
            return;
        }
        
        m_mesh = new TriangleMesh(std::move(mesh));
        LOG_INF_S("Mesh created: " + std::to_string(m_mesh->vertices.size()) + " vertices, " + std::to_string(m_mesh->triangles.size() / 3) + " triangles");
        
        LOG_INF_S("Converting mesh to Coin3D...");
        SoSeparator* stepGeometry = OCCBrepConverter::convertToCoin3D(shape, 0.5);
        
        if (!stepGeometry) {
            LOG_ERR_S("Failed to convert STEP geometry to Coin3D");
            return;
        }
        
        m_surfaceNode->addChild(stepGeometry);
        LOG_INF_S("Surface geometry added to scene");
        
        SetCurrent(*m_glContext);
        SbViewportRegion viewport(100, 100);
        SoGetBoundingBoxAction bboxAction(viewport);
        bboxAction.apply(m_geometryRoot);
        SbBox3f bbox = bboxAction.getBoundingBox();
        
        if (bbox.isEmpty()) {
            LOG_WRN_S("Warning: Geometry bounding box is empty - geometry may not be visible");
        } else {
            SbVec3f min = bbox.getMin();
            SbVec3f max = bbox.getMax();
            SbVec3f center = bbox.getCenter();
            float size = (max - min).length();
            LOG_INF_S("Geometry bounding box:");
            LOG_INF_S("  Min: (" + std::to_string(min[0]) + ", " + std::to_string(min[1]) + ", " + std::to_string(min[2]) + ")");
            LOG_INF_S("  Max: (" + std::to_string(max[0]) + ", " + std::to_string(max[1]) + ", " + std::to_string(max[2]) + ")");
            LOG_INF_S("  Center: (" + std::to_string(center[0]) + ", " + std::to_string(center[1]) + ", " + std::to_string(center[2]) + ")");
            LOG_INF_S("  Size: " + std::to_string(size));
        }
        
        CallAfter([this]() {
            performViewAll();
        });
        
        LOG_INF_S("STEP file loaded successfully: " + stepPathStr);
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception loading STEP geometry: " + std::string(e.what()));
    } catch (...) {
        LOG_ERR_S("Unknown exception loading STEP geometry");
    }
}

void DisplayModePreviewCanvas::updateGeometryFromConfig(const DisplayModeConfig& config) {
    if (!m_glContext || !m_surfaceSwitch || !m_lightModel || !m_material || !m_drawStyle) {
        LOG_WRN_S("updateGeometryFromConfig: Scene not ready, skipping");
        return;
    }
    
    SetCurrent(*m_glContext);
    
    if (config.rendering.lightModel == DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR) {
        m_lightModel->model.setValue(SoLightModel::BASE_COLOR);
    } else {
        m_lightModel->model.setValue(SoLightModel::PHONG);
    }
    
    if (config.rendering.materialOverride.enabled) {
        Standard_Real r, g, b;
        config.rendering.materialOverride.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        m_material->diffuseColor.setValue(r, g, b);
        
        config.rendering.materialOverride.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
        m_material->ambientColor.setValue(r, g, b);
        
        config.rendering.materialOverride.specularColor.Values(r, g, b, Quantity_TOC_RGB);
        m_material->specularColor.setValue(r, g, b);
        
        config.rendering.materialOverride.emissiveColor.Values(r, g, b, Quantity_TOC_RGB);
        m_material->emissiveColor.setValue(r, g, b);
        
        m_material->shininess.setValue(config.rendering.materialOverride.shininess);
        m_material->transparency.setValue(config.rendering.materialOverride.transparency);
    } else {
        m_material->diffuseColor.setValue(0.5f, 0.5f, 0.6f);
        m_material->ambientColor.setValue(0.3f, 0.3f, 0.4f);
        m_material->specularColor.setValue(1.0f, 1.0f, 1.0f);
        m_material->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
        m_material->shininess.setValue(50.0f);
        m_material->transparency.setValue(0.0f);
    }
    
    int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
    m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("updateGeometryFromConfig: Surface switch set to " + std::to_string(surfaceSwitchValue) + " (requireSurface=" + (config.nodes.requireSurface ? "true" : "false") + ")");
    
    if (m_shape.IsNull() || !m_mesh) {
        LOG_WRN_S("updateGeometryFromConfig: Shape or mesh not available");
        m_needsRedraw = true;
        Refresh();
        return;
    }
    
    m_edgesNode->removeAllChildren();
    
    bool showOriginalEdges = config.nodes.requireOriginalEdges && config.edges.originalEdge.enabled;
    bool showMeshEdges = config.nodes.requireMeshEdges && config.edges.meshEdge.enabled;
    
    if (showOriginalEdges || showMeshEdges) {
        if (m_edgeComponent) {
            if (showOriginalEdges) {
                if (!m_edgeComponent->getEdgeNode(EdgeType::Original)) {
                    m_edgeComponent->extractOriginalEdges(m_shape,
                                                          80.0, 0.01, false,
                                                          config.edges.originalEdge.color,
                                                          config.edges.originalEdge.width,
                                                          false,
                                                          Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                                                          3.0);
                }
                m_edgeComponent->setEdgeDisplayType(EdgeType::Original, true);
                m_edgeComponent->setEdgeDisplayType(EdgeType::Mesh, false);
            } else if (showMeshEdges && m_mesh && !m_mesh->triangles.empty()) {
                if (!m_edgeComponent->getEdgeNode(EdgeType::Mesh)) {
                    Quantity_Color edgeColor = config.edges.meshEdge.color;
                    if (config.edges.meshEdge.useEffectiveColor) {
                        if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                            edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                        }
                    }
                    m_edgeComponent->extractMeshEdges(*m_mesh, edgeColor, config.edges.meshEdge.width);
                }
                m_edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
                m_edgeComponent->setEdgeDisplayType(EdgeType::Mesh, true);
            }
            
            m_edgeComponent->updateEdgeDisplay(m_edgesNode);
        }
        
        m_edgesSwitch->whichChild.setValue(0);
    } else {
        m_edgesSwitch->whichChild.setValue(-1);
    }
    
    m_pointsNode->removeAllChildren();
    
    if (config.nodes.requirePoints && m_pointViewBuilder && m_mesh) {
        GeometryRenderContext defaultContext;
        defaultContext.display.pointViewColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);
        defaultContext.display.pointViewSize = 3.0;
        defaultContext.display.pointViewShape = 0;
        
        m_pointViewBuilder->createPointViewRepresentation(m_pointsNode, *m_mesh, defaultContext.display);
        m_pointsSwitch->whichChild.setValue(0);
        LOG_INF_S("Points view created: " + std::to_string(m_mesh->vertices.size()) + " points");
    } else {
        m_pointsSwitch->whichChild.setValue(-1);
    }
    
    m_needsRedraw = true;
    Refresh();
}

void DisplayModePreviewCanvas::updateDisplayMode(RenderingConfig::DisplayMode mode, const DisplayModeConfig& config) {
    m_currentMode = mode;
    m_currentConfig = config;
    updateGeometryFromConfig(config);
}

void DisplayModePreviewCanvas::refreshPreview() {
    m_needsRedraw = true;
    Refresh();
}

void DisplayModePreviewCanvas::performViewAll() {
    if (!m_initialized || !m_camera || !m_sceneRoot) {
        return;
    }
    
    wxSize size = GetSize();
    if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
        return;
    }
    
    SetCurrent(*m_glContext);
    
    const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!glVersion) {
        LOG_WRN_S("performViewAll: GL context not available");
        return;
    }
    
    float aspect = static_cast<float>(size.GetWidth()) / static_cast<float>(size.GetHeight());
    SoPerspectiveCamera* perspCam = static_cast<SoPerspectiveCamera*>(m_camera);
    if (perspCam) {
        perspCam->aspectRatio.setValue(aspect);
    }
    
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    m_camera->viewAll(m_sceneRoot, viewport, 1.1f);
    
    LOG_INF_S("performViewAll: ViewAll called with size: " + std::to_string(size.GetWidth()) + "x" + std::to_string(size.GetHeight()));
    
    m_needsRedraw = true;
    Refresh();
}

void DisplayModePreviewCanvas::onPaint(wxPaintEvent& event) {
    if (!m_initialized || !m_sceneRoot) {
        event.Skip();
        return;
    }
    
    wxPaintDC dc(this);
    SetCurrent(*m_glContext);
    
    wxSize size = GetSize();
    if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
        event.Skip();
        return;
    }
    
    glViewport(0, 0, size.GetWidth(), size.GetHeight());
    
    glClearColor(0.85f, 0.9f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    SbViewportRegion vpRegion(size.GetWidth(), size.GetHeight());
    SoGLRenderAction renderAction(vpRegion);
    renderAction.apply(m_sceneRoot);
    
    SwapBuffers();
    event.Skip();
}

void DisplayModePreviewCanvas::onSize(wxSizeEvent& event) {
    if (!m_initialized || !m_camera) {
        event.Skip();
        return;
    }
    
    wxSize size = event.GetSize();
    if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
        LOG_WRN_S("onSize: Invalid size " + std::to_string(size.GetWidth()) + "x" + std::to_string(size.GetHeight()));
        event.Skip();
        return;
    }
    
    performViewAll();
    event.Skip();
}

void DisplayModePreviewCanvas::onEraseBackground(wxEraseEvent& event) {
}

void DisplayModePreviewCanvas::onMouseEvent(wxMouseEvent& event) {
    if (!m_initialized || !m_camera) {
        event.Skip();
        return;
    }
    
    if (event.GetEventType() == wxEVT_LEFT_DOWN) {
        m_mouseDown = true;
        m_lastMousePos = event.GetPosition();
        CaptureMouse();
        SetFocus();
    } else if (event.GetEventType() == wxEVT_LEFT_UP) {
        m_mouseDown = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    } else if (event.GetEventType() == wxEVT_MOTION && m_mouseDown && event.Dragging()) {
        wxPoint pos = event.GetPosition();
        int dx = pos.x - m_lastMousePos.x;
        int dy = pos.y - m_lastMousePos.y;
        
        if (dx != 0 || dy != 0) {
            SoPerspectiveCamera* cam = static_cast<SoPerspectiveCamera*>(m_camera);
            if (cam) {
                SetCurrent(*m_glContext);
                
                SbVec3f cameraPos = cam->position.getValue();
                float focalDist = cam->focalDistance.getValue();
                
                SbVec3f viewDir;
                cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), viewDir);
                SbVec3f focalPoint = cameraPos + viewDir * focalDist;
                
                float rotY = -dx * 0.01f;
                float rotX = -dy * 0.01f;
                
                SbVec3f rightVec, upVec;
                cam->orientation.getValue().multVec(SbVec3f(1, 0, 0), rightVec);
                cam->orientation.getValue().multVec(SbVec3f(0, 1, 0), upVec);
                
                SbRotation rotXAxis(rightVec, rotX);
                SbRotation rotYAxis(upVec, rotY);
                
                SbRotation newOrientation = rotYAxis * cam->orientation.getValue() * rotXAxis;
                cam->orientation.setValue(newOrientation);
                
                SbVec3f newViewDir;
                newOrientation.multVec(SbVec3f(0, 0, -1), newViewDir);
                SbVec3f newCameraPos = focalPoint - newViewDir * focalDist;
                cam->position.setValue(newCameraPos);
            }
            
            m_lastMousePos = pos;
            m_needsRedraw = true;
            Refresh();
        }
    } else if (event.GetEventType() == wxEVT_MOUSEWHEEL) {
        int wheelRotation = event.GetWheelRotation();
        if (wheelRotation != 0) {
            SoPerspectiveCamera* cam = static_cast<SoPerspectiveCamera*>(m_camera);
            if (cam) {
                SetCurrent(*m_glContext);
                
                SbVec3f cameraPos = cam->position.getValue();
                float focalDist = cam->focalDistance.getValue();
                
                SbVec3f viewDir;
                cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), viewDir);
                SbVec3f focalPoint = cameraPos + viewDir * focalDist;
                
                float zoomFactor = 1.0f - (wheelRotation * 0.001f);
                float newFocalDist = focalDist * zoomFactor;
                
                const float MIN_FOCAL_DIST = 0.1f;
                const float MAX_FOCAL_DIST = 10000.0f;
                
                if (newFocalDist < MIN_FOCAL_DIST) {
                    newFocalDist = MIN_FOCAL_DIST;
                } else if (newFocalDist > MAX_FOCAL_DIST) {
                    newFocalDist = MAX_FOCAL_DIST;
                }
                
                SbVec3f newCameraPos = focalPoint - viewDir * newFocalDist;
                cam->position.setValue(newCameraPos);
                cam->focalDistance.setValue(newFocalDist);
                
                float farDist = std::max(newFocalDist * 10.0f, 100000.0f);
                cam->farDistance.setValue(farDist);
            }
            
            m_needsRedraw = true;
            Refresh();
        }
    }
    
    event.Skip();
}
