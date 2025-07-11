#include "flatui/FlatUIBarEventDispatcher.h"
#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIBarStateManager.h"
#include "flatui/FlatUIPageManager.h"
#include "flatui/FlatUIBarLayoutManager.h"
#include "logger/Logger.h"

FlatUIBarEventDispatcher::FlatUIBarEventDispatcher(FlatUIBar* bar)
    : m_bar(bar),
      m_stateManager(nullptr),
      m_pageManager(nullptr),
      m_layoutManager(nullptr),
      m_initialized(false),
      m_processingEvent(false)
{
    LOG_INF("FlatUIBarEventDispatcher created", "EventDispatcher");
}

void FlatUIBarEventDispatcher::Initialize(FlatUIBarStateManager* stateManager, 
                                        FlatUIPageManager* pageManager, 
                                        FlatUIBarLayoutManager* layoutManager)
{
    m_stateManager = stateManager;
    m_pageManager = pageManager;
    m_layoutManager = layoutManager;
    m_initialized = ValidateComponents();
    
    LOG_INF("EventDispatcher initialized, valid: " + std::string(m_initialized ? "true" : "false"), 
           "EventDispatcher");
}

void FlatUIBarEventDispatcher::HandleTabClick(size_t tabIndex)
{
    if (!CanHandleEvent() || !ValidateTabClick(tabIndex)) {
        LOG_ERR("Cannot handle tab click for index: " + std::to_string(tabIndex), "EventDispatcher");
        return;
    }

    BeginEventProcessing();
    LogEventInfo("TabClick", "Index: " + std::to_string(tabIndex));

    if (m_stateManager->IsPinned()) {
        ProcessPinnedTabClick(tabIndex);
    } else {
        ProcessUnpinnedTabClick(tabIndex);
    }

    UpdateLayoutAfterEvent();
    RefreshDisplayAfterEvent();
    EndEventProcessing();
}

void FlatUIBarEventDispatcher::HandlePinButtonClick()
{
    if (!CanHandleEvent()) {
        LOG_ERR("Cannot handle pin button click", "EventDispatcher");
        return;
    }

    BeginEventProcessing();
    LogEventInfo("PinButtonClick");

    TransitionToPinned();
    BroadcastStateChange(true);

    // Layout update and refresh are handled by OnGlobalPinStateChanged in TransitionToPinned
    // No additional refresh needed to prevent flickering
    EndEventProcessing();
}

void FlatUIBarEventDispatcher::HandleUnpinButtonClick()
{
    if (!CanHandleEvent()) {
        LOG_ERR("Cannot handle unpin button click", "EventDispatcher");
        return;
    }

    BeginEventProcessing();
    LogEventInfo("UnpinButtonClick");

    TransitionToUnpinned();
    BroadcastStateChange(false);

    // Layout update and refresh are handled by OnGlobalPinStateChanged in TransitionToUnpinned
    // No additional refresh needed to prevent flickering
    EndEventProcessing();
}

void FlatUIBarEventDispatcher::HandleSizeEvent(const wxSize& newSize)
{
    LogEventInfo("SizeEvent", "Size: " + std::to_string(newSize.GetWidth()) + 
                "x" + std::to_string(newSize.GetHeight()));
    
    // Size events are handled directly by layout manager
    // No additional processing needed here for now
}

void FlatUIBarEventDispatcher::HandleShowEvent(bool isShown)
{
    LogEventInfo("ShowEvent", isShown ? "shown" : "hidden");
    
    if (isShown && m_layoutManager) {
        m_layoutManager->UpdateLayout(m_bar->GetClientSize());
    }
}

bool FlatUIBarEventDispatcher::ValidateTabClick(size_t tabIndex) const
{
    return tabIndex < m_pageManager->GetPageCount();
}

bool FlatUIBarEventDispatcher::CanHandleEvent() const
{
    return m_initialized && !m_processingEvent && ValidateComponents();
}

void FlatUIBarEventDispatcher::BroadcastStateChange(bool isPinned)
{
    if (!m_bar) return;
    
    // Send backward compatibility event
    wxCommandEvent event(wxEVT_PIN_STATE_CHANGED, m_bar->GetId());
    event.SetEventObject(m_bar);
    event.SetInt(isPinned ? 1 : 0);
    
    wxWindow* parent = m_bar->GetParent();
    if (parent) {
        parent->GetEventHandler()->ProcessEvent(event);
    }
    
    LOG_INF("Broadcasted state change: " + std::string(isPinned ? "pinned" : "unpinned"), 
           "EventDispatcher");
}

void FlatUIBarEventDispatcher::BroadcastPageChange(size_t oldIndex, size_t newIndex)
{
    LOG_INF("Broadcasted page change: " + std::to_string(oldIndex) + " -> " + std::to_string(newIndex), 
           "EventDispatcher");
}

void FlatUIBarEventDispatcher::ProcessPinnedTabClick(size_t tabIndex)
{
    if (m_stateManager->GetActivePage() == tabIndex) {
        LOG_INF("Tab already active in pinned state, ignoring", "EventDispatcher");
        return;
    }
    
    // Use FlatUIBar's SetActivePage method to ensure proper page switching
    // This will handle both state management and FixPanel page display
    if (m_bar) {
        m_bar->SetActivePage(tabIndex);
        LOG_INF("Called FlatUIBar::SetActivePage for pinned tab click: " + std::to_string(tabIndex), "EventDispatcher");
    } else {
        // Fallback if bar is not available
        m_stateManager->SetActivePage(tabIndex);
        m_pageManager->SetAllPagesInactive();
        m_pageManager->SetPageActive(tabIndex, true);
        LOG_WRN("Used fallback logic for pinned tab click: " + std::to_string(tabIndex), "EventDispatcher");
    }
    
    LOG_INF("Processed pinned tab click for index: " + std::to_string(tabIndex), "EventDispatcher");
}

void FlatUIBarEventDispatcher::ProcessUnpinnedTabClick(size_t tabIndex)
{
    m_stateManager->SetActiveFloatingPage(tabIndex);
    m_stateManager->SetActivePage(tabIndex); // Keep synchronized
    
    // Get the page and show it in float panel
    FlatUIPage* page = m_pageManager->GetPage(tabIndex);
    if (page && m_bar) {
        m_bar->ShowPageInFloatPanel(page);
        LOG_INF("Showed page '" + page->GetLabel().ToStdString() + "' in float panel", "EventDispatcher");
    }
    
    LOG_INF("Processed unpinned tab click for index: " + std::to_string(tabIndex), "EventDispatcher");
}

void FlatUIBarEventDispatcher::TransitionToPinned()
{
    // Get the currently active floating page before transition
    size_t activeFloatingPage = m_stateManager->GetActiveFloatingPage();
    
    // Transition state first
    m_stateManager->TransitionTo(FlatUIBarStateManager::BarState::PINNED);
    
    // Hide the float panel
    if (m_bar) {
        m_bar->HideFloatPanel();
        LOG_INF("Hidden float panel during transition to pinned", "EventDispatcher");
    }
    
    // If there was an active floating page, make sure it becomes the active pinned page
    if (activeFloatingPage != static_cast<size_t>(-1)) {
        m_stateManager->SetActivePage(activeFloatingPage);
        m_pageManager->SetAllPagesInactive();
        m_pageManager->SetPageActive(activeFloatingPage, true);
        LOG_INF("Set active page to " + std::to_string(activeFloatingPage) + " after pinning", "EventDispatcher");
    }
    
    // Call the bar's OnGlobalPinStateChanged to handle UI updates
    if (m_bar) {
        m_bar->OnGlobalPinStateChanged(true);
    }
    
    LOG_INF("Transitioned to pinned state", "EventDispatcher");
}

void FlatUIBarEventDispatcher::TransitionToUnpinned()
{
    // Save the current active page before transition
    size_t currentActivePage = m_stateManager->GetActivePage();
    
    // Transition state first
    m_stateManager->TransitionTo(FlatUIBarStateManager::BarState::UNPINNED);
    
    // Call the bar's OnGlobalPinStateChanged to handle UI updates (hide fix panel, etc.)
    if (m_bar) {
        m_bar->OnGlobalPinStateChanged(false);
    }
    
    // Reset floating page state
    m_stateManager->SetActiveFloatingPage(static_cast<size_t>(-1));
    
    LOG_INF("Transitioned to unpinned state, active page preserved: " + std::to_string(currentActivePage), "EventDispatcher");
}

void FlatUIBarEventDispatcher::UpdateLayoutAfterEvent()
{
    if (m_layoutManager && m_bar) {
        m_layoutManager->UpdateLayoutIfNeeded(m_bar->GetClientSize());
    }
}

void FlatUIBarEventDispatcher::RefreshDisplayAfterEvent()
{
    if (m_layoutManager) {
        m_layoutManager->DeferredRefresh();
    }
}

void FlatUIBarEventDispatcher::BeginEventProcessing()
{
    m_processingEvent = true;
}

void FlatUIBarEventDispatcher::EndEventProcessing()
{
    m_processingEvent = false;
}

bool FlatUIBarEventDispatcher::ValidateComponents() const
{
    return m_bar != nullptr && 
           m_stateManager != nullptr && 
           m_pageManager != nullptr && 
           m_layoutManager != nullptr;
}

void FlatUIBarEventDispatcher::HandleTabAreaClick(const wxPoint& position)
{
    if (!CanHandleEvent()) {
        LOG_ERR("Cannot handle tab area click", "EventDispatcher");
        return;
    }

    BeginEventProcessing();
    LogEventInfo("TabAreaClick", "Position: (" + std::to_string(position.x) + "," + std::to_string(position.y) + ")");

    // Get tab index from position
    size_t tabIndex = GetTabIndexFromPosition(position);
    if (tabIndex != static_cast<size_t>(-1)) {
        HandleTabClick(tabIndex);
    }

    EndEventProcessing();
}

size_t FlatUIBarEventDispatcher::GetTabIndexFromPosition(const wxPoint& position) const
{
    // This would need to be implemented based on the tab layout logic
    // For now, return invalid index
    LOG_WRN("GetTabIndexFromPosition not implemented", "EventDispatcher");
    return static_cast<size_t>(-1);
}

bool FlatUIBarEventDispatcher::IsPositionInTabArea(const wxPoint& position) const
{
    // This would need to be implemented based on the tab area bounds
    // For now, return false
    LOG_WRN("IsPositionInTabArea not implemented", "EventDispatcher");
    return false;
}

bool FlatUIBarEventDispatcher::IsPositionInBarArea(const wxPoint& position) const
{
    if (!m_bar) return false;
    
    wxRect barRect = m_bar->GetRect();
    return barRect.Contains(position);
}

void FlatUIBarEventDispatcher::LogEventInfo(const wxString& eventType, const wxString& details) const
{
    wxString logMessage = "Event: " + eventType;
    if (!details.IsEmpty()) {
        logMessage += " - " + details;
    }
    LOG_INF(logMessage.ToStdString(), "EventDispatcher");
} 
