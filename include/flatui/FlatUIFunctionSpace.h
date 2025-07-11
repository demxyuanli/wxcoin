#ifndef FLATUI_FUNCTION_SPACE_H
#define FLATUI_FUNCTION_SPACE_H

#include <wx/wx.h>

class FlatUIFunctionSpace : public wxControl // Inherit from wxPanel for easy child control management
{
public:
    FlatUIFunctionSpace(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FlatUIFunctionSpace();

    // Sets the main child control for this space
    void SetChildControl(wxWindow* child);
    wxWindow* GetChildControl() const { return m_childControl; }

    // Optional: Set a fixed width for this space, FlatUIBar might override or use it as a hint
    void SetSpaceWidth(int width);
    int GetSpaceWidth() const;


protected:
    void OnSize(wxSizeEvent& evt); // To resize the child control to fill this panel
    // void OnPaint(wxPaintEvent& evt); // Optional: if custom background/border needed

private:
    wxWindow* m_childControl; // The actual functional control (e.g., search bar)
    int m_spaceWidth;         // Desired width for this space

};

#endif // FLATUI_FUNCTION_SPACE_H 