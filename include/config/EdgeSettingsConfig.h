#pragma once

#include <string>
#include <OpenCASCADE/Quantity_Color.hxx>

struct EdgeSettings {
	bool showEdges;
	double edgeWidth;
	Quantity_Color edgeColor;
	bool edgeColorEnabled;
	int edgeStyle;
	double edgeOpacity;

	EdgeSettings()
		: showEdges(false)
		, edgeWidth(1.0)
		, edgeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
		, edgeColorEnabled(true)
		, edgeStyle(0)
		, edgeOpacity(1.0)
	{
	}
};

class EdgeSettingsConfig {
public:
	static EdgeSettingsConfig& getInstance();

	// Load and save settings
	bool loadFromFile(const std::string& filename = "");
	bool saveToFile(const std::string& filename = "") const;
	std::string getConfigFilePath() const;

	// Get settings for different object states
	const EdgeSettings& getGlobalSettings() const { return m_globalSettings; }
	const EdgeSettings& getSelectedSettings() const { return m_selectedSettings; }
	const EdgeSettings& getHoverSettings() const { return m_hoverSettings; }

	// Set settings for different object states
	void setGlobalSettings(const EdgeSettings& settings);
	void setSelectedSettings(const EdgeSettings& settings);
	void setHoverSettings(const EdgeSettings& settings);

	// Individual setting setters
	void setGlobalShowEdges(bool show) { m_globalSettings.showEdges = show; }
	void setGlobalEdgeWidth(double width) { m_globalSettings.edgeWidth = width; }
	void setGlobalEdgeColor(const Quantity_Color& color) { m_globalSettings.edgeColor = color; }
	void setGlobalEdgeColorEnabled(bool enabled) { m_globalSettings.edgeColorEnabled = enabled; }
	void setGlobalEdgeStyle(int style) { m_globalSettings.edgeStyle = style; }
	void setGlobalEdgeOpacity(double opacity) { m_globalSettings.edgeOpacity = opacity; }

	void setSelectedShowEdges(bool show) { m_selectedSettings.showEdges = show; }
	void setSelectedEdgeWidth(double width) { m_selectedSettings.edgeWidth = width; }
	void setSelectedEdgeColor(const Quantity_Color& color) { m_selectedSettings.edgeColor = color; }
	void setSelectedEdgeColorEnabled(bool enabled) { m_selectedSettings.edgeColorEnabled = enabled; }
	void setSelectedEdgeStyle(int style) { m_selectedSettings.edgeStyle = style; }
	void setSelectedEdgeOpacity(double opacity) { m_selectedSettings.edgeOpacity = opacity; }

	void setHoverShowEdges(bool show) { m_hoverSettings.showEdges = show; }
	void setHoverEdgeWidth(double width) { m_hoverSettings.edgeWidth = width; }
	void setHoverEdgeColor(const Quantity_Color& color) { m_hoverSettings.edgeColor = color; }
	void setHoverEdgeColorEnabled(bool enabled) { m_hoverSettings.edgeColorEnabled = enabled; }
	void setHoverEdgeStyle(int style) { m_hoverSettings.edgeStyle = style; }
	void setHoverEdgeOpacity(double opacity) { m_hoverSettings.edgeOpacity = opacity; }

	// Get settings based on object state
	EdgeSettings getSettingsForState(const std::string& state) const;

	// Reset to defaults
	void resetToDefaults();

	// Apply settings to geometries
	void applySettingsToGeometries();

	// Notification system
	void notifySettingsChanged();
	void addSettingsChangedCallback(std::function<void()> callback);

	double getFeatureEdgeAngle() const;
	void setFeatureEdgeAngle(double angle);
	double getFeatureEdgeMinLength() const;
	void setFeatureEdgeMinLength(double len);
	bool getFeatureEdgeOnlyConvex() const;
	void setFeatureEdgeOnlyConvex(bool v);
	bool getFeatureEdgeOnlyConcave() const;
	void setFeatureEdgeOnlyConcave(bool v);

private:
	EdgeSettingsConfig();
	~EdgeSettingsConfig() = default;
	EdgeSettingsConfig(const EdgeSettingsConfig&) = delete;
	EdgeSettingsConfig& operator=(const EdgeSettingsConfig&) = delete;

	// Helper methods
	std::string colorToString(const Quantity_Color& color) const;
	Quantity_Color stringToColor(const std::string& str) const;
	std::string boolToString(bool value) const;
	bool stringToBool(const std::string& str) const;

	EdgeSettings m_globalSettings;
	EdgeSettings m_selectedSettings;
	EdgeSettings m_hoverSettings;

	std::vector<std::function<void()>> m_callbacks;

	double m_featureEdgeAngle = 30.0;
	double m_featureEdgeMinLength = 0.1;
	bool m_onlyConvex = false;
	bool m_onlyConcave = false;
};