#include "config/EdgeSettingsConfig.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <wx/stdpaths.h>
#include <wx/filename.h>

EdgeSettingsConfig& EdgeSettingsConfig::getInstance()
{
	static EdgeSettingsConfig instance;
	return instance;
}

EdgeSettingsConfig::EdgeSettingsConfig()
{
	// Load configuration from file on startup
	loadFromFile();
}

std::string EdgeSettingsConfig::getConfigFilePath() const
{
	// Save to local root directory instead of user config directory
	wxString currentDir = wxGetCwd();
	wxFileName configFile(currentDir, "edge_settings.ini");

	// Create directory if it doesn't exist (should always exist for current directory)
	if (!configFile.DirExists()) {
		configFile.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	return configFile.GetFullPath().ToStdString();
}

bool EdgeSettingsConfig::loadFromFile(const std::string& filename)
{
	std::string configPath = filename.empty() ? getConfigFilePath() : filename;
	std::ifstream file(configPath);

	if (!file.is_open()) {
		return false;
	}

	std::string line;
	std::string currentSection;

	while (std::getline(file, line)) {
		// Remove comments and trim whitespace
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos) {
			line = line.substr(0, commentPos);
		}

		// Trim whitespace
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (line.empty()) {
			continue;
		}

		// Check if this is a section header
		if (line[0] == '[' && line[line.length() - 1] == ']') {
			currentSection = line.substr(1, line.length() - 2);
			continue;
		}

		// Parse key-value pairs
		size_t equalPos = line.find('=');
		if (equalPos == std::string::npos) {
			continue;
		}

		std::string key = line.substr(0, equalPos);
		std::string value = line.substr(equalPos + 1);

		// Trim whitespace from key and value
		key.erase(0, key.find_first_not_of(" \t"));
		key.erase(key.find_last_not_of(" \t") + 1);
		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of(" \t") + 1);

		EdgeSettings* targetSettings = nullptr;
		if (currentSection == "Global") {
			targetSettings = &m_globalSettings;
		}
		else if (currentSection == "Selected") {
			targetSettings = &m_selectedSettings;
		}
		else if (currentSection == "Hover") {
			targetSettings = &m_hoverSettings;
		}

		if (targetSettings) {
			if (key == "ShowEdges") {
				targetSettings->showEdges = stringToBool(value);
			}
			else if (key == "EdgeWidth") {
				targetSettings->edgeWidth = std::stod(value);
			}
			else if (key == "EdgeColor") {
				targetSettings->edgeColor = stringToColor(value);
			}
			else if (key == "EdgeColorEnabled") {
				targetSettings->edgeColorEnabled = stringToBool(value);
			}
			else if (key == "EdgeStyle") {
				targetSettings->edgeStyle = std::stoi(value);
			}
			else if (key == "EdgeOpacity") {
				targetSettings->edgeOpacity = std::stod(value);
			}
		}

		// Load feature edge settings
		if (currentSection == "Global") {
			if (key == "FeatureEdgeAngle") m_featureEdgeAngle = std::stod(value);
			else if (key == "FeatureEdgeMinLength") m_featureEdgeMinLength = std::stod(value);
			else if (key == "FeatureEdgeOnlyConvex") m_onlyConvex = stringToBool(value);
			else if (key == "FeatureEdgeOnlyConcave") m_onlyConcave = stringToBool(value);
		}
	}

	file.close();

	// Notify listeners of settings change
	notifySettingsChanged();

	return true;
}

bool EdgeSettingsConfig::saveToFile(const std::string& filename) const
{
	std::string configPath = filename.empty() ? getConfigFilePath() : filename;
	std::ofstream file(configPath);

	if (!file.is_open()) {
		LOG_ERR_S("EdgeSettingsConfig: Failed to save configuration to: " + configPath);
		return false;
	}


	// Save Global settings
	file << "[Global]\n";
	file << "ShowEdges=" << boolToString(m_globalSettings.showEdges) << "\n";
	file << "EdgeWidth=" << m_globalSettings.edgeWidth << "\n";
	file << "EdgeColor=" << colorToString(m_globalSettings.edgeColor) << "\n";
	file << "EdgeColorEnabled=" << boolToString(m_globalSettings.edgeColorEnabled) << "\n";
	file << "EdgeStyle=" << m_globalSettings.edgeStyle << "\n";
	file << "EdgeOpacity=" << m_globalSettings.edgeOpacity << "\n";
	file << "FeatureEdgeAngle=" << m_featureEdgeAngle << "\n";
	file << "FeatureEdgeMinLength=" << m_featureEdgeMinLength << "\n";
	file << "FeatureEdgeOnlyConvex=" << boolToString(m_onlyConvex) << "\n";
	file << "FeatureEdgeOnlyConcave=" << boolToString(m_onlyConcave) << "\n";
	file << "\n";

	// Save Selected settings
	file << "[Selected]\n";
	file << "ShowEdges=" << boolToString(m_selectedSettings.showEdges) << "\n";
	file << "EdgeWidth=" << m_selectedSettings.edgeWidth << "\n";
	file << "EdgeColor=" << colorToString(m_selectedSettings.edgeColor) << "\n";
	file << "EdgeColorEnabled=" << boolToString(m_selectedSettings.edgeColorEnabled) << "\n";
	file << "EdgeStyle=" << m_selectedSettings.edgeStyle << "\n";
	file << "EdgeOpacity=" << m_selectedSettings.edgeOpacity << "\n\n";

	// Save Hover settings
	file << "[Hover]\n";
	file << "ShowEdges=" << boolToString(m_hoverSettings.showEdges) << "\n";
	file << "EdgeWidth=" << m_hoverSettings.edgeWidth << "\n";
	file << "EdgeColor=" << colorToString(m_hoverSettings.edgeColor) << "\n";
	file << "EdgeColorEnabled=" << boolToString(m_hoverSettings.edgeColorEnabled) << "\n";
	file << "EdgeStyle=" << m_hoverSettings.edgeStyle << "\n";
	file << "EdgeOpacity=" << m_hoverSettings.edgeOpacity << "\n";

	file.close();
	return true;
}

void EdgeSettingsConfig::setGlobalSettings(const EdgeSettings& settings)
{
	m_globalSettings = settings;
	notifySettingsChanged();
}

void EdgeSettingsConfig::setSelectedSettings(const EdgeSettings& settings)
{
	m_selectedSettings = settings;
	notifySettingsChanged();
}

void EdgeSettingsConfig::setHoverSettings(const EdgeSettings& settings)
{
	m_hoverSettings = settings;
	notifySettingsChanged();
}

EdgeSettings EdgeSettingsConfig::getSettingsForState(const std::string& state) const
{
	if (state == "selected") {
		return m_selectedSettings;
	}
	else if (state == "hover") {
		return m_hoverSettings;
	}
	else {
		return m_globalSettings;
	}
}

void EdgeSettingsConfig::resetToDefaults()
{
	m_globalSettings = EdgeSettings();
	m_selectedSettings = EdgeSettings();
	m_selectedSettings.edgeWidth = 2.0;
	m_selectedSettings.edgeColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);

	m_hoverSettings = EdgeSettings();
	m_hoverSettings.edgeWidth = 1.5;
	m_hoverSettings.edgeColor = Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB);
	m_hoverSettings.edgeStyle = 1;
	m_hoverSettings.edgeOpacity = 0.8;

	m_featureEdgeAngle = 30.0;
	m_featureEdgeMinLength = 0.1;
	m_onlyConvex = false;
	m_onlyConcave = false;

	notifySettingsChanged();
}

void EdgeSettingsConfig::applySettingsToGeometries()
{
	// This method will be called when settings are applied
	// The actual geometry update is handled by OCCMeshConverter and OCCViewer
	// This method serves as a notification point for settings application
	notifySettingsChanged();
}

void EdgeSettingsConfig::notifySettingsChanged()
{
	for (auto& callback : m_callbacks) {
		callback();
	}
}

void EdgeSettingsConfig::addSettingsChangedCallback(std::function<void()> callback)
{
	m_callbacks.push_back(callback);
}

std::string EdgeSettingsConfig::colorToString(const Quantity_Color& color) const
{
	std::ostringstream oss;
	oss << color.Red() << "," << color.Green() << "," << color.Blue();
	return oss.str();
}

Quantity_Color EdgeSettingsConfig::stringToColor(const std::string& str) const
{
	std::istringstream iss(str);
	std::string token;
	std::vector<double> values;

	while (std::getline(iss, token, ',')) {
		values.push_back(std::stod(token));
	}

	if (values.size() >= 3) {
		return Quantity_Color(values[0], values[1], values[2], Quantity_TOC_RGB);
	}

	return Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
}

std::string EdgeSettingsConfig::boolToString(bool value) const
{
	return value ? "true" : "false";
}

bool EdgeSettingsConfig::stringToBool(const std::string& str) const
{
	std::string lowerStr = str;
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
	return (lowerStr == "true" || lowerStr == "1" || lowerStr == "yes");
}

static constexpr const char* kFeatureEdgeAngle = "feature_edge_angle";
static constexpr const char* kFeatureEdgeMinLength = "feature_edge_min_length";
static constexpr const char* kFeatureEdgeOnlyConvex = "feature_edge_only_convex";
static constexpr const char* kFeatureEdgeOnlyConcave = "feature_edge_only_concave";

double EdgeSettingsConfig::getFeatureEdgeAngle() const { return m_featureEdgeAngle; }
void EdgeSettingsConfig::setFeatureEdgeAngle(double angle) { m_featureEdgeAngle = angle; }
double EdgeSettingsConfig::getFeatureEdgeMinLength() const { return m_featureEdgeMinLength; }
void EdgeSettingsConfig::setFeatureEdgeMinLength(double len) { m_featureEdgeMinLength = len; }
bool EdgeSettingsConfig::getFeatureEdgeOnlyConvex() const { return m_onlyConvex; }
void EdgeSettingsConfig::setFeatureEdgeOnlyConvex(bool v) { m_onlyConvex = v; }
bool EdgeSettingsConfig::getFeatureEdgeOnlyConcave() const { return m_onlyConcave; }
void EdgeSettingsConfig::setFeatureEdgeOnlyConcave(bool v) { m_onlyConcave = v; }