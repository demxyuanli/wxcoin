#ifndef FRAMELESS_MODAL_POPUP_H
#define FRAMELESS_MODAL_POPUP_H

#include <wx/dialog.h>
#include <wx/graphics.h>
#include <wx/bitmap.h>
#include <wx/colour.h>

class FramelessModalPopup : public wxDialog
{
public:
    FramelessModalPopup(wxWindow* parent,
                       const wxString& title = wxEmptyString,
                       const wxSize& size = wxDefaultSize);
    virtual ~FramelessModalPopup();

    // Title bar methods
    void SetTitle(const wxString& title);
    void SetTitleIcon(const wxBitmap& icon);
    void SetTitleIcon(const wxString& svgIconName, const wxSize& size = wxSize(16, 16));
    void ShowTitleIcon(bool show = true);

    // Override to handle custom painting and events
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

    // Close button handling
    void OnCloseButton(wxCommandEvent& event);
    void OnThemeChanged();

    // Modal behavior
    virtual int ShowModal() override;

protected:
    virtual void DoSetSize(int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO) override;

private:
    void CreateControls();
    void LayoutControls();
    void UpdateLayout();
    void LoadThemeConfiguration();
    wxRect GetCloseButtonRect() const;
    bool IsCloseButtonHit(const wxPoint& pos) const;
    void DrawBorder(wxDC& dc);
    void DrawTitleBarContent(wxGraphicsContext* gc);

    // UI elements
protected:
    wxWindow* m_contentPanel;
    wxBitmap m_closeIcon;

    // Layout constants (loaded from theme)
    int CLOSE_BUTTON_SIZE;
    int CLOSE_BUTTON_MARGIN;
    int BORDER_WIDTH;
    int TITLE_BAR_HEIGHT;

    // Title bar elements
    wxString m_titleText;
    wxBitmap m_titleIcon;
    wxString m_titleIconName; // For SVG icons that need theme updates
    wxSize m_titleIconSize;
    bool m_showTitleIcon;

    // State tracking
    bool m_closeButtonHovered;
    bool m_closeButtonPressed;
    bool m_dragging;
    wxPoint m_dragStartPos;
    wxRect m_parentWindowRect;

    wxDECLARE_EVENT_TABLE();
};

#endif // FRAMELESS_MODAL_POPUP_H
