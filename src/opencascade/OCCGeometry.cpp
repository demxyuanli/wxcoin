#include "OCCGeometry.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include <algorithm>
#include <Standard_ConstructionError.hxx>
#include <Standard_Failure.hxx>
#include "config/RenderingConfig.h"
#include "rendering/GeometryProcessor.h"
#include <limits>
#include <cmath>
#include <cstdlib>
#include <wx/gdicmn.h>
#include <chrono>
#include <fstream> // Added for file validation
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <TopoDS.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Standard_ConstructionError.hxx>
#include "EdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "OCCMeshConverter.h"

// OpenCASCADE includes
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <gp_Trsf.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <TopLoc_Location.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBndLib.hxx>

// Coin3D includes
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

// OCCGeometry base class implementation
OCCGeometry::OCCGeometry(const std::string& name)
	: m_name(name)
	, m_visible(true)
	, m_selected(false)
	, m_facesVisible(true)
	, m_transparency(0.0)
	, m_wireframeMode(false)
	, m_coinNode(nullptr)
	, m_coinTransform(nullptr)
	, m_coinNeedsUpdate(true)
	, m_meshRegenerationNeeded(true)
	, m_position(0, 0, 0)
	, m_rotationAxis(0, 0, 1)
	, m_rotationAngle(0.0)
	, m_scale(1.0)
	, m_color(0.8, 0.8, 0.8, Quantity_TOC_RGB)
	, m_materialAmbientColor(0.5, 0.5, 0.5, Quantity_TOC_RGB)
	, m_materialDiffuseColor(0.95, 0.95, 0.95, Quantity_TOC_RGB)
	, m_materialSpecularColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
	, m_materialEmissiveColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
	, m_materialShininess(50.0)
	, m_textureIntensity(1.0)
	, m_textureEnabled(false)
	, m_textureMode(RenderingConfig::TextureMode::Replace)
	, m_blendMode(RenderingConfig::BlendMode::None)
	, m_depthTest(true)
	, m_depthWrite(true)
	, m_cullFace(true)
	, m_alphaThreshold(0.1)
	, m_smoothNormals(false)
	, m_wireframeWidth(1.0)
	, m_pointSize(1.0)
	, m_subdivisionEnabled(false)
	, m_subdivisionLevels(2)
	, m_materialExplicitlySet(false) // Added flag to track if material was explicitly set
{
	// Get blend settings from configuration
	auto blendSettings = RenderingConfig::getInstance().getBlendSettings();
	m_blendMode = blendSettings.blendMode;
	m_depthTest = blendSettings.depthTest;
	m_depthWrite = blendSettings.depthWrite;
	m_cullFace = blendSettings.cullFace;
	m_alphaThreshold = blendSettings.alphaThreshold;

	// Apply settings from RenderingConfig first
	updateFromRenderingConfig();

	edgeComponent = std::make_unique<EdgeComponent>();

	// Ensure edge display is disabled by default to avoid conflicts with new EdgeComponent system
	m_showEdges = false;
	m_showWireframe = false;
}

OCCGeometry::~OCCGeometry()
{
	if (m_coinNode) {
		m_coinNode->unref();
	}
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
		Standard_Real minSize = 1e-6; // Minimum size threshold
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
		m_shape = shape;
		m_coinNeedsUpdate = true;
	}
	catch (const Standard_ConstructionError& e) {
		LOG_ERR_S("Construction error in setShape for " + getName() + ": " + std::string(e.GetMessageString()));
		LOG_ERR_S("This typically indicates invalid or degenerate geometry. Shape assignment failed.");
		throw; // Re-throw to be handled by caller
	}
	catch (const Standard_Failure& e) {
		LOG_ERR_S("OpenCASCADE error in setShape for " + getName() + ": " + std::string(e.GetMessageString()));
		throw; // Re-throw to be handled by caller
	}
}

void OCCGeometry::setPosition(const gp_Pnt& position)
{
	m_position = position;
	if (m_coinTransform) {
		m_coinTransform->translation.setValue(
			static_cast<float>(m_position.X()),
			static_cast<float>(m_position.Y()),
			static_cast<float>(m_position.Z())
		);
	}
	else {
		m_coinNeedsUpdate = true;
	}
}

void OCCGeometry::setRotation(const gp_Vec& axis, double angle)
{
	m_rotationAxis = axis;
	m_rotationAngle = angle;
	if (m_coinTransform) {
		SbVec3f rot_axis(
			static_cast<float>(m_rotationAxis.X()),
			static_cast<float>(m_rotationAxis.Y()),
			static_cast<float>(m_rotationAxis.Z())
		);
		m_coinTransform->rotation.setValue(rot_axis, static_cast<float>(m_rotationAngle));
	}
	else {
		m_coinNeedsUpdate = true;
	}
}

void OCCGeometry::setScale(double scale)
{
	m_scale = scale;
	if (m_coinTransform) {
		m_coinTransform->scaleFactor.setValue(
			static_cast<float>(m_scale),
			static_cast<float>(m_scale),
			static_cast<float>(m_scale)
		);
	}
	else {
		m_coinNeedsUpdate = true;
	}
}

void OCCGeometry::setVisible(bool visible)
{
	if (m_visible != visible) {
		m_visible = visible;
		// Update Coin3D node visibility by toggling faces display
		if (m_coinNode) {
			setFacesVisible(visible);
			m_coinNode->touch();
		}
		else {
			// Will be applied on next build
			m_coinNeedsUpdate = true;
		}
	}
}

void OCCGeometry::setSelected(bool selected)
{
	if (m_selected != selected) {
		m_selected = selected;

		// Force rebuild of Coin3D representation to update edge colors
		m_coinNeedsUpdate = true;

		if (m_coinNode) {
			// Rebuild the entire Coin3D representation to update edge colors
			buildCoinRepresentation();

			// Force a refresh of the scene to show the selection change
			m_coinNode->touch();
		}
	}
}

void OCCGeometry::setColor(const Quantity_Color& color)
{
	m_color = color;
	m_materialDiffuseColor = color;
	m_materialExplicitlySet = true; // Mark that material has been explicitly set
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setTransparency(double transparency)
{
	m_transparency = std::min(1.0, std::max(0.0, transparency));  // Clamp 0-1
	if (m_wireframeMode) {
		m_transparency = std::min(m_transparency, 0.6);  // Limit for visible lines
	}

	// Mark that the Coin representation needs update
	m_coinNeedsUpdate = true;

	if (m_coinNode) {
		// Clear old material and texture
		for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
			SoNode* child = m_coinNode->getChild(i);
			if (child && (child->isOfType(SoMaterial::getClassTypeId()) || child->isOfType(SoTexture2::getClassTypeId()))) {
				m_coinNode->removeChild(i);
			}
		}
		// Force rebuild
		buildCoinRepresentation();

		// Mark the Coin node as modified to trigger scene graph update
		m_coinNode->touch();
	}
}

void OCCGeometry::setWireframeMode(bool wireframe)
{
	if (m_wireframeMode != wireframe) {
		m_wireframeMode = wireframe;
		m_coinNeedsUpdate = true;

		// Force rebuild of Coin3D representation to apply wireframe mode change
		if (m_coinNode) {
			buildCoinRepresentation();
			m_coinNode->touch();
			LOG_INF_S("Wireframe mode changed to " + std::string(wireframe ? "enabled" : "disabled") + " for " + m_name);
		}
	}
}

// Removed setShadingMode method - functionality not needed

// Material property setters
void OCCGeometry::setMaterialAmbientColor(const Quantity_Color& color)
{
	m_materialAmbientColor = color;
	m_materialExplicitlySet = true; // Mark that material has been explicitly set
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialDiffuseColor(const Quantity_Color& color)
{
	m_materialDiffuseColor = color;
	m_materialExplicitlySet = true; // Mark that material has been explicitly set
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialSpecularColor(const Quantity_Color& color)
{
	m_materialSpecularColor = color;
	m_materialExplicitlySet = true; // Mark that material has been explicitly set
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialEmissiveColor(const Quantity_Color& color)
{
	m_materialEmissiveColor = color;
	m_materialExplicitlySet = true; // Mark that material has been explicitly set
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setMaterialShininess(double shininess)
{
	m_materialShininess = std::max(0.0, std::min(100.0, shininess));
	m_materialExplicitlySet = true; // Mark that material has been explicitly set
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setDefaultBrightMaterial()
{
	// Set bright material colors for better visibility without textures
	m_materialAmbientColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
	m_materialDiffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
	m_materialSpecularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
	m_materialShininess = 30.0; // Lower shininess for more diffuse appearance

	m_coinNeedsUpdate = true;

	if (m_coinNode) {
		// Force rebuild of Coin3D representation to update material
		buildCoinRepresentation();
		m_coinNode->touch();
	}
}

// Texture property setters
void OCCGeometry::setTextureColor(const Quantity_Color& color)
{
	m_textureColor = color;
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setTextureIntensity(double intensity)
{
	m_textureIntensity = std::max(0.0, std::min(1.0, intensity));
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setTextureEnabled(bool enabled)
{
	m_textureEnabled = enabled;
	m_coinNeedsUpdate = true;

	if (m_coinNode) {
		// Force rebuild of Coin3D representation to apply texture changes
		buildCoinRepresentation();
		m_coinNode->touch();
		LOG_INF_S("Texture enabled set to " + std::to_string(enabled) + " for " + m_name);
	}
}

void OCCGeometry::setTextureImagePath(const std::string& path)
{
	m_textureImagePath = path;
	m_coinNeedsUpdate = true;

	if (m_coinNode) {
		// Force rebuild of Coin3D representation to apply texture changes
		buildCoinRepresentation();
		m_coinNode->touch();
		LOG_INF_S("Texture image path set to " + path + " for " + m_name);
	}
}

void OCCGeometry::setTextureMode(RenderingConfig::TextureMode mode)
{
	m_textureMode = mode;
	m_coinNeedsUpdate = true;

	if (m_coinNode) {
		// Force rebuild of Coin3D representation to apply texture changes
		buildCoinRepresentation();
		m_coinNode->touch();
		LOG_INF_S("Texture mode set to " + std::to_string(static_cast<int>(mode)) + " for " + m_name);
	}
}

void OCCGeometry::setBlendMode(RenderingConfig::BlendMode mode)
{
	m_blendMode = mode;
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setDepthTest(bool enabled)
{
	m_depthTest = enabled;
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setDepthWrite(bool enabled)
{
	m_depthWrite = enabled;
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setCullFace(bool enabled)
{
	m_cullFace = enabled;
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setAlphaThreshold(double threshold)
{
	m_alphaThreshold = threshold;
	m_coinNeedsUpdate = true;
}

SoSeparator* OCCGeometry::getCoinNode()
{
	if (!m_coinNode || m_coinNeedsUpdate) {
		buildCoinRepresentation();
	}

	return m_coinNode;
}

void OCCGeometry::setCoinNode(SoSeparator* node)
{
	if (m_coinNode) {
		m_coinNode->unref();
	}
	m_coinNode = node;
	if (m_coinNode) {
		m_coinNode->ref();
	}
	m_coinNeedsUpdate = false;
}

void OCCGeometry::regenerateMesh(const MeshParameters& params)
{
	// Clear old face mapping since mesh will be regenerated
	m_faceIndexMappings.clear();
	LOG_INF_S("Regenerating mesh for " + m_name + " - face mapping will be rebuilt");
	
	buildCoinRepresentation(params);
}

// Performance optimization methods
bool OCCGeometry::needsMeshRegeneration() const
{
	return m_meshRegenerationNeeded;
}

void OCCGeometry::setMeshRegenerationNeeded(bool needed)
{
	m_meshRegenerationNeeded = needed;
}

void OCCGeometry::forceCoinRepresentationRebuild(const MeshParameters& params)
{
	try {
		// Force rebuild by marking as needed and calling buildCoinRepresentation directly
		m_meshRegenerationNeeded = true;
		
		// Clear old face mapping since mesh will be regenerated
		m_faceIndexMappings.clear();
		LOG_INF_S("Force rebuilding mesh for " + m_name + " - face mapping will be rebuilt");
		
		buildCoinRepresentation(params,
			m_materialDiffuseColor, m_materialAmbientColor,
			m_materialSpecularColor, m_materialEmissiveColor,
			m_materialShininess, m_transparency);
		m_lastMeshParams = params;
		m_meshRegenerationNeeded = false;

		// Update advanced parameter tracking after rebuild
		auto& config = RenderingToolkitAPI::getConfig();
		auto& smoothingSettings = config.getSmoothingSettings();
		auto& subdivisionSettings = config.getSubdivisionSettings();
		auto& edgeSettings = config.getEdgeSettings();

		m_lastSmoothingEnabled = smoothingSettings.enabled;
		m_lastSmoothingIterations = smoothingSettings.iterations;
		m_lastSmoothingCreaseAngle = smoothingSettings.creaseAngle;
		m_lastSubdivisionEnabled = subdivisionSettings.enabled;
		m_lastSubdivisionLevel = subdivisionSettings.levels;
		m_lastSubdivisionCreaseAngle = edgeSettings.featureEdgeAngle;

		// Update custom parameter tracking with safer conversion
		try {
			m_lastSmoothingStrength = std::stod(config.getParameter("smoothing_strength", "0.5"));
			m_lastTessellationMethod = std::stoi(config.getParameter("tessellation_method", "0"));
			m_lastTessellationQuality = std::stoi(config.getParameter("tessellation_quality", "2"));
			m_lastFeaturePreservation = std::stod(config.getParameter("feature_preservation", "0.5"));
			m_lastAdaptiveMeshing = (config.getParameter("adaptive_meshing", "false") == "true");
			m_lastParallelProcessing = (config.getParameter("parallel_processing", "true") == "true");
		}
		catch (const std::exception& e) {
			LOG_WRN_S("Error parsing custom parameters for " + m_name + ": " + std::string(e.what()));
		}

	}
	catch (const Standard_ConstructionError& e) {
		LOG_ERR_S("Standard_ConstructionError in forceCoinRepresentationRebuild for " + m_name + ": " + std::string(e.GetMessageString()));
		m_meshRegenerationNeeded = false;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception in forceCoinRepresentationRebuild for " + m_name + ": " + std::string(e.what()));
		m_meshRegenerationNeeded = false;
	}
	catch (...) {
		LOG_ERR_S("Unknown exception in forceCoinRepresentationRebuild for " + m_name);
		m_meshRegenerationNeeded = false;
	}
}

void OCCGeometry::updateCoinRepresentationIfNeeded(const MeshParameters& params)
{

	// Check if mesh parameters have changed
	bool paramsChanged = (params.deflection != m_lastMeshParams.deflection ||
		params.angularDeflection != m_lastMeshParams.angularDeflection ||
		params.relative != m_lastMeshParams.relative ||
		params.inParallel != m_lastMeshParams.inParallel);

	// Also check if advanced parameters have changed by checking RenderingToolkitAPI config
	// This ensures that smoothing, subdivision, and tessellation changes trigger remesh
	bool advancedParamsChanged = false;
	try {
		auto& config = RenderingToolkitAPI::getConfig();
		auto& smoothingSettings = config.getSmoothingSettings();
		auto& subdivisionSettings = config.getSubdivisionSettings();
		
		// Check if advanced parameters have actually changed from last known values
		// Compare current values with stored instance values to detect real changes
		bool smoothingChanged = (smoothingSettings.enabled != m_lastSmoothingEnabled ||
			smoothingSettings.iterations != m_lastSmoothingIterations ||
			smoothingSettings.creaseAngle != m_lastSmoothingCreaseAngle);
		
		// Check if subdivision parameters changed
		bool subdivisionChanged = (subdivisionSettings.enabled != m_lastSubdivisionEnabled ||
			subdivisionSettings.levels != m_lastSubdivisionLevel);
		
		// Check custom parameters by comparing with stored values
		double currentSmoothingStrength = std::stod(config.getParameter("smoothing_strength", "0.5"));
		int currentTessellationMethod = std::stoi(config.getParameter("tessellation_method", "0"));
		int currentTessellationQuality = std::stoi(config.getParameter("tessellation_quality", "2"));
		double currentFeaturePreservation = std::stod(config.getParameter("feature_preservation", "0.5"));
		bool currentAdaptiveMeshing = (config.getParameter("adaptive_meshing", "false") == "true");
		bool currentParallelProcessing = (config.getParameter("parallel_processing", "true") == "true");
		
		bool customParamsChanged = (currentSmoothingStrength != m_lastSmoothingStrength ||
			currentTessellationMethod != m_lastTessellationMethod ||
			currentTessellationQuality != m_lastTessellationQuality ||
			currentFeaturePreservation != m_lastFeaturePreservation ||
			currentAdaptiveMeshing != m_lastAdaptiveMeshing ||
			currentParallelProcessing != m_lastParallelProcessing);
		
		// Only regenerate if parameters actually changed
		advancedParamsChanged = (smoothingChanged || subdivisionChanged || customParamsChanged);
	} catch (const std::exception& e) {
		// If we can't access config, assume no advanced changes
		LOG_ERR_S("Exception accessing RenderingToolkitAPI config: " + std::string(e.what()));
		advancedParamsChanged = false;
	} catch (...) {
		// If we can't access config, assume no advanced changes
		LOG_ERR_S("Unknown exception accessing RenderingToolkitAPI config");
		advancedParamsChanged = false;
	}

	if (m_meshRegenerationNeeded || paramsChanged || advancedParamsChanged) {
		try {
			// Clear old face mapping since mesh parameters have changed
			if (paramsChanged || advancedParamsChanged) {
				m_faceIndexMappings.clear();
				LOG_INF_S("Mesh parameters changed for " + m_name + " - face mapping will be rebuilt");
			}
			
			// Use the material-aware version to preserve imported geometry colors
			buildCoinRepresentation(params,
				m_materialDiffuseColor, m_materialAmbientColor,
				m_materialSpecularColor, m_materialEmissiveColor,
				m_materialShininess, m_transparency);
			m_lastMeshParams = params;
			m_meshRegenerationNeeded = false;
		}
		catch (const Standard_ConstructionError& e) {
			LOG_ERR_S("Standard_ConstructionError in buildCoinRepresentation for " + m_name + ": " + std::string(e.GetMessageString()));
			m_meshRegenerationNeeded = false; // Don't retry to avoid infinite loop
		}
		catch (const std::exception& e) {
			LOG_ERR_S("Exception in buildCoinRepresentation for " + m_name + ": " + std::string(e.what()));
			m_meshRegenerationNeeded = false; // Don't retry to avoid infinite loop
		}
		catch (...) {
			LOG_ERR_S("Unknown exception in buildCoinRepresentation for " + m_name);
			m_meshRegenerationNeeded = false; // Don't retry to avoid infinite loop
		}
	}
}

void OCCGeometry::buildCoinRepresentation(const MeshParameters& params)
{
	auto buildStartTime = std::chrono::high_resolution_clock::now();

	if (m_coinNode) {
		m_coinNode->removeAllChildren();
	}
	else {
		m_coinNode = new SoSeparator;
		m_coinNode->ref();
	}

	// Clean up any existing texture nodes to prevent memory issues
	// This is especially important when switching between different textures
	if (m_coinNode) {
		for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
			SoNode* child = m_coinNode->getChild(i);
			if (child && (child->isOfType(SoTexture2::getClassTypeId()) ||
				child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
				m_coinNode->removeChild(i);
			}
		}
	}

	// Transform setup
	m_coinTransform = new SoTransform;
	m_coinTransform->translation.setValue(
		static_cast<float>(m_position.X()),
		static_cast<float>(m_position.Y()),
		static_cast<float>(m_position.Z())
	);

	if (m_rotationAngle != 0.0) {
		SbVec3f axis(
			static_cast<float>(m_rotationAxis.X()),
			static_cast<float>(m_rotationAxis.Y()),
			static_cast<float>(m_rotationAxis.Z())
		);
		m_coinTransform->rotation.setValue(axis, static_cast<float>(m_rotationAngle));
	}

	m_coinTransform->scaleFactor.setValue(
		static_cast<float>(m_scale),
		static_cast<float>(m_scale),
		static_cast<float>(m_scale)
	);
	m_coinNode->addChild(m_coinTransform);

	// Shape hints - adjust for shell models
	SoShapeHints* hints = new SoShapeHints;
	hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
	
	// Check if this is a shell model requiring different shape hints
	bool isShellModel = (m_shape.ShapeType() == TopAbs_SHELL) || !m_cullFace;
	if (isShellModel) {
		// For shell models, use UNKNOWN_SHAPE_TYPE to allow better double-sided rendering
		hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
		hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
	} else {
		// For solid models, use standard solid hints
		hints->shapeType = SoShapeHints::SOLID;
		hints->faceType = SoShapeHints::CONVEX;
	}
	
	m_coinNode->addChild(hints);

	// Draw style (wireframe vs filled)
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->style = m_wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
	drawStyle->lineWidth = m_wireframeMode ? 1.0f : 0.0f;
	m_coinNode->addChild(drawStyle);

	// Material
	SoMaterial* material = new SoMaterial;
	if (m_wireframeMode) {
		material->diffuseColor.setValue(0.0f, 0.0f, 1.0f);
		material->transparency.setValue(static_cast<float>(m_transparency));
	}
	else {
		// Convert Quantity_Color to 0-1 range for Coin3D
		Standard_Real r, g, b;
		
		// Enhanced ambient color for better lighting consistency
		// Increase ambient intensity to reduce dependency on normal direction
		m_materialAmbientColor.Values(r, g, b, Quantity_TOC_RGB);
		material->ambientColor.setValue(
			static_cast<float>(r * 1.5),  // Increase ambient by 50%
			static_cast<float>(g * 1.5),
			static_cast<float>(b * 1.5)
		);

		// Slightly reduce diffuse to balance with increased ambient
		m_materialDiffuseColor.Values(r, g, b, Quantity_TOC_RGB);
		material->diffuseColor.setValue(
			static_cast<float>(r * 0.8),  // Reduce diffuse by 20%
			static_cast<float>(g * 0.8),
			static_cast<float>(b * 0.8)
		);

		// Keep specular unchanged for highlights
		m_materialSpecularColor.Values(r, g, b, Quantity_TOC_RGB);
		material->specularColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));

		material->shininess.setValue(static_cast<float>(m_materialShininess / 100.0));
		// Hide faces when requested by pushing transparency to 1.0
		double appliedTransparency = m_facesVisible ? m_transparency : 1.0;
		material->transparency.setValue(static_cast<float>(appliedTransparency));

		// Add emissive color for better lighting response
		material->emissiveColor.setValue(0.0f, 0.0f, 0.0f);

	}
	m_coinNode->addChild(material);

	// Texture support
	if (m_textureEnabled && !m_textureImagePath.empty()) {
		// Validate texture file exists
		std::ifstream fileCheck(m_textureImagePath);
		if (!fileCheck.good()) {
			LOG_WRN_S("Texture file not found or invalid: " + m_textureImagePath + " for " + m_name);
			m_textureEnabled = false; // Disable texture if file is invalid
		}
		else {
			fileCheck.close();

			try {
				// Create texture node
				SoTexture2* texture = new SoTexture2;

				// Load texture image - use safer approach
				texture->filename.setValue(m_textureImagePath.c_str());

				// Set texture mode based on configuration
				switch (m_textureMode) {
				case RenderingConfig::TextureMode::Replace:
					texture->model.setValue(SoTexture2::DECAL);
					break;
				case RenderingConfig::TextureMode::Modulate:
					texture->model.setValue(SoTexture2::MODULATE);
					break;
				case RenderingConfig::TextureMode::Blend:
					texture->model.setValue(SoTexture2::BLEND);
					break;
				case RenderingConfig::TextureMode::Decal:
				default:
					texture->model.setValue(SoTexture2::DECAL);
					break;
				}

				// Set texture intensity
				Standard_Real tr, tg, tb;
				m_textureColor.Values(tr, tg, tb, Quantity_TOC_RGB);
				texture->blendColor.setValue(static_cast<float>(tr),
					static_cast<float>(tg),
					static_cast<float>(tb));

				// Add texture node first
				m_coinNode->addChild(texture);

				// Set texture transformation - use safer approach
				SoTextureCoordinate2* texCoord = new SoTextureCoordinate2;

				// Use default texture coordinates (Coin3D will handle this automatically)
				// This avoids potential memory access issues with custom coordinate arrays
				// Coin3D will generate appropriate texture coordinates based on the geometry

				// Add texture coordinate node
				m_coinNode->addChild(texCoord);

				LOG_INF_S("Texture applied to " + m_name + " - path: " + m_textureImagePath +
					" mode: " + std::to_string(static_cast<int>(m_textureMode)));
			}
			catch (const std::exception& e) {
				LOG_ERR_S("Exception while loading texture for " + m_name + ": " + std::string(e.what()));
				m_textureEnabled = false; // Disable texture on error
			}
			catch (...) {
				LOG_ERR_S("Unknown exception while loading texture for " + m_name);
				m_textureEnabled = false; // Disable texture on error
			}
		}
	}

	// Blend and depth settings
	if (m_blendMode != RenderingConfig::BlendMode::None && m_transparency > 0.0) {
		// Apply blend settings through material transparency
		material->transparency.setValue(static_cast<float>(m_transparency));

		// Set material blend mode through hints
		SoShapeHints* blendHints = new SoShapeHints;
		blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
		blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
		m_coinNode->addChild(blendHints);

		LOG_INF_S("Applied blend mode " + std::to_string(static_cast<int>(m_blendMode)) +
			" with transparency " + std::to_string(m_transparency) + " for " + m_name);
	}

	// Face culling settings
	if (m_cullFace) {
		SoShapeHints* cullHints = new SoShapeHints;
		cullHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
		cullHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
		m_coinNode->addChild(cullHints);
	}

	// Edge visualization is now handled by EdgeComponent
	// Old edge drawing code removed to avoid conflicts with new edge system

	// Add shape to scene (simplified)
	if (!m_shape.IsNull()) {
		if (m_wireframeMode) {
			// In wireframe mode, create wireframe representation directly
			createWireframeRepresentation(params);
		}
		else {
			// Use rendering toolkit to create scene node for solid/filled mode
			auto& manager = RenderingToolkitAPI::getManager();
			auto backend = manager.getRenderBackend("Coin3D");
			if (backend) {
				// Use the material-aware version to preserve custom material settings
				auto sceneNode = backend->createSceneNode(m_shape, params, m_selected,
					m_materialDiffuseColor, m_materialAmbientColor,
					m_materialSpecularColor, m_materialEmissiveColor,
					m_materialShininess, m_transparency);
				if (sceneNode && m_facesVisible) {
					SoSeparator* meshNode = sceneNode.get();
					meshNode->ref(); // Take ownership
					m_coinNode->addChild(meshNode);
				}
			}
		}
	}

	// Set visibility
	if (m_coinNode) {
		m_coinNode->renderCulling = m_visible ? SoSeparator::OFF : SoSeparator::ON;
	}

	m_coinNeedsUpdate = false;

	auto buildEndTime = std::chrono::high_resolution_clock::now();
	auto buildDuration = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTime - buildStartTime);

	// Only log detailed breakdown for debugging or when needed
	if (buildDuration.count() > 100) { // Only log if build takes more than 100ms
		LOG_INF_S("=== COIN3D BUILD BREAKDOWN ===");
		LOG_INF_S("Geometry: " + m_name);
		LOG_INF_S("TOTAL BUILD TIME: " + std::to_string(buildDuration.count()) + "ms");
		LOG_INF_S("==============================");
	}

	// Generate edge nodes for EdgeComponent on demand when any edge type is toggled or if overlay edges are requested
	bool anyEdgeDisplayRequested = false;
	if (edgeComponent) {
		// If edgeComponent already exists, reuse; else create when needed
		const EdgeDisplayFlags& flags = edgeComponent->edgeFlags;
		anyEdgeDisplayRequested = flags.showOriginalEdges || flags.showFeatureEdges ||
			flags.showMeshEdges || flags.showHighlightEdges ||
			flags.showNormalLines || flags.showFaceNormalLines;
	}
	const EdgeSettingsConfig& edgeCfg = EdgeSettingsConfig::getInstance();
	anyEdgeDisplayRequested = anyEdgeDisplayRequested || edgeCfg.getGlobalSettings().showEdges ||
		edgeCfg.getSelectedSettings().showEdges ||
		edgeCfg.getHoverSettings().showEdges;

	if (edgeComponent && anyEdgeDisplayRequested) {

		// Generate nodes only for requested types to avoid unnecessary overlays
		if (edgeComponent->edgeFlags.showOriginalEdges) {
			edgeComponent->extractOriginalEdges(m_shape);
		}
		if (edgeComponent->edgeFlags.showFeatureEdges) {
			// Start with permissive parameters for responsiveness
			edgeComponent->extractFeatureEdges(m_shape, 15.0, 0.005, false, false);
		}

		// Generate mesh edges and normal lines
		auto& manager = RenderingToolkitAPI::getManager();
		auto processor = manager.getGeometryProcessor("OpenCASCADE");
		if (processor) {
			TriangleMesh mesh = processor->convertToMesh(m_shape, params);

			// Generate mesh edges when requested
			if (edgeComponent->edgeFlags.showMeshEdges) {
				edgeComponent->extractMeshEdges(mesh);
			}
			// Generate normal/face-normal lines when requested
			if (edgeComponent->edgeFlags.showNormalLines) {
				edgeComponent->generateNormalLineNode(mesh, 0.5); // Use default length for now
			}
			if (edgeComponent->edgeFlags.showFaceNormalLines) {
				edgeComponent->generateFaceNormalLineNode(mesh, 0.5); // Use default length for now
			}
		}
		else {
			LOG_WRN_S("OpenCASCADE processor not found for geometry: " + m_name);
		}

		// Generate highlight edge node only if requested
		if (edgeComponent->edgeFlags.showHighlightEdges) {
			edgeComponent->generateHighlightEdgeNode();
		}
	}
	else {
		if (!edgeComponent) {
			LOG_WRN_S("EdgeComponent is null for geometry: " + m_name);
		}
		else {
			LOG_INF_S("Skipping edge generation for geometry (edge display disabled): " + m_name);
		}
	}

	// Build face index mapping if not already built to enable face picking for all geometries
	if (!hasFaceIndexMapping()) {
		LOG_INF_S("Building face index mapping for geometry: " + m_name);
		buildFaceIndexMapping(params);
	}

	// Update tracking variables for next comparison
	m_lastMeshParams = params;

	// Update advanced parameter tracking
	auto& config = RenderingToolkitAPI::getConfig();
	auto& smoothingSettings = config.getSmoothingSettings();
	auto& subdivisionSettings = config.getSubdivisionSettings();
	auto& edgeSettings = config.getEdgeSettings();

	m_lastSmoothingEnabled = smoothingSettings.enabled;
	m_lastSmoothingIterations = smoothingSettings.iterations;
	m_lastSmoothingCreaseAngle = smoothingSettings.creaseAngle;
	m_lastSubdivisionEnabled = subdivisionSettings.enabled;
	m_lastSubdivisionLevel = subdivisionSettings.levels;
	m_lastSubdivisionCreaseAngle = edgeSettings.featureEdgeAngle;

	// Update custom parameter tracking with safer conversion
	try {
		m_lastSmoothingStrength = std::stod(config.getParameter("smoothing_strength", "0.5"));
		m_lastTessellationMethod = std::stoi(config.getParameter("tessellation_method", "0"));
		m_lastTessellationQuality = std::stoi(config.getParameter("tessellation_quality", "2"));
		m_lastFeaturePreservation = std::stod(config.getParameter("feature_preservation", "0.5"));
		m_lastAdaptiveMeshing = (config.getParameter("adaptive_meshing", "false") == "true");
		m_lastParallelProcessing = (config.getParameter("parallel_processing", "true") == "true");
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Error parsing custom parameters for " + m_name + ": " + std::string(e.what()));
		// Keep default values if parsing fails
	}
}

void OCCGeometry::addLODLevel(double distance, double deflection) {
    m_lodLevels.push_back({distance, deflection});
    // Sort by distance
    std::sort(m_lodLevels.begin(), m_lodLevels.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
}

int OCCGeometry::getLODLevel(double viewDistance) const {
    if (!m_enableLOD || m_lodLevels.empty()) {
        return 0;
    }
    
    for (size_t i = 0; i < m_lodLevels.size(); ++i) {
        if (viewDistance <= m_lodLevels[i].first) {
            return static_cast<int>(i);
        }
    }
    
    return static_cast<int>(m_lodLevels.size() - 1);
}

void OCCGeometry::releaseTemporaryData() {
    // Release any cached tessellation data that can be regenerated
    if (m_coinNode) {
        // Keep the node but release large data structures if possible
        // This would require more detailed management of Coin3D data
    }
}

void OCCGeometry::optimizeMemory() {
    // Optimize internal data structures
    // This could include:
    // - Compacting vectors
    // - Releasing unused memory
    // - Optimizing OpenCASCADE shape representations
    
    // For now, just ensure vectors are sized appropriately
    m_lodLevels.shrink_to_fit();
}

void OCCGeometry::createWireframeRepresentation(const MeshParameters& params)
{
	if (m_shape.IsNull()) {
		return;
	}

	// Convert shape to mesh for wireframe generation
	auto& manager = RenderingToolkitAPI::getManager();
	auto processor = manager.getGeometryProcessor("OpenCASCADE");
	if (!processor) {
		LOG_WRN_S("OpenCASCADE processor not found for wireframe generation");
		return;
	}

	TriangleMesh mesh = processor->convertToMesh(m_shape, params);
	if (mesh.isEmpty()) {
		LOG_WRN_S("Empty mesh generated for wireframe representation");
		return;
	}

	// Create coordinate node
	SoCoordinate3* coords = new SoCoordinate3;
	std::vector<float> vertices;
	for (const auto& vertex : mesh.vertices) {
		vertices.push_back(static_cast<float>(vertex.X()));
		vertices.push_back(static_cast<float>(vertex.Y()));
		vertices.push_back(static_cast<float>(vertex.Z()));
	}
	coords->point.setValues(0, static_cast<int>(mesh.vertices.size()),
		reinterpret_cast<const SbVec3f*>(vertices.data()));
	m_coinNode->addChild(coords);

	// Create wireframe line set
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	std::vector<int32_t> indices;

	// Create wireframe from triangle edges
	for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
		int v0 = mesh.triangles[i];
		int v1 = mesh.triangles[i + 1];
		int v2 = mesh.triangles[i + 2];

		// Add triangle edges
		indices.push_back(v0);
		indices.push_back(v1);
		indices.push_back(SO_END_LINE_INDEX);

		indices.push_back(v1);
		indices.push_back(v2);
		indices.push_back(SO_END_LINE_INDEX);

		indices.push_back(v2);
		indices.push_back(v0);
		indices.push_back(SO_END_LINE_INDEX);
	}

	lineSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
	m_coinNode->addChild(lineSet);

	LOG_INF_S("Created wireframe representation for " + m_name +
		" with " + std::to_string(mesh.vertices.size()) + " vertices and " +
		std::to_string(mesh.triangles.size() / 3) + " triangles");
}

// All primitive classes (OCCBox, OCCCylinder, etc.) call setShape(),
// which sets the m_coinNeedsUpdate flag. The representation will be
// built on the next call to getCoinNode().

// Primitive class implementations moved to OCCGeometryPrimitives.cpp

void OCCGeometry::setSmoothNormals(bool enabled) {
	m_smoothNormals = enabled;
}

void OCCGeometry::setWireframeWidth(double width) {
	m_wireframeWidth = width;
}

void OCCGeometry::setPointSize(double size) {
	m_pointSize = size;
}

// Display methods implementation
void OCCGeometry::setDisplayMode(RenderingConfig::DisplayMode mode) {
	m_displayMode = mode;
}

void OCCGeometry::setShowEdges(bool enabled) {
	m_showEdges = enabled;
	m_coinNeedsUpdate = true;
}
void OCCGeometry::setShowWireframe(bool enabled) {
	m_showWireframe = enabled;
	m_coinNeedsUpdate = true;
}

void OCCGeometry::setShowVertices(bool enabled) {
	m_showVertices = enabled;

}

void OCCGeometry::setEdgeWidth(double width) {
	m_edgeWidth = width;

}

void OCCGeometry::setVertexSize(double size) {
	m_vertexSize = size;

}

void OCCGeometry::setEdgeColor(const Quantity_Color& color) {
	m_edgeColor = color;

}

void OCCGeometry::setVertexColor(const Quantity_Color& color) {
	m_vertexColor = color;

}

// Quality methods implementation
void OCCGeometry::setRenderingQuality(RenderingConfig::RenderingQuality quality) {
	m_renderingQuality = quality;

}

void OCCGeometry::setTessellationLevel(int level) {
	m_tessellationLevel = level;

}

void OCCGeometry::setAntiAliasingSamples(int samples) {
	m_antiAliasingSamples = samples;

}

void OCCGeometry::setEnableLOD(bool enabled) {
	m_enableLOD = enabled;

}

void OCCGeometry::setLODDistance(double distance) {
	m_lodDistance = distance;

}

// Shadow methods implementation
void OCCGeometry::setShadowMode(RenderingConfig::ShadowMode mode) {
	m_shadowMode = mode;

}

void OCCGeometry::setShadowIntensity(double intensity) {
	m_shadowIntensity = intensity;

}

void OCCGeometry::setShadowSoftness(double softness) {
	m_shadowSoftness = softness;

}

void OCCGeometry::setShadowMapSize(int size) {
	m_shadowMapSize = size;

}

void OCCGeometry::setShadowBias(double bias) {
	m_shadowBias = bias;

}

// Lighting model methods implementation
void OCCGeometry::setLightingModel(RenderingConfig::LightingModel model) {
	m_lightingModel = model;

}

void OCCGeometry::setRoughness(double roughness) {
	m_roughness = roughness;

}

void OCCGeometry::setMetallic(double metallic) {
	m_metallic = metallic;

}

void OCCGeometry::setFresnel(double fresnel) {
	m_fresnel = fresnel;

}

void OCCGeometry::setSubsurfaceScattering(double scattering) {
	m_subsurfaceScattering = scattering;

}

void OCCGeometry::updateFromRenderingConfig()
{
	// Load current settings from configuration
	RenderingConfig& config = RenderingConfig::getInstance();
	const auto& materialSettings = config.getMaterialSettings();
	const auto& textureSettings = config.getTextureSettings();
	const auto& blendSettings = config.getBlendSettings();

	// Only update material settings if they haven't been explicitly set for this geometry
	// This prevents global config from overriding individual geometry material settings
	if (!m_materialExplicitlySet) {
		m_color = materialSettings.diffuseColor;
		m_transparency = materialSettings.transparency;
		m_materialAmbientColor = materialSettings.ambientColor;
		m_materialDiffuseColor = materialSettings.diffuseColor;
		m_materialSpecularColor = materialSettings.specularColor;
		m_materialShininess = materialSettings.shininess;
	}

	// Update texture settings
	m_textureColor = textureSettings.color;
	m_textureIntensity = textureSettings.intensity;
	m_textureEnabled = textureSettings.enabled;
	m_textureImagePath = textureSettings.imagePath;
	m_textureMode = textureSettings.textureMode;

	// Update blend settings
	m_blendMode = blendSettings.blendMode;
	m_depthTest = blendSettings.depthTest;
	m_depthWrite = blendSettings.depthWrite;
	m_cullFace = blendSettings.cullFace;
	m_alphaThreshold = blendSettings.alphaThreshold;

	// Force rebuild of Coin3D representation
	m_coinNeedsUpdate = true;

	// Actually rebuild the Coin3D representation to apply changes immediately
	if (m_coinNode) {
		// Force Coin3D to invalidate its cache
		m_coinNode->touch();

		// Use the material-aware version to preserve custom material settings
		MeshParameters meshParams;
		buildCoinRepresentation(meshParams,
			m_materialDiffuseColor, m_materialAmbientColor,
			m_materialSpecularColor, m_materialEmissiveColor,
			m_materialShininess, m_transparency);

		// Force the scene graph to be marked as needing update
		// Note: Coin3D nodes don't have getParent() method, so we just touch the current node
		m_coinNode->touch();
	}
}

void OCCGeometry::updateMaterialForLighting()
{
	// This method is called when lighting changes to adjust material properties
	// for better lighting response without changing the base material settings

	if (!m_coinNode) {
		LOG_WRN_S("Cannot update material for lighting: Coin3D node not available for " + m_name);
		return;
	}

	// Find the material node in the Coin3D representation
	for (int i = 0; i < m_coinNode->getNumChildren(); ++i) {
		SoNode* child = m_coinNode->getChild(i);
		if (child && child->isOfType(SoMaterial::getClassTypeId())) {
			SoMaterial* material = static_cast<SoMaterial*>(child);

			// Adjust material properties for better lighting response
			if (!m_wireframeMode) {
				// Enhanced ambient component for better lighting consistency
				// Increase ambient intensity to reduce dependency on normal direction
				Standard_Real r, g, b;
				m_materialAmbientColor.Values(r, g, b, Quantity_TOC_RGB);
				material->ambientColor.setValue(static_cast<float>(r * 1.5),
					static_cast<float>(g * 1.5),
					static_cast<float>(b * 1.5));

				// Slightly reduce diffuse to balance with increased ambient
				m_materialDiffuseColor.Values(r, g, b, Quantity_TOC_RGB);
				material->diffuseColor.setValue(static_cast<float>(r * 0.8),
					static_cast<float>(g * 0.8),
					static_cast<float>(b * 0.8));

				// Enhance specular component for better lighting highlights
				m_materialSpecularColor.Values(r, g, b, Quantity_TOC_RGB);
				material->specularColor.setValue(static_cast<float>(r * 1.1),
					static_cast<float>(g * 1.1),
					static_cast<float>(b * 1.1));

				// Adjust shininess for better lighting response
				material->shininess.setValue(static_cast<float>(m_materialShininess / 100.0));

			}
			break;
		}
	}

	// Force Coin3D to update
	m_coinNode->touch();
}

void OCCGeometry::forceTextureUpdate()
{
	if (m_textureEnabled && !m_textureImagePath.empty()) {
		m_coinNeedsUpdate = true;

		if (m_coinNode) {
			// Force rebuild of Coin3D representation to apply texture changes
			buildCoinRepresentation();
			m_coinNode->touch();
			LOG_INF_S("Forced texture update for " + m_name + " - path: " + m_textureImagePath);
		}
	}
}

void OCCGeometry::setFaceDisplay(bool enable) {
	// Removed setShadingMode call - functionality not needed
	if (m_coinNode) {
		buildCoinRepresentation();
		m_coinNode->touch();
	}
}

void OCCGeometry::setFacesVisible(bool visible) {
	if (m_facesVisible == visible) return;
	m_facesVisible = visible;
	m_coinNeedsUpdate = true;
	if (m_coinNode) {
		buildCoinRepresentation();
		m_coinNode->touch();
	}
}
void OCCGeometry::setWireframeOverlay(bool enable) {
	setWireframeMode(enable);
	if (m_coinNode) {
		buildCoinRepresentation();
		m_coinNode->touch();
	}
}
bool OCCGeometry::hasOriginalEdges() const {
	return m_showEdges;
}
void OCCGeometry::setEdgeDisplay(bool enable) {
	setShowEdges(enable);
	if (m_coinNode) {
		buildCoinRepresentation();
		m_coinNode->touch();
	}
}
void OCCGeometry::setFeatureEdgeDisplay(bool enable) {
	if (edgeComponent) edgeComponent->setEdgeDisplayType(EdgeType::Feature, enable);
	if (m_coinNode) {
		buildCoinRepresentation();
		m_coinNode->touch();
	}
}
void OCCGeometry::setNormalDisplay(bool enable) {
	setSmoothNormals(enable);
	if (m_coinNode) {
		buildCoinRepresentation();
		m_coinNode->touch();
	}
}

void OCCGeometry::setEdgeDisplayType(EdgeType type, bool show) {
	if (edgeComponent) edgeComponent->setEdgeDisplayType(type, show);
}

bool OCCGeometry::isEdgeDisplayTypeEnabled(EdgeType type) const {
	return edgeComponent ? edgeComponent->isEdgeDisplayTypeEnabled(type) : false;
}

void OCCGeometry::buildCoinRepresentation(const MeshParameters& params,
	const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
	const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
	double shininess, double transparency)
{
	auto buildStartTime = std::chrono::high_resolution_clock::now();

	if (m_coinNode) {
		m_coinNode->removeAllChildren();
	}
	else {
		m_coinNode = new SoSeparator;
		m_coinNode->ref();
	}

	// Clean up any existing texture nodes to prevent memory issues
	if (m_coinNode) {
		for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
			SoNode* child = m_coinNode->getChild(i);
			if (child && (child->isOfType(SoTexture2::getClassTypeId()) ||
				child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
				m_coinNode->removeChild(i);
			}
		}
	}

	// Transform setup
	m_coinTransform = new SoTransform;
	m_coinTransform->translation.setValue(
		static_cast<float>(m_position.X()),
		static_cast<float>(m_position.Y()),
		static_cast<float>(m_position.Z())
	);

	if (m_rotationAngle != 0.0) {
		SbVec3f axis(
			static_cast<float>(m_rotationAxis.X()),
			static_cast<float>(m_rotationAxis.Y()),
			static_cast<float>(m_rotationAxis.Z())
		);
		m_coinTransform->rotation.setValue(axis, static_cast<float>(m_rotationAngle));
	}

	m_coinTransform->scaleFactor.setValue(
		static_cast<float>(m_scale),
		static_cast<float>(m_scale),
		static_cast<float>(m_scale)
	);
	m_coinNode->addChild(m_coinTransform);

	// Shape hints
	SoShapeHints* hints = new SoShapeHints;
	hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
	hints->shapeType = SoShapeHints::SOLID;
	hints->faceType = SoShapeHints::CONVEX;
	m_coinNode->addChild(hints);

	// Draw style
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->style = m_wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
	drawStyle->lineWidth = m_wireframeMode ? 1.0f : 0.0f;
	m_coinNode->addChild(drawStyle);

	// Build face index mapping and create scene node together to avoid duplicate mesh generation
	if (!hasFaceIndexMapping()) {
		// Use the processor to generate mesh with face mapping
		GeometryProcessor* baseProcessor = RenderingToolkitAPI::getManager().getGeometryProcessor();
		auto* processor = dynamic_cast<OpenCASCADEProcessor*>(baseProcessor);
		if (processor) {
			std::vector<std::pair<int, std::vector<int>>> faceMappings;
			TriangleMesh meshWithMapping = processor->convertToMeshWithFaceMapping(m_shape, params, faceMappings);
			
			if (!faceMappings.empty()) {
				// Build face index mappings and reverse map
				m_faceIndexMappings.clear();
				m_faceIndexMappings.reserve(faceMappings.size());
				m_triangleToFaceMap.clear();
				m_triangleToFaceMap.reserve(meshWithMapping.getTriangleCount());
				
				for (const auto& [faceId, triangleIndices] : faceMappings) {
					FaceIndexMapping mapping(faceId);
					mapping.triangleIndices = triangleIndices;
					m_faceIndexMappings.push_back(mapping);
					
					// Build reverse mapping for O(1) lookup
					for (int triIdx : triangleIndices) {
						m_triangleToFaceMap[triIdx] = faceId;
					}
				}
				
				LOG_INF_S("Built face index mapping with " + std::to_string(m_faceIndexMappings.size()) +
					" face mappings, total triangles: " + std::to_string(meshWithMapping.getTriangleCount()));
			}
		}
	}
	
	// Use RenderingToolkitAPI for rendering operations
	auto& manager = RenderingToolkitAPI::getManager();
	auto backend = manager.getRenderBackend("Coin3D");
	if (backend) {
		// Use the material-aware version to preserve custom material settings
		auto sceneNode = backend->createSceneNode(m_shape, params, m_selected,
			diffuseColor, ambientColor, specularColor, emissiveColor, shininess, transparency);
		if (sceneNode) {
			SoSeparator* meshNode = sceneNode.get();
			meshNode->ref(); // Take ownership
			m_coinNode->addChild(meshNode);
		}
	}
	else {
		LOG_ERR_S("Coin3D backend not available for " + m_name);
	}

	auto buildEndTime = std::chrono::high_resolution_clock::now();
	auto buildDuration = std::chrono::duration_cast<std::chrono::microseconds>(buildEndTime - buildStartTime);

}

void OCCGeometry::updateEdgeDisplay() {
	if (edgeComponent) edgeComponent->updateEdgeDisplay(getCoinNode());
}

void OCCGeometry::applyAdvancedParameters(const AdvancedGeometryParameters& params)
{
	LOG_INF_S("Applying advanced parameters to geometry: " + m_name);

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

	// Apply edge display types (silhouette disabled)
	if (edgeComponent) {
		edgeComponent->setEdgeDisplayType(EdgeType::Original, params.showOriginalEdges);
		edgeComponent->setEdgeDisplayType(EdgeType::Feature, params.showFeatureEdges);
		edgeComponent->setEdgeDisplayType(EdgeType::Mesh, params.showMeshEdges);
	}

	// Apply subdivision settings
	m_subdivisionEnabled = params.subdivisionEnabled;
	m_subdivisionLevels = params.subdivisionLevels;

	// Apply blend settings
	setBlendMode(params.blendMode);
	setDepthTest(params.depthTest);
	setCullFace(params.backfaceCulling);

	// Force rebuild of Coin3D representation
	m_coinNeedsUpdate = true;

	if (m_coinNode) {
		buildCoinRepresentation();
		m_coinNode->touch();
		LOG_INF_S("Rebuilt Coin3D representation for geometry '" + m_name + "' with advanced parameters");
	}

	LOG_INF_S("Advanced parameters applied to geometry '" + m_name + "':");
	LOG_INF_S("  - Material diffuse color: " + std::to_string(params.materialDiffuseColor.Red()) + "," +
		std::to_string(params.materialDiffuseColor.Green()) + "," + std::to_string(params.materialDiffuseColor.Blue()));
	LOG_INF_S("  - Transparency: " + std::to_string(params.materialTransparency));
	LOG_INF_S("  - Texture enabled: " + std::string(params.textureEnabled ? "true" : "false"));
	LOG_INF_S("  - Show edges: " + std::string(params.showEdges ? "true" : "false"));
	LOG_INF_S("  - Subdivision enabled: " + std::string(params.subdivisionEnabled ? "true" : "false"));
}

// ===== FACE INDEX MAPPING IMPLEMENTATION =====

// Build face index mapping during mesh generation
void OCCGeometry::buildFaceIndexMapping(const MeshParameters& params) {
	try {
		if (m_shape.IsNull()) {
			LOG_WRN_S("Cannot build face index mapping for null shape");
			return;
		}

		m_faceIndexMappings.clear();

		// Extract all faces from the shape
		std::vector<TopoDS_Face> faces;
		for (TopExp_Explorer exp(m_shape, TopAbs_FACE); exp.More(); exp.Next()) {
			faces.push_back(TopoDS::Face(exp.Current()));
		}

		if (faces.empty()) {
			LOG_WRN_S("No faces found in shape for index mapping");
			return;
		}

		LOG_INF_S("Building face index mapping for " + std::to_string(faces.size()) + " faces");

		// Use the new processor method with face mapping support
		GeometryProcessor* baseProcessor = RenderingToolkitAPI::getManager().getGeometryProcessor();
		auto* processor = dynamic_cast<OpenCASCADEProcessor*>(baseProcessor);
		if (!processor) {
			LOG_WRN_S("OpenCASCADEProcessor not available for face mapping");
			return;
		}

		std::vector<std::pair<int, std::vector<int>>> faceMappings;
		TriangleMesh mesh = processor->convertToMeshWithFaceMapping(m_shape, params, faceMappings);

		if (mesh.triangles.empty() || faceMappings.empty()) {
			LOG_WRN_S("No mesh triangles or face mappings found");
			return;
		}

		// Convert to FaceIndexMapping format and build reverse map
		m_faceIndexMappings.reserve(faceMappings.size());
		m_triangleToFaceMap.clear();
		m_triangleToFaceMap.reserve(mesh.getTriangleCount());
		
		for (const auto& [faceId, triangleIndices] : faceMappings) {
			FaceIndexMapping mapping(faceId);
			mapping.triangleIndices = triangleIndices;
			m_faceIndexMappings.push_back(mapping);
			
			// Build reverse mapping for O(1) lookup
			for (int triIdx : triangleIndices) {
				m_triangleToFaceMap[triIdx] = faceId;
			}
		}

		LOG_INF_S("Built face index mapping with " + std::to_string(m_faceIndexMappings.size()) +
			" face mappings, total triangles: " + std::to_string(mesh.getTriangleCount()));

	} catch (const std::exception& e) {
		LOG_ERR_S("Failed to build face index mapping: " + std::string(e.what()));
		m_faceIndexMappings.clear();
	}
}

// Get geometry face ID for a given triangle index
int OCCGeometry::getGeometryFaceIdForTriangle(int triangleIndex) const {
	if (!hasFaceIndexMapping()) {
		return -1;
	}

	// Use optimized reverse mapping for O(1) lookup
	auto it = m_triangleToFaceMap.find(triangleIndex);
	if (it != m_triangleToFaceMap.end()) {
		return it->second;
	}

	return -1; // Triangle not found in any face mapping
}

// Get all triangle indices for a given geometry face ID
std::vector<int> OCCGeometry::getTrianglesForGeometryFace(int geometryFaceId) const {
	if (!hasFaceIndexMapping()) {
		return {};
	}

	for (const auto& mapping : m_faceIndexMappings) {
		if (mapping.geometryFaceId == geometryFaceId) {
			return mapping.triangleIndices;
		}
	}

	return {}; // Face ID not found
}

