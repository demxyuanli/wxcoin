#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <vector>
#include <memory>
#include "DockManager.h"

namespace ads {
// Forward declarations
class DockArea;
class DockWidget;
class DockSplitter;
class FloatingDockContainer;

/**
 * @brief Container widget that manages the layout of dock areas using splitters
 */
class DockContainerWidget : public wxPanel {
public:
    DockContainerWidget(DockManager* dockManager, wxWindow* parent);
    virtual ~DockContainerWidget();
    
    // Manager access  
    DockManager* dockManager() const { return m_dockManager; }

    // Area management
    DockArea* addDockWidget(DockWidgetArea area, DockWidget* dockWidget, 
                           DockArea* targetDockArea = nullptr, int index = -1);
    void removeDockArea(DockArea* area);
    void removeDockWidget(DockWidget* widget);
    DockArea* dockAreaAt(const wxPoint& globalPos) const;
    DockArea* dockArea(int index) const;
    std::vector<DockArea*> dockAreas() const { return m_dockAreas; }
    int dockAreaCount() const { return static_cast<int>(m_dockAreas.size()); }
    
    // Splitter management
    void addDockArea(DockArea* dockArea, DockWidgetArea area = CenterDockWidgetArea);
    void splitDockArea(DockArea* dockArea, DockArea* newDockArea, 
                      DockWidgetArea area, int splitRatio = 50);
    
    // Floating widgets
    FloatingDockContainer* floatingWidget() const { return m_floatingWidget; }
    void setFloatingWidget(FloatingDockContainer* floatingWidget);
    
    // Layout
    void saveState(wxString& xmlData) const;
    bool restoreState(const wxString& xmlData);
    wxWindow* rootSplitter() const { return m_rootSplitter; }
    
    // Visibility
    bool isInFrontOf(DockContainerWidget* other) const;
    void dumpLayout() const;
    
    // Features
    DockManagerFeatures features() const;
    
    // Raise and activate
    void raiseAndActivate();
    
    // Last added area
    DockArea* lastAddedDockArea() const { return m_lastAddedArea; }
    
    // Events
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREAS_ADDED;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREAS_REMOVED;
    
protected:
    // Internal layout management
    void updateSplitterHandles(wxWindow* splitter);
    wxWindow* createSplitter(wxOrientation orientation);
    void adjustSplitterSizes(wxWindow* splitter, int availableSize);
    DockArea* getDockAreaBySplitterChild(wxWindow* child) const;
    
    // Event handlers
    void onSize(wxSizeEvent& event);
    void onDockAreaDestroyed(wxWindowDestroyEvent& event);
    
private:
    // Private implementation details
    class Private;
    std::unique_ptr<Private> d;
    
    // Member variables
    DockManager* m_dockManager;
    std::vector<DockArea*> m_dockAreas;
    wxWindow* m_rootSplitter;
    wxBoxSizer* m_layout;
    FloatingDockContainer* m_floatingWidget;
    DockArea* m_lastAddedArea;
    
    // Helper methods
    void dropFloatingWidget(FloatingDockContainer* floatingWidget, const wxPoint& targetPos);
    void dropDockArea(DockArea* dockArea, DockWidgetArea area);
    void addDockAreaToContainer(DockWidgetArea area, DockArea* dockArea);
    void dropDockWidget(DockWidget* widget, DockWidgetArea dropArea, DockArea* targetArea);
    DockSplitter* newSplitter(wxOrientation orientation);
    void addDockAreaToSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);
    
    friend class DockManager;
    friend class DockArea;
    friend class FloatingDockContainer;
    friend class DockWidget;
    
    wxDECLARE_EVENT_TABLE();
};

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
