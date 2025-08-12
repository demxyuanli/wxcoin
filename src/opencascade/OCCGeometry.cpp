#include "OCCGeometry.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include "config/RenderingConfig.h"
#include "rendering/GeometryProcessor.h"
#include <limits>
#include <cmath>
#include <wx/gdicmn.h>
#include <chrono>
#include <fstream> // Added for file validation
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <TopoDS.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include "EdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "OCCMeshConverter.h"

// OpenCASCADE includes
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <gp_Trsf.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <TopLoc_Location.hxx>
#include <BRepBuilderAPI_Transform.hxx>

// Coin3D includes
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

// OCCGeometry base class implementation
OCCGeometry::OCCGeometry(const std::string& name)
    : m_name(name)
    , m_visible(true)
    , m_selected(false)
    , m_facesVisible(true)
    , m_transparency(0.0)
    , m_wireframeMode(false)
    , m_coinNode(nullptr)
    , m_coinTransform(nullptr)
    , m_coinNeedsUpdate(true)
    , m_meshRegenerationNeeded(true)
    , m_position(0, 0, 0)
    , m_rotationAxis(0, 0, 1)
    , m_rotationAngle(0.0)
    , m_scale(1.0)
    , m_color(0.8, 0.8, 0.8, Quantity_TOC_RGB)
    , m_materialAmbientColor(0.5, 0.5, 0.5, Quantity_TOC_RGB)
    , m_materialDiffuseColor(0.95, 0.95, 0.95, Quantity_TOC_RGB)
    , m_materialSpecularColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
    , m_materialEmissiveColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_materialShininess(50.0)
    , m_textureIntensity(1.0)
    , m_textureEnabled(false)
    , m_textureMode(RenderingConfig::TextureMode::Replace)
    , m_blendMode(RenderingConfig::BlendMode::None)
    , m_depthTest(true)
    , m_depthWrite(true)
    , m_cullFace(true)
    , m_alphaThreshold(0.1)
    , m_smoothNormals(false)
    , m_wireframeWidth(1.0)
    , m_pointSize(1.0)
    , m_subdivisionEnabled(false)
    , m_subdivisionLevels(2)
    , m_materialExplicitlySet(false) // Added flag to track if material was explicitly set
{
    // Get blend settings from configuration
    auto blendSettings = RenderingConfig::getInstance().getBlendSettings();
    m_blendMode = blendSettings.blendMode;
    m_depthTest = blendSettings.depthTest;
    m_depthWrite = blendSettings.depthWrite;
    m_cullFace = blendSettings.cullFace;
    m_alphaThreshold = blendSettings.alphaThreshold;
    
    // Apply settings from RenderingConfig first
    updateFromRenderingConfig();
    
    
    edgeComponent = std::make_unique<EdgeComponent>();
    
    // Ensure edge display is disabled by default to avoid conflicts with new EdgeComponent system
    m_showEdges = false;
    m_showWireframe = false;
}

OCCGeometry::~OCCGeometry()
{
    if (m_coinNode) {
        m_coinNode->unref();
    }
}

void OCCGeometry::setShape(const TopoDS_Shape& shape)
{
    m_shape = shape;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setPosition(const gp_Pnt& position)
{
    m_position = position;
    if (m_coinTransform) {
        m_coinTransform->translation.setValue(
            static_cast<float>(m_position.X()),
            static_cast<float>(m_position.Y()),
            static_cast<float>(m_position.Z())
        );
    }
    else {
        m_coinNeedsUpdate = true;
    }
}

void OCCGeometry::setRotation(const gp_Vec& axis, double angle)
{
    m_rotationAxis = axis;
    m_rotationAngle = angle;
    if (m_coinTransform) {
        SbVec3f rot_axis(
            static_cast<float>(m_rotationAxis.X()),
            static_cast<float>(m_rotationAxis.Y()),
            static_cast<float>(m_rotationAxis.Z())
        );
        m_coinTransform->rotation.setValue(rot_axis, static_cast<float>(m_rotationAngle));
    }
    else {
        m_coinNeedsUpdate = true;
    }
}

void OCCGeometry::setScale(double scale)
{
    m_scale = scale;
    if (m_coinTransform) {
        m_coinTransform->scaleFactor.setValue(
            static_cast<float>(m_scale),
            static_cast<float>(m_scale),
            static_cast<float>(m_scale)
        );
    }
    else {
        m_coinNeedsUpdate = true;
    }
}

void OCCGeometry::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        // Update Coin3D node visibility by toggling faces display
        if (m_coinNode) {
            setFacesVisible(visible);
            m_coinNode->touch();
        } else {
            // Will be applied on next build
            m_coinNeedsUpdate = true;
        }
    }
}

void OCCGeometry::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;

        // Force rebuild of Coin3D representation to update edge colors
        m_coinNeedsUpdate = true;

        if (m_coinNode) {
            // Rebuild the entire Coin3D representation to update edge colors
            buildCoinRepresentation();

            // Force a refresh of the scene to show the selection change
            m_coinNode->touch();
        }
    }
}

void OCCGeometry::setColor(const Quantity_Color& color)
{
    m_color = color;
    m_materialDiffuseColor = color;
    m_materialExplicitlySet = true; // Mark that material has been explicitly set
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setTransparency(double transparency)
{
    m_transparency = std::min(1.0, std::max(0.0, transparency));  // Clamp 0-1
    if (m_wireframeMode) {
        m_transparency = std::min(m_transparency, 0.6);  // Limit for visible lines
    }
    
    // Mark that the Coin representation needs update
    m_coinNeedsUpdate = true;
    
    if (m_coinNode) {
        // Clear old material and texture
        for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
            SoNode* child = m_coinNode->getChild(i);
            if (child && (child->isOfType(SoMaterial::getClassTypeId()) || child->isOfType(SoTexture2::getClassTypeId()))) {
                m_coinNode->removeChild(i);
            }
        }
        // Force rebuild
        buildCoinRepresentation();
        
        // Mark the Coin node as modified to trigger scene graph update
        m_coinNode->touch();
    }
}

void OCCGeometry::setWireframeMode(bool wireframe)
{
    if (m_wireframeMode != wireframe) {
        m_wireframeMode = wireframe;
        m_coinNeedsUpdate = true;
        
        // Force rebuild of Coin3D representation to apply wireframe mode change
        if (m_coinNode) {
            buildCoinRepresentation();
            m_coinNode->touch();
            LOG_INF_S("Wireframe mode changed to " + std::string(wireframe ? "enabled" : "disabled") + " for " + m_name);
        }
    }
}

// Removed setShadingMode method - functionality not needed

// Material property setters
void OCCGeometry::setMaterialAmbientColor(const Quantity_Color& color)
{
    m_materialAmbientColor = color;
    m_materialExplicitlySet = true; // Mark that material has been explicitly set
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialDiffuseColor(const Quantity_Color& color)
{
    m_materialDiffuseColor = color;
    m_materialExplicitlySet = true; // Mark that material has been explicitly set
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialSpecularColor(const Quantity_Color& color)
{
    m_materialSpecularColor = color;
    m_materialExplicitlySet = true; // Mark that material has been explicitly set
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialEmissiveColor(const Quantity_Color& color)
{
    m_materialEmissiveColor = color;
    m_materialExplicitlySet = true; // Mark that material has been explicitly set
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialShininess(double shininess)
{
    m_materialShininess = std::max(0.0, std::min(100.0, shininess));
    m_materialExplicitlySet = true; // Mark that material has been explicitly set
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setDefaultBrightMaterial()
{
    // Set bright material colors for better visibility without textures
    m_materialAmbientColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
    m_materialDiffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    m_materialSpecularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    m_materialShininess = 30.0; // Lower shininess for more diffuse appearance
    
    m_coinNeedsUpdate = true;
    
    if (m_coinNode) {
        // Force rebuild of Coin3D representation to update material
        buildCoinRepresentation();
        m_coinNode->touch();
    }
}

// Texture property setters
void OCCGeometry::setTextureColor(const Quantity_Color& color)
{
    m_textureColor = color;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setTextureIntensity(double intensity)
{
    m_textureIntensity = std::max(0.0, std::min(1.0, intensity));
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setTextureEnabled(bool enabled)
{
    m_textureEnabled = enabled;
    m_coinNeedsUpdate = true;
    
    if (m_coinNode) {
        // Force rebuild of Coin3D representation to apply texture changes
        buildCoinRepresentation();
        m_coinNode->touch();
        LOG_INF_S("Texture enabled set to " + std::to_string(enabled) + " for " + m_name);
    }
}

void OCCGeometry::setTextureImagePath(const std::string& path)
{
    m_textureImagePath = path;
    m_coinNeedsUpdate = true;
    
    if (m_coinNode) {
        // Force rebuild of Coin3D representation to apply texture changes
        buildCoinRepresentation();
        m_coinNode->touch();
        LOG_INF_S("Texture image path set to " + path + " for " + m_name);
    }
}

void OCCGeometry::setTextureMode(RenderingConfig::TextureMode mode)
{
    m_textureMode = mode;
    m_coinNeedsUpdate = true;
    
    if (m_coinNode) {
        // Force rebuild of Coin3D representation to apply texture changes
        buildCoinRepresentation();
        m_coinNode->touch();
        LOG_INF_S("Texture mode set to " + std::to_string(static_cast<int>(mode)) + " for " + m_name);
    }
}

void OCCGeometry::setBlendMode(RenderingConfig::BlendMode mode)
{
    m_blendMode = mode;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setDepthTest(bool enabled)
{
    m_depthTest = enabled;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setDepthWrite(bool enabled)
{
    m_depthWrite = enabled;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setCullFace(bool enabled)
{
    m_cullFace = enabled;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setAlphaThreshold(double threshold)
{
    m_alphaThreshold = threshold;
    m_coinNeedsUpdate = true;
}

SoSeparator* OCCGeometry::getCoinNode()
{
    if (!m_coinNode || m_coinNeedsUpdate) {
        buildCoinRepresentation();
    }

    return m_coinNode;
}

void OCCGeometry::setCoinNode(SoSeparator* node)
{
    if (m_coinNode) {
        m_coinNode->unref();
    }
    m_coinNode = node;
    if (m_coinNode) {
        m_coinNode->ref();
    }
    m_coinNeedsUpdate = false;
}

void OCCGeometry::regenerateMesh(const MeshParameters& params)
{
    buildCoinRepresentation(params);
}

// Performance optimization methods
bool OCCGeometry::needsMeshRegeneration() const
{
    return m_meshRegenerationNeeded;
}

void OCCGeometry::setMeshRegenerationNeeded(bool needed)
{
    m_meshRegenerationNeeded = needed;
}

void OCCGeometry::updateCoinRepresentationIfNeeded(const MeshParameters& params)
{
    // Check if mesh parameters have changed
    bool paramsChanged = (params.deflection != m_lastMeshParams.deflection ||
                         params.angularDeflection != m_lastMeshParams.angularDeflection ||
                         params.relative != m_lastMeshParams.relative ||
                         params.inParallel != m_lastMeshParams.inParallel);
    
    if (m_meshRegenerationNeeded || paramsChanged) {
        buildCoinRepresentation(params);
        m_lastMeshParams = params;
        m_meshRegenerationNeeded = false;
    }
}

void OCCGeometry::buildCoinRepresentation(const MeshParameters& params)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();

    if (m_coinNode) {
        m_coinNode->removeAllChildren();
    }
    else {
        m_coinNode = new SoSeparator;
        m_coinNode->ref();
    }

    // Clean up any existing texture nodes to prevent memory issues
    // This is especially important when switching between different textures
    if (m_coinNode) {
        for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
            SoNode* child = m_coinNode->getChild(i);
            if (child && (child->isOfType(SoTexture2::getClassTypeId()) || 
                         child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
                m_coinNode->removeChild(i);
            }
        }
    }

    // Transform setup
    m_coinTransform = new SoTransform;
    m_coinTransform->translation.setValue(
        static_cast<float>(m_position.X()),
        static_cast<float>(m_position.Y()),
        static_cast<float>(m_position.Z())
    );

    if (m_rotationAngle != 0.0) {
        SbVec3f axis(
            static_cast<float>(m_rotationAxis.X()),
            static_cast<float>(m_rotationAxis.Y()),
            static_cast<float>(m_rotationAxis.Z())
        );
        m_coinTransform->rotation.setValue(axis, static_cast<float>(m_rotationAngle));
    }

    m_coinTransform->scaleFactor.setValue(
        static_cast<float>(m_scale),
        static_cast<float>(m_scale),
        static_cast<float>(m_scale)
    );
    m_coinNode->addChild(m_coinTransform);

    // Shape hints
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    hints->faceType = SoShapeHints::CONVEX;
    m_coinNode->addChild(hints);

    // Draw style (wireframe vs filled)
    SoDrawStyle* drawStyle = new SoDrawStyle;
    drawStyle->style = m_wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
    drawStyle->lineWidth = m_wireframeMode ? 1.0f : 0.0f;
    m_coinNode->addChild(drawStyle);

    // Material
    SoMaterial* material = new SoMaterial;
    if (m_wireframeMode) {
        material->diffuseColor.setValue(0.0f, 0.0f, 1.0f);
        material->transparency.setValue(static_cast<float>(m_transparency));
    } else {
        // Convert Quantity_Color to 0-1 range for Coin3D
        Standard_Real r, g, b;
        m_materialAmbientColor.Values(r, g, b, Quantity_TOC_RGB);
        material->ambientColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        
        m_materialDiffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        material->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        
        m_materialSpecularColor.Values(r, g, b, Quantity_TOC_RGB);
        material->specularColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        
        material->shininess.setValue(static_cast<float>(m_materialShininess / 100.0));
        // Hide faces when requested by pushing transparency to 1.0
        double appliedTransparency = m_facesVisible ? m_transparency : 1.0;
        material->transparency.setValue(static_cast<float>(appliedTransparency));
        
        // Add emissive color for better lighting response
        material->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
        
        LOG_INF_S("Material set for " + m_name + " - ambient: " + 
                 std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + 
                 " diffuse: " + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + 
                 " shininess: " + std::to_string(m_materialShininess) + " transparency: " + std::to_string(m_transparency));
    }
    m_coinNode->addChild(material);

    // Texture support
    if (m_textureEnabled && !m_textureImagePath.empty()) {
        // Validate texture file exists
        std::ifstream fileCheck(m_textureImagePath);
        if (!fileCheck.good()) {
            LOG_WRN_S("Texture file not found or invalid: " + m_textureImagePath + " for " + m_name);
            m_textureEnabled = false; // Disable texture if file is invalid
        } else {
            fileCheck.close();
            
            try {
                // Create texture node
                SoTexture2* texture = new SoTexture2;
                
                // Load texture image - use safer approach
                texture->filename.setValue(m_textureImagePath.c_str());
                
                // Set texture mode based on configuration
                switch (m_textureMode) {
                    case RenderingConfig::TextureMode::Replace:
                        texture->model.setValue(SoTexture2::DECAL);
                        break;
                    case RenderingConfig::TextureMode::Modulate:
                        texture->model.setValue(SoTexture2::MODULATE);
                        break;
                    case RenderingConfig::TextureMode::Blend:
                        texture->model.setValue(SoTexture2::BLEND);
                        break;
                    case RenderingConfig::TextureMode::Decal:
                    default:
                        texture->model.setValue(SoTexture2::DECAL);
                        break;
                }
                
                // Set texture intensity
                Standard_Real tr, tg, tb;
                m_textureColor.Values(tr, tg, tb, Quantity_TOC_RGB);
                texture->blendColor.setValue(static_cast<float>(tr),
                                           static_cast<float>(tg),
                                           static_cast<float>(tb));
                
                // Add texture node first
                m_coinNode->addChild(texture);
                
                // Set texture transformation - use safer approach
                SoTextureCoordinate2* texCoord = new SoTextureCoordinate2;
                
                // Use default texture coordinates (Coin3D will handle this automatically)
                // This avoids potential memory access issues with custom coordinate arrays
                // Coin3D will generate appropriate texture coordinates based on the geometry
                
                // Add texture coordinate node
                m_coinNode->addChild(texCoord);
                
                LOG_INF_S("Texture applied to " + m_name + " - path: " + m_textureImagePath + 
                         " mode: " + std::to_string(static_cast<int>(m_textureMode)));
            } catch (const std::exception& e) {
                LOG_ERR_S("Exception while loading texture for " + m_name + ": " + std::string(e.what()));
                m_textureEnabled = false; // Disable texture on error
            } catch (...) {
                LOG_ERR_S("Unknown exception while loading texture for " + m_name);
                m_textureEnabled = false; // Disable texture on error
            }
        }
    }

    // Blend and depth settings
    if (m_blendMode != RenderingConfig::BlendMode::None && m_transparency > 0.0) {
        // Apply blend settings through material transparency
        material->transparency.setValue(static_cast<float>(m_transparency));
        
        // Set material blend mode through hints
        SoShapeHints* blendHints = new SoShapeHints;
        blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        m_coinNode->addChild(blendHints);
        
        LOG_INF_S("Applied blend mode " + std::to_string(static_cast<int>(m_blendMode)) + 
                 " with transparency " + std::to_string(m_transparency) + " for " + m_name);
    }

    // Face culling settings
    if (m_cullFace) {
        SoShapeHints* cullHints = new SoShapeHints;
        cullHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        cullHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        m_coinNode->addChild(cullHints);
    }

    // Edge visualization is now handled by EdgeComponent
    // Old edge drawing code removed to avoid conflicts with new edge system

    // Add shape to scene (simplified)
    if (!m_shape.IsNull()) {
        if (m_wireframeMode) {
            // In wireframe mode, create wireframe representation directly
            createWireframeRepresentation(params);
        } else {
                    // Use rendering toolkit to create scene node for solid/filled mode
        auto& manager = RenderingToolkitAPI::getManager();
        auto backend = manager.getRenderBackend("Coin3D");
        if (backend) {
            // Use the material-aware version to preserve custom material settings
            auto sceneNode = backend->createSceneNode(m_shape, params, m_selected,
                                                     m_materialDiffuseColor, m_materialAmbientColor,
                                                     m_materialSpecularColor, m_materialEmissiveColor,
                                                     m_materialShininess, m_transparency);
            if (sceneNode && m_facesVisible) {
                SoSeparator* meshNode = sceneNode.get();
                meshNode->ref(); // Take ownership
                m_coinNode->addChild(meshNode);
            }
        }
        }
    }

    // Set visibility
    if (m_coinNode) {
        m_coinNode->renderCulling = m_visible ? SoSeparator::OFF : SoSeparator::ON;
    }
    
    m_coinNeedsUpdate = false;

    auto buildEndTime = std::chrono::high_resolution_clock::now();
    auto buildDuration = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTime - buildStartTime);
    
    // Only log detailed breakdown for debugging or when needed
    if (buildDuration.count() > 100) { // Only log if build takes more than 100ms
        LOG_INF_S("=== COIN3D BUILD BREAKDOWN ===");
        LOG_INF_S("Geometry: " + m_name);
        LOG_INF_S("TOTAL BUILD TIME: " + std::to_string(buildDuration.count()) + "ms");
        LOG_INF_S("==============================");
    }

    // Generate edge nodes for EdgeComponent on demand when any edge type is toggled or if overlay edges are requested
    bool anyEdgeDisplayRequested = false;
    if (edgeComponent) {
        // If edgeComponent already exists, reuse; else create when needed
        const EdgeDisplayFlags& flags = edgeComponent->edgeFlags;
        anyEdgeDisplayRequested = flags.showOriginalEdges || flags.showFeatureEdges ||
                                  flags.showMeshEdges || flags.showHighlightEdges ||
                                  flags.showNormalLines || flags.showFaceNormalLines;
    }
    const EdgeSettingsConfig& edgeCfg = EdgeSettingsConfig::getInstance();
    anyEdgeDisplayRequested = anyEdgeDisplayRequested || edgeCfg.getGlobalSettings().showEdges ||
                              edgeCfg.getSelectedSettings().showEdges ||
                              edgeCfg.getHoverSettings().showEdges;

    if (edgeComponent && anyEdgeDisplayRequested) {
        LOG_INF_S("Generating edge nodes for geometry: " + m_name);
        
        // Generate nodes only for requested types to avoid unnecessary overlays
        if (edgeComponent->edgeFlags.showOriginalEdges) {
            edgeComponent->extractOriginalEdges(m_shape);
        }
        if (edgeComponent->edgeFlags.showFeatureEdges) {
            // Start with permissive parameters for responsiveness
            edgeComponent->extractFeatureEdges(m_shape, 15.0, 0.005, false, false);
        }
        
        // Generate mesh edges and normal lines
        auto& manager = RenderingToolkitAPI::getManager();
        auto processor = manager.getGeometryProcessor("OpenCASCADE");
        if (processor) {
            LOG_INF_S("Converting shape to mesh for edge visualization");
            TriangleMesh mesh = processor->convertToMesh(m_shape, params);
            LOG_INF_S("Mesh conversion result: " + std::to_string(mesh.vertices.size()) + 
                      " vertices, " + std::to_string(mesh.normals.size()) + " normals");
            
            // Generate mesh edges when requested
            if (edgeComponent->edgeFlags.showMeshEdges) {
                edgeComponent->extractMeshEdges(mesh);
            }
            // Generate normal/face-normal lines when requested
            if (edgeComponent->edgeFlags.showNormalLines) {
                LOG_INF_S("Generating normal line node for geometry: " + m_name);
                edgeComponent->generateNormalLineNode(mesh, 0.5);
            }
            if (edgeComponent->edgeFlags.showFaceNormalLines) {
                LOG_INF_S("Generating face normal line node for geometry: " + m_name);
                edgeComponent->generateFaceNormalLineNode(mesh, 0.5);
            }
        } else {
            LOG_WRN_S("OpenCASCADE processor not found for geometry: " + m_name);
        }
        
        // Generate highlight edge node only if requested
        if (edgeComponent->edgeFlags.showHighlightEdges) {
            edgeComponent->generateHighlightEdgeNode();
        }
        
    } else {
        if (!edgeComponent) {
            LOG_WRN_S("EdgeComponent is null for geometry: " + m_name);
        } else {
            LOG_INF_S("Skipping edge generation for geometry (edge display disabled): " + m_name);
        }
    }
}

void OCCGeometry::createWireframeRepresentation(const MeshParameters& params)
{
    if (m_shape.IsNull()) {
        return;
    }

    // Convert shape to mesh for wireframe generation
    auto& manager = RenderingToolkitAPI::getManager();
    auto processor = manager.getGeometryProcessor("OpenCASCADE");
    if (!processor) {
        LOG_WRN_S("OpenCASCADE processor not found for wireframe generation");
        return;
    }

    TriangleMesh mesh = processor->convertToMesh(m_shape, params);
    if (mesh.isEmpty()) {
        LOG_WRN_S("Empty mesh generated for wireframe representation");
        return;
    }

    // Create coordinate node
    SoCoordinate3* coords = new SoCoordinate3;
    std::vector<float> vertices;
    for (const auto& vertex : mesh.vertices) {
        vertices.push_back(static_cast<float>(vertex.X()));
        vertices.push_back(static_cast<float>(vertex.Y()));
        vertices.push_back(static_cast<float>(vertex.Z()));
    }
    coords->point.setValues(0, static_cast<int>(mesh.vertices.size()), 
                           reinterpret_cast<const SbVec3f*>(vertices.data()));
    m_coinNode->addChild(coords);

    // Create wireframe line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    std::vector<int32_t> indices;
    
    // Create wireframe from triangle edges
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v0 = mesh.triangles[i];
        int v1 = mesh.triangles[i + 1];
        int v2 = mesh.triangles[i + 2];
        
        // Add triangle edges
        indices.push_back(v0);
        indices.push_back(v1);
        indices.push_back(SO_END_LINE_INDEX);
        
        indices.push_back(v1);
        indices.push_back(v2);
        indices.push_back(SO_END_LINE_INDEX);
        
        indices.push_back(v2);
        indices.push_back(v0);
        indices.push_back(SO_END_LINE_INDEX);
    }
    
    lineSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
    m_coinNode->addChild(lineSet);

    LOG_INF_S("Created wireframe representation for " + m_name + 
              " with " + std::to_string(mesh.vertices.size()) + " vertices and " + 
              std::to_string(mesh.triangles.size() / 3) + " triangles");
}

// All primitive classes (OCCBox, OCCCylinder, etc.) call setShape(),
// which sets the m_coinNeedsUpdate flag. The representation will be
// built on the next call to getCoinNode().

// OCCBox implementation
OCCBox::OCCBox(const std::string& name, double width, double height, double depth)
    : OCCGeometry(name)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
{
    buildShape();
}

void OCCBox::setDimensions(double width, double height, double depth)
{
    m_width = width;
    m_height = height;
    m_depth = depth;
    buildShape();
}

void OCCBox::getSize(double& width, double& height, double& depth) const
{
    width = m_width;
    height = m_height;
    depth = m_depth;
}

void OCCBox::buildShape()
{
    LOG_INF_S("Building OCCBox shape with dimensions: " + std::to_string(m_width) + " x " + std::to_string(m_height) + " x " + std::to_string(m_depth));

    try {
        // Use the simplest constructor that takes dimensions only
        BRepPrimAPI_MakeBox boxMaker(m_width, m_height, m_depth);
        boxMaker.Build();
        LOG_INF_S("BRepPrimAPI_MakeBox created for OCCBox: " + m_name);

        if (boxMaker.IsDone()) {
            TopoDS_Shape shape = boxMaker.Shape();
            LOG_INF_S("Box shape created successfully for OCCBox: " + m_name + " - Shape is null: " + (shape.IsNull() ? "yes" : "no"));
            setShape(shape);

            // Log the center of the created shape
            Bnd_Box box;
            BRepBndLib::Add(shape, box);
            if (!box.IsVoid()) {
                Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
                LOG_INF_S("[OCCGeometryDebug] OCCBox shape center: (" + std::to_string(center.X()) + ", " + std::to_string(center.Y()) + ", " + std::to_string(center.Z()) + ")");
            }
        }
        else {
            LOG_ERR_S("BRepPrimAPI_MakeBox failed for OCCBox: " + m_name);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create box: " + std::string(e.what()) + " for OCCBox: " + m_name);
    }
}

// OCCCylinder implementation
OCCCylinder::OCCCylinder(const std::string& name, double radius, double height)
    : OCCGeometry(name)
    , m_radius(radius)
    , m_height(height)
{
    buildShape();
}

void OCCCylinder::setDimensions(double radius, double height)
{
    m_radius = radius;
    m_height = height;
    buildShape();
}

void OCCCylinder::getSize(double& radius, double& height) const
{
    radius = m_radius;
    height = m_height;
}

void OCCCylinder::buildShape()
{
    LOG_INF_S("Building OCCCylinder shape with radius: " + std::to_string(m_radius) + " height: " + std::to_string(m_height));

    try {
        // Use the simplest constructor that takes radius and height
        BRepPrimAPI_MakeCylinder cylinderMaker(m_radius, m_height);
        LOG_INF_S("BRepPrimAPI_MakeCylinder created for OCCCylinder: " + m_name);
        cylinderMaker.Build();
        if (cylinderMaker.IsDone()) {
            TopoDS_Shape shape = cylinderMaker.Shape();
            LOG_INF_S("Cylinder shape created successfully for OCCCylinder: " + m_name + " - Shape is null: " + (shape.IsNull() ? "yes" : "no"));
            setShape(shape);

            // Log the center of the created shape
            Bnd_Box box;
            BRepBndLib::Add(shape, box);
            if (!box.IsVoid()) {
                Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
                LOG_INF_S("[OCCGeometryDebug] OCCCylinder shape center: (" + std::to_string(center.X()) + ", " + std::to_string(center.Y()) + ", " + std::to_string(center.Z()) + ")");
            }
        }
        else {
            LOG_ERR_S("BRepPrimAPI_MakeCylinder failed for OCCCylinder: " + m_name);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create cylinder: " + std::string(e.what()) + " for OCCCylinder: " + m_name);
    }
}

// OCCSphere implementation
OCCSphere::OCCSphere(const std::string& name, double radius)
    : OCCGeometry(name)
    , m_radius(radius)
{
    buildShape();
}

void OCCSphere::setRadius(double radius)
{
    m_radius = radius;
    buildShape();
}

void OCCSphere::buildShape()
{
    LOG_INF_S("Building OCCSphere shape with radius: " + std::to_string(m_radius));

    try {
        // Validate radius parameter
        if (m_radius <= 0.0) {
            LOG_ERR_S("Invalid radius for OCCSphere: " + m_name + " - radius: " + std::to_string(m_radius));
            return;
        }

        // Try simple constructor first, as in pythonocc examples
        BRepPrimAPI_MakeSphere sphereMaker(m_radius);
        sphereMaker.Build();
        LOG_INF_S("Simple BRepPrimAPI_MakeSphere created for OCCSphere: " + m_name);

        if (sphereMaker.IsDone()) {
            TopoDS_Shape shape = sphereMaker.Shape();
            if (!shape.IsNull()) {
                LOG_INF_S("Sphere shape created successfully with simple constructor for: " + m_name);
                setShape(shape);

                // Log the center of the created shape
                Bnd_Box box;
                BRepBndLib::Add(shape, box);
                if (!box.IsVoid()) {
                    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
                    LOG_INF_S("[OCCGeometryDebug] OCCSphere shape center (simple): (" + std::to_string(center.X()) + ", " + std::to_string(center.Y()) + ", " + std::to_string(center.Z()) + ")");
                }
                return;
            }
            else {
                LOG_ERR_S("Simple constructor returned null shape for: " + m_name);
            }
        }
        else {
            LOG_ERR_S("Simple BRepPrimAPI_MakeSphere failed (IsDone = false) for: " + m_name);
        }

        // Fallback to axis-based constructor
        LOG_INF_S("Falling back to axis-based constructor for OCCSphere: " + m_name);
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeSphere fallbackMaker(axis, m_radius);
        fallbackMaker.Build();
        if (fallbackMaker.IsDone()) {
            TopoDS_Shape fallbackShape = fallbackMaker.Shape();
            if (!fallbackShape.IsNull()) {
                LOG_INF_S("Fallback sphere creation successful for: " + m_name);
                setShape(fallbackShape);

                // Log the center of the created shape
                Bnd_Box box;
                BRepBndLib::Add(fallbackShape, box);
                if (!box.IsVoid()) {
                    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
                    LOG_INF_S("[OCCGeometryDebug] OCCSphere shape center (fallback): (" + std::to_string(center.X()) + ", " + std::to_string(center.Y()) + ", " + std::to_string(center.Z()) + ")");
                }
            }
            else {
                LOG_ERR_S("Fallback also returned null shape for: " + m_name);
            }
        }
        else {
            LOG_ERR_S("Fallback BRepPrimAPI_MakeSphere failed for: " + m_name);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create sphere: " + std::string(e.what()) + " for OCCSphere: " + m_name);
    }
}

// OCCCone implementation
OCCCone::OCCCone(const std::string& name, double bottomRadius, double topRadius, double height)
    : OCCGeometry(name)
    , m_bottomRadius(bottomRadius)
    , m_topRadius(topRadius)
    , m_height(height)
{
    buildShape();
}

void OCCCone::setDimensions(double bottomRadius, double topRadius, double height)
{
    m_bottomRadius = bottomRadius;
    m_topRadius = topRadius;
    m_height = height;
    buildShape();
}

void OCCCone::getSize(double& bottomRadius, double& topRadius, double& height) const
{
    bottomRadius = m_bottomRadius;
    topRadius = m_topRadius;
    height = m_height;
}

void OCCCone::buildShape()
{
    LOG_INF_S("Building OCCCone shape with bottomRadius: " + std::to_string(m_bottomRadius) + " topRadius: " + std::to_string(m_topRadius) + " height: " + std::to_string(m_height));

    try {
        // Following OpenCASCADE examples: use gp_Ax2 for proper cone orientation
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));

        // Enhanced parameter validation and fallback mechanism
        double actualTopRadius = m_topRadius;

        // Log which constructor approach we're using
        if (actualTopRadius <= 0.001) {
            LOG_INF_S("Creating perfect cone (topRadius ~= 0) for: " + m_name);
        }
        else {
            LOG_INF_S("Creating truncated cone for: " + m_name);
        }

        // Always use axis-based constructor with proper parameter validation
        BRepPrimAPI_MakeCone coneMaker(axis, m_bottomRadius, actualTopRadius, m_height);
        LOG_INF_S("BRepPrimAPI_MakeCone created for OCCCone: " + m_name +
            " with params - bottomRadius: " + std::to_string(m_bottomRadius) +
            ", topRadius: " + std::to_string(actualTopRadius) +
            ", height: " + std::to_string(m_height));

        coneMaker.Build();

        if (coneMaker.IsDone()) {
            TopoDS_Shape shape = coneMaker.Shape();
            if (!shape.IsNull()) {
                LOG_INF_S("Cone shape created successfully for OCCCone: " + m_name);
                setShape(shape);

                // Log the center of the created shape
                Bnd_Box box;
                BRepBndLib::Add(shape, box);
                if (!box.IsVoid()) {
                    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
                    LOG_INF_S("[OCCGeometryDebug] OCCCone shape center: (" + std::to_string(center.X()) + ", " + std::to_string(center.Y()) + ", " + std::to_string(center.Z()) + ")");
                }
            }
            else {
                LOG_ERR_S("BRepPrimAPI_MakeCone returned null shape for OCCCone: " + m_name);

                // Fallback: try with small non-zero topRadius if it was exactly 0
                if (m_topRadius == 0.0 && actualTopRadius == 0.0) {
                    LOG_INF_S("Attempting fallback with small topRadius for perfect cone: " + m_name);
                    actualTopRadius = 0.001;
                    BRepPrimAPI_MakeCone fallbackMaker(axis, m_bottomRadius, actualTopRadius, m_height);
                    fallbackMaker.Build();
                    if (fallbackMaker.IsDone()) {
                        TopoDS_Shape fallbackShape = fallbackMaker.Shape();
                        if (!fallbackShape.IsNull()) {
                            LOG_INF_S("Fallback cone creation successful for: " + m_name);
                            setShape(fallbackShape);

                            // Log the center of the created shape
                            Bnd_Box box;
                            BRepBndLib::Add(fallbackShape, box);
                            if (!box.IsVoid()) {
                                Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                                box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                                gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
                                LOG_INF_S("[OCCGeometryDebug] OCCCone fallback shape center: (" + std::to_string(center.X()) + ", " + std::to_string(center.Y()) + ", " + std::to_string(center.Z()) + ")");
                            }
                        }
                        else {
                            LOG_ERR_S("Fallback cone creation also failed for: " + m_name);
                        }
                    }
                    else {
                        LOG_ERR_S("Fallback BRepPrimAPI_MakeCone also failed for: " + m_name);
                    }
                }
            }
        }
        else {
            LOG_ERR_S("BRepPrimAPI_MakeCone failed (IsDone = false) for OCCCone: " + m_name);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create cone: " + std::string(e.what()) + " for OCCCone: " + m_name);
    }
}

// OCCTorus implementation
OCCTorus::OCCTorus(const std::string& name, double majorRadius, double minorRadius)
    : OCCGeometry(name), m_majorRadius(majorRadius), m_minorRadius(minorRadius)
{
    LOG_INF_S("Creating OCCTorus: " + name + " with major radius: " + std::to_string(majorRadius) + " minor radius: " + std::to_string(minorRadius));
    buildShape();
}

void OCCTorus::setDimensions(double majorRadius, double minorRadius)
{
    if (m_majorRadius != majorRadius || m_minorRadius != minorRadius) {
        m_majorRadius = majorRadius;
        m_minorRadius = minorRadius;
        LOG_INF_S("OCCTorus dimensions changed: " + m_name + " major: " + std::to_string(majorRadius) + " minor: " + std::to_string(minorRadius));
        buildShape();
        m_coinNeedsUpdate = true;
    }
}

void OCCTorus::getSize(double& majorRadius, double& minorRadius) const
{
    majorRadius = m_majorRadius;
    minorRadius = m_minorRadius;
}

void OCCTorus::buildShape()
{
    LOG_INF_S("Building OCCTorus shape with major radius: " + std::to_string(m_majorRadius) + " minor radius: " + std::to_string(m_minorRadius));

    try {
        // Validate parameters
        if (m_majorRadius <= 0.0 || m_minorRadius <= 0.0) {
            LOG_ERR_S("Invalid radii for OCCTorus: " + m_name + " - major: " + std::to_string(m_majorRadius) + " minor: " + std::to_string(m_minorRadius));
            return;
        }

        if (m_minorRadius >= m_majorRadius) {
            LOG_ERR_S("Invalid torus dimensions: minor radius must be less than major radius for " + m_name);
            return;
        }

        // Create torus using axis-based constructor for better control
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeTorus torusMaker(axis, m_majorRadius, m_minorRadius);
        torusMaker.Build();

        if (torusMaker.IsDone()) {
            TopoDS_Shape shape = torusMaker.Shape();
            if (!shape.IsNull()) {
                m_shape = shape;
                LOG_INF_S("OCCTorus shape created successfully: " + m_name);
            }
            else {
                LOG_ERR_S("OCCTorus shape is null after creation: " + m_name);
            }
        }
        else {
            LOG_ERR_S("BRepPrimAPI_MakeTorus failed for: " + m_name);
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception in OCCTorus::buildShape for " + m_name + ": " + e.what());
    }
}

// OCCTruncatedCylinder implementation
OCCTruncatedCylinder::OCCTruncatedCylinder(const std::string& name, double bottomRadius, double topRadius, double height)
    : OCCGeometry(name), m_bottomRadius(bottomRadius), m_topRadius(topRadius), m_height(height)
{
    LOG_INF_S("Creating OCCTruncatedCylinder: " + name + " with bottom radius: " + std::to_string(bottomRadius) +
        " top radius: " + std::to_string(topRadius) + " height: " + std::to_string(height));
    buildShape();
}

void OCCTruncatedCylinder::setDimensions(double bottomRadius, double topRadius, double height)
{
    if (m_bottomRadius != bottomRadius || m_topRadius != topRadius || m_height != height) {
        m_bottomRadius = bottomRadius;
        m_topRadius = topRadius;
        m_height = height;
        LOG_INF_S("OCCTruncatedCylinder dimensions changed: " + m_name +
            " bottom: " + std::to_string(bottomRadius) +
            " top: " + std::to_string(topRadius) +
            " height: " + std::to_string(height));
        buildShape();
        m_coinNeedsUpdate = true;
    }
}

void OCCTruncatedCylinder::getSize(double& bottomRadius, double& topRadius, double& height) const
{
    bottomRadius = m_bottomRadius;
    topRadius = m_topRadius;
    height = m_height;
}

void OCCTruncatedCylinder::buildShape()
{
    LOG_INF_S("Building OCCTruncatedCylinder shape with bottom radius: " + std::to_string(m_bottomRadius) +
        " top radius: " + std::to_string(m_topRadius) + " height: " + std::to_string(m_height));

    try {
        // Validate parameters
        if (m_bottomRadius <= 0.0 || m_topRadius <= 0.0 || m_height <= 0.0) {
            LOG_ERR_S("Invalid dimensions for OCCTruncatedCylinder: " + m_name +
                " - bottom: " + std::to_string(m_bottomRadius) +
                " top: " + std::to_string(m_topRadius) +
                " height: " + std::to_string(m_height));
            return;
        }

        // Create truncated cylinder using cone with different radii
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCone truncatedCylinderMaker(axis, m_bottomRadius, m_topRadius, m_height);
        truncatedCylinderMaker.Build();

        if (truncatedCylinderMaker.IsDone()) {
            TopoDS_Shape shape = truncatedCylinderMaker.Shape();
            if (!shape.IsNull()) {
                m_shape = shape;
                LOG_INF_S("OCCTruncatedCylinder shape created successfully: " + m_name);
            }
            else {
                LOG_ERR_S("OCCTruncatedCylinder shape is null after creation: " + m_name);
            }
        }
        else {
            LOG_ERR_S("BRepPrimAPI_MakeCone failed for OCCTruncatedCylinder: " + m_name);
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception in OCCTruncatedCylinder::buildShape for " + m_name + ": " + e.what());
    }
}

// Removed setShadingMode(RenderingConfig::ShadingMode) method - functionality not needed

void OCCGeometry::setSmoothNormals(bool enabled) {
    m_smoothNormals = enabled;
    LOG_INF_S("Smooth normals " + std::string(enabled ? "enabled" : "disabled"));
}

void OCCGeometry::setWireframeWidth(double width) {
    m_wireframeWidth = width;
    LOG_INF_S("Wireframe width set to: " + std::to_string(width));
}

void OCCGeometry::setPointSize(double size) {
    m_pointSize = size;
    LOG_INF_S("Point size set to: " + std::to_string(size));
}

// Display methods implementation
void OCCGeometry::setDisplayMode(RenderingConfig::DisplayMode mode) {
    m_displayMode = mode;
    LOG_INF_S("Display mode set to: " + RenderingConfig::getDisplayModeName(mode));
}

void OCCGeometry::setShowEdges(bool enabled) {
    m_showEdges = enabled;
    m_coinNeedsUpdate = true;
}
void OCCGeometry::setShowWireframe(bool enabled) {
    m_showWireframe = enabled;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setShowVertices(bool enabled) {
    m_showVertices = enabled;
    LOG_INF_S("Show vertices " + std::string(enabled ? "enabled" : "disabled"));
}

void OCCGeometry::setEdgeWidth(double width) {
    m_edgeWidth = width;
    LOG_INF_S("Edge width set to: " + std::to_string(width));
}

void OCCGeometry::setVertexSize(double size) {
    m_vertexSize = size;
    LOG_INF_S("Vertex size set to: " + std::to_string(size));
}

void OCCGeometry::setEdgeColor(const Quantity_Color& color) {
    m_edgeColor = color;
    LOG_INF_S("Edge color set");
}

void OCCGeometry::setVertexColor(const Quantity_Color& color) {
    m_vertexColor = color;
    LOG_INF_S("Vertex color set");
}

// Quality methods implementation
void OCCGeometry::setRenderingQuality(RenderingConfig::RenderingQuality quality) {
    m_renderingQuality = quality;
    LOG_INF_S("Rendering quality set to: " + RenderingConfig::getQualityModeName(quality));
}

void OCCGeometry::setTessellationLevel(int level) {
    m_tessellationLevel = level;
    LOG_INF_S("Tessellation level set to: " + std::to_string(level));
}

void OCCGeometry::setAntiAliasingSamples(int samples) {
    m_antiAliasingSamples = samples;
    LOG_INF_S("Anti-aliasing samples set to: " + std::to_string(samples));
}

void OCCGeometry::setEnableLOD(bool enabled) {
    m_enableLOD = enabled;
    LOG_INF_S("LOD " + std::string(enabled ? "enabled" : "disabled"));
}

void OCCGeometry::setLODDistance(double distance) {
    m_lodDistance = distance;
    LOG_INF_S("LOD distance set to: " + std::to_string(distance));
}

// Shadow methods implementation
void OCCGeometry::setShadowMode(RenderingConfig::ShadowMode mode) {
    m_shadowMode = mode;
    LOG_INF_S("Shadow mode set to: " + RenderingConfig::getShadowModeName(mode));
}

void OCCGeometry::setShadowIntensity(double intensity) {
    m_shadowIntensity = intensity;
    LOG_INF_S("Shadow intensity set to: " + std::to_string(intensity));
}

void OCCGeometry::setShadowSoftness(double softness) {
    m_shadowSoftness = softness;
    LOG_INF_S("Shadow softness set to: " + std::to_string(softness));
}

void OCCGeometry::setShadowMapSize(int size) {
    m_shadowMapSize = size;
    LOG_INF_S("Shadow map size set to: " + std::to_string(size));
}

void OCCGeometry::setShadowBias(double bias) {
    m_shadowBias = bias;
    LOG_INF_S("Shadow bias set to: " + std::to_string(bias));
}

// Lighting model methods implementation
void OCCGeometry::setLightingModel(RenderingConfig::LightingModel model) {
    m_lightingModel = model;
    LOG_INF_S("Lighting model set to: " + RenderingConfig::getLightingModelName(model));
}

void OCCGeometry::setRoughness(double roughness) {
    m_roughness = roughness;
    LOG_INF_S("Roughness set to: " + std::to_string(roughness));
}

void OCCGeometry::setMetallic(double metallic) {
    m_metallic = metallic;
    LOG_INF_S("Metallic set to: " + std::to_string(metallic));
}

void OCCGeometry::setFresnel(double fresnel) {
    m_fresnel = fresnel;
    LOG_INF_S("Fresnel set to: " + std::to_string(fresnel));
}

void OCCGeometry::setSubsurfaceScattering(double scattering) {
    m_subsurfaceScattering = scattering;
    LOG_INF_S("Subsurface scattering set to: " + std::to_string(scattering));
}

void OCCGeometry::updateFromRenderingConfig()
{
    // Load current settings from configuration
    RenderingConfig& config = RenderingConfig::getInstance();
    const auto& materialSettings = config.getMaterialSettings();
    const auto& textureSettings = config.getTextureSettings();
    const auto& blendSettings = config.getBlendSettings();
    
    // Only update material settings if they haven't been explicitly set for this geometry
    // This prevents global config from overriding individual geometry material settings
    if (!m_materialExplicitlySet) {
        m_color = materialSettings.diffuseColor;
        m_transparency = materialSettings.transparency;
        m_materialAmbientColor = materialSettings.ambientColor;
        m_materialDiffuseColor = materialSettings.diffuseColor;
        m_materialSpecularColor = materialSettings.specularColor;
        m_materialShininess = materialSettings.shininess;
    }
    
    // Update texture settings
    m_textureColor = textureSettings.color;
    m_textureIntensity = textureSettings.intensity;
    m_textureEnabled = textureSettings.enabled;
    m_textureImagePath = textureSettings.imagePath;
    m_textureMode = textureSettings.textureMode;
    
    // Update blend settings
    m_blendMode = blendSettings.blendMode;
    m_depthTest = blendSettings.depthTest;
    m_depthWrite = blendSettings.depthWrite;
    m_cullFace = blendSettings.cullFace;
    m_alphaThreshold = blendSettings.alphaThreshold;
    
    // Force rebuild of Coin3D representation
    m_coinNeedsUpdate = true;
    
    // Actually rebuild the Coin3D representation to apply changes immediately
    if (m_coinNode) {
        // Force Coin3D to invalidate its cache
        m_coinNode->touch();
        
        // Use the material-aware version to preserve custom material settings
        MeshParameters meshParams;
        buildCoinRepresentation(meshParams, 
                               m_materialDiffuseColor, m_materialAmbientColor, 
                               m_materialSpecularColor, m_materialEmissiveColor,
                               m_materialShininess, m_transparency);
        LOG_INF_S("Rebuilt Coin3D representation for geometry '" + m_name + "' with custom material");
        
        // Force the scene graph to be marked as needing update
        // Note: Coin3D nodes don't have getParent() method, so we just touch the current node
        m_coinNode->touch();
    }
    
    LOG_INF_S("Updated geometry '" + m_name + "' from RenderingConfig:");
    LOG_INF_S("  - Material diffuse color: " + std::to_string(m_materialDiffuseColor.Red()) + "," + 
              std::to_string(m_materialDiffuseColor.Green()) + "," + std::to_string(m_materialDiffuseColor.Blue()));
    LOG_INF_S("  - Material ambient color: " + std::to_string(m_materialAmbientColor.Red()) + "," + 
              std::to_string(m_materialAmbientColor.Green()) + "," + std::to_string(m_materialAmbientColor.Blue()));
    LOG_INF_S("  - Transparency: " + std::to_string(m_transparency));
    LOG_INF_S("  - Texture enabled: " + std::string(m_textureEnabled ? "true" : "false"));
    LOG_INF_S("  - Blend mode: " + RenderingConfig::getBlendModeName(m_blendMode));
}

void OCCGeometry::updateMaterialForLighting()
{
    // This method is called when lighting changes to adjust material properties
    // for better lighting response without changing the base material settings
    
    if (!m_coinNode) {
        LOG_WRN_S("Cannot update material for lighting: Coin3D node not available for " + m_name);
        return;
    }
    
    // Find the material node in the Coin3D representation
    for (int i = 0; i < m_coinNode->getNumChildren(); ++i) {
        SoNode* child = m_coinNode->getChild(i);
        if (child && child->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(child);
            
            // Adjust material properties for better lighting response
            if (!m_wireframeMode) {
                // Enhance ambient component for better lighting visibility
                Standard_Real r, g, b;
                m_materialAmbientColor.Values(r, g, b, Quantity_TOC_RGB);
                material->ambientColor.setValue(static_cast<float>(r * 1.2), 
                                              static_cast<float>(g * 1.2), 
                                              static_cast<float>(b * 1.2));
                
                // Ensure diffuse component is properly set
                m_materialDiffuseColor.Values(r, g, b, Quantity_TOC_RGB);
                material->diffuseColor.setValue(static_cast<float>(r), 
                                              static_cast<float>(g), 
                                              static_cast<float>(b));
                
                // Enhance specular component for better lighting highlights
                m_materialSpecularColor.Values(r, g, b, Quantity_TOC_RGB);
                material->specularColor.setValue(static_cast<float>(r * 1.1), 
                                               static_cast<float>(g * 1.1), 
                                               static_cast<float>(b * 1.1));
                
                // Adjust shininess for better lighting response
                material->shininess.setValue(static_cast<float>(m_materialShininess / 100.0));
                
                LOG_INF_S("Updated material for lighting response: " + m_name);
            }
            break;
        }
    }
    
    // Force Coin3D to update
    m_coinNode->touch();
}

void OCCGeometry::forceTextureUpdate()
{
    if (m_textureEnabled && !m_textureImagePath.empty()) {
        m_coinNeedsUpdate = true;
        
        if (m_coinNode) {
            // Force rebuild of Coin3D representation to apply texture changes
            buildCoinRepresentation();
            m_coinNode->touch();
            LOG_INF_S("Forced texture update for " + m_name + " - path: " + m_textureImagePath);
        }
    }
}

void OCCGeometry::setFaceDisplay(bool enable) {
    // Removed setShadingMode call - functionality not needed
    if (m_coinNode) {
        buildCoinRepresentation();
        m_coinNode->touch();
    }
}

void OCCGeometry::setFacesVisible(bool visible) {
    if (m_facesVisible == visible) return;
    m_facesVisible = visible;
    m_coinNeedsUpdate = true;
    if (m_coinNode) {
        buildCoinRepresentation();
        m_coinNode->touch();
    }
}
void OCCGeometry::setWireframeOverlay(bool enable) {
    setWireframeMode(enable);
    if (m_coinNode) {
        buildCoinRepresentation();
        m_coinNode->touch();
    }
}
bool OCCGeometry::hasOriginalEdges() const {
    return m_showEdges;
}
void OCCGeometry::setEdgeDisplay(bool enable) {
    setShowEdges(enable);
    if (m_coinNode) {
        buildCoinRepresentation();
        m_coinNode->touch();
    }
}
void OCCGeometry::setFeatureEdgeDisplay(bool enable) {
    if (edgeComponent) edgeComponent->setEdgeDisplayType(EdgeType::Feature, enable);
    if (m_coinNode) {
        buildCoinRepresentation();
        m_coinNode->touch();
    }
}
void OCCGeometry::setNormalDisplay(bool enable) {
    setSmoothNormals(enable);
    if (m_coinNode) {
        buildCoinRepresentation();
        m_coinNode->touch();
    }
}

void OCCGeometry::setEdgeDisplayType(EdgeType type, bool show) {
    if (edgeComponent) edgeComponent->setEdgeDisplayType(type, show);
}

bool OCCGeometry::isEdgeDisplayTypeEnabled(EdgeType type) const {
    return edgeComponent ? edgeComponent->isEdgeDisplayTypeEnabled(type) : false;
}

void OCCGeometry::buildCoinRepresentation(const MeshParameters& params, 
                                        const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
                                        const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
                                        double shininess, double transparency)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();

    if (m_coinNode) {
        m_coinNode->removeAllChildren();
    }
    else {
        m_coinNode = new SoSeparator;
        m_coinNode->ref();
    }

    // Clean up any existing texture nodes to prevent memory issues
    if (m_coinNode) {
        for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
            SoNode* child = m_coinNode->getChild(i);
            if (child && (child->isOfType(SoTexture2::getClassTypeId()) || 
                         child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
                m_coinNode->removeChild(i);
            }
        }
    }

    // Transform setup
    m_coinTransform = new SoTransform;
    m_coinTransform->translation.setValue(
        static_cast<float>(m_position.X()),
        static_cast<float>(m_position.Y()),
        static_cast<float>(m_position.Z())
    );

    if (m_rotationAngle != 0.0) {
        SbVec3f axis(
            static_cast<float>(m_rotationAxis.X()),
            static_cast<float>(m_rotationAxis.Y()),
            static_cast<float>(m_rotationAxis.Z())
        );
        m_coinTransform->rotation.setValue(axis, static_cast<float>(m_rotationAngle));
    }

    m_coinTransform->scaleFactor.setValue(
        static_cast<float>(m_scale),
        static_cast<float>(m_scale),
        static_cast<float>(m_scale)
    );
    m_coinNode->addChild(m_coinTransform);

    // Shape hints
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    hints->faceType = SoShapeHints::CONVEX;
    m_coinNode->addChild(hints);

    // Draw style
    SoDrawStyle* drawStyle = new SoDrawStyle;
    drawStyle->style = m_wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
    drawStyle->lineWidth = m_wireframeMode ? 1.0f : 0.0f;
    m_coinNode->addChild(drawStyle);

    // Convert shape to mesh using OCCMeshConverter (geometry conversion only)
    TriangleMesh mesh = OCCMeshConverter::convertToMesh(m_shape, params.deflection);
    
    if (mesh.isEmpty()) {
        LOG_ERR_S("Failed to convert shape to mesh for " + m_name);
        return;
    }

    // Apply smoothing if enabled
    if (m_smoothNormals) {
        mesh = OCCMeshConverter::smoothNormals(mesh, 30.0, 2);
    }

    // Apply subdivision if enabled
    if (m_subdivisionEnabled) {
        mesh = OCCMeshConverter::createSubdivisionSurface(mesh, m_subdivisionLevels);
    }

    // Use RenderingToolkitAPI for all rendering operations (proper architecture)
    auto& manager = RenderingToolkitAPI::getManager();
    auto backend = manager.getRenderBackend("Coin3D");
    if (backend) {
        auto sceneNode = backend->createSceneNode(mesh, m_selected,
                                                 diffuseColor, ambientColor, specularColor, emissiveColor, shininess, transparency);
        if (sceneNode) {
            SoSeparator* meshNode = sceneNode.get();
            meshNode->ref(); // Take ownership
            m_coinNode->addChild(meshNode);
        }
    } else {
        LOG_ERR_S("Coin3D backend not available for " + m_name);
    }

    auto buildEndTime = std::chrono::high_resolution_clock::now();
    auto buildDuration = std::chrono::duration_cast<std::chrono::microseconds>(buildEndTime - buildStartTime);
    
    LOG_INF_S("Coin3D representation built for " + m_name + " with custom material in " + std::to_string(buildDuration.count()) + " microseconds");
    LOG_INF_S("Mesh statistics: " + std::to_string(mesh.getVertexCount()) + " vertices, " + std::to_string(mesh.getTriangleCount()) + " triangles");
}

void OCCGeometry::updateEdgeDisplay() {
    if (edgeComponent) edgeComponent->updateEdgeDisplay(getCoinNode());
}

void OCCGeometry::applyAdvancedParameters(const AdvancedGeometryParameters& params)
{
    LOG_INF_S("Applying advanced parameters to geometry: " + m_name);
    
    // Apply material parameters
    setMaterialDiffuseColor(params.materialDiffuseColor);
    setMaterialAmbientColor(params.materialAmbientColor);
    setMaterialSpecularColor(params.materialSpecularColor);
    setMaterialEmissiveColor(params.materialEmissiveColor);
    setMaterialShininess(params.materialShininess);
    setTransparency(params.materialTransparency);
    
    // Apply texture parameters
    setTextureEnabled(params.textureEnabled);
    setTextureImagePath(params.texturePath);
    setTextureMode(params.textureMode);
    
    // Apply display parameters
    setShowEdges(params.showEdges);
    setShowWireframe(params.showWireframe);
    setSmoothNormals(params.showNormals);
    
    // Apply edge display types (silhouette disabled)
    if (edgeComponent) {
        edgeComponent->setEdgeDisplayType(EdgeType::Original, params.showOriginalEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, params.showFeatureEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, params.showMeshEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
    }
    
    // Apply subdivision settings
    m_subdivisionEnabled = params.subdivisionEnabled;
    m_subdivisionLevels = params.subdivisionLevels;
    
    // Apply blend settings
    setBlendMode(params.blendMode);
    setDepthTest(params.depthTest);
    setCullFace(params.backfaceCulling);
    
    // Force rebuild of Coin3D representation
    m_coinNeedsUpdate = true;
    
    if (m_coinNode) {
        buildCoinRepresentation();
        m_coinNode->touch();
        LOG_INF_S("Rebuilt Coin3D representation for geometry '" + m_name + "' with advanced parameters");
    }
    
    LOG_INF_S("Advanced parameters applied to geometry '" + m_name + "':");
    LOG_INF_S("  - Material diffuse color: " + std::to_string(params.materialDiffuseColor.Red()) + "," + 
              std::to_string(params.materialDiffuseColor.Green()) + "," + std::to_string(params.materialDiffuseColor.Blue()));
    LOG_INF_S("  - Transparency: " + std::to_string(params.materialTransparency));
    LOG_INF_S("  - Texture enabled: " + std::string(params.textureEnabled ? "true" : "false"));
    LOG_INF_S("  - Show edges: " + std::string(params.showEdges ? "true" : "false"));
    LOG_INF_S("  - Subdivision enabled: " + std::string(params.subdivisionEnabled ? "true" : "false"));
}