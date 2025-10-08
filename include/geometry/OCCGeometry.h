#pragma once

#include "geometry/OCCGeometryCore.h"
#include "geometry/OCCGeometryTransform.h"
#include "geometry/OCCGeometryMaterial.h"
#include "geometry/OCCGeometryAppearance.h"
#include "geometry/OCCGeometryDisplay.h"
#include "geometry/OCCGeometryQuality.h"
#include "geometry/OCCGeometryMesh.h"
#include "geometry/OCCGeometryPrimitives.h"

/**
 * @brief Main OpenCASCADE geometry class - Composition of all geometry aspects
 * 
 * This class combines all geometry modules into a single interface.
 * Uses composition pattern to organize complex functionality.
 */
class OCCGeometry {
public:
    OCCGeometry(const std::string& name);
    virtual ~OCCGeometry();

    // Core functionality
    const std::string& getName() const { return m_core.getName(); }
    void setName(const std::string& name) { m_core.setName(name); }
    
    const std::string& getFileName() const { return m_core.getFileName(); }
    void setFileName(const std::string& fileName) { m_core.setFileName(fileName); }
    
    const TopoDS_Shape& getShape() const { return m_core.getShape(); }
    virtual void setShape(const TopoDS_Shape& shape) { m_core.setShape(shape); }

    // Transform functionality
    gp_Pnt getPosition() const { return m_transform.getPosition(); }
    virtual void setPosition(const gp_Pnt& position) { m_transform.setPosition(position); }
    
    void getRotation(gp_Vec& axis, double& angle) const { m_transform.getRotation(axis, angle); }
    virtual void setRotation(const gp_Vec& axis, double angle) { m_transform.setRotation(axis, angle); }
    
    double getScale() const { return m_transform.getScale(); }
    virtual void setScale(double scale) { m_transform.setScale(scale); }

    // Material functionality
    Quantity_Color getMaterialAmbientColor() const { return m_material.getMaterialAmbientColor(); }
    virtual void setMaterialAmbientColor(const Quantity_Color& color) { m_material.setMaterialAmbientColor(color); }
    
    Quantity_Color getMaterialDiffuseColor() const { return m_material.getMaterialDiffuseColor(); }
    virtual void setMaterialDiffuseColor(const Quantity_Color& color) { m_material.setMaterialDiffuseColor(color); }
    
    Quantity_Color getMaterialSpecularColor() const { return m_material.getMaterialSpecularColor(); }
    virtual void setMaterialSpecularColor(const Quantity_Color& color) { m_material.setMaterialSpecularColor(color); }
    
    virtual void setMaterialEmissiveColor(const Quantity_Color& color) { m_material.setMaterialEmissiveColor(color); }
    virtual void setMaterialShininess(double shininess) { m_material.setMaterialShininess(shininess); }
    virtual void setDefaultBrightMaterial() { m_material.setDefaultBrightMaterial(); }
    virtual void resetMaterialExplicitFlag() { m_material.resetMaterialExplicitFlag(); }
    virtual bool isMaterialExplicitlySet() const { return m_material.isMaterialExplicitlySet(); }

    // Appearance functionality
    Quantity_Color getColor() const { return m_appearance.getColor(); }
    virtual void setColor(const Quantity_Color& color) { m_appearance.setColor(color); }
    
    double getTransparency() const { return m_appearance.getTransparency(); }
    virtual void setTransparency(double transparency) { m_appearance.setTransparency(transparency); }
    
    bool isVisible() const { return m_appearance.isVisible(); }
    virtual void setVisible(bool visible) { m_appearance.setVisible(visible); }
    
    bool isSelected() const { return m_appearance.isSelected(); }
    virtual void setSelected(bool selected) { m_appearance.setSelected(selected); }

    // Texture properties
    Quantity_Color getTextureColor() const { return m_appearance.getTextureColor(); }
    virtual void setTextureColor(const Quantity_Color& color) { m_appearance.setTextureColor(color); }
    
    double getTextureIntensity() const { return m_appearance.getTextureIntensity(); }
    virtual void setTextureIntensity(double intensity) { m_appearance.setTextureIntensity(intensity); }
    
    bool isTextureEnabled() const { return m_appearance.isTextureEnabled(); }
    virtual void setTextureEnabled(bool enabled) { m_appearance.setTextureEnabled(enabled); }
    
    std::string getTextureImagePath() const { return m_appearance.getTextureImagePath(); }
    virtual void setTextureImagePath(const std::string& path) { m_appearance.setTextureImagePath(path); }
    
    RenderingConfig::TextureMode getTextureMode() const { return m_appearance.getTextureMode(); }
    virtual void setTextureMode(RenderingConfig::TextureMode mode) { m_appearance.setTextureMode(mode); }

    // Blend settings
    RenderingConfig::BlendMode getBlendMode() const { return m_appearance.getBlendMode(); }
    virtual void setBlendMode(RenderingConfig::BlendMode mode) { m_appearance.setBlendMode(mode); }
    
    bool isDepthTestEnabled() const { return m_appearance.isDepthTestEnabled(); }
    virtual void setDepthTest(bool enabled) { m_appearance.setDepthTest(enabled); }
    
    bool isDepthWriteEnabled() const { return m_appearance.isDepthWriteEnabled(); }
    virtual void setDepthWrite(bool enabled) { m_appearance.setDepthWrite(enabled); }
    
    bool isCullFaceEnabled() const { return m_appearance.isCullFaceEnabled(); }
    virtual void setCullFace(bool enabled) { m_appearance.setCullFace(enabled); }
    
    double getAlphaThreshold() const { return m_appearance.getAlphaThreshold(); }
    virtual void setAlphaThreshold(double threshold) { m_appearance.setAlphaThreshold(threshold); }

    // Display functionality
    RenderingConfig::DisplayMode getDisplayMode() const { return m_display.getDisplayMode(); }
    virtual void setDisplayMode(RenderingConfig::DisplayMode mode) { m_display.setDisplayMode(mode); }
    
    bool isShowEdgesEnabled() const { return m_display.isShowEdgesEnabled(); }
    virtual void setShowEdges(bool enabled) { m_display.setShowEdges(enabled); }
    
    bool isShowVerticesEnabled() const { return m_display.isShowVerticesEnabled(); }
    virtual void setShowVertices(bool enabled) { m_display.setShowVertices(enabled); }
    
    double getEdgeWidth() const { return m_display.getEdgeWidth(); }
    virtual void setEdgeWidth(double width) { m_display.setEdgeWidth(width); }
    
    double getVertexSize() const { return m_display.getVertexSize(); }
    virtual void setVertexSize(double size) { m_display.setVertexSize(size); }
    
    Quantity_Color getEdgeColor() const { return m_display.getEdgeColor(); }
    virtual void setEdgeColor(const Quantity_Color& color) { m_display.setEdgeColor(color); }
    
    Quantity_Color getVertexColor() const { return m_display.getVertexColor(); }
    virtual void setVertexColor(const Quantity_Color& color) { m_display.setVertexColor(color); }

    bool isWireframeMode() const { return m_display.isWireframeMode(); }
    virtual void setWireframeMode(bool wireframe) { m_display.setWireframeMode(wireframe); }
    
    bool isShowWireframe() const { return m_display.isShowWireframe(); }
    void setShowWireframe(bool enabled) { m_display.setShowWireframe(enabled); }
    
    bool isSmoothNormalsEnabled() const { return m_display.isSmoothNormalsEnabled(); }
    virtual void setSmoothNormals(bool enabled) { m_display.setSmoothNormals(enabled); }
    
    double getWireframeWidth() const { return m_display.getWireframeWidth(); }
    virtual void setWireframeWidth(double width) { m_display.setWireframeWidth(width); }
    
    double getPointSize() const { return m_display.getPointSize(); }
    virtual void setPointSize(double size) { m_display.setPointSize(size); }

    void setFaceDisplay(bool enable) { m_display.setFaceDisplay(enable); }
    void setFacesVisible(bool visible) { m_display.setFacesVisible(visible); }
    void setWireframeOverlay(bool enable) { m_display.setWireframeOverlay(enable); }
    void setEdgeDisplay(bool enable) { m_display.setEdgeDisplay(enable); }
    void setFeatureEdgeDisplay(bool enable) { m_display.setFeatureEdgeDisplay(enable); }
    void setNormalDisplay(bool enable) { m_display.setNormalDisplay(enable); }

    // Quality functionality
    RenderingConfig::RenderingQuality getRenderingQuality() const { return m_quality.getRenderingQuality(); }
    virtual void setRenderingQuality(RenderingConfig::RenderingQuality quality) { m_quality.setRenderingQuality(quality); }
    
    int getTessellationLevel() const { return m_quality.getTessellationLevel(); }
    virtual void setTessellationLevel(int level) { m_quality.setTessellationLevel(level); }
    
    int getAntiAliasingSamples() const { return m_quality.getAntiAliasingSamples(); }
    virtual void setAntiAliasingSamples(int samples) { m_quality.setAntiAliasingSamples(samples); }
    
    bool isLODEnabled() const { return m_quality.isLODEnabled(); }
    virtual void setEnableLOD(bool enabled) { m_quality.setEnableLOD(enabled); }
    
    double getLODDistance() const { return m_quality.getLODDistance(); }
    virtual void setLODDistance(double distance) { m_quality.setLODDistance(distance); }

    // Shadow settings
    RenderingConfig::ShadowMode getShadowMode() const { return m_quality.getShadowMode(); }
    virtual void setShadowMode(RenderingConfig::ShadowMode mode) { m_quality.setShadowMode(mode); }
    
    double getShadowIntensity() const { return m_quality.getShadowIntensity(); }
    virtual void setShadowIntensity(double intensity) { m_quality.setShadowIntensity(intensity); }
    
    double getShadowSoftness() const { return m_quality.getShadowSoftness(); }
    virtual void setShadowSoftness(double softness) { m_quality.setShadowSoftness(softness); }
    
    int getShadowMapSize() const { return m_quality.getShadowMapSize(); }
    virtual void setShadowMapSize(int size) { m_quality.setShadowMapSize(size); }
    
    double getShadowBias() const { return m_quality.getShadowBias(); }
    virtual void setShadowBias(double bias) { m_quality.setShadowBias(bias); }

    // Lighting model settings
    RenderingConfig::LightingModel getLightingModel() const { return m_quality.getLightingModel(); }
    virtual void setLightingModel(RenderingConfig::LightingModel model) { m_quality.setLightingModel(model); }
    
    double getRoughness() const { return m_quality.getRoughness(); }
    virtual void setRoughness(double roughness) { m_quality.setRoughness(roughness); }
    
    double getMetallic() const { return m_quality.getMetallic(); }
    virtual void setMetallic(double metallic) { m_quality.setMetallic(metallic); }
    
    double getFresnel() const { return m_quality.getFresnel(); }
    virtual void setFresnel(double fresnel) { m_quality.setFresnel(fresnel); }
    
    double getSubsurfaceScattering() const { return m_quality.getSubsurfaceScattering(); }
    virtual void setSubsurfaceScattering(double scattering) { m_quality.setSubsurfaceScattering(scattering); }

    virtual void applyAdvancedParameters(const AdvancedGeometryParameters& params) { m_quality.applyAdvancedParameters(params); }
    virtual void updateFromRenderingConfig() { m_quality.updateFromRenderingConfig(); }
    virtual void updateMaterialForLighting() { m_material.updateMaterialForLighting(); }
    virtual void forceTextureUpdate() { m_appearance.forceTextureUpdate(); }

    bool isSmoothingEnabled() const { return m_quality.isSmoothingEnabled(); }
    int getSmoothingIterations() const { return m_quality.getSmoothingIterations(); }
    bool isSubdivisionEnabled() const { return m_quality.isSubdivisionEnabled(); }
    int getSubdivisionLevel() const { return m_quality.getSubdivisionLevel(); }

    void addLODLevel(double distance, double deflection) { m_quality.addLODLevel(distance, deflection); }
    int getLODLevel(double viewDistance) const { return m_quality.getLODLevel(viewDistance); }

    // Mesh functionality
    SoSeparator* getCoinNode() { return m_mesh.getCoinNode(); }
    void setCoinNode(SoSeparator* node) { m_mesh.setCoinNode(node); }
    
    void regenerateMesh(const MeshParameters& params) { m_mesh.regenerateMesh(m_core.getShape(), params); }
    void buildCoinRepresentation(const MeshParameters& params = MeshParameters()) { 
        m_mesh.buildCoinRepresentation(m_core.getShape(), params); 
    }
    void buildCoinRepresentation(const MeshParameters& params,
        const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
        const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
        double shininess, double transparency) {
        m_mesh.buildCoinRepresentation(m_core.getShape(), params, diffuseColor, ambientColor, 
            specularColor, emissiveColor, shininess, transparency);
    }

    bool needsMeshRegeneration() const { return m_mesh.needsMeshRegeneration(); }
    void setMeshRegenerationNeeded(bool needed) { m_mesh.setMeshRegenerationNeeded(needed); }
    void updateCoinRepresentationIfNeeded(const MeshParameters& params) { 
        m_mesh.updateCoinRepresentationIfNeeded(m_core.getShape(), params); 
    }
    void forceCoinRepresentationRebuild(const MeshParameters& params) { 
        m_mesh.forceCoinRepresentationRebuild(m_core.getShape(), params); 
    }

    // Edge component (delegate to mesh module)
    std::unique_ptr<EdgeComponent>& getEdgeComponent() { return m_mesh.edgeComponent; }
    void setEdgeDisplayType(EdgeType type, bool show) { m_mesh.setEdgeDisplayType(type, show); }
    bool isEdgeDisplayTypeEnabled(EdgeType type) const { return m_mesh.isEdgeDisplayTypeEnabled(type); }
    void updateEdgeDisplay() { m_mesh.updateEdgeDisplay(); }
    bool hasOriginalEdges() const { return m_mesh.hasOriginalEdges(); }

    // Assembly level
    int getAssemblyLevel() const { return m_mesh.getAssemblyLevel(); }
    void setAssemblyLevel(int level) { m_mesh.setAssemblyLevel(level); }

    // Face index mapping
    const std::vector<FaceIndexMapping>& getFaceIndexMappings() const { return m_mesh.getFaceIndexMappings(); }
    void setFaceIndexMappings(const std::vector<FaceIndexMapping>& mappings) { m_mesh.setFaceIndexMappings(mappings); }
    int getGeometryFaceIdForTriangle(int triangleIndex) const { return m_mesh.getGeometryFaceIdForTriangle(triangleIndex); }
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const { return m_mesh.getTrianglesForGeometryFace(geometryFaceId); }
    bool hasFaceIndexMapping() const { return m_mesh.hasFaceIndexMapping(); }
    void buildFaceIndexMapping(const MeshParameters& params = MeshParameters()) { 
        m_mesh.buildFaceIndexMapping(m_core.getShape(), params); 
    }

    // Memory optimization
    void releaseTemporaryData() { m_mesh.releaseTemporaryData(); }
    void optimizeMemory() { m_mesh.optimizeMemory(); }

protected:
    // Composition: All geometry aspects are separate modules
    OCCGeometryCore m_core;
    OCCGeometryTransform m_transform;
    OCCGeometryMaterial m_material;
    OCCGeometryAppearance m_appearance;
    OCCGeometryDisplay m_display;
    OCCGeometryQuality m_quality;
    OCCGeometryMesh m_mesh;
};
