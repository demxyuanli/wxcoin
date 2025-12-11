#include "config/RenderingConfig.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

// Initialize static member
std::map<RenderingConfig::MaterialPreset, RenderingConfig::MaterialSettings> RenderingConfig::s_materialPresets;

RenderingConfig& RenderingConfig::getInstance()
{
	static RenderingConfig instance;
	return instance;
}

RenderingConfig::RenderingConfig()
	: m_autoSave(true)
{
	// Initialize material presets
	initializeMaterialPresets();

	// Load configuration from file on startup
	loadFromFile();
}

void RenderingConfig::initializeMaterialPresets()
{
	if (!s_materialPresets.empty()) {
		return; // Already initialized
	}

	// Glass material
	s_materialPresets[MaterialPreset::Glass] = MaterialSettings(
		Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB),    // ambient
		Quantity_Color(0.8, 0.9, 1.0, Quantity_TOC_RGB),    // diffuse (light blue)
		Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),    // specular (white)
		128.0,  // shininess (very shiny)
		0.7     // transparency (70% transparent)
	);

	// Metal material
	s_materialPresets[MaterialPreset::Metal] = MaterialSettings(
		Quantity_Color(0.2, 0.2, 0.2, Quantity_TOC_RGB),    // ambient
		Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB),    // diffuse (gray)
		Quantity_Color(0.9, 0.9, 0.9, Quantity_TOC_RGB),    // specular
		80.0,   // shininess
		0.0     // transparency (opaque)
	);

	// Plastic material
	s_materialPresets[MaterialPreset::Plastic] = MaterialSettings(
		Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB),    // ambient
		Quantity_Color(0.8, 0.2, 0.2, Quantity_TOC_RGB),    // diffuse (red)
		Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB),    // specular
		32.0,   // shininess
		0.0     // transparency
	);

	// Wood material
	s_materialPresets[MaterialPreset::Wood] = MaterialSettings(
		Quantity_Color(0.2, 0.1, 0.05, Quantity_TOC_RGB),   // ambient (dark brown)
		Quantity_Color(0.6, 0.4, 0.2, Quantity_TOC_RGB),    // diffuse (brown)
		Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB),    // specular
		16.0,   // shininess (low)
		0.0     // transparency
	);

	// Ceramic material
	s_materialPresets[MaterialPreset::Ceramic] = MaterialSettings(
		Quantity_Color(0.15, 0.15, 0.15, Quantity_TOC_RGB), // ambient
		Quantity_Color(0.9, 0.9, 0.85, Quantity_TOC_RGB),   // diffuse (off-white)
		Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB),    // specular
		64.0,   // shininess
		0.0     // transparency
	);

	// Rubber material
	s_materialPresets[MaterialPreset::Rubber] = MaterialSettings(
		Quantity_Color(0.05, 0.05, 0.05, Quantity_TOC_RGB), // ambient (very dark)
		Quantity_Color(0.2, 0.2, 0.2, Quantity_TOC_RGB),    // diffuse (dark gray)
		Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB),    // specular (very low)
		4.0,    // shininess (very low)
		0.0     // transparency
	);

	// Chrome material
	s_materialPresets[MaterialPreset::Chrome] = MaterialSettings(
		Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB),    // ambient
		Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB),    // diffuse
		Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),    // specular (bright white)
		128.0,  // shininess (very shiny)
		0.0     // transparency
	);

	// Gold material
	s_materialPresets[MaterialPreset::Gold] = MaterialSettings(
		Quantity_Color(0.24, 0.20, 0.07, Quantity_TOC_RGB), // ambient (dark gold)
		Quantity_Color(0.75, 0.61, 0.23, Quantity_TOC_RGB), // diffuse (gold)
		Quantity_Color(0.63, 0.56, 0.37, Quantity_TOC_RGB), // specular (golden)
		64.0,   // shininess
		0.0     // transparency
	);

	// Silver material
	s_materialPresets[MaterialPreset::Silver] = MaterialSettings(
		Quantity_Color(0.19, 0.19, 0.19, Quantity_TOC_RGB), // ambient
		Quantity_Color(0.51, 0.51, 0.51, Quantity_TOC_RGB), // diffuse
		Quantity_Color(0.51, 0.51, 0.51, Quantity_TOC_RGB), // specular
		64.0,   // shininess
		0.0     // transparency
	);

	// Copper material
	s_materialPresets[MaterialPreset::Copper] = MaterialSettings(
		Quantity_Color(0.19, 0.07, 0.02, Quantity_TOC_RGB), // ambient (dark copper)
		Quantity_Color(0.70, 0.27, 0.08, Quantity_TOC_RGB), // diffuse (copper)
		Quantity_Color(0.26, 0.14, 0.09, Quantity_TOC_RGB), // specular (copper)
		64.0,   // shininess
		0.0     // transparency
	);

	// Aluminum material
	s_materialPresets[MaterialPreset::Aluminum] = MaterialSettings(
		Quantity_Color(0.2, 0.2, 0.2, Quantity_TOC_RGB),    // ambient
		Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB),    // diffuse
		Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB),    // specular
		96.0,   // shininess
		0.0     // transparency
	);
}

std::vector<std::string> RenderingConfig::getAvailablePresets()
{
	return {
		"Custom",
		"Glass",
		"Metal",
		"Plastic",
		"Wood",
		"Ceramic",
		"Rubber",
		"Chrome",
		"Gold",
		"Silver",
		"Copper",
		"Aluminum"
	};
}

std::string RenderingConfig::getPresetName(MaterialPreset preset)
{
	switch (preset) {
	case MaterialPreset::Custom: return "Custom";
	case MaterialPreset::Glass: return "Glass";
	case MaterialPreset::Metal: return "Metal";
	case MaterialPreset::Plastic: return "Plastic";
	case MaterialPreset::Wood: return "Wood";
	case MaterialPreset::Ceramic: return "Ceramic";
	case MaterialPreset::Rubber: return "Rubber";
	case MaterialPreset::Chrome: return "Chrome";
	case MaterialPreset::Gold: return "Gold";
	case MaterialPreset::Silver: return "Silver";
	case MaterialPreset::Copper: return "Copper";
	case MaterialPreset::Aluminum: return "Aluminum";
	default: return "Custom";
	}
}

RenderingConfig::MaterialPreset RenderingConfig::getPresetFromName(const std::string& name)
{
	if (name == "Glass") return MaterialPreset::Glass;
	if (name == "Metal") return MaterialPreset::Metal;
	if (name == "Plastic") return MaterialPreset::Plastic;
	if (name == "Wood") return MaterialPreset::Wood;
	if (name == "Ceramic") return MaterialPreset::Ceramic;
	if (name == "Rubber") return MaterialPreset::Rubber;
	if (name == "Chrome") return MaterialPreset::Chrome;
	if (name == "Gold") return MaterialPreset::Gold;
	if (name == "Silver") return MaterialPreset::Silver;
	if (name == "Copper") return MaterialPreset::Copper;
	if (name == "Aluminum") return MaterialPreset::Aluminum;
	return MaterialPreset::Custom;
}

RenderingConfig::MaterialSettings RenderingConfig::getPresetMaterial(MaterialPreset preset) const
{
	auto it = s_materialPresets.find(preset);
	if (it != s_materialPresets.end()) {
		return it->second;
	}
	return MaterialSettings(); // Return default if not found
}

std::vector<std::string> RenderingConfig::getAvailableBlendModes()
{
	return {
		"None",
		"Alpha",
		"Additive",
		"Multiply",
		"Screen",
		"Overlay"
	};
}

std::string RenderingConfig::getBlendModeName(BlendMode mode)
{
	switch (mode) {
	case BlendMode::None: return "None";
	case BlendMode::Alpha: return "Alpha";
	case BlendMode::Additive: return "Additive";
	case BlendMode::Multiply: return "Multiply";
	case BlendMode::Screen: return "Screen";
	case BlendMode::Overlay: return "Overlay";
	default: return "Alpha";
	}
}

RenderingConfig::BlendMode RenderingConfig::getBlendModeFromName(const std::string& name)
{
	if (name == "None") return BlendMode::None;
	if (name == "Alpha") return BlendMode::Alpha;
	if (name == "Additive") return BlendMode::Additive;
	if (name == "Multiply") return BlendMode::Multiply;
	if (name == "Screen") return BlendMode::Screen;
	if (name == "Overlay") return BlendMode::Overlay;
	return BlendMode::Alpha;
}

std::vector<std::string> RenderingConfig::getAvailableTextureModes()
{
	return {
		"Replace",
		"Modulate",
		"Decal",
		"Blend"
	};
}

std::string RenderingConfig::getTextureModeName(TextureMode mode)
{
	switch (mode) {
	case TextureMode::Replace: return "Replace";
	case TextureMode::Modulate: return "Modulate";
	case TextureMode::Decal: return "Decal";
	case TextureMode::Blend: return "Blend";
	default: return "Modulate";
	}
}

RenderingConfig::TextureMode RenderingConfig::getTextureModeFromName(const std::string& name)
{
	if (name == "Replace") return TextureMode::Replace;
	if (name == "Modulate") return TextureMode::Modulate;
	if (name == "Decal") return TextureMode::Decal;
	if (name == "Blend") return TextureMode::Blend;
	return TextureMode::Modulate;
}

// Shading mode utility methods
std::vector<std::string> RenderingConfig::getAvailableShadingModes()
{
	return {
		"Flat",
		"Gouraud",
		"Phong",
		"Smooth",
		"Wireframe",
		"Points"
	};
}

std::string RenderingConfig::getShadingModeName(ShadingMode mode)
{
	switch (mode) {
	case ShadingMode::Flat: return "Flat";
	case ShadingMode::Gouraud: return "Gouraud";
	case ShadingMode::Phong: return "Phong";
	case ShadingMode::Smooth: return "Smooth";
	case ShadingMode::Wireframe: return "Wireframe";
	case ShadingMode::Points: return "Points";
	default: return "Smooth";
	}
}

RenderingConfig::ShadingMode RenderingConfig::getShadingModeFromName(const std::string& name)
{
	if (name == "Flat") return ShadingMode::Flat;
	if (name == "Gouraud") return ShadingMode::Gouraud;
	if (name == "Phong") return ShadingMode::Phong;
	if (name == "Smooth") return ShadingMode::Smooth;
	if (name == "Wireframe") return ShadingMode::Wireframe;
	if (name == "Points") return ShadingMode::Points;
	return ShadingMode::Smooth;
}

// Display mode utility methods
std::vector<std::string> RenderingConfig::getAvailableDisplayModes()
{
	return {
		"Solid",
		"Wireframe",
		"HiddenLine",
		"SolidWireframe",
		"Points",
		"Transparent",
		"NoShading"
	};
}

std::string RenderingConfig::getDisplayModeName(DisplayMode mode)
{
	switch (mode) {
	case DisplayMode::NoShading: return "NoShading";
	case DisplayMode::Points: return "Points";
	case DisplayMode::Wireframe: return "Wireframe";
	case DisplayMode::FlatLines: return "FlatLines";
	case DisplayMode::Solid: return "Solid";
	case DisplayMode::Transparent: return "Transparent";
	case DisplayMode::HiddenLine: return "HiddenLine";
	default: return "Solid";
	}
}

RenderingConfig::DisplayMode RenderingConfig::getDisplayModeFromName(const std::string& name)
{
	if (name == "NoShading") return DisplayMode::NoShading;
	if (name == "Points") return DisplayMode::Points;
	if (name == "Wireframe") return DisplayMode::Wireframe;
	if (name == "FlatLines") return DisplayMode::FlatLines;
	if (name == "Solid") return DisplayMode::Solid;
	if (name == "Transparent") return DisplayMode::Transparent;
	if (name == "HiddenLine") return DisplayMode::HiddenLine;
	return DisplayMode::Solid;
}

// Quality utility methods
std::vector<std::string> RenderingConfig::getAvailableQualityModes()
{
	return {
		"Draft",
		"Normal",
		"High",
		"Ultra",
		"Realtime"
	};
}

std::string RenderingConfig::getQualityModeName(RenderingQuality quality)
{
	switch (quality) {
	case RenderingQuality::Draft: return "Draft";
	case RenderingQuality::Normal: return "Normal";
	case RenderingQuality::High: return "High";
	case RenderingQuality::Ultra: return "Ultra";
	case RenderingQuality::Realtime: return "Realtime";
	default: return "Normal";
	}
}

RenderingConfig::RenderingQuality RenderingConfig::getQualityModeFromName(const std::string& name)
{
	if (name == "Draft") return RenderingQuality::Draft;
	if (name == "Normal") return RenderingQuality::Normal;
	if (name == "High") return RenderingQuality::High;
	if (name == "Ultra") return RenderingQuality::Ultra;
	if (name == "Realtime") return RenderingQuality::Realtime;
	return RenderingQuality::Normal;
}

// Shadow mode utility methods
std::vector<std::string> RenderingConfig::getAvailableShadowModes()
{
	return {
		"None",
		"Hard",
		"Soft",
		"Volumetric",
		"Contact",
		"Cascade"
	};
}

std::string RenderingConfig::getShadowModeName(ShadowMode mode)
{
	switch (mode) {
	case ShadowMode::None: return "None";
	case ShadowMode::Hard: return "Hard";
	case ShadowMode::Soft: return "Soft";
	case ShadowMode::Volumetric: return "Volumetric";
	case ShadowMode::Contact: return "Contact";
	case ShadowMode::Cascade: return "Cascade";
	default: return "Soft";
	}
}

RenderingConfig::ShadowMode RenderingConfig::getShadowModeFromName(const std::string& name)
{
	if (name == "None") return ShadowMode::None;
	if (name == "Hard") return ShadowMode::Hard;
	if (name == "Soft") return ShadowMode::Soft;
	if (name == "Volumetric") return ShadowMode::Volumetric;
	if (name == "Contact") return ShadowMode::Contact;
	if (name == "Cascade") return ShadowMode::Cascade;
	return ShadowMode::Soft;
}

// Lighting model utility methods
std::vector<std::string> RenderingConfig::getAvailableLightingModels()
{
	return {
		"Lambert",
		"BlinnPhong",
		"CookTorrance",
		"OrenNayar",
		"Minnaert",
		"Fresnel"
	};
}

std::string RenderingConfig::getLightingModelName(LightingModel model)
{
	switch (model) {
	case LightingModel::Lambert: return "Lambert";
	case LightingModel::BlinnPhong: return "BlinnPhong";
	case LightingModel::CookTorrance: return "CookTorrance";
	case LightingModel::OrenNayar: return "OrenNayar";
	case LightingModel::Minnaert: return "Minnaert";
	case LightingModel::Fresnel: return "Fresnel";
	default: return "BlinnPhong";
	}
}

RenderingConfig::LightingModel RenderingConfig::getLightingModelFromName(const std::string& name)
{
	if (name == "Lambert") return LightingModel::Lambert;
	if (name == "BlinnPhong") return LightingModel::BlinnPhong;
	if (name == "CookTorrance") return LightingModel::CookTorrance;
	if (name == "OrenNayar") return LightingModel::OrenNayar;
	if (name == "Minnaert") return LightingModel::Minnaert;
	if (name == "Fresnel") return LightingModel::Fresnel;
	return LightingModel::BlinnPhong;
}

void RenderingConfig::applyMaterialPreset(MaterialPreset preset)
{
	if (preset == MaterialPreset::Custom) {
		return; // Don't change anything for custom
	}

	MaterialSettings presetMaterial = getPresetMaterial(preset);
	setMaterialSettings(presetMaterial);
}

std::string RenderingConfig::getConfigFilePath() const
{
	// Use ConfigManager's config file path instead of independent file
	ConfigManager& cm = ConfigManager::getInstance();
	return cm.getConfigFilePath();
}

bool RenderingConfig::loadFromFile(const std::string& filename)
{
	// Use ConfigManager instead of independent file
	ConfigManager& cm = ConfigManager::getInstance();
	if (!cm.isInitialized()) {
		LOG_WRN_S("RenderingConfig: ConfigManager not initialized, using defaults");
		return false;
	}

	// Load from ConfigManager sections
	// Material section
	{
		std::string value = cm.getString("Material", "AmbientColor", "");
		if (!value.empty()) {
			m_materialSettings.ambientColor = parseColor(value, m_materialSettings.ambientColor);
		}
		value = cm.getString("Material", "DiffuseColor", "");
		if (!value.empty()) {
			m_materialSettings.diffuseColor = parseColor(value, m_materialSettings.diffuseColor);
		}
		value = cm.getString("Material", "SpecularColor", "");
		if (!value.empty()) {
			m_materialSettings.specularColor = parseColor(value, m_materialSettings.specularColor);
		}
		m_materialSettings.shininess = cm.getDouble("Material", "Shininess", m_materialSettings.shininess);
		m_materialSettings.transparency = cm.getDouble("Material", "Transparency", m_materialSettings.transparency);
	}

	// Lighting section
	{
		std::string value = cm.getString("Lighting", "AmbientColor", "");
		if (!value.empty()) {
			m_lightingSettings.ambientColor = parseColor(value, m_lightingSettings.ambientColor);
		}
		value = cm.getString("Lighting", "DiffuseColor", "");
		if (!value.empty()) {
			m_lightingSettings.diffuseColor = parseColor(value, m_lightingSettings.diffuseColor);
		}
		value = cm.getString("Lighting", "SpecularColor", "");
		if (!value.empty()) {
			m_lightingSettings.specularColor = parseColor(value, m_lightingSettings.specularColor);
		}
		m_lightingSettings.intensity = cm.getDouble("Lighting", "Intensity", m_lightingSettings.intensity);
		m_lightingSettings.ambientIntensity = cm.getDouble("Lighting", "AmbientIntensity", m_lightingSettings.ambientIntensity);
	}

	// Texture section
	{
		std::string value = cm.getString("Texture", "Color", "");
		if (!value.empty()) {
			m_textureSettings.color = parseColor(value, m_textureSettings.color);
		}
		m_textureSettings.intensity = cm.getDouble("Texture", "Intensity", m_textureSettings.intensity);
		m_textureSettings.enabled = cm.getBool("Texture", "Enabled", m_textureSettings.enabled);
		m_textureSettings.imagePath = cm.getString("Texture", "ImagePath", m_textureSettings.imagePath);
		value = cm.getString("Texture", "TextureMode", "");
		if (!value.empty()) {
			m_textureSettings.textureMode = getTextureModeFromName(value);
		}
	}

	// Blend section
	{
		std::string value = cm.getString("Blend", "BlendMode", "");
		if (!value.empty()) {
			m_blendSettings.blendMode = getBlendModeFromName(value);
		}
		m_blendSettings.depthTest = cm.getBool("Blend", "DepthTest", m_blendSettings.depthTest);
		m_blendSettings.depthWrite = cm.getBool("Blend", "DepthWrite", m_blendSettings.depthWrite);
		m_blendSettings.cullFace = cm.getBool("Blend", "CullFace", m_blendSettings.cullFace);
		m_blendSettings.alphaThreshold = cm.getDouble("Blend", "AlphaThreshold", m_blendSettings.alphaThreshold);
	}

	// Shading section
	{
		std::string value = cm.getString("Shading", "ShadingMode", "");
		if (!value.empty()) {
			m_shadingSettings.shadingMode = getShadingModeFromName(value);
		}
		m_shadingSettings.smoothNormals = cm.getBool("Shading", "SmoothNormals", m_shadingSettings.smoothNormals);
		m_shadingSettings.wireframeWidth = cm.getDouble("Shading", "WireframeWidth", m_shadingSettings.wireframeWidth);
		m_shadingSettings.pointSize = cm.getDouble("Shading", "PointSize", m_shadingSettings.pointSize);
	}

	// Display section
	{
		std::string value = cm.getString("Display", "DisplayMode", "");
		if (!value.empty()) {
			m_displaySettings.displayMode = getDisplayModeFromName(value);
		}
		m_displaySettings.showEdges = cm.getBool("Display", "ShowEdges", m_displaySettings.showEdges);
		m_displaySettings.showVertices = cm.getBool("Display", "ShowVertices", m_displaySettings.showVertices);
		m_displaySettings.edgeWidth = cm.getDouble("Display", "EdgeWidth", m_displaySettings.edgeWidth);
		m_displaySettings.vertexSize = cm.getDouble("Display", "VertexSize", m_displaySettings.vertexSize);
		value = cm.getString("Display", "EdgeColor", "");
		if (!value.empty()) {
			m_displaySettings.edgeColor = parseColor(value, m_displaySettings.edgeColor);
		}
		value = cm.getString("Display", "VertexColor", "");
		if (!value.empty()) {
			m_displaySettings.vertexColor = parseColor(value, m_displaySettings.vertexColor);
		}
		m_displaySettings.showOriginalEdges = cm.getBool("Display", "ShowOriginalEdges", m_displaySettings.showOriginalEdges);
	}

	// Quality section
	{
		std::string value = cm.getString("Quality", "Quality", "");
		if (!value.empty()) {
			m_qualitySettings.quality = getQualityModeFromName(value);
		}
		m_qualitySettings.tessellationLevel = cm.getInt("Quality", "TessellationLevel", m_qualitySettings.tessellationLevel);
		m_qualitySettings.antiAliasingSamples = cm.getInt("Quality", "AntiAliasingSamples", m_qualitySettings.antiAliasingSamples);
		m_qualitySettings.enableLOD = cm.getBool("Quality", "EnableLOD", m_qualitySettings.enableLOD);
		m_qualitySettings.lodDistance = cm.getDouble("Quality", "LODDistance", m_qualitySettings.lodDistance);
	}

	// Shadow section
	{
		std::string value = cm.getString("Shadow", "ShadowMode", "");
		if (!value.empty()) {
			m_shadowSettings.shadowMode = getShadowModeFromName(value);
		}
		m_shadowSettings.shadowIntensity = cm.getDouble("Shadow", "ShadowIntensity", m_shadowSettings.shadowIntensity);
		m_shadowSettings.shadowSoftness = cm.getDouble("Shadow", "ShadowSoftness", m_shadowSettings.shadowSoftness);
		m_shadowSettings.shadowMapSize = cm.getInt("Shadow", "ShadowMapSize", m_shadowSettings.shadowMapSize);
		m_shadowSettings.shadowBias = cm.getDouble("Shadow", "ShadowBias", m_shadowSettings.shadowBias);
	}

	// LightingModel section
	{
		std::string value = cm.getString("LightingModel", "LightingModel", "");
		if (!value.empty()) {
			m_lightingModelSettings.lightingModel = getLightingModelFromName(value);
		}
		m_lightingModelSettings.roughness = cm.getDouble("LightingModel", "Roughness", m_lightingModelSettings.roughness);
		m_lightingModelSettings.metallic = cm.getDouble("LightingModel", "Metallic", m_lightingModelSettings.metallic);
		m_lightingModelSettings.fresnel = cm.getDouble("LightingModel", "Fresnel", m_lightingModelSettings.fresnel);
		m_lightingModelSettings.subsurfaceScattering = cm.getDouble("LightingModel", "SubsurfaceScattering", m_lightingModelSettings.subsurfaceScattering);
	}

	// Notify listeners of settings change
	notifySettingsChanged();

	return true;
}

bool RenderingConfig::saveToFile(const std::string& filename) const
{
	// Use ConfigManager instead of independent file
	ConfigManager& cm = ConfigManager::getInstance();
	if (!cm.isInitialized()) {
		LOG_ERR_S("RenderingConfig: ConfigManager not initialized, cannot save");
		return false;
	}

	// Save to ConfigManager sections
	// Material section
	cm.setString("Material", "AmbientColor", colorToString(m_materialSettings.ambientColor));
	cm.setString("Material", "DiffuseColor", colorToString(m_materialSettings.diffuseColor));
	cm.setString("Material", "SpecularColor", colorToString(m_materialSettings.specularColor));
	cm.setDouble("Material", "Shininess", m_materialSettings.shininess);
	cm.setDouble("Material", "Transparency", m_materialSettings.transparency);

	// Lighting section
	cm.setString("Lighting", "AmbientColor", colorToString(m_lightingSettings.ambientColor));
	cm.setString("Lighting", "DiffuseColor", colorToString(m_lightingSettings.diffuseColor));
	cm.setString("Lighting", "SpecularColor", colorToString(m_lightingSettings.specularColor));
	cm.setDouble("Lighting", "Intensity", m_lightingSettings.intensity);
	cm.setDouble("Lighting", "AmbientIntensity", m_lightingSettings.ambientIntensity);

	// Texture section
	cm.setString("Texture", "Color", colorToString(m_textureSettings.color));
	cm.setDouble("Texture", "Intensity", m_textureSettings.intensity);
	cm.setBool("Texture", "Enabled", m_textureSettings.enabled);
	cm.setString("Texture", "ImagePath", m_textureSettings.imagePath);
	cm.setString("Texture", "TextureMode", getTextureModeName(m_textureSettings.textureMode));

	// Blend section
	cm.setString("Blend", "BlendMode", getBlendModeName(m_blendSettings.blendMode));
	cm.setBool("Blend", "DepthTest", m_blendSettings.depthTest);
	cm.setBool("Blend", "DepthWrite", m_blendSettings.depthWrite);
	cm.setBool("Blend", "CullFace", m_blendSettings.cullFace);
	cm.setDouble("Blend", "AlphaThreshold", m_blendSettings.alphaThreshold);

	// Shading section
	cm.setString("Shading", "ShadingMode", getShadingModeName(m_shadingSettings.shadingMode));
	cm.setBool("Shading", "SmoothNormals", m_shadingSettings.smoothNormals);
	cm.setDouble("Shading", "WireframeWidth", m_shadingSettings.wireframeWidth);
	cm.setDouble("Shading", "PointSize", m_shadingSettings.pointSize);

	// Display section
	cm.setString("Display", "DisplayMode", getDisplayModeName(m_displaySettings.displayMode));
	cm.setBool("Display", "ShowEdges", m_displaySettings.showEdges);
	cm.setBool("Display", "ShowVertices", m_displaySettings.showVertices);
	cm.setDouble("Display", "EdgeWidth", m_displaySettings.edgeWidth);
	cm.setDouble("Display", "VertexSize", m_displaySettings.vertexSize);
	cm.setString("Display", "EdgeColor", colorToString(m_displaySettings.edgeColor));
	cm.setString("Display", "VertexColor", colorToString(m_displaySettings.vertexColor));
	cm.setBool("Display", "ShowOriginalEdges", m_displaySettings.showOriginalEdges);

	// Quality section
	cm.setString("Quality", "Quality", getQualityModeName(m_qualitySettings.quality));
	cm.setInt("Quality", "TessellationLevel", m_qualitySettings.tessellationLevel);
	cm.setInt("Quality", "AntiAliasingSamples", m_qualitySettings.antiAliasingSamples);
	cm.setBool("Quality", "EnableLOD", m_qualitySettings.enableLOD);
	cm.setDouble("Quality", "LODDistance", m_qualitySettings.lodDistance);

	// Shadow section
	cm.setString("Shadow", "ShadowMode", getShadowModeName(m_shadowSettings.shadowMode));
	cm.setDouble("Shadow", "ShadowIntensity", m_shadowSettings.shadowIntensity);
	cm.setDouble("Shadow", "ShadowSoftness", m_shadowSettings.shadowSoftness);
	cm.setInt("Shadow", "ShadowMapSize", m_shadowSettings.shadowMapSize);
	cm.setDouble("Shadow", "ShadowBias", m_shadowSettings.shadowBias);

	// LightingModel section
	cm.setString("LightingModel", "LightingModel", getLightingModelName(m_lightingModelSettings.lightingModel));
	cm.setDouble("LightingModel", "Roughness", m_lightingModelSettings.roughness);
	cm.setDouble("LightingModel", "Metallic", m_lightingModelSettings.metallic);
	cm.setDouble("LightingModel", "Fresnel", m_lightingModelSettings.fresnel);
	cm.setDouble("LightingModel", "SubsurfaceScattering", m_lightingModelSettings.subsurfaceScattering);

	// Save ConfigManager to file
	return cm.save();

	// Notify listeners of settings change
	notifySettingsChanged();

	return true;
}

Quantity_Color RenderingConfig::parseColor(const std::string& value, const Quantity_Color& defaultValue) const
{
	std::istringstream iss(value);
	std::string token;
	std::vector<double> components;

	while (std::getline(iss, token, ',')) {
		token.erase(0, token.find_first_not_of(" \t"));
		token.erase(token.find_last_not_of(" \t") + 1);
		try {
			components.push_back(std::stod(token));
		}
		catch (const std::exception& e) {
			LOG_ERR_S("RenderingConfig: Failed to parse color component: " + token);
			return defaultValue;
		}
	}

	if (components.size() >= 3) {
		return Quantity_Color(components[0], components[1], components[2], Quantity_TOC_RGB);
	}

	return defaultValue;
}

std::string RenderingConfig::colorToString(const Quantity_Color& color) const
{
	std::ostringstream oss;
	oss << color.Red() << "," << color.Green() << "," << color.Blue();
	return oss.str();
}

// Setters with auto-save
void RenderingConfig::setMaterialSettings(const MaterialSettings& settings)
{
	m_materialSettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setLightingSettings(const LightingSettings& settings)
{
	m_lightingSettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setTextureSettings(const TextureSettings& settings)
{
	m_textureSettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setMaterialAmbientColor(const Quantity_Color& color)
{
	m_materialSettings.ambientColor = color;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setMaterialDiffuseColor(const Quantity_Color& color)
{
	m_materialSettings.diffuseColor = color;
	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

void RenderingConfig::setMaterialSpecularColor(const Quantity_Color& color)
{
	m_materialSettings.specularColor = color;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setMaterialShininess(double shininess)
{
	m_materialSettings.shininess = shininess;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setMaterialTransparency(double transparency)
{
	m_materialSettings.transparency = transparency;
	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

void RenderingConfig::setLightAmbientColor(const Quantity_Color& color)
{
	m_lightingSettings.ambientColor = color;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setLightDiffuseColor(const Quantity_Color& color)
{
	m_lightingSettings.diffuseColor = color;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setLightSpecularColor(const Quantity_Color& color)
{
	m_lightingSettings.specularColor = color;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setLightIntensity(double intensity)
{
	m_lightingSettings.intensity = intensity;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setLightAmbientIntensity(double intensity)
{
	m_lightingSettings.ambientIntensity = intensity;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setTextureColor(const Quantity_Color& color)
{
	m_textureSettings.color = color;
	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

void RenderingConfig::setTextureIntensity(double intensity)
{
	m_textureSettings.intensity = intensity;
	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

void RenderingConfig::setTextureEnabled(bool enabled)
{
	m_textureSettings.enabled = enabled;
	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

void RenderingConfig::setTextureImagePath(const std::string& path)
{
	m_textureSettings.imagePath = path;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setTextureMode(TextureMode mode)
{
	m_textureSettings.textureMode = mode;
	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

void RenderingConfig::setBlendSettings(const BlendSettings& settings)
{
	m_blendSettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setBlendMode(BlendMode mode)
{
	m_blendSettings.blendMode = mode;
	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

void RenderingConfig::setDepthTest(bool enabled)
{
	m_blendSettings.depthTest = enabled;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setDepthWrite(bool enabled)
{
	m_blendSettings.depthWrite = enabled;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setCullFace(bool enabled)
{
	m_blendSettings.cullFace = enabled;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setAlphaThreshold(double threshold)
{
	m_blendSettings.alphaThreshold = threshold;
	if (m_autoSave) {
		saveToFile();
	}
}

// New setter methods for rendering modes
void RenderingConfig::setShadingSettings(const ShadingSettings& settings)
{
	m_shadingSettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setDisplaySettings(const DisplaySettings& settings)
{
	m_displaySettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setQualitySettings(const QualitySettings& settings)
{
	m_qualitySettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setShadowSettings(const ShadowSettings& settings)
{
	m_shadowSettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setLightingModelSettings(const LightingModelSettings& settings)
{
	m_lightingModelSettings = settings;
	if (m_autoSave) {
		saveToFile();
	}
}

// Shading mode individual setters
void RenderingConfig::setShadingMode(ShadingMode mode)
{
	m_shadingSettings.shadingMode = mode;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setSmoothNormals(bool enabled)
{
	m_shadingSettings.smoothNormals = enabled;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setWireframeWidth(double width)
{
	m_shadingSettings.wireframeWidth = width;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setPointSize(double size)
{
	m_shadingSettings.pointSize = size;
	if (m_autoSave) {
		saveToFile();
	}
}

// Display mode individual setters
void RenderingConfig::setDisplayMode(DisplayMode mode)
{
	m_displaySettings.displayMode = mode;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setShowEdges(bool enabled)
{
	m_displaySettings.showEdges = enabled;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setShowVertices(bool enabled)
{
	m_displaySettings.showVertices = enabled;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setEdgeWidth(double width)
{
	m_displaySettings.edgeWidth = width;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setVertexSize(double size)
{
	m_displaySettings.vertexSize = size;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setEdgeColor(const Quantity_Color& color)
{
	m_displaySettings.edgeColor = color;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setVertexColor(const Quantity_Color& color)
{
	m_displaySettings.vertexColor = color;
	if (m_autoSave) {
		saveToFile();
	}
}

// Quality individual setters
void RenderingConfig::setRenderingQuality(RenderingQuality quality)
{
	m_qualitySettings.quality = quality;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setTessellationLevel(int level)
{
	m_qualitySettings.tessellationLevel = level;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setAntiAliasingSamples(int samples)
{
	m_qualitySettings.antiAliasingSamples = samples;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setEnableLOD(bool enabled)
{
	m_qualitySettings.enableLOD = enabled;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setLODDistance(double distance)
{
	m_qualitySettings.lodDistance = distance;
	if (m_autoSave) {
		saveToFile();
	}
}

// Shadow individual setters
void RenderingConfig::setShadowMode(ShadowMode mode)
{
	m_shadowSettings.shadowMode = mode;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setShadowIntensity(double intensity)
{
	m_shadowSettings.shadowIntensity = intensity;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setShadowSoftness(double softness)
{
	m_shadowSettings.shadowSoftness = softness;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setShadowMapSize(int size)
{
	m_shadowSettings.shadowMapSize = size;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setShadowBias(double bias)
{
	m_shadowSettings.shadowBias = bias;
	if (m_autoSave) {
		saveToFile();
	}
}

// Lighting model individual setters
void RenderingConfig::setLightingModel(LightingModel model)
{
	m_lightingModelSettings.lightingModel = model;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setRoughness(double roughness)
{
	m_lightingModelSettings.roughness = roughness;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setMetallic(double metallic)
{
	m_lightingModelSettings.metallic = metallic;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setFresnel(double fresnel)
{
	m_lightingModelSettings.fresnel = fresnel;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::setSubsurfaceScattering(double scattering)
{
	m_lightingModelSettings.subsurfaceScattering = scattering;
	if (m_autoSave) {
		saveToFile();
	}
}

void RenderingConfig::resetToDefaults()
{
	m_materialSettings = MaterialSettings();
	m_lightingSettings = LightingSettings();
	m_textureSettings = TextureSettings();
	m_blendSettings = BlendSettings();
	m_shadingSettings = ShadingSettings();
	m_displaySettings = DisplaySettings();
	m_qualitySettings = QualitySettings();
	m_shadowSettings = ShadowSettings();
	m_lightingModelSettings = LightingModelSettings();

	if (m_autoSave) {
		saveToFile();
	}

	// Notify listeners of settings change
	notifySettingsChanged();
}

// Notification system implementation
void RenderingConfig::registerSettingsChangedCallback(SettingsChangedCallback callback)
{
	m_settingsChangedCallback = callback;
}

void RenderingConfig::unregisterSettingsChangedCallback()
{
	m_settingsChangedCallback = nullptr;
}

void RenderingConfig::notifySettingsChanged() const
{
	if (m_settingsChangedCallback) {
		m_settingsChangedCallback();
	}
	else {
		LOG_WRN_S("RenderingConfig: Settings changed but no callback is registered");
	}
}

// Selected objects rendering settings implementation
// These methods apply settings only to selected geometries

void RenderingConfig::applyMaterialSettingsToSelected(const MaterialSettings& settings)
{
	// Store the settings for selected objects
	// The actual application to selected objects is handled by SceneManager callback
	m_materialSettings = settings;
	notifySettingsChanged();
}

void RenderingConfig::applyTextureSettingsToSelected(const TextureSettings& settings)
{
	m_textureSettings = settings;
	notifySettingsChanged();
}

void RenderingConfig::applyBlendSettingsToSelected(const BlendSettings& settings)
{
	m_blendSettings = settings;
	notifySettingsChanged();
}

void RenderingConfig::applyShadingSettingsToSelected(const ShadingSettings& settings)
{
	m_shadingSettings = settings;
	notifySettingsChanged();
}

void RenderingConfig::applyDisplaySettingsToSelected(const DisplaySettings& settings)
{
	m_displaySettings = settings;
	notifySettingsChanged();
}

// Individual property setters for selected objects
void RenderingConfig::setSelectedMaterialAmbientColor(const Quantity_Color& color)
{
	m_materialSettings.ambientColor = color;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialDiffuseColor(const Quantity_Color& color)
{
	m_materialSettings.diffuseColor = color;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialSpecularColor(const Quantity_Color& color)
{
	m_materialSettings.specularColor = color;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialShininess(double shininess)
{
	m_materialSettings.shininess = shininess;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialTransparency(double transparency)
{
	m_materialSettings.transparency = transparency;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureColor(const Quantity_Color& color)
{
	m_textureSettings.color = color;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureIntensity(double intensity)
{
	m_textureSettings.intensity = intensity;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureEnabled(bool enabled)
{
	m_textureSettings.enabled = enabled;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureImagePath(const std::string& path)
{
	m_textureSettings.imagePath = path;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureMode(TextureMode mode)
{
	m_textureSettings.textureMode = mode;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedBlendMode(BlendMode mode)
{
	m_blendSettings.blendMode = mode;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedDepthTest(bool enabled)
{
	m_blendSettings.depthTest = enabled;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedDepthWrite(bool enabled)
{
	m_blendSettings.depthWrite = enabled;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedCullFace(bool enabled)
{
	m_blendSettings.cullFace = enabled;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedAlphaThreshold(double threshold)
{
	m_blendSettings.alphaThreshold = threshold;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedShadingMode(ShadingMode mode)
{
	m_shadingSettings.shadingMode = mode;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedSmoothNormals(bool enabled)
{
	m_shadingSettings.smoothNormals = enabled;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedWireframeWidth(double width)
{
	m_shadingSettings.wireframeWidth = width;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedPointSize(double size)
{
	m_shadingSettings.pointSize = size;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedDisplayMode(DisplayMode mode)
{
	m_displaySettings.displayMode = mode;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedShowEdges(bool enabled)
{
	m_displaySettings.showEdges = enabled;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedShowVertices(bool enabled)
{
	m_displaySettings.showVertices = enabled;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedEdgeWidth(double width)
{
	m_displaySettings.edgeWidth = width;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedVertexSize(double size)
{
	m_displaySettings.vertexSize = size;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedEdgeColor(const Quantity_Color& color)
{
	m_displaySettings.edgeColor = color;
	notifySettingsChanged();
}

void RenderingConfig::setSelectedVertexColor(const Quantity_Color& color)
{
	m_displaySettings.vertexColor = color;
	notifySettingsChanged();
}

bool RenderingConfig::hasSelectedObjects() const
{
	// This method should check with SceneManager or OCCViewer
	// For now, we'll return true to indicate that selection-based rendering is available
	// The actual selection check is done in SceneManager callback
	return true;
}

// Static method to get OCCViewer instance for selection checking
OCCViewer* RenderingConfig::getOCCViewerInstance()
{
	// This is a temporary solution - in a real implementation, we would have a proper
	// way to access the OCCViewer instance from RenderingConfig
	// For now, we'll return nullptr and handle the selection check in SceneManager callback
	return nullptr;
}

void RenderingConfig::applyMaterialPresetToSelected(MaterialPreset preset)
{
	MaterialSettings presetSettings = getPresetMaterial(preset);
	applyMaterialSettingsToSelected(presetSettings);
}

std::string RenderingConfig::getCurrentSelectionStatus() const
{
	// This would ideally check with OCCViewer, but for now return a placeholder
	return "Selection status: Available (check OCCViewer for actual selection)";
}

std::string RenderingConfig::getCurrentRenderingSettings() const
{
	std::stringstream ss;
	ss << "Current Rendering Settings:\n";
	ss << "- Material Diffuse: R=" << m_materialSettings.diffuseColor.Red()
		<< " G=" << m_materialSettings.diffuseColor.Green()
		<< " B=" << m_materialSettings.diffuseColor.Blue() << "\n";
	ss << "- Material Transparency: " << m_materialSettings.transparency << "\n";
	ss << "- Texture Enabled: " << (m_textureSettings.enabled ? "Yes" : "No") << "\n";
	ss << "- Texture Mode: " << getTextureModeName(m_textureSettings.textureMode) << "\n";
	ss << "- Texture Color: R=" << m_textureSettings.color.Red()
		<< " G=" << m_textureSettings.color.Green()
		<< " B=" << m_textureSettings.color.Blue() << "\n";
	ss << "- Blend Mode: " << getBlendModeName(m_blendSettings.blendMode) << "\n";
	ss << "- Display Mode: " << getDisplayModeName(m_displaySettings.displayMode) << "\n";
	ss << "- Shading Mode: " << getShadingModeName(m_shadingSettings.shadingMode);

	return ss.str();
}

void RenderingConfig::showTestFeedback() const
{
	std::string status = getCurrentSelectionStatus();
	std::string settings = getCurrentRenderingSettings();

}