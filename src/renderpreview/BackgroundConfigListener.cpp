#include "renderpreview/BackgroundConfigListener.h"
#include "renderpreview/PreviewCanvas.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/timer.h>

BackgroundConfigListener::BackgroundConfigListener(PreviewCanvas* canvas)
    : m_canvas(canvas), m_timer(nullptr)
{
    // Create a timer to periodically check for configuration changes
    m_timer = new wxTimer(this);
    m_timer->Start(500); // Check every 500ms

    // Bind the timer event
    Bind(wxEVT_TIMER, &BackgroundConfigListener::onTimer, this);

    // Store initial config values
    updateStoredConfig();

    LOG_INF_S("BackgroundConfigListener: Started config monitoring");
}

BackgroundConfigListener::~BackgroundConfigListener()
{
    if (m_timer) {
        m_timer->Stop();
        delete m_timer;
        m_timer = nullptr;
    }
    LOG_INF_S("BackgroundConfigListener: Destroyed");
}

void BackgroundConfigListener::updateStoredConfig()
{
    m_storedBackgroundMode = ConfigManager::getInstance().getInt("Canvas", "BackgroundMode", 0);
    m_storedBackgroundColorR = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorR", 0.6);
    m_storedBackgroundColorG = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorG", 0.8);
    m_storedBackgroundColorB = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorB", 1.0);
    m_storedGradientTopR = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopR", 0.7);
    m_storedGradientTopG = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopG", 0.7);
    m_storedGradientTopB = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopB", 0.9);
    m_storedGradientBottomR = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomR", 0.5);
    m_storedGradientBottomG = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomG", 0.5);
    m_storedGradientBottomB = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomB", 0.8);
    m_storedBackgroundTexturePath = ConfigManager::getInstance().getString("Canvas", "BackgroundTexturePath", "");
}

void BackgroundConfigListener::onTimer(wxTimerEvent& event)
{
    // Check if any background-related configuration has changed
    bool changed = false;

    int newBackgroundMode = ConfigManager::getInstance().getInt("Canvas", "BackgroundMode", 0);
    if (newBackgroundMode != m_storedBackgroundMode) {
        changed = true;
    }

    double newBackgroundColorR = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorR", 0.6);
    if (newBackgroundColorR != m_storedBackgroundColorR) {
        changed = true;
    }

    double newBackgroundColorG = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorG", 0.8);
    if (newBackgroundColorG != m_storedBackgroundColorG) {
        changed = true;
    }

    double newBackgroundColorB = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorB", 1.0);
    if (newBackgroundColorB != m_storedBackgroundColorB) {
        changed = true;
    }

    double newGradientTopR = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopR", 0.7);
    if (newGradientTopR != m_storedGradientTopR) {
        changed = true;
    }

    double newGradientTopG = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopG", 0.7);
    if (newGradientTopG != m_storedGradientTopG) {
        changed = true;
    }

    double newGradientTopB = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopB", 0.9);
    if (newGradientTopB != m_storedGradientTopB) {
        changed = true;
    }

    double newGradientBottomR = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomR", 0.5);
    if (newGradientBottomR != m_storedGradientBottomR) {
        changed = true;
    }

    double newGradientBottomG = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomG", 0.5);
    if (newGradientBottomG != m_storedGradientBottomG) {
        changed = true;
    }

    double newGradientBottomB = ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomB", 0.8);
    if (newGradientBottomB != m_storedGradientBottomB) {
        changed = true;
    }

    std::string newBackgroundTexturePath = ConfigManager::getInstance().getString("Canvas", "BackgroundTexturePath", "");
    if (newBackgroundTexturePath != m_storedBackgroundTexturePath) {
        changed = true;
    }

    if (changed) {
        LOG_INF_S("BackgroundConfigListener: Background config changed, updating canvas");

        // Update stored values
        updateStoredConfig();

        // Update the canvas background settings
        if (m_canvas) {
            m_canvas->updateBackgroundFromConfig();
            m_canvas->render(true); // Trigger a re-render
        }
    }
}
