#include "docking/FloatingDockContainer.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"
#include "docking/DockOverlay.h"
#include "config/ThemeManager.h"
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>

namespace ads {

// Define custom events
wxEventTypeTag<wxCommandEvent> FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSING(wxNewEventType());
wxEventTypeTag<wxCommandEvent> FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSED(wxNewEventType());

// Event table
wxBEGIN_EVENT_TABLE(FloatingDockContainer, wxFrame)
    EVT_CLOSE(FloatingDockContainer::onClose)
    EVT_PAINT(FloatingDockContainer::onPaint)
    EVT_LEFT_DOWN(FloatingDockContainer::onMouseLeftDown)
    EVT_LEFT_UP(FloatingDockContainer::onMouseLeftUp)
    EVT_MOTION(FloatingDockContainer::onMouseMove)
    EVT_LEFT_DCLICK(FloatingDockContainer::onMouseDoubleClick)
    EVT_MAXIMIZE(FloatingDockContainer::onMaximize)
wxEND_EVENT_TABLE()

// Private implementation
class FloatingDockContainer::Private {
public:
    Private(FloatingDockContainer* parent) : q(parent) {}
    
    FloatingDockContainer* q;
    bool isDragging = false;
    wxPoint dragOffset;
};

// Constructors
FloatingDockContainer::FloatingDockContainer(DockManager* dockManager)
    : wxFrame(dockManager ? dockManager->containerWidget()->GetParent() : nullptr,
             wxID_ANY, "Floating Dock",
             wxDefaultPosition, wxSize(400, 300),
             wxFRAME_NO_TASKBAR | wxBORDER_NONE) // No system title bar
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockManager)
    , m_hasNativeTitleBar(false) // Use DockArea's title bar
    , m_dragState(DraggingInactive)
    , m_dragPreview(nullptr)
{
    init();
}

FloatingDockContainer::FloatingDockContainer(DockArea* dockArea)
    : wxFrame(dockArea && dockArea->dockManager() ? dockArea->dockManager()->containerWidget()->GetParent() : nullptr,
             wxID_ANY, "Floating Dock",
             wxDefaultPosition, dockArea ? dockArea->GetSize() : wxSize(400, 300),
             wxFRAME_NO_TASKBAR | wxBORDER_NONE) // No system title bar
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockArea ? dockArea->dockManager() : nullptr)
    , m_hasNativeTitleBar(false) // Use DockArea's title bar
    , m_dragState(DraggingInactive)
    , m_dragPreview(nullptr)
{
    init();
    
    // Add the dock area
    if (dockArea && m_dockContainer) {
        // Remove from old container
        if (dockArea->dockContainer()) {
            dockArea->dockContainer()->removeDockArea(dockArea);
        }
        
        // Add to floating container
        m_dockContainer->addDockArea(dockArea, CenterDockWidgetArea);
        dockArea->Reparent(m_dockContainer);
        
        // Ensure proper layout after adding content
        m_dockContainer->Layout();
        Layout();
        Refresh();
        
        // Update window title
        updateWindowTitle();
    }
}

FloatingDockContainer::FloatingDockContainer(DockWidget* dockWidget)
    : wxFrame(dockWidget && dockWidget->dockManager() ? dockWidget->dockManager()->containerWidget()->GetParent() : nullptr,
             wxID_ANY,
             dockWidget ? dockWidget->title() : "Floating Dock",
             wxDefaultPosition, dockWidget ? dockWidget->GetSize() : wxSize(400, 300),
             wxFRAME_NO_TASKBAR | wxBORDER_NONE) // No system title bar
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockWidget ? dockWidget->dockManager() : nullptr)
    , m_hasNativeTitleBar(false) // Use DockArea's title bar
    , m_dragState(DraggingInactive)
    , m_dragPreview(nullptr)
{
    init();
    
    // Add the dock widget
    if (dockWidget) {
        addDockWidget(dockWidget);
    }
}

FloatingDockContainer::~FloatingDockContainer() {
    // Notify manager
    if (m_dockManager) {
        m_dockManager->onFloatingWidgetAboutToClose(this);
    }
}

void FloatingDockContainer::init() {
    // Set background style for custom drawing
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // Create dock container
    m_dockContainer = new DockContainerWidget(m_dockManager, this);
    m_dockContainer->setFloatingWidget(this);

    // Set up layout to make dock container fill the entire window
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_dockContainer, 1, wxEXPAND);
    SetSizer(sizer);

    // Ensure dock container is properly sized and visible
    m_dockContainer->Show(true);
    m_dockContainer->SetMinSize(wxSize(200, 150)); // Set minimum size
    
    // Force layout update
    Layout();
    Refresh();

    // Set proper window behavior for always-on-top
    SetWindowStyleFlag(GetWindowStyleFlag() | wxSTAY_ON_TOP);
    Raise(); // Ensure it's on top immediately
    SetFocus(); // Ensure focus for proper z-order

    // Bind close event to handle proper cleanup
    Bind(wxEVT_CLOSE_WINDOW, &FloatingDockContainer::onClose, this);

    // Register with manager
    if (m_dockManager) {
        m_dockManager->onFloatingWidgetCreated(this);
    }
}

void FloatingDockContainer::addDockWidget(DockWidget* dockWidget) {
    if (!dockWidget || !m_dockContainer) {
        return;
    }
    
    m_dockContainer->addDockWidget(CenterDockWidgetArea, dockWidget);
    updateWindowTitle();
    
    // Ensure proper layout after adding content
    m_dockContainer->Layout();
    Layout();
    Refresh();
    
    // Make sure the window is shown after content is added
    Show(true);
    Raise(); // Always bring to front to ensure it's on top
    SetFocus(); // Ensure focus for proper z-order
}

void FloatingDockContainer::removeDockWidget(DockWidget* dockWidget) {
    if (!dockWidget || !m_dockContainer) {
        return;
    }
    
    m_dockContainer->removeDockWidget(dockWidget);
    
    // Close if empty
    if (m_dockContainer->dockAreaCount() == 0) {
        Close();
    }
}

bool FloatingDockContainer::isClosable() const {
    // Check if all dock widgets are closable
    std::vector<DockWidget*> widgets = dockWidgets();
    for (auto* widget : widgets) {
        if (!widget->hasFeature(DockWidgetClosable)) {
            return false;
        }
    }
    return true;
}

bool FloatingDockContainer::hasTopLevelDockWidget() const {
    return m_dockContainer && m_dockContainer->dockAreaCount() == 1 &&
           m_dockContainer->dockArea(0)->dockWidgetsCount() == 1;
}

DockWidget* FloatingDockContainer::topLevelDockWidget() const {
    if (hasTopLevelDockWidget()) {
        return m_dockContainer->dockArea(0)->currentDockWidget();
    }
    return nullptr;
}

std::vector<DockWidget*> FloatingDockContainer::dockWidgets() const {
    std::vector<DockWidget*> result;
    
    if (m_dockContainer) {
        for (auto* area : m_dockContainer->dockAreas()) {
            for (auto* widget : area->dockWidgets()) {
                result.push_back(widget);
            }
        }
    }
    
    return result;
}

void FloatingDockContainer::updateWindowTitle() {
    if (hasTopLevelDockWidget()) {
        SetTitle(topLevelDockWidget()->title());
    } else {
        SetTitle("Floating Dock");
    }
}

void FloatingDockContainer::setNativeTitleBar(bool native) {
    // Always use DockArea's title bar now
    m_hasNativeTitleBar = false;
}

void FloatingDockContainer::saveState(wxString& xmlData) const {
    // TODO: Implement state saving
    xmlData = "<FloatingContainer />";
}

bool FloatingDockContainer::restoreState(const wxString& xmlData) {
    // TODO: Implement state restoration
    return true;
}

void FloatingDockContainer::startFloating(const wxPoint& dragStartPos, const wxSize& size, 
                                         eDragState dragState, wxWindow* mouseEventHandler) {
    m_dragStartPos = dragStartPos;
    m_dragState = dragState;
    SetSize(size);
}

void FloatingDockContainer::moveFloating() {
    if (m_dragState != DraggingInactive) {
        wxPoint mousePos = wxGetMousePosition();
        SetPosition(mousePos - m_dragStartPos);
    }
}

void FloatingDockContainer::finishDragging() {
    m_dragState = DraggingInactive;
}

bool FloatingDockContainer::isInTitleBar(const wxPoint& pos) const {
    // Check if we're in DockArea's title bar
    if (m_dockContainer && m_dockContainer->dockAreaCount() > 0) {
        DockArea* area = m_dockContainer->dockArea(0);
        if (area && area->titleBar()) {
            wxPoint clientPos = ScreenToClient(pos);
            wxRect titleRect = area->titleBar()->GetRect();
            return titleRect.Contains(clientPos);
        }
    }
    return false;
}


bool FloatingDockContainer::testConfigFlag(DockManagerFeature flag) const {
    if (m_dockManager) {
        return m_dockManager->testConfigFlag(flag);
    }
    return false;
}

void FloatingDockContainer::onClose(wxCloseEvent& event) {
    // Notify closing
    wxCommandEvent closingEvent(EVT_FLOATING_CONTAINER_CLOSING);
    closingEvent.SetEventObject(this);
    ProcessWindowEvent(closingEvent);
    
    if (event.GetVeto()) {
        return;
    }
    
    // Close all dock widgets
    std::vector<DockWidget*> widgets = dockWidgets();
    for (auto* widget : widgets) {
        widget->closeDockWidget();
    }
    
    // Notify closed
    wxCommandEvent closedEvent(EVT_FLOATING_CONTAINER_CLOSED);
    closedEvent.SetEventObject(this);
    ProcessWindowEvent(closedEvent);
    
    event.Skip();
}

void FloatingDockContainer::onMouseLeftDown(wxMouseEvent& event) {
    if (!m_hasNativeTitleBar && isInTitleBar(event.GetPosition())) {
        d->isDragging = true;
        d->dragOffset = event.GetPosition();
        CaptureMouse();
    }
    event.Skip();
}

void FloatingDockContainer::onMouseLeftUp(wxMouseEvent& event) {
    bool wasDragging = d->isDragging;
    
    if (d->isDragging) {
        d->isDragging = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
        
        // Check if we should dock on mouse release
        if (m_dockManager) {
            wxPoint mousePos = wxGetMousePosition();
            DockWidgetArea dropArea = InvalidDockWidgetArea;
            wxWindow* dropTarget = nullptr;
            
            // Check dock area overlay first
            DockOverlay* areaOverlay = m_dockManager->dockAreaOverlay();
            if (areaOverlay && areaOverlay->IsShown()) {
                dropArea = areaOverlay->dropAreaUnderCursor();
                dropTarget = areaOverlay->targetWidget();
            }
            
            // If no area overlay, check container overlay
            if (dropArea == InvalidDockWidgetArea) {
                DockOverlay* containerOverlay = m_dockManager->containerOverlay();
                if (containerOverlay && containerOverlay->IsShown()) {
                    dropArea = containerOverlay->dropAreaUnderCursor();
                    dropTarget = containerOverlay->targetWidget();
                }
            }
            
            // Perform the drop
            if (dropArea != InvalidDockWidgetArea && dropTarget) {
                // Get all dock widgets from this floating container
                std::vector<DockWidget*> widgets = dockWidgets();
                
                if (!widgets.empty()) {
                    DockArea* targetArea = dynamic_cast<DockArea*>(dropTarget);
                    DockContainerWidget* targetContainer = nullptr;
                    if (!targetArea) {
                        // Try to cast to container if not a dock area
                        targetContainer = dynamic_cast<DockContainerWidget*>(dropTarget);
                    }
                    
                    if (targetArea && dropArea == CenterDockWidgetArea) {
                        // Add to existing dock area as tabs
                        for (auto* widget : widgets) {
                            targetArea->addDockWidget(widget);
                        }
                        
                        // Close this floating container
                        Close();
                    } else if (targetArea || targetContainer) {
                        // Create new dock area at the specified location
                        if (widgets.size() == 1) {
                            // Single widget - add directly
                            if (targetContainer) {
                                m_dockManager->addDockWidget(dropArea, widgets[0]);
                            } else if (targetArea) {
                                targetArea->dockContainer()->addDockWidget(dropArea, widgets[0], targetArea);
                            }
                        } else {
                            // Multiple widgets - create new area with all widgets
                            DockContainerWidget* container = targetContainer;
                            if (!container) {
                                if (targetArea) {
                                    container = targetArea->dockContainer();
                                } else {
                                    // Try to cast the manager's container widget
                                    wxWindow* managerContainer = m_dockManager->containerWidget();
                                    container = dynamic_cast<DockContainerWidget*>(managerContainer);
                                }
                            }
                            
                            if (container) {
                                DockArea* newArea = new DockArea(m_dockManager, container);
                                for (auto* widget : widgets) {
                                    newArea->addDockWidget(widget);
                                }
                                
                                if (targetContainer) {
                                    targetContainer->addDockArea(newArea, dropArea);
                                } else if (targetArea) {
                                    targetArea->dockContainer()->addDockAreaToContainer(dropArea, newArea);
                                }
                            }
                        }
                        
                        // Close this floating container
                        Close();
                    }
                }
            }
            
            // Hide overlays
            if (areaOverlay) {
                areaOverlay->hideOverlay();
            }
            if (m_dockManager->containerOverlay()) {
                m_dockManager->containerOverlay()->hideOverlay();
            }
        }
    }
    
    event.Skip();
}

void FloatingDockContainer::onMouseMove(wxMouseEvent& event) {
    if (d->isDragging && event.Dragging()) {
        wxPoint mousePos = wxGetMousePosition();
        SetPosition(mousePos - d->dragOffset);

        // Ensure window stays on top during dragging
        Raise();
        
        // Check for drop targets while dragging
        if (m_dockManager) {
            wxWindow* windowUnderMouse = wxFindWindowAtPoint(mousePos);
            
            // Skip if the window under mouse is this floating container or its children
            if (windowUnderMouse) {
                wxWindow* topLevel = wxGetTopLevelParent(windowUnderMouse);
                if (topLevel == this) {
                    // Hide temporarily to find window below
                    Hide();
                    windowUnderMouse = wxFindWindowAtPoint(mousePos);
                    Show();
                    
                    // Still check if we found something valid
                    if (windowUnderMouse) {
                        topLevel = wxGetTopLevelParent(windowUnderMouse);
                        if (topLevel == this) {
                            windowUnderMouse = nullptr;
                        }
                    }
                }
            }
            
            // Check if we're over a dock area
            DockArea* targetArea = nullptr;
            wxWindow* checkWindow = windowUnderMouse;
            while (checkWindow && !targetArea) {
                targetArea = dynamic_cast<DockArea*>(checkWindow);
                checkWindow = checkWindow->GetParent();
            }
            
            // Show overlay on dock area
            if (targetArea && targetArea->dockManager() == m_dockManager) {
                DockOverlay* overlay = m_dockManager->dockAreaOverlay();
                if (overlay) {
                    overlay->showOverlay(targetArea);
                }
            } else {
                // Check for container drop
                wxWindow* containerWindow = m_dockManager->containerWidget();
                DockContainerWidget* container = dynamic_cast<DockContainerWidget*>(containerWindow);
                if (container) {
                    wxRect containerRect = container->GetScreenRect();
                    if (containerRect.Contains(mousePos)) {
                        DockOverlay* overlay = m_dockManager->containerOverlay();
                        if (overlay) {
                            overlay->showOverlay(container);
                        }
                    } else {
                        // Hide overlays if not over any target
                        if (m_dockManager->containerOverlay()) {
                            m_dockManager->containerOverlay()->hideOverlay();
                        }
                        if (m_dockManager->dockAreaOverlay()) {
                            m_dockManager->dockAreaOverlay()->hideOverlay();
                        }
                    }
                }
            }
        }
    }
    event.Skip();
}

void FloatingDockContainer::onMouseDoubleClick(wxMouseEvent& event) {
    if (isInTitleBar(event.GetPosition())) {
        // TODO: Implement docking on double-click
    }
    event.Skip();
}

void FloatingDockContainer::onNonClientHitTest(wxMouseEvent& event) {
    // TODO: Implement for custom title bar
    event.Skip();
}

void FloatingDockContainer::onMaximize(wxMaximizeEvent& event) {
    event.Skip();
}

// FloatingDragPreview implementation
wxBEGIN_EVENT_TABLE(FloatingDragPreview, wxFrame)
    EVT_PAINT(FloatingDragPreview::onPaint)
    EVT_TIMER(wxID_ANY, FloatingDragPreview::onTimer)
wxEND_EVENT_TABLE()

FloatingDragPreview::FloatingDragPreview(DockWidget* content, wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
             wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE | wxSTAY_ON_TOP)
    , m_content(content)
    , m_animated(true)
    , m_fadeAlpha(0)
    , m_fadingIn(false)
    , m_defaultSize(content ? content->GetSize() : wxSize(200, 150))
    , m_currentSize(content ? content->GetSize() : wxSize(200, 150))
    , m_currentArea(InvalidDockWidgetArea)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_animationTimer = new wxTimer(this);
    updateContentBitmap();
}

FloatingDragPreview::FloatingDragPreview(DockArea* content, wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
             wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE | wxSTAY_ON_TOP)
    , m_content(content)
    , m_animated(true)
    , m_fadeAlpha(0)
    , m_fadingIn(false)
    , m_defaultSize(content ? content->GetSize() : wxSize(200, 150))
    , m_currentSize(content ? content->GetSize() : wxSize(200, 150))
    , m_currentArea(InvalidDockWidgetArea)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_animationTimer = new wxTimer(this);
    updateContentBitmap();
}

FloatingDragPreview::~FloatingDragPreview() {
    if (m_animationTimer) {
        if (m_animationTimer->IsRunning()) {
            m_animationTimer->Stop();
        }
        delete m_animationTimer;
    }
}

void FloatingDragPreview::setContent(DockWidget* content) {
    m_content = content;
    updateContentBitmap();
}

void FloatingDragPreview::setContent(DockArea* content) {
    m_content = content;
    updateContentBitmap();
}

void FloatingDragPreview::startDrag(const wxPoint& globalPos) {
    m_dragStartPos = globalPos;
    
    // Offset the preview so mouse is positioned at the tab area
    wxSize size = GetSize();
    wxPoint offset(-40, -15);  // Mouse at tab position (40, 15)
    SetPosition(globalPos + offset);
    
    if (m_animated) {
        // Start fade-in animation
        m_fadeAlpha = 0;
        m_fadingIn = true;
        SetTransparent(0);
        Show();
        m_animationTimer->Start(16); // ~60 FPS
    } else {
        SetTransparent(200); // 80% opacity
        Show();
    }
}

void FloatingDragPreview::moveFloating(const wxPoint& globalPos) {
    // Keep the same offset as in startDrag
    wxSize size = GetSize();
    wxPoint offset(-40, -15);
    SetPosition(globalPos + offset);
}

void FloatingDragPreview::finishDrag() {
    if (m_animationTimer->IsRunning()) {
        m_animationTimer->Stop();
    }
    Hide();
}

void FloatingDragPreview::setPreviewSize(DockWidgetArea area, const wxSize& targetSize) {
    wxLogDebug("FloatingDragPreview::setPreviewSize called: area=%d, targetSize=%dx%d", area, targetSize.GetWidth(), targetSize.GetHeight());
    
    if (area == m_currentArea && targetSize == m_currentSize) {
        wxLogDebug("No change needed, same area and size");
        return; // No change needed
    }
    
    m_currentArea = area;
    m_currentSize = calculatePreviewSize(area, targetSize);
    
    wxLogDebug("Calculated preview size: %dx%d", m_currentSize.GetWidth(), m_currentSize.GetHeight());
    
    // Update window size
    SetSize(m_currentSize);
    
    // Update position to maintain cursor offset
    wxPoint mousePos = wxGetMousePosition();
    wxPoint offset(-40, -15);
    SetPosition(mousePos + offset);
    
    // Trigger repaint
    Refresh();
    
    wxLogDebug("Preview window updated to size %dx%d", m_currentSize.GetWidth(), m_currentSize.GetHeight());
}

void FloatingDragPreview::resetToDefaultSize() {
    if (m_currentSize == m_defaultSize) {
        return; // No change needed
    }
    
    m_currentArea = InvalidDockWidgetArea;
    m_currentSize = m_defaultSize;
    
    // Update window size
    SetSize(m_currentSize);
    
    // Update position to maintain cursor offset
    wxPoint mousePos = wxGetMousePosition();
    wxPoint offset(-40, -15);
    SetPosition(mousePos + offset);
    
    // Trigger repaint
    Refresh();
}

wxSize FloatingDragPreview::calculatePreviewSize(DockWidgetArea area, const wxSize& targetSize) const {
    // Calculate preview size based on dock area type
    switch (area) {
    case TopDockWidgetArea:
    case BottomDockWidgetArea:
        // Horizontal areas - use target width, but limit height
        return wxSize(std::min(targetSize.GetWidth(), 400), 
                     std::min(targetSize.GetHeight() / 3, 150));
                     
    case LeftDockWidgetArea:
    case RightDockWidgetArea:
        // Vertical areas - use target height, but limit width
        return wxSize(std::min(targetSize.GetWidth() / 3, 150),
                     std::min(targetSize.GetHeight(), 300));
                     
    case CenterDockWidgetArea:
        // Center area - use target size with limits
        return wxSize(std::min(targetSize.GetWidth(), 400),
                     std::min(targetSize.GetHeight(), 300));
                     
    default:
        return m_defaultSize;
    }
}

void FloatingDragPreview::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    
    // Get the title of the widget being dragged
    wxString title = "Dock Widget";
    DockWidget* dockWidget = dynamic_cast<DockWidget*>(m_content);
    if (dockWidget) {
        title = dockWidget->title();
    } else {
        DockArea* dockArea = dynamic_cast<DockArea*>(m_content);
        if (dockArea && dockArea->currentDockWidget()) {
            title = dockArea->currentDockWidget()->title();
        }
    }
    
    // Draw a schematic representation instead of actual content
    wxSize size = GetClientSize();
    
    // Background
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME), 2));
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
    
    // Tab bar area
    int tabHeight = 30;
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.DrawRectangle(0, 0, size.GetWidth(), tabHeight);
    
    // Tab
    int tabWidth = std::min(150, size.GetWidth() - 20);
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
    dc.DrawRectangle(10, 5, tabWidth, tabHeight - 5);
    
    // Tab text
    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    wxSize textSize = dc.GetTextExtent(title);
    int textX = 10 + (tabWidth - textSize.GetWidth()) / 2;
    int textY = 5 + (tabHeight - 5 - textSize.GetHeight()) / 2;
    dc.DrawText(title, textX, textY);
    
    // Content area hint
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT), 1, wxPENSTYLE_DOT));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    int margin = 10;
    dc.DrawRectangle(margin, tabHeight + margin, 
                    size.GetWidth() - 2 * margin, 
                    size.GetHeight() - tabHeight - 2 * margin);
}

void FloatingDragPreview::updateContentBitmap() {
    if (!m_content) {
        return;
    }
    
    // Use the actual size of the content for the preview
    wxSize size = m_content->GetSize();
    if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
        // Fallback to default size if content size is invalid
        size = wxSize(200, 150);
    }
    
    SetSize(size);
    
    // We don't need to create a bitmap anymore since we draw directly in onPaint
    // Just trigger a repaint
    Refresh();
    
    // Apply transparency
    SetTransparent(200); // 80% opacity
}

void FloatingDragPreview::onTimer(wxTimerEvent& event) {
    if (m_fadingIn) {
        m_fadeAlpha += 20; // Fade in over ~10 frames
        if (m_fadeAlpha >= 200) {
            m_fadeAlpha = 200;
            m_fadingIn = false;
            m_animationTimer->Stop();
        }
        SetTransparent(m_fadeAlpha);
    }
}

void FloatingDockContainer::setupCustomTitleBar() {
    // TODO: Implement custom title bar for FloatingDockContainer
    // This method is called when switching to custom title bar mode
    // For now, this is a placeholder implementation
}

void FloatingDockContainer::startDragging(const wxPoint& dragOffset) {
    // Start dragging programmatically
    d->isDragging = true;
    d->dragOffset = dragOffset;

    // Capture mouse to receive all mouse events
    if (!HasCapture()) {
        CaptureMouse();
    }

    // Move window to follow mouse
    wxPoint mousePos = wxGetMousePosition();
    SetPosition(mousePos - dragOffset);
}

void FloatingDockContainer::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    
    // Get border color from theme management system
    auto& themeManager = ThemeManager::getInstance();
    wxColour borderColor = themeManager.getColour("BorderColour");
    
    // If theme color is not available, use fallback
    if (!borderColor.IsOk()) {
        borderColor = wxColour(170, 170, 170); // Default light gray border
    }
    
    // Draw 1-pixel border around the entire window
    wxSize size = GetClientSize();
    dc.SetPen(wxPen(borderColor, 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, size.GetWidth() - 1, size.GetHeight() - 1);
}

} // namespace ads
