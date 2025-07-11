#ifndef CUSTOMDROPDOWN_H
#define CUSTOMDROPDOWN_H

#include <wx/wx.h>
#include <wx/popupwin.h>
#include <wx/listbox.h>
#include <wx/timer.h>
#include <wx/display.h>
#include <vector>

class CustomDropDownPopup;

// Dropdown display styles
enum class CustomDropDownStyle
{
    Icon_Text_Dropdown,    // Left icon + text + right dropdown arrow
    Text_Dropdown,             // Text + right dropdown arrow (default)
    DropdownOnly                    // Only dropdown arrow button
};

class CustomDropDown : public wxControl
{
public:
    CustomDropDown(wxWindow* parent, 
                   wxWindowID id = wxID_ANY,
                   const wxString& value = wxEmptyString,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0);
    
    virtual ~CustomDropDown();

    // Item management
    void Clear();
    void Append(const wxString& item);
    void Insert(const wxString& item, unsigned int pos);
    void Delete(unsigned int n);
    unsigned int GetCount() const;
    wxString GetString(unsigned int n) const;
    void SetString(unsigned int n, const wxString& s);

    // Selection
    void SetSelection(int n);
    int GetSelection() const;
    wxString GetStringSelection() const;
    bool SetStringSelection(const wxString& s);

    // Value
    void SetValue(const wxString& value);
    wxString GetValue() const;

    // Appearance customization
    bool SetBackgroundColour(const wxColour& colour) override;
    bool SetForegroundColour(const wxColour& colour) override;
    void SetBorderColour(const wxColour& colour);
    void SetBorderWidth(int width);
    void SetDropDownButtonColour(const wxColour& colour);
    void SetDropDownButtonHoverColour(const wxColour& colour);
    void SetPopupBackgroundColour(const wxColour& colour);
    void SetPopupBorderColour(const wxColour& colour);
    void SetSelectionBackgroundColour(const wxColour& colour);
    void SetSelectionForegroundColour(const wxColour& colour);
    
    // Style control
    void SetDropDownStyle(CustomDropDownStyle style);
    CustomDropDownStyle GetDropDownStyle() const;
    void SetLeftIcon(const wxString& iconName);
    wxString GetLeftIcon() const;
    void SetLeftIconSize(const wxSize& size);
    wxSize GetLeftIconSize() const;

    // Dropdown control
    void ShowDropDown();
    void HideDropDown();
    bool IsDropDownShown() const;

    // Size management
    wxSize DoGetBestSize() const override;
    void SetMinDropDownWidth(int width);
    void SetMaxDropDownHeight(int height);

protected:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    // Drawing methods
    void DrawBackground(wxDC& dc);
    void DrawBorder(wxDC& dc);
    void DrawText(wxDC& dc);
    void DrawDropDownButton(wxDC& dc);
    void DrawDropDownIcon(wxDC& dc, const wxRect& buttonRect);
    void DrawLeftIcon(wxDC& dc);
    wxRect GetDropDownButtonRect() const;
    wxRect GetTextRect() const;
    wxRect GetLeftIconRect() const;

    // Popup management
    void CreatePopup();
    void DestroyPopup();
    void PositionPopup();

public:
    void OnPopupSelection(int selection);
    void OnPopupDismiss();

protected:

private:
    // Items
    std::vector<wxString> m_items;
    int m_selection;
    wxString m_value;

    // Appearance
    wxColour m_borderColour;
    wxColour m_dropDownButtonColour;
    wxColour m_dropDownButtonHoverColour;
    wxColour m_popupBackgroundColour;
    wxColour m_popupBorderColour;
    wxColour m_selectionBackgroundColour;
    wxColour m_selectionForegroundColour;
    int m_borderWidth;
    
    // Style settings
    CustomDropDownStyle m_style;
    wxString m_leftIconName;
    wxSize m_leftIconSize;

    // State
    bool m_isDropDownShown;
    bool m_isButtonHovered;
    bool m_isButtonPressed;

    // Popup
    CustomDropDownPopup* m_popup;
    int m_minDropDownWidth;
    int m_maxDropDownHeight;

    // Constants
    static const int DEFAULT_BUTTON_WIDTH = 16;
    static const int DEFAULT_BORDER_WIDTH = 1;
    static const int DEFAULT_TEXT_MARGIN = 4;

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(CustomDropDown);
};

// Popup window class
class CustomDropDownPopup : public wxPopupWindow
{
public:
    CustomDropDownPopup(CustomDropDown* parent);
    virtual ~CustomDropDownPopup();

    void SetItems(const std::vector<wxString>& items);
    void SetSelection(int selection);
    int GetSelection() const;
    
    bool SetBackgroundColour(const wxColour& colour);
    void SetBorderColour(const wxColour& colour);
    void SetSelectionBackgroundColour(const wxColour& colour);
    void SetSelectionForegroundColour(const wxColour& colour);

    wxSize GetBestSize() const;

protected:
    void OnPaint(wxPaintEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKillFocus(wxFocusEvent& event);

    void DrawItems(wxDC& dc);
    int HitTest(const wxPoint& pos) const;
    void SetHoverItem(int item);

private:
    CustomDropDown* m_parent;
    std::vector<wxString> m_items;
    int m_selection;
    int m_hoverItem;
    
    wxColour m_backgroundColour;
    wxColour m_borderColour;
    wxColour m_selectionBackgroundColour;
    wxColour m_selectionForegroundColour;
    
    int m_itemHeight;
    
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(CustomDropDownPopup);
};

// Events
wxDECLARE_EVENT(wxEVT_CUSTOM_DROPDOWN_SELECTION, wxCommandEvent);

#endif // CUSTOMDROPDOWN_H 