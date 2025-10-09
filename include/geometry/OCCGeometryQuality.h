#pragma once

#include "config/RenderingConfig.h"
#include "GeometryDialogTypes.h"
#include <vector>
#include <utility>

/**
 * @brief Geometry rendering quality settings
 * 
 * Manages tessellation, LOD, shadows, lighting model, and advanced rendering parameters
 */
class OCCGeometryQuality {
public:
    OCCGeometryQuality();
    virtual ~OCCGeometryQuality() = default;

    // Quality settings
    RenderingConfig::RenderingQuality getRenderingQuality() const { return m_renderingQuality; }
    virtual void setRenderingQuality(RenderingConfig::RenderingQuality quality);

    int getTessellationLevel() const { return m_tessellationLevel; }
    virtual void setTessellationLevel(int level);

    int getAntiAliasingSamples() const { return m_antiAliasingSamples; }
    virtual void setAntiAliasingSamples(int samples);

    // LOD settings
    bool isLODEnabled() const { return m_enableLOD; }
    virtual void setEnableLOD(bool enabled);

    double getLODDistance() const { return m_lodDistance; }
    virtual void setLODDistance(double distance);

    void addLODLevel(double distance, double deflection);
    int getLODLevel(double viewDistance) const;

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

    // Advanced parameters
    virtual void applyAdvancedParameters(const AdvancedGeometryParameters& params);

    bool isSmoothingEnabled() const { return m_lastSmoothingEnabled; }
    int getSmoothingIterations() const { return m_lastSmoothingIterations; }
    bool isSubdivisionEnabled() const { return m_lastSubdivisionEnabled; }
    int getSubdivisionLevel() const { return m_lastSubdivisionLevel; }

    // Update from configuration
    virtual void updateFromRenderingConfig();

protected:
    // Quality settings
    RenderingConfig::RenderingQuality m_renderingQuality;
    int m_tessellationLevel;
    int m_antiAliasingSamples;
    
    // LOD settings
    bool m_enableLOD;
    double m_lodDistance;
    std::vector<std::pair<double, double>> m_lodLevels; // distance, deflection pairs

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

    // Advanced parameters tracking
    bool m_lastSmoothingEnabled;
    int m_lastSmoothingIterations;
    double m_lastSmoothingStrength;
    double m_lastSmoothingCreaseAngle;
    bool m_lastSubdivisionEnabled;
    int m_lastSubdivisionLevel;
    int m_lastSubdivisionMethod;
    double m_lastSubdivisionCreaseAngle;
    int m_lastTessellationMethod;
    int m_lastTessellationQuality;
    double m_lastFeaturePreservation;
    bool m_lastAdaptiveMeshing;
    bool m_lastParallelProcessing;
};
