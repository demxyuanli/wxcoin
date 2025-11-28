#include "docking/LayoutOptimizer.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"
#include "docking/DockOverlay.h"
#include "docking/FloatingDockContainer.h"

namespace ads {

LayoutOptimizer::LayoutOptimizer(DockManager* manager)
    : m_manager(manager)
    , m_layoutUpdateTimer(nullptr)
    , m_batchOperationCount(0)
    , m_containerWidget(manager ? manager->containerWidget() : nullptr)
{
    initializeTimer();
}

LayoutOptimizer::~LayoutOptimizer() {
    if (m_layoutUpdateTimer) {
        if (m_layoutUpdateTimer->IsRunning()) {
            m_layoutUpdateTimer->Stop();
        }
        delete m_layoutUpdateTimer;
    }
}

void LayoutOptimizer::initializeTimer() {
    if (!m_manager) {
        return;
    }

    m_layoutUpdateTimer = new wxTimer(m_manager);
    m_manager->Bind(wxEVT_TIMER, &LayoutOptimizer::onLayoutUpdateTimer, this, m_layoutUpdateTimer->GetId());
}

void LayoutOptimizer::beginBatchOperation() {
    m_batchOperationCount++;
    if (m_batchOperationCount == 1) {
        if (m_layoutUpdateTimer && m_layoutUpdateTimer->IsRunning()) {
            m_layoutUpdateTimer->Stop();
        }
    }
}

void LayoutOptimizer::endBatchOperation() {
    if (m_batchOperationCount > 0) {
        m_batchOperationCount--;
        if (m_batchOperationCount == 0) {
            updateLayout();
        }
    }
}

void LayoutOptimizer::updateLayout() {
    if (m_batchOperationCount > 0) {
        return;
    }

    if (m_containerWidget) {
        m_containerWidget->Freeze();
        m_containerWidget->Layout();
        
        wxRect clientRect = m_containerWidget->GetClientRect();
        m_containerWidget->RefreshRect(clientRect, false);
        
        m_containerWidget->Thaw();
    }
}

void LayoutOptimizer::onLayoutUpdateTimer(wxTimerEvent& event) {
    updateLayout();
}

void LayoutOptimizer::cleanupUnusedResources() {
    if (!m_manager) {
        return;
    }

    auto floatingWidgets = m_manager->floatingWidgets();
    for (auto* floating : floatingWidgets) {
        if (floating && floating->dockWidgets().empty()) {
            floating->Destroy();
        }
    }
}

void LayoutOptimizer::optimizeMemoryUsage() {
    cleanupUnusedResources();

    if (!m_manager) {
        return;
    }

    DockOverlay* areaOverlay = m_manager->dockAreaOverlay();
    if (areaOverlay) {
        areaOverlay->optimizeRendering();
    }

    DockOverlay* containerOverlay = m_manager->containerOverlay();
    if (containerOverlay) {
        containerOverlay->optimizeRendering();
    }
}

} // namespace ads

