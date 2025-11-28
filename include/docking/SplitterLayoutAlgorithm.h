#pragma once

#include <wx/wx.h>
#include "DockManager.h"

namespace ads {

class DockArea;
class DockSplitter;
class DockContainerWidget;

class SplitterLayoutAlgorithm {
public:
    SplitterLayoutAlgorithm(DockContainerWidget* container);
    virtual ~SplitterLayoutAlgorithm();

    void addDockAreaSimple(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void addToVerticalSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);
    void addToHorizontalLayout(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);
    void create3WaySplit(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);
    void handleTopBottomArea(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void handleMiddleLayerArea(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void restructureForTopBottom(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void ensureTopBottomLayout(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void addToMiddleLayer(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void createMiddleSplitter(DockSplitter* rootSplitter, DockArea* existingArea, DockArea* newArea, DockWidgetArea area);
    void addDockAreaToMiddleSplitter(DockSplitter* middleSplitter, DockArea* dockArea, DockWidgetArea area);
    void addDockAreaToSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);

private:
    DockContainerWidget* m_container;
    
    int getConfiguredAreaSize(DockWidgetArea area) const;
    void ensureAllChildrenVisible(wxWindow* window);
};

} // namespace ads

