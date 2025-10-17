#include "ViewportConfig.h"
#include <wx/log.h>

//==============================================================================
// ViewportConfig Implementation
//==============================================================================

ViewportConfig& ViewportConfig::getInstance() {
    static ViewportConfig instance;
    return instance;
}

ViewportConfig::ViewportConfig() {
    // Initialize default values
    m_config.margin = 20;
    m_config.cubeSize = 120;
    m_config.coordSystemSize = 80;
    m_config.normalColor = SbColor(0.7f, 0.7f, 0.7f);
    m_config.hoverColor = SbColor(1.0f, 0.85f, 0.4f);
    m_config.backgroundColor = SbColor(0.2f, 0.2f, 0.25f);
    m_config.enableCube = true;
    m_config.enableCoordSystem = true;
    m_config.enableOutline = true;
    m_config.dpiScale = 1.0f;

    wxLogDebug("ViewportConfig: Initialized with default values");
}

int ViewportConfig::getMargin() const {
    return m_config.margin;
}

int ViewportConfig::getCubeSize() const {
    return m_config.cubeSize;
}

int ViewportConfig::getCoordSystemSize() const {
    return m_config.coordSystemSize;
}

const SbColor& ViewportConfig::getNormalColor() const {
    return m_config.normalColor;
}

const SbColor& ViewportConfig::getHoverColor() const {
    return m_config.hoverColor;
}

const SbColor& ViewportConfig::getBackgroundColor() const {
    return m_config.backgroundColor;
}

bool ViewportConfig::isCubeEnabled() const {
    return m_config.enableCube;
}

bool ViewportConfig::isCoordSystemEnabled() const {
    return m_config.enableCoordSystem;
}

bool ViewportConfig::isOutlineEnabled() const {
    return m_config.enableOutline;
}

float ViewportConfig::getDPIScale() const {
    return m_config.dpiScale;
}

void ViewportConfig::setMargin(int margin) {
    m_config.margin = margin;
    notifyConfigChanged();
}

void ViewportConfig::setCubeSize(int size) {
    m_config.cubeSize = size;
    notifyConfigChanged();
}

void ViewportConfig::setCoordSystemSize(int size) {
    m_config.coordSystemSize = size;
    notifyConfigChanged();
}

void ViewportConfig::setNormalColor(const SbColor& color) {
    m_config.normalColor = color;
    notifyConfigChanged();
}

void ViewportConfig::setHoverColor(const SbColor& color) {
    m_config.hoverColor = color;
    notifyConfigChanged();
}

void ViewportConfig::setBackgroundColor(const SbColor& color) {
    m_config.backgroundColor = color;
    notifyConfigChanged();
}

void ViewportConfig::setCubeEnabled(bool enabled) {
    m_config.enableCube = enabled;
    notifyConfigChanged();
}

void ViewportConfig::setCoordSystemEnabled(bool enabled) {
    m_config.enableCoordSystem = enabled;
    notifyConfigChanged();
}

void ViewportConfig::setOutlineEnabled(bool enabled) {
    m_config.enableOutline = enabled;
    notifyConfigChanged();
}

void ViewportConfig::setDPIScale(float scale) {
    m_config.dpiScale = scale;
    notifyConfigChanged();
}

void ViewportConfig::addConfigChangeListener(ConfigChangeCallback callback) {
    m_configChangeListeners.push_back(callback);
}

void ViewportConfig::removeConfigChangeListener(ConfigChangeCallback callback) {
    auto it = std::find(m_configChangeListeners.begin(), m_configChangeListeners.end(), callback);
    if (it != m_configChangeListeners.end()) {
        m_configChangeListeners.erase(it);
    }
}

void ViewportConfig::notifyConfigChanged() {
    for (auto& callback : m_configChangeListeners) {
        if (callback) {
            callback();
        }
    }
}

bool ViewportConfig::saveToFile(const wxString& filename) const {
    // TODO: Implement configuration file saving
    wxLogWarning("ViewportConfig: saveToFile not implemented yet");
    return false;
}

bool ViewportConfig::loadFromFile(const wxString& filename) {
    // TODO: Implement configuration file loading
    wxLogWarning("ViewportConfig: loadFromFile not implemented yet");
    return false;
}

void ViewportConfig::resetToDefaults() {
    m_config.margin = 20;
    m_config.cubeSize = 120;
    m_config.coordSystemSize = 80;
    m_config.normalColor = SbColor(0.7f, 0.7f, 0.7f);
    m_config.hoverColor = SbColor(1.0f, 0.85f, 0.4f);
    m_config.backgroundColor = SbColor(0.2f, 0.2f, 0.25f);
    m_config.enableCube = true;
    m_config.enableCoordSystem = true;
    m_config.enableOutline = true;
    m_config.dpiScale = 1.0f;

    notifyConfigChanged();
    wxLogDebug("ViewportConfig: Reset to default values");
}

