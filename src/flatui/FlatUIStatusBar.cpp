#include "flatui/FlatUIStatusBar.h"

wxBEGIN_EVENT_TABLE(FlatUIStatusBar, FlatUIThemeAware)
    EVT_PAINT(FlatUIStatusBar::OnPaint)
wxEND_EVENT_TABLE()

FlatUIStatusBar::FlatUIStatusBar(wxWindow* parent, wxWindowID id)
    : FlatUIThemeAware(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
    SetFieldsCount(1);
    SetMinSize(wxSize(-1, 24)); 
    UpdateThemeValues();
}

void FlatUIStatusBar::SetFieldsCount(int count) {
    m_fields.resize(count);
    m_fieldWidths.resize(count, -1); 
    Refresh();
}

void FlatUIStatusBar::SetStatusText(const wxString& text, int field) {
    if (field >= 0 && field < (int)m_fields.size()) {
        m_fields[field] = text;
        Refresh();
    }
}

void FlatUIStatusBar::SetFieldText(const wxString& text, int field) {
    SetStatusText(text, field);
}

void FlatUIStatusBar::OnThemeChanged() {
    UpdateThemeValues();
    Refresh();
}

void FlatUIStatusBar::UpdateThemeValues() {
    m_bgColour = GetThemeColour("StatusBarBgColour"); 
    m_textColour = GetThemeColour("StatusBarTextColour");
    m_borderColour = GetThemeColour("StatusBarBorderColour");
    m_font = GetThemeFont();
}
 
void FlatUIStatusBar::OnPaint(wxPaintEvent& evt) {
    wxPaintDC dc(this);
    dc.SetBackground(wxBrush(m_bgColour));
    dc.Clear();
    dc.SetFont(m_font);
    dc.SetTextForeground(m_textColour);

    int w, h;
    GetClientSize(&w, &h);

    int fieldCount = m_fields.size();
    int x = 4;
    int fieldWidth = w / (fieldCount > 0 ? fieldCount : 1);

    for (int i = 0; i < fieldCount; ++i) {
        wxRect rect(x, 0, fieldWidth, h);
        dc.DrawText(m_fields[i], rect.x + 4, rect.y + (h - dc.GetCharHeight()) / 2);
        x += fieldWidth;
        if (i < fieldCount - 1) {
            dc.SetPen(wxPen(m_borderColour));
            dc.DrawLine(x - 1, 2, x - 1, h - 2);
        }
    }
} 