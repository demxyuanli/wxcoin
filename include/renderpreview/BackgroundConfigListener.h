#pragma once

#include <wx/timer.h>
#include <string>

class PreviewCanvas;

/**
 * @brief Configuration listener for background settings changes
 *
 * This listener monitors configuration changes in the "Canvas" section
 * and updates the PreviewCanvas background accordingly using a timer-based approach.
 */
class BackgroundConfigListener : public wxEvtHandler {
public:
    /**
     * @brief Constructor
     * @param canvas The PreviewCanvas to update when config changes
     */
    explicit BackgroundConfigListener(PreviewCanvas* canvas);

    /**
     * @brief Destructor
     */
    ~BackgroundConfigListener();

private:
    /**
     * @brief Timer event handler to check for configuration changes
     * @param event The timer event
     */
    void onTimer(wxTimerEvent& event);

    /**
     * @brief Update stored configuration values
     */
    void updateStoredConfig();

    PreviewCanvas* m_canvas;  ///< The canvas to update
    wxTimer* m_timer;         ///< Timer for periodic checking

    // Stored configuration values for comparison
    int m_storedBackgroundMode;
    double m_storedBackgroundColorR;
    double m_storedBackgroundColorG;
    double m_storedBackgroundColorB;
    double m_storedGradientTopR;
    double m_storedGradientTopG;
    double m_storedGradientTopB;
    double m_storedGradientBottomR;
    double m_storedGradientBottomG;
    double m_storedGradientBottomB;
    std::string m_storedBackgroundTexturePath;
};
