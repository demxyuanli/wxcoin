#include "docking/DockContainerWidget.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/DockSplitter.h"
#include "docking/FloatingDockContainer.h"
#include <algorithm>

namespace ads {

// Utility functions implementation
void DockContainerWidget::dropFloatingWidget(FloatingDockContainer* floatingWidget, const wxPoint& targetPos) {
    // TODO: Implement floating widget dropping
}

void DockContainerWidget::dropDockArea(DockArea* dockArea, DockWidgetArea area) {
    addDockArea(dockArea, area);
}

void DockContainerWidget::addDockAreaToContainer(DockWidgetArea area, DockArea* dockArea) {
    addDockArea(dockArea, area);
}

void DockContainerWidget::dropDockWidget(DockWidget* widget, DockWidgetArea dropArea, DockArea* targetArea) {
    if (!widget || !targetArea) {
        return;
    }

    // Create new dock area for the widget
    DockArea* newArea = new DockArea(m_dockManager, this);
    newArea->addDockWidget(widget);

    // Add the new area relative to target area
    addDockAreaToContainer(dropArea, newArea);
}

DockSplitter* DockContainerWidget::newSplitter(wxOrientation orientation) {
    return new DockSplitter(this);
}

void DockContainerWidget::updateSplitterHandles(wxWindow* splitter) {
    // TODO: Update splitter appearance
}

void DockContainerWidget::adjustSplitterSizes(wxWindow* splitter, int availableSize) {
    // TODO: Adjust splitter sizes
}

DockArea* DockContainerWidget::getDockAreaBySplitterChild(wxWindow* child) const {
    for (auto* area : m_dockAreas) {
        if (area == child) {
            return area;
        }
    }
    return nullptr;
}

void DockContainerWidget::saveState(wxString& xmlData) const {
    // TODO: Implement state saving
    xmlData = "<DockContainer />";
}

bool DockContainerWidget::restoreState(const wxString& xmlData) {
    // TODO: Implement state restoration
    return true;
}

bool DockContainerWidget::isInFrontOf(DockContainerWidget* other) const {
    // TODO: Implement z-order checking
    return false;
}

void DockContainerWidget::dumpLayout() const {
    // TODO: Implement layout debugging
}

DockManagerFeatures DockContainerWidget::features() const {
    return m_dockManager ? m_dockManager->configFlags() : DefaultConfig;
}

void DockContainerWidget::raiseAndActivate() {
    Raise();
    SetFocus();
}

void DockContainerWidget::onDockAreaDestroyed(wxWindowDestroyEvent& event) {
    // Handle dock area destruction
}

void DockContainerWidget::setFloatingWidget(FloatingDockContainer* floatingWidget) {
    m_floatingWidget = floatingWidget;
}

void DockContainerWidget::splitDockArea(DockArea* dockArea, DockArea* newDockArea,
                                       DockWidgetArea area, int splitRatio) {
    // TODO: Implement dock area splitting
    addDockArea(newDockArea, area);
}

} // namespace ads
