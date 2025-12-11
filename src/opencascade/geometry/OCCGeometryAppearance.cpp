#include "geometry/OCCGeometryAppearance.h"
#include "config/RenderingConfig.h"

OCCGeometryAppearance::OCCGeometryAppearance()
    : m_visible(true)
    , m_selected(false)
    , m_color(0.8, 0.8, 0.8, Quantity_TOC_RGB)
    , m_transparency(0.0)
    , m_textureColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
    , m_textureIntensity(1.0)
    , m_textureEnabled(false)
    , m_textureMode(RenderingConfig::TextureMode::Replace)
    , m_blendMode(RenderingConfig::BlendMode::None)
    , m_depthTest(true)
    , m_depthWrite(true)
    , m_cullFace(true)
    , m_alphaThreshold(0.1)
{
    // Get blend settings from configuration
    auto blendSettings = RenderingConfig::getInstance().getBlendSettings();
    m_blendMode = blendSettings.blendMode;
    m_depthTest = blendSettings.depthTest;
    m_depthWrite = blendSettings.depthWrite;
    m_cullFace = blendSettings.cullFace;
    m_alphaThreshold = blendSettings.alphaThreshold;
}

void OCCGeometryAppearance::setColor(const Quantity_Color& color)
{
    m_color = color;
}

void OCCGeometryAppearance::setTransparency(double transparency)
{
    if (transparency < 0.0) transparency = 0.0;
    if (transparency > 1.0) transparency = 1.0;
    m_transparency = transparency;
}

void OCCGeometryAppearance::setVisible(bool visible)
{
    m_visible = visible;
}

void OCCGeometryAppearance::setSelected(bool selected)
{
    m_selected = selected;
}

void OCCGeometryAppearance::setTextureColor(const Quantity_Color& color)
{
    m_textureColor = color;
}

void OCCGeometryAppearance::setTextureIntensity(double intensity)
{
    if (intensity < 0.0) intensity = 0.0;
    if (intensity > 1.0) intensity = 1.0;
    m_textureIntensity = intensity;
}

void OCCGeometryAppearance::setTextureEnabled(bool enabled)
{
    m_textureEnabled = enabled;
}

void OCCGeometryAppearance::setTextureImagePath(const std::string& path)
{
    m_textureImagePath = path;
    if (!path.empty()) {
    }
}

void OCCGeometryAppearance::setTextureMode(RenderingConfig::TextureMode mode)
{
    m_textureMode = mode;
}

void OCCGeometryAppearance::setBlendMode(RenderingConfig::BlendMode mode)
{
    m_blendMode = mode;
}

void OCCGeometryAppearance::setDepthTest(bool enabled)
{
    m_depthTest = enabled;
}

void OCCGeometryAppearance::setDepthWrite(bool enabled)
{
    m_depthWrite = enabled;
}

void OCCGeometryAppearance::setCullFace(bool enabled)
{
    m_cullFace = enabled;
}

void OCCGeometryAppearance::setAlphaThreshold(double threshold)
{
    if (threshold < 0.0) threshold = 0.0;
    if (threshold > 1.0) threshold = 1.0;
    m_alphaThreshold = threshold;
}

void OCCGeometryAppearance::forceTextureUpdate()
{
    if (m_textureEnabled) {
        // Force texture reload/update in rendering pipeline
        // This would trigger re-application of texture in the actual rendering code
    }
}
