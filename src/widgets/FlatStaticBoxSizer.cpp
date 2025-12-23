#include "widgets/FlatStaticBoxSizer.h"
#include "config/ThemeManager.h"
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

wxBEGIN_EVENT_TABLE(FlatStaticBox, wxStaticBox)
EVT_PAINT(FlatStaticBox::OnPaint)
wxEND_EVENT_TABLE()

FlatStaticBox::FlatStaticBox(wxWindow* parent, wxWindowID id, const wxString& label,
                             const wxPoint& pos, const wxSize& size, long style,
                             const wxString& name)
    : wxStaticBox(parent, id, label, pos, size, style, name)
    , m_borderWidth(1)
    , m_cornerRadius(4)
{
    InitializeThemeColors();

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});

    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        OnThemeChanged();
    });
}

FlatStaticBox::~FlatStaticBox()
{
    ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FlatStaticBox::InitializeThemeColors()
{
    m_borderColor = CFG_COLOUR("ButtonBorderColour");
    m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
    m_textColor = CFG_COLOUR("PrimaryTextColour");

    SetBackgroundColour(m_backgroundColor);
    SetForegroundColour(m_textColor);
}

void FlatStaticBox::OnThemeChanged()
{
    InitializeThemeColors();
    Refresh();
}

void FlatStaticBox::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    
    wxRect rect = GetClientRect();
    
    wxColour bgColor = GetBackgroundColour();
    if (!bgColor.IsOk())
    {
        bgColor = m_backgroundColor;
    }

    dc.SetBackground(wxBrush(bgColor));
    dc.Clear();

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        if (m_borderWidth > 0)
        {
            wxPen borderPen(m_borderColor, m_borderWidth);
            gc->SetPen(borderPen);
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            
            int halfBorder = m_borderWidth / 2;
            if (m_cornerRadius > 0)
            {
                gc->DrawRoundedRectangle(rect.x + halfBorder, rect.y + halfBorder, 
                                         rect.width - m_borderWidth, rect.height - m_borderWidth, 
                                         m_cornerRadius);
            }
            else
            {
                gc->DrawRectangle(rect.x + halfBorder, rect.y + halfBorder, 
                                  rect.width - m_borderWidth, rect.height - m_borderWidth);
            }
        }

        delete gc;
    }
    else
    {
        DrawFlatBorder(dc, rect);
    }

    event.Skip();
}

void FlatStaticBox::DrawFlatBorder(wxDC& dc, const wxRect& rect)
{
    wxColour bgColor = GetBackgroundColour();
    if (!bgColor.IsOk())
    {
        bgColor = m_backgroundColor;
    }

    dc.SetBackground(wxBrush(bgColor));
    dc.Clear();

    if (m_borderWidth > 0)
    {
        dc.SetPen(wxPen(m_borderColor, m_borderWidth));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
    }
}

FlatStaticBoxSizer::FlatStaticBoxSizer(int orient, wxWindow* parent, const wxString& label)
    : wxStaticBoxSizer(CreateStaticBox(parent, label), orient)
    , m_staticBox(nullptr)
{
    m_staticBox = static_cast<FlatStaticBox*>(wxStaticBoxSizer::GetStaticBox());
}

FlatStaticBox* FlatStaticBoxSizer::CreateStaticBox(wxWindow* parent, const wxString& label)
{
    return new FlatStaticBox(parent, wxID_ANY, label);
}

FlatStaticBoxSizer::FlatStaticBoxSizer(FlatStaticBox* box, int orient)
    : wxStaticBoxSizer(box, orient)
    , m_staticBox(box)
{
}

FlatStaticBoxSizer::~FlatStaticBoxSizer()
{
}

