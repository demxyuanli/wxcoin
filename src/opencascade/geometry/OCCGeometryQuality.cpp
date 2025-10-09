#include "geometry/OCCGeometryQuality.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"

OCCGeometryQuality::OCCGeometryQuality()
    : m_renderingQuality(RenderingConfig::RenderingQuality::Normal)
    , m_tessellationLevel(2)
    , m_antiAliasingSamples(4)
    , m_enableLOD(false)
    , m_lodDistance(1000.0)
    , m_shadowMode(RenderingConfig::ShadowMode::None)
    , m_shadowIntensity(0.5)
    , m_shadowSoftness(0.5)
    , m_shadowMapSize(1024)
    , m_shadowBias(0.001)
    , m_lightingModel(RenderingConfig::LightingModel::BlinnPhong)
    , m_roughness(0.5)
    , m_metallic(0.0)
    , m_fresnel(0.04)
    , m_subsurfaceScattering(0.0)
    , m_lastSmoothingEnabled(false)
    , m_lastSmoothingIterations(2)
    , m_lastSmoothingStrength(0.5)
    , m_lastSmoothingCreaseAngle(30.0)
    , m_lastSubdivisionEnabled(false)
    , m_lastSubdivisionLevel(2)
    , m_lastSubdivisionMethod(0)
    , m_lastSubdivisionCreaseAngle(30.0)
    , m_lastTessellationMethod(0)
    , m_lastTessellationQuality(2)
    , m_lastFeaturePreservation(0.5)
    , m_lastAdaptiveMeshing(false)
    , m_lastParallelProcessing(true)
{
}

void OCCGeometryQuality::setRenderingQuality(RenderingConfig::RenderingQuality quality)
{
    m_renderingQuality = quality;
    
    // Adjust related parameters based on quality
    switch (quality) {
        case RenderingConfig::RenderingQuality::Draft:
            m_tessellationLevel = 1;
            m_antiAliasingSamples = 0;
            break;
        case RenderingConfig::RenderingQuality::Normal:
            m_tessellationLevel = 2;
            m_antiAliasingSamples = 4;
            break;
        case RenderingConfig::RenderingQuality::High:
            m_tessellationLevel = 3;
            m_antiAliasingSamples = 8;
            break;
        case RenderingConfig::RenderingQuality::Ultra:
            m_tessellationLevel = 4;
            m_antiAliasingSamples = 16;
            break;
        case RenderingConfig::RenderingQuality::Realtime:
            m_tessellationLevel = 1;
            m_antiAliasingSamples = 2;
            break;
    }
    
    LOG_INF_S("Rendering quality set to: " + std::to_string(static_cast<int>(quality)));
}

void OCCGeometryQuality::setTessellationLevel(int level)
{
    if (level < 0) level = 0;
    if (level > 5) level = 5;
    m_tessellationLevel = level;
}

void OCCGeometryQuality::setAntiAliasingSamples(int samples)
{
    // Valid samples: 0, 2, 4, 8, 16
    if (samples < 0) samples = 0;
    if (samples > 16) samples = 16;
    m_antiAliasingSamples = samples;
}

void OCCGeometryQuality::setEnableLOD(bool enabled)
{
    m_enableLOD = enabled;
    LOG_INF_S(std::string("LOD: ") + (enabled ? "enabled" : "disabled"));
}

void OCCGeometryQuality::setLODDistance(double distance)
{
    if (distance < 0.0) distance = 0.0;
    m_lodDistance = distance;
}

void OCCGeometryQuality::addLODLevel(double distance, double deflection)
{
    m_lodLevels.push_back(std::make_pair(distance, deflection));
    LOG_INF_S("Added LOD level at distance: " + std::to_string(distance));
}

int OCCGeometryQuality::getLODLevel(double viewDistance) const
{
    if (m_lodLevels.empty()) return 0;
    
    for (size_t i = 0; i < m_lodLevels.size(); ++i) {
        if (viewDistance < m_lodLevels[i].first) {
            return static_cast<int>(i);
        }
    }
    
    return static_cast<int>(m_lodLevels.size());
}

void OCCGeometryQuality::setShadowMode(RenderingConfig::ShadowMode mode)
{
    m_shadowMode = mode;
}

void OCCGeometryQuality::setShadowIntensity(double intensity)
{
    if (intensity < 0.0) intensity = 0.0;
    if (intensity > 1.0) intensity = 1.0;
    m_shadowIntensity = intensity;
}

void OCCGeometryQuality::setShadowSoftness(double softness)
{
    if (softness < 0.0) softness = 0.0;
    if (softness > 1.0) softness = 1.0;
    m_shadowSoftness = softness;
}

void OCCGeometryQuality::setShadowMapSize(int size)
{
    // Power of 2: 256, 512, 1024, 2048, 4096
    if (size < 256) size = 256;
    if (size > 4096) size = 4096;
    m_shadowMapSize = size;
}

void OCCGeometryQuality::setShadowBias(double bias)
{
    if (bias < 0.0) bias = 0.0;
    if (bias > 0.1) bias = 0.1;
    m_shadowBias = bias;
}

void OCCGeometryQuality::setLightingModel(RenderingConfig::LightingModel model)
{
    m_lightingModel = model;
    LOG_INF_S("Lighting model changed");
}

void OCCGeometryQuality::setRoughness(double roughness)
{
    if (roughness < 0.0) roughness = 0.0;
    if (roughness > 1.0) roughness = 1.0;
    m_roughness = roughness;
}

void OCCGeometryQuality::setMetallic(double metallic)
{
    if (metallic < 0.0) metallic = 0.0;
    if (metallic > 1.0) metallic = 1.0;
    m_metallic = metallic;
}

void OCCGeometryQuality::setFresnel(double fresnel)
{
    if (fresnel < 0.0) fresnel = 0.0;
    if (fresnel > 1.0) fresnel = 1.0;
    m_fresnel = fresnel;
}

void OCCGeometryQuality::setSubsurfaceScattering(double scattering)
{
    if (scattering < 0.0) scattering = 0.0;
    if (scattering > 1.0) scattering = 1.0;
    m_subsurfaceScattering = scattering;
}

void OCCGeometryQuality::applyAdvancedParameters(const AdvancedGeometryParameters& params)
{
    // Smoothing parameters
    m_lastSmoothingEnabled = params.smoothingEnabled;
    m_lastSmoothingIterations = params.smoothingIterations;
    m_lastSmoothingStrength = params.smoothingStrength;
    m_lastSmoothingCreaseAngle = params.smoothingCreaseAngle;
    
    // Subdivision parameters
    m_lastSubdivisionEnabled = params.subdivisionEnabled;
    m_lastSubdivisionLevel = params.subdivisionLevel;
    m_lastSubdivisionMethod = params.subdivisionMethod;
    m_lastSubdivisionCreaseAngle = params.subdivisionCreaseAngle;
    
    // Tessellation parameters
    m_lastTessellationMethod = params.tessellationMethod;
    m_lastTessellationQuality = params.tessellationQuality;
    m_lastFeaturePreservation = params.featurePreservation;
    
    // Performance parameters
    m_lastAdaptiveMeshing = params.adaptiveMeshing;
    m_lastParallelProcessing = params.parallelProcessing;
    
    LOG_INF_S("Advanced parameters applied");
}

void OCCGeometryQuality::updateFromRenderingConfig()
{
    auto& config = RenderingConfig::getInstance();
    
    // Update quality settings from global config
    m_renderingQuality = config.getQualitySettings().quality;
    m_shadowMode = config.getShadowSettings().shadowMode;
    m_shadowIntensity = config.getShadowSettings().shadowIntensity;
    m_lightingModel = config.getLightingModelSettings().lightingModel;
    
    LOG_INF_S("Updated quality settings from RenderingConfig");
}
