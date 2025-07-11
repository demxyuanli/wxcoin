#ifndef FLATUI_TAB_DROPDOWN_H
#define FLATUI_TAB_DROPDOWN_H

#include <wx/wx.h>
#include <vector>
#include "flatui/CustomDropDown.h"

// Forward declarations
class FlatUIPage;
class FlatUIBar;

class FlatUITabDropdown : public wxPanel
{
public:
    FlatUITabDropdown(wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = 0);
    virtual ~FlatUITabDropdown();

    // Menu management
    void UpdateHiddenTabs(const std::vector<size_t>& hiddenIndices);
    void ClearMenu();
    
    // Visibility control
    void ShowDropdown(bool show = true);
    void HideDropdown() { ShowDropdown(false); }
    bool IsDropdownShown() const;
    
    // Position and size
    void SetDropdownRect(const wxRect& rect);
    wxRect GetDropdownRect() const { return m_dropdownRect; }
    
    // Parent bar reference
    void SetParentBar(FlatUIBar* parentBar) { m_parentBar = parentBar; }
    FlatUIBar* GetParentBar() const { return m_parentBar; }

    // Override for best size calculation
    virtual wxSize DoGetBestSize() const override;

protected:
    // Event handlers
    void OnDropdownSelection(wxCommandEvent& event);

private:
    // Setup custom dropdown
    void SetupCustomDropdown();
    void UpdateDropdownItems();
    void ApplyCustomStyling();
    
    // Member variables
    FlatUIBar* m_parentBar;
    CustomDropDown* m_customDropdown;
    std::vector<size_t> m_hiddenTabIndices;
    wxRect m_dropdownRect;

    wxDECLARE_EVENT_TABLE();
};

#endif // FLATUI_TAB_DROPDOWN_H 