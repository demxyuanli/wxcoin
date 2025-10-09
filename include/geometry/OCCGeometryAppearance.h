#pragma once

#include <string>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/RenderingConfig.h"

/**
 * @brief Geometry appearance properties
 * 
 * Manages visual appearance: color, transparency, texture, and blend settings
 */
class OCCGeometryAppearance {
public:
    OCCGeometryAppearance();
    virtual ~OCCGeometryAppearance() = default;

    // Basic appearance
    Quantity_Color getColor() const { return m_color; }
    virtual void setColor(const Quantity_Color& color);

    double getTransparency() const { return m_transparency; }
    virtual void setTransparency(double transparency);

    bool isVisible() const { return m_visible; }
    virtual void setVisible(bool visible);

    bool isSelected() const { return m_selected; }
    virtual void setSelected(bool selected);

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

    // Force texture update
    virtual void forceTextureUpdate();

protected:
    bool m_visible;
    bool m_selected;
    Quantity_Color m_color;
    double m_transparency;

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
};
