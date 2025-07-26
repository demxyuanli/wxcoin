#pragma once

#include <string>
#include <Inventor/SbColor.h>
#include "config/RenderingConfig.h"
#include "config/EdgeSettingsConfig.h"

// 只包含与坐标和尺寸相关的参数
struct BasicGeometryParameters {
    std::string geometryType;
    // Box
    double width = 2.0;
    double height = 2.0;
    double depth = 2.0;
    // Sphere
    double radius = 1.0;
    // Cylinder
    double cylinderRadius = 1.0;
    double cylinderHeight = 2.0;
    // Cone
    double bottomRadius = 1.0;
    double topRadius = 0.0;
    double coneHeight = 2.0;
    // Torus
    double majorRadius = 2.0;
    double minorRadius = 0.5;
    // Truncated Cylinder
    double truncatedBottomRadius = 1.0;
    double truncatedTopRadius = 0.5;
    double truncatedHeight = 2.0;
};

// 包含材质、贴图、渲染、显示等高级参数
struct AdvancedGeometryParameters {
    // Material
    SbColor materialDiffuseColor = SbColor(0.8f, 0.8f, 0.8f);
    SbColor materialAmbientColor = SbColor(0.2f, 0.2f, 0.2f);
    SbColor materialSpecularColor = SbColor(1.0f, 1.0f, 1.0f);
    float materialShininess = 50.0f;
    float materialTransparency = 0.0f;
    SbColor materialEmissiveColor = SbColor(0.0f, 0.0f, 0.0f);
    
    // Texture
    std::string texturePath;
    RenderingConfig::TextureMode textureMode = RenderingConfig::TextureMode::Modulate;
    bool textureEnabled = false;
    
    // Rendering
    RenderingConfig::RenderingQuality renderingQuality = RenderingConfig::RenderingQuality::Normal;
    RenderingConfig::BlendMode blendMode = RenderingConfig::BlendMode::None;
    RenderingConfig::LightingModel lightingModel = RenderingConfig::LightingModel::BlinnPhong;
    bool backfaceCulling = true;
    bool depthTest = true;
    
    // Display
    bool showNormals = false;
    bool showEdges = false;
    bool showWireframe = false;
    bool showSilhouette = false;
    bool showFeatureEdges = false;
    bool showMeshEdges = false;
    bool showOriginalEdges = false;
    bool showFaceNormals = false;
    
    // Subdivision
    bool subdivisionEnabled = false;
    int subdivisionLevels = 1;
    
    // Edge Settings
    int edgeStyle = 0; // 0=Solid, 1=Dashed, 2=Dotted
    float edgeWidth = 1.0f;
    SbColor edgeColor = SbColor(0.0f, 0.0f, 0.0f);
    bool edgeEnabled = true;
}; 