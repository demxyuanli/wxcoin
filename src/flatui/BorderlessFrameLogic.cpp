#include "flatui/BorderlessFrameLogic.h"
#include <wx/dcbuffer.h> // For wxScreenDC if used, and double buffering
#include "logger/Logger.h"

#ifdef __WXMSW__
#define NOMINMAX
#include <windows.h> // For Windows specific GDI calls for rubber band
#endif
#include "config/ThemeManager.h"


wxBEGIN_EVENT_TABLE(BorderlessFrameLogic, wxFrame)
EVT_LEFT_DOWN(BorderlessFrameLogic::OnLeftDown)
EVT_LEFT_UP(BorderlessFrameLogic::OnLeftUp)
EVT_MOTION(BorderlessFrameLogic::OnMotion)
EVT_PAINT(BorderlessFrameLogic::OnPaint)
#ifdef __WXMSW__
EVT_DPI_CHANGED(BorderlessFrameLogic::OnDPIChanged)
#endif
wxEND_EVENT_TABLE()

BorderlessFrameLogic::BorderlessFrameLogic(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxFrame(parent, id, title, pos, size, style),
    m_dragging(false),
    m_resizing(false),
    m_resizeMode(ResizeMode::NONE),
    m_rubberBandVisible(false)
{
    // Calculate DPI-aware border threshold
    UpdateBorderThreshold();

    // Basic setup, often common for borderless windows
    SetDoubleBuffered(true);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));

    m_eventFilter = new BorderlessFrameLogicEventFilter(this);
    this->PushEventHandler(m_eventFilter);

    m_mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_mainSizer);
    m_statusBar = new FlatUIStatusBar(this);

    Refresh(); // Ensure background is painted initially
}

BorderlessFrameLogic::~BorderlessFrameLogic()
{
    wxLogDebug("BorderlessFrameLogic destruction started.");

    // Remove the pushed event handler before destruction
    if (m_eventFilter && GetEventHandler() == m_eventFilter) {
        PopEventHandler(true); // true means delete the handler
        m_eventFilter = nullptr;
    }

    wxLogDebug("BorderlessFrameLogic destruction completed.");
}

void BorderlessFrameLogic::ResetCursorToDefault() {
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

void BorderlessFrameLogic::UpdateBorderThreshold()
{
    // Get current DPI scaling factor
    double scaleFactor = GetCurrentDPIScale();

    // Adjust border threshold based on DPI scaling
    m_borderThreshold = static_cast<int>(8 * scaleFactor);

    // Ensure minimum threshold
    if (m_borderThreshold < 4) m_borderThreshold = 4;
}

double BorderlessFrameLogic::GetCurrentDPIScale()
{
    double scaleFactor = 1.0;

#ifdef __WXMSW__
    // On Windows, get DPI scaling factor
    HDC hdc = ::GetDC(NULL);
    if (hdc) {
        int dpiX = ::GetDeviceCaps(hdc, LOGPIXELSX);
        scaleFactor = dpiX / 96.0; // 96 DPI is 100% scaling
        ::ReleaseDC(NULL, hdc);
    }
#else
    // On other platforms, use wxWidgets content scale factor
    scaleFactor = GetContentScaleFactor();
#endif

    return scaleFactor;
}

#ifdef __WXMSW__
void BorderlessFrameLogic::OnDPIChanged(wxDPIChangedEvent& event)
{
    // Update border threshold when DPI changes
    UpdateBorderThreshold();

    // Force layout update
    Layout();
    Refresh();

    event.Skip();
}

wxRect BorderlessFrameLogic::ConvertLogicalToPhysicalRect(const wxRect& logicalRect)
{
    double scaleFactor = GetCurrentDPIScale();
    
    // Only apply scaling if we have actual DPI scaling (not 100%)
    if (scaleFactor == 1.0) {
        return logicalRect;  // No scaling needed
    }
    
    return wxRect(
        static_cast<int>(logicalRect.x * scaleFactor),
        static_cast<int>(logicalRect.y * scaleFactor),
        static_cast<int>(logicalRect.width * scaleFactor),
        static_cast<int>(logicalRect.height * scaleFactor)
    );
}
#endif

void BorderlessFrameLogic::OnLeftDown(wxMouseEvent& event)
{
    // Basic drag/resize initiation logic (without pseudo-maximization check, which is for derived class)
    // Derived classes (like FlatUIFrame) will call this and add their specific checks first.

    // Check if the event originated from a child control that should handle it.
    // This is a placeholder; derived classes should implement more specific logic
    // if they have interactive child controls in draggable/resizable areas.
    wxWindow* eventSource = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (eventSource != this && eventSource->GetParent() == this) { // Example: direct child
        // If child wants to handle it, it should not Skip() or this logic takes over.
        // This area needs careful handling in a complex UI.
    }

    ResizeMode hoverMode = GetResizeModeForPosition(event.GetPosition());
    if (hoverMode != ResizeMode::NONE)
    {
        m_resizing = true;
        m_resizeMode = hoverMode;
        m_resizeStartMouseScreenPos = wxGetMousePosition();
        m_resizeStartWindowRect = GetScreenRect();
        if (!HasCapture()) {
            CaptureMouse();
        }
    }
    else // Click was on the main frame body (not border), initiate drag
    {
        m_dragging = true;
        // m_dragStartPos is mouse position relative to the window's client area top-left
        m_dragStartPos = event.GetPosition();
        m_resizeStartWindowRect = GetScreenRect(); // Save initial window rect for rubber band reference
        if (!HasCapture()) {
            CaptureMouse();
        }
    }
    event.Skip(); // Allow further processing if needed by wxWidgets or other handlers
}

void BorderlessFrameLogic::OnLeftUp(wxMouseEvent& event)
{
    if (HasCapture()) {
        ReleaseMouse();
    }

    if (m_dragging) {
        if (m_rubberBandVisible) EraseRubberBand();
        wxPoint mousePosOnScreen = wxGetMousePosition();
        // New window position is current mouse position on screen minus the initial offset (m_dragStartPos)
        int newX = mousePosOnScreen.x - m_dragStartPos.x;
        int newY = mousePosOnScreen.y - m_dragStartPos.y;
        Move(newX, newY);
        m_dragging = false;
    }
    else if (m_resizing) {
        if (m_rubberBandVisible) EraseRubberBand();
        wxPoint currentMouseScreenPos = wxGetMousePosition();
        int dx = currentMouseScreenPos.x - m_resizeStartMouseScreenPos.x;
        int dy = currentMouseScreenPos.y - m_resizeStartMouseScreenPos.y;
        wxRect newRect = m_resizeStartWindowRect;
        int minWidth = GetMinWidth() > 0 ? GetMinWidth() : 100; // Basic min size
        int minHeight = GetMinHeight() > 0 ? GetMinHeight() : 100;

        switch (m_resizeMode)
        {
        case ResizeMode::LEFT:
            newRect.x += dx;
            newRect.width -= dx;
            if (newRect.width < minWidth) { newRect.width = minWidth; newRect.x = m_resizeStartWindowRect.GetRight() - minWidth; }
            break;
        case ResizeMode::RIGHT:
            newRect.width += dx;
            if (newRect.width < minWidth) newRect.width = minWidth;
            break;
        case ResizeMode::TOP:
            newRect.y += dy;
            newRect.height -= dy;
            if (newRect.height < minHeight) { newRect.height = minHeight; newRect.y = m_resizeStartWindowRect.GetBottom() - minHeight; }
            break;
        case ResizeMode::BOTTOM:
            newRect.height += dy;
            if (newRect.height < minHeight) newRect.height = minHeight;
            break;
        case ResizeMode::TOP_LEFT:
            newRect.x += dx; newRect.width -= dx;
            newRect.y += dy; newRect.height -= dy;
            if (newRect.width < minWidth) { newRect.width = minWidth; newRect.x = m_resizeStartWindowRect.GetRight() - minWidth; }
            if (newRect.height < minHeight) { newRect.height = minHeight; newRect.y = m_resizeStartWindowRect.GetBottom() - minHeight; }
            break;
        case ResizeMode::TOP_RIGHT:
            newRect.width += dx;
            newRect.y += dy; newRect.height -= dy;
            if (newRect.width < minWidth) newRect.width = minWidth;
            if (newRect.height < minHeight) { newRect.height = minHeight; newRect.y = m_resizeStartWindowRect.GetBottom() - minHeight; }
            break;
        case ResizeMode::BOTTOM_LEFT:
            newRect.x += dx; newRect.width -= dx;
            newRect.height += dy;
            if (newRect.width < minWidth) { newRect.width = minWidth; newRect.x = m_resizeStartWindowRect.GetRight() - minWidth; }
            if (newRect.height < minHeight) newRect.height = minHeight;
            break;
        case ResizeMode::BOTTOM_RIGHT:
            newRect.width += dx; newRect.height += dy;
            if (newRect.width < minWidth) newRect.width = minWidth;
            if (newRect.height < minHeight) newRect.height = minHeight;
            break;
        case ResizeMode::NONE: break; // Should not happen if m_resizing is true
        }
        SetSize(newRect); // Apply the new size and position
        Layout(); // Recalculate layout if sizers are used
        Refresh(); // Redraw the window and its children
        Update();  // Ensure UI updates are processed

        m_resizing = false;
        m_resizeMode = ResizeMode::NONE;
        UpdateCursorForResizeMode(ResizeMode::NONE); // Reset cursor to arrow
    }
    event.Skip();
}

void BorderlessFrameLogic::OnMotion(wxMouseEvent& event)
{
    static wxRect lastDrawnRect;
    static wxLongLong lastDrawTime = 0;
    wxLongLong currentTime = wxGetLocalTimeMillis();

    // Basic rubber band and cursor update logic (without pseudo-maximization check)
    // Derived classes (like FlatUIFrame) will call this and add their specific checks first.

    if (m_dragging && event.Dragging() && event.LeftIsDown()) {
        wxPoint mousePosOnScreen = wxGetMousePosition();
        wxRect newRect(
            mousePosOnScreen.x - m_dragStartPos.x, // Top-left X of dragging rect
            mousePosOnScreen.y - m_dragStartPos.y, // Top-left Y of dragging rect
            m_resizeStartWindowRect.GetWidth(),    // Original width
            m_resizeStartWindowRect.GetHeight()    // Original height
        );
        
        // Debug logging for coordinate tracking
        static int logCount = 0;
        if (logCount++ % 10 == 0) { // Log every 10th call to avoid spam
            LOG_DBG("OnMotion drag: mouseScreen=(" + std::to_string(mousePosOnScreen.x) + "," + std::to_string(mousePosOnScreen.y) + ")" +
                    ", dragStart=(" + std::to_string(m_dragStartPos.x) + "," + std::to_string(m_dragStartPos.y) + ")" +
                    ", newRect=(" + std::to_string(newRect.x) + "," + std::to_string(newRect.y) + "," + 
                    std::to_string(newRect.width) + "," + std::to_string(newRect.height) + ")", "BorderlessFrameLogic");
        }
        
        if (!m_rubberBandVisible || lastDrawnRect != newRect && (currentTime - lastDrawTime > 16 ||
            abs(newRect.x - lastDrawnRect.x) > 5 || abs(newRect.y - lastDrawnRect.y) > 5)) {
            if (m_rubberBandVisible) EraseRubberBand();
            DrawRubberBand(newRect);
            lastDrawnRect = newRect;
            lastDrawTime = currentTime;
        }
    }
    else if (m_resizing && event.Dragging() && event.LeftIsDown()) {
        wxPoint currentMouseScreenPos = wxGetMousePosition();
        int dx = currentMouseScreenPos.x - m_resizeStartMouseScreenPos.x;
        int dy = currentMouseScreenPos.y - m_resizeStartMouseScreenPos.y;
        wxRect newRect = m_resizeStartWindowRect;
        int minWidth = GetMinWidth() > 0 ? GetMinWidth() : 100;
        int minHeight = GetMinHeight() > 0 ? GetMinHeight() : 100;

        // Calculate newRect based on resizeMode, dx, dy, and minWidth/minHeight
        // (This logic is identical to OnLeftUp's resizing part for drawing rubber band)
        switch (m_resizeMode)
        {
        case ResizeMode::LEFT:
            newRect.x += dx;
            newRect.width -= dx;
            if (newRect.width < minWidth) { newRect.width = minWidth; newRect.x = m_resizeStartWindowRect.GetRight() - minWidth; }
            break;
        case ResizeMode::RIGHT:
            newRect.width += dx;
            if (newRect.width < minWidth) newRect.width = minWidth;
            break;
        case ResizeMode::TOP:
            newRect.y += dy; newRect.height -= dy;
            if (newRect.height < minHeight) { newRect.height = minHeight; newRect.y = m_resizeStartWindowRect.GetBottom() - minHeight; }
            break;
        case ResizeMode::BOTTOM:
            newRect.height += dy;
            if (newRect.height < minHeight) newRect.height = minHeight;
            break;
        case ResizeMode::TOP_LEFT:
            newRect.x += dx; newRect.width -= dx;
            newRect.y += dy; newRect.height -= dy;
            if (newRect.width < minWidth) { newRect.width = minWidth; newRect.x = m_resizeStartWindowRect.GetRight() - minWidth; }
            if (newRect.height < minHeight) { newRect.height = minHeight; newRect.y = m_resizeStartWindowRect.GetBottom() - minHeight; }
            break;
        case ResizeMode::TOP_RIGHT:
            newRect.width += dx;
            newRect.y += dy; newRect.height -= dy;
            if (newRect.width < minWidth) newRect.width = minWidth;
            if (newRect.height < minHeight) { newRect.height = minHeight; newRect.y = m_resizeStartWindowRect.GetBottom() - minHeight; }
            break;
        case ResizeMode::BOTTOM_LEFT:
            newRect.x += dx; newRect.width -= dx;
            newRect.height += dy;
            if (newRect.width < minWidth) { newRect.width = minWidth; newRect.x = m_resizeStartWindowRect.GetRight() - minWidth; }
            if (newRect.height < minHeight) newRect.height = minHeight;
            break;
        case ResizeMode::BOTTOM_RIGHT:
            newRect.width += dx; newRect.height += dy;
            if (newRect.width < minWidth) newRect.width = minWidth;
            if (newRect.height < minHeight) newRect.height = minHeight;
            break;
        case ResizeMode::NONE: break;
        }
        if (!m_rubberBandVisible || lastDrawnRect != newRect && (currentTime - lastDrawTime > 16 ||
            abs(newRect.x - lastDrawnRect.x) > 5 || abs(newRect.y - lastDrawnRect.y) > 5 ||
            abs(newRect.width - lastDrawnRect.width) > 5 || abs(newRect.height - lastDrawnRect.height) > 5)) {
            if (m_rubberBandVisible) EraseRubberBand();
            DrawRubberBand(newRect);
            lastDrawnRect = newRect;
            lastDrawTime = currentTime;
        }
    }
    else { // Only update cursor if mouse button is not down or if not dragging/resizing
        ResizeMode hoverMode = GetResizeModeForPosition(event.GetPosition());
        UpdateCursorForResizeMode(hoverMode);
    }
    event.Skip();
}

ResizeMode BorderlessFrameLogic::GetResizeModeForPosition(const wxPoint& clientPos)
{
    wxSize clientSize = GetClientSize();
    int x = clientPos.x;
    int y = clientPos.y;
    bool onLeft = (x >= 0 && x < m_borderThreshold);
    bool onRight = (x >= clientSize.GetWidth() - m_borderThreshold && x < clientSize.GetWidth());
    bool onTop = (y >= 0 && y < m_borderThreshold);
    bool onBottom = (y >= clientSize.GetHeight() - m_borderThreshold && y < clientSize.GetHeight());

    if (onTop && onLeft) return ResizeMode::TOP_LEFT;
    if (onBottom && onLeft) return ResizeMode::BOTTOM_LEFT;
    if (onTop && onRight) return ResizeMode::TOP_RIGHT;
    if (onBottom && onRight) return ResizeMode::BOTTOM_RIGHT;
    if (onLeft) return ResizeMode::LEFT;
    if (onRight) return ResizeMode::RIGHT;
    if (onTop) return ResizeMode::TOP;
    if (onBottom) return ResizeMode::BOTTOM;
    return ResizeMode::NONE;
}

void BorderlessFrameLogic::UpdateCursorForResizeMode(ResizeMode mode)
{
    wxStockCursor cursorId = wxCURSOR_ARROW;
    switch (mode)
    {
    case ResizeMode::LEFT: case ResizeMode::RIGHT: cursorId = wxCURSOR_SIZEWE; break;
    case ResizeMode::TOP: case ResizeMode::BOTTOM: cursorId = wxCURSOR_SIZENS; break;
    case ResizeMode::TOP_LEFT: case ResizeMode::BOTTOM_RIGHT: cursorId = wxCURSOR_SIZENWSE; break;
    case ResizeMode::TOP_RIGHT: case ResizeMode::BOTTOM_LEFT: cursorId = wxCURSOR_SIZENESW; break;
    case ResizeMode::NONE: default: cursorId = wxCURSOR_ARROW; break;
    }
    SetCursor(wxCursor(cursorId));
}

void BorderlessFrameLogic::DrawRubberBand(const wxRect& rect)
{
    if (m_rubberBandVisible) EraseRubberBand();

    wxRect drawRect = rect;

#ifdef __WXMSW__
    // Windows-specific DPI scaling adjustment for rubber band
    // On Windows, when system DPI scaling is enabled, the GDI coordinate system
    // requires conversion from logical to physical coordinates
    drawRect = ConvertLogicalToPhysicalRect(rect);
    double scaleFactor = GetCurrentDPIScale();

    // Debug logging for Windows DPI scaling
    LOG_DBG("DrawRubberBand [Windows]: scaleFactor=" + std::to_string(scaleFactor) + 
            ", logical=(" + std::to_string(rect.x) + "," + std::to_string(rect.y) + "," + 
            std::to_string(rect.width) + "," + std::to_string(rect.height) + ")" +
            ", physical=(" + std::to_string(drawRect.x) + "," + std::to_string(drawRect.y) + "," + 
            std::to_string(drawRect.width) + "," + std::to_string(drawRect.height) + ")", "BorderlessFrameLogic");

    // Calculate pen width with DPI scaling
    int penWidth = static_cast<int>(3 * scaleFactor);
    if (penWidth < 1) penWidth = 1;

    HDC hdc = ::GetDC(NULL);
    int oldROP = ::SetROP2(hdc, R2_NOTXORPEN);
    wxColour penColour = CFG_COLOUR("BorderlessFramePenColour");
    HPEN hPen = ::CreatePen(PS_GEOMETRIC | PS_SOLID, penWidth, RGB(penColour.Red(), penColour.Green(), penColour.Blue()));
    HPEN hOldPen = (HPEN)::SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)::SelectObject(hdc, GetStockObject(NULL_BRUSH));

    ::Rectangle(hdc, drawRect.GetLeft(), drawRect.GetTop(),
        drawRect.GetRight(), drawRect.GetBottom());

    ::SelectObject(hdc, hOldBrush);
    ::SelectObject(hdc, hOldPen);
    ::DeleteObject(hPen);
    ::SetROP2(hdc, oldROP);
    ::ReleaseDC(NULL, hdc);
#else
    // Non-Windows platforms: use wxWidgets drawing directly
    // These platforms handle DPI scaling automatically
    wxScreenDC dc;
    dc.SetLogicalFunction(wxINVERT);

    double scaleFactor = GetCurrentDPIScale();
    int penWidth = static_cast<int>(3 * scaleFactor);
    if (penWidth < 1) penWidth = 1;

    wxPen pen(CFG_COLOUR("BorderlessFramePenColour"), penWidth, wxPENSTYLE_SOLID);
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(drawRect);
#endif

    m_currentRubberBandRect = drawRect;
    m_rubberBandVisible = true;
}

void BorderlessFrameLogic::EraseRubberBand()
{
    if (!m_rubberBandVisible) return;

#ifdef __WXMSW__
    // Windows-specific rubber band erasure with DPI scaling
    double scaleFactor = GetCurrentDPIScale();
    int penWidth = static_cast<int>(3 * scaleFactor);
    if (penWidth < 1) penWidth = 1;

    HDC hdc = ::GetDC(NULL);
    int oldROP = ::SetROP2(hdc, R2_NOTXORPEN);
    wxColour penColour = CFG_COLOUR("BorderlessFramePenColour");
    HPEN hPen = ::CreatePen(PS_GEOMETRIC | PS_SOLID, penWidth, RGB(penColour.Red(), penColour.Green(), penColour.Blue()));
    HPEN hOldPen = (HPEN)::SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)::SelectObject(hdc, GetStockObject(NULL_BRUSH));

    // Use the stored m_currentRubberBandRect which already contains the correct coordinates
    ::Rectangle(hdc, m_currentRubberBandRect.GetLeft(), m_currentRubberBandRect.GetTop(),
        m_currentRubberBandRect.GetRight(), m_currentRubberBandRect.GetBottom());

    ::SelectObject(hdc, hOldBrush);
    ::SelectObject(hdc, hOldPen);
    ::DeleteObject(hPen);
    ::SetROP2(hdc, oldROP);
    ::ReleaseDC(NULL, hdc);
#else
    // Non-Windows platforms: direct wxWidgets drawing
    wxScreenDC dc;
    dc.SetLogicalFunction(wxINVERT);

    double scaleFactor = GetCurrentDPIScale();
    int penWidth = static_cast<int>(3 * scaleFactor);
    if (penWidth < 1) penWidth = 1;

    wxPen pen(CFG_COLOUR("BorderlessFramePenColour"), penWidth, wxPENSTYLE_SOLID);
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(m_currentRubberBandRect);
#endif

    m_rubberBandVisible = false;
}

void BorderlessFrameLogic::OnPaint(wxPaintEvent& event)
{

    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    wxSize sz = GetClientSize();


    dc.SetPen(wxPen(CFG_COLOUR("FrameBorderColor"), 1));
    dc.DrawLine(0, 0, sz.x, 0);
    dc.DrawLine(0, sz.y - 1, sz.x, sz.y - 1);
    dc.DrawLine(0, 0, 0, sz.y);
    dc.DrawLine(sz.x - 1, 0, sz.x - 1, sz.y);

    event.Skip();
}

void BorderlessFrameLogic::UpdateMinSizeBasedOnBarContent()
{
    // This method can be implemented by derived classes
    // Base implementation does nothing
}

// Add these missing virtual function implementations
int BorderlessFrameLogic::GetMinWidth() const
{
    // Default minimum width - can be overridden by derived classes
    return 300;
}

int BorderlessFrameLogic::GetMinHeight() const
{
    // Default minimum height - can be overridden by derived classes
    return 200;
}

void BorderlessFrameLogic::SetSize(const wxRect& rect)
{
    // Call the base wxFrame::SetSize method
    wxFrame::SetSize(rect);
}

void BorderlessFrameLogic::SetSize(const wxSize& size)
{
    // Call the base wxFrame::SetSize method
    wxFrame::SetSize(size);
}

void BorderlessFrameLogic::addStatusBar()
{
    // Call the base wxFrame::SetSize method
    m_mainSizer->Add(m_statusBar, 0, wxEXPAND | wxALL, 1);
}


void BorderlessFrameLogic::SetStatusText(const wxString& text, int field) {
    if (m_statusBar) m_statusBar->SetStatusText(text, field);
}
