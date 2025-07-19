#include "rendering/RenderConfig.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>

RenderConfig& RenderConfig::getInstance() {
    static RenderConfig instance;
    return instance;
}

bool RenderConfig::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_WRN_S("Cannot open configuration file: " + filename);
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                setParameter(key, value);
            }
        }
        
        LOG_INF_S("Configuration loaded from file: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception loading configuration: " + std::string(e.what()));
        return false;
    }
}

bool RenderConfig::saveToFile(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_WRN_S("Cannot open configuration file for writing: " + filename);
            return false;
        }
        
        file << "# Rendering Configuration File" << std::endl;
        file << "# Generated automatically" << std::endl << std::endl;
        
        // Save edge settings
        file << "[RenderEdgeSettings]" << std::endl;
        file << "showEdges=" << (m_edgeSettings.showEdges ? "true" : "false") << std::endl;
        file << "edgeColorEnabled=" << (m_edgeSettings.edgeColorEnabled ? "true" : "false") << std::endl;
        file << "featureEdgeAngle=" << m_edgeSettings.featureEdgeAngle << std::endl;
        file << std::endl;
        
        // Save smoothing settings
        file << "[SmoothingSettings]" << std::endl;
        file << "enabled=" << (m_smoothingSettings.enabled ? "true" : "false") << std::endl;
        file << "creaseAngle=" << m_smoothingSettings.creaseAngle << std::endl;
        file << "iterations=" << m_smoothingSettings.iterations << std::endl;
        file << std::endl;
        
        // Save subdivision settings
        file << "[SubdivisionSettings]" << std::endl;
        file << "enabled=" << (m_subdivisionSettings.enabled ? "true" : "false") << std::endl;
        file << "levels=" << m_subdivisionSettings.levels << std::endl;
        file << std::endl;
        
        // Save custom parameters
        if (!m_customParameters.empty()) {
            file << "[CustomParameters]" << std::endl;
            for (const auto& param : m_customParameters) {
                file << param.first << "=" << param.second << std::endl;
            }
        }
        
        LOG_INF_S("Configuration saved to file: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception saving configuration: " + std::string(e.what()));
        return false;
    }
}

void RenderConfig::setParameter(const std::string& key, const std::string& value) {
    m_customParameters[key] = value;
}

std::string RenderConfig::getParameter(const std::string& key, const std::string& defaultValue) const {
    auto it = m_customParameters.find(key);
    if (it != m_customParameters.end()) {
        return it->second;
    }
    return defaultValue;
}

void RenderConfig::resetToDefaults() {
    m_edgeSettings = RenderEdgeSettings();
    m_smoothingSettings = SmoothingSettings();
    m_subdivisionSettings = SubdivisionSettings();
    m_customParameters.clear();
    
    LOG_INF_S("Configuration reset to defaults");
} 