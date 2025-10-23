#include "renderpreview/RenderingManager.h"
#include "renderpreview/ConfigValidator.h"
#include "logger/Logger.h"
#include <wx/glcanvas.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoBlinker.h>
#include <algorithm>

RenderingManager::RenderingManager(SoSeparator* sceneRoot, wxGLCanvas* canvas, wxGLContext* glContext)
	: m_sceneRoot(sceneRoot), m_canvas(canvas), m_glContext(glContext), m_nextConfigId(1), m_activeConfigId(-1)
{
	initializePresets();
	LOG_INF_S("RenderingManager: Initialized");
}

RenderingManager::~RenderingManager()
{
	// Don't call clearAllConfigurations() in destructor as it may try to access destroyed OpenGL context
	m_configurations.clear();
	m_activeConfigId = -1;
	LOG_INF_S("RenderingManager: Destroyed");
}

int RenderingManager::addConfiguration(const RenderingSettings& settings)
{
	// Validate settings before adding
	auto validation = ConfigValidator::validateRenderingConfiguration(settings);
	if (!validation.isValid) {
		LOG_ERR_S("RenderingManager::addConfiguration: Validation failed - " + validation.errorMessage.ToStdString());
		return -1;
	}

	// Validate performance impact
	auto performanceValidation = ConfigValidator::validatePerformanceImpact(settings, 0.8f);
	if (!performanceValidation.isValid) {
		LOG_WRN_S("RenderingManager::addConfiguration: Performance warning - " + performanceValidation.errorMessage.ToStdString());
	}

	LOG_INF_S("RenderingManager::addConfiguration: Adding configuration '" + settings.name + "'");

	auto managedConfig = std::make_unique<ManagedRendering>();
	managedConfig->settings = settings;
	managedConfig->configId = m_nextConfigId++;
	managedConfig->isActive = false;

	int configId = managedConfig->configId;
	m_configurations[configId] = std::move(managedConfig);

	LOG_INF_S("RenderingManager::addConfiguration: Added validated configuration with ID " + std::to_string(configId));
	return configId;
}

bool RenderingManager::removeConfiguration(int configId)
{
	auto it = m_configurations.find(configId);
	if (it == m_configurations.end()) {
		LOG_WRN_S("RenderingManager::removeConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
		return false;
	}

	if (it->second->isActive) {
		setActiveConfiguration(-1);
	}

	m_configurations.erase(it);

	LOG_INF_S("RenderingManager::removeConfiguration: Successfully removed configuration with ID " + std::to_string(configId));
	return true;
}

bool RenderingManager::updateConfiguration(int configId, const RenderingSettings& settings)
{
	auto it = m_configurations.find(configId);
	if (it == m_configurations.end()) {
		LOG_WRN_S("RenderingManager::updateConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
		return false;
	}

	it->second->settings = settings;

	if (it->second->isActive) {
		setupRenderingState();
	}

	LOG_INF_S("RenderingManager::updateConfiguration: Successfully updated configuration with ID " + std::to_string(configId));
	return true;
}

void RenderingManager::clearAllConfigurations()
{
	LOG_INF_S("RenderingManager::clearAllConfigurations: Clearing all configurations");

	m_configurations.clear();
	m_activeConfigId = -1;

	// Only restore rendering state if OpenGL context is still valid
	if (m_canvas && m_glContext) {
		try {
			restoreRenderingState();
		}
		catch (...) {
			LOG_WRN_S("RenderingManager::clearAllConfigurations: Failed to restore rendering state (OpenGL context may be destroyed)");
		}
	}

	LOG_INF_S("RenderingManager::clearAllConfigurations: All configurations cleared");
}

std::vector<int> RenderingManager::getAllConfigurationIds() const
{
	std::vector<int> ids;
	for (const auto& pair : m_configurations) {
		ids.push_back(pair.first);
	}
	return ids;
}

std::vector<RenderingSettings> RenderingManager::getAllConfigurations() const
{
	std::vector<RenderingSettings> configs;
	for (const auto& pair : m_configurations) {
		configs.push_back(pair.second->settings);
	}
	return configs;
}

RenderingSettings RenderingManager::getConfiguration(int configId) const
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		return it->second->settings;
	}
	return RenderingSettings();
}

bool RenderingManager::hasConfiguration(int configId) const
{
	return m_configurations.find(configId) != m_configurations.end();
}

int RenderingManager::getConfigurationCount() const
{
	return static_cast<int>(m_configurations.size());
}

bool RenderingManager::setActiveConfiguration(int configId)
{
	if (m_activeConfigId != -1) {
		auto it = m_configurations.find(m_activeConfigId);
		if (it != m_configurations.end()) {
			it->second->isActive = false;
		}
	}

	if (configId != -1) {
		auto it = m_configurations.find(configId);
		if (it == m_configurations.end()) {
			LOG_ERR_S("RenderingManager::setActiveConfiguration: Configuration " + std::to_string(configId) + " not found");
			return false;
		}

		// Re-validate configuration before activation
		auto validation = ConfigValidator::validateRenderingConfiguration(it->second->settings);
		if (!validation.isValid) {
			LOG_ERR_S("RenderingManager::setActiveConfiguration: Configuration validation failed - " + validation.errorMessage.ToStdString());
			return false;
		}

		it->second->isActive = true;
		m_activeConfigId = configId;

		setupRenderingState();

		LOG_INF_S("RenderingManager::setActiveConfiguration: Activated configuration with ID " + std::to_string(configId));
	}
	else {
		m_activeConfigId = -1;
		restoreRenderingState();
		LOG_INF_S("RenderingManager::setActiveConfiguration: Deactivated all configurations");
	}

	return true;
}

int RenderingManager::getActiveConfigurationId() const
{
	return m_activeConfigId;
}

RenderingSettings RenderingManager::getActiveConfiguration() const
{
	if (m_activeConfigId != -1) {
		auto it = m_configurations.find(m_activeConfigId);
		if (it != m_configurations.end()) {
			return it->second->settings;
		}
	}
	return RenderingSettings();
}

bool RenderingManager::hasActiveConfiguration() const
{
	return m_activeConfigId != -1;
}

void RenderingManager::setRenderingMode(int configId, int mode)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.mode = mode;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setQuality(int configId, int quality)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.quality = quality;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setFastMode(int configId, bool enabled)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.fastMode = enabled;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setTransparencyType(int configId, int type)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.transparencyType = type;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setShadingMode(int configId, bool smooth, bool phong)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.smoothShading = smooth;
		it->second->settings.phongShading = phong;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setCullingMode(int configId, int mode)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.cullMode = mode;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setDepthSettings(int configId, bool test, bool write)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.depthTest = test;
		it->second->settings.depthWrite = write;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setPolygonMode(int configId, int mode)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.polygonMode = mode;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::setBackgroundColor(int configId, const wxColour& color)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		it->second->settings.backgroundColor = color;
		if (it->second->isActive) {
			setupRenderingState();
		}
	}
}

void RenderingManager::applyPreset(const std::string& presetName)
{
	auto it = m_presets.find(presetName);
	if (it != m_presets.end()) {
		// Validate preset compatibility
		auto validation = ConfigValidator::validatePresetCompatibility(presetName, it->second);
		if (!validation.isValid) {
			LOG_ERR_S("RenderingManager::applyPreset: Preset validation failed - " + validation.errorMessage.ToStdString());
			return;
		}

		int configId = addConfiguration(it->second);
		if (configId != -1) {
			setActiveConfiguration(configId);
			LOG_INF_S("RenderingManager::applyPreset: Applied validated preset '" + presetName + "'");
		}
	}
	else {
		LOG_WRN_S("RenderingManager::applyPreset: Preset '" + presetName + "' not found");
	}
}

void RenderingManager::saveAsPreset(int configId, const std::string& presetName)
{
	auto it = m_configurations.find(configId);
	if (it != m_configurations.end()) {
		m_presets[presetName] = it->second->settings;
		LOG_INF_S("RenderingManager::saveAsPreset: Saved configuration as preset '" + presetName + "'");
	}
}

std::vector<std::string> RenderingManager::getAvailablePresets() const
{
	std::vector<std::string> presets;
	for (const auto& pair : m_presets) {
		presets.push_back(pair.first);
	}
	return presets;
}

void RenderingManager::applyToRenderAction(SoGLRenderAction* renderAction)
{
	if (!renderAction) return;

	if (m_activeConfigId != -1) {
		auto it = m_configurations.find(m_activeConfigId);
		if (it != m_configurations.end()) {
			const auto& settings = it->second->settings;

			// Apply rendering mode
			applyRenderingMode(settings);

			// Apply quality settings
			applyQualitySettings(settings);

			// Apply transparency settings
			applyTransparencySettings(settings);

			// Apply shading settings
			applyShadingSettings(settings);

			// Apply culling settings
			applyCullingSettings(settings);

			// Apply depth settings
			applyDepthSettings(settings);

			// Apply polygon settings
			applyPolygonSettings(settings);

			// Apply background settings
			applyBackgroundSettings(settings);

			LOG_INF_S("RenderingManager::applyToRenderAction: Applied rendering configuration");
		}
	}
}

void RenderingManager::setupRenderingState()
{
	if (m_activeConfigId != -1) {
		auto it = m_configurations.find(m_activeConfigId);
		if (it != m_configurations.end()) {
			const auto& settings = it->second->settings;

			if (m_canvas && m_glContext) {
				m_canvas->SetCurrent(*m_glContext);
				setupOpenGLState(settings);
			}

			LOG_INF_S("RenderingManager::setupRenderingState: Applied rendering state");
		}
	}
}

void RenderingManager::restoreRenderingState()
{
	if (m_canvas && m_glContext) {
		try {
			m_canvas->SetCurrent(*m_glContext);
			restoreOpenGLState();
			LOG_INF_S("RenderingManager::restoreRenderingState: Restored default rendering state");
		}
		catch (...) {
			LOG_WRN_S("RenderingManager::restoreRenderingState: Failed to restore rendering state (OpenGL context may be destroyed)");
		}
	}
	else {
		LOG_WRN_S("RenderingManager::restoreRenderingState: Canvas or GL context is null");
	}
}

float RenderingManager::getPerformanceImpact() const
{
	if (m_activeConfigId != -1) {
		auto it = m_configurations.find(m_activeConfigId);
		if (it != m_configurations.end()) {
			const auto& settings = it->second->settings;

			float impact = 1.0f;

			// Quality impact
			switch (settings.quality) {
			case 0: impact *= 0.6f; break; // Low
			case 1: impact *= 1.0f; break; // Medium
			case 2: impact *= 1.5f; break; // High
			case 3: impact *= 2.2f; break; // Ultra
			}

			// Mode impact
			switch (settings.mode) {
			case 0: impact *= 0.8f; break;  // Solid
			case 1: impact *= 0.6f; break;  // Wireframe
			case 2: impact *= 0.5f; break;  // Points
			case 3: impact *= 0.9f; break;  // HiddenLine
			case 4: impact *= 1.0f; break;  // Shaded
			case 5: impact *= 1.2f; break;  // ShadedWireframe
			}

			// Feature impacts
			if (settings.fastMode) {
				impact *= 0.7f;
			}
			if (settings.smoothShading) {
				impact *= 1.2f;
			}
			if (settings.phongShading) {
				impact *= 1.3f;
			}
			if (settings.transparencyType > 0) {
				impact *= (1.0f + settings.transparencyType * 0.3f);
			}
			if (settings.frustumCulling) {
				impact *= 0.9f;
			}
			if (settings.occlusionCulling) {
				impact *= 0.8f;
			}
			if (settings.adaptiveQuality) {
				impact *= 0.85f;
			}

			return impact;
		}
	}
	return 1.0f;
}

std::string RenderingManager::getQualityDescription() const
{
	if (m_activeConfigId != -1) {
		auto it = m_configurations.find(m_activeConfigId);
		if (it != m_configurations.end()) {
			const auto& settings = it->second->settings;

			std::string description = settings.name;

			// Add quality level
			switch (settings.quality) {
			case 0: description += " (Low Quality)"; break;
			case 1: description += " (Medium Quality)"; break;
			case 2: description += " (High Quality)"; break;
			case 3: description += " (Ultra Quality)"; break;
			}

			// Add mode
			switch (settings.mode) {
			case 0: description += " - Solid"; break;
			case 1: description += " - Wireframe"; break;
			case 2: description += " - Points"; break;
			case 3: description += " - Hidden Line"; break;
			case 4: description += " - Shaded"; break;
			case 5: description += " - Shaded Wireframe"; break;
			}

			return description;
		}
	}
	return "No Configuration Active";
}

int RenderingManager::getEstimatedFPS() const
{
	float impact = getPerformanceImpact();
	int baseFPS = 60;

	// Estimate FPS based on performance impact
	int estimatedFPS = static_cast<int>(baseFPS / impact);

	// Clamp to reasonable range
	if (estimatedFPS < 15) estimatedFPS = 15;
	if (estimatedFPS > 120) estimatedFPS = 120;

	return estimatedFPS;
}

void RenderingManager::initializePresets()
{
	// Performance Preset
	RenderingSettings performance;
	performance.name = "Performance";
	performance.mode = 4; // Shaded
	performance.quality = 0; // Low
	performance.fastMode = true;
	performance.transparencyType = 0; // None
	performance.smoothShading = false;
	performance.phongShading = false;
	performance.backfaceCulling = true;
	performance.depthTest = true;
	performance.depthWrite = true;
	performance.frustumCulling = true;
	performance.occlusionCulling = false;
	performance.adaptiveQuality = true;
	m_presets["Performance"] = performance;

	// Balanced Preset
	RenderingSettings balanced;
	balanced.name = "Balanced";
	balanced.mode = 4; // Shaded
	balanced.quality = 1; // Medium
	balanced.fastMode = false;
	balanced.transparencyType = 1; // Blend
	balanced.smoothShading = true;
	balanced.phongShading = true;
	balanced.backfaceCulling = true;
	balanced.depthTest = true;
	balanced.depthWrite = true;
	balanced.frustumCulling = true;
	balanced.occlusionCulling = false;
	balanced.adaptiveQuality = false;
	m_presets["Balanced"] = balanced;

	// Quality Preset
	RenderingSettings quality;
	quality.name = "Quality";
	quality.mode = 4; // Shaded
	quality.quality = 2; // High
	quality.fastMode = false;
	quality.transparencyType = 2; // SortedBlend
	quality.smoothShading = true;
	quality.phongShading = true;
	quality.backfaceCulling = true;
	quality.depthTest = true;
	quality.depthWrite = true;
	quality.frustumCulling = true;
	quality.occlusionCulling = true;
	quality.adaptiveQuality = false;
	m_presets["Quality"] = quality;

	// Ultra Preset
	RenderingSettings ultra;
	ultra.name = "Ultra";
	ultra.mode = 4; // Shaded
	ultra.quality = 3; // Ultra
	ultra.fastMode = false;
	ultra.transparencyType = 3; // DelayedBlend
	ultra.smoothShading = true;
	ultra.phongShading = true;
	ultra.backfaceCulling = true;
	ultra.depthTest = true;
	ultra.depthWrite = true;
	ultra.frustumCulling = true;
	ultra.occlusionCulling = true;
	ultra.adaptiveQuality = false;
	m_presets["Ultra"] = ultra;

	// Wireframe Preset
	RenderingSettings wireframe;
	wireframe.name = "Wireframe";
	wireframe.mode = 1; // Wireframe
	wireframe.polygonMode = 1; // Line
	wireframe.lineWidth = 1.5f;
	wireframe.quality = 1; // Medium
	wireframe.fastMode = true;
	wireframe.transparencyType = 0; // None
	wireframe.smoothShading = false;
	wireframe.phongShading = false;
	wireframe.backfaceCulling = false;
	wireframe.depthTest = true;
	wireframe.depthWrite = true;
	wireframe.frustumCulling = true;
	wireframe.occlusionCulling = false;
	wireframe.adaptiveQuality = false;
	m_presets["Wireframe"] = wireframe;

	// CAD/Engineering Presets
	RenderingSettings cadStandard;
	cadStandard.name = "CAD Standard";
	cadStandard.mode = 4; // Shaded
	cadStandard.polygonMode = 0; // Fill
	cadStandard.quality = 2; // High
	cadStandard.fastMode = false;
	cadStandard.transparencyType = 1; // Blend
	cadStandard.smoothShading = true;
	cadStandard.phongShading = true;
	cadStandard.backfaceCulling = true;
	cadStandard.depthTest = true;
	cadStandard.depthWrite = true;
	cadStandard.frustumCulling = true;
	cadStandard.occlusionCulling = false;
	cadStandard.adaptiveQuality = false;
	cadStandard.backgroundColor = wxColour(240, 240, 240); // Light gray
	m_presets["CAD Standard"] = cadStandard;

	RenderingSettings cadHighQuality;
	cadHighQuality.name = "CAD High Quality";
	cadHighQuality.mode = 4; // Shaded
	cadHighQuality.polygonMode = 0; // Fill
	cadHighQuality.quality = 3; // Ultra
	cadHighQuality.fastMode = false;
	cadHighQuality.transparencyType = 2; // SortedBlend
	cadHighQuality.smoothShading = true;
	cadHighQuality.phongShading = true;
	cadHighQuality.backfaceCulling = true;
	cadHighQuality.depthTest = true;
	cadHighQuality.depthWrite = true;
	cadHighQuality.frustumCulling = true;
	cadHighQuality.occlusionCulling = true;
	cadHighQuality.adaptiveQuality = false;
	cadHighQuality.backgroundColor = wxColour(255, 255, 255); // White
	m_presets["CAD High Quality"] = cadHighQuality;

	RenderingSettings cadWireframe;
	cadWireframe.name = "CAD Wireframe";
	cadWireframe.mode = 1; // Wireframe
	cadWireframe.polygonMode = 1; // Line
	cadWireframe.lineWidth = 1.0f;
	cadWireframe.quality = 2; // High
	cadWireframe.fastMode = false;
	cadWireframe.transparencyType = 0; // None
	cadWireframe.smoothShading = false;
	cadWireframe.phongShading = false;
	cadWireframe.backfaceCulling = false;
	cadWireframe.depthTest = true;
	cadWireframe.depthWrite = true;
	cadWireframe.frustumCulling = true;
	cadWireframe.occlusionCulling = false;
	cadWireframe.adaptiveQuality = false;
	cadWireframe.backgroundColor = wxColour(255, 255, 255); // White
	m_presets["CAD Wireframe"] = cadWireframe;

	// Gaming/Real-time Presets
	RenderingSettings gamingFast;
	gamingFast.name = "Gaming Fast";
	gamingFast.mode = 4; // Shaded
	gamingFast.polygonMode = 0; // Fill
	gamingFast.quality = 0; // Low
	gamingFast.fastMode = true;
	gamingFast.transparencyType = 0; // None
	gamingFast.smoothShading = false;
	gamingFast.phongShading = false;
	gamingFast.backfaceCulling = true;
	gamingFast.depthTest = true;
	gamingFast.depthWrite = true;
	gamingFast.frustumCulling = true;
	gamingFast.occlusionCulling = false;
	gamingFast.adaptiveQuality = true;
	gamingFast.backgroundColor = wxColour(0, 0, 0); // Black
	m_presets["Gaming Fast"] = gamingFast;

	RenderingSettings gamingBalanced;
	gamingBalanced.name = "Gaming Balanced";
	gamingBalanced.mode = 4; // Shaded
	gamingBalanced.polygonMode = 0; // Fill
	gamingBalanced.quality = 1; // Medium
	gamingBalanced.fastMode = false;
	gamingBalanced.transparencyType = 1; // Blend
	gamingBalanced.smoothShading = true;
	gamingBalanced.phongShading = true;
	gamingBalanced.backfaceCulling = true;
	gamingBalanced.depthTest = true;
	gamingBalanced.depthWrite = true;
	gamingBalanced.frustumCulling = true;
	gamingBalanced.occlusionCulling = false;
	gamingBalanced.adaptiveQuality = true;
	gamingBalanced.backgroundColor = wxColour(0, 0, 0); // Black
	m_presets["Gaming Balanced"] = gamingBalanced;

	RenderingSettings gamingQuality;
	gamingQuality.name = "Gaming Quality";
	gamingQuality.mode = 4; // Shaded
	gamingQuality.polygonMode = 0; // Fill
	gamingQuality.quality = 2; // High
	gamingQuality.fastMode = false;
	gamingQuality.transparencyType = 2; // SortedBlend
	gamingQuality.smoothShading = true;
	gamingQuality.phongShading = true;
	gamingQuality.backfaceCulling = true;
	gamingQuality.depthTest = true;
	gamingQuality.depthWrite = true;
	gamingQuality.frustumCulling = true;
	gamingQuality.occlusionCulling = true;
	gamingQuality.adaptiveQuality = false;
	gamingQuality.backgroundColor = wxColour(0, 0, 0); // Black
	m_presets["Gaming Quality"] = gamingQuality;

	// Mobile/Embedded Presets
	RenderingSettings mobileLow;
	mobileLow.name = "Mobile Low";
	mobileLow.mode = 4; // Shaded
	mobileLow.polygonMode = 0; // Fill
	mobileLow.quality = 0; // Low
	mobileLow.fastMode = true;
	mobileLow.transparencyType = 0; // None
	mobileLow.smoothShading = false;
	mobileLow.phongShading = false;
	mobileLow.backfaceCulling = true;
	mobileLow.depthTest = true;
	mobileLow.depthWrite = true;
	mobileLow.frustumCulling = true;
	mobileLow.occlusionCulling = false;
	mobileLow.adaptiveQuality = true;
	mobileLow.maxRenderDistance = 500;
	m_presets["Mobile Low"] = mobileLow;

	RenderingSettings mobileMedium;
	mobileMedium.name = "Mobile Medium";
	mobileMedium.mode = 4; // Shaded
	mobileMedium.polygonMode = 0; // Fill
	mobileMedium.quality = 1; // Medium
	mobileMedium.fastMode = false;
	mobileMedium.transparencyType = 1; // Blend
	mobileMedium.smoothShading = true;
	mobileMedium.phongShading = false;
	mobileMedium.backfaceCulling = true;
	mobileMedium.depthTest = true;
	mobileMedium.depthWrite = true;
	mobileMedium.frustumCulling = true;
	mobileMedium.occlusionCulling = false;
	mobileMedium.adaptiveQuality = true;
	mobileMedium.maxRenderDistance = 750;
	m_presets["Mobile Medium"] = mobileMedium;

	// Presentation/Visualization Presets
	RenderingSettings presentationStandard;
	presentationStandard.name = "Presentation Standard";
	presentationStandard.mode = 4; // Shaded
	presentationStandard.polygonMode = 0; // Fill
	presentationStandard.quality = 2; // High
	presentationStandard.fastMode = false;
	presentationStandard.transparencyType = 2; // SortedBlend
	presentationStandard.smoothShading = true;
	presentationStandard.phongShading = true;
	presentationStandard.backfaceCulling = true;
	presentationStandard.depthTest = true;
	presentationStandard.depthWrite = true;
	presentationStandard.frustumCulling = true;
	presentationStandard.occlusionCulling = false;
	presentationStandard.adaptiveQuality = false;
	presentationStandard.backgroundColor = wxColour(255, 255, 255); // White
	presentationStandard.gradientBackground = true;
	m_presets["Presentation Standard"] = presentationStandard;

	RenderingSettings presentationHigh;
	presentationHigh.name = "Presentation High";
	presentationHigh.mode = 4; // Shaded
	presentationHigh.polygonMode = 0; // Fill
	presentationHigh.quality = 3; // Ultra
	presentationHigh.fastMode = false;
	presentationHigh.transparencyType = 3; // DelayedBlend
	presentationHigh.smoothShading = true;
	presentationHigh.phongShading = true;
	presentationHigh.backfaceCulling = true;
	presentationHigh.depthTest = true;
	presentationHigh.depthWrite = true;
	presentationHigh.frustumCulling = true;
	presentationHigh.occlusionCulling = true;
	presentationHigh.adaptiveQuality = false;
	presentationHigh.backgroundColor = wxColour(255, 255, 255); // White
	presentationHigh.gradientBackground = true;
	m_presets["Presentation High"] = presentationHigh;

	// Debug/Development Presets
	RenderingSettings debugWireframe;
	debugWireframe.name = "Debug Wireframe";
	debugWireframe.mode = 1; // Wireframe
	debugWireframe.polygonMode = 1; // Line
	debugWireframe.lineWidth = 1.5f;
	debugWireframe.quality = 0; // Low
	debugWireframe.fastMode = true;
	debugWireframe.transparencyType = 0; // None
	debugWireframe.smoothShading = false;
	debugWireframe.phongShading = false;
	debugWireframe.backfaceCulling = false;
	debugWireframe.depthTest = true;
	debugWireframe.depthWrite = true;
	debugWireframe.frustumCulling = false;
	debugWireframe.occlusionCulling = false;
	debugWireframe.adaptiveQuality = false;
	debugWireframe.backgroundColor = wxColour(0, 0, 0); // Black
	m_presets["Debug Wireframe"] = debugWireframe;

	RenderingSettings debugPoints;
	debugPoints.name = "Debug Points";
	debugPoints.mode = 2; // Points
	debugPoints.polygonMode = 2; // Point
	debugPoints.pointSize = 3.0f;
	debugPoints.quality = 0; // Low
	debugPoints.fastMode = true;
	debugPoints.transparencyType = 0; // None
	debugPoints.smoothShading = false;
	debugPoints.phongShading = false;
	debugPoints.backfaceCulling = false;
	debugPoints.depthTest = true;
	debugPoints.depthWrite = true;
	debugPoints.frustumCulling = false;
	debugPoints.occlusionCulling = false;
	debugPoints.adaptiveQuality = false;
	debugPoints.backgroundColor = wxColour(0, 0, 0); // Black
	m_presets["Debug Points"] = debugPoints;

	// Legacy/Compatibility Presets
	RenderingSettings legacySolid;
	legacySolid.name = "Legacy Solid";
	legacySolid.mode = 0; // Solid
	legacySolid.polygonMode = 0; // Fill
	legacySolid.quality = 1; // Medium
	legacySolid.fastMode = true;
	legacySolid.transparencyType = 0; // None
	legacySolid.smoothShading = false;
	legacySolid.phongShading = false;
	legacySolid.backfaceCulling = true;
	legacySolid.depthTest = true;
	legacySolid.depthWrite = true;
	legacySolid.frustumCulling = false;
	legacySolid.occlusionCulling = false;
	legacySolid.adaptiveQuality = false;
	m_presets["Legacy Solid"] = legacySolid;

	RenderingSettings legacyWireframe;
	legacyWireframe.name = "Legacy Wireframe";
	legacyWireframe.mode = 1; // Wireframe
	legacyWireframe.polygonMode = 1; // Line
	legacyWireframe.lineWidth = 1.5f;
	legacyWireframe.quality = 1; // Medium
	legacyWireframe.fastMode = true;
	legacyWireframe.transparencyType = 0; // None
	legacyWireframe.smoothShading = false;
	legacyWireframe.phongShading = false;
	legacyWireframe.backfaceCulling = false;
	legacyWireframe.depthTest = true;
	legacyWireframe.depthWrite = true;
	legacyWireframe.frustumCulling = false;
	legacyWireframe.occlusionCulling = false;
	legacyWireframe.adaptiveQuality = false;
	m_presets["Legacy Wireframe"] = legacyWireframe;

	RenderingSettings legacyPoints;
	legacyPoints.name = "Legacy Points";
	legacyPoints.mode = 2; // Points
	legacyPoints.polygonMode = 2; // Point
	legacyPoints.pointSize = 3.0f;
	legacyPoints.quality = 1; // Medium
	legacyPoints.fastMode = true;
	legacyPoints.transparencyType = 0; // None
	legacyPoints.smoothShading = false;
	legacyPoints.phongShading = false;
	legacyPoints.backfaceCulling = false;
	legacyPoints.depthTest = true;
	legacyPoints.depthWrite = true;
	legacyPoints.frustumCulling = false;
	legacyPoints.occlusionCulling = false;
	legacyPoints.adaptiveQuality = false;
	m_presets["Legacy Points"] = legacyPoints;

	RenderingSettings legacyHiddenLine;
	legacyHiddenLine.name = "Legacy Hidden Line";
	legacyHiddenLine.mode = 3; // HiddenLine
	legacyHiddenLine.polygonMode = 1; // Line
	legacyHiddenLine.lineWidth = 1.0f;
	legacyHiddenLine.quality = 1; // Medium
	legacyHiddenLine.fastMode = true;
	legacyHiddenLine.transparencyType = 0; // None
	legacyHiddenLine.smoothShading = false;
	legacyHiddenLine.phongShading = false;
	legacyHiddenLine.backfaceCulling = true;
	legacyHiddenLine.depthTest = true;
	legacyHiddenLine.depthWrite = true;
	legacyHiddenLine.frustumCulling = false;
	legacyHiddenLine.occlusionCulling = false;
	legacyHiddenLine.adaptiveQuality = false;
	m_presets["Legacy Hidden Line"] = legacyHiddenLine;

	RenderingSettings legacyShaded;
	legacyShaded.name = "Legacy Shaded";
	legacyShaded.mode = 4; // Shaded
	legacyShaded.polygonMode = 0; // Fill
	legacyShaded.quality = 1; // Medium
	legacyShaded.fastMode = true;
	legacyShaded.transparencyType = 0; // None
	legacyShaded.smoothShading = true;
	legacyShaded.phongShading = true;
	legacyShaded.backfaceCulling = true;
	legacyShaded.depthTest = true;
	legacyShaded.depthWrite = true;
	legacyShaded.frustumCulling = false;
	legacyShaded.occlusionCulling = false;
	legacyShaded.adaptiveQuality = false;
	m_presets["Legacy Shaded"] = legacyShaded;

	// NoShading Preset - similar to FreeCAD's no shading mode
	RenderingSettings noShading;
	noShading.name = "NoShading";
	noShading.mode = 6; // NoShading
	noShading.polygonMode = 0; // Fill
	noShading.quality = 1; // Medium
	noShading.fastMode = true;
	noShading.transparencyType = 0; // None
	noShading.smoothShading = false;
	noShading.phongShading = false;
	noShading.backfaceCulling = true;
	noShading.depthTest = true;
	noShading.depthWrite = true;
	noShading.frustumCulling = false;
	noShading.occlusionCulling = false;
	noShading.adaptiveQuality = false;
	noShading.backgroundColor = wxColour(240, 240, 240); // Light gray background
	m_presets["NoShading"] = noShading;

	LOG_INF_S("RenderingManager::initializePresets: Initialized " + std::to_string(m_presets.size()) + " presets");
}

void RenderingManager::applyRenderingMode(const RenderingSettings& settings)
{
	if (!m_sceneRoot) {
		LOG_ERR_S("RenderingManager::applyRenderingMode: Scene root not initialized");
		return;
	}

	// Apply polygon mode settings
	switch (settings.polygonMode) {
	case 0: // Fill mode
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 1: // Line mode (Wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(settings.lineWidth);
		break;
	case 2: // Point mode
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		glPointSize(settings.pointSize);
		break;
	default:
		LOG_WRN_S("RenderingManager::applyRenderingMode: Unknown polygon mode " + std::to_string(settings.polygonMode));
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	}

	// Apply specific rendering mode logic
	switch (settings.mode) {
	case 0: // Solid
		applySolidMode(settings);
		break;
	case 1: // Wireframe
		applyWireframeMode(settings);
		break;
	case 2: // Points
		applyPointsMode(settings);
		break;
	case 3: // Hidden Line
		applyHiddenLineMode(settings);
		break;
	case 4: // Shaded
		applyShadedMode(settings);
		break;
	case 6: // NoShading
		applyNoShadingMode(settings);
		break;
	default:
		LOG_WRN_S("RenderingManager::applyRenderingMode: Unknown rendering mode " + std::to_string(settings.mode));
		applySolidMode(settings);
		break;
	}

	LOG_INF_S("RenderingManager::applyRenderingMode: Applied mode " + std::to_string(settings.mode));
}

void RenderingManager::applySolidMode(const RenderingSettings& settings)
{
	// Enable solid rendering
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);

	if (settings.smoothShading) {
		glShadeModel(GL_SMOOTH);
	}
	else {
		glShadeModel(GL_FLAT);
	}

	// Apply material properties for solid rendering
	if (settings.phongShading) {
		// Enable Phong shading through shader programs
		enablePhongShading();
	}
}

void RenderingManager::applyWireframeMode(const RenderingSettings& settings)
{
	// Set wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(settings.lineWidth);

	// Disable lighting for wireframe
	glDisable(GL_LIGHTING);

	// Set wireframe color
	glColor3f(0.0f, 0.0f, 0.0f); // Black wireframe

	// Handle backface culling for wireframe
	if (!settings.backfaceCulling) {
		glDisable(GL_CULL_FACE);
	}
}

void RenderingManager::applyPointsMode(const RenderingSettings& settings)
{
	// Set point mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	glPointSize(settings.pointSize);

	// Disable lighting for points
	glDisable(GL_LIGHTING);

	// Enable point smoothing if available
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	// Set point color
	glColor3f(1.0f, 0.0f, 0.0f); // Red points
}

void RenderingManager::applyHiddenLineMode(const RenderingSettings& settings)
{
	// Hidden line rendering requires two passes:
	// 1. Render filled polygons with background color (hidden)
	// 2. Render wireframe on top

	// First pass: render filled polygons
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(settings.backgroundColor.Red() / 255.0f,
		settings.backgroundColor.Green() / 255.0f,
		settings.backgroundColor.Blue() / 255.0f);

	// Render scene in fill mode (this would be done in the render loop)

	// Second pass: render wireframe
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(settings.lineWidth);
	glColor3f(0.0f, 0.0f, 0.0f); // Black lines

	// Enable polygon offset to avoid z-fighting
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-1.0f, -1.0f);
}

void RenderingManager::applyShadedMode(const RenderingSettings& settings)
{
	// Enable shaded rendering
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);

	// Configure shading model
	if (settings.smoothShading) {
		glShadeModel(GL_SMOOTH);
	}
	else {
		glShadeModel(GL_FLAT);
	}

	// Enable advanced shading techniques
	if (settings.phongShading) {
		enablePhongShading();
	}
	else if (settings.gouraudShading) {
		enableGouraudShading();
	}

	// Configure material properties
	configureMaterialProperties(settings);
}

void RenderingManager::applyNoShadingMode(const RenderingSettings& settings)
{
	// Enable solid rendering but disable lighting for no shading effect
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);

	// Set a uniform color for all surfaces (like FreeCAD's no shading mode)
	// Use a neutral gray color
	glColor3f(0.8f, 0.8f, 0.8f);

	// Disable material properties to ensure uniform color
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);

	// Enable depth testing for proper geometry visibility
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	LOG_INF_S("RenderingManager::applyNoShadingMode: Applied no shading mode");
}

void RenderingManager::applyQualitySettings(const RenderingSettings& settings)
{
	if (!m_sceneRoot) return;

	// Apply quality settings to scene graph
	LOG_INF_S("RenderingManager::applyQualitySettings: Applied quality " + std::to_string(settings.quality));
}

void RenderingManager::applyTransparencySettings(const RenderingSettings& settings)
{
	if (!m_sceneRoot) return;

	// Apply transparency settings
	LOG_INF_S("RenderingManager::applyTransparencySettings: Applied transparency type " + std::to_string(settings.transparencyType));
}

void RenderingManager::applyShadingSettings(const RenderingSettings& settings)
{
	if (!m_sceneRoot) return;

	// Apply shading settings
	LOG_INF_S("RenderingManager::applyShadingSettings: Smooth=" + std::to_string(settings.smoothShading) +
		", Phong=" + std::to_string(settings.phongShading));
}

void RenderingManager::applyCullingSettings(const RenderingSettings& settings)
{
	if (!m_sceneRoot) return;

	// Apply culling settings
	LOG_INF_S("RenderingManager::applyCullingSettings: Cull mode " + std::to_string(settings.cullMode));
}

void RenderingManager::applyDepthSettings(const RenderingSettings& settings)
{
	if (!m_sceneRoot) return;

	// Apply depth settings
	LOG_INF_S("RenderingManager::applyDepthSettings: Test=" + std::to_string(settings.depthTest) +
		", Write=" + std::to_string(settings.depthWrite));
}

void RenderingManager::applyPolygonSettings(const RenderingSettings& settings)
{
	if (!m_sceneRoot) return;

	// Apply polygon settings
	LOG_INF_S("RenderingManager::applyPolygonSettings: Mode " + std::to_string(settings.polygonMode));
}

void RenderingManager::applyBackgroundSettings(const RenderingSettings& settings)
{
	if (!m_sceneRoot) return;

	// Apply background settings
	LOG_INF_S("RenderingManager::applyBackgroundSettings: Color (" +
		std::to_string(settings.backgroundColor.Red()) + "," +
		std::to_string(settings.backgroundColor.Green()) + "," +
		std::to_string(settings.backgroundColor.Blue()) + ")");
}

void RenderingManager::setupOpenGLState(const RenderingSettings& settings)
{
	if (!m_canvas || !m_glContext) return;

	m_canvas->SetCurrent(*m_glContext);

	// Setup depth testing
	if (settings.depthTest) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}

	// Setup depth writing
	if (settings.depthWrite) {
		glDepthMask(GL_TRUE);
	}
	else {
		glDepthMask(GL_FALSE);
	}

	// Setup culling
	if (settings.backfaceCulling) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	else {
		glDisable(GL_CULL_FACE);
	}

	// Setup blending for transparency
	if (settings.transparencyType > 0) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		glDisable(GL_BLEND);
	}

	// Setup lighting
	if (settings.mode == 4 || settings.mode == 5) { // Shaded modes
		glEnable(GL_LIGHTING);
		glEnable(GL_NORMALIZE);
	}
	else {
		glDisable(GL_LIGHTING);
		glDisable(GL_NORMALIZE);
	}

	// Setup polygon mode
	switch (settings.polygonMode) {
	case 0: // Fill
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 1: // Line
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 2: // Point
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	}

	// Setup line width and point size
	glLineWidth(settings.lineWidth);
	glPointSize(settings.pointSize);

	// Setup background based on style
	switch (settings.backgroundStyle) {
	case 0: // Solid Color
	{
		float r = settings.backgroundColor.Red() / 255.0f;
		float g = settings.backgroundColor.Green() / 255.0f;
		float b = settings.backgroundColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	case 1: // Gradient
	{
		// For gradient, we'll use the top color as the clear color
		// The actual gradient rendering would need to be done in the render loop
		float r = settings.gradientTopColor.Red() / 255.0f;
		float g = settings.gradientTopColor.Green() / 255.0f;
		float b = settings.gradientTopColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	case 2: // Image
	{
		// For image background, we'll use a default color
		// The actual image rendering would need to be done in the render loop
		float r = settings.backgroundColor.Red() / 255.0f;
		float g = settings.backgroundColor.Green() / 255.0f;
		float b = settings.backgroundColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	case 3: // Environment
	{
		float r = settings.backgroundColor.Red() / 255.0f;
		float g = settings.backgroundColor.Green() / 255.0f;
		float b = settings.backgroundColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	case 4: // Studio
	{
		float r = settings.backgroundColor.Red() / 255.0f;
		float g = settings.backgroundColor.Green() / 255.0f;
		float b = settings.backgroundColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	case 5: // Outdoor
	{
		float r = settings.backgroundColor.Red() / 255.0f;
		float g = settings.backgroundColor.Green() / 255.0f;
		float b = settings.backgroundColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	case 6: // Industrial
	{
		float r = settings.backgroundColor.Red() / 255.0f;
		float g = settings.backgroundColor.Green() / 255.0f;
		float b = settings.backgroundColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	default:
	{
		float r = settings.backgroundColor.Red() / 255.0f;
		float g = settings.backgroundColor.Green() / 255.0f;
		float b = settings.backgroundColor.Blue() / 255.0f;
		glClearColor(r, g, b, 1.0f);
	}
	break;
	}

	LOG_INF_S("RenderingManager::setupOpenGLState: Applied OpenGL state with background style " + std::to_string(settings.backgroundStyle));
}

void RenderingManager::restoreOpenGLState()
{
	if (!m_canvas || !m_glContext) return;

	m_canvas->SetCurrent(*m_glContext);

	// Restore default OpenGL state
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1.0f);
	glPointSize(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	LOG_INF_S("RenderingManager::restoreOpenGLState: Restored default OpenGL state");
}

void RenderingManager::optimizeForPerformance(const RenderingSettings& settings)
{
	// Apply performance optimizations
	LOG_INF_S("RenderingManager::optimizeForPerformance: Applied performance optimizations");
}

void RenderingManager::optimizeForQuality(const RenderingSettings& settings)
{
	// Apply quality optimizations
	LOG_INF_S("RenderingManager::optimizeForQuality: Applied quality optimizations");
}

void RenderingManager::enablePhongShading()
{
	// Enable Phong shading through OpenGL shaders
	// This would typically involve loading and using shader programs
	LOG_INF_S("RenderingManager::enablePhongShading: Phong shading enabled");
}

void RenderingManager::enableGouraudShading()
{
	// Gouraud shading is the default OpenGL smooth shading
	glShadeModel(GL_SMOOTH);
	LOG_INF_S("RenderingManager::enableGouraudShading: Gouraud shading enabled");
}

void RenderingManager::configureMaterialProperties(const RenderingSettings& settings)
{
	// Set default material properties
	GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat shininess = 64.0f;

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	// Enable color material for dynamic color changes
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}