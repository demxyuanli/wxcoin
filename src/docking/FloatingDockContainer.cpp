#include "docking/FloatingDockContainer.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"

namespace ads {

// Define custom events
wxDEFINE_EVENT(FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSING, wxCommandEvent);
wxDEFINE_EVENT(FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSED, wxCommandEvent);

// Event table
wxBEGIN_EVENT_TABLE(FloatingDockContainer, wxFrame)
    EVT_CLOSE(FloatingDockContainer::onClose)
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
    : wxFrame(nullptr, wxID_ANY, "Floating Dock", 
             wxDefaultPosition, wxSize(400, 300),
             wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockManager)
    , m_hasNativeTitleBar(true)
    , m_dragState(DraggingInactive)
    , m_dragPreview(nullptr)
{
    init();
}

FloatingDockContainer::FloatingDockContainer(DockArea* dockArea)
    : wxFrame(nullptr, wxID_ANY, "Floating Dock", 
             wxDefaultPosition, wxSize(400, 300),
             wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockArea ? dockArea->dockManager() : nullptr)
    , m_hasNativeTitleBar(true)
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
    }
}

FloatingDockContainer::FloatingDockContainer(DockWidget* dockWidget)
    : wxFrame(nullptr, wxID_ANY, dockWidget ? dockWidget->title() : "Floating Dock", 
             wxDefaultPosition, wxSize(400, 300),
             wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockWidget ? dockWidget->dockManager() : nullptr)
    , m_hasNativeTitleBar(true)
    , m_dragState(DraggingInactive)
    , m_dragPreview(nullptr)
{
    init();
    
    // Add the dock widget
    if (dockWidget) {
        addDockWidget(dockWidget, CenterDockWidgetArea);
    }
}

FloatingDockContainer::~FloatingDockContainer() {
    // Notify manager
    if (m_dockManager) {
        m_dockManager->onFloatingWidgetAboutToClose(this);
    }
}

void FloatingDockContainer::init() {
    // Create dock container
    m_dockContainer = new DockContainerWidget(m_dockManager, this);
    m_dockContainer->setFloatingWidget(this);
    
    // Setup custom title bar if needed
    if (!m_hasNativeTitleBar) {
        setupCustomTitleBar();
    }
    
    // Register with manager
    if (m_dockManager) {
        m_dockManager->onFloatingWidgetCreated(this);
    }
}

void FloatingDockContainer::addDockWidget(DockWidget* dockWidget, DockWidgetArea area) {
    if (!dockWidget || !m_dockContainer) {
        return;
    }
    
    m_dockContainer->addDockWidget(area, dockWidget);
    updateWindowTitle();
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
    if (m_hasNativeTitleBar == native) {
        return;
    }
    
    m_hasNativeTitleBar = native;
    
    if (native) {
        // Use native title bar
        SetWindowStyleFlag(wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT);
    } else {
        // Use custom title bar
        SetWindowStyleFlag(wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE);
        setupCustomTitleBar();
    }
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
    if (m_hasNativeTitleBar) {
        // For native title bar, check if in top area
        wxRect titleRect(0, 0, GetSize().GetWidth(), 30);
        return titleRect.Contains(ScreenToClient(pos));
    } else {
        // TODO: Check custom title bar
        return false;
    }
}

void FloatingDockContainer::setupCustomTitleBar() {
    // TODO: Implement custom title bar
}

void FloatingDockContainer::testConfigFlag(DockManagerFeature flag) const {
    if (m_dockManager) {
        m_dockManager->testConfigFlag(flag);
    }
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
    if (d->isDragging) {
        d->isDragging = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
    event.Skip();
}

void FloatingDockContainer::onMouseMove(wxMouseEvent& event) {
    if (d->isDragging && event.Dragging()) {
        wxPoint mousePos = wxGetMousePosition();
        SetPosition(mousePos - d->dragOffset);
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
wxEND_EVENT_TABLE()

FloatingDragPreview::FloatingDragPreview(DockWidget* content, wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
             wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE | wxSTAY_ON_TOP)
    , m_content(content)
    , m_animated(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    updateContentBitmap();
}

FloatingDragPreview::FloatingDragPreview(DockArea* content, wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
             wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE | wxSTAY_ON_TOP)
    , m_content(content)
    , m_animated(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    updateContentBitmap();
}

FloatingDragPreview::~FloatingDragPreview() {
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
    SetPosition(globalPos);
    Show();
}

void FloatingDragPreview::moveFloating(const wxPoint& globalPos) {
    SetPosition(globalPos);
}

void FloatingDragPreview::finishDrag() {
    Hide();
}

void FloatingDragPreview::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    if (m_contentBitmap.IsOk()) {
        dc.DrawBitmap(m_contentBitmap, 0, 0);
    }
}

void FloatingDragPreview::updateContentBitmap() {
    if (!m_content) {
        return;
    }
    
    wxSize size = m_content->GetSize();
    SetSize(size);
    
    // Create bitmap from content
    m_contentBitmap = wxBitmap(size);
    wxMemoryDC memDC(m_contentBitmap);
    
    // Draw content
    memDC.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    memDC.Clear();
    
    // TODO: Render actual content
    memDC.SetPen(*wxBLACK_PEN);
    memDC.SetBrush(*wxTRANSPARENT_BRUSH);
    memDC.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
    
    memDC.SelectObject(wxNullBitmap);
}

} // namespace ads