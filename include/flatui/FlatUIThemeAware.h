#ifndef FLATUI_THEME_AWARE_H
#define FLATUI_THEME_AWARE_H

#include <wx/wx.h>
#include "config/ThemeManager.h"

/**
 * @brief Base class for all FlatUI components that need theme awareness
 * 
 * This class provides standardized theme change listening and refresh mechanism.
 * Any FlatUI component that needs to respond to theme changes should inherit from this class.
 */
class FlatUIThemeAware : public wxControl
{
public:
    /**
     * @brief Constructor for theme-aware controls
     * @param parent Parent window
     * @param id Window ID
     * @param pos Position
     * @param size Size
     * @param style Window style
     * @param name Window name
     */
    FlatUIThemeAware(wxWindow* parent, wxWindowID id = wxID_ANY, 
                     const wxPoint& pos = wxDefaultPosition, 
                     const wxSize& size = wxDefaultSize, 
                     long style = 0, 
                     const wxString& name = wxControlNameStr)
        : wxControl(parent, id, pos, size, style, wxDefaultValidator, name)
    {
        // Register theme change listener
        ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
            OnThemeChanged();
        });
    }

    /**
     * @brief Virtual destructor - removes theme listener
     */
    virtual ~FlatUIThemeAware()
    {
        // Remove theme change listener
        ThemeManager::getInstance().removeThemeChangeListener(this);
    }

    /**
     * @brief Called when theme changes - derived classes should override this
     * 
     * This method is automatically called when the theme changes.
     * Derived classes should override this method to implement their specific
     * theme refresh logic.
     */
    virtual void OnThemeChanged()
    {
        // Default implementation - just refresh the control
        Refresh(true);
        Update();
    }

    /**
     * @brief Public method to manually trigger theme refresh
     * 
     * This can be called externally to force a theme refresh.
     */
    void RefreshTheme()
    {
        OnThemeChanged();
    }

    /**
     * @brief Batch refresh method - updates theme values without immediate refresh
     * 
     * This method should be overridden by derived classes to update theme values
     * without triggering immediate refresh. The actual refresh will be handled
     * by the parent frame to avoid multiple refreshes.
     */
    virtual void UpdateThemeValues()
    {
        // Default implementation - just call OnThemeChanged
        OnThemeChanged();
    }

    /**
     * @brief Check if this control needs refresh after theme change
     * 
     * @return true if the control needs refresh, false otherwise
     */
    virtual bool NeedsThemeRefresh() const
    {
        return true;
    }

protected:
    /**
     * @brief Helper method to get current theme color
     * @param key Color key
     * @return Theme color
     */
    wxColour GetThemeColour(const std::string& key) const
    {
        return CFG_COLOUR(key);
    }

    /**
     * @brief Helper method to get current theme integer value
     * @param key Integer key
     * @return Theme integer value
     */
    int GetThemeInt(const std::string& key) const
    {
        return CFG_INT(key);
    }

    /**
     * @brief Helper method to get current theme string value
     * @param key String key
     * @return Theme string value
     */
    std::string GetThemeString(const std::string& key) const
    {
        return CFG_STRING(key);
    }

    /**
     * @brief Helper method to get current theme font
     * @return Theme font
     */
    wxFont GetThemeFont() const
    {
        return CFG_DEFAULTFONT();
    }

    /**
     * @brief Batch refresh method - updates theme values and marks for refresh
     * 
     * This method updates theme values and marks the control for refresh
     * without immediately calling Refresh(). The actual refresh will be
     * handled by the parent frame.
     */
    void BatchUpdateTheme()
    {
        UpdateThemeValues();
        // Mark for refresh but don't call Refresh() immediately
        // The parent frame will handle the actual refresh
    }

private:
    // Disable copy constructor and assignment operator
    FlatUIThemeAware(const FlatUIThemeAware&) = delete;
    FlatUIThemeAware& operator=(const FlatUIThemeAware&) = delete;
};

#endif // FLATUI_THEME_AWARE_H 