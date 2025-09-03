// DockLayoutPreview.cpp - Implementation of DockLayoutPreview class

#include "docking/DockLayoutConfig.h"
#include <wx/dcbuffer.h>

namespace ads {

// Event table for DockLayoutPreview
wxBEGIN_EVENT_TABLE(DockLayoutPreview, wxPanel)
    EVT_PAINT(DockLayoutPreview::OnPaint)
    EVT_SIZE(DockLayoutPreview::OnSize)
wxEND_EVENT_TABLE()

// DockLayoutPreview implementation
DockLayoutPreview::DockLayoutPreview(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);
}

void DockLayoutPreview::SetConfig(const DockLayoutConfig& config) {
    m_config = config;
    Refresh();
}

void DockLayoutPreview::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    DrawLayoutPreview(dc);
}

void DockLayoutPreview::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

void DockLayoutPreview::DrawLayoutPreview(wxDC& dc) {
    wxRect clientRect = GetClientRect();
    clientRect.Deflate(10); // Margin

    // Colors for different areas
    wxColour topColor(200, 200, 255);      // Light blue
    wxColour bottomColor(200, 255, 200);   // Light green
    wxColour leftColor(255, 200, 200);     // Light red
    wxColour rightColor(255, 255, 200);    // Light yellow
    wxColour centerColor(240, 240, 240);   // Light gray
    wxColour borderColor(100, 100, 100);   // Dark gray
    wxColour textColor(50, 50, 50);       // Dark gray for text

    // Calculate areas
    wxRect topRect = CalculateAreaRect(TopDockWidgetArea, clientRect);
    wxRect bottomRect = CalculateAreaRect(BottomDockWidgetArea, clientRect);
    wxRect leftRect = CalculateAreaRect(LeftDockWidgetArea, clientRect);
    wxRect rightRect = CalculateAreaRect(RightDockWidgetArea, clientRect);
    wxRect centerRect = CalculateAreaRect(CenterDockWidgetArea, clientRect);

    // Draw areas
    dc.SetPen(wxPen(borderColor, 1));

    if (m_config.showTopArea && !topRect.IsEmpty()) {
        dc.SetBrush(wxBrush(topColor));
        dc.DrawRectangle(topRect);

        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Top";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.topAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.topAreaHeight);
        }
        dc.DrawText(label, topRect.x + 5, topRect.y + 5);
    }

    if (m_config.showBottomArea && !bottomRect.IsEmpty()) {
        dc.SetBrush(wxBrush(bottomColor));
        dc.DrawRectangle(bottomRect);

        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Bottom";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.bottomAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.bottomAreaHeight);
        }
        dc.DrawText(label, bottomRect.x + 5, bottomRect.y + 5);
    }

    if (m_config.showLeftArea && !leftRect.IsEmpty()) {
        dc.SetBrush(wxBrush(leftColor));
        dc.DrawRectangle(leftRect);

        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Left";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.leftAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.leftAreaWidth);
        }
        dc.DrawText(label, leftRect.x + 5, leftRect.y + 5);
    }

    if (m_config.showRightArea && !rightRect.IsEmpty()) {
        dc.SetBrush(wxBrush(rightColor));
        dc.DrawRectangle(rightRect);

        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Right";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.rightAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.rightAreaWidth);
        }
        dc.DrawText(label, rightRect.x + 5, rightRect.y + 5);
    }

    // Always draw center
    dc.SetBrush(wxBrush(centerColor));
    dc.DrawRectangle(centerRect);

    // Draw center label with calculated percentage
    dc.SetTextForeground(textColor);
    wxString centerLabel = "Center";
    if (m_config.usePercentage) {
        int leftRight = (m_config.showLeftArea ? m_config.leftAreaPercent : 0) +
                       (m_config.showRightArea ? m_config.rightAreaPercent : 0);
        int topBottom = (m_config.showTopArea ? m_config.topAreaPercent : 0) +
                       (m_config.showBottomArea ? m_config.bottomAreaPercent : 0);
        centerLabel += wxString::Format(" (H:%d%%, V:%d%%)", 100 - leftRight, 100 - topBottom);
    }
    dc.DrawText(centerLabel, centerRect.x + 5, centerRect.y + 5);

    // Draw splitter lines
    dc.SetPen(wxPen(borderColor, m_config.splitterWidth));

    if (m_config.showTopArea) {
        dc.DrawLine(clientRect.x, topRect.GetBottom(),
                   clientRect.GetRight(), topRect.GetBottom());
    }

    if (m_config.showBottomArea) {
        dc.DrawLine(clientRect.x, bottomRect.GetTop(),
                   clientRect.GetRight(), bottomRect.GetTop());
    }

    if (m_config.showLeftArea) {
        dc.DrawLine(leftRect.GetRight(), leftRect.GetTop(),
                   leftRect.GetRight(), leftRect.GetBottom());
    }

    if (m_config.showRightArea) {
        dc.DrawLine(rightRect.GetLeft(), rightRect.GetTop(),
                   rightRect.GetLeft(), rightRect.GetBottom());
    }
}

wxRect DockLayoutPreview::CalculateAreaRect(DockWidgetArea area, const wxRect& totalRect) {
    int topHeight = 0, bottomHeight = 0, leftWidth = 0, rightWidth = 0;

    if (m_config.usePercentage) {
        // Calculate from percentages
        if (m_config.showTopArea) {
            topHeight = totalRect.height * m_config.topAreaPercent / 100;
        }
        if (m_config.showBottomArea) {
            bottomHeight = totalRect.height * m_config.bottomAreaPercent / 100;
        }
        if (m_config.showLeftArea) {
            leftWidth = totalRect.width * m_config.leftAreaPercent / 100;
        }
        if (m_config.showRightArea) {
            rightWidth = totalRect.width * m_config.rightAreaPercent / 100;
        }
    } else {
        // Scale pixel values to fit preview
        double scaleX = (double)totalRect.width / 1200.0;  // Assume 1200px reference width
        double scaleY = (double)totalRect.height / 800.0;  // Assume 800px reference height

        if (m_config.showTopArea) {
            topHeight = m_config.topAreaHeight * scaleY;
        }
        if (m_config.showBottomArea) {
            bottomHeight = m_config.bottomAreaHeight * scaleY;
        }
        if (m_config.showLeftArea) {
            leftWidth = m_config.leftAreaWidth * scaleX;
        }
        if (m_config.showRightArea) {
            rightWidth = m_config.rightAreaWidth * scaleX;
        }
    }

    // Calculate actual rectangles
    switch (area) {
    case TopDockWidgetArea:
        if (m_config.showTopArea) {
            return wxRect(totalRect.x, totalRect.y, totalRect.width, topHeight);
        }
        break;

    case BottomDockWidgetArea:
        if (m_config.showBottomArea) {
            return wxRect(totalRect.x, totalRect.GetBottom() - bottomHeight,
                         totalRect.width, bottomHeight);
        }
        break;

    case LeftDockWidgetArea:
        if (m_config.showLeftArea) {
            int y = totalRect.y + topHeight;
            int h = totalRect.height - topHeight - bottomHeight;
            return wxRect(totalRect.x, y, leftWidth, h);
        }
        break;

    case RightDockWidgetArea:
        if (m_config.showRightArea) {
            int y = totalRect.y + topHeight;
            int h = totalRect.height - topHeight - bottomHeight;
            return wxRect(totalRect.GetRight() - rightWidth, y, rightWidth, h);
        }
        break;

    case CenterDockWidgetArea:
        {
            int x = totalRect.x + leftWidth;
            int y = totalRect.y + topHeight;
            int w = totalRect.width - leftWidth - rightWidth;
            int h = totalRect.height - topHeight - bottomHeight;
            return wxRect(x, y, w, h);
        }
        break;
    }

    return wxRect();
}

} // namespace ads
