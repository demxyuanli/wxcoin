#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
#include "config/RenderingConfig.h"

DisplayModeConfig DisplayModeConfigFactory::getConfig(RenderingConfig::DisplayMode mode,
                                                       const GeometryRenderContext& context) {
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        return createNoShadingConfig(context);
    case RenderingConfig::DisplayMode::Points:
        return createPointsConfig(context);
    case RenderingConfig::DisplayMode::Wireframe:
        return createWireframeConfig(context);
    case RenderingConfig::DisplayMode::Solid:
        return createSolidConfig(context);
    case RenderingConfig::DisplayMode::FlatLines:
        return createFlatLinesConfig(context);
    case RenderingConfig::DisplayMode::Transparent:
        return createTransparentConfig(context);
    case RenderingConfig::DisplayMode::HiddenLine:
        return createHiddenLineConfig(context);
    default:
        return createSolidConfig(context);
    }
}

DisplayModeConfig DisplayModeConfigFactory::createNoShadingConfig(const GeometryRenderContext& context) {
    DisplayModeConfig config;
    
    // Node requirements
    config.nodes.requireSurface = true;
    config.nodes.requireOriginalEdges = true;  // For BREP, will be converted to mesh edges for pure mesh
    config.nodes.requirePoints = false;
    
    // Rendering properties
    config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
    config.rendering.textureEnabled = false;
    config.rendering.blendMode = RenderingConfig::BlendMode::None;
    
    // Material override: NoShading uses BASE_COLOR, so only diffuse matters
    config.rendering.materialOverride.enabled = true;
    config.rendering.materialOverride.ambientColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.diffuseColor = context.material.diffuseColor;  // Preserve original color
    config.rendering.materialOverride.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.shininess = 0.0;
    config.rendering.materialOverride.transparency = 0.0;
    
    // Edge configuration
    config.edges.originalEdge.enabled = true;
    config.edges.originalEdge.color = context.display.wireframeColor;
    config.edges.originalEdge.width = context.display.wireframeWidth;
    
    return config;
}

DisplayModeConfig DisplayModeConfigFactory::createPointsConfig(const GeometryRenderContext& context) {
    DisplayModeConfig config;
    
    // Node requirements
    config.nodes.requirePoints = true;
    config.nodes.surfaceWithPoints = context.display.showSolidWithPointView;
    config.nodes.requireSurface = context.display.showSolidWithPointView;
    
    // Rendering properties
    config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
    config.rendering.textureEnabled = false;
    
    return config;
}

DisplayModeConfig DisplayModeConfigFactory::createWireframeConfig(const GeometryRenderContext& context) {
    DisplayModeConfig config;
    
    // Node requirements
    config.nodes.requireSurface = false;  // Wireframe mode hides surface
    config.nodes.requireOriginalEdges = true;  // For BREP, will be converted to mesh edges for pure mesh
    
    // Rendering properties
    // Note: Wireframe mode is achieved by requireSurface=false and requireOriginalEdges=true
    // No need for wireframeMode flag - it's redundant
    config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
    config.rendering.textureEnabled = false;
    config.rendering.materialOverride.enabled = true;
    config.rendering.materialOverride.ambientColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.shininess = 0.0;
    
    // Edge configuration
    config.edges.originalEdge.enabled = true;
    config.edges.originalEdge.color = context.display.wireframeColor;
    config.edges.originalEdge.width = context.display.wireframeWidth;
    
    return config;
}

DisplayModeConfig DisplayModeConfigFactory::createSolidConfig(const GeometryRenderContext& context) {
    DisplayModeConfig config;
    
    // Node requirements
    config.nodes.requireSurface = true;
    config.nodes.requireOriginalEdges = false;  // Default off, user can enable if needed
    // Edges are controlled separately, not part of display mode
    
    // Rendering properties
    config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::PHONG;
    config.rendering.textureEnabled = false;
    config.rendering.blendMode = RenderingConfig::BlendMode::None;
    // No material override - use original material from context
    
    // Edge configuration - default disabled, user can enable
    config.edges.originalEdge.enabled = false;
    config.edges.originalEdge.color = context.display.wireframeColor;
    config.edges.originalEdge.width = context.display.wireframeWidth;
    
    return config;
}

DisplayModeConfig DisplayModeConfigFactory::createFlatLinesConfig(const GeometryRenderContext& context) {
    DisplayModeConfig config;
    
    // Node requirements
    config.nodes.requireSurface = true;
    config.nodes.requireOriginalEdges = true;
    
    // Rendering properties
    config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::PHONG;
    config.rendering.textureEnabled = false;
    config.rendering.materialOverride.enabled = true;
    config.rendering.materialOverride.shininess = 30.0;  // Fixed shininess for flat shading
    // Other material properties preserved from context
    
    // Edge configuration
    config.edges.originalEdge.enabled = true;
    config.edges.originalEdge.color = context.display.wireframeColor;
    config.edges.originalEdge.width = context.display.wireframeWidth;
    
    return config;
}

DisplayModeConfig DisplayModeConfigFactory::createTransparentConfig(const GeometryRenderContext& context) {
    DisplayModeConfig config;
    
    // Node requirements: Transparent mode requires surface rendering
    config.nodes.requireSurface = true;
    config.nodes.requireOriginalEdges = false;
    config.nodes.requireMeshEdges = false;
    config.nodes.requirePoints = false;
    
    // Rendering properties: Use PHONG lighting with Alpha blend mode for transparency
    config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::PHONG;
    config.rendering.textureEnabled = false;
    config.rendering.blendMode = RenderingConfig::BlendMode::Alpha;
    
    // Material override: Enable material override with transparency
    config.rendering.materialOverride.enabled = true;
    config.rendering.materialOverride.ambientColor = context.material.ambientColor;
    config.rendering.materialOverride.diffuseColor = context.material.diffuseColor;
    config.rendering.materialOverride.specularColor = context.material.specularColor;
    config.rendering.materialOverride.emissiveColor = context.material.emissiveColor;
    config.rendering.materialOverride.shininess = context.material.shininess;
    // Default transparency is 0.5 (50% transparent) if not set in context
    config.rendering.materialOverride.transparency = (context.material.transparency > 0.0) 
        ? context.material.transparency : 0.5;
    
    // Edge configuration: No edges in transparent mode
    config.edges.originalEdge.enabled = false;
    config.edges.meshEdge.enabled = false;
    
    // Post-processing: No polygon offset needed for transparent mode
    config.postProcessing.polygonOffset.enabled = false;
    
    return config;
}

DisplayModeConfig DisplayModeConfigFactory::createHiddenLineConfig(const GeometryRenderContext& context) {
    DisplayModeConfig config;
    
    // Node requirements
    config.nodes.requireSurface = true;
    config.nodes.requireMeshEdges = true;  // HiddenLine uses mesh edges, not original edges
    
    // Rendering properties
    config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
    config.rendering.textureEnabled = false;
    config.rendering.blendMode = RenderingConfig::BlendMode::None;
    
    // Material override: White surface
    config.rendering.materialOverride.enabled = true;
    config.rendering.materialOverride.ambientColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.diffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    config.rendering.materialOverride.shininess = 0.0;
    config.rendering.materialOverride.transparency = 0.0;
    
    // Post-processing: Polygon offset for depth sorting
    config.postProcessing.polygonOffset.enabled = true;
    config.postProcessing.polygonOffset.factor = 1.0f;
    config.postProcessing.polygonOffset.units = 1.0f;
    
    // Edge configuration: Mesh edges with effective color (black if too light)
    config.edges.meshEdge.enabled = true;
    config.edges.meshEdge.color = context.material.diffuseColor;  // Original face color
    config.edges.meshEdge.width = context.display.wireframeWidth;
    config.edges.meshEdge.useEffectiveColor = true;  // Use black if color is too light
    
    return config;
}

