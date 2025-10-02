#include "widgets/FramelessModalPopup.h"
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/dcclient.h>
#include <wx/display.h>


wxBEGIN_EVENT_TABLE(FramelessModalPopup, wxDialog)
EVT_PAINT(FramelessModalPopup::OnPaint)
EVT_SIZE(FramelessModalPopup::OnSize)
EVT_MOTION(FramelessModalPopup::OnMouseMove)
EVT_LEFT_DOWN(FramelessModalPopup::OnMouseLeftDown)
EVT_LEFT_UP(FramelessModalPopup::OnMouseLeftUp)
EVT_LEAVE_WINDOW(FramelessModalPopup::OnMouseLeave)
wxEND_EVENT_TABLE()

FramelessModalPopup::FramelessModalPopup(wxWindow* parent,
                                       const wxString& title,
                                       const wxSize& size)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, size,
               wxNO_BORDER | wxFRAME_SHAPED),
      m_contentPanel(nullptr),
      m_titleText(title),
      m_titleIconSize(wxSize(16, 16)),
      m_showTitleIcon(false),
      m_closeButtonHovered(false),
      m_closeButtonPressed(false),
      m_dragging(false),
      m_inSizeEvent(false)
{
    // Set background style for custom painting
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);

    // Load theme configuration
    LoadThemeConfiguration();

    // Initialize close icon
    m_closeIcon = SVG_ICON("close", wxSize(16, 16));
    if (!m_closeIcon.IsOk()) {
        // Fallback to wxArtProvider
        m_closeIcon = wxArtProvider::GetBitmap(wxART_CLOSE);
    }

    // Create content panel
    CreateControls();
    LayoutControls();

    // Set minimum size including title bar
    SetMinSize(wxSize(180, 80 + TITLE_BAR_HEIGHT));

    // Center on parent window and ensure size constraints
    if (parent) {
        // Get parent window's position and size
        wxRect parentRect = parent->GetRect();
        wxSize dialogSize = GetSize();
        
        // Ensure dialog size doesn't exceed parent window size, but allow larger dialogs
        if (dialogSize.GetWidth() > parentRect.width) {
            // Only constrain if parent is very small, otherwise allow larger dialogs
            if (parentRect.width < 600) {
                dialogSize.SetWidth(parentRect.width - 20); // Leave some margin
            }
        }
        if (dialogSize.GetHeight() > parentRect.height) {
            // Only constrain if parent is very small, otherwise allow larger dialogs
            if (parentRect.height < 600) {
                dialogSize.SetHeight(parentRect.height - 20); // Leave some margin
            }
        }
        
        // Set the constrained size
        SetSize(dialogSize);
        
        // Recalculate center position based on the actual dialog size after constraints
        wxSize actualSize = GetSize();
        
        // Use screen coordinates for positioning
        wxPoint parentScreenPos = parent->ClientToScreen(wxPoint(0, 0));
        int centerX = parentScreenPos.x + (parentRect.width - actualSize.GetWidth()) / 2;
        int centerY = parentScreenPos.y + (parentRect.height - actualSize.GetHeight()) / 2;
        
        // Ensure dialog stays within screen bounds
        wxRect screenRect = wxDisplay().GetClientArea();
        if (centerX < screenRect.x) centerX = screenRect.x + 10;
        if (centerY < screenRect.y) centerY = screenRect.y + 10;
        if (centerX + actualSize.GetWidth() > screenRect.x + screenRect.width) {
            centerX = screenRect.x + screenRect.width - actualSize.GetWidth() - 10;
        }
        if (centerY + actualSize.GetHeight() > screenRect.y + screenRect.height) {
            centerY = screenRect.y + screenRect.height - actualSize.GetHeight() - 10;
        }
        
        SetPosition(wxPoint(centerX, centerY));
    } else {
        Centre();
    }

    // Bind close button event
    Bind(wxEVT_BUTTON, &FramelessModalPopup::OnCloseButton, this, wxID_CLOSE);

    // Register theme change listener
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        OnThemeChanged();
    });
}

FramelessModalPopup::~FramelessModalPopup()
{
    // Remove theme change listener
    ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FramelessModalPopup::SetTitle(const wxString& title)
{
    m_titleText = title;
    Refresh();
}

void FramelessModalPopup::SetTitleIcon(const wxBitmap& icon)
{
    m_titleIcon = icon;
    m_titleIconName.Clear(); // Clear SVG name since we're using a bitmap
    Refresh();
}

void FramelessModalPopup::SetTitleIcon(const wxString& svgIconName, const wxSize& size)
{
    m_titleIconName = svgIconName;
    m_titleIconSize = size;
    m_titleIcon = SVG_ICON(svgIconName, size);
    if (!m_titleIcon.IsOk()) {
        // Fallback to a default icon or leave empty
        m_titleIcon = wxNullBitmap;
    }
    Refresh();
}

void FramelessModalPopup::ShowTitleIcon(bool show)
{
    m_showTitleIcon = show;
    Refresh();
}

void FramelessModalPopup::LoadThemeConfiguration()
{
    // Check if theme manager is initialized
    if (!ThemeManager::getInstance().isInitialized()) {
        wxLogDebug("FramelessModalPopup LoadThemeConfiguration - ThemeManager not initialized, using defaults");
        CLOSE_BUTTON_SIZE = 30; // Use SystemButtonHeight as default
        CLOSE_BUTTON_MARGIN = 2; // Use SystemButtonSpacing as default
        BORDER_WIDTH = 1;
        TITLE_BAR_HEIGHT = 32;
        return;
    }

    // Load layout constants from theme manager
    // Use SystemButtonHeight for close button size to match FlatUIBar
    CLOSE_BUTTON_SIZE = CFG_INT("SystemButtonHeight");
    if (CLOSE_BUTTON_SIZE <= 0) CLOSE_BUTTON_SIZE = 30; // fallback

    // Use SystemButtonSpacing for close button margin
    CLOSE_BUTTON_MARGIN = CFG_INT("SystemButtonSpacing");
    if (CLOSE_BUTTON_MARGIN <= 0) CLOSE_BUTTON_MARGIN = 2; // fallback

    BORDER_WIDTH = CFG_INT("BorderWidth");
    if (BORDER_WIDTH <= 0) BORDER_WIDTH = 1; // fallback

    TITLE_BAR_HEIGHT = CFG_INT("TitleBarHeight");
    if (TITLE_BAR_HEIGHT <= 0) TITLE_BAR_HEIGHT = 32; // fallback

    // Debug output
    wxLogDebug("FramelessModalPopup Constructor - TitleBarHeight: %d, BorderWidth: %d, Title: %s",
               TITLE_BAR_HEIGHT, BORDER_WIDTH, m_titleText.c_str());
}

void FramelessModalPopup::OnThemeChanged()
{
    // Reload theme configuration
    LoadThemeConfiguration();

    // Update UI elements
    if (m_contentPanel) {
        m_contentPanel->SetBackgroundColour(CFG_COLOUR("PanelBgColour"));
    }

    // Reinitialize close icon with new theme colors
    m_closeIcon = SVG_ICON("close", wxSize(16, 16));
    if (!m_closeIcon.IsOk()) {
        m_closeIcon = wxArtProvider::GetBitmap(wxART_CLOSE);
    }

    // Update title icon if it's an SVG icon (reapply theme colors)
    if (m_showTitleIcon && !m_titleIconName.IsEmpty()) {
        m_titleIcon = SVG_ICON(m_titleIconName, m_titleIconSize);
        if (!m_titleIcon.IsOk()) {
            m_titleIcon = wxNullBitmap;
        }
    }

    // Update layout
    UpdateLayout();
}

void FramelessModalPopup::CreateControls()
{
    // Create a main sizer for the dialog to control layout
    wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add spacer for title bar
    dialogSizer->AddSpacer(TITLE_BAR_HEIGHT);
    
    // Create content panel that fills the remaining area
    m_contentPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxTAB_TRAVERSAL | wxNO_BORDER);
    
    // Set background color safely
    if (ThemeManager::getInstance().isInitialized()) {
        m_contentPanel->SetBackgroundColour(CFG_COLOUR("PanelBgColour"));
    } else {
        m_contentPanel->SetBackgroundColour(wxColour(250, 250, 250)); // Default light gray
    }
    
    // Add content panel to sizer with proper borders
    dialogSizer->Add(m_contentPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BORDER_WIDTH);
    
    // Set the sizer for this dialog
    SetSizer(dialogSizer);
    
    wxLogDebug("FramelessModalPopup CreateControls - Content panel added to sizer with title bar spacer: %d", 
               TITLE_BAR_HEIGHT);
}

void FramelessModalPopup::LayoutControls()
{
    // Don't set sizer here - let derived classes manage their own content layout
    // The title bar is drawn in the paint event, so we don't need to reserve space in the sizer
    
    wxLogDebug("FramelessModalPopup LayoutControls - TitleBarHeight: %d, BorderWidth: %d (drawn in paint event)", 
               TITLE_BAR_HEIGHT, BORDER_WIDTH);
}

void FramelessModalPopup::UpdateLayout()
{
    LayoutControls();
    Refresh();
}

wxRect FramelessModalPopup::GetCloseButtonRect() const
{
    wxRect clientRect = GetClientRect();
    int buttonX = clientRect.GetRight() - CLOSE_BUTTON_MARGIN - CLOSE_BUTTON_SIZE;
    int buttonY = CLOSE_BUTTON_MARGIN;

    return wxRect(buttonX, buttonY, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE);
}

bool FramelessModalPopup::IsCloseButtonHit(const wxPoint& pos) const
{
    return GetCloseButtonRect().Contains(pos);
}

void FramelessModalPopup::DrawTitleBarContent(wxGraphicsContext* gc)
{
    if (!gc) {
        wxLogDebug("FramelessModalPopup DrawTitleBarContent - Graphics context is null");
        return;
    }

    wxColour titleTextColor = CFG_COLOUR("TitleBarTextColour");
    if (!titleTextColor.IsOk()) {
        titleTextColor = CFG_COLOUR("PanelTextColour");
    }
    if (!titleTextColor.IsOk()) {
        titleTextColor = wxColour(0, 0, 0); // Fallback to black
    }

    // Set up font for title text
    wxFont titleFont = CFG_FONT();
    titleFont.SetPointSize(titleFont.GetPointSize() + 1);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    gc->SetFont(titleFont, titleTextColor);

    // Calculate positions
    int leftMargin = this->BORDER_WIDTH + 8; // Left margin from border
    int currentX = leftMargin;

    // Draw title icon if enabled and available
    if (this->m_showTitleIcon && this->m_titleIcon.IsOk()) {
        int iconY = (this->TITLE_BAR_HEIGHT - this->m_titleIcon.GetHeight()) / 2;
        gc->DrawBitmap(this->m_titleIcon, currentX, iconY,
                      this->m_titleIcon.GetWidth(), this->m_titleIcon.GetHeight());
        currentX += this->m_titleIcon.GetWidth() + 8; // Add spacing after icon
    }

    // Draw title text if available
    if (!this->m_titleText.IsEmpty()) {
        double textWidth, textHeight, descent, externalLeading;
        gc->GetTextExtent(this->m_titleText, &textWidth, &textHeight, &descent, &externalLeading);

        int textY = (this->TITLE_BAR_HEIGHT - textHeight) / 2;
        gc->DrawText(this->m_titleText, currentX, textY);
    }
}

void FramelessModalPopup::DrawBorder(wxDC& dc)
{
    wxRect clientRect = GetClientRect();

    // Draw border around the window
    wxPen borderPen(CFG_COLOUR("BorderColour"), BORDER_WIDTH);
    dc.SetPen(borderPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    // Draw four sides of the border
    if (BORDER_WIDTH > 0) {
        // Top border
        dc.DrawLine(0, 0, clientRect.GetWidth(), 0);
        // Bottom border
        dc.DrawLine(0, clientRect.GetHeight() - BORDER_WIDTH, clientRect.GetWidth(), clientRect.GetHeight() - BORDER_WIDTH);
        // Left border
        dc.DrawLine(0, 0, 0, clientRect.GetHeight());
        // Right border
        dc.DrawLine(clientRect.GetWidth() - BORDER_WIDTH, 0, clientRect.GetWidth() - BORDER_WIDTH, clientRect.GetHeight());
    }
}

void FramelessModalPopup::OnPaint(wxPaintEvent& event)
{
    wxUnusedVar(event);

    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

    if (gc) {
        wxRect clientRect = GetClientRect();

        // Clear background
        gc->SetBrush(wxBrush(CFG_COLOUR("PanelBgColour")));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, clientRect.GetWidth(), clientRect.GetHeight());

        // Draw title bar background
        wxColour titleBarBg = CFG_COLOUR("TitleBarBgColour");

        // Use theme-managed title bar background color
        if (!titleBarBg.IsOk()) {
            // Fallback to a default title bar color if not configured
            titleBarBg = wxColour(200, 200, 200);
        }

        // Draw title bar background with theme-managed color
        gc->SetBrush(wxBrush(titleBarBg));
        gc->SetPen(wxPen(wxColour(100, 100, 100), 1)); // Dark border for title bar
        gc->DrawRectangle(0, 0, clientRect.GetWidth(), TITLE_BAR_HEIGHT);

        // Draw title bar separator line
        wxColour separatorColor = CFG_COLOUR("BorderColour");
        gc->SetPen(wxPen(separatorColor, 1));
        gc->StrokeLine(0, TITLE_BAR_HEIGHT, clientRect.GetWidth(), TITLE_BAR_HEIGHT);

        // Draw close button
        wxRect closeButtonRect = GetCloseButtonRect();
        
        // Use FlatUISystemButtons style for close button
        wxColour normalBgColour = CFG_COLOUR("TitleBarBgColour");
        wxColour hoverBgColour = CFG_COLOUR("SystemButtonCloseHoverColour");
        wxColour textColour = CFG_COLOUR("SystemButtonTextColour");
        wxColour hoverTextColour = CFG_COLOUR("SystemButtonHoverTextColour");

        if (m_closeButtonPressed) {
            gc->SetBrush(wxBrush(hoverBgColour));
        } else if (m_closeButtonHovered) {
            gc->SetBrush(wxBrush(hoverBgColour));
        } else {
            gc->SetBrush(wxBrush(normalBgColour));
        }

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(closeButtonRect.x, closeButtonRect.y,
                         closeButtonRect.width, closeButtonRect.height);

        // Draw close icon
        if (m_closeIcon.IsOk()) {
            int iconX = closeButtonRect.x + (closeButtonRect.width - m_closeIcon.GetWidth()) / 2;
            int iconY = closeButtonRect.y + (closeButtonRect.height - m_closeIcon.GetHeight()) / 2;

            // Use appropriate text color for the icon
            wxColour iconColor = m_closeButtonHovered ? hoverTextColour : textColour;
            gc->SetPen(wxPen(iconColor, 1));
            
            gc->DrawBitmap(m_closeIcon, iconX, iconY,
                          m_closeIcon.GetWidth(), m_closeIcon.GetHeight());
        }

        // Draw title bar content (title text and icon)
        DrawTitleBarContent(gc);

        // Draw border
        DrawBorder(dc);

        delete gc;
    } else {
        wxLogDebug("FramelessModalPopup OnPaint - Failed to create graphics context");
    }
}

void FramelessModalPopup::OnSize(wxSizeEvent& event)
{
    // Prevent recursive OnSize events
    if (m_inSizeEvent) {
        event.Skip();
        return;
    }
    
    m_inSizeEvent = true;
    
    // Sizer will automatically handle content panel layout
    // Just trigger a layout update
    Layout();
    
    m_inSizeEvent = false;
    event.Skip();
}

void FramelessModalPopup::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();

    bool wasCloseButtonHovered = m_closeButtonHovered;
    m_closeButtonHovered = IsCloseButtonHit(pos);

    // Handle dragging
    if (m_dragging && event.Dragging()) {
        wxPoint currentPos = ClientToScreen(pos);
        wxPoint delta = currentPos - m_dragStartPos;
        wxPoint newPos = GetPosition() + delta;

        Move(newPos);
        m_dragStartPos = currentPos;
    }

    // Refresh if hover state changed
    if (wasCloseButtonHovered != m_closeButtonHovered) {
        Refresh();
    }
}

void FramelessModalPopup::OnMouseLeftDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();

    if (IsCloseButtonHit(pos)) {
        m_closeButtonPressed = true;
        Refresh();
    } else {
        // Start dragging if not on close button
        m_dragging = true;
        m_dragStartPos = ClientToScreen(pos);
        CaptureMouse();
    }
}

void FramelessModalPopup::OnMouseLeftUp(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();

    if (m_closeButtonPressed && IsCloseButtonHit(pos)) {
        // Close button was clicked
        Close();
    }

    m_closeButtonPressed = false;
    m_closeButtonHovered = false;

    if (m_dragging) {
        m_dragging = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }

    Refresh();
}

void FramelessModalPopup::OnMouseLeave(wxMouseEvent& event)
{
    wxUnusedVar(event);

    m_closeButtonHovered = false;
    m_closeButtonPressed = false;

    if (m_dragging) {
        m_dragging = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }

    Refresh();
}

void FramelessModalPopup::OnCloseButton(wxCommandEvent& event)
{
    wxUnusedVar(event);
    Close();
}

int FramelessModalPopup::ShowModal()
{
    // Ensure the window is modal
    wxWindow* parent = GetParent();
    if (parent) {
        parent->Disable();
        // Store current window position for restoration
        m_parentWindowRect = parent->GetRect();
    }

    int result = wxDialog::ShowModal();

    if (parent) {
        parent->Enable();
        // Restore parent window to top level
        parent->Raise();
        // Ensure parent window is visible and active
        parent->SetFocus();
        parent->Refresh();
    }

    return result;
}

void FramelessModalPopup::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    // Ensure minimum height includes title bar
    if (height > 0 && height < TITLE_BAR_HEIGHT + 50) {
        height = TITLE_BAR_HEIGHT + 50;
    }

    wxDialog::DoSetSize(x, y, width, height, sizeFlags);
    // Don't call UpdateLayout() here as it can cause sizer conflicts
    // Layout will be updated when needed by derived classes

    wxLogDebug("FramelessModalPopup DoSetSize - Size: %d,%d, TitleBarHeight: %d", width, height, TITLE_BAR_HEIGHT);
}
