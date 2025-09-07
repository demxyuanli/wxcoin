#pragma once

#include "flatui/BorderlessFrameLogic.h"
#include "flatui/FlatUISystemButtons.h"
#include <wx/wx.h>
#include <memory>

namespace ads {

/**
 * @brief Custom frameless window for floating dock containers
 * Extends BorderlessFrameLogic with custom title bar and system buttons
 */
class FloatingDockFrame : public BorderlessFrameLogic
{
public:
    FloatingDockFrame(wxWindow* parent, wxWindowID id, const wxString& title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxSize(400, 300));

    virtual ~FloatingDockFrame();

    // Title bar management
    void SetTitle(const wxString& title);
    wxString GetTitle() const { return m_titleText; }

    // System buttons
    void ShowSystemButtons(bool show);
    bool HasSystemButtons() const { return m_showSystemButtons; }

    // Content area
    wxWindow* GetContentArea() const { return m_contentArea; }
    void SetContentArea(wxWindow* content);

    // Custom title bar height
    static const int TITLE_BAR_HEIGHT = 30;

protected:
    // Override paint to draw custom title bar
    virtual void OnPaint(wxPaintEvent& event);

    // Override mouse events for title bar interaction
    virtual void OnLeftDown(wxMouseEvent& event) override;
    virtual void OnLeftUp(wxMouseEvent& event) override;
    virtual void OnMotion(wxMouseEvent& event) override;

    // System button event handlers
    void OnSystemButtonMinimize(wxCommandEvent& event);
    void OnSystemButtonMaximize(wxCommandEvent& event);
    void OnSystemButtonClose(wxCommandEvent& event);
    void OnSystemButtonMouseDown(wxMouseEvent& event);

    // Helper methods
    void UpdateTitleBarLayout();
    bool IsPointInTitleBar(const wxPoint& pos) const;
    wxRect GetTitleBarRect() const;

private:
    wxString m_titleText;
    bool m_showSystemButtons;
    wxWindow* m_contentArea;

    // Custom title bar components
    wxPanel* m_titleBarPanel;
    wxStaticText* m_titleLabel;
    FlatUISystemButtons* m_systemButtons;

    wxDECLARE_EVENT_TABLE();
};

} // namespace ads
