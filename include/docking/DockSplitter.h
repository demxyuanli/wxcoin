#pragma once

#include <wx/wx.h>
#include <wx/splitter.h>
#include <vector>

namespace ads {

/**
 * @brief Custom splitter window for dock areas
 */
class DockSplitter : public wxSplitterWindow {
public:
    DockSplitter(wxWindow* parent);
    virtual ~DockSplitter();

    // Orientation
    void setOrientation(wxOrientation orientation);
    wxOrientation orientation() const { return m_orientation; }

    // Insert widget
    void insertWidget(int index, wxWindow* widget, bool stretch = true);
    void addWidget(wxWindow* widget, bool stretch = true);
    wxWindow* replaceWidget(wxWindow* from, wxWindow* to);

    // Widget access
    wxWindow* widget(int index) const;
    int indexOf(wxWindow* widget) const;
    int widgetCount() const;
    bool hasVisibleContent() const;

    // Sizes
    void setSizes(const std::vector<int>& sizes);
    std::vector<int> sizes() const;

protected:
    void OnSplitterSashPosChanging(wxSplitterEvent& event);
    void OnSplitterSashPosChanged(wxSplitterEvent& event);

private:
    wxOrientation m_orientation;
    std::vector<wxWindow*> m_widgets;
    std::vector<int> m_sizes;

    void updateSplitter();

    wxDECLARE_EVENT_TABLE();
};

} // namespace ads
