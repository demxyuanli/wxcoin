#ifndef FLAT_STATIC_BOX_SIZER_H
#define FLAT_STATIC_BOX_SIZER_H

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

class FlatStaticBox : public wxStaticBox
{
public:
    FlatStaticBox(wxWindow* parent, wxWindowID id, const wxString& label,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0,
                  const wxString& name = wxStaticBoxNameStr);

    virtual ~FlatStaticBox();

protected:
    void OnPaint(wxPaintEvent& event);
    void OnThemeChanged();
    void InitializeThemeColors();
    void DrawFlatBorder(wxDC& dc, const wxRect& rect);

private:
    wxColour m_borderColor;
    wxColour m_backgroundColor;
    wxColour m_textColor;
    int m_borderWidth;
    int m_cornerRadius;

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(FlatStaticBox);
};

class FlatStaticBoxSizer : public wxStaticBoxSizer
{
public:
    FlatStaticBoxSizer(int orient, wxWindow* parent, const wxString& label = wxEmptyString);
    FlatStaticBoxSizer(FlatStaticBox* box, int orient);
    virtual ~FlatStaticBoxSizer();

    FlatStaticBox* GetStaticBox() const { return m_staticBox; }

private:
    static FlatStaticBox* CreateStaticBox(wxWindow* parent, const wxString& label);
    FlatStaticBox* m_staticBox;
};

#endif // FLAT_STATIC_BOX_SIZER_H

