#include "param/ParameterSynchronizer.h"
#include "param/ParameterTree.h"
#include "param/ParameterUpdateManager.h"
#include "OCCGeometry.h"
#include "config/RenderingConfig.h"

// ParameterSynchronizer implementation
ParameterSynchronizer& ParameterSynchronizer::getInstance() {
    static ParameterSynchronizer instance;
    return instance;
}

ParameterSynchronizer::ParameterSynchronizer()
    : m_synchronizationEnabled(true)
    , m_inBatchSync(false)
    , m_defaultTreeToSystem(true)
    , m_defaultSystemToTree(false) {
    
    // Initialize parameter mappings
    initializeParameterMappings();
}

void ParameterSynchronizer::synchronizeGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry) return;
    
    std::lock_guard<std::mutex> lock(m_syncMutex);
    
    // Check if already synchronized
    if (m_synchronizedGeometries.find(geometry) != m_synchronizedGeometries.end()) {
        return;
    }
    
    setupGeometrySynchronization(geometry);
}

void ParameterSynchronizer::unsynchronizeGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry) return;
    
    std::lock_guard<std::mutex> lock(m_syncMutex);
    
    auto it = m_synchronizedGeometries.find(geometry);
    if (it != m_synchronizedGeometries.end()) {
        m_synchronizedGeometries.erase(it);
    }
}

void ParameterSynchronizer::synchronizeRenderingConfig(RenderingConfig* config) {
    if (!config) return;
    
    std::lock_guard<std::mutex> lock(m_syncMutex);
    
    // Check if already synchronized
    if (m_synchronizedConfigs.find(config) != m_synchronizedConfigs.end()) {
        return;
    }
    
    setupRenderingConfigSynchronization(config);
}

void ParameterSynchronizer::unsynchronizeRenderingConfig(RenderingConfig* config) {
    if (!config) return;
    
    std::lock_guard<std::mutex> lock(m_syncMutex);
    
    auto it = m_synchronizedConfigs.find(config);
    if (it != m_synchronizedConfigs.end()) {
        m_synchronizedConfigs.erase(it);
    }
}

void ParameterSynchronizer::setSyncDirection(const std::string& parameterPath, bool treeToSystem, bool systemToTree) {
    // Implementation for setting sync direction for specific parameters
    // This would require additional data structures to track per-parameter sync directions
}

void ParameterSynchronizer::setDefaultSyncDirection(bool treeToSystem, bool systemToTree) {
    m_defaultTreeToSystem = treeToSystem;
    m_defaultSystemToTree = systemToTree;
}

void ParameterSynchronizer::beginBatchSync() {
    m_inBatchSync = true;
}

void ParameterSynchronizer::endBatchSync() {
    m_inBatchSync = false;
}

bool ParameterSynchronizer::isParameterSynchronized(const std::string& parameterPath) const {
    std::lock_guard<std::mutex> lock(m_syncMutex);
    
    // Check if parameter is mapped to any geometry or config
    return m_parameterToGeometryProperty.find(parameterPath) != m_parameterToGeometryProperty.end() ||
           m_parameterToConfigProperty.find(parameterPath) != m_parameterToConfigProperty.end();
}

std::vector<std::string> ParameterSynchronizer::getSynchronizedParameters() const {
    std::lock_guard<std::mutex> lock(m_syncMutex);
    
    std::vector<std::string> synchronizedParams;
    
    for (const auto& pair : m_parameterToGeometryProperty) {
        synchronizedParams.push_back(pair.first);
    }
    
    for (const auto& pair : m_parameterToConfigProperty) {
        synchronizedParams.push_back(pair.first);
    }
    
    return synchronizedParams;
}

void ParameterSynchronizer::setupGeometrySynchronization(std::shared_ptr<OCCGeometry> geometry) {
    // Create geometry parameter synchronizer
    auto synchronizer = std::make_unique<GeometryParameterSynchronizer>(geometry);
    
    // Register parameters for this geometry
    std::vector<std::string> geometryParams = {
        "geometry/transform/position/x",
        "geometry/transform/position/y", 
        "geometry/transform/position/z",
        "geometry/transform/rotation/axis/x",
        "geometry/transform/rotation/axis/y",
        "geometry/transform/rotation/axis/z",
        "geometry/transform/rotation/angle",
        "geometry/transform/scale",
        "geometry/display/visible",
        "geometry/display/selected",
        "geometry/color/main",
        "geometry/transparency",
        "material/color/ambient",
        "material/color/diffuse",
        "material/color/specular",
        "material/properties/shininess",
        "material/properties/transparency"
    };
    
    m_synchronizedGeometries[geometry] = geometryParams;
    
    // Set up parameter mappings
    for (const auto& param : geometryParams) {
        m_parameterToGeometryProperty[param] = param; // Simplified mapping
    }
}

void ParameterSynchronizer::setupRenderingConfigSynchronization(RenderingConfig* config) {
    // Create config parameter synchronizer
    auto synchronizer = std::make_unique<RenderingConfigParameterSynchronizer>(config);
    
    // Register parameters for this config
    std::vector<std::string> configParams = {
        "rendering/mode/display_mode",
        "rendering/mode/shading_mode",
        "rendering/mode/rendering_quality",
        "lighting/ambient/color",
        "lighting/ambient/intensity",
        "lighting/diffuse/color",
        "lighting/diffuse/intensity",
        "shadow/mode/shadow_mode",
        "shadow/intensity/shadow_intensity",
        "quality/level/rendering_quality",
        "quality/level/tessellation_level"
    };
    
    m_synchronizedConfigs[config] = configParams;
    
    // Set up parameter mappings
    for (const auto& param : configParams) {
        m_parameterToConfigProperty[param] = param; // Simplified mapping
    }
}

void ParameterSynchronizer::onParameterTreeChanged(const std::string& path, const ParameterValue& value) {
    if (!m_synchronizationEnabled) return;
    
    // Forward to update manager
    auto& updateManager = ParameterUpdateManager::getInstance();
    updateManager.onParameterChanged(path, value);
}

void ParameterSynchronizer::onSystemParameterChanged(const std::string& path, const ParameterValue& value) {
    if (!m_synchronizationEnabled) return;
    
    // Update parameter tree
    auto& tree = ParameterTree::getInstance();
    tree.setParameterValue(path, value);
}

void ParameterSynchronizer::initializeParameterMappings() {
    // Initialize default parameter mappings
    // This would contain the mapping between parameter tree paths and system properties
}

// GeometryParameterSynchronizer implementation
GeometryParameterSynchronizer::GeometryParameterSynchronizer(std::shared_ptr<OCCGeometry> geometry)
    : m_geometry(geometry) {
    initializePropertyMappings();
    setupTreeCallbacks();
    setupGeometryCallbacks();
}

GeometryParameterSynchronizer::~GeometryParameterSynchronizer() {
    // Cleanup callbacks
}

void GeometryParameterSynchronizer::syncFromTree() {
    if (!m_geometry) return;
    
    auto& tree = ParameterTree::getInstance();
    
    // Sync transform parameters
    if (tree.hasParameter("geometry/transform/position/x")) {
        double x = tree.getParameterValue("geometry/transform/position/x").getValueAs<double>();
        double y = tree.getParameterValue("geometry/transform/position/y").getValueAs<double>();
        double z = tree.getParameterValue("geometry/transform/position/z").getValueAs<double>();
        m_geometry->setPosition(gp_Pnt(x, y, z));
    }
    
    // Sync color parameters
    if (tree.hasParameter("geometry/color/main")) {
        auto color = tree.getParameterValue("geometry/color/main").getValueAs<Quantity_Color>();
        m_geometry->setColor(color);
    }
    
    // Sync material parameters
    if (tree.hasParameter("material/color/diffuse")) {
        auto color = tree.getParameterValue("material/color/diffuse").getValueAs<Quantity_Color>();
        m_geometry->setMaterialDiffuseColor(color);
    }
    
    if (tree.hasParameter("material/properties/transparency")) {
        double transparency = tree.getParameterValue("material/properties/transparency").getValueAs<double>();
        m_geometry->setTransparency(transparency);
    }
}

void GeometryParameterSynchronizer::syncToTree() {
    if (!m_geometry) return;
    
    auto& tree = ParameterTree::getInstance();
    
    // Sync transform parameters
    auto position = m_geometry->getPosition();
    tree.setParameterValue("geometry/transform/position/x", position.X());
    tree.setParameterValue("geometry/transform/position/y", position.Y());
    tree.setParameterValue("geometry/transform/position/z", position.Z());
    
    // Sync color parameters
    tree.setParameterValue("geometry/color/main", m_geometry->getColor());
    
    // Sync material parameters
    tree.setParameterValue("material/color/diffuse", m_geometry->getMaterialDiffuseColor());
    tree.setParameterValue("material/properties/transparency", m_geometry->getTransparency());
}

void GeometryParameterSynchronizer::setupParameterMappings() {
    // Set up mappings between parameter paths and geometry properties
    m_propertySetters["geometry/transform/position/x"] = [this](const ParameterValue& value) {
        if (std::holds_alternative<double>(value)) {
            double x = std::get<double>(value);
            auto pos = m_geometry->getPosition();
            m_geometry->setPosition(gp_Pnt(x, pos.Y(), pos.Z()));
        }
    };
    
    m_propertyGetters["geometry/transform/position/x"] = [this]() -> ParameterValue {
        return m_geometry->getPosition().X();
    };
    
    m_propertySetters["geometry/color/main"] = [this](const ParameterValue& value) {
        if (std::holds_alternative<Quantity_Color>(value)) {
            auto color = std::get<Quantity_Color>(value);
            m_geometry->setColor(color);
        }
    };
    
    m_propertyGetters["geometry/color/main"] = [this]() -> ParameterValue {
        return m_geometry->getColor();
    };
}

void GeometryParameterSynchronizer::onTreeParameterChanged(const std::string& path, const ParameterValue& value) {
    auto it = m_propertySetters.find(path);
    if (it != m_propertySetters.end()) {
        it->second(value);
    }
}

void GeometryParameterSynchronizer::onGeometryPropertyChanged(const std::string& property, const ParameterValue& value) {
    auto it = m_propertyGetters.find(property);
    if (it != m_propertyGetters.end()) {
        auto& tree = ParameterTree::getInstance();
        tree.setParameterValue(property, it->second());
    }
}

void GeometryParameterSynchronizer::initializePropertyMappings() {
    // Initialize all property mappings
    setupParameterMappings();
}

void GeometryParameterSynchronizer::setupTreeCallbacks() {
    auto& tree = ParameterTree::getInstance();
    
    // Register callback for parameter changes
    tree.addGlobalChangedCallback([this](const std::string& path, const ParameterValue& value) {
        onTreeParameterChanged(path, value);
    });
}

void GeometryParameterSynchronizer::setupGeometryCallbacks() {
    // Set up callbacks for geometry property changes
    // This would require the geometry to support property change notifications
}

// RenderingConfigParameterSynchronizer implementation
RenderingConfigParameterSynchronizer::RenderingConfigParameterSynchronizer(RenderingConfig* config)
    : m_config(config) {
    initializePropertyMappings();
    setupTreeCallbacks();
    setupConfigCallbacks();
}

RenderingConfigParameterSynchronizer::~RenderingConfigParameterSynchronizer() {
    // Cleanup callbacks
}

void RenderingConfigParameterSynchronizer::syncFromTree() {
    if (!m_config) return;
    
    auto& tree = ParameterTree::getInstance();
    
    // Sync rendering mode parameters
    if (tree.hasParameter("rendering/mode/display_mode")) {
        auto mode = tree.getParameterValue("rendering/mode/display_mode").getValueAs<RenderingConfig::DisplayMode>();
        m_config->setDisplayMode(mode);
    }
    
    // Sync lighting parameters
    if (tree.hasParameter("lighting/ambient/color")) {
        auto color = tree.getParameterValue("lighting/ambient/color").getValueAs<Quantity_Color>();
        m_config->setLightAmbientColor(color);
    }
    
    // Sync quality parameters
    if (tree.hasParameter("quality/level/rendering_quality")) {
        auto quality = tree.getParameterValue("quality/level/rendering_quality").getValueAs<RenderingConfig::RenderingQuality>();
        m_config->setRenderingQuality(quality);
    }
}

void RenderingConfigParameterSynchronizer::syncToTree() {
    if (!m_config) return;
    
    auto& tree = ParameterTree::getInstance();
    
    // Sync rendering mode parameters
    tree.setParameterValue("rendering/mode/display_mode", static_cast<int>(m_config->getDisplaySettings().displayMode));
    
    // Sync lighting parameters
    tree.setParameterValue("lighting/ambient/color", m_config->getLightingSettings().ambientColor);
    
    // Sync quality parameters
    tree.setParameterValue("quality/level/rendering_quality", static_cast<int>(m_config->getQualitySettings().quality));
}

void RenderingConfigParameterSynchronizer::setupParameterMappings() {
    // Set up mappings between parameter paths and config properties
    m_propertySetters["rendering/mode/display_mode"] = [this](const ParameterValue& value) {
        if (std::holds_alternative<RenderingConfig::DisplayMode>(value)) {
            auto mode = std::get<RenderingConfig::DisplayMode>(value);
            m_config->setDisplayMode(mode);
        }
    };
    
    m_propertyGetters["rendering/mode/display_mode"] = [this]() -> ParameterValue {
        return static_cast<int>(m_config->getDisplaySettings().displayMode);
    };
}

void RenderingConfigParameterSynchronizer::onTreeParameterChanged(const std::string& path, const ParameterValue& value) {
    auto it = m_propertySetters.find(path);
    if (it != m_propertySetters.end()) {
        it->second(value);
    }
}

void RenderingConfigParameterSynchronizer::onConfigPropertyChanged(const std::string& property, const ParameterValue& value) {
    auto it = m_propertyGetters.find(property);
    if (it != m_propertyGetters.end()) {
        auto& tree = ParameterTree::getInstance();
        tree.setParameterValue(property, it->second());
    }
}

void RenderingConfigParameterSynchronizer::initializePropertyMappings() {
    // Initialize all property mappings
    setupParameterMappings();
}

void RenderingConfigParameterSynchronizer::setupTreeCallbacks() {
    auto& tree = ParameterTree::getInstance();
    
    // Register callback for parameter changes
    tree.addGlobalChangedCallback([this](const std::string& path, const ParameterValue& value) {
        onTreeParameterChanged(path, value);
    });
}

void RenderingConfigParameterSynchronizer::setupConfigCallbacks() {
    // Set up callbacks for config property changes
    // This would require the config to support property change notifications
}

// ParameterSynchronizerInitializer implementation
void ParameterSynchronizerInitializer::initialize() {
    initializeParameterMappings();
    initializeDefaultSynchronizations();
}

void ParameterSynchronizerInitializer::initializeParameterMappings() {
    // Initialize default parameter mappings
}

void ParameterSynchronizerInitializer::initializeDefaultSynchronizations() {
    // Initialize default synchronizations
}