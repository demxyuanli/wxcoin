#pragma once

#include <OpenCASCADE/Quantity_Color.hxx>

// Forward declarations
class SoMaterial;

/**
 * @brief Geometry material properties
 * 
 * Manages material properties: ambient, diffuse, specular, emissive colors and shininess
 */
class OCCGeometryMaterial {
public:
    OCCGeometryMaterial();
    virtual ~OCCGeometryMaterial();

    // Material color accessors
    Quantity_Color getMaterialAmbientColor() const { return m_materialAmbientColor; }
    virtual void setMaterialAmbientColor(const Quantity_Color& color);

    Quantity_Color getMaterialDiffuseColor() const { return m_materialDiffuseColor; }
    virtual void setMaterialDiffuseColor(const Quantity_Color& color);

    Quantity_Color getMaterialSpecularColor() const { return m_materialSpecularColor; }
    virtual void setMaterialSpecularColor(const Quantity_Color& color);

    Quantity_Color getMaterialEmissiveColor() const { return m_materialEmissiveColor; }
    virtual void setMaterialEmissiveColor(const Quantity_Color& color);

    double getMaterialShininess() const { return m_materialShininess; }
    virtual void setMaterialShininess(double shininess);

    // Material presets
    virtual void setDefaultBrightMaterial();
    virtual void updateMaterialForLighting();

    // Material tracking
    virtual void resetMaterialExplicitFlag() { m_materialExplicitlySet = false; }
    virtual bool isMaterialExplicitlySet() const { return m_materialExplicitlySet; }

    // Coin3D material node management
    SoMaterial* getCoinMaterial() const { return m_coinMaterial; }
    void updateCoinMaterial();

protected:
    Quantity_Color m_materialAmbientColor;
    Quantity_Color m_materialDiffuseColor;
    Quantity_Color m_materialSpecularColor;
    Quantity_Color m_materialEmissiveColor;
    double m_materialShininess;
    bool m_materialExplicitlySet;

    SoMaterial* m_coinMaterial;
};
