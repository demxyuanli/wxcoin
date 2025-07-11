#include "flatui/FlatUITabDropdown.h"
#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIBarStateManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>



wxBEGIN_EVENT_TABLE(FlatUITabDropdown, wxPanel)
    EVT_COMMAND(wxID_ANY, wxEVT_CUSTOM_DROPDOWN_SELECTION, FlatUITabDropdown::OnDropdownSelection)
wxEND_EVENT_TABLE()

FlatUITabDropdown::FlatUITabDropdown(wxWindow* parent, wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style | wxBORDER_NONE),
      m_parentBar(nullptr),
      m_customDropdown(nullptr)
{
    SetName("FlatUITabDropdown");
    SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
    
    SetupCustomDropdown();
    
    LOG_INF("Created FlatUITabDropdown with CustomDropDown", "TabDropdown");
}

FlatUITabDropdown::~FlatUITabDropdown()
{
    // m_customDropdown will be destroyed automatically as a child window
    LOG_INF("Destroyed FlatUITabDropdown", "TabDropdown");
}

void FlatUITabDropdown::UpdateHiddenTabs(const std::vector<size_t>& hiddenIndices)
{
    m_hiddenTabIndices = hiddenIndices;
    UpdateDropdownItems();
    
    // Show or hide the dropdown based on whether there are hidden tabs
    ShowDropdown(!hiddenIndices.empty());
    
    // LOG_INF_S("Updated hidden tabs: " + std::to_string(hiddenIndices.size()) + " tabs", "TabDropdown");
}

void FlatUITabDropdown::ClearMenu()
{
    if (m_customDropdown) {
        m_customDropdown->Clear();
    }
    // Don't clear m_hiddenTabIndices here as it's managed by UpdateHiddenTabs
}

void FlatUITabDropdown::ShowDropdown(bool show)
{
    if (IsShown() != show) {
        Show(show);
        if (show) {
            Refresh();
        }
        LOG_DBG("Dropdown " + std::string(show ? "shown" : "hidden"), "TabDropdown");
    }
}

bool FlatUITabDropdown::IsDropdownShown() const
{
    return IsShown();
}

void FlatUITabDropdown::SetDropdownRect(const wxRect& rect)
{
    m_dropdownRect = rect;
    SetPosition(rect.GetPosition());
    SetSize(rect.GetSize());
}

wxSize FlatUITabDropdown::DoGetBestSize() const
{
    if (m_customDropdown) {
        return m_customDropdown->GetBestSize();
    }
    return wxSize(80, 20); // Default size for custom dropdown
}

void FlatUITabDropdown::OnDropdownSelection(wxCommandEvent& event)
{
    if (!m_parentBar) {
        return;
    }
    
    int selection = event.GetInt();
    
    // Validate selection index
    if (selection >= 0 && selection < static_cast<int>(m_hiddenTabIndices.size())) {
        size_t actualTabIndex = m_hiddenTabIndices[selection];
        
        // Verify the page exists
        FlatUIPage* selectedPage = m_parentBar->GetPage(actualTabIndex);
        if (!selectedPage) {
            return;
        }
        
        // Use the same logic as FlatBarSpaceContainer::HandleTabClick for consistency
        if (m_parentBar->IsBarPinned()) {
            // Pinned state: use SetActivePage directly
            m_parentBar->SetActivePage(actualTabIndex);
        } else {
            // Unpinned state: manually handle like EventDispatcher does
            FlatUIBarStateManager* stateManager = m_parentBar->GetStateManager();
            if (stateManager) {
                stateManager->SetActiveFloatingPage(actualTabIndex);
                stateManager->SetActivePage(actualTabIndex);
                
                // Show the page in float panel
                m_parentBar->ShowPageInFloatPanel(selectedPage);
            }
        }
        
        // Refresh the parent container to update tab display
        wxWindow* parent = GetParent();
        if (parent) {
            parent->Refresh();
        }
        
        LOG_INF_S("Selected hidden tab: " + selectedPage->GetLabel().ToStdString(), "TabDropdown");
    }
}

void FlatUITabDropdown::SetupCustomDropdown()
{
    // Create a sizer to hold the custom dropdown
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create the custom dropdown
    m_customDropdown = new CustomDropDown(this, wxID_ANY, "...");
    
    // Apply custom styling to match the bar theme
    ApplyCustomStyling();
    
    // Add to sizer
    sizer->Add(m_customDropdown, 1, wxEXPAND);
    SetSizer(sizer);
    
    LOG_DBG("Setup custom dropdown", "TabDropdown");
}

void FlatUITabDropdown::UpdateDropdownItems()
{
    if (!m_customDropdown || !m_parentBar) {
        return;
    }
    
    // Clear existing items
    m_customDropdown->Clear();
    
    // Add items for hidden tabs
    size_t totalPageCount = m_parentBar->GetPageCount();
    
    for (size_t tabIndex : m_hiddenTabIndices) {
        // Check if tab index is within valid range
        if (tabIndex >= totalPageCount) {
            continue;
        }
        
        FlatUIPage* page = m_parentBar->GetPage(tabIndex);
        if (page) {
            m_customDropdown->Append(page->GetLabel());
        }
    }
    
    // Set default text if no items
    if (m_customDropdown->GetCount() == 0) {
        m_customDropdown->SetValue("...");
    } else {
        m_customDropdown->SetValue("...");
    }
    
    // LOG_DBG("Updated dropdown items: " + std::to_string(m_customDropdown->GetCount()) + " items", "TabDropdown");
}

void FlatUITabDropdown::ApplyCustomStyling()
{
    if (!m_customDropdown) {
        return;
    }
    
    // Match the bar theme colors
    wxColour barBgColour = CFG_COLOUR("BarBackgroundColour");
    wxColour barTextColour = CFG_COLOUR("BarInactiveTextColour");
    wxColour borderColour = CFG_COLOUR("BarBorderColour");
    
    // Set basic colors
    m_customDropdown->SetBackgroundColour(barBgColour);
    m_customDropdown->SetForegroundColour(barTextColour);
    
    // Flat design: minimal or no border
    m_customDropdown->SetBorderColour(borderColour);
    m_customDropdown->SetBorderWidth(0); // No border for flat design
    
    // Set dropdown button colors to match bar (flat style)
    m_customDropdown->SetDropDownButtonColour(barBgColour);
    // Subtle hover effect for flat design
    wxColour hoverColour = wxColour(
        wxMax(0, barBgColour.Red() - 10),
        wxMax(0, barBgColour.Green() - 10),
        wxMax(0, barBgColour.Blue() - 10)
    );
    m_customDropdown->SetDropDownButtonHoverColour(hoverColour);
    
    // Set popup colors
    m_customDropdown->SetPopupBackgroundColour(CFG_COLOUR("ThemeWhiteColour"));
    m_customDropdown->SetPopupBorderColour(borderColour);
    
    // Set selection colors to match system
    wxColour selectionBg = CFG_COLOUR("BarActiveTabBgColour");
    wxColour selectionFg = CFG_COLOUR("BarActiveTextColour");
    m_customDropdown->SetSelectionBackgroundColour(selectionBg);
    m_customDropdown->SetSelectionForegroundColour(selectionFg);
    
    // Set size constraints
    m_customDropdown->SetMinDropDownWidth(120);
    m_customDropdown->SetMaxDropDownHeight(200);
    
    LOG_DBG("Applied flat styling to dropdown", "TabDropdown");
} 
