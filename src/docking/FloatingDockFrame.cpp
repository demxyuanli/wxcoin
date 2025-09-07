#include "docking/FloatingDockFrame.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>

namespace ads {

// Event table
wxBEGIN_EVENT_TABLE(FloatingDockFrame, BorderlessFrameLogic)
    EVT_PAINT(FloatingDockFrame::OnPaint)
    EVT_LEFT_DOWN(FloatingDockFrame::OnLeftDown)
    EVT_LEFT_UP(FloatingDockFrame::OnLeftUp)
    EVT_MOTION(FloatingDockFrame::OnMotion)
wxEND_EVENT_TABLE()

FloatingDockFrame::FloatingDockFrame(wxWindow* parent, wxWindowID id, const wxString& title,
                                   const wxPoint& pos, const wxSize& size)
    : BorderlessFrameLogic(parent, id, title, pos, size, wxBORDER_NONE | wxFRAME_NO_TASKBAR)
    , m_titleText(title)
    , m_showSystemButtons(false)
    , m_contentArea(nullptr)
    , m_titleBarPanel(nullptr)
    , m_titleLabel(nullptr)
    , m_systemButtons(nullptr)
{
    // Create title bar panel
    m_titleBarPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                                 wxSize(size.GetWidth(), TITLE_BAR_HEIGHT));
    m_titleBarPanel->SetBackgroundColour(CFG_COLOUR("PrimaryBgColour"));

    // Create title label
    m_titleLabel = new wxStaticText(m_titleBarPanel, wxID_ANY, title,
                                   wxPoint(10, 5), wxDefaultSize);
    m_titleLabel->SetForegroundColour(CFG_COLOUR("PrimaryFgColour"));
    wxFont titleFont = m_titleLabel->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_titleLabel->SetFont(titleFont);

    // Create system buttons
    m_systemButtons = new FlatUISystemButtons(m_titleBarPanel, wxID_ANY);
    m_systemButtons->SetButtonWidth(30);
    m_systemButtons->SetButtonSpacing(2);

    // Override the system buttons' event handling to work with our custom frame
    m_systemButtons->Bind(wxEVT_LEFT_DOWN, &FloatingDockFrame::OnSystemButtonMouseDown, this);

    // Create content area
    m_contentArea = new wxPanel(this, wxID_ANY);
    m_contentArea->SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));

    // Setup layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_titleBarPanel, 0, wxEXPAND);
    mainSizer->Add(m_contentArea, 1, wxEXPAND);
    SetSizer(mainSizer);

    // System button events will be handled through custom mouse event handling
    // No need to bind command events since we handle them manually

    // Update layout
    UpdateTitleBarLayout();
    Layout();
}

FloatingDockFrame::~FloatingDockFrame()
{
    // Components will be cleaned up by wxWidgets parent-child destruction
}

void FloatingDockFrame::SetTitle(const wxString& title)
{
    m_titleText = title;
    if (m_titleLabel) {
        m_titleLabel->SetLabel(title);
        UpdateTitleBarLayout();
    }
}

void FloatingDockFrame::ShowSystemButtons(bool show)
{
    m_showSystemButtons = show;
    if (m_systemButtons) {
        m_systemButtons->Show(show);
        UpdateTitleBarLayout();
    }
}

void FloatingDockFrame::SetContentArea(wxWindow* content)
{
    if (m_contentArea && content) {
        // Reparent the content to our content area
        content->Reparent(m_contentArea);

        // Setup content area layout
        wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
        contentSizer->Add(content, 1, wxEXPAND);
        m_contentArea->SetSizer(contentSizer);
        m_contentArea->Layout();
    }
}

void FloatingDockFrame::UpdateTitleBarLayout()
{
    if (!m_titleBarPanel || !m_titleLabel || !m_systemButtons) {
        return;
    } 
     
    wxSize panelSize = m_titleBarPanel->GetSize();

    // Position title label
    wxSize titleSize = m_titleLabel->GetSize();
    m_titleLabel->SetPosition(wxPoint(10, (TITLE_BAR_HEIGHT - titleSize.GetHeight()) / 2));

    // Position system buttons
    if (m_showSystemButtons) {
        int buttonWidth = m_systemButtons->GetRequiredWidth();
        m_systemButtons->SetPosition(wxPoint(panelSize.GetWidth() - buttonWidth - 10,
                                           (TITLE_BAR_HEIGHT - 20) / 2));

        // Set button rectangles for the system buttons
        wxRect minimizeRect(panelSize.GetWidth() - buttonWidth - 10, (TITLE_BAR_HEIGHT - 20) / 2, 30, 20);
        wxRect maximizeRect(panelSize.GetWidth() - buttonWidth - 10 + 32, (TITLE_BAR_HEIGHT - 20) / 2, 30, 20);
        wxRect closeRect(panelSize.GetWidth() - buttonWidth - 10 + 64, (TITLE_BAR_HEIGHT - 20) / 2, 30, 20);
        m_systemButtons->SetButtonRects(minimizeRect, maximizeRect, closeRect);
    }
}

void FloatingDockFrame::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(this);
    wxSize size = GetClientSize();

    // Draw title bar background
    wxRect titleBarRect(0, 0, size.GetWidth(), TITLE_BAR_HEIGHT);
    dc.SetBrush(wxBrush(CFG_COLOUR("PrimaryBgColour")));
    dc.SetPen(wxPen(CFG_COLOUR("PrimaryBorderColour")));
    dc.DrawRectangle(titleBarRect);

    // Draw separator line
    dc.SetPen(wxPen(CFG_COLOUR("PrimaryBorderColour"), 1));
    dc.DrawLine(0, TITLE_BAR_HEIGHT, size.GetWidth(), TITLE_BAR_HEIGHT);

    // Call parent paint for content area
    event.Skip();
}

void FloatingDockFrame::OnLeftDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    if (IsPointInTitleBar(pos)) {
        // Start dragging from title bar
        CaptureMouse();
        wxFrame* frame = dynamic_cast<wxFrame*>(this);
        if (frame) {
            // Use wxWidgets built-in dragging for frames
            // This will handle the actual dragging
        }
    } else {
        // Pass to parent for resizing
        BorderlessFrameLogic::OnLeftDown(event);
    }
}

void FloatingDockFrame::OnLeftUp(wxMouseEvent& event)
{
    if (HasCapture()) {
        ReleaseMouse();
    }
    BorderlessFrameLogic::OnLeftUp(event);
}

void FloatingDockFrame::OnMotion(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    if (IsPointInTitleBar(pos)) {
        SetCursor(wxCursor(wxCURSOR_HAND));
    } else {
        ResetCursorToDefault();
        BorderlessFrameLogic::OnMotion(event);
    }
}

bool FloatingDockFrame::IsPointInTitleBar(const wxPoint& pos) const
{
    return pos.y >= 0 && pos.y <= TITLE_BAR_HEIGHT;
}

wxRect FloatingDockFrame::GetTitleBarRect() const
{
    return wxRect(0, 0, GetSize().GetWidth(), TITLE_BAR_HEIGHT);
}

void FloatingDockFrame::OnSystemButtonMinimize(wxCommandEvent& event)
{
    wxFrame* frame = dynamic_cast<wxFrame*>(this);
    if (frame) {
        frame->Iconize();
    }
}

void FloatingDockFrame::OnSystemButtonMaximize(wxCommandEvent& event)
{
    wxFrame* frame = dynamic_cast<wxFrame*>(this);
    if (frame) {
        if (frame->IsMaximized()) {
            frame->Restore();
        } else {
            frame->Maximize();
        }
    }
}

void FloatingDockFrame::OnSystemButtonMouseDown(wxMouseEvent& event)
{
    // Convert position to system buttons coordinate space
    wxPoint pos = event.GetPosition();
    wxPoint screenPos = ClientToScreen(pos);
    wxPoint buttonPos = m_systemButtons->ScreenToClient(screenPos);

    // Calculate button positions based on system buttons layout
    int buttonWidth = m_systemButtons->GetConfiguredButtonWidth();
    int buttonSpacing = m_systemButtons->GetConfiguredButtonSpacing();
    wxSize buttonSize = m_systemButtons->GetSize();

    // Buttons are right-aligned in the system buttons control
    int currentX = buttonSize.GetWidth() - buttonWidth;
    wxRect closeRect(currentX, 0, buttonWidth, buttonSize.GetHeight());

    currentX -= buttonSpacing + buttonWidth;
    wxRect maximizeRect(currentX, 0, buttonWidth, buttonSize.GetHeight());

    currentX -= buttonSpacing + buttonWidth;
    wxRect minimizeRect(currentX, 0, buttonWidth, buttonSize.GetHeight());

    if (closeRect.Contains(buttonPos)) {
        OnSystemButtonClose(wxCommandEvent());
    } else if (maximizeRect.Contains(buttonPos)) {
        OnSystemButtonMaximize(wxCommandEvent());
    } else if (minimizeRect.Contains(buttonPos)) {
        OnSystemButtonMinimize(wxCommandEvent());
    }

    event.Skip();
}

void FloatingDockFrame::OnSystemButtonClose(wxCommandEvent& event)
{
    // Send close event instead of directly closing
    // This allows derived classes to handle the close event properly
    wxCloseEvent closeEvent(wxEVT_CLOSE_WINDOW, GetId());
    closeEvent.SetCanVeto(true);
    GetEventHandler()->ProcessEvent(closeEvent);

    if (!closeEvent.GetVeto()) {
        Destroy();
    }
}

} // namespace ads
