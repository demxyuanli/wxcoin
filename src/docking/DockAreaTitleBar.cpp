// DockAreaTitleBar.cpp - Implementation of DockAreaTitleBar class

#include "docking/DockArea.h"
#include "docking/DockContainerWidget.h"
#include "config/SvgIconManager.h"
#include <wx/dcbuffer.h>
#include <wx/button.h>

namespace ads {

// Event table for DockAreaTitleBar
wxBEGIN_EVENT_TABLE(DockAreaTitleBar, wxPanel)
    EVT_PAINT(DockAreaTitleBar::onPaint)
wxEND_EVENT_TABLE()

// DockAreaTitleBar implementation (legacy)
DockAreaTitleBar::DockAreaTitleBar(DockArea* dockArea)
    : wxPanel(dockArea)
    , m_dockArea(dockArea)
    , m_titleLabel(nullptr)
    , m_closeButton(nullptr)
    , m_autoHideButton(nullptr)
    , m_menuButton(nullptr)
    , m_pinButton(nullptr)
    , m_layout(nullptr)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 25));

    // Create layout
    m_layout = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_layout);

    // Create title label
    m_titleLabel = new wxStaticText(this, wxID_ANY, "");
    m_layout->Add(m_titleLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

    // Create buttons
    createButtons();

    // Note: updateTitle() is deferred to avoid race conditions during construction
    // It will be called later when the object is fully initialized
}

DockAreaTitleBar::~DockAreaTitleBar() {
    // Clear pointers to prevent access after destruction
    m_titleLabel = nullptr;
    m_closeButton = nullptr;
    m_autoHideButton = nullptr;
    m_menuButton = nullptr;
    m_pinButton = nullptr;
    m_layout = nullptr;
    m_dockArea = nullptr;
}

void DockAreaTitleBar::updateTitle() {
    if (!m_dockArea || !m_titleLabel) {
        return;
    }
    
    wxString title = m_dockArea->currentTabTitle();
    m_titleLabel->SetLabel(title);
    Layout();
}

void DockAreaTitleBar::updateButtonStates() {
    // Update button visibility based on features
    if (!m_dockArea || !m_closeButton) {
        return;
    }

    // Check if we should disable close button
    if (m_dockArea->dockContainer()) {
        bool canClose = m_dockArea->dockContainer()->dockAreaCount() > 1;
        m_closeButton->Enable(canClose);

        // Update tooltip
        if (!canClose) {
            m_closeButton->SetToolTip("Cannot close the last dock area");
        } else {
            m_closeButton->SetToolTip("Close this dock area");
        }
    }
}

void DockAreaTitleBar::showCloseButton(bool show) {
    if (m_closeButton) {
        m_closeButton->Show(show);
        Layout();
    }
}

void DockAreaTitleBar::showAutoHideButton(bool show) {
    if (m_autoHideButton) {
        m_autoHideButton->Show(show);
        Layout();
    }
}

void DockAreaTitleBar::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

    // Set font from ThemeManager
    dc.SetFont(style.font);

    // Draw styled background using new style system
    wxRect rect(0, 0, GetClientSize().GetWidth(), style.titleBarHeight);
    DrawStyledRect(dc, rect, style, false, false, true);  // Title bar with bottom border
    
    // Draw decorative pattern on title bar
    drawTitleBarPattern(dc, rect);
}

void DockAreaTitleBar::onCloseButtonClicked(wxCommandEvent& event) {
    if (m_dockArea) {
        m_dockArea->closeArea();
    }
}

void DockAreaTitleBar::onAutoHideButtonClicked(wxCommandEvent& event) {
    // TODO: Implement auto-hide
}

void DockAreaTitleBar::onMenuButtonClicked(wxCommandEvent& event) {
    // TODO: Show dock area menu
}

void DockAreaTitleBar::onPinButtonClicked(wxCommandEvent& event) {
    // TODO: Auto-hide feature not yet fully implemented
    // For now, just show a message
    wxMessageBox("Auto-hide feature is not yet implemented", "Info", wxOK | wxICON_INFORMATION);
}

void DockAreaTitleBar::createButtons() {
    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

    // Create pin button (for auto-hide) - borderless with SVG icon
    m_pinButton = new wxButton(this, wxID_ANY, "", wxDefaultPosition,
                                      wxSize(12, 12));  // Fixed 12x12 size
    // Remove tool tip - no hover hints as specified
    m_pinButton->SetWindowStyle(wxBORDER_NONE); // No border
    m_pinButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onPinButtonClicked, this);
    m_layout->Add(m_pinButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);

    // Set pin button icon
    if (style.useSvgIcons) {
        try {
            wxBitmap pinIcon = SvgIconManager::GetInstance().GetIconBitmap(
                style.pinIconName, wxSize(style.buttonSize, style.buttonSize));
            if (pinIcon.IsOk()) {
                m_pinButton->SetBitmap(pinIcon);
            }
        } catch (...) {}
    }

    // Create close button - borderless with SVG icon
    m_closeButton = new wxButton(this, wxID_ANY, "", wxDefaultPosition,
                                wxSize(12, 12));  // Fixed 12x12 size
    // Remove tool tip - no hover hints as specified
    m_closeButton->SetWindowStyle(wxBORDER_NONE); // No border
    m_closeButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onCloseButtonClicked, this);
    m_layout->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);

    // Set close button icon
    if (style.useSvgIcons) {
        try {
            wxBitmap closeIcon = SvgIconManager::GetInstance().GetIconBitmap(
                style.closeIconName, wxSize(12, 12));
            if (closeIcon.IsOk()) {
                m_closeButton->SetBitmap(closeIcon);
            }
        } catch (...) {}
    }

    // Create auto-hide button - borderless with SVG icon
    m_autoHideButton = new wxButton(this, wxID_ANY, "", wxDefaultPosition,
                                   wxSize(12, 12));  // Fixed 12x12 size
    // Remove tool tip - no hover hints as specified
    m_autoHideButton->SetWindowStyle(wxBORDER_NONE); // No border
    m_autoHideButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onAutoHideButtonClicked, this);
    m_layout->Add(m_autoHideButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    m_autoHideButton->Hide(); // Hidden by default

    // Set auto-hide button icon
    if (style.useSvgIcons) {
        try {
            wxBitmap autoHideIcon = SvgIconManager::GetInstance().GetIconBitmap(
                style.autoHideIconName, wxSize(12, 12));
            if (autoHideIcon.IsOk()) {
                m_autoHideButton->SetBitmap(autoHideIcon);
            }
        } catch (...) {}
    }

    // Create menu button - borderless with SVG icon
    m_menuButton = new wxButton(this, wxID_ANY, "", wxDefaultPosition,
                               wxSize(12, 12));  // Fixed 12x12 size
    // Remove tool tip - no hover hints as specified
    m_menuButton->SetWindowStyle(wxBORDER_NONE); // No border
    m_menuButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onMenuButtonClicked, this);
    m_layout->Add(m_menuButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);

    // Set menu button icon
    if (style.useSvgIcons) {
        try {
            wxBitmap menuIcon = SvgIconManager::GetInstance().GetIconBitmap(
                style.menuIconName, wxSize(12, 12));
            if (menuIcon.IsOk()) {
                m_menuButton->SetBitmap(menuIcon);
            }
        } catch (...) {}
    }
}

void DockAreaTitleBar::drawTitleBarPattern(wxDC& dc, const wxRect& rect) {
    // Draw decorative horizontal dot bar between title and buttons
    // Create a 3x5 pixel dot pattern decoration in the middle area
    
    // Save current pen and brush
    wxPen oldPen = dc.GetPen();
    wxBrush oldBrush = dc.GetBrush();
    
    // Calculate the position for the decoration bar
    // Find the rightmost title text and leftmost button to position the bar between them
    int leftX = 0;
    int rightX = rect.width;
    
    // Find rightmost title text position
    if (m_titleLabel && m_titleLabel->IsShown()) {
        wxRect titleRect = m_titleLabel->GetRect();
        leftX = std::max(leftX, titleRect.GetRight());
    }
    
    // Find leftmost button position
    if (m_pinButton && m_pinButton->IsShown()) {
        wxRect buttonRect = m_pinButton->GetRect();
        rightX = std::min(rightX, buttonRect.GetLeft());
    }
    if (m_menuButton && m_menuButton->IsShown()) {
        wxRect buttonRect = m_menuButton->GetRect();
        rightX = std::min(rightX, buttonRect.GetLeft());
    }
    if (m_autoHideButton && m_autoHideButton->IsShown()) {
        wxRect buttonRect = m_autoHideButton->GetRect();
        rightX = std::min(rightX, buttonRect.GetLeft());
    }
    if (m_closeButton && m_closeButton->IsShown()) {
        wxRect buttonRect = m_closeButton->GetRect();
        rightX = std::min(rightX, buttonRect.GetLeft());
    }
    
    // Add some margin
    leftX += 8;   // 8px margin from title
    rightX -= 8;  // 8px margin from buttons
    
    // Only draw if there's enough space
    if (rightX > leftX + 20) {
        // Get style config with theme initialization
        const DockStyleConfig& style = GetDockStyleConfig();
        
        // Set dot pattern colors from theme
        wxColour dotColor = style.patternDotColour;
        
        // Set pen and brush for dots
        dc.SetPen(wxPen(dotColor, 1));
        dc.SetBrush(wxBrush(dotColor));
        
        // Pattern parameters from theme configuration
        int patternWidth = style.patternWidth;   // Pattern width from theme
        int patternHeight = style.patternHeight; // Pattern height from theme
        int dotSize = 1;        // 1 pixel dots
        
        // Calculate vertical center position
        int centerY = rect.y + (rect.height - patternHeight) / 2;
        
        // Draw horizontal tiling pattern across the available space (no spacing)
        int currentX = leftX;
        int patternCount = 0;
        
        while (currentX + patternWidth <= rightX) {
            // Draw the 3 specific dots in 3x3 pattern at current position
            // Top-left dot (position 0,0)
            dc.DrawCircle(currentX, centerY, dotSize);
            
            // Bottom-left dot (position 0,2)
            dc.DrawCircle(currentX, centerY + 2, dotSize);
            
            // Right-middle dot (position 2,1)
            dc.DrawCircle(currentX + 2, centerY + 1, dotSize);
            
            // Move to next pattern position (no spacing)
            currentX += patternWidth;
            patternCount++;
        }
    }
    
    // Restore original pen and brush
    dc.SetPen(oldPen);
    dc.SetBrush(oldBrush);
}

} // namespace ads
