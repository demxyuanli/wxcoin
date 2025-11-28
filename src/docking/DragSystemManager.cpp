#include "docking/DragSystemManager.h"
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/DockContainerWidget.h"
#include "docking/DockOverlay.h"
#include <wx/log.h>
#include <cmath>
#include <algorithm>

namespace ads {

DragSystemManager::DragSystemManager(DockManager* manager)
    : m_manager(manager)
    , m_dragState(DragState::Inactive)
    , m_isProcessingDrag(false)
    , m_lastMousePos(wxDefaultPosition)
    , m_dockAreaOverlay(manager ? manager->dockAreaOverlay() : nullptr)
    , m_containerOverlay(manager ? manager->containerOverlay() : nullptr)
    , m_containerWidget(manager ? manager->containerWidget() : nullptr)
{
}

DragSystemManager::~DragSystemManager() {
}

void DragSystemManager::optimizeDragOperation(DockWidget* draggedWidget) {
    if (!draggedWidget || m_isProcessingDrag) {
        return;
    }

    m_isProcessingDrag = true;
    m_lastMousePos = wxGetMousePosition();
    updateDragTargets();

    if (m_dragState == DragState::Inactive) {
        startDragOperation(draggedWidget);
    } else {
        updateDragOperation(draggedWidget);
    }

    m_isProcessingDrag = false;
}

void DragSystemManager::startDragOperation(DockWidget* draggedWidget) {
    m_dragState = DragState::Started;

    m_dragContext.draggedWidget = draggedWidget;
    m_dragContext.startTime = wxGetLocalTimeMillis();
    m_dragContext.lastUpdateTime = m_dragContext.startTime;
    m_dragContext.dragDistance = 0;
    m_dragContext.isGlobalDocking = false;

    setOptimizedRendering(true);
    showInitialDragHints(draggedWidget);

    wxLogDebug("Started drag operation for widget: %p", draggedWidget);
}

void DragSystemManager::updateDragOperation(DockWidget* draggedWidget) {
    if (m_dragState == DragState::Started) {
        m_dragState = DragState::Active;
    }

    wxLongLong currentTime = wxGetLocalTimeMillis();
    wxLongLong timeDelta = currentTime - m_dragContext.lastUpdateTime;

    wxPoint currentPos = wxGetMousePosition();
    wxPoint delta = currentPos - m_lastMousePos;
    double distance = sqrt(delta.x * delta.x + delta.y * delta.y);
    m_dragContext.dragDistance += distance;

    if (timeDelta > 0) {
        m_dragContext.dragVelocity = distance / timeDelta.GetValue();
    }

    checkGlobalDockingConditions();
    updateOverlayHints();

    m_dragContext.lastUpdateTime = currentTime;
    m_lastMousePos = currentPos;
}

void DragSystemManager::finishDragOperation(DockWidget* draggedWidget, bool cancelled) {
    wxLogDebug("Finishing drag operation for widget: %p, cancelled: %d", draggedWidget, cancelled);

    hideAllOverlays();
    m_dragState = DragState::Inactive;
    setOptimizedRendering(false);
    m_dragContext = DragContext();
    m_cachedDropTargets.clear();
}

void DragSystemManager::checkGlobalDockingConditions() {
    wxPoint mousePos = wxGetMousePosition();
    wxRect screenRect = wxGetClientDisplayRect();

    const int globalDockThreshold = 20;

    bool nearEdge = mousePos.x <= globalDockThreshold ||
                    mousePos.x >= screenRect.GetWidth() - globalDockThreshold ||
                    mousePos.y <= globalDockThreshold ||
                    mousePos.y >= screenRect.GetHeight() - globalDockThreshold;

    wxLongLong dragDuration = wxGetLocalTimeMillis() - m_dragContext.startTime;
    bool sufficientDrag = dragDuration > 500 && m_dragContext.dragDistance > 50;

    if (nearEdge && sufficientDrag && !m_dragContext.isGlobalDocking) {
        enableGlobalDocking();
    } else if (!nearEdge && m_dragContext.isGlobalDocking) {
        disableGlobalDocking();
    }
}

void DragSystemManager::enableGlobalDocking() {
    wxLogDebug("Enabling global docking mode");

    m_dragContext.isGlobalDocking = true;

    if (m_containerOverlay) {
        m_containerOverlay->setGlobalMode(true);
        m_containerOverlay->showOverlay(m_containerWidget);
    }

    if (m_dockAreaOverlay) {
        m_dockAreaOverlay->hideOverlay();
    }
}

void DragSystemManager::disableGlobalDocking() {
    wxLogDebug("Disabling global docking mode");

    m_dragContext.isGlobalDocking = false;

    if (m_containerOverlay) {
        m_containerOverlay->setGlobalMode(false);
        m_containerOverlay->hideOverlay();
    }
}

void DragSystemManager::showInitialDragHints(DockWidget* draggedWidget) {
    if (m_dockAreaOverlay && draggedWidget && draggedWidget->dockAreaWidget()) {
        DockArea* dockArea = draggedWidget->dockAreaWidget();
        m_dockAreaOverlay->showOverlay(dockArea);
    }
}

void DragSystemManager::updateOverlayHints() {
    wxPoint mousePos = wxGetMousePosition();

    if (m_dragContext.isGlobalDocking) {
        if (m_containerOverlay && m_containerOverlay->IsShown()) {
            m_containerOverlay->updatePosition();
        }
    } else {
        updateLocalDockingHints(mousePos);
    }
}

void DragSystemManager::updateLocalDockingHints(const wxPoint& mousePos) {
    if (m_dragState != DragState::Started && m_dragState != DragState::Active) {
        return;
    }

    DockArea* bestTargetArea = nullptr;
    double bestDistance = std::numeric_limits<double>::max();

    for (wxWindow* window : m_cachedDropTargets) {
        if (DockArea* area = dynamic_cast<DockArea*>(window)) {
            wxRect areaRect = area->GetScreenRect();
            if (areaRect.Contains(mousePos)) {
                bestTargetArea = area;
                break;
            } else {
                wxPoint center = areaRect.GetPosition() + wxPoint(areaRect.GetWidth()/2, areaRect.GetHeight()/2);
                double distance = sqrt(pow(mousePos.x - center.x, 2) + pow(mousePos.y - center.y, 2));
                if (distance < bestDistance && distance < 200) {
                    bestDistance = distance;
                    bestTargetArea = area;
                }
            }
        }
    }

    if (bestTargetArea && bestTargetArea != m_dragContext.lastTargetArea) {
        if (m_dockAreaOverlay) {
            m_dockAreaOverlay->showOverlay(bestTargetArea);
        }
        m_dragContext.lastTargetArea = bestTargetArea;
    } else if (!bestTargetArea && m_dragContext.lastTargetArea) {
        if (m_dockAreaOverlay) {
            m_dockAreaOverlay->hideOverlay();
        }
        m_dragContext.lastTargetArea = nullptr;
    }
}

void DragSystemManager::hideAllOverlays() {
    if (m_containerOverlay) {
        m_containerOverlay->hideOverlay();
    }
    if (m_dockAreaOverlay) {
        m_dockAreaOverlay->hideOverlay();
    }
}

void DragSystemManager::setOptimizedRendering(bool enabled) {
    if (m_containerOverlay) {
        m_containerOverlay->setRenderingOptimization(enabled);
    }
    if (m_dockAreaOverlay) {
        m_dockAreaOverlay->setRenderingOptimization(enabled);
    }
}

void DragSystemManager::updateDragTargets() {
    m_cachedDropTargets.clear();

    if (m_containerWidget) {
        collectDropTargets(m_containerWidget);
    }
}

void DragSystemManager::collectDropTargets(wxWindow* window) {
    if (!window) return;

    if (dynamic_cast<DockArea*>(window)) {
        m_cachedDropTargets.push_back(window);
    } else if (dynamic_cast<DockContainerWidget*>(window)) {
        m_cachedDropTargets.push_back(window);
    }

    wxWindowList& children = window->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        collectDropTargets(*it);
    }
}

} // namespace ads

