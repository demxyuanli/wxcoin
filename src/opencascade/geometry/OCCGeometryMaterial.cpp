#include "geometry/OCCGeometryMaterial.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoMaterial.h>

OCCGeometryMaterial::OCCGeometryMaterial()
    : m_materialAmbientColor(0.5, 0.5, 0.5, Quantity_TOC_RGB)
    , m_materialDiffuseColor(0.95, 0.95, 0.95, Quantity_TOC_RGB)
    , m_materialSpecularColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
    , m_materialEmissiveColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_materialShininess(50.0)
    , m_materialExplicitlySet(false)
    , m_coinMaterial(nullptr)
{
    m_coinMaterial = new SoMaterial();
    m_coinMaterial->ref();
}

OCCGeometryMaterial::~OCCGeometryMaterial()
{
    if (m_coinMaterial) {
        m_coinMaterial->unref();
        m_coinMaterial = nullptr;
    }
}

void OCCGeometryMaterial::setMaterialAmbientColor(const Quantity_Color& color)
{
    m_materialAmbientColor = color;
    m_materialExplicitlySet = true;
    updateCoinMaterial();
}

void OCCGeometryMaterial::setMaterialDiffuseColor(const Quantity_Color& color)
{
    m_materialDiffuseColor = color;
    m_materialExplicitlySet = true;
    updateCoinMaterial();
}

void OCCGeometryMaterial::setMaterialSpecularColor(const Quantity_Color& color)
{
    m_materialSpecularColor = color;
    m_materialExplicitlySet = true;
    updateCoinMaterial();
}

void OCCGeometryMaterial::setMaterialEmissiveColor(const Quantity_Color& color)
{
    m_materialEmissiveColor = color;
    m_materialExplicitlySet = true;
    updateCoinMaterial();
}

void OCCGeometryMaterial::setMaterialShininess(double shininess)
{
    m_materialShininess = shininess;
    m_materialExplicitlySet = true;
    updateCoinMaterial();
}

void OCCGeometryMaterial::setDefaultBrightMaterial()
{
    // Set bright material suitable for rendering without textures
    m_materialAmbientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
    m_materialDiffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    m_materialSpecularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    m_materialEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    m_materialShininess = 80.0;
    
    m_materialExplicitlySet = true;
    updateCoinMaterial();
    
    LOG_INF_S("Applied default bright material");
}

void OCCGeometryMaterial::updateMaterialForLighting()
{
    // Optimize material for better lighting response
    if (!m_materialExplicitlySet) {
        // Increase ambient slightly for better visibility
        double ambient = 0.4;
        m_materialAmbientColor = Quantity_Color(ambient, ambient, ambient, Quantity_TOC_RGB);
        
        // Keep diffuse bright
        m_materialDiffuseColor = Quantity_Color(0.95, 0.95, 0.95, Quantity_TOC_RGB);
        
        // Strong specular for highlights
        m_materialSpecularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        
        // Moderate shininess
        m_materialShininess = 60.0;
        
        updateCoinMaterial();
    }
}

void OCCGeometryMaterial::updateCoinMaterial()
{
    if (!m_coinMaterial) return;
    
    // Set ambient color
    m_coinMaterial->ambientColor.setValue(
        static_cast<float>(m_materialAmbientColor.Red()),
        static_cast<float>(m_materialAmbientColor.Green()),
        static_cast<float>(m_materialAmbientColor.Blue())
    );
    
    // Set diffuse color
    m_coinMaterial->diffuseColor.setValue(
        static_cast<float>(m_materialDiffuseColor.Red()),
        static_cast<float>(m_materialDiffuseColor.Green()),
        static_cast<float>(m_materialDiffuseColor.Blue())
    );
    
    // Set specular color
    m_coinMaterial->specularColor.setValue(
        static_cast<float>(m_materialSpecularColor.Red()),
        static_cast<float>(m_materialSpecularColor.Green()),
        static_cast<float>(m_materialSpecularColor.Blue())
    );
    
    // Set emissive color
    m_coinMaterial->emissiveColor.setValue(
        static_cast<float>(m_materialEmissiveColor.Red()),
        static_cast<float>(m_materialEmissiveColor.Green()),
        static_cast<float>(m_materialEmissiveColor.Blue())
    );
    
    // Set shininess (Coin3D uses 0-1 range, convert from 0-128)
    float shininess = static_cast<float>(m_materialShininess / 128.0);
    if (shininess > 1.0f) shininess = 1.0f;
    if (shininess < 0.0f) shininess = 0.0f;
    m_coinMaterial->shininess.setValue(shininess);
}
