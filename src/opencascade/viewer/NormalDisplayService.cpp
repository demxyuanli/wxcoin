#include "viewer/NormalDisplayService.h"
#include "edges/EdgeDisplayManager.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"

NormalDisplayService::NormalDisplayService()
    : m_edgeDisplayManager(nullptr)
{
}

NormalDisplayService::~NormalDisplayService()
{
}

void NormalDisplayService::setNormalDisplayConfig(const NormalDisplayConfig& config)
{
    m_config = config;
    updateNormalDisplaySettings();
}

const NormalDisplayConfig& NormalDisplayService::getNormalDisplayConfig() const
{
    return m_config;
}

void NormalDisplayService::setShowNormals(bool showNormals, EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams)
{
    m_config.showNormals = showNormals;
    if (edgeDisplayManager) {
        edgeDisplayManager->setShowNormalLines(showNormals, meshParams);
    }
    LOG_INF_S("Normal display " + std::string(showNormals ? "enabled" : "disabled"));
}

bool NormalDisplayService::isShowNormals() const
{
    return m_config.showNormals;
}

void NormalDisplayService::setNormalLength(double length)
{
    m_config.length = length;
    LOG_INF_S("Normal length set to: " + std::to_string(length));
}

double NormalDisplayService::getNormalLength() const
{
    return m_config.length;
}

void NormalDisplayService::setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect)
{
    m_config.correctColor = correct;
    m_config.incorrectColor = incorrect;
    LOG_INF_S("Normal colors updated");
}

void NormalDisplayService::getNormalColor(Quantity_Color& correct, Quantity_Color& incorrect) const
{
    correct = m_config.correctColor;
    incorrect = m_config.incorrectColor;
}

void NormalDisplayService::setNormalConsistencyMode(bool enabled)
{
    m_config.consistencyMode = enabled;
    LOG_INF_S("Normal consistency mode " + std::string(enabled ? "enabled" : "disabled"));
}

bool NormalDisplayService::isNormalConsistencyModeEnabled() const
{
    return m_config.consistencyMode;
}

void NormalDisplayService::setNormalDebugMode(bool enabled)
{
    m_config.debugMode = enabled;

    // Toggle normal display based on debug mode
    if (enabled) {
        // Enable debug visualization with specific colors
        m_config.correctColor = Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB);   // Green for correct
        m_config.incorrectColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB); // Red for incorrect
        LOG_INF_S("Normal debug mode enabled with debug colors");
    } else {
        // Restore default colors
        m_config.correctColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);   // Red for correct
        m_config.incorrectColor = Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB); // Green for incorrect
        LOG_INF_S("Normal debug mode disabled");
    }
}

bool NormalDisplayService::isNormalDebugModeEnabled() const
{
    return m_config.debugMode;
}

void NormalDisplayService::refreshNormalDisplay(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams)
{
    // Force regeneration of all normal lines
    if (edgeDisplayManager) {
        edgeDisplayManager->updateAll(meshParams);
    }
    LOG_INF_S("Normal display refreshed");
}

void NormalDisplayService::toggleNormalDisplay(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams)
{
    bool currentState = isShowNormals();
    setShowNormals(!currentState, edgeDisplayManager, meshParams);
}

void NormalDisplayService::enableNormalDebugVisualization(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams)
{
    setNormalDebugMode(true);
    setShowNormals(true, edgeDisplayManager, meshParams);
    LOG_INF_S("Normal debug visualization enabled");
}

void NormalDisplayService::updateNormalDisplaySettings()
{
    LOG_INF_S("Normal display settings updated");
}

void NormalDisplayService::forceNormalRegeneration(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams)
{
    if (edgeDisplayManager) {
        edgeDisplayManager->updateAll(meshParams);
    }
}
