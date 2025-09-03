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

    updateTitle();
}

DockAreaTitleBar::~DockAreaTitleBar() {
}

void DockAreaTitleBar::updateTitle() {
    wxString title = m_dockArea->currentTabTitle();
    m_titleLabel->SetLabel(title);
    Layout();
}

void DockAreaTitleBar::updateButtonStates() {
    // Update button visibility based on features

    // Check if we should disable close button
    if (m_dockArea && m_dockArea->dockContainer()) {
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
    m_closeButton->Show(show);
    Layout();
}

void DockAreaTitleBar::showAutoHideButton(bool show) {
    m_autoHideButton->Show(show);
    Layout();
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
}

void DockAreaTitleBar::onCloseButtonClicked(wxCommandEvent& event) {
    m_dockArea->closeArea();
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
    wxButton* pinButton = new wxButton(this, wxID_ANY, "", wxDefaultPosition,
                                      wxSize(12, 12));  // Fixed 12x12 size
    // Remove tool tip - no hover hints as specified
    pinButton->SetWindowStyle(wxBORDER_NONE); // No border
    pinButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onPinButtonClicked, this);
    m_layout->Add(pinButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);

    // Set pin button icon
    if (style.useSvgIcons) {
        try {
            wxBitmap pinIcon = SvgIconManager::GetInstance().GetIconBitmap(
                style.pinIconName, wxSize(style.buttonSize, style.buttonSize));
            if (pinIcon.IsOk()) {
                pinButton->SetBitmap(pinIcon);
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

} // namespace ads
