#pragma once

#include "rendering/GeometryProcessor.h"
#include "config/RenderingConfig.h"
#include <string>
#include <memory>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>

// Forward declarations
class SoSeparator;
class SoTransform;
class SoMaterial;
class OCCMeshConverter;

/**
 * @brief Base class for OpenCASCADE geometry objects
 */
class OCCGeometry {
public:
    OCCGeometry(const std::string& name);
    virtual ~OCCGeometry();

    // Property accessors
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    const TopoDS_Shape& getShape() const { return m_shape; }
    virtual void setShape(const TopoDS_Shape& shape);

    gp_Pnt getPosition() const { return m_position; }
    virtual void setPosition(const gp_Pnt& position);

    void getRotation(gp_Vec& axis, double& angle) const { axis = m_rotationAxis; angle = m_rotationAngle; }
    virtual void setRotation(const gp_Vec& axis, double angle);

    double getScale() const { return m_scale; }
    virtual void setScale(double scale);

    bool isVisible() const { return m_visible; }
    virtual void setVisible(bool visible);

    bool isSelected() const { return m_selected; }
    virtual void setSelected(bool selected);

    Quantity_Color getColor() const { return m_color; }
    virtual void setColor(const Quantity_Color& color);

    double getTransparency() const { return m_transparency; }
    virtual void setTransparency(double transparency);

    // Material properties
    Quantity_Color getMaterialAmbientColor() const { return m_materialAmbientColor; }
    virtual void setMaterialAmbientColor(const Quantity_Color& color);
    
    Quantity_Color getMaterialDiffuseColor() const { return m_materialDiffuseColor; }
    virtual void setMaterialDiffuseColor(const Quantity_Color& color);
    
    Quantity_Color getMaterialSpecularColor() const { return m_materialSpecularColor; }
    virtual void setMaterialSpecularColor(const Quantity_Color& color);
    
    double getMaterialShininess() const { return m_materialShininess; }
    virtual void setMaterialShininess(double shininess);

    // Set default bright material for better visibility without textures
    virtual void setDefaultBrightMaterial();

    // Texture properties
    Quantity_Color getTextureColor() const { return m_textureColor; }
    virtual void setTextureColor(const Quantity_Color& color);
    
    double getTextureIntensity() const { return m_textureIntensity; }
    virtual void setTextureIntensity(double intensity);
    
    bool isTextureEnabled() const { return m_textureEnabled; }
    virtual void setTextureEnabled(bool enabled);
    
    std::string getTextureImagePath() const { return m_textureImagePath; }
    virtual void setTextureImagePath(const std::string& path);
    
    RenderingConfig::TextureMode getTextureMode() const { return m_textureMode; }
    virtual void setTextureMode(RenderingConfig::TextureMode mode);
    
    // Blend settings
    RenderingConfig::BlendMode getBlendMode() const { return m_blendMode; }
    virtual void setBlendMode(RenderingConfig::BlendMode mode);
    
    bool isDepthTestEnabled() const { return m_depthTest; }
    virtual void setDepthTest(bool enabled);
    
    bool isDepthWriteEnabled() const { return m_depthWrite; }
    virtual void setDepthWrite(bool enabled);
    
    bool isCullFaceEnabled() const { return m_cullFace; }
    virtual void setCullFace(bool enabled);
    
    double getAlphaThreshold() const { return m_alphaThreshold; }
    virtual void setAlphaThreshold(double threshold);

    // Shading settings
    RenderingConfig::ShadingMode getShadingMode() const { return m_shadingModeType; }
    virtual void setShadingMode(RenderingConfig::ShadingMode mode);
    
    bool isSmoothNormalsEnabled() const { return m_smoothNormals; }
    virtual void setSmoothNormals(bool enabled);
    
    double getWireframeWidth() const { return m_wireframeWidth; }
    virtual void setWireframeWidth(double width);
    
    double getPointSize() const { return m_pointSize; }
    virtual void setPointSize(double size);
    
    // Display settings
    RenderingConfig::DisplayMode getDisplayMode() const { return m_displayMode; }
    virtual void setDisplayMode(RenderingConfig::DisplayMode mode);
    
    bool isShowEdgesEnabled() const { return m_showEdges; }
    virtual void setShowEdges(bool enabled);
    
    bool isShowVerticesEnabled() const { return m_showVertices; }
    virtual void setShowVertices(bool enabled);
    
    double getEdgeWidth() const { return m_edgeWidth; }
    virtual void setEdgeWidth(double width);
    
    double getVertexSize() const { return m_vertexSize; }
    virtual void setVertexSize(double size);
    
    Quantity_Color getEdgeColor() const { return m_edgeColor; }
    virtual void setEdgeColor(const Quantity_Color& color);
    
    Quantity_Color getVertexColor() const { return m_vertexColor; }
    virtual void setVertexColor(const Quantity_Color& color);
    
    // Quality settings
    RenderingConfig::RenderingQuality getRenderingQuality() const { return m_renderingQuality; }
    virtual void setRenderingQuality(RenderingConfig::RenderingQuality quality);
    
    int getTessellationLevel() const { return m_tessellationLevel; }
    virtual void setTessellationLevel(int level);
    
    int getAntiAliasingSamples() const { return m_antiAliasingSamples; }
    virtual void setAntiAliasingSamples(int samples);
    
    bool isLODEnabled() const { return m_enableLOD; }
    virtual void setEnableLOD(bool enabled);
    
    double getLODDistance() const { return m_lodDistance; }
    virtual void setLODDistance(double distance);
    
    // Shadow settings
    RenderingConfig::ShadowMode getShadowMode() const { return m_shadowMode; }
    virtual void setShadowMode(RenderingConfig::ShadowMode mode);
    
    double getShadowIntensity() const { return m_shadowIntensity; }
    virtual void setShadowIntensity(double intensity);
    
    double getShadowSoftness() const { return m_shadowSoftness; }
    virtual void setShadowSoftness(double softness);
    
    int getShadowMapSize() const { return m_shadowMapSize; }
    virtual void setShadowMapSize(int size);
    
    double getShadowBias() const { return m_shadowBias; }
    virtual void setShadowBias(double bias);
    
    // Lighting model settings
    RenderingConfig::LightingModel getLightingModel() const { return m_lightingModel; }
    virtual void setLightingModel(RenderingConfig::LightingModel model);
    
    double getRoughness() const { return m_roughness; }
    virtual void setRoughness(double roughness);
    
    double getMetallic() const { return m_metallic; }
    virtual void setMetallic(double metallic);
    
    double getFresnel() const { return m_fresnel; }
    virtual void setFresnel(double fresnel);
    
    double getSubsurfaceScattering() const { return m_subsurfaceScattering; }
    virtual void setSubsurfaceScattering(double scattering);

    // Update settings from RenderingConfig
    virtual void updateFromRenderingConfig();

    // Force texture update
    virtual void forceTextureUpdate();

    // Display modes
    bool isWireframeMode() const { return m_wireframeMode; }
    virtual void setWireframeMode(bool wireframe);
    bool isShadingMode() const { return m_shadingMode; }
    virtual void setShadingMode(bool shaded);

    void setFaceDisplay(bool enable);
    void setWireframeOverlay(bool enable);
    bool hasOriginalEdges() const;
    void setEdgeDisplay(bool enable);
    void setFeatureEdgeDisplay(bool enable);
    void setNormalDisplay(bool enable);
    void setShowWireframe(bool enabled);
    bool isShowWireframe() const { return m_showWireframe; }

    // Coin3D integration
    SoSeparator* getCoinNode();
    void setCoinNode(SoSeparator* node);
    void regenerateMesh(const MeshParameters& params);
    
    // Performance optimization
    bool needsMeshRegeneration() const;
    void setMeshRegenerationNeeded(bool needed);
    void updateCoinRepresentationIfNeeded(const MeshParameters& params);

private:
    void buildCoinRepresentation(const MeshParameters& params = MeshParameters());

protected:
    std::string m_name;
    TopoDS_Shape m_shape;
    
    // Transform parameters
    gp_Pnt m_position;
    gp_Vec m_rotationAxis;
    double m_rotationAngle;
    double m_scale;
    
    // Display properties
    bool m_visible;
    bool m_selected;
    Quantity_Color m_color;
    double m_transparency;
    
    // Material properties
    Quantity_Color m_materialAmbientColor;
    Quantity_Color m_materialDiffuseColor;
    Quantity_Color m_materialSpecularColor;
    double m_materialShininess;
    
    // Texture properties
    Quantity_Color m_textureColor;
    double m_textureIntensity;
    bool m_textureEnabled;
    std::string m_textureImagePath;
    RenderingConfig::TextureMode m_textureMode;
    
    // Blend properties
    RenderingConfig::BlendMode m_blendMode;
    bool m_depthTest;
    bool m_depthWrite;
    bool m_cullFace;
    double m_alphaThreshold;
    
    // Shading settings
    RenderingConfig::ShadingMode m_shadingModeType;
    bool m_smoothNormals;
    double m_wireframeWidth;
    double m_pointSize;
    
    // Display settings
    RenderingConfig::DisplayMode m_displayMode;
    bool m_showEdges;
    bool m_showVertices;
    double m_edgeWidth;
    double m_vertexSize;
    Quantity_Color m_edgeColor;
    Quantity_Color m_vertexColor;
    
    // Quality settings
    RenderingConfig::RenderingQuality m_renderingQuality;
    int m_tessellationLevel;
    int m_antiAliasingSamples;
    bool m_enableLOD;
    double m_lodDistance;
    
    // Shadow settings
    RenderingConfig::ShadowMode m_shadowMode;
    double m_shadowIntensity;
    double m_shadowSoftness;
    int m_shadowMapSize;
    double m_shadowBias;
    
    // Lighting model settings
    RenderingConfig::LightingModel m_lightingModel;
    double m_roughness;
    double m_metallic;
    double m_fresnel;
    double m_subsurfaceScattering;
    
    // Display modes
    bool m_wireframeMode;
    bool m_shadingMode;
    bool m_showWireframe = false; // controls mesh wireframe
    
    // Coin3D representation
    SoSeparator* m_coinNode;
    SoTransform* m_coinTransform;
    bool m_coinNeedsUpdate;
    
    // Performance optimization
    bool m_meshRegenerationNeeded;
    MeshParameters m_lastMeshParams;
};

/**
 * @brief OpenCASCADE box geometry
 */
class OCCBox : public OCCGeometry {
public:
    OCCBox(const std::string& name, double width, double height, double depth);
    
    void setDimensions(double width, double height, double depth);
    void getSize(double& width, double& height, double& depth) const;

private:
    void buildShape();
    
    double m_width, m_height, m_depth;
};

/**
 * @brief OpenCASCADE cylinder geometry
 */
class OCCCylinder : public OCCGeometry {
public:
    OCCCylinder(const std::string& name, double radius, double height);
    
    void setDimensions(double radius, double height);
    void getSize(double& radius, double& height) const;

private:
    void buildShape();
    
    double m_radius, m_height;
};

/**
 * @brief OpenCASCADE sphere geometry
 */
class OCCSphere : public OCCGeometry {
public:
    OCCSphere(const std::string& name, double radius);
    
    void setRadius(double radius);
    double getRadius() const { return m_radius; }

private:
    void buildShape();
    
    double m_radius;
};

/**
 * @brief OpenCASCADE cone geometry
 */
class OCCCone : public OCCGeometry {
public:
    OCCCone(const std::string& name, double bottomRadius, double topRadius, double height);
    
    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

private:
    void buildShape();
    
    double m_bottomRadius, m_topRadius, m_height;
}; 

/**
 * @brief OpenCASCADE torus geometry
 */
class OCCTorus : public OCCGeometry {
public:
    OCCTorus(const std::string& name, double majorRadius, double minorRadius);
    
    void setDimensions(double majorRadius, double minorRadius);
    void getSize(double& majorRadius, double& minorRadius) const;

private:
    void buildShape();
    
    double m_majorRadius, m_minorRadius;
}; 

/**
 * @brief OpenCASCADE truncated cylinder geometry (frustum)
 */
class OCCTruncatedCylinder : public OCCGeometry {
public:
    OCCTruncatedCylinder(const std::string& name, double bottomRadius, double topRadius, double height);
    
    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

private:
    void buildShape();
    
    double m_bottomRadius, m_topRadius, m_height;
}; 