#pragma once

#include "OCCGeometry.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class SoSeparator;
class Quantity_Color;
class TopoDS_Shape;
struct MeshParameters;

/**
 * @brief OCCGeometryCore - 几何体核心功能类
 *
 * 包含几何体的基本属性管理、Coin3D构建、网格管理等核心功能
 */
class OCCGeometryCore : public std::enable_shared_from_this<OCCGeometryCore>
{
public:
    // 构造函数和析构函数
    OCCGeometryCore(const std::string& name);
    virtual ~OCCGeometryCore();

    // 基本属性访问器
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    // 几何体形状管理
    virtual void setShape(const TopoDS_Shape& shape);
    const TopoDS_Shape& getShape() const { return m_shape; }

    // 变换属性
    virtual void setPosition(const gp_Pnt& position);
    const gp_Pnt& getPosition() const { return m_position; }

    virtual void setRotation(const gp_Vec& axis, double angle);
    void getRotation(gp_Vec& axis, double& angle) const { axis = m_rotationAxis; angle = m_rotationAngle; }

    virtual void setScale(double scale);
    double getScale() const { return m_scale; }

    // 可见性控制
    virtual void setVisible(bool visible);
    bool isVisible() const { return m_visible; }

    virtual void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

    // 面显示控制
    virtual void setFacesVisible(bool visible);
    bool areFacesVisible() const { return m_facesVisible; }

    // 透明度
    virtual void setTransparency(double transparency);
    double getTransparency() const { return m_transparency; }

    // 线框模式
    virtual void setWireframeMode(bool wireframe);
    bool isWireframeMode() const { return m_wireframeMode; }

    // 材质属性
    virtual void setColor(const Quantity_Color& color);
    const Quantity_Color& getColor() const { return m_color; }

    virtual void setMaterialAmbientColor(const Quantity_Color& color);
    const Quantity_Color& getMaterialAmbientColor() const { return m_materialAmbientColor; }

    virtual void setMaterialDiffuseColor(const Quantity_Color& color);
    const Quantity_Color& getMaterialDiffuseColor() const { return m_materialDiffuseColor; }

    virtual void setMaterialSpecularColor(const Quantity_Color& color);
    const Quantity_Color& getMaterialSpecularColor() const { return m_materialSpecularColor; }

    virtual void setMaterialEmissiveColor(const Quantity_Color& color);
    const Quantity_Color& getMaterialEmissiveColor() const { return m_materialEmissiveColor; }

    virtual void setMaterialShininess(double shininess);
    double getMaterialShininess() const { return m_materialShininess; }

    // 纹理属性
    virtual void setTextureEnabled(bool enabled);
    bool isTextureEnabled() const { return m_textureEnabled; }

    virtual void setTextureImagePath(const std::string& path);
    const std::string& getTextureImagePath() const { return m_textureImagePath; }

    virtual void setTextureMode(RenderingConfig::TextureMode mode);
    RenderingConfig::TextureMode getTextureMode() const { return m_textureMode; }

    // Coin3D节点管理
    virtual SoSeparator* getCoinNode();
    virtual void setCoinNode(SoSeparator* node);
    SoSeparator* getCoinTransform() const { return m_coinTransform; }

    // 网格管理
    virtual void regenerateMesh(const MeshParameters& params);
    bool needsMeshRegeneration() const { return m_meshRegenerationNeeded; }
    void setMeshRegenerationNeeded(bool needed) { m_meshRegenerationNeeded = needed; }

    // 性能优化
    virtual void releaseTemporaryData();
    virtual void optimizeMemory();

    // 面索引映射（前向声明接口）
    virtual bool hasFaceIndexMapping() const = 0;
    virtual void buildFaceIndexMapping(const MeshParameters& params) = 0;
    virtual int getGeometryFaceIdForTriangle(int triangleIndex) const = 0;
    virtual std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const = 0;

protected:
    // Coin3D构建核心方法
    virtual void buildCoinRepresentation(const MeshParameters& params);
    virtual void updateCoinRepresentationIfNeeded(const MeshParameters& params);
    virtual void forceCoinRepresentationRebuild(const MeshParameters& params);

    // 配置更新
    virtual void updateFromRenderingConfig();

    // 成员变量
    std::string m_name;
    TopoDS_Shape m_shape;

    // 变换属性
    gp_Pnt m_position;
    gp_Vec m_rotationAxis;
    double m_rotationAngle;
    double m_scale;

    // 显示属性
    bool m_visible;
    bool m_selected;
    bool m_facesVisible;
    double m_transparency;
    bool m_wireframeMode;

    // 材质属性
    Quantity_Color m_color;
    Quantity_Color m_materialAmbientColor;
    Quantity_Color m_materialDiffuseColor;
    Quantity_Color m_materialSpecularColor;
    Quantity_Color m_materialEmissiveColor;
    double m_materialShininess;
    bool m_materialExplicitlySet;

    // 纹理属性
    Quantity_Color m_textureColor;
    double m_textureIntensity;
    bool m_textureEnabled;
    std::string m_textureImagePath;
    RenderingConfig::TextureMode m_textureMode;

    // Coin3D节点
    SoSeparator* m_coinNode;
    SoSeparator* m_coinTransform;
    bool m_coinNeedsUpdate;

    // 网格管理
    bool m_meshRegenerationNeeded;
    MeshParameters m_lastMeshParams;

    // 渲染配置缓存
    bool m_lastSmoothingEnabled;
    int m_lastSmoothingIterations;
    double m_lastSmoothingCreaseAngle;
    bool m_lastSubdivisionEnabled;
    int m_lastSubdivisionLevel;
    double m_lastSubdivisionCreaseAngle;
    double m_lastSmoothingStrength;
    int m_lastTessellationMethod;
    int m_lastTessellationQuality;
    double m_lastFeaturePreservation;
    bool m_lastAdaptiveMeshing;
    bool m_lastParallelProcessing;
};
