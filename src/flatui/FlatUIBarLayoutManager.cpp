#include "flatui/FlatUIBarLayoutManager.h"
#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIBarConfig.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/dcmemory.h>
#include <wx/button.h>



FlatUIBarLayoutManager::FlatUIBarLayoutManager(FlatUIBar* bar)
    : m_bar(bar),
      m_layoutValid(false),
      m_layoutDirty(false),
      m_refreshPending(false)
{
    LOG_INF("FlatUIBarLayoutManager initialized", "LayoutManager");
}

void FlatUIBarLayoutManager::UpdateLayout(const wxSize& barClientSize)
{
    if (!m_bar || barClientSize.GetWidth() <= 0 || barClientSize.GetHeight() <= 0) {
        LOG_ERR("Invalid parameters for UpdateLayout", "LayoutManager");
        return;
    }
    
    // Get all child components from the bar
    FlatUIHomeSpace* homeSpace = m_bar->GetHomeSpace();
    FlatUISystemButtons* systemButtons = m_bar->GetSystemButtons();
    FlatUIFunctionSpace* functionSpace = m_bar->GetFunctionSpace();
    FlatUIProfileSpace* profileSpace = m_bar->GetProfileSpace();
    FlatUISpacerControl* tabFunctionSpacer = m_bar->GetTabFunctionSpacer();
    FlatUISpacerControl* functionProfileSpacer = m_bar->GetFunctionProfileSpacer();
    FlatUIFixPanel* fixPanel = m_bar->GetFixPanel();
    
    // Comprehensive component readiness check
    bool homeSpaceReady = (homeSpace != nullptr);
    bool systemButtonsReady = (systemButtons != nullptr);
    bool functionSpaceReady = (functionSpace != nullptr);
    bool profileSpaceReady = (profileSpace != nullptr);
    bool fixPanelReady = (fixPanel != nullptr);
    
    // Log component status for debugging
    if (!homeSpaceReady || !systemButtonsReady || !functionSpaceReady || !profileSpaceReady) {
        LOG_WRN("Component readiness check: HomeSpace=" + std::string(homeSpaceReady ? "ready" : "not ready") +
                ", SystemButtons=" + std::string(systemButtonsReady ? "ready" : "not ready") +
                ", FunctionSpace=" + std::string(functionSpaceReady ? "ready" : "not ready") +
                ", ProfileSpace=" + std::string(profileSpaceReady ? "ready" : "not ready"), "LayoutManager");
    }
    
    // Check if basic components are ready - be more lenient and continue with partial layout
    if (!homeSpaceReady) {
        LOG_WRN("HomeSpace not ready for layout, scheduling delayed update", "LayoutManager");
        // Schedule a delayed update instead of failing
        if (m_bar) {
            m_bar->CallAfter([this, barClientSize]() {
                if (m_bar && m_bar->GetHomeSpace()) {
                    UpdateLayout(barClientSize);
                }
            });
        }
        return;
    }
    
    // Check if other critical components are ready
    bool componentsReady = systemButtonsReady && functionSpaceReady && profileSpaceReady;
    if (!componentsReady) {
        LOG_WRN("Some core components not ready for layout, using minimal layout", "LayoutManager");
        // Continue with minimal layout instead of returning
    }

    // Cache the new size
    m_lastBarSize = barClientSize;
    m_layoutDirty = false;
    
    // Create a temporary DC for calculations
    wxMemoryDC dc;
    wxBitmap tempBitmap(1, 1);
    dc.SelectObject(tempBitmap);
    
    // Basic layout parameters
    int barPadding = GetBarPadding();
    int elemSpacing = GetElementSpacing();
    int currentX = barPadding;
    int barStripHeight = m_bar->GetBarHeight();
    int barTopMargin = m_bar->GetBarTopMargin();
    int barBottomMargin = m_bar->GetBarBottomMargin();
    int innerHeight = barStripHeight - barTopMargin - barBottomMargin;
    int elementY = barTopMargin;
    
    // Debug: Log bar measurements
    // LOG_INF_S("Bar measurements: barStripHeight=" + std::to_string(barStripHeight) + 
    //        ", barTopMargin=" + std::to_string(barTopMargin) + 
    //        ", barBottomMargin=" + std::to_string(barBottomMargin) + 
    //        ", innerHeight=" + std::to_string(innerHeight), "LayoutManager");

    // Skip HomeSpace and SystemButtons - they are now managed by BarContainer
    // Just calculate their widths for tab area calculation
    int homeSpaceWidth = 0;
    if (homeSpace && homeSpace->IsShown()) {
        homeSpaceWidth = homeSpace->GetButtonWidth() + elemSpacing;
        currentX += homeSpaceWidth;
        // LOG_INF_S("HomeSpace width (managed by container): " + std::to_string(homeSpaceWidth), "LayoutManager");
    }

    // System Buttons (Rightmost) - Just calculate width for boundary calculation
    int sysButtonsWidth = 0;
    if (systemButtons && systemButtons->IsShown()) {
        sysButtonsWidth = systemButtons->GetRequiredWidth();
        // LOG_INF_S("SystemButtons width (managed by container): " + std::to_string(sysButtonsWidth), "LayoutManager");
    }

    // Calculate right boundary for flexible elements (excluding system buttons)
    int rightBoundaryForFlexibleElements = barClientSize.GetWidth() - barPadding;
    if (sysButtonsWidth > 0) {
        rightBoundaryForFlexibleElements -= (sysButtonsWidth + elemSpacing);
    }

    // Tabs are now managed by FlatBarSpaceContainer
    // No longer calculate tab area here
    // LOG_INF("Tab area calculation skipped - managed by FlatBarSpaceContainer", "LayoutManager");

    // Function and Profile spaces are now managed by BarContainer
    // Skip their positioning logic entirely
    // LOG_INF("FunctionSpace and ProfileSpace are managed by BarContainer", "LayoutManager");

    // Spacers are still managed by the old layout manager for now
    // Hide them since we're using the container approach
    if (tabFunctionSpacer) {
        tabFunctionSpacer->Show(false);
    }
    if (functionProfileSpacer) {
        functionProfileSpacer->Show(false);
    }

    // FixPanel handling based on pin state - only if components are ready
    if (fixPanel && m_bar->GetStateManager() && componentsReady) {
        bool shouldShowFixPanel = m_bar->GetStateManager()->IsPinned();
        
        if (shouldShowFixPanel) {
            // Calculate the correct Y position for FixPanel
            // Use the bar strip height + margins for proper positioning 
            int totalBarHeight = barStripHeight + barBottomMargin;
            int FIXED_PANEL_Y = totalBarHeight;
            
            // Safety check: Ensure we have a minimum Y position
            if (FIXED_PANEL_Y <= 0) {
                FIXED_PANEL_Y = 31; // Standard fallback based on common bar height
                LOG_INF("WARNING: Calculated bar height is " + std::to_string(totalBarHeight) + 
                       ", using fallback Y position " + std::to_string(FIXED_PANEL_Y), "LayoutManager");
            }
            
            // Calculate proposed FixPanel height
            int fixPanelHeight = barClientSize.GetHeight() - FIXED_PANEL_Y;
            
            // Minimum viable height check to prevent invisible or overlapping panels
            const int MIN_FIXPANEL_HEIGHT = 30; // Must be at least 20px to be useful
            if (fixPanelHeight < MIN_FIXPANEL_HEIGHT) {
                
                // Schedule a delayed layout update to retry when window size is proper
                m_bar->CallAfter([this]() {
                    if (m_bar && m_bar->GetSize().GetHeight() > 50) { // Minimum reasonable window height
                        UpdateLayout(m_bar->GetSize());
                    }
                });
                return; // Don't position with invalid height
            }
            
            // Debug logging
            // LOG_INF_S("FixPanel positioning: barStripHeight=" + std::to_string(barStripHeight) + 
            //        ", barTopMargin=" + std::to_string(barTopMargin) + 
            //        ", barBottomMargin=" + std::to_string(barBottomMargin) + 
            //        ", totalBarHeight=" + std::to_string(totalBarHeight) + 
            //        ", FIXED_PANEL_Y=" + std::to_string(FIXED_PANEL_Y), "LayoutManager");
            
            fixPanel->SetPosition(wxPoint(0, FIXED_PANEL_Y));
            fixPanel->SetSize(barClientSize.GetWidth(), fixPanelHeight);

            // LOG_INF("Positioned FixPanel at (0, " + std::to_string(FIXED_PANEL_Y) + 
            //        ") with size (" + std::to_string(barClientSize.GetWidth()) + 
            //        ", " + std::to_string(fixPanelHeight) + ")", "LayoutManager");

            if (!fixPanel->IsShown()) {
                fixPanel->Show();
                // LOG_INF("Showed FixPanel", "LayoutManager");
            }

            // Set active page in FixPanel
            if (m_bar->GetPageCount() > 0) {
                size_t activePage = m_bar->GetActivePage();
                fixPanel->SetActivePage(activePage);
            }
        } else {
            // Hide FixPanel in unpinned state
            if (fixPanel->IsShown()) {
                fixPanel->Hide();
                // LOG_INF("Hidden FixPanel", "LayoutManager");
            }
        }
    }

    // LOG_INF("Layout update completed", "LayoutManager");
    m_layoutValid = true;
    
    // Only use deferred refresh if not called during state transition
    // During state transitions, the calling code manages refresh timing
    if (m_bar && !m_bar->IsFrozen()) {
        // Only refresh if the window is not frozen (indicating no ongoing state transition)
        DeferredRefresh();
    } else {
        // LOG_INF_S("Skipping DeferredRefresh during state transition (window frozen)", "LayoutManager");
    }
}

void FlatUIBarLayoutManager::UpdateLayoutIfNeeded(const wxSize& barClientSize)
{
    if (m_layoutDirty || barClientSize != m_lastBarSize || !m_layoutValid) {
        UpdateLayout(barClientSize);
    }
}

void FlatUIBarLayoutManager::MarkLayoutDirty()
{
    m_layoutDirty = true;
}

void FlatUIBarLayoutManager::ForceRefresh()
{
    if (m_bar && m_bar->IsShown()) {
        m_bar->Refresh();
        m_bar->Update();
    }
}

void FlatUIBarLayoutManager::DeferredRefresh()
{
    if (!m_refreshPending && m_bar && m_bar->IsShown()) {
        m_refreshPending = true;
        m_bar->CallAfter([this]() {
            if (m_refreshPending && m_bar && m_bar->IsShown()) {
                m_bar->Refresh();
                m_refreshPending = false;
            }
        });
    }
}

int FlatUIBarLayoutManager::CalculateTabsWidth(wxDC& dc) const
{
    if (!m_bar) return 0;
    
    int tabPadding = CFG_INT("BarTabPadding");
    int tabSpacing = CFG_INT("BarTabSpacing");
    int totalWidth = 0;
    
    size_t pageCount = m_bar->GetPageCount();
    if (pageCount == 0) return 0;

    for (size_t i = 0; i < pageCount; ++i) {
        FlatUIPage* page = m_bar->GetPage(i);
        if (!page) continue;
        
        wxString label = page->GetLabel();
        wxSize labelSize = dc.GetTextExtent(label);
        totalWidth += labelSize.GetWidth() + tabPadding * 2;
        
        if (i < pageCount - 1) {
            totalWidth += tabSpacing;
        }
    }
    
    return totalWidth;
}

TabLayoutParams FlatUIBarLayoutManager::CalculateVisibleTabs(wxDC& dc, int availableWidth) const
{
    TabLayoutParams result;
    if (!m_bar) return result;

    int tabPadding = CFG_INT("BarTabPadding");
    int tabSpacing = CFG_INT("BarTabSpacing");
    int currentWidth = 0;
    size_t pageCount = m_bar->GetPageCount();

    for (size_t i = 0; i < pageCount; ++i) {
        FlatUIPage* page = m_bar->GetPage(i);
        if (!page) continue;

        wxString label = page->GetLabel();
        wxSize labelSize = dc.GetTextExtent(label);
        int tabWidth = labelSize.GetWidth() + tabPadding * 2;
        int widthWithSpacing = tabWidth + (result.visibleCount > 0 ? tabSpacing : 0);

        if (result.visibleCount == 0 || (currentWidth + widthWithSpacing) <= availableWidth) {
            currentWidth += widthWithSpacing;
            result.visibleCount++;
        } else {
            result.hiddenIndices.push_back(i);
        }
    }
    
    result.visibleWidth = currentWidth;
    return result;
}

wxRect FlatUIBarLayoutManager::CalculateTabAreaRect(int currentX, int elementY, int tabsWidth, int barHeight) const
{
    if (tabsWidth > 0) {
        return wxRect(currentX, elementY, tabsWidth, barHeight);
    }
    return wxRect();
}

void FlatUIBarLayoutManager::UpdateElementVisibility()
{
    // Placeholder - will be implemented as we refactor more
}

void FlatUIBarLayoutManager::ShowElement(wxWindow* element, bool show)
{
    if (element && element->IsShown() != show) {
        element->Show(show);
    }
}

bool FlatUIBarLayoutManager::ValidateLayout() const
{
    return m_layoutValid && m_bar != nullptr;
}

void FlatUIBarLayoutManager::LogLayoutInfo(const wxString& context) const
{
}

int FlatUIBarLayoutManager::GetElementSpacing() const
{
    return FlatUIBarConfig::ELEMENT_SPACING;
}

int FlatUIBarLayoutManager::GetBarPadding() const
{
    return FlatUIBarConfig::BAR_PADDING;
}

bool FlatUIBarLayoutManager::ShouldShowElement(wxWindow* element) const
{
    return element != nullptr && element->IsShown();
} 
