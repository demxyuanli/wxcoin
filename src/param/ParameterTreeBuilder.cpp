#include "param/ParameterTree.h"
#include "config/RenderingConfig.h"
#include <OpenCASCADE/Quantity_Color.hxx>

void ParameterTreeBuilder::buildGeometryParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 几何变换参数
    tree.registerParameter("geometry/transform/position/x", 0.0);
    tree.registerParameter("geometry/transform/position/y", 0.0);
    tree.registerParameter("geometry/transform/position/z", 0.0);
    tree.registerParameter("geometry/transform/rotation/axis/x", 0.0);
    tree.registerParameter("geometry/transform/rotation/axis/y", 0.0);
    tree.registerParameter("geometry/transform/rotation/axis/z", 1.0);
    tree.registerParameter("geometry/transform/rotation/angle", 0.0);
    tree.registerParameter("geometry/transform/scale", 1.0);
    
    // 几何显示参数
    tree.registerParameter("geometry/display/visible", true);
    tree.registerParameter("geometry/display/selected", false);
    tree.registerParameter("geometry/display/wireframe_mode", false);
    tree.registerParameter("geometry/display/show_wireframe", false);
    tree.registerParameter("geometry/display/faces_visible", true);
    tree.registerParameter("geometry/display/wireframe_overlay", false);
    
    // 几何颜色参数
    tree.registerParameter("geometry/color/main", Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB));
    tree.registerParameter("geometry/color/edge", Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB));
    tree.registerParameter("geometry/color/vertex", Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB));
    tree.registerParameter("geometry/transparency", 0.0);
    
    // 几何尺寸参数
    tree.registerParameter("geometry/size/edge_width", 1.0);
    tree.registerParameter("geometry/size/vertex_size", 2.0);
    tree.registerParameter("geometry/size/point_size", 2.0);
    tree.registerParameter("geometry/size/wireframe_width", 1.0);
    
    // 几何质量参数
    tree.registerParameter("geometry/quality/tessellation_level", 2);
    tree.registerParameter("geometry/quality/deflection", 0.5);
    tree.registerParameter("geometry/quality/angular_deflection", 1.0);
    tree.registerParameter("geometry/quality/relative", false);
    tree.registerParameter("geometry/quality/in_parallel", true);
}

void ParameterTreeBuilder::buildRenderingParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 渲染模式参数
    tree.registerParameter("rendering/mode/display_mode", RenderingConfig::DisplayMode::Solid);
    tree.registerParameter("rendering/mode/shading_mode", RenderingConfig::ShadingMode::Smooth);
    tree.registerParameter("rendering/mode/rendering_quality", RenderingConfig::RenderingQuality::Normal);
    
    // 渲染特性参数
    tree.registerParameter("rendering/features/show_edges", false);
    tree.registerParameter("rendering/features/show_vertices", false);
    tree.registerParameter("rendering/features/smooth_normals", true);
    tree.registerParameter("rendering/features/enable_lod", true);
    tree.registerParameter("rendering/features/lod_distance", 100.0);
    
    // 渲染性能参数
    tree.registerParameter("rendering/performance/anti_aliasing_samples", 4);
    tree.registerParameter("rendering/performance/enable_culling", true);
    tree.registerParameter("rendering/performance/enable_depth_test", true);
    tree.registerParameter("rendering/performance/enable_depth_write", true);
}

void ParameterTreeBuilder::buildDisplayParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 显示模式参数
    tree.registerParameter("display/mode/display_mode", RenderingConfig::DisplayMode::Solid);
    tree.registerParameter("display/mode/wireframe_mode", false);
    tree.registerParameter("display/mode/transparent_mode", false);
    
    // 显示元素参数
    tree.registerParameter("display/elements/show_edges", false);
    tree.registerParameter("display/elements/show_vertices", false);
    tree.registerParameter("display/elements/show_faces", true);
    tree.registerParameter("display/elements/show_normals", false);
    
    // 显示样式参数
    tree.registerParameter("display/style/edge_width", 1.0);
    tree.registerParameter("display/style/vertex_size", 2.0);
    tree.registerParameter("display/style/point_size", 2.0);
    tree.registerParameter("display/style/wireframe_width", 1.0);
}

void ParameterTreeBuilder::buildQualityParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 质量等级参数
    tree.registerParameter("quality/level/rendering_quality", RenderingConfig::RenderingQuality::Normal);
    tree.registerParameter("quality/level/tessellation_level", 2);
    
    // 抗锯齿参数
    tree.registerParameter("quality/antialiasing/samples", 4);
    tree.registerParameter("quality/antialiasing/enabled", true);
    
    // LOD参数
    tree.registerParameter("quality/lod/enabled", true);
    tree.registerParameter("quality/lod/distance", 100.0);
    tree.registerParameter("quality/lod/levels", 3);
    
    // 细分参数
    tree.registerParameter("quality/subdivision/enabled", false);
    tree.registerParameter("quality/subdivision/levels", 2);
}

void ParameterTreeBuilder::buildLightingParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 光照模型参数
    tree.registerParameter("lighting/model/lighting_model", RenderingConfig::LightingModel::BlinnPhong);
    tree.registerParameter("lighting/model/roughness", 0.5);
    tree.registerParameter("lighting/model/metallic", 0.0);
    tree.registerParameter("lighting/model/fresnel", 0.04);
    tree.registerParameter("lighting/model/subsurface_scattering", 0.0);
    
    // 环境光参数
    tree.registerParameter("lighting/ambient/color", Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB));
    tree.registerParameter("lighting/ambient/intensity", 0.8);
    
    // 漫射光参数
    tree.registerParameter("lighting/diffuse/color", Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB));
    tree.registerParameter("lighting/diffuse/intensity", 1.0);
    
    // 镜面光参数
    tree.registerParameter("lighting/specular/color", Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB));
    tree.registerParameter("lighting/specular/intensity", 1.0);
}

void ParameterTreeBuilder::buildMaterialParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 材质颜色参数
    tree.registerParameter("material/color/ambient", Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB));
    tree.registerParameter("material/color/diffuse", Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB));
    tree.registerParameter("material/color/specular", Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB));
    tree.registerParameter("material/color/emissive", Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB));
    
    // 材质属性参数
    tree.registerParameter("material/properties/shininess", 30.0);
    tree.registerParameter("material/properties/transparency", 0.0);
    tree.registerParameter("material/properties/metallic", 0.0);
    tree.registerParameter("material/properties/roughness", 0.5);
    
    // 材质预设参数
    tree.registerParameter("material/preset/current", RenderingConfig::MaterialPreset::Custom);
}

void ParameterTreeBuilder::buildTextureParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 纹理启用参数
    tree.registerParameter("texture/enabled", false);
    tree.registerParameter("texture/image_path", std::string(""));
    
    // 纹理颜色参数
    tree.registerParameter("texture/color/main", Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB));
    tree.registerParameter("texture/intensity", 0.5);
    
    // 纹理模式参数
    tree.registerParameter("texture/mode/texture_mode", RenderingConfig::TextureMode::Modulate);
    
    // 纹理坐标参数
    tree.registerParameter("texture/coordinates/repeat_u", 1.0);
    tree.registerParameter("texture/coordinates/repeat_v", 1.0);
    tree.registerParameter("texture/coordinates/offset_u", 0.0);
    tree.registerParameter("texture/coordinates/offset_v", 0.0);
}

void ParameterTreeBuilder::buildShadowParameterTree() {
    auto& tree = ParameterTree::getInstance();
    
    // 阴影模式参数
    tree.registerParameter("shadow/mode/shadow_mode", RenderingConfig::ShadowMode::Soft);
    tree.registerParameter("shadow/mode/enabled", true);
    
    // 阴影强度参数
    tree.registerParameter("shadow/intensity/shadow_intensity", 0.7);
    tree.registerParameter("shadow/intensity/shadow_softness", 0.5);
    
    // 阴影质量参数
    tree.registerParameter("shadow/quality/shadow_map_size", 1024);
    tree.registerParameter("shadow/quality/shadow_bias", 0.001);
    
    // 阴影距离参数
    tree.registerParameter("shadow/distance/near_plane", 0.1);
    tree.registerParameter("shadow/distance/far_plane", 1000.0);
}