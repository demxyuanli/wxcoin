#ifndef DOCK_SYSTEM_BUTTONS_H
#define DOCK_SYSTEM_BUTTONS_H

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/graphics.h>
#include <vector>
#include <memory>

class ModernDockPanel;

// System button types for dock panels
enum class DockSystemButtonType {
    MINIMIZE,       // Minimize button
    MAXIMIZE,       // Maximize/Restore button
    CLOSE,          // Close button
    PIN,            // Pin/Unpin button
    FLOAT,          // Float button
    DOCK            // Dock button
};

// System button configuration
struct DockSystemButtonConfig {
    DockSystemButtonType type;
    wxString tooltip;
    wxBitmap icon;
    wxBitmap hoverIcon;
    wxBitmap pressedIcon;
    bool enabled;
    bool visible;
    
    DockSystemButtonConfig(DockSystemButtonType t, const wxString& tip = wxEmptyString)
        : type(t), tooltip(tip), enabled(true), visible(true) {}
};

// System buttons panel for dock panels
class DockSystemButtons : public wxPanel {
public:
    DockSystemButtons(ModernDockPanel* parent, wxWindowID id = wxID_ANY);
    ~DockSystemButtons() override;
    
    // Button management
    void AddButton(DockSystemButtonType type, const wxString& tooltip = wxEmptyString);
    void RemoveButton(DockSystemButtonType type);
    void SetButtonEnabled(DockSystemButtonType type, bool enabled);
    void SetButtonVisible(DockSystemButtonType type, bool visible);
    void SetButtonIcon(DockSystemButtonType type, const wxBitmap& icon);
    void SetButtonTooltip(DockSystemButtonType type, const wxString& tooltip);
    
    // Layout
    void UpdateLayout();
    wxSize GetBestSize() const;
    
    // Theme support
    void UpdateThemeColors();
    void OnThemeChanged();
    
    // Event handling
    void OnButtonClick(wxCommandEvent& event);
    void OnButtonHover(wxMouseEvent& event);
    void OnButtonLeave(wxMouseEvent& event);
    
protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    
private:
    void InitializeButtons();
    void CreateButton(DockSystemButtonType type, const wxString& tooltip);
    void RenderButton(wxGraphicsContext* gc, const wxRect& rect, const DockSystemButtonConfig& config, bool hovered, bool pressed);
    wxRect GetButtonRect(int index) const;
    int GetButtonIndex(DockSystemButtonType type) const;
    wxString GetIconName(DockSystemButtonType type) const;
    
    ModernDockPanel* m_parent;
    std::vector<DockSystemButtonConfig> m_buttons;
    std::vector<wxButton*> m_buttonControls;
    
    // Layout
    int m_buttonSize;
    int m_buttonSpacing;
    int m_margin;
    
    // Colors (theme-aware)
    wxColour m_backgroundColor;
    wxColour m_buttonBgColor;
    wxColour m_buttonHoverColor;
    wxColour m_buttonPressedColor;
    wxColour m_buttonBorderColor;
    wxColour m_buttonTextColor;
    
    // State
    int m_hoveredButtonIndex;
    int m_pressedButtonIndex;
    
    // Constants
    static constexpr int DEFAULT_BUTTON_SIZE = 18;  // Slightly smaller for title bar
    static constexpr int DEFAULT_BUTTON_SPACING = 1; // Tighter spacing
    static constexpr int DEFAULT_MARGIN = 2;        // Smaller margin
    
    wxDECLARE_EVENT_TABLE();
};

#endif // DOCK_SYSTEM_BUTTONS_H
