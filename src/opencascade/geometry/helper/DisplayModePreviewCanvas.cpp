#include "opencascade/geometry/helper/DisplayModePreviewCanvas.h"
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
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoType.h>
#include <GL/gl.h>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <functional>

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
    m_pointViewBuilder = std::make_unique<helper::PointViewBuilder>();
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
    if (m_geometryRoot) {
        m_geometryRoot->unref();
    }
    if (m_camera) {
        m_camera->unref();
    }
    if (m_material) {
        m_material->unref();
    }
    if (m_drawStyle) {
        m_drawStyle->unref();
    }
    if (m_lightModel) {
        m_lightModel->unref();
    }
    if (m_shapeHints) {
        m_shapeHints->unref();
    }
    if (m_polygonOffset) {
        m_polygonOffset->unref();
    }
    if (m_surfaceSwitch) {
        m_surfaceSwitch->unref();
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
}

void DisplayModePreviewCanvas::setupCamera() {
    m_camera = new SoPerspectiveCamera;
    m_camera->ref();
    
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
    m_lightModel->ref();
    m_sceneRoot->addChild(m_lightModel);
}

void DisplayModePreviewCanvas::setupMaterial() {
    // Note: Material, DrawStyle, and ShapeHints will be added to m_surfaceNode
    // in createGeometry() to ensure correct rendering order relative to geometry
    m_material = new SoMaterial;
    m_material->ref();
    
    m_drawStyle = new SoDrawStyle;
    m_drawStyle->ref();
    
    m_shapeHints = new SoShapeHints;
    m_shapeHints->ref();
}

void DisplayModePreviewCanvas::createGeometry() {
    m_geometryRoot = new SoSeparator;
    m_geometryRoot->ref();
    
    m_surfaceNode = new SoSeparator;
    m_surfaceNode->ref();
    m_geometryRoot->addChild(m_surfaceNode);
    
    // Add rendering state nodes to surface node in correct order
    // Order is critical for proper transparency rendering:
    // LightModel -> DrawStyle -> Material -> ShapeHints -> PolygonOffset -> Geometry
    // Note: LightModel is already in scene root, so we add the rest here
    if (m_drawStyle) {
        m_surfaceNode->addChild(m_drawStyle);
    }
    if (m_material) {
        m_surfaceNode->addChild(m_material);
    }
    if (m_shapeHints) {
        m_surfaceNode->addChild(m_shapeHints);
    }
    
    // Create polygon offset node and add it to surface node
    m_polygonOffset = new SoPolygonOffset;
    m_polygonOffset->ref();
    m_surfaceNode->addChild(m_polygonOffset);
    
    m_surfaceSwitch = new SoSwitch;
    m_surfaceSwitch->ref();
    m_surfaceSwitch->addChild(m_geometryRoot);
    m_sceneRoot->addChild(m_surfaceSwitch);
    
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
        m_meshParams.deflection = 0.5;
        m_meshParams.angularDeflection = 0.5;
        
        auto& manager = RenderingToolkitAPI::getManager();
        auto processor = manager.getGeometryProcessor("OpenCASCADE");
        if (!processor) {
            LOG_ERR_S("OpenCASCADE geometry processor not available");
            return;
        }
        
        TriangleMesh mesh = processor->convertToMesh(shape, m_meshParams);
        if (mesh.vertices.empty()) {
            LOG_ERR_S("Failed to convert STEP geometry to mesh");
            return;
        }
        
        m_mesh = new TriangleMesh(std::move(mesh));
        LOG_INF_S("Mesh created: " + std::to_string(m_mesh->vertices.size()) + " vertices, " + std::to_string(m_mesh->triangles.size() / 3) + " triangles");
        
        LOG_INF_S("Converting mesh to Coin3D...");
        SoSeparator* stepGeometry = OCCBrepConverter::convertToCoin3D(shape, m_meshParams.deflection);
        
        if (!stepGeometry) {
            LOG_ERR_S("Failed to convert STEP geometry to Coin3D");
            return;
        }
        
        // CRITICAL: Remove Material and ShapeHints nodes from geometry to prevent them from overriding our settings
        // The geometry node created by Coin3DBackendImpl contains its own Material and ShapeHints nodes,
        // which would override the nodes we add to m_surfaceNode
        // We need to remove these nodes so our Material and ShapeHints nodes take effect
        std::function<void(SoSeparator*)> removeMaterialAndShapeHints = [&](SoSeparator* sep) {
            if (!sep) return;
            // Remove from end to avoid index shifting issues
            for (int i = sep->getNumChildren() - 1; i >= 0; --i) {
                SoNode* child = sep->getChild(i);
                if (!child) continue;
                
                // Remove Material nodes
                if (child->isOfType(SoMaterial::getClassTypeId())) {
                    sep->removeChild(i);
                    LOG_INF_S("Removed Material node from geometry to allow material override");
                }
                // Remove ShapeHints nodes (we have our own)
                else if (child->isOfType(SoShapeHints::getClassTypeId())) {
                    sep->removeChild(i);
                    LOG_INF_S("Removed ShapeHints node from geometry to use our own");
                }
                // Recursively check nested Separators
                else if (child->isOfType(SoSeparator::getClassTypeId())) {
                    removeMaterialAndShapeHints(static_cast<SoSeparator*>(child));
                }
            }
        };
        removeMaterialAndShapeHints(stepGeometry);
        
        m_surfaceNode->addChild(stepGeometry);
        LOG_INF_S("Surface geometry added to scene with polygon offset");

        // Extract topological edges from the BRep shape for non-HiddenLine modes
        // HiddenLine mode will use mesh edges instead for better performance
        if (m_edgeComponent) {
            LOG_INF_S("Extracting topological edges from BRep shape for preview...");

            // Extract original edges (topological edges from BRep)
            m_edgeComponent->extractOriginalEdges(
                shape,
                80.0,  // sampling density
                0.01,  // minimum length
                false, // show lines only
                Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB), // black edges
                1.0,  // width
                false, // don't highlight intersection nodes
                Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), // red intersection nodes
                3.0   // intersection node size
            );

            // Initially disable all edge types - they will be enabled based on display mode
            m_edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
            m_edgeComponent->setEdgeDisplayType(EdgeType::Mesh, false);
            m_edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
            m_edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
            m_edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
            m_edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
            m_edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);

            LOG_INF_S("Topological edges extracted successfully for preview");
        }

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
            wxSize size = GetSize();
            if (size.GetWidth() > 0 && size.GetHeight() > 0) {
                performViewAll();
            } else {
                CallAfter([this]() {
                    performViewAll();
                });
            }
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
    
    const auto& rendering = config.rendering;
    const auto& matOverride = rendering.materialOverride;
    
    // 1. Light Model
    // In Coin3D, parent nodes override child nodes by default in the scene graph
    // Setting the value here will override any child node's light model
    if (rendering.lightModel == DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR) {
        m_lightModel->model.setValue(SoLightModel::BASE_COLOR);
    } else {
        m_lightModel->model.setValue(SoLightModel::PHONG);
    }
    
    // 2. Material Override Logic with caching
    // Cache material state to avoid unnecessary setValue calls when switching between modes
    bool materialChanged = false;

    if (matOverride.enabled) {
        Standard_Real r, g, b;

        matOverride.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        float diffuse[3] = {(float)r, (float)g, (float)b};

        matOverride.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
        float ambient[3] = {(float)r, (float)g, (float)b};

        matOverride.specularColor.Values(r, g, b, Quantity_TOC_RGB);
        float specular[3] = {(float)r, (float)g, (float)b};

        matOverride.emissiveColor.Values(r, g, b, Quantity_TOC_RGB);
        float emissive[3] = {(float)r, (float)g, (float)b};

        // Check if material state has changed
        if (!m_materialCache.cached ||
            m_materialCache.enabled != true ||
            memcmp(m_materialCache.diffuse, diffuse, sizeof(diffuse)) != 0 ||
            memcmp(m_materialCache.ambient, ambient, sizeof(ambient)) != 0 ||
            memcmp(m_materialCache.specular, specular, sizeof(specular)) != 0 ||
            memcmp(m_materialCache.emissive, emissive, sizeof(emissive)) != 0 ||
            m_materialCache.shininess != (float)matOverride.shininess ||
            m_materialCache.transparency != (float)matOverride.transparency) {

            materialChanged = true;

            m_material->diffuseColor.setValue(diffuse[0], diffuse[1], diffuse[2]);
            m_material->ambientColor.setValue(ambient[0], ambient[1], ambient[2]);
            m_material->specularColor.setValue(specular[0], specular[1], specular[2]);
            m_material->emissiveColor.setValue(emissive[0], emissive[1], emissive[2]);
            m_material->shininess.setValue((float)matOverride.shininess);
            m_material->transparency.setValue((float)matOverride.transparency);

            // Update cache
            m_materialCache.cached = true;
            m_materialCache.enabled = true;
            memcpy(m_materialCache.diffuse, diffuse, sizeof(diffuse));
            memcpy(m_materialCache.ambient, ambient, sizeof(ambient));
            memcpy(m_materialCache.specular, specular, sizeof(specular));
            memcpy(m_materialCache.emissive, emissive, sizeof(emissive));
            m_materialCache.shininess = (float)matOverride.shininess;
            m_materialCache.transparency = (float)matOverride.transparency;

            LOG_INF_S("updateGeometryFromConfig: Material override enabled and updated, transparency=" +
                     std::to_string(matOverride.transparency));
        }
    } else {
        // Check if default material state has changed
        const float defaultDiffuse[3] = {0.5f, 0.5f, 0.6f};
        const float defaultAmbient[3] = {0.3f, 0.3f, 0.4f};
        const float defaultSpecular[3] = {1.0f, 1.0f, 1.0f};
        const float defaultEmissive[3] = {0.0f, 0.0f, 0.0f};
        const float defaultShininess = 50.0f;
        const float defaultTransparency = 0.0f;

        if (!m_materialCache.cached ||
            m_materialCache.enabled != false ||
            memcmp(m_materialCache.diffuse, defaultDiffuse, sizeof(defaultDiffuse)) != 0 ||
            memcmp(m_materialCache.ambient, defaultAmbient, sizeof(defaultAmbient)) != 0 ||
            memcmp(m_materialCache.specular, defaultSpecular, sizeof(defaultSpecular)) != 0 ||
            memcmp(m_materialCache.emissive, defaultEmissive, sizeof(defaultEmissive)) != 0 ||
            m_materialCache.shininess != defaultShininess ||
            m_materialCache.transparency != defaultTransparency) {

            materialChanged = true;

            m_material->diffuseColor.setValue(defaultDiffuse[0], defaultDiffuse[1], defaultDiffuse[2]);
            m_material->ambientColor.setValue(defaultAmbient[0], defaultAmbient[1], defaultAmbient[2]);
            m_material->specularColor.setValue(defaultSpecular[0], defaultSpecular[1], defaultSpecular[2]);
            m_material->emissiveColor.setValue(defaultEmissive[0], defaultEmissive[1], defaultEmissive[2]);
            m_material->shininess.setValue(defaultShininess);
            m_material->transparency.setValue(defaultTransparency);

            // Update cache
            m_materialCache.cached = true;
            m_materialCache.enabled = false;
            memcpy(m_materialCache.diffuse, defaultDiffuse, sizeof(defaultDiffuse));
            memcpy(m_materialCache.ambient, defaultAmbient, sizeof(defaultAmbient));
            memcpy(m_materialCache.specular, defaultSpecular, sizeof(defaultSpecular));
            memcpy(m_materialCache.emissive, defaultEmissive, sizeof(defaultEmissive));
            m_materialCache.shininess = defaultShininess;
            m_materialCache.transparency = defaultTransparency;
        }
    }

    if (!materialChanged) {
        LOG_INF_S("updateGeometryFromConfig: Material state unchanged, skipping update");
    }
    
    // 3. Polygon Offset logic
    if (m_polygonOffset) {
        m_polygonOffset->styles = SoPolygonOffset::FILLED;
        m_polygonOffset->factor.setValue((float)config.postProcessing.polygonOffset.factor);
        m_polygonOffset->units.setValue((float)config.postProcessing.polygonOffset.units);
        m_polygonOffset->on.setValue(config.postProcessing.polygonOffset.enabled);
    }
    
    // Configure SoShapeHints for optimal rendering performance
    // Optimize based on transparency and geometry type for better performance
    double transparency = config.rendering.materialOverride.enabled
        ? config.rendering.materialOverride.transparency
        : 0.0;

    if (m_shapeHints) {
        if (transparency > 0.0) {
            // For transparent objects, use UNKNOWN settings to let Coin3D handle face culling
            // This is necessary for correct transparency rendering
            m_shapeHints->faceType.setValue(SoShapeHints::UNKNOWN_FACE_TYPE);
            m_shapeHints->vertexOrdering.setValue(SoShapeHints::UNKNOWN_ORDERING);

            LOG_INF_S("updateGeometryFromConfig: Transparency enabled (" + std::to_string(transparency) +
                     "), using UNKNOWN face type for transparency");
        } else {
            // For opaque objects, optimize based on rendering mode
            if (m_currentMode == RenderingConfig::DisplayMode::Transparent) {
                // Even in Transparent mode, if transparency is 0, we can use optimized settings
                // This provides better performance for non-transparent geometry in Transparent mode
                m_shapeHints->faceType.setValue(SoShapeHints::SOLID);
                m_shapeHints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);

                LOG_INF_S("updateGeometryFromConfig: Transparent mode with no transparency, using SOLID face type for performance");
            } else {
                // For other opaque modes, use standard SOLID settings for best performance
                m_shapeHints->faceType.setValue(SoShapeHints::SOLID);
                m_shapeHints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);

                LOG_INF_S("updateGeometryFromConfig: Opaque mode, using SOLID face type for performance");
            }
        }
    }
    
    // Configure DrawStyle based on DrawStyle selection for single geometry with multiple render passes
    // This new architecture uses DrawStyle to control what gets rendered in multi-pass approach
    if (m_drawStyle) {
        // Get DrawStyle selection from config (this should come from DisplayModeConfigDialog)
        int drawStyleIndex = 0; // Default to FILLED
        // Note: The actual drawStyle selection logic should be moved to DisplayModeConfigDialog
        // For now, we determine it from the node requirements

        bool showSurface = config.nodes.requireSurface;
        bool showEdges = config.nodes.requireOriginalEdges || config.nodes.requireMeshEdges;
        bool showPoints = config.nodes.requirePoints;

        // For single geometry approach, we use DrawStyle to determine base rendering mode
        // The actual multi-pass rendering is handled in onPaint()
        if (showPoints && !showEdges && !showSurface) {
            // Points only mode - single pass
            m_drawStyle->style.setValue(SoDrawStyle::POINTS);
            LOG_INF_S("updateGeometryFromConfig: DrawStyle set to POINTS (single pass)");
            m_showSurface = false;
            m_showEdges = false;
            m_showPoints = true;
        } else if (showEdges && !showPoints && !showSurface) {
            // Edges only mode - single pass
            m_drawStyle->style.setValue(SoDrawStyle::LINES);
            LOG_INF_S("updateGeometryFromConfig: DrawStyle set to LINES (single pass)");
            m_showSurface = false;
            m_showEdges = true;
            m_showPoints = false;
        } else if (showSurface && !showEdges && !showPoints) {
            // Surface only mode - single pass
            m_drawStyle->style.setValue(SoDrawStyle::FILLED);
            LOG_INF_S("updateGeometryFromConfig: DrawStyle set to FILLED (single pass)");
            m_showSurface = true;
            m_showEdges = false;
            m_showPoints = false;
        } else {
            // Multi-pass modes: surface + edges, surface + points, edges + points, or all three
            // Base DrawStyle is set to FILLED, actual rendering handled in onPaint()
            m_drawStyle->style.setValue(SoDrawStyle::FILLED);
            LOG_INF_S("updateGeometryFromConfig: DrawStyle set to FILLED (multi-pass mode)");
            m_showSurface = showSurface;
            m_showEdges = showEdges;
            m_showPoints = showPoints;
        }

        LOG_INF_S("updateGeometryFromConfig: Multi-pass config - surface=" +
                  std::string(m_showSurface ? "true" : "false") +
                  ", edges=" + std::string(m_showEdges ? "true" : "false") +
                  ", points=" + std::string(m_showPoints ? "true" : "false"));

    // Configure edge display based on display mode
    // HiddenLine mode uses mesh edges, other modes use topological edges
    if (m_edgeComponent && m_showEdges) {
        RenderingConfig::DisplayMode currentMode = m_currentMode;

        if (currentMode == RenderingConfig::DisplayMode::HiddenLine) {
            // HiddenLine mode: use silhouette edges for clean outline display (FreeCAD style)
            LOG_INF_S("updateGeometryFromConfig: HiddenLine mode - extracting silhouette edges");

            // Extract silhouette edges when in HiddenLine mode (they depend on camera position)
            // Re-extract if camera position has changed or if silhouette edges don't exist
            if (!m_shape.IsNull()) {
                // Calculate camera position for silhouette detection
                SbVec3f cameraPos;
                if (m_camera) {
                    cameraPos = m_camera->position.getValue();
                } else {
                    cameraPos = SbVec3f(0, 0, 10); // Default camera position
                }

                // Check if camera position has changed significantly (more than 1% of distance)
                bool cameraChanged = false;
                if (m_silhouetteNeedsUpdate || !m_edgeComponent->getEdgeNode(EdgeType::Silhouette)) {
                    cameraChanged = true;
                } else {
                    SbVec3f diff = cameraPos - m_lastSilhouetteCameraPos;
                    float distance = diff.length();
                    float cameraDistance = cameraPos.length();
                    // Re-extract if camera moved more than 1% of its distance from origin
                    if (distance > cameraDistance * 0.01f) {
                        cameraChanged = true;
                    }
                }

                if (cameraChanged) {
                    gp_Pnt cameraPoint(cameraPos[0], cameraPos[1], cameraPos[2]);

                    // Extract silhouette edges with camera position
                    Quantity_Color silhouetteColor = config.edges.silhouetteEdge.color;
                    m_edgeComponent->extractSilhouetteEdges(m_shape, cameraPoint, silhouetteColor, config.edges.silhouetteEdge.width);
                    
                    // Update tracked camera position
                    m_lastSilhouetteCameraPos = cameraPos;
                    m_silhouetteNeedsUpdate = false;
                    
                    LOG_INF_S("updateGeometryFromConfig: Silhouette edges extracted for HiddenLine mode (camera pos: " +
                             std::to_string(cameraPos[0]) + ", " + std::to_string(cameraPos[1]) + ", " + std::to_string(cameraPos[2]) + ")");
                }
            }

            // Enable silhouette edges for HiddenLine
            m_edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, true);
            m_edgeComponent->setEdgeDisplayType(EdgeType::Mesh, false);
            m_edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
        } else {
            // Other modes: use topological edges for accuracy
            LOG_INF_S("updateGeometryFromConfig: Non-HiddenLine mode - using topological edges");

            // Enable original topological edges
            m_edgeComponent->setEdgeDisplayType(EdgeType::Original, true);
            m_edgeComponent->setEdgeDisplayType(EdgeType::Mesh, false);

            // Apply appearance settings for topological edges
            if (m_edgeComponent->getEdgeNode(EdgeType::Original)) {
                m_edgeComponent->applyAppearanceToEdgeNode(EdgeType::Original,
                    config.edges.originalEdge.color,
                    config.edges.originalEdge.width,
                    0);
                LOG_INF_S("updateGeometryFromConfig: Applied appearance to topological edges");
            }
        }

        // Disable other edge types (but preserve Silhouette for HiddenLine mode)
        m_edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        m_edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        if (currentMode != RenderingConfig::DisplayMode::HiddenLine) {
            m_edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        }
        m_edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        m_edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
    }
    }

    // Configure PolygonOffset for multi-pass rendering
    // In multi-pass approach, we use polygon offset to prevent Z-fighting and create depth separation
    // The actual offset values are controlled dynamically in onPaint() for each pass
    if (m_polygonOffset) {
        // Disable polygon offset by default - it will be enabled per-pass in onPaint()
        m_polygonOffset->on.setValue(false);
        // Set default values that will be overridden in onPaint()
        m_polygonOffset->factor.setValue(0.0f);
        m_polygonOffset->units.setValue(0.0f);
        m_polygonOffset->styles = SoPolygonOffset::FILLED;
    }

    m_needsRedraw = true;

    int surfaceSwitchValue = config.nodes.requireSurface || config.nodes.requireOriginalEdges ||
                           config.nodes.requireMeshEdges || config.nodes.requirePoints ? 0 : -1;
    m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("updateGeometryFromConfig: Surface switch set to " + std::to_string(surfaceSwitchValue) +
              " (single geometry with DrawStyle control)");
    
    m_needsRedraw = true;
    Refresh();

    // Note: performViewAll will be called when the dialog is shown and sized properly
    // No need to call it here as it causes excessive updates during initialization
}

void DisplayModePreviewCanvas::updateDisplayMode(RenderingConfig::DisplayMode mode, const DisplayModeConfig& config) {
    LOG_INF_S("DisplayModePreviewCanvas: updateDisplayMode called for mode " + std::to_string(static_cast<int>(mode)));
    m_currentMode = mode;
    m_currentConfig = config;
    updateGeometryFromConfig(config);
    m_needsRedraw = true;
    Refresh();
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
    
    // Multi-pass rendering for single geometry with DrawStyle control
    // This new architecture renders surface, edges, and points in separate passes

    SbViewportRegion vpRegion(size.GetWidth(), size.GetHeight());

    // Determine what needs to be rendered based on configuration
    bool renderSurface = m_showSurface;
    bool renderEdges = m_showEdges;
    bool renderPoints = m_showPoints;

    LOG_INF_S("onPaint: Multi-pass rendering - surface=" + std::string(renderSurface ? "true" : "false") +
              ", edges=" + std::string(renderEdges ? "true" : "false") +
              ", points=" + std::string(renderPoints ? "true" : "false"));

    // Pass 1: Render surface (if enabled)
    if (renderSurface) {
        // Set DrawStyle for filled rendering
        if (m_drawStyle) {
            m_drawStyle->style.setValue(SoDrawStyle::FILLED);
        }

        // Disable polygon offset for surface pass
        if (m_polygonOffset) {
            m_polygonOffset->on.setValue(false);
        }

        // Check if transparency is enabled
        double transparency = m_currentConfig.rendering.materialOverride.enabled
            ? m_currentConfig.rendering.materialOverride.transparency : 0.0;
        bool hasTransparency = transparency > 0.0;

        // Save current OpenGL state for transparency rendering
        GLboolean depthWriteEnabled = GL_TRUE;
        GLboolean blendEnabled = GL_FALSE;
        GLboolean cullFaceEnabled = GL_FALSE;
        if (hasTransparency) {
            glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
            blendEnabled = glIsEnabled(GL_BLEND);
            cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

            // For transparent objects: disable depth writing but keep depth reading enabled
            // Depth testing must remain enabled so that front faces correctly occlude back faces
            // This is critical for proper transparency rendering where front faces should blend
            // over back faces based on depth
            glDepthMask(GL_FALSE);  // Don't write depth, but depth test is still active
            glEnable(GL_DEPTH_TEST);  // Ensure depth test is enabled
            glDepthFunc(GL_LEQUAL);   // Standard depth function

            // Disable face culling for transparent objects so both front and back faces are rendered
            // This is necessary for proper transparency where we need to see through the object
            glDisable(GL_CULL_FACE);

            // Enable blending for transparency
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            // For opaque objects: enable depth writing for proper depth testing
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDisable(GL_BLEND);
            // Face culling can be enabled for opaque objects for better performance
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }

        SoGLRenderAction surfaceAction(vpRegion);
        surfaceAction.setSmoothing(true);
        surfaceAction.setNumPasses(1);
        surfaceAction.setTransparencyType(SoGLRenderAction::NONE);

        // Handle transparency if enabled
        if (hasTransparency) {
            // Use SORTED_OBJECT_SORTED_TRIANGLE_BLEND for more accurate depth sorting within a single object
            // This ensures that front faces correctly occlude back faces even within the same object
            // SORTED_OBJECT_BLEND only sorts between objects, not triangles within an object
            surfaceAction.setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);
            // Increase passes for better triangle sorting quality
            surfaceAction.setNumPasses(3);
        }

        surfaceAction.apply(m_sceneRoot);

        // Restore OpenGL state if transparency was used
        if (hasTransparency) {
            glDepthMask(depthWriteEnabled);
            if (!blendEnabled) {
                glDisable(GL_BLEND);
            }
            if (cullFaceEnabled) {
                glEnable(GL_CULL_FACE);
            }
        }

        LOG_INF_S("onPaint: Surface pass completed (transparency=" + std::to_string(transparency) + ")");
    }

    // Pass 2: Render edges (if enabled)
    if (renderEdges) {
        // Ensure depth writing is enabled for edges so they render on top of transparent surfaces
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);  // Edges should be opaque

        RenderingConfig::DisplayMode currentMode = m_currentMode;

        if (currentMode == RenderingConfig::DisplayMode::HiddenLine) {
            // HiddenLine mode: Render silhouette edges in single pass (FreeCAD style)
            LOG_INF_S("onPaint: Rendering HiddenLine silhouette edges in single pass");

            // Check if we have silhouette edges to display
            bool hasSilhouetteEdges = false;
            SoSeparator* silhouetteEdges = nullptr;
            if (m_edgeComponent) {
                silhouetteEdges = m_edgeComponent->getEdgeNode(EdgeType::Silhouette);
                hasSilhouetteEdges = (silhouetteEdges && m_edgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Silhouette));
            }

            if (hasSilhouetteEdges) {
                // Create a temporary separator for silhouette edges
                SoSeparator* silhouetteSeparator = new SoSeparator;
                silhouetteSeparator->ref();

                // Add polygon offset for edges to bring them forward (shrink effect)
                SoPolygonOffset* edgeOffset = new SoPolygonOffset;
                edgeOffset->factor.setValue(-1.0f);
                edgeOffset->units.setValue(-1.0f);
                edgeOffset->styles = SoPolygonOffset::LINES;
                edgeOffset->on.setValue(true);
                silhouetteSeparator->addChild(edgeOffset);

                // Add silhouette edge node
                silhouetteSeparator->addChild(silhouetteEdges);

                SoGLRenderAction silhouetteAction(vpRegion);
                silhouetteAction.setSmoothing(true);
                silhouetteAction.setNumPasses(1);
                silhouetteAction.setTransparencyType(SoGLRenderAction::NONE);

                // Create a minimal scene with just the silhouette edges
                SoSeparator* edgeScene = new SoSeparator;
                edgeScene->ref();

                // Add camera and lighting to edge scene
                if (m_camera) {
                    edgeScene->addChild(m_camera);
                }
                if (m_sceneRoot) {
                    // Find and add the light from the main scene
                    for (int i = 0; i < m_sceneRoot->getNumChildren(); ++i) {
                        SoNode* child = m_sceneRoot->getChild(i);
                        if (child && child->isOfType(SoDirectionalLight::getClassTypeId())) {
                            edgeScene->addChild(child);
                            break;
                        }
                    }
                }

                // Add the silhouette separator to the scene
                edgeScene->addChild(silhouetteSeparator);

                silhouetteAction.apply(edgeScene);

                // Clean up
                edgeScene->unref();
                silhouetteSeparator->unref();

                LOG_INF_S("onPaint: HiddenLine silhouette edges rendered successfully");
            } else {
                LOG_WRN_S("onPaint: No silhouette edges available for HiddenLine mode");
            }
        } else {
            // Other modes: Use topological edges from ModularEdgeComponent
            LOG_INF_S("onPaint: Rendering topological edges using ModularEdgeComponent");

            // Check if we have edges to display first
            bool hasEdgesToRender = false;
            if (m_edgeComponent) {
                SoSeparator* originalEdges = m_edgeComponent->getEdgeNode(EdgeType::Original);
                SoSeparator* meshEdges = m_edgeComponent->getEdgeNode(EdgeType::Mesh);
                hasEdgesToRender = (originalEdges && m_edgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Original)) ||
                                   (meshEdges && m_edgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Mesh));
            }

            if (hasEdgesToRender) {
                // Create a temporary separator for edges to apply polygon offset
                SoSeparator* edgeSeparator = new SoSeparator;
                edgeSeparator->ref();  // Ref it immediately to manage its lifetime

                // Add polygon offset for edges to bring them forward (shrink effect)
                if (m_polygonOffset) {
                    // Clone polygon offset settings for edges
                    SoPolygonOffset* edgeOffset = new SoPolygonOffset;
                    edgeOffset->factor.setValue(-1.0f);  // Negative offset to bring forward
                    edgeOffset->units.setValue(-1.0f);
                    edgeOffset->styles = SoPolygonOffset::LINES;
                    edgeOffset->on.setValue(true);
                    edgeSeparator->addChild(edgeOffset);  // addChild automatically refs edgeOffset
                }

                // Add edge nodes from ModularEdgeComponent
                if (m_edgeComponent) {
                    SoSeparator* originalEdges = m_edgeComponent->getEdgeNode(EdgeType::Original);
                    SoSeparator* meshEdges = m_edgeComponent->getEdgeNode(EdgeType::Mesh);

                    if (originalEdges && m_edgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Original)) {
                        edgeSeparator->addChild(originalEdges);
                        LOG_INF_S("onPaint: Added original topological edges to render");
                    }

                    if (meshEdges && m_edgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Mesh)) {
                        edgeSeparator->addChild(meshEdges);
                        LOG_INF_S("onPaint: Added mesh edges to render");
                    }
                }

                SoGLRenderAction edgesAction(vpRegion);
                edgesAction.setSmoothing(true);
                edgesAction.setNumPasses(1);
                edgesAction.setTransparencyType(SoGLRenderAction::NONE);

                // Create a minimal scene with just the edges
                SoSeparator* edgeScene = new SoSeparator;
                edgeScene->ref();

                // Add camera and lighting to edge scene
                if (m_camera) {
                    edgeScene->addChild(m_camera);
                }
                if (m_sceneRoot) {
                    // Find and add the light from the main scene
                    for (int i = 0; i < m_sceneRoot->getNumChildren(); ++i) {
                        SoNode* child = m_sceneRoot->getChild(i);
                        if (child && child->isOfType(SoDirectionalLight::getClassTypeId())) {
                            edgeScene->addChild(child);
                            break;
                        }
                    }
                }

                // Add the edge separator to the scene
                // Note: addChild will automatically ref edgeSeparator (now ref count = 2)
                edgeScene->addChild(edgeSeparator);

                edgesAction.apply(edgeScene);

                // Unref edgeScene - this will automatically unref all children including edgeSeparator
                // After this, edgeSeparator ref count = 1 (our ref)
                edgeScene->unref();

                // Now unref edgeSeparator - this will unref all its children and delete it
                edgeSeparator->unref();

                LOG_INF_S("onPaint: Topological edges rendered successfully");
            } else {
                LOG_INF_S("onPaint: No topological edges to render");
            }

            LOG_INF_S("onPaint: Topological edges pass completed");
        }
    }

    // Pass 3: Render points (if enabled)
    if (renderPoints) {
        // Ensure depth writing is enabled for points so they render on top of transparent surfaces
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);  // Points should be opaque

        // Set DrawStyle for point rendering
        if (m_drawStyle) {
            m_drawStyle->style.setValue(SoDrawStyle::POINTS);
        }

        // Enable polygon offset for points to bring them forward (shrink effect)
        if (m_polygonOffset) {
            m_polygonOffset->on.setValue(true);
            m_polygonOffset->styles = SoPolygonOffset::POINTS;
            // Use negative offset to bring points closer to viewer (shrink effect)
            // This makes points appear to "pop out" from the surface
            m_polygonOffset->factor.setValue(-2.0f);
            m_polygonOffset->units.setValue(-2.0f);
        }

        // Set point color
        SbColor originalDiffuse;
        if (m_material) {
            originalDiffuse = m_material->diffuseColor[0];
            // Use red for points
            m_material->diffuseColor.setValue(1.0f, 0.0f, 0.0f);
        }

        SoGLRenderAction pointsAction(vpRegion);
        pointsAction.setSmoothing(true);
        pointsAction.setNumPasses(1);
        pointsAction.setTransparencyType(SoGLRenderAction::NONE);

        pointsAction.apply(m_sceneRoot);

        // Restore original material color
        if (m_material) {
            m_material->diffuseColor.setValue(originalDiffuse);
        }

        LOG_INF_S("onPaint: Points pass completed");
    }

    // Handle blend modes if needed (for compatibility)
    bool hasBlendMode = m_currentConfig.rendering.blendMode != RenderingConfig::BlendMode::None;
    double transparency = m_currentConfig.rendering.materialOverride.enabled
        ? m_currentConfig.rendering.materialOverride.transparency : 0.0;

    if (hasBlendMode && transparency == 0.0) {
        // Note: Multi-pass rendering doesn't use blend modes in the same way
        // Blend modes are primarily for transparency effects
        LOG_WRN_S("onPaint: Blend modes not fully supported in multi-pass rendering");
    }
    
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
                
                // Mark silhouette edges for update when camera rotates in HiddenLine mode
                if (m_currentMode == RenderingConfig::DisplayMode::HiddenLine) {
                    m_silhouetteNeedsUpdate = true;
                }
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


