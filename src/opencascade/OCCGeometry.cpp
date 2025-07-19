#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "logger/Logger.h"
#include "config/RenderingConfig.h"
#include <limits>
#include <cmath>
#include <wx/gdicmn.h>

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

// OCCGeometry base class implementation
OCCGeometry::OCCGeometry(const std::string& name)
    : m_name(name)
    , m_position(0, 0, 0)
    , m_rotationAxis(0, 0, 1)
    , m_rotationAngle(0.0)
    , m_scale(1.0)
    , m_visible(true)
    , m_selected(false)
    , m_wireframeMode(false)
    , m_shadingMode(true)
    , m_coinNode(nullptr)
    , m_coinTransform(nullptr)
    , m_coinNeedsUpdate(true)
{
    // Load settings from configuration
    RenderingConfig& config = RenderingConfig::getInstance();
    const auto& materialSettings = config.getMaterialSettings();
    const auto& textureSettings = config.getTextureSettings();
    
    m_color = materialSettings.diffuseColor;
    m_transparency = materialSettings.transparency;
    m_materialAmbientColor = materialSettings.ambientColor;
    m_materialDiffuseColor = materialSettings.diffuseColor;
    m_materialSpecularColor = materialSettings.specularColor;
    m_materialShininess = materialSettings.shininess;
    m_textureColor = textureSettings.color;
    m_textureIntensity = textureSettings.intensity;
    m_textureEnabled = textureSettings.enabled;
    m_textureImagePath = textureSettings.imagePath;
    m_textureMode = textureSettings.textureMode;
    
    const auto& blendSettings = config.getBlendSettings();
    m_blendMode = blendSettings.blendMode;
    m_depthTest = blendSettings.depthTest;
    m_depthWrite = blendSettings.depthWrite;
    m_cullFace = blendSettings.cullFace;
    m_alphaThreshold = blendSettings.alphaThreshold;
    
    LOG_INF_S("Creating OCC geometry: " + name + " with configured material and blend settings");
    LOG_INF_S("Initial transparency: " + std::to_string(m_transparency) + ", blend mode: " + RenderingConfig::getBlendModeName(m_blendMode));
}

OCCGeometry::~OCCGeometry()
{
    if (m_coinNode) {
        m_coinNode->unref();
    }
    LOG_INF_S("Destroyed OCC geometry: " + m_name);
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
        
        // Update Coin3D node visibility
        if (m_coinNode) {
            // Set the node's render action based on visibility
            if (visible) {
                m_coinNode->renderCulling = SoSeparator::OFF;
            } else {
                m_coinNode->renderCulling = SoSeparator::ON;
            }
            
            // Force a refresh
            m_coinNode->touch();
            
            LOG_INF_S("Set geometry visibility: " + m_name + " -> " + (visible ? "visible" : "hidden"));
        } else {
            LOG_INF_S("Set geometry visibility (no Coin3D node yet): " + m_name + " -> " + (visible ? "visible" : "hidden"));
        }
    }
}

void OCCGeometry::setSelected(bool selected)
{
    if (m_selected != selected) {
        LOG_INF_S("Setting selection for geometry '" + m_name + "': " + (selected ? "true" : "false"));
        m_selected = selected;

        // Force rebuild of Coin3D representation to update edge colors
        m_coinNeedsUpdate = true;

        if (m_coinNode) {
            LOG_INF_S("Rebuilding Coin3D representation for selection change: " + m_name + ", selected: " + (selected ? "true" : "false"));

            // Rebuild the entire Coin3D representation to update edge colors
            buildCoinRepresentation();

            // Force a refresh of the scene to show the selection change
            m_coinNode->touch();
        }
        else {
            LOG_INF_S("Coin3D node not yet created for geometry: " + m_name);
        }
    }
}

void OCCGeometry::setColor(const Quantity_Color& color)
{
    m_color = color;
    if (m_coinNode) {
        // Find the material node and update it. It might not be at a fixed index.
        for (int i = 0; i < m_coinNode->getNumChildren(); ++i) {
            SoNode* child = m_coinNode->getChild(i);
            if (child && child->isOfType(SoMaterial::getClassTypeId())) {
                SoMaterial* material = static_cast<SoMaterial*>(child);
                material->diffuseColor.setValue(
                    static_cast<float>(m_color.Red()),
                    static_cast<float>(m_color.Green()),
                    static_cast<float>(m_color.Blue())
                );
                break;
            }
        }
    }
}

void OCCGeometry::setTransparency(double transparency)
{
    m_transparency = std::min(1.0, std::max(0.0, transparency));  // Clamp 0-1
    if (m_wireframeMode) {
        m_transparency = std::min(m_transparency, 0.6);  // Limit for visible lines
        LOG_WRN_S("Wireframe: Limited transparency to " + std::to_string(m_transparency));
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
        
        LOG_INF_S("Transparency set to " + std::to_string(m_transparency) + ", cleared old nodes, rebuilt in mode " + (m_wireframeMode ? "wireframe" : "filling"));
    }
}

void OCCGeometry::setWireframeMode(bool wireframe)
{
    m_wireframeMode = (m_wireframeMode == wireframe) ? !wireframe : wireframe;
    m_coinNeedsUpdate = true;
    LOG_INF_S("Toggled Wireframe to " + std::string(m_wireframeMode ? "enabled" : "disabled"));
}

void OCCGeometry::setShadingMode(bool shaded)
{
    m_shadingMode = (m_shadingMode == shaded) ? !shaded : shaded;
    m_coinNeedsUpdate = true;
    LOG_INF_S("Toggled Shading to " + std::string(m_shadingMode ? "enabled" : "disabled"));
}

// Material property setters
void OCCGeometry::setMaterialAmbientColor(const Quantity_Color& color)
{
    m_materialAmbientColor = color;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialDiffuseColor(const Quantity_Color& color)
{
    m_materialDiffuseColor = color;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialSpecularColor(const Quantity_Color& color)
{
    m_materialSpecularColor = color;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialShininess(double shininess)
{
    m_materialShininess = std::max(0.0, std::min(100.0, shininess));
    m_coinNeedsUpdate = true;
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
}

void OCCGeometry::setTextureImagePath(const std::string& path)
{
    m_textureImagePath = path;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setTextureMode(RenderingConfig::TextureMode mode)
{
    m_textureMode = mode;
    m_coinNeedsUpdate = true;
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
    LOG_INF_S("Getting Coin3D node for geometry: " + m_name + " - Node exists: " + (m_coinNode ? "yes" : "no") + " - Needs update: " + (m_coinNeedsUpdate ? "yes" : "no"));

    if (!m_coinNode || m_coinNeedsUpdate) {
        LOG_INF_S("Building Coin3D representation for geometry: " + m_name);
        buildCoinRepresentation();
    }

    LOG_INF_S("Returning Coin3D node for geometry: " + m_name + " - Node: " + (m_coinNode ? "valid" : "null"));
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
    LOG_INF_S("Set Coin3D node for geometry: " + m_name);
}

void OCCGeometry::regenerateMesh(const OCCMeshConverter::MeshParameters& params)
{
    buildCoinRepresentation(params);
}

void OCCGeometry::buildCoinRepresentation(const OCCMeshConverter::MeshParameters& params)
{
    LOG_INF_S("Building Coin3D representation for geometry: " + m_name);

    if (m_coinNode) {
        m_coinNode->removeAllChildren();
    }
    else {
        m_coinNode = new SoSeparator;
        m_coinNode->ref();
    }

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

    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    hints->faceType = SoShapeHints::CONVEX;
    m_coinNode->addChild(hints);

    // Add draw style for wireframe/shading mode
    SoDrawStyle* drawStyle = new SoDrawStyle;
    drawStyle->style = m_wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
    drawStyle->lineWidth = m_wireframeMode ? 1.0f : 0.0f;  // 隐藏时线宽为0
    m_coinNode->addChild(drawStyle);

    SoMaterial* material = new SoMaterial;
    if (m_wireframeMode) {
        SoMaterial* lineMaterial = new SoMaterial;  // Separate for lines
        lineMaterial->diffuseColor.setValue(0.0f, 0.0f, 1.0f);
        lineMaterial->transparency.setValue(static_cast<float>(m_transparency));
        m_coinNode->addChild(lineMaterial);
    } else {
        // Use material properties from rendering settings
        material->ambientColor.setValue(
            static_cast<float>(m_materialAmbientColor.Red()),
            static_cast<float>(m_materialAmbientColor.Green()),
            static_cast<float>(m_materialAmbientColor.Blue())
        );
        material->diffuseColor.setValue(
            static_cast<float>(m_materialDiffuseColor.Red()),
            static_cast<float>(m_materialDiffuseColor.Green()),
            static_cast<float>(m_materialDiffuseColor.Blue())
        );
        material->specularColor.setValue(
            static_cast<float>(m_materialSpecularColor.Red()),
            static_cast<float>(m_materialSpecularColor.Green()),
            static_cast<float>(m_materialSpecularColor.Blue())
        );
        material->shininess.setValue(static_cast<float>(m_materialShininess / 100.0));
        material->transparency.setValue(static_cast<float>(m_transparency));
        m_coinNode->addChild(material);
    }
    LOG_INF_S("Applied material with rendering settings properties - transparency: " + std::to_string(m_transparency));

    // Apply blend settings - using material transparency instead of SoBlendFunc
    if (m_blendMode != RenderingConfig::BlendMode::None) {
        // Set material transparency based on blend mode
        SoMaterial* material = new SoMaterial;
        
        switch (m_blendMode) {
            case RenderingConfig::BlendMode::Alpha:
                material->transparency = 0.5f;
                break;
            case RenderingConfig::BlendMode::Additive:
                material->transparency = 0.3f;
                break;
            case RenderingConfig::BlendMode::Multiply:
                material->transparency = 0.4f;
                break;
            case RenderingConfig::BlendMode::Screen:
                material->transparency = 0.6f;
                break;
            case RenderingConfig::BlendMode::Overlay:
                material->transparency = 0.5f;
                break;
            default:
                material->transparency = 0.5f;
                break;
        }
        
        m_coinNode->addChild(material);
        LOG_INF_S("Applied blend mode: " + RenderingConfig::getBlendModeName(m_blendMode));
    } else {
        // When blend mode is None, ensure transparency is 0.0
        if (m_transparency > 0.0) {
            m_transparency = 0.0;
            LOG_INF_S("Reset transparency to 0.0 for None blend mode");
        }
    }
    
    // Apply depth and face culling settings
    if (!m_depthTest || !m_depthWrite || !m_cullFace) {
        SoShapeHints* customHints = new SoShapeHints;
        customHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        customHints->shapeType = SoShapeHints::SOLID;
        customHints->faceType = m_cullFace ? SoShapeHints::CONVEX : SoShapeHints::UNKNOWN_FACE_TYPE;
        m_coinNode->addChild(customHints);
        
        LOG_INF_S("Applied custom blend settings - DepthTest: " + std::string(m_depthTest ? "enabled" : "disabled") + 
                  ", DepthWrite: " + std::string(m_depthWrite ? "enabled" : "disabled") + 
                  ", CullFace: " + std::string(m_cullFace ? "enabled" : "disabled"));
    }

    // Apply texture settings if enabled
    if (m_textureEnabled) {
        SoTexture2* texture = new SoTexture2;
        texture->wrapS = SoTexture2::REPEAT;
        texture->wrapT = SoTexture2::REPEAT;
        
        // Set texture model based on configuration
        // Note: Coin3D SoTexture2 only supports DECAL and MODULATE modes
        switch (m_textureMode) {
            case RenderingConfig::TextureMode::Replace:
                texture->model = SoTexture2::REPLACE;
                break;
            case RenderingConfig::TextureMode::Modulate:
        texture->model = SoTexture2::MODULATE;
                break;
            case RenderingConfig::TextureMode::Decal:
                texture->model = SoTexture2::DECAL;
                break;
            case RenderingConfig::TextureMode::Blend:
                texture->model = SoTexture2::BLEND;
                break;
            default:
                texture->model = SoTexture2::MODULATE;
                break;
        }
        
        // Try to load image texture first
        if (!m_textureImagePath.empty()) {
            SoInput input;
            if (input.openFile(m_textureImagePath.c_str())) {
                SoNode* node = SoDB::readAll(&input);
                if (node && node->isOfType(SoTexture2::getClassTypeId())) {
                    // If it's a texture node, copy its image data
                    SoTexture2* loadedTexture = static_cast<SoTexture2*>(node);
                    texture->filename.setValue(m_textureImagePath.c_str());
                    LOG_INF_S("Loaded texture image from file: " + m_textureImagePath);
                } else {
                    // Try to load as image file directly
                    texture->filename.setValue(m_textureImagePath.c_str());
                    LOG_INF_S("Set texture filename: " + m_textureImagePath);
                }
                input.closeFile();
            } else {
                LOG_WRN_S("Failed to load texture image: " + m_textureImagePath + ", using color texture");
                // Fall back to color texture
                unsigned char alpha = static_cast<unsigned char>(255 * (1.0 - m_transparency) * m_textureIntensity);
                unsigned char r = static_cast<unsigned char>(m_textureColor.Red() * 255);
                unsigned char g = static_cast<unsigned char>(m_textureColor.Green() * 255);
                unsigned char b = static_cast<unsigned char>(m_textureColor.Blue() * 255);
                unsigned char texData[16] = {r, g, b, alpha, r, g, b, alpha, r, g, b, alpha, r, g, b, alpha};
                texture->image.setValue(SbVec2s(2, 2), 4, texData);
                LOG_INF_S("Applied fallback color texture with color (" + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + ") and alpha " + std::to_string(alpha));
            }
        } else {
            // Create a more complex texture pattern based on texture mode
            unsigned char r = static_cast<unsigned char>(m_textureColor.Red() * 255);
            unsigned char g = static_cast<unsigned char>(m_textureColor.Green() * 255);
            unsigned char b = static_cast<unsigned char>(m_textureColor.Blue() * 255);
            unsigned char alpha = static_cast<unsigned char>(255 * (1.0 - m_transparency) * m_textureIntensity);
            
            // Create a 4x4 texture with different patterns for each mode
            unsigned char texData[64]; // 4x4 pixels, 4 bytes per pixel (RGBA)
            
            switch (m_textureMode) {
                case RenderingConfig::TextureMode::Decal:
                    // Decal: Strong texture color with some transparency
                    for (int i = 0; i < 16; i++) {
                        texData[i*4] = r;     // Red
                        texData[i*4+1] = g;   // Green
                        texData[i*4+2] = b;   // Blue
                        texData[i*4+3] = alpha; // Alpha
                    }
                    break;
                    
                case RenderingConfig::TextureMode::Modulate:
                    // Modulate: Alternating strong and weak texture
                    for (int i = 0; i < 16; i++) {
                        bool strong = (i % 2 == 0); // Alternate pattern
                        unsigned char intensity = strong ? alpha : (alpha / 3);
                        texData[i*4] = r;     // Red
                        texData[i*4+1] = g;   // Green
                        texData[i*4+2] = b;   // Blue
                        texData[i*4+3] = intensity; // Alpha
                    }
                    break;
                    
                case RenderingConfig::TextureMode::Replace:
                    // Replace: Full intensity texture
                    for (int i = 0; i < 16; i++) {
                        texData[i*4] = r;     // Red
                        texData[i*4+1] = g;   // Green
                        texData[i*4+2] = b;   // Blue
                        texData[i*4+3] = 255; // Full alpha
                    }
                    break;
                    
                case RenderingConfig::TextureMode::Blend:
                    // Blend: Gradient pattern
                    for (int i = 0; i < 16; i++) {
                        int row = i / 4;
                        int col = i % 4;
                        unsigned char blendAlpha = static_cast<unsigned char>(alpha * (row + col) / 6.0);
                        texData[i*4] = r;     // Red
                        texData[i*4+1] = g;   // Green
                        texData[i*4+2] = b;   // Blue
                        texData[i*4+3] = blendAlpha; // Gradient alpha
                    }
                    break;
                    
                default:
                    // Default: Simple pattern
                    for (int i = 0; i < 16; i++) {
                        texData[i*4] = r;     // Red
                        texData[i*4+1] = g;   // Green
                        texData[i*4+2] = b;   // Blue
                        texData[i*4+3] = alpha; // Alpha
                    }
                    break;
            }
            
            texture->image.setValue(SbVec2s(4, 4), 4, texData);
            LOG_INF_S("Applied enhanced texture pattern for mode " + RenderingConfig::getTextureModeName(m_textureMode) + 
                     " with color (" + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + ") and alpha " + std::to_string(alpha));
        }
        
        m_coinNode->addChild(texture);
    }

    LOG_INF_S("Shape is null: " + std::string(m_shape.IsNull() ? "yes" : "no") + " for geometry: " + m_name);

    if (!m_shape.IsNull()) {
        LOG_INF_S("Creating mesh node for geometry: " + m_name);
        SoSeparator* meshNode = OCCMeshConverter::createCoinNode(m_shape, params, m_selected);
        if (meshNode) {
            m_coinNode->addChild(meshNode);
            LOG_INF_S("Successfully added mesh node to Coin3D representation for: " + m_name);
        }
        else {
            LOG_ERR_S("Failed to create mesh node for geometry: " + m_name);
        }
    }
    else {
        LOG_ERR_S("Shape is null, cannot create mesh node for geometry: " + m_name);
    }

    // Set visibility based on current state
    if (m_coinNode) {
        if (m_visible) {
            m_coinNode->renderCulling = SoSeparator::OFF;
        } else {
            m_coinNode->renderCulling = SoSeparator::ON;
        }
        LOG_INF_S("Set Coin3D node visibility for geometry: " + m_name + " -> " + (m_visible ? "visible" : "hidden"));
    }
    
    m_coinNeedsUpdate = false;
    LOG_INF_S("Finished building Coin3D representation for geometry: " + m_name);
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

// Shading methods implementation
void OCCGeometry::setShadingMode(RenderingConfig::ShadingMode mode) {
    m_shadingModeType = mode;
    LOG_INF_S("Shading mode set to: " + RenderingConfig::getShadingModeName(mode));
}

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
    LOG_INF_S("Show edges " + std::string(enabled ? "enabled" : "disabled"));
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
    
    // Update material settings
    m_color = materialSettings.diffuseColor;
    m_transparency = materialSettings.transparency;
    m_materialAmbientColor = materialSettings.ambientColor;
    m_materialDiffuseColor = materialSettings.diffuseColor;
    m_materialSpecularColor = materialSettings.specularColor;
    m_materialShininess = materialSettings.shininess;
    
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
        
        buildCoinRepresentation();
        LOG_INF_S("Rebuilt Coin3D representation for geometry '" + m_name + "'");
        
        // Force the scene graph to be marked as needing update
        // Note: Coin3D nodes don't have getParent() method, so we just touch the current node
        m_coinNode->touch();
    }
    
    LOG_INF_S("Updated geometry '" + m_name + "' from RenderingConfig - transparency: " + std::to_string(m_transparency) + 
              ", blend mode: " + RenderingConfig::getBlendModeName(m_blendMode) + 
              ", texture enabled: " + std::string(m_textureEnabled ? "true" : "false") + 
              ", texture mode: " + RenderingConfig::getTextureModeName(m_textureMode));
}