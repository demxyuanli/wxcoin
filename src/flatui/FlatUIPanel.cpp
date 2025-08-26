#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIGallery.h"
#include "flatui/FlatUIEventManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/event.h>
#include "config/ThemeManager.h"  


enum {
    TIMER_RESIZE = wxID_HIGHEST + 1000,
    TIMER_ADD_CONTROL = wxID_HIGHEST + 1001
};

FlatUIPanel::FlatUIPanel(FlatUIPage* parent, const wxString& label, int orientation) 
    : FlatUIThemeAware(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
    m_label(label),
    m_orientation(orientation),
    m_panelBorderTop(0),
    m_panelBorderBottom(0),
    m_panelBorderLeft(0),
    m_panelBorderRight(1),
    m_borderStyle(PanelBorderStyle::NONE),
    m_resizeTimer(this, TIMER_RESIZE)
{
    SetFont(GetThemeFont());
    SetDoubleBuffered(true);

    // Initialize theme-based configuration values
    m_bgColour = GetThemeColour("ActBarBackgroundColour");
    m_borderColour = GetThemeColour("PanelBorderColour");
    m_headerColour = GetThemeColour("PanelHeaderColour");
    m_headerTextColour = GetThemeColour("PanelHeaderTextColour");
    m_headerBorderColour = GetThemeColour("PanelBorderColour");

    // int headerArea = CFG_INT("PanelDefaultHeaderAreaSize", FLATUI_PANEL_DEFAULT_HEADER_AREA_SIZE);
    // int padVertical = CFG_INT("PanelInternalVerticalPadding", FLATUI_PANEL_INTERNAL_VERTICAL_PADDING);
    // int targetHeight= CFG_INT("PanelTargetHeight", FLATUI_PANEL_TARGET_HEIGHT);

    SetBackgroundColour(CFG_COLOUR("PanelBgColour"));

    SetBackgroundStyle(wxBG_STYLE_PAINT);

#ifdef __WXMSW__
    HWND hwnd = (HWND)GetHandle();
    if (hwnd) {
        long exStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ::SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED);
    }
#endif

    m_sizer = new wxBoxSizer(orientation);
    SetSizer(m_sizer);

    SetAutoLayout(true);

    FlatUIEventManager::getInstance().bindPanelEvents(this);

    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        Freeze();
        wxEventBlocker blocker(this, wxEVT_SIZE);
        RecalculateBestSize();
        Layout();
        Refresh(false);
        Thaw();
        event.Skip();
        });

    Bind(wxEVT_TIMER, &FlatUIPanel::OnTimer, this);

    RecalculateBestSize();
    
    // Register theme change listener
    auto& themeManager = ThemeManager::getInstance();
    themeManager.addThemeChangeListener(this, [this]() {
        RefreshTheme();
    });
}

void FlatUIPanel::OnTimer(wxTimerEvent& event)
{
    if (event.GetId() == TIMER_RESIZE || event.GetId() == TIMER_ADD_CONTROL) {
        UpdatePanelSize();
    }
}

void FlatUIPanel::UpdatePanelSize()
{
    Freeze();
    wxEventBlocker blocker(this, wxEVT_SIZE);

    bool wasHidden = !IsShown();
    if (wasHidden)
        Show();

    RecalculateBestSize();
    Layout();

    if (wasHidden)
        Hide();

    wxWindow* parent = GetParent();
    if (parent) {
        FlatUIPage* page = dynamic_cast<FlatUIPage*>(parent);
        if (page) {
            page->RecalculatePageHeight();
        }
        parent->Layout();
    }

    Refresh(false);
    Thaw();

    //LOG_INF_S("Updated panel: " + GetLabel().ToStdString() +
    //    ", Size: (" + std::to_string(GetSize().GetWidth()) +
    //    "," + std::to_string(GetSize().GetHeight()) + ")", "FlatUIPanel");
}

void FlatUIPanel::ResizeChildControls(int width, int height)
{
    if (m_sizer) {
        int sizerX = 0;
        int sizerY = 0;
        int sizerWidth = width;
        int sizerHeight = height;

        // Adjust for header placement
        switch (m_headerStyle) {
        case PanelHeaderStyle::TOP:
            sizerY = CFG_INT("PanelDefaultHeaderAreaSize");
            sizerHeight -= CFG_INT("PanelDefaultHeaderAreaSize");
            break;
        case PanelHeaderStyle::LEFT:
            sizerX = CFG_INT("PanelDefaultHeaderAreaSize");
            sizerWidth -= CFG_INT("PanelDefaultHeaderAreaSize");
            break;
        case PanelHeaderStyle::BOTTOM_CENTERED:
            sizerHeight -= CFG_INT("PanelDefaultHeaderAreaSize");
            break;
        case PanelHeaderStyle::EMBEDDED:
            // For embedded style, add top padding to avoid text overlap
            if (!m_label.IsEmpty()) {
                const int embeddedHeaderHeight = 20; // Space for embedded text
                sizerY = embeddedHeaderHeight;
                sizerHeight -= embeddedHeaderHeight;
            }
            break;
        case PanelHeaderStyle::NONE:
            // No adjustment needed
            break;
        }

        if (sizerWidth < 0) sizerWidth = 0;
        if (sizerHeight < 0) sizerHeight = 0;

        m_sizer->SetDimension(sizerX, sizerY, sizerWidth, sizerHeight);
        Layout();
    }

    LOG_DBG("ResizeChildControls called for panel: " + GetLabel().ToStdString() +
        ", Width: " + std::to_string(width) +
        ", Height: " + std::to_string(height), "FlatUIPanel");
}

void FlatUIPanel::RecalculateBestSize()
{
    static bool isRecalculating = false;
    if (isRecalculating)
        return;

    isRecalculating = true;

    Freeze();

    wxSize bestPanelSize(0, 0); // Renamed from bestSize for clarity with TARGET_PANEL_HEIGHT
    int headerOffsetWidth = 0, headerOffsetHeight = 0;
     
    if (m_headerStyle == PanelHeaderStyle::TOP || m_headerStyle == PanelHeaderStyle::BOTTOM_CENTERED) {
        headerOffsetHeight = CFG_INT("PanelDefaultHeaderAreaSize");
    }
    else if (m_headerStyle == PanelHeaderStyle::LEFT) {
        headerOffsetWidth = CFG_INT("PanelDefaultHeaderAreaSize");
    }

    // Calculate actual width and height needed by children
    int childrenTotalWidth = 0;
    int childrenTotalHeight = 0;
    int childControlCount = 0;

    if (m_buttonBars.size() + m_galleries.size() > 0) {
        for (auto buttonBar : m_buttonBars) {
            if (buttonBar) {
                bool wasHidden = !buttonBar->IsShown();
                if (wasHidden) buttonBar->Show();
                wxSize barSize = buttonBar->GetBestSize();
                if (m_orientation == wxHORIZONTAL) {
                    childrenTotalWidth += barSize.GetWidth();
                    childControlCount++;
                    childrenTotalHeight = wxMax(childrenTotalHeight, barSize.GetHeight());
                }
                else {
                    childrenTotalWidth = wxMax(childrenTotalWidth, barSize.GetWidth());
                    childrenTotalHeight += barSize.GetHeight();
                }
                if (wasHidden) buttonBar->Hide();
            }
        }

        for (auto gallery : m_galleries) {
            if (gallery) {
                bool wasHidden = !gallery->IsShown();
                if (wasHidden) gallery->Show();
                wxSize gallerySize = gallery->GetBestSize();
                if (m_orientation == wxHORIZONTAL) {
                    childrenTotalWidth += gallerySize.GetWidth();
                    childControlCount++;
                    childrenTotalHeight = wxMax(childrenTotalHeight, gallerySize.GetHeight());
                }
                else {
                    childrenTotalWidth = wxMax(childrenTotalWidth, gallerySize.GetWidth());
                    childrenTotalHeight += gallerySize.GetHeight();
                }
                if (wasHidden) gallery->Hide();
            }
        }

        if (m_orientation == wxHORIZONTAL && childControlCount > 1) {
            childrenTotalWidth += (childControlCount - 1) * CFG_INT("PanelMargin");
        }
    }
    else {
        // Default minimum size if no children, to ensure panel is visible
        // Values based on original implicit empty panel size, adjust if needed
        childrenTotalWidth = GetMinSize().GetWidth() > 0
            ? GetMinSize().GetWidth() - CFG_INT("PanelInternalPaddingTotal") - headerOffsetWidth
            : CFG_INT("PanelMargin");
        // childrenTotalHeight calculation for empty panel needs to result in TARGET_PANEL_HEIGHT
        // This part is tricky if we want an empty panel to also be TARGET_PANEL_HEIGHT.
        // Let's assume childrenTotalHeight is 0 if no children, 
        // and TARGET_PANEL_HEIGHT will handle the overall panel height.
        childrenTotalHeight = 0;
    }

    // Panel's best width is based on children content + padding + header + sizer borders
    // Each control has left and right borders (2*2=4 pixels per control)
    int sizerBorderWidth = 0;
    if (childControlCount > 0) {
        sizerBorderWidth = childControlCount * 4; // 2 pixels left + 2 pixels right per control
    }
    bestPanelSize.SetWidth(childrenTotalWidth + CFG_INT("PanelInternalPaddingTotal") + headerOffsetWidth + sizerBorderWidth);

    // Panel's best height should be based on actual content + header + padding
    // Use the larger of: calculated height or minimum target height
    int calculatedHeight = childrenTotalHeight + headerOffsetHeight + CFG_INT("PanelInternalVerticalPadding");
    int minHeight = (childrenTotalHeight > 0) ? calculatedHeight + 4 : CFG_INT("PanelTargetHeight");
    bestPanelSize.SetHeight(wxMax(calculatedHeight, minHeight));

    int targetHeight = CFG_INT("PanelTargetHeight");
    int finalHeight = wxMax(calculatedHeight, targetHeight);

    // Add debug logging 
    // LOG_INF_S("Panel " + GetLabel().ToStdString() +
    //     " - Children: " + std::to_string(childrenTotalHeight) +
    //     ", Header: " + std::to_string(headerOffsetHeight) +
    //     ", Padding: " + std::to_string(CFG_INT("PanelInternalVerticalPadding")) +
    //     ", Calculated: " + std::to_string(calculatedHeight) +
    //     ", Target: " + std::to_string(targetHeight) +
    //     ", Final: " + std::to_string(finalHeight), "FlatUIPanel");

    SetMinSize(bestPanelSize);
    SetSize(bestPanelSize); // Keep this to enforce size, as in original

    if (m_sizer) {
        int sizerAreaX = 0;
        int sizerAreaY = 0;
        int sizerAreaWidth = bestPanelSize.GetWidth();
        int sizerAreaHeight = bestPanelSize.GetHeight();  // Use actual panel height, not fixed target

        // Adjust sizer area based on header style
        switch (m_headerStyle) {
        case PanelHeaderStyle::TOP:
            sizerAreaY = CFG_INT("PanelDefaultHeaderAreaSize");
            sizerAreaHeight -= CFG_INT("PanelDefaultHeaderAreaSize");
            break;
        case PanelHeaderStyle::LEFT:
            sizerAreaX = CFG_INT("PanelDefaultHeaderAreaSize");
            sizerAreaWidth -= CFG_INT("PanelDefaultHeaderAreaSize");
            break;
        case PanelHeaderStyle::BOTTOM_CENTERED:
            sizerAreaHeight -= CFG_INT("PanelDefaultHeaderAreaSize");
            break;
        case PanelHeaderStyle::EMBEDDED:
            // For embedded style, add top padding to avoid text overlap
            if (!m_label.IsEmpty()) {
                const int embeddedHeaderHeight = 20; // Space for embedded text
                sizerAreaY = embeddedHeaderHeight;
                sizerAreaHeight -= embeddedHeaderHeight;
            }
            break;
        case PanelHeaderStyle::NONE:
            // No adjustment needed
            break;
        }

        if (sizerAreaWidth < 0) sizerAreaWidth = 0;
        if (sizerAreaHeight < 0) sizerAreaHeight = 0;

        m_sizer->SetDimension(sizerAreaX, sizerAreaY, sizerAreaWidth, sizerAreaHeight);
    }

    //LOG_INF_S("RecalculateBestSize for panel: " + GetLabel().ToStdString() + " Best Size: (" + std::to_string(bestPanelSize.GetWidth()) + ", " +  std::to_string(bestPanelSize.GetHeight()) + ")", "FlatUIPanel");

    Thaw();
    isRecalculating = false;
}

FlatUIPanel::~FlatUIPanel()
{
    for (auto buttonBar : m_buttonBars)
        delete buttonBar;
    for (auto gallery : m_galleries)
        delete gallery;
}

void FlatUIPanel::OnThemeChanged()
{
    // Use batch update to avoid multiple refreshes
    BatchUpdateTheme();
}

void FlatUIPanel::UpdateThemeValues()
{
    // Update all theme-based colors and settings
    m_bgColour = GetThemeColour("ActBarBackgroundColour");
    m_borderColour = GetThemeColour("PanelBorderColour");
    m_headerColour = GetThemeColour("PanelHeaderColour");
    m_headerTextColour = GetThemeColour("PanelHeaderTextColour");
    m_headerBorderColour = GetThemeColour("PanelBorderColour");
    
    // Update control properties
    SetFont(GetThemeFont());
    SetBackgroundColour(m_bgColour);
    
    // Update child controls without immediate refresh
    wxWindowList& children = GetChildren();
    for (wxWindow* child : children) {
        // Check if child has RefreshTheme method
        wxString className = child->GetClassInfo()->GetClassName();
        if (className == wxT("FlatUIButtonBar")) {
            FlatUIButtonBar* buttonBar = static_cast<FlatUIButtonBar*>(child);
            buttonBar->UpdateThemeValues();
        }
        else if (className == wxT("FlatUIGallery")) {
            // FlatUIGallery will be handled separately if needed
            // Don't refresh here - let parent frame handle it
        }
        else {
            // For other controls, just update properties without refresh
            // Don't refresh here - let parent frame handle it
        }
    }
    
    // Note: Don't call Refresh() here - it will be handled by the parent frame
}

void FlatUIPanel::SetPanelBackgroundColour(const wxColour& colour)
{
    m_bgColour = colour;
    Refresh();
}

void FlatUIPanel::SetBorderStyle(PanelBorderStyle style)
{
    m_borderStyle = style;

    if (style == PanelBorderStyle::NONE) {
        m_panelBorderTop = 0;
        m_panelBorderBottom = 0;
        m_panelBorderLeft = 0;
        m_panelBorderRight = 0;
    }
    else if (m_panelBorderTop == 0 && m_panelBorderBottom == 0 &&
        m_panelBorderLeft == 0 && m_panelBorderRight == 0) {
        switch (style) {
        case PanelBorderStyle::THIN:
            m_panelBorderTop = 1;
            m_panelBorderBottom = 1;
            m_panelBorderLeft = 1;
            m_panelBorderRight = 1;
            break;
        case PanelBorderStyle::MEDIUM:
            m_panelBorderTop = 2;
            m_panelBorderBottom = 2;
            m_panelBorderLeft = 2;
            m_panelBorderRight = 2;
            break;
        case PanelBorderStyle::THICK:
            m_panelBorderTop = 3;
            m_panelBorderBottom = 3;
            m_panelBorderLeft = 3;
            m_panelBorderRight = 3;
            break;
        case PanelBorderStyle::ROUNDED:
            break;
        default:
            break;
        }
    }

    Refresh();
}

void FlatUIPanel::SetBorderColour(const wxColour& colour)
{
    m_borderColour = colour;
    Refresh();
}

void FlatUIPanel::SetPanelBorderWidths(int top, int bottom, int left, int right)
{
    m_panelBorderTop = top;
    m_panelBorderBottom = bottom;
    m_panelBorderLeft = left;
    m_panelBorderRight = right;
    Refresh();
}

void FlatUIPanel::GetPanelBorderWidths(int& top, int& bottom, int& left, int& right) const
{
    top = m_panelBorderTop;
    bottom = m_panelBorderBottom;
    left = m_panelBorderLeft;
    right = m_panelBorderRight;
}

void FlatUIPanel::SetHeaderStyle(PanelHeaderStyle style)
{
    m_headerStyle = style;
    UpdatePanelSize();
}

void FlatUIPanel::SetHeaderColour(const wxColour& colour)
{
    m_headerColour = colour;
    Refresh();
}

void FlatUIPanel::SetHeaderTextColour(const wxColour& colour)
{
    m_headerTextColour = colour;
    Refresh();
}

void FlatUIPanel::SetHeaderBorderWidths(int top, int bottom, int left, int right)
{
    m_headerBorderTop = top;
    m_headerBorderBottom = bottom;
    m_headerBorderLeft = left;
    m_headerBorderRight = right;
    Refresh();
}

void FlatUIPanel::GetHeaderBorderWidths(int& top, int& bottom, int& left, int& right) const
{
    top = m_headerBorderTop;
    bottom = m_headerBorderBottom;
    left = m_headerBorderLeft;
    right = m_headerBorderRight;
}

void FlatUIPanel::SetHeaderBorderColour(const wxColour& colour)
{
    m_headerBorderColour = colour;
    Refresh();
}

void FlatUIPanel::SetLabel(const wxString& label)
{
    m_label = label;
    Refresh();
}

void FlatUIPanel::AddButtonBar(FlatUIButtonBar* buttonBar, int proportion, int flag, int border)
{
    Freeze();
    m_buttonBars.push_back(buttonBar);
    LOG_INF("Added ButtonBar to panel: " + GetLabel().ToStdString(), "FlatUIPanel");
    FlatUIEventManager::getInstance().bindButtonBarEvents(buttonBar);

    flag = wxLEFT | wxRIGHT | wxTOP | wxALIGN_TOP;
    border = 2; // 4-pixel margin
    proportion = 0;

    if (m_sizer) {
        m_sizer->Add(buttonBar, proportion, flag, border);
        UpdatePanelSize();
    }

    buttonBar->Show();
    Thaw();
}

void FlatUIPanel::AddGallery(FlatUIGallery* gallery, int proportion, int flag, int border)
{
    Freeze();
    m_galleries.push_back(gallery);
    LOG_INF("Added Gallery to panel: " + GetLabel().ToStdString() +
        ", Size: " + std::to_string(gallery->GetSize().GetWidth()) +
        "x" + std::to_string(gallery->GetSize().GetHeight()), "FlatUIPanel");
    FlatUIEventManager::getInstance().bindGalleryEvents(gallery); 

    gallery->SetMinSize(gallery->GetBestSize());
    flag = wxLEFT | wxRIGHT | wxTOP | wxALIGN_TOP;
    border = 2; // 4-pixel margin
    proportion = 0;

    if (m_sizer) { 
        m_sizer->Add(gallery, proportion, flag, border);
        UpdatePanelSize();
    }

    gallery->Show();
    Thaw();
}

void FlatUIPanel::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize size = GetSize();

    dc.SetBackground(m_bgColour);
    dc.Clear();

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) {
        LOG_ERR("Failed to create wxGraphicsContext in FlatUIPanel::OnPaint", "FlatUIPanel");
        evt.Skip();
        return;
    }

    //LOG_DBG("Panel: " + m_label.ToStdString() +
    //    " BorderStyle: " + std::to_string((int)m_borderStyle) +
    //    " BorderWidths: " + std::to_string(m_panelBorderTop) + "," +
    //    std::to_string(m_panelBorderBottom) + "," +
    //    std::to_string(m_panelBorderLeft) + "," +
    //    std::to_string(m_panelBorderRight), "FlatUIPanel");

    if (m_borderStyle != PanelBorderStyle::NONE) {
        if (m_borderStyle == PanelBorderStyle::ROUNDED) {
            // For rounded style, use the maximum border width
            int maxBorderWidth = wxMax(wxMax(m_panelBorderTop, m_panelBorderBottom),
                wxMax(m_panelBorderLeft, m_panelBorderRight));
            if (maxBorderWidth > 0) {
                gc->SetPen(wxPen(m_borderColour, maxBorderWidth));
                gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
                wxGraphicsPath path = gc->CreatePath();
                path.AddRoundedRectangle(maxBorderWidth / 2.0, maxBorderWidth / 2.0,
                    size.GetWidth() - maxBorderWidth,
                    size.GetHeight() - maxBorderWidth, 5.0);
                gc->StrokePath(path);
            }
        }
        else {
            // Draw individual borders based on their widths
            if (m_panelBorderTop > 0) {
                gc->SetPen(wxPen(m_borderColour, m_panelBorderTop));
                gc->StrokeLine(0, m_panelBorderTop / 2.0, size.GetWidth(), m_panelBorderTop / 2.0);
            }
            if (m_panelBorderRight > 0) {
                gc->SetPen(wxPen(m_borderColour, m_panelBorderRight));
                gc->StrokeLine(size.GetWidth() - m_panelBorderRight / 2.0, 0,
                    size.GetWidth() - m_panelBorderRight / 2.0, size.GetHeight());
            }
            if (m_panelBorderBottom > 0) {
                gc->SetPen(wxPen(m_borderColour, m_panelBorderBottom));
                gc->StrokeLine(0, size.GetHeight() - m_panelBorderBottom / 2.0,
                    size.GetWidth(), size.GetHeight() - m_panelBorderBottom / 2.0);
            }
            if (m_panelBorderLeft > 0) {
                gc->SetPen(wxPen(m_borderColour, m_panelBorderLeft));
                gc->StrokeLine(m_panelBorderLeft / 2.0, 0, m_panelBorderLeft / 2.0, size.GetHeight());
            }
        }
    }
    else {
        if (m_panelBorderTop > 0) {
            gc->SetPen(wxPen(m_borderColour, m_panelBorderTop));
            gc->StrokeLine(0, m_panelBorderTop / 2.0, size.GetWidth(), m_panelBorderTop / 2.0);
        }
        if (m_panelBorderRight > 0) {
            gc->SetPen(wxPen(m_borderColour, m_panelBorderRight));
            gc->StrokeLine(size.GetWidth() - m_panelBorderRight / 2.0, 0,
                size.GetWidth() - m_panelBorderRight / 2.0, size.GetHeight());
        }
        if (m_panelBorderBottom > 0) {
            gc->SetPen(wxPen(m_borderColour, m_panelBorderBottom));
            gc->StrokeLine(0, size.GetHeight() - m_panelBorderBottom / 2.0,
                size.GetWidth(), size.GetHeight() - m_panelBorderBottom / 2.0);
        }
        if (m_panelBorderLeft > 0) {
            gc->SetPen(wxPen(m_borderColour, m_panelBorderLeft));
            gc->StrokeLine(m_panelBorderLeft / 2.0, 0, m_panelBorderLeft / 2.0, size.GetHeight());
        }
    }

    if (m_headerStyle != PanelHeaderStyle::NONE && !m_label.IsEmpty()) {
        gc->SetFont(CFG_DEFAULTFONT(), m_headerTextColour);
        wxDouble textWidth, textHeight;
        gc->GetTextExtent(m_label, &textWidth, &textHeight);

        switch (m_headerStyle) {
        case PanelHeaderStyle::TOP:
            gc->SetBrush(wxBrush(m_headerColour));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(0, 0, size.GetWidth(), CFG_INT("PanelDefaultHeaderAreaSize"));
            gc->DrawText(m_label, 0, (CFG_INT("PanelDefaultHeaderAreaSize") - textHeight) / 2);

            // Draw header borders
            gc->SetPen(wxPen(m_headerBorderColour, 1));
            if (m_headerBorderTop > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderTop));
                gc->StrokeLine(0, 0, size.GetWidth(), 0);
            }
            if (m_headerBorderBottom > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderBottom));
                gc->StrokeLine(0, CFG_INT("PanelDefaultHeaderAreaSize"), size.GetWidth(), CFG_INT("PanelDefaultHeaderAreaSize"));
            }
            if (m_headerBorderLeft > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderLeft));
                gc->StrokeLine(0, 0, 0, CFG_INT("PanelDefaultHeaderAreaSize"));
            }
            if (m_headerBorderRight > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderRight));
                gc->StrokeLine(size.GetWidth(), 0, size.GetWidth(), CFG_INT("PanelDefaultHeaderAreaSize"));
            }
            break;
        case PanelHeaderStyle::LEFT:
            gc->SetBrush(wxBrush(m_headerColour));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(0, 0, CFG_INT("PanelDefaultHeaderAreaSize"), size.GetHeight());
            gc->PushState();
            gc->Translate(CFG_INT("PanelDefaultHeaderAreaSize") / 2, size.GetHeight() / 2);
            gc->Rotate(-M_PI / 2);
            gc->DrawText(m_label, -textWidth / 2, -textHeight / 2);
            gc->PopState();

            // Draw header borders
            if (m_headerBorderTop > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderTop));
                gc->StrokeLine(0, 0, CFG_INT("PanelDefaultHeaderAreaSize"), 0);
            }
            if (m_headerBorderBottom > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderBottom));
                gc->StrokeLine(0, size.GetHeight(), CFG_INT("PanelDefaultHeaderAreaSize"), size.GetHeight());
            }
            if (m_headerBorderLeft > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderLeft));
                gc->StrokeLine(0, 0, 0, size.GetHeight());
            }
            if (m_headerBorderRight > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderRight));
                gc->StrokeLine(CFG_INT("PanelDefaultHeaderAreaSize"), 0, CFG_INT("PanelDefaultHeaderAreaSize"), size.GetHeight());
            }
            break;
        case PanelHeaderStyle::EMBEDDED:
            // Draw text with padding to avoid overlapping with child controls
        {
            const int textPadding = 5;
            gc->DrawText(m_label, textPadding, textPadding);

            // For embedded style, draw borders around the text area if specified
            if (m_headerBorderTop > 0 || m_headerBorderBottom > 0 || m_headerBorderLeft > 0 || m_headerBorderRight > 0) {
                const int embeddedHeaderHeight = 20;
                if (m_headerBorderTop > 0) {
                    gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderTop));
                    gc->StrokeLine(0, 0, size.GetWidth(), 0);
                }
                if (m_headerBorderBottom > 0) {
                    gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderBottom));
                    gc->StrokeLine(0, embeddedHeaderHeight, size.GetWidth(), embeddedHeaderHeight);
                }
                if (m_headerBorderLeft > 0) {
                    gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderLeft));
                    gc->StrokeLine(0, 0, 0, embeddedHeaderHeight);
                }
                if (m_headerBorderRight > 0) {
                    gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderRight));
                    gc->StrokeLine(size.GetWidth(), 0, size.GetWidth(), embeddedHeaderHeight);
                }
            }
        }
        break;
        case PanelHeaderStyle::BOTTOM_CENTERED:
            gc->SetBrush(wxBrush(m_headerColour));
            gc->SetPen(*wxTRANSPARENT_PEN);
            // Draw header bar at the bottom
            gc->DrawRectangle(0, size.GetHeight() - CFG_INT("PanelDefaultHeaderAreaSize"), size.GetWidth(), CFG_INT("PanelDefaultHeaderAreaSize"));

            // Draw text centered in the bottom header bar
            gc->DrawText(m_label,
                (size.GetWidth() - textWidth) / 2,
                size.GetHeight() - CFG_INT("PanelDefaultHeaderAreaSize") + (CFG_INT("PanelDefaultHeaderAreaSize") - textHeight) / 2);

            // Draw header borders
            // LOG_INF_S("Drawing bottom header borders for panel: " + m_headerBorderColour.GetAsString().ToStdString(), "FlatUIPanel");
            if (m_headerBorderTop > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderTop));
                gc->StrokeLine(0,
                    size.GetHeight() - CFG_INT("PanelDefaultHeaderAreaSize"),
                    size.GetWidth(), 
                    size.GetHeight() - CFG_INT("PanelDefaultHeaderAreaSize"));
            }
            if (m_headerBorderBottom > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderBottom));
                gc->StrokeLine(CFG_INT("PanelInnerBarBorderSpacing"),
                    size.GetHeight(),
                    size.GetWidth() - CFG_INT("PanelInnerBarBorderSpacing"), size.GetHeight());
            }
            if (m_headerBorderLeft > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderLeft));
                gc->StrokeLine(0,
                    size.GetHeight() - CFG_INT("PanelDefaultHeaderAreaSize") - CFG_INT("PanelInnerBarBorderSpacing"),
                    0,
                    size.GetHeight() - CFG_INT("PanelInnerBarBorderSpacing"));
            }
            if (m_headerBorderRight > 0) {
                gc->SetPen(wxPen(m_headerBorderColour, m_headerBorderRight));
                gc->StrokeLine(size.GetWidth(),
                    size.GetHeight() - CFG_INT("PanelDefaultHeaderAreaSize") - CFG_INT("PanelInnerBarBorderSpacing"),
                    size.GetWidth(),
                    size.GetHeight() - CFG_INT("PanelInnerBarBorderSpacing"));
            }
            break;
        }
    }

    delete gc;
    evt.Skip();
}

void FlatUIPanel::RefreshTheme() {
    // Update all theme-based colors and settings
    m_bgColour = GetThemeColour("ActBarBackgroundColour");
    m_borderColour = GetThemeColour("PanelBorderColour");
    m_headerColour = GetThemeColour("PanelHeaderColour");
    m_headerTextColour = GetThemeColour("PanelHeaderTextColour");
    m_headerBorderColour = GetThemeColour("PanelBorderColour");
    
    // Update control properties
    SetFont(GetThemeFont());
    SetBackgroundColour(CFG_COLOUR("PanelBgColour"));
    
    // Update child controls
    for (auto buttonBar : m_buttonBars) {
        if (buttonBar) {
            buttonBar->RefreshTheme();
        }
    }
    
    for (auto gallery : m_galleries) {
        if (gallery) {
            gallery->RefreshTheme();
        }
    }
    
    // Force refresh
    Refresh(true);
    Update();
}
