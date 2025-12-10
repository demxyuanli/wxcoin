#pragma once

#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <string>
#include "config/RenderingConfig.h"

/**
 * @brief Transform data for geometry rendering
 */
struct TransformData {
    gp_Pnt position{0, 0, 0};
    gp_Vec rotationAxis{0, 0, 1};
    double rotationAngle{0.0};
    double scale{1.0};
};

/**
 * @brief Material data for geometry rendering
 */
struct MaterialData {
    Quantity_Color ambientColor{0.5, 0.5, 0.5, Quantity_TOC_RGB};
    Quantity_Color diffuseColor{0.95, 0.95, 0.95, Quantity_TOC_RGB};
    Quantity_Color specularColor{1.0, 1.0, 1.0, Quantity_TOC_RGB};
    Quantity_Color emissiveColor{0.0, 0.0, 0.0, Quantity_TOC_RGB};
    double shininess{50.0};
    double transparency{0.0};
};

/**
 * @brief Texture data for geometry rendering
 */
struct TextureData {
    bool enabled{false};
    std::string imagePath;
    Quantity_Color color{1.0, 1.0, 1.0, Quantity_TOC_RGB};
    double intensity{1.0};
    RenderingConfig::TextureMode mode{RenderingConfig::TextureMode::Replace};
};

/**
 * @brief Display settings for geometry rendering
 */
struct DisplaySettings {
    RenderingConfig::DisplayMode displayMode{RenderingConfig::DisplayMode::Solid};
    bool wireframeMode{false};
    bool facesVisible{true};
    bool visible{true};
    bool selected{false};
    double wireframeWidth{1.0};
    Quantity_Color wireframeColor{0.0, 0.0, 0.0, Quantity_TOC_RGB};
    bool cullFace{true};
    TopAbs_ShapeEnum shapeType{TopAbs_SOLID};

        // Point view settings
        bool showPointView{false};
        bool showSolidWithPointView{true};
        double pointViewSize{3.0};
        Quantity_Color pointViewColor{1.0, 0.0, 0.0, Quantity_TOC_RGB};
        int pointViewShape{0}; // 0 = square, 1 = circle, 2 = triangle
};

/**
 * @brief Blend and depth settings
 */
struct BlendSettings {
    RenderingConfig::BlendMode blendMode{RenderingConfig::BlendMode::None};
    bool depthTest{true};
    bool depthWrite{true};
    double alphaThreshold{0.1};
};

/**
 * @brief Complete rendering context for geometry
 * 
 * This structure encapsulates all data needed to render a geometry,
 * allowing GeomCoinRepresentation to be completely independent of other modules.
 */
struct GeometryRenderContext {
    TransformData transform;
    MaterialData material;
    TextureData texture;
    DisplaySettings display;
    BlendSettings blend;
    
    // Helper: create context from a geometry object
    template<typename GeometryType>
    static GeometryRenderContext fromGeometry(const GeometryType& geom) {
        GeometryRenderContext ctx;
        
        // Transform
        ctx.transform.position = geom.getPosition();
        geom.getRotation(ctx.transform.rotationAxis, ctx.transform.rotationAngle);
        ctx.transform.scale = geom.getScale();
        
        // Material
        ctx.material.ambientColor = geom.getMaterialAmbientColor();
        ctx.material.diffuseColor = geom.getMaterialDiffuseColor();
        ctx.material.specularColor = geom.getMaterialSpecularColor();
        ctx.material.emissiveColor = geom.getMaterialEmissiveColor();
        ctx.material.shininess = geom.getMaterialShininess();
        ctx.material.transparency = geom.getTransparency();
        
        // Texture
        ctx.texture.enabled = geom.isTextureEnabled();
        ctx.texture.imagePath = geom.getTextureImagePath();
        ctx.texture.color = geom.getTextureColor();
        ctx.texture.intensity = geom.getTextureIntensity();
        ctx.texture.mode = geom.getTextureMode();
        
        // Display
        ctx.display.displayMode = geom.getDisplayMode();
        ctx.display.wireframeMode = geom.isWireframeMode();
        ctx.display.facesVisible = geom.isFacesVisible();
        ctx.display.visible = geom.isVisible();
        ctx.display.selected = geom.isSelected();
        ctx.display.wireframeWidth = geom.getWireframeWidth();
        ctx.display.wireframeColor = geom.getWireframeColor();
        ctx.display.cullFace = geom.isCullFaceEnabled();
        if (!geom.getShape().IsNull()) {
            ctx.display.shapeType = geom.getShape().ShapeType();
        }

    // Point view settings
    ctx.display.showPointView = geom.isShowPointViewEnabled();
    ctx.display.showSolidWithPointView = geom.isShowSolidWithPointView();
    ctx.display.pointViewSize = geom.getPointViewSize();
    ctx.display.pointViewColor = geom.getPointViewColor();
    ctx.display.pointViewShape = geom.getPointViewShape();
        
        // Blend
        ctx.blend.blendMode = geom.getBlendMode();
        ctx.blend.depthTest = geom.isDepthTestEnabled();
        ctx.blend.depthWrite = geom.isDepthWriteEnabled();
        ctx.blend.alphaThreshold = geom.getAlphaThreshold();
        
        return ctx;
    }
};

