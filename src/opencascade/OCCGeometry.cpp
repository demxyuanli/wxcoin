#include "OCCGeometry.h"
#include "edges/ModularEdgeComponent.h"
#include "logger/Logger.h"
#include "config/RenderingConfig.h"
#include "config/EdgeSettingsConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/GeometryProcessor.h"
#include <Standard_Failure.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoNode.h>
#include <chrono>
#include <fstream>
#include <cmath>

OCCGeometry::OCCGeometry(const std::string& name)
    : OCCGeometryCore(name)
    , OCCGeometryTransform()
    , OCCGeometryAppearance()
    , OCCGeometryMaterial()
    , OCCGeometryDisplay()
    , OCCGeometryQuality()
    , OCCGeometryMesh()
{

    // Apply settings from RenderingConfig
    updateFromRenderingConfig();

    // Ensure edge display is disabled by default to avoid conflicts
    setShowEdges(false);
    setShowWireframe(false);
}

OCCGeometry::~OCCGeometry()
{
}

void OCCGeometry::setShape(const TopoDS_Shape& shape)
{
    // Validate shape before assignment
    if (shape.IsNull()) {
        LOG_WRN_S("OCCGeometry::setShape called with null shape for: " + getName());
        return;
    }
    
    // Additional validation: check for degenerate geometry
    try {
        Bnd_Box bbox;
        BRepBndLib::Add(shape, bbox);
        if (bbox.IsVoid()) {
            LOG_WRN_S("Shape has void bounding box for: " + getName() + ", skipping");
            return;
        }
        
        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        Standard_Real sizeX = xmax - xmin;
        Standard_Real sizeY = ymax - ymin;
        Standard_Real sizeZ = zmax - zmin;
        
        // Check for degenerate shapes (too small in all dimensions)
        Standard_Real minSize = 1e-6;
        if (sizeX < minSize && sizeY < minSize && sizeZ < minSize) {
            LOG_WRN_S("Shape is too small (degenerate) for: " + getName() + ", skipping");
            return;
        }
        
        // Check for infinite or NaN values
        if (std::isnan(sizeX) || std::isnan(sizeY) || std::isnan(sizeZ) ||
            std::isinf(sizeX) || std::isinf(sizeY) || std::isinf(sizeZ)) {
            LOG_WRN_S("Shape has invalid dimensions (NaN or infinite) for: " + getName() + ", skipping");
            return;
        }
    }
    catch (const Standard_Failure& e) {
        LOG_WRN_S("Failed to validate shape bounding box for: " + getName() + ": " + std::string(e.GetMessageString()));
        // Continue with shape assignment despite validation failure
    }
    
    try {
        // Call base class setShape
        OCCGeometryCore::setShape(shape);
        // Mark that mesh needs regeneration
        setMeshRegenerationNeeded(true);
    }
    catch (const Standard_Failure& e) {
        LOG_ERR_S("OpenCASCADE error in setShape for " + getName() + ": " + std::string(e.GetMessageString()));
        throw;
    }
}

void OCCGeometry::setColor(const Quantity_Color& color)
{
    // Set color in appearance module
    OCCGeometryAppearance::setColor(color);
    
    // Also update material diffuse color
    setMaterialDiffuseColor(color);
    
    // Mark mesh regeneration needed to update Coin3D representation
    setMeshRegenerationNeeded(true);
}

void OCCGeometry::setTransparency(double transparency)
{
    // Clamp to valid range
    if (transparency < 0.0) transparency = 0.0;
    if (transparency > 1.0) transparency = 1.0;
    
    // Set in appearance module
    OCCGeometryAppearance::setTransparency(transparency);
    
    // If wireframe mode, limit transparency for visible lines
    if (isWireframeMode()) {
        transparency = std::min(transparency, 0.6);
    }
    
    // Mark mesh regeneration needed
    setMeshRegenerationNeeded(true);
}

void OCCGeometry::setPosition(const gp_Pnt& position)
{
    // Call base class to update position
    OCCGeometryTransform::setPosition(position);
    
    // Trigger mesh rebuild to apply new position (will create coinNode if needed)
    if (!getShape().IsNull()) {
        MeshParameters params;
        buildCoinRepresentation(params);
        if (getCoinNode()) {
            getCoinNode()->touch();
        }
    }
}

void OCCGeometry::setRotation(const gp_Vec& axis, double angle)
{
    // Call base class to update rotation
    OCCGeometryTransform::setRotation(axis, angle);
    
    // Trigger mesh rebuild to apply new rotation (will create coinNode if needed)
    if (!getShape().IsNull()) {
        MeshParameters params;
        buildCoinRepresentation(params);
        if (getCoinNode()) {
            getCoinNode()->touch();
        }
    }
}

void OCCGeometry::setScale(double scale)
{
    // Call base class to update scale
    OCCGeometryTransform::setScale(scale);
    
    // Trigger mesh rebuild to apply new scale (will create coinNode if needed)
    if (!getShape().IsNull()) {
        MeshParameters params;
        buildCoinRepresentation(params);
        if (getCoinNode()) {
            getCoinNode()->touch();
        }
    }
}

void OCCGeometry::setWireframeMode(bool wireframe)
{
    if (isWireframeMode() != wireframe) {
        // Set in display module
        OCCGeometryDisplay::setWireframeMode(wireframe);
        
        // Mark mesh regeneration needed
        setMeshRegenerationNeeded(true);
        
        // Force rebuild of Coin3D representation to apply wireframe mode change immediately
        if (getCoinNode() && !getShape().IsNull()) {
            MeshParameters params;
            buildCoinRepresentation(params);
            getCoinNode()->touch();
        }
    }
}

void OCCGeometry::updateCoinRepresentationIfNeeded(const MeshParameters& params)
{
    // Use the new modular interface instead of legacy version
    if (needsMeshRegeneration() && !getShape().IsNull()) {
        buildCoinRepresentation(params);
    }
}

SoSeparator* OCCGeometry::getCoinNodeWithShape()
{
    // Update representation if needed before returning node
    if (needsMeshRegeneration() && !getShape().IsNull()) {
        MeshParameters params;
        buildCoinRepresentation(params);
    }
    return getCoinNode();
}

void OCCGeometry::applyAdvancedParameters(const AdvancedGeometryParameters& params)
{
    // Apply material parameters
    setMaterialDiffuseColor(params.materialDiffuseColor);
    setMaterialAmbientColor(params.materialAmbientColor);
    setMaterialSpecularColor(params.materialSpecularColor);
    setMaterialEmissiveColor(params.materialEmissiveColor);
    setMaterialShininess(params.materialShininess);
    setTransparency(params.materialTransparency);

    // Apply texture parameters
    setTextureEnabled(params.textureEnabled);
    setTextureImagePath(params.texturePath);
    setTextureMode(params.textureMode);

    // Apply display parameters
    setShowEdges(params.showEdges);
    setShowWireframe(params.showWireframe);
    setSmoothNormals(params.showNormals);

    // Apply edge display types to modular edge component
    if (modularEdgeComponent) {
        modularEdgeComponent->setEdgeDisplayType(EdgeType::Original, params.showOriginalEdges);
        modularEdgeComponent->setEdgeDisplayType(EdgeType::Feature, params.showFeatureEdges);
        modularEdgeComponent->setEdgeDisplayType(EdgeType::Mesh, params.showMeshEdges);
    }

    // Also apply to OCCGeometryMesh components
    OCCGeometryMesh::setEdgeDisplayType(EdgeType::Original, params.showOriginalEdges);
    OCCGeometryMesh::setEdgeDisplayType(EdgeType::Feature, params.showFeatureEdges);
    OCCGeometryMesh::setEdgeDisplayType(EdgeType::Mesh, params.showMeshEdges);

    // Apply subdivision settings
    m_subdivisionEnabled = params.subdivisionEnabled;
    m_subdivisionLevels = params.subdivisionLevels;

    // Apply blend settings
    setBlendMode(params.blendMode);
    setDepthTest(params.depthTest);
    setCullFace(params.backfaceCulling);

    // Apply advanced quality parameters
    OCCGeometryQuality::applyAdvancedParameters(params);

    // Force rebuild of Coin3D representation
    setMeshRegenerationNeeded(true);
}

void OCCGeometry::updateFromRenderingConfig()
{
    // Load current settings from configuration
    RenderingConfig& config = RenderingConfig::getInstance();
    const auto& materialSettings = config.getMaterialSettings();
    const auto& textureSettings = config.getTextureSettings();
    const auto& blendSettings = config.getBlendSettings();

    // Only update material if not explicitly set for this geometry
    if (!isMaterialExplicitlySet()) {
        setColor(materialSettings.diffuseColor);
        setTransparency(materialSettings.transparency);
        setMaterialAmbientColor(materialSettings.ambientColor);
        setMaterialDiffuseColor(materialSettings.diffuseColor);
        setMaterialSpecularColor(materialSettings.specularColor);
        setMaterialShininess(materialSettings.shininess);
    }

    // Update texture settings
    setTextureColor(textureSettings.color);
    setTextureIntensity(textureSettings.intensity);
    setTextureEnabled(textureSettings.enabled);
    setTextureImagePath(textureSettings.imagePath);
    setTextureMode(textureSettings.textureMode);

    // Update blend settings
    setBlendMode(blendSettings.blendMode);
    setDepthTest(blendSettings.depthTest);
    setDepthWrite(blendSettings.depthWrite);
    setCullFace(blendSettings.cullFace);
    setAlphaThreshold(blendSettings.alphaThreshold);

    // Update quality settings
    OCCGeometryQuality::updateFromRenderingConfig();

    // Force rebuild of Coin3D representation
    setMeshRegenerationNeeded(true);
}

void OCCGeometry::setFaceDisplay(bool enable)
{
    setFacesVisible(enable);
    setMeshRegenerationNeeded(true);
}

void OCCGeometry::buildFaceIndexMapping(const MeshParameters& params)
{
    // Domain system - this method now builds domain mapping instead of legacy mapping
    if (!getShape().IsNull()) {
        // The actual implementation is now in the base class buildFaceIndexMapping method
        // which has been updated to build domain mappings instead
        // LOG_INF_S("Building domain face mapping for geometry");
        // The base class will handle the actual domain building
    }
}

// Full Coin3D scene graph construction - thin wrapper for modular implementation
void OCCGeometry::buildCoinRepresentation(const MeshParameters& params)
{
    if (getShape().IsNull()) {
        LOG_WRN_S("Cannot build coin representation for null shape");
        return;
    }

    // Create render context from current state
    GeometryRenderContext context = GeometryRenderContext::fromGeometry(*this);
    
    // Delegate to Mesh module for actual rendering
    OCCGeometryMesh::buildCoinRepresentation(getShape(), params, context);
    
    setMeshRegenerationNeeded(false);
}

void OCCGeometry::buildCoinRepresentation(const MeshParameters& params,
    const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
    const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
    double shininess, double transparency)
{
    // Temporarily set material properties
    setMaterialDiffuseColor(diffuseColor);
    setMaterialAmbientColor(ambientColor);
    setMaterialSpecularColor(specularColor);
    setMaterialEmissiveColor(emissiveColor);
    setMaterialShininess(shininess);
    setTransparency(transparency);
    
    // Build with current parameters
    buildCoinRepresentation(params);
}

// Incremental intersection node API implementation
void OCCGeometry::addSingleIntersectionNode(const gp_Pnt& point, const Quantity_Color& color, double size) {
    if (modularEdgeComponent) {
        modularEdgeComponent->addSingleIntersectionNode(point, color, size);
    }
}

void OCCGeometry::addBatchIntersectionNodes(const std::vector<gp_Pnt>& points, const Quantity_Color& color, double size) {
    if (modularEdgeComponent) {
        modularEdgeComponent->addBatchIntersectionNodes(points, color, size);
    }
}

void OCCGeometry::clearIntersectionNodes() {
    if (modularEdgeComponent) {
        modularEdgeComponent->clearIntersectionNodes();
    }
}

bool OCCGeometry::hasIntersectionNodes() const {
    if (modularEdgeComponent) {
        return modularEdgeComponent->hasIntersectionNodes();
    }
    return false;
}

// No longer needed - wireframe is handled internally by Mesh module

