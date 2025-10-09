#pragma once

#include "geometry/OCCGeometryCore.h"
#include <memory>

// Forward declarations
struct MeshParameters;
class Quantity_Color;
class SoSeparator;

/**
 * @brief OCCGeometryRendering - 几何体渲染功能扩展类
 *
 * 提供完整的Coin3D渲染功能，包括材质、纹理、网格参数等
 */
class OCCGeometryRendering : public OCCGeometryCore
{
public:
    OCCGeometryRendering(const std::string& name);
    virtual ~OCCGeometryRendering() = default;

    // Coin3D构建和渲染
    virtual void buildCoinRepresentation(const MeshParameters& params) override;
    virtual void createWireframeRepresentation(const MeshParameters& params);

    // 渲染参数管理
    virtual void updateFromRenderingConfig() override;
    virtual void updateMaterialForLighting();

    // 渲染优化
    virtual void forceCoinRepresentationRebuild(const MeshParameters& params) override;

    // 高级渲染参数
    virtual void applyAdvancedParameters(const AdvancedGeometryParameters& params);

    // LOD支持
    virtual void addLODLevel(double distance, double deflection);
    virtual int getLODLevel(double viewDistance) const;
    std::vector<std::pair<double, double>> getLODLevels() const { return m_lodLevels; }

protected:
    // Coin3D构建辅助方法
    virtual void buildCoinRepresentation(const MeshParameters& params,
        const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
        const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
        double shininess, double transparency);

    // 渲染配置参数
    RenderingConfig::BlendMode m_blendMode;
    bool m_depthTest;
    bool m_depthWrite;
    bool m_cullFace;
    double m_alphaThreshold;

    // 显示配置
    bool m_smoothNormals;
    double m_wireframeWidth;
    double m_pointSize;
    bool m_subdivisionEnabled;
    int m_subdivisionLevels;

    // LOD配置
    bool m_enableLOD;
    std::vector<std::pair<double, double>> m_lodLevels;

    // 渲染质量配置
    RenderingConfig::RenderingQuality m_renderingQuality;
    int m_tessellationLevel;
    int m_antiAliasingSamples;

    // 阴影配置
    RenderingConfig::ShadowMode m_shadowMode;
    double m_shadowIntensity;
    double m_shadowSoftness;
    int m_shadowMapSize;
    double m_shadowBias;

    // 光照模型配置
    RenderingConfig::LightingModel m_lightingModel;
    double m_roughness;
    double m_metallic;
    double m_fresnel;
    double m_subsurfaceScattering;
};
