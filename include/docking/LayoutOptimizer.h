#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <memory>

namespace ads {

class DockManager;
class DockContainerWidget;
class DockOverlay;

class LayoutOptimizer {
public:
    LayoutOptimizer(DockManager* manager);
    virtual ~LayoutOptimizer();

    void beginBatchOperation();
    void endBatchOperation();
    void updateLayout();
    void optimizeMemoryUsage();
    void cleanupUnusedResources();

    void onLayoutUpdateTimer(wxTimerEvent& event);

private:
    DockManager* m_manager;
    wxTimer* m_layoutUpdateTimer;
    int m_batchOperationCount;
    wxWindow* m_containerWidget;

    void initializeTimer();
};

} // namespace ads

