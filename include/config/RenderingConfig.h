#pragma once

#include <OpenCASCADE/Quantity_Color.hxx>
#include <string>
#include <map>
#include <vector>

// Forward declaration
class OCCViewer;

class RenderingConfig
{
public:
	struct MaterialSettings {
		Quantity_Color ambientColor;
		Quantity_Color diffuseColor;
		Quantity_Color specularColor;
		double shininess;
		double transparency;

		MaterialSettings()
			: ambientColor(0.6, 0.6, 0.6, Quantity_TOC_RGB)  // Increased ambient for better consistency
			, diffuseColor(0.8, 0.8, 0.8, Quantity_TOC_RGB)  // Slightly reduced diffuse
			, specularColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
			, shininess(30.0)
			, transparency(0.0)
		{
		}

		MaterialSettings(const Quantity_Color& ambient, const Quantity_Color& diffuse,
			const Quantity_Color& specular, double shine, double trans)
			: ambientColor(ambient)
			, diffuseColor(diffuse)
			, specularColor(specular)
			, shininess(shine)
			, transparency(trans)
		{
		}
	};

	struct LightingSettings {
		Quantity_Color ambientColor;
		Quantity_Color diffuseColor;
		Quantity_Color specularColor;
		double intensity;
		double ambientIntensity;

		LightingSettings()
			: ambientColor(0.7, 0.7, 0.7, Quantity_TOC_RGB)  // Increased ambient light
			, diffuseColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
			, specularColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
			, intensity(1.0)
			, ambientIntensity(0.8)  // Increased ambient intensity
		{
		}
	};

	// Texture mode enumeration
	// Note: Coin3D SoTexture2 only supports DECAL and MODULATE modes
	// Other modes will fall back to MODULATE
	enum class TextureMode {
		Replace,        // Replace base color with texture (falls back to Modulate in Coin3D)
		Modulate,       // Multiply texture with base color (Coin3D supported)
		Decal,          // Apply texture as decal over base color (Coin3D supported)
		Blend           // Blend texture with base color (falls back to Modulate in Coin3D)
	};

	struct TextureSettings {
		Quantity_Color color;
		double intensity;
		bool enabled;
		std::string imagePath;  // Path to texture image file
		TextureMode textureMode;

		TextureSettings()
			: color(1.0, 1.0, 1.0, Quantity_TOC_RGB)
			, intensity(0.5)
			, enabled(false)
			, imagePath("")
			, textureMode(TextureMode::Modulate)
		{
		}
	};

	// Blend mode enumeration
	enum class BlendMode {
		None,           // No blending
		Alpha,          // Standard alpha blending
		Additive,       // Additive blending
		Multiply,       // Multiplicative blending
		Screen,         // Screen blending
		Overlay         // Overlay blending
	};

	struct BlendSettings {
		BlendMode blendMode;
		bool depthTest;
		bool depthWrite;
		bool cullFace;
		double alphaThreshold;

		BlendSettings()
			: blendMode(BlendMode::None)
			, depthTest(true)
			, depthWrite(true)
			, cullFace(true)
			, alphaThreshold(0.1)
		{
		}
	};

	// Shading mode enumeration
	enum class ShadingMode {
		Flat,           // Flat shading - single color per face
		Gouraud,        // Gouraud shading - vertex interpolation
		Phong,          // Phong shading - pixel-level interpolation
		Smooth,         // Smooth shading
		Wireframe,      // Wireframe mode
		Points          // Points mode
	};

	struct ShadingSettings {
		ShadingMode shadingMode;
		bool smoothNormals;
		double wireframeWidth;
		double pointSize;
		
		// Normal consistency settings
		bool enableNormalConsistency;
		bool autoFixNormals;
		bool showNormalDebug;
		double normalConsistencyThreshold;

		ShadingSettings()
			: shadingMode(ShadingMode::Smooth)
			, smoothNormals(true)
			, wireframeWidth(1.0)
			, pointSize(2.0)
			, enableNormalConsistency(true)
			, autoFixNormals(true)
			, showNormalDebug(false)
			, normalConsistencyThreshold(0.1)
		{
		}
	};

	// Display mode enumeration
	enum class DisplayMode {
		Solid,          // Solid display
		Wireframe,      // Wireframe display
		HiddenLine,     // Hidden line display
		SolidWireframe, // Solid + wireframe combination
		Points,         // Points display
		Transparent     // Transparent display
	};

	struct DisplaySettings {
		DisplayMode displayMode;
		bool showEdges;
		bool showVertices;
		double edgeWidth;
		double vertexSize;
		Quantity_Color edgeColor;
		Quantity_Color vertexColor;

		DisplaySettings()
			: displayMode(DisplayMode::Solid)
			, showEdges(false)
			, showVertices(false)
			, edgeWidth(1.0)
			, vertexSize(2.0)
			, edgeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
			, vertexColor(1.0, 0.0, 0.0, Quantity_TOC_RGB)
		{
		}
	};

	// Rendering quality enumeration
	enum class RenderingQuality {
		Draft,          // Draft quality - fast rendering
		Normal,         // Normal quality
		High,           // High quality
		Ultra,          // Ultra quality
		Realtime        // Real-time rendering
	};

	struct QualitySettings {
		RenderingQuality quality;
		int tessellationLevel;
		int antiAliasingSamples;
		bool enableLOD;
		double lodDistance;

		QualitySettings()
			: quality(RenderingQuality::Normal)
			, tessellationLevel(2)
			, antiAliasingSamples(4)
			, enableLOD(true)
			, lodDistance(100.0)
		{
		}
	};

	// Shadow mode enumeration
	enum class ShadowMode {
		None,           // No shadows
		Hard,           // Hard shadows
		Soft,           // Soft shadows
		Volumetric,     // Volumetric shadows
		Contact,        // Contact shadows
		Cascade         // Cascade shadows
	};

	struct ShadowSettings {
		ShadowMode shadowMode;
		double shadowIntensity;
		double shadowSoftness;
		int shadowMapSize;
		double shadowBias;

		ShadowSettings()
			: shadowMode(ShadowMode::Soft)
			, shadowIntensity(0.7)
			, shadowSoftness(0.5)
			, shadowMapSize(1024)
			, shadowBias(0.001)
		{
		}
	};

	// Lighting model enumeration
	enum class LightingModel {
		Lambert,        // Lambert diffuse reflection
		BlinnPhong,     // Blinn-Phong model
		CookTorrance,   // Cook-Torrance microfacet model
		OrenNayar,      // Oren-Nayar rough surface model
		Minnaert,       // Minnaert lunar surface model
		Fresnel         // Fresnel reflection model
	};

	struct LightingModelSettings {
		LightingModel lightingModel;
		double roughness;
		double metallic;
		double fresnel;
		double subsurfaceScattering;

		LightingModelSettings()
			: lightingModel(LightingModel::BlinnPhong)
			, roughness(0.5)
			, metallic(0.0)
			, fresnel(0.04)
			, subsurfaceScattering(0.0)
		{
		}
	};

	// Material preset types
	enum class MaterialPreset {
		Custom,
		Glass,
		Metal,
		Plastic,
		Wood,
		Ceramic,
		Rubber,
		Chrome,
		Gold,
		Silver,
		Copper,
		Aluminum
	};

public:
	static RenderingConfig& getInstance();

	// Load/Save configuration
	bool loadFromFile(const std::string& filename = "");
	bool saveToFile(const std::string& filename = "") const;

	// Getters
	const MaterialSettings& getMaterialSettings() const { return m_materialSettings; }
	const LightingSettings& getLightingSettings() const { return m_lightingSettings; }
	const TextureSettings& getTextureSettings() const { return m_textureSettings; }
	const BlendSettings& getBlendSettings() const { return m_blendSettings; }
	const ShadingSettings& getShadingSettings() const { return m_shadingSettings; }
	const DisplaySettings& getDisplaySettings() const { return m_displaySettings; }
	const QualitySettings& getQualitySettings() const { return m_qualitySettings; }
	const ShadowSettings& getShadowSettings() const { return m_shadowSettings; }
	const LightingModelSettings& getLightingModelSettings() const { return m_lightingModelSettings; }

	// Setters
	void setMaterialSettings(const MaterialSettings& settings);
	void setLightingSettings(const LightingSettings& settings);
	void setTextureSettings(const TextureSettings& settings);
	void setBlendSettings(const BlendSettings& settings);
	void setShadingSettings(const ShadingSettings& settings);
	void setDisplaySettings(const DisplaySettings& settings);
	void setQualitySettings(const QualitySettings& settings);
	void setShadowSettings(const ShadowSettings& settings);
	void setLightingModelSettings(const LightingModelSettings& settings);

	// Individual property setters
	void setMaterialAmbientColor(const Quantity_Color& color);
	void setMaterialDiffuseColor(const Quantity_Color& color);
	void setMaterialSpecularColor(const Quantity_Color& color);
	void setMaterialShininess(double shininess);
	void setMaterialTransparency(double transparency);

	void setLightAmbientColor(const Quantity_Color& color);
	void setLightDiffuseColor(const Quantity_Color& color);
	void setLightSpecularColor(const Quantity_Color& color);
	void setLightIntensity(double intensity);
	void setLightAmbientIntensity(double intensity);

	void setTextureColor(const Quantity_Color& color);
	void setTextureIntensity(double intensity);
	void setTextureEnabled(bool enabled);
	void setTextureImagePath(const std::string& path);
	void setTextureMode(TextureMode mode);

	void setBlendMode(BlendMode mode);
	void setDepthTest(bool enabled);
	void setDepthWrite(bool enabled);
	void setCullFace(bool enabled);
	void setAlphaThreshold(double threshold);

	// Blend mode utility methods
	static std::vector<std::string> getAvailableBlendModes();
	static std::string getBlendModeName(BlendMode mode);
	static BlendMode getBlendModeFromName(const std::string& name);

	// Texture mode utility methods
	static std::vector<std::string> getAvailableTextureModes();
	static std::string getTextureModeName(TextureMode mode);
	static TextureMode getTextureModeFromName(const std::string& name);

	// Shading mode individual setters
	void setShadingMode(ShadingMode mode);
	void setSmoothNormals(bool enabled);
	void setWireframeWidth(double width);
	void setPointSize(double size);

	// Display mode individual setters
	void setDisplayMode(DisplayMode mode);
	void setShowEdges(bool enabled);
	void setShowVertices(bool enabled);
	void setEdgeWidth(double width);
	void setVertexSize(double size);
	void setEdgeColor(const Quantity_Color& color);
	void setVertexColor(const Quantity_Color& color);

	// Quality individual setters
	void setRenderingQuality(RenderingQuality quality);
	void setTessellationLevel(int level);
	void setAntiAliasingSamples(int samples);
	void setEnableLOD(bool enabled);
	void setLODDistance(double distance);

	// Shadow individual setters
	void setShadowMode(ShadowMode mode);
	void setShadowIntensity(double intensity);
	void setShadowSoftness(double softness);
	void setShadowMapSize(int size);
	void setShadowBias(double bias);

	// Lighting model individual setters
	void setLightingModel(LightingModel model);
	void setRoughness(double roughness);
	void setMetallic(double metallic);
	void setFresnel(double fresnel);
	void setSubsurfaceScattering(double scattering);

	// Shading mode utility methods
	static std::vector<std::string> getAvailableShadingModes();
	static std::string getShadingModeName(ShadingMode mode);
	static ShadingMode getShadingModeFromName(const std::string& name);

	// Display mode utility methods
	static std::vector<std::string> getAvailableDisplayModes();
	static std::string getDisplayModeName(DisplayMode mode);
	static DisplayMode getDisplayModeFromName(const std::string& name);

	// Quality utility methods
	static std::vector<std::string> getAvailableQualityModes();
	static std::string getQualityModeName(RenderingQuality quality);
	static RenderingQuality getQualityModeFromName(const std::string& name);

	// Shadow mode utility methods
	static std::vector<std::string> getAvailableShadowModes();
	static std::string getShadowModeName(ShadowMode mode);
	static ShadowMode getShadowModeFromName(const std::string& name);

	// Lighting model utility methods
	static std::vector<std::string> getAvailableLightingModels();
	static std::string getLightingModelName(LightingModel model);
	static LightingModel getLightingModelFromName(const std::string& name);

	// Material preset methods
	static std::vector<std::string> getAvailablePresets();
	static std::string getPresetName(MaterialPreset preset);
	static MaterialPreset getPresetFromName(const std::string& name);
	MaterialSettings getPresetMaterial(MaterialPreset preset) const;
	void applyMaterialPreset(MaterialPreset preset);

	// Reset to defaults
	void resetToDefaults();

	// Notification system for real-time updates
	using SettingsChangedCallback = std::function<void()>;
	void registerSettingsChangedCallback(SettingsChangedCallback callback);
	void unregisterSettingsChangedCallback();
	void notifySettingsChanged() const;

	// Selected objects rendering settings
	// These methods apply settings only to selected geometries
	void applyMaterialSettingsToSelected(const MaterialSettings& settings);
	void applyTextureSettingsToSelected(const TextureSettings& settings);
	void applyBlendSettingsToSelected(const BlendSettings& settings);
	void applyShadingSettingsToSelected(const ShadingSettings& settings);
	void applyDisplaySettingsToSelected(const DisplaySettings& settings);

	// Individual property setters for selected objects
	void setSelectedMaterialAmbientColor(const Quantity_Color& color);
	void setSelectedMaterialDiffuseColor(const Quantity_Color& color);
	void setSelectedMaterialSpecularColor(const Quantity_Color& color);
	void setSelectedMaterialShininess(double shininess);
	void setSelectedMaterialTransparency(double transparency);

	void setSelectedTextureColor(const Quantity_Color& color);
	void setSelectedTextureIntensity(double intensity);
	void setSelectedTextureEnabled(bool enabled);
	void setSelectedTextureImagePath(const std::string& path);
	void setSelectedTextureMode(TextureMode mode);

	void setSelectedBlendMode(BlendMode mode);
	void setSelectedDepthTest(bool enabled);
	void setSelectedDepthWrite(bool enabled);
	void setSelectedCullFace(bool enabled);
	void setSelectedAlphaThreshold(double threshold);

	void setSelectedShadingMode(ShadingMode mode);
	void setSelectedSmoothNormals(bool enabled);
	void setSelectedWireframeWidth(double width);
	void setSelectedPointSize(double size);

	void setSelectedDisplayMode(DisplayMode mode);
	void setSelectedShowEdges(bool enabled);
	void setSelectedShowVertices(bool enabled);
	void setSelectedEdgeWidth(double width);
	void setSelectedVertexSize(double size);
	void setSelectedEdgeColor(const Quantity_Color& color);
	void setSelectedVertexColor(const Quantity_Color& color);

	// Utility method to check if any objects are selected
	bool hasSelectedObjects() const;

	// Static method to get OCCViewer instance for selection checking
	static OCCViewer* getOCCViewerInstance();

	// Apply material preset to selected objects
	void applyMaterialPresetToSelected(MaterialPreset preset);

	// Test feedback methods
	std::string getCurrentSelectionStatus() const;
	std::string getCurrentRenderingSettings() const;
	void showTestFeedback() const;

private:
	RenderingConfig();
	~RenderingConfig() = default;
	RenderingConfig(const RenderingConfig&) = delete;
	RenderingConfig& operator=(const RenderingConfig&) = delete;

	std::string getConfigFilePath() const;
	Quantity_Color parseColor(const std::string& value, const Quantity_Color& defaultValue) const;
	std::string colorToString(const Quantity_Color& color) const;

	// Initialize material presets
	void initializeMaterialPresets();

	MaterialSettings m_materialSettings;
	LightingSettings m_lightingSettings;
	TextureSettings m_textureSettings;
	BlendSettings m_blendSettings;
	ShadingSettings m_shadingSettings;
	DisplaySettings m_displaySettings;
	QualitySettings m_qualitySettings;
	ShadowSettings m_shadowSettings;
	LightingModelSettings m_lightingModelSettings;

	bool m_autoSave;
	static std::map<MaterialPreset, MaterialSettings> s_materialPresets;

	// Callback for settings changes
	SettingsChangedCallback m_settingsChangedCallback;
};