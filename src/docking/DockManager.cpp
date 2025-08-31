#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/DockContainerWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockOverlay.h"
#include "docking/AutoHideContainer.h"
#include "docking/PerspectiveManager.h"
#include "docking/DockLayoutConfig.h"
#include <wx/xml/xml.h>
#include <wx/sstream.h>
#include <algorithm>

namespace ads {

// Private implementation class
class DockManager::Private {
public:
    Private(DockManager* parent) : q(parent) {}
    
    DockManager* q;
    std::vector<FloatingDockContainer*> floatingWidgets;
    DockOverlay* containerOverlay = nullptr;
    DockOverlay* dockAreaOverlay = nullptr;
    AutoHideManager* autoHideManager = nullptr;
    PerspectiveManager* perspectiveManager = nullptr;
};

DockManager::DockManager(wxWindow* parent)
    : d(std::make_unique<Private>(this))
    , m_parent(parent)
    , m_containerWidget(nullptr)
    , m_centralWidget(nullptr)
    , m_dockOverlay(nullptr)
    , m_activeDockWidget(nullptr)
    , m_configFlags(DefaultConfig)
    , m_dragState(DragInactive)
    , m_lastMousePos(wxDefaultPosition)
    , m_isProcessingDrag(false)
{
    // Create the main container widget
    m_containerWidget = new DockContainerWidget(this, parent);

    // Create overlays for drag and drop with better performance
    m_dockOverlay = new DockOverlay(m_containerWidget);
    d->containerOverlay = new DockOverlay(m_containerWidget, DockOverlay::ModeContainerOverlay);
    d->dockAreaOverlay = new DockOverlay(m_containerWidget, DockOverlay::ModeDockAreaOverlay);

    // Create auto-hide manager
    d->autoHideManager = new AutoHideManager(static_cast<DockContainerWidget*>(m_containerWidget));

    // Create perspective manager
    d->perspectiveManager = new PerspectiveManager(this);

    // Initialize layout configuration
    m_layoutConfig = std::make_unique<DockLayoutConfig>();
    m_layoutConfig->LoadFromConfig();

    // Initialize performance monitoring
    m_layoutUpdateTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &DockManager::onLayoutUpdateTimer, this, m_layoutUpdateTimer->GetId());

    // Initialize performance variables
    initializePerformanceVariables();
}

DockManager::~DockManager() {
    // Stop any running timers
    if (m_layoutUpdateTimer && m_layoutUpdateTimer->IsRunning()) {
        m_layoutUpdateTimer->Stop();
    }
    delete m_layoutUpdateTimer;

    // First, clear the dock widgets list to prevent callbacks during destruction
    std::vector<DockWidget*> widgetsToDelete = m_dockWidgets;
    m_dockWidgets.clear();
    m_dockWidgetsMap.clear();

    // IMPORTANT: Destroy dock widgets BEFORE destroying the container
    // This ensures widgets are cleaned up while their parent container is still valid
    for (auto* widget : widgetsToDelete) {
        // Just destroy - the widget will handle cleanup in its destructor
        widget->Destroy();
    }

    // Clean up floating widgets
    while (!m_floatingWidgets.empty()) {
        m_floatingWidgets.back()->Destroy();
        m_floatingWidgets.pop_back();
    }

    // Delete perspective manager
    delete d->perspectiveManager;

    // Delete auto-hide manager
    delete d->autoHideManager;

    // Delete overlays
    delete m_dockOverlay;
    delete d->containerOverlay;
    delete d->dockAreaOverlay;

    // Delete container widget LAST after all children are destroyed
    if (m_containerWidget) {
        m_containerWidget->Destroy();
    }
}

DockArea* DockManager::addDockWidget(DockWidgetArea area, DockWidget* dockWidget, 
                                    DockArea* targetDockArea) {
    wxLogDebug("DockManager::addDockWidget - area: %d, widget: %p, targetArea: %p", 
        area, dockWidget, targetDockArea);
        
    if (!dockWidget) {
        wxLogDebug("  -> dockWidget is null");
        return nullptr;
    }
    
    // Register the dock widget
    registerDockWidget(dockWidget);
    
    // Get the container widget
    DockContainerWidget* container = static_cast<DockContainerWidget*>(m_containerWidget);
    wxLogDebug("  -> container: %p", container);
    
    if (!container) {
        wxLogDebug("  -> container is null!");
        return nullptr;
    }
    
    // Add to container
    DockArea* dockArea = container->addDockWidget(area, dockWidget, targetDockArea);
    wxLogDebug("  -> result dockArea: %p", dockArea);
    
    // Emit signal
    for (auto& callback : m_dockWidgetAddedCallbacks) {
        callback(dockWidget);
    }
    
    return dockArea;
}

DockArea* DockManager::addDockWidgetTab(DockWidgetArea area, DockWidget* dockWidget) {
    DockArea* targetArea = nullptr;
    
    // Find an existing dock area in the specified area
    if (!m_dockAreas.empty()) {
        // For now, just use the first dock area
        targetArea = m_dockAreas.front();
    }
    
    return addDockWidget(area, dockWidget, targetArea);
}

DockArea* DockManager::addDockWidgetTabToArea(DockWidget* dockWidget, DockArea* targetDockArea) {
    if (!targetDockArea) {
        return addDockWidget(CenterDockWidgetArea, dockWidget);
    }
    
    targetDockArea->addDockWidget(dockWidget);
    return targetDockArea;
}

void DockManager::removeDockWidget(DockWidget* dockWidget) {
    if (!dockWidget) {
        return;
    }
    
    // Remove from dock area if docked
    if (dockWidget->dockAreaWidget()) {
        dockWidget->dockAreaWidget()->removeDockWidget(dockWidget);
    }
    
    // Unregister the widget
    unregisterDockWidget(dockWidget);
    
    // Emit signal
    for (auto& callback : m_dockWidgetRemovedCallbacks) {
        callback(dockWidget);
    }
}

std::vector<DockWidget*> DockManager::dockWidgets() const {
    return m_dockWidgets;
}

DockWidget* DockManager::findDockWidget(const wxString& objectName) const {
    auto it = m_dockWidgetsMap.find(objectName);
    if (it != m_dockWidgetsMap.end()) {
        return it->second;
    }
    return nullptr;
}

void DockManager::saveState(wxString& xmlData) const {
    wxXmlDocument doc;
    wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "DockingState");
    doc.SetRoot(root);
    
    // Save version
    root->AddAttribute("Version", "1.0");
    
    // Save dock widgets state
    wxXmlNode* widgetsNode = new wxXmlNode(wxXML_ELEMENT_NODE, "DockWidgets");
    root->AddChild(widgetsNode);
    
    for (auto* widget : m_dockWidgets) {
        wxXmlNode* widgetNode = new wxXmlNode(wxXML_ELEMENT_NODE, "DockWidget");
        widgetNode->AddAttribute("Name", widget->objectName());
        widgetNode->AddAttribute("Closed", widget->isClosed() ? "1" : "0");
        widgetsNode->AddChild(widgetNode);
    }
    
    // Save container state
    wxXmlNode* containerNode = new wxXmlNode(wxXML_ELEMENT_NODE, "Container");
    root->AddChild(containerNode);
    
    wxString containerState;
    static_cast<DockContainerWidget*>(m_containerWidget)->saveState(containerState);
    containerNode->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", containerState));
    
    // Save floating widgets state
    wxXmlNode* floatingNode = new wxXmlNode(wxXML_ELEMENT_NODE, "FloatingWidgets");
    root->AddChild(floatingNode);
    
    for (auto* floating : m_floatingWidgets) {
        wxXmlNode* floatNode = new wxXmlNode(wxXML_ELEMENT_NODE, "FloatingWidget");
        
        wxString floatState;
        floating->saveState(floatState);
        floatNode->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", floatState));
        
        floatingNode->AddChild(floatNode);
    }
    
    // Convert to string
    wxStringOutputStream stream(&xmlData);
    doc.Save(stream);
}

bool DockManager::restoreState(const wxString& xmlData) {
    wxXmlDocument doc;
    wxStringInputStream stream(xmlData);
    
    if (!doc.Load(stream)) {
        return false;
    }
    
    wxXmlNode* root = doc.GetRoot();
    if (!root || root->GetName() != "DockingState") {
        return false;
    }
    
    // Check version
    wxString version = root->GetAttribute("Version", "");
    if (version != "1.0") {
        return false;
    }
    
    // Restore dock widgets state
    wxXmlNode* widgetsNode = root->GetChildren();
    while (widgetsNode) {
        if (widgetsNode->GetName() == "DockWidgets") {
            wxXmlNode* widgetNode = widgetsNode->GetChildren();
            while (widgetNode) {
                if (widgetNode->GetName() == "DockWidget") {
                    wxString name = widgetNode->GetAttribute("Name", "");
                    bool closed = widgetNode->GetAttribute("Closed", "0") == "1";
                    
                    DockWidget* widget = findDockWidget(name);
                    if (widget) {
                        if (closed) {
                            widget->toggleView(false);
                        }
                    }
                }
                widgetNode = widgetNode->GetNext();
            }
        }
        widgetsNode = widgetsNode->GetNext();
    }
    
    return true;
}

FloatingDockContainer* DockManager::addDockWidgetFloating(DockWidget* dockWidget) {
    if (!dockWidget) {
        return nullptr;
    }
    
    // Create floating container
    FloatingDockContainer* floatingContainer = new FloatingDockContainer(dockWidget);
    
    // Register it
    registerFloatingWidget(floatingContainer);
    
    // Show it
    floatingContainer->Show();
    
    return floatingContainer;
}

void DockManager::setFloatingContainersTitle(const wxString& title) {
    for (auto* floating : m_floatingWidgets) {
        floating->SetTitle(title);
    }
}

void DockManager::setConfigFlags(DockManagerFeatures features) {
    m_configFlags = features;
}

DockManagerFeatures DockManager::configFlags() const {
    return m_configFlags;
}

void DockManager::setConfigFlag(DockManagerFeature flag, bool on) {
    if (on) {
        m_configFlags |= flag;
    } else {
        m_configFlags &= ~flag;
    }
}

bool DockManager::testConfigFlag(DockManagerFeature flag) const {
    return (m_configFlags & flag) == flag;
}

void DockManager::setStyleSheet(const wxString& styleSheet) {
    m_styleSheet = styleSheet;
    // TODO: Apply style sheet to widgets
}

wxString DockManager::styleSheet() const {
    return m_styleSheet;
}

void DockManager::onDockWidgetAdded(const DockWidgetCallback& callback) {
    m_dockWidgetAddedCallbacks.push_back(callback);
}

void DockManager::onDockWidgetRemoved(const DockWidgetCallback& callback) {
    m_dockWidgetRemovedCallbacks.push_back(callback);
}

void DockManager::onDockWidgetAboutToClose(const DockWidgetCallback& callback) {
    m_dockWidgetAboutToCloseCallbacks.push_back(callback);
}

void DockManager::setActiveDockWidget(DockWidget* widget) {
    if (m_activeDockWidget == widget) {
        return;
    }
    
    m_activeDockWidget = widget;
    
    // Update UI to reflect active widget
    if (widget && widget->dockAreaWidget()) {
        widget->dockAreaWidget()->setCurrentDockWidget(widget);
    }
}

DockWidget* DockManager::focusedDockWidget() const {
    // Find the dock widget that has focus
    wxWindow* focused = wxWindow::FindFocus();
    
    while (focused) {
        // Check if it's a dock widget
        DockWidget* dockWidget = dynamic_cast<DockWidget*>(focused);
        if (dockWidget) {
            return dockWidget;
        }
        
        // Check parent
        focused = focused->GetParent();
    }
    
    return nullptr;
}

std::vector<DockArea*> DockManager::dockAreas() const {
    return m_dockAreas;
}

std::vector<FloatingDockContainer*> DockManager::floatingWidgets() const {
    return m_floatingWidgets;
}

void DockManager::setCentralWidget(wxWindow* widget) {
    m_centralWidget = widget;
    
    // Add to center of container
    if (widget && m_containerWidget) {
        // TODO: Implement central widget support
    }
}

void DockManager::registerDockWidget(DockWidget* dockWidget) {
    if (!dockWidget) {
        return;
    }
    
    // Add to list
    m_dockWidgets.push_back(dockWidget);
    
    // Add to map
    if (!dockWidget->objectName().IsEmpty()) {
        m_dockWidgetsMap[dockWidget->objectName()] = dockWidget;
    }
    
    // Set manager
    dockWidget->setDockManager(this);
}

void DockManager::unregisterDockWidget(DockWidget* dockWidget) {
    if (!dockWidget) {
        return;
    }
    
    // Remove from list
    auto it = std::find(m_dockWidgets.begin(), m_dockWidgets.end(), dockWidget);
    if (it != m_dockWidgets.end()) {
        m_dockWidgets.erase(it);
    }
    
    // Remove from map
    if (!dockWidget->objectName().IsEmpty()) {
        m_dockWidgetsMap.erase(dockWidget->objectName());
    }
    
    // Clear manager
    dockWidget->setDockManager(nullptr);
}

void DockManager::registerDockArea(DockArea* dockArea) {
    if (!dockArea) {
        return;
    }
    
    m_dockAreas.push_back(dockArea);
}

void DockManager::unregisterDockArea(DockArea* dockArea) {
    if (!dockArea) {
        return;
    }
    
    auto it = std::find(m_dockAreas.begin(), m_dockAreas.end(), dockArea);
    if (it != m_dockAreas.end()) {
        m_dockAreas.erase(it);
    }
}

void DockManager::registerFloatingWidget(FloatingDockContainer* floatingWidget) {
    if (!floatingWidget) {
        return;
    }
    
    m_floatingWidgets.push_back(floatingWidget);
}

void DockManager::unregisterFloatingWidget(FloatingDockContainer* floatingWidget) {
    if (!floatingWidget) {
        return;
    }
    
    auto it = std::find(m_floatingWidgets.begin(), m_floatingWidgets.end(), floatingWidget);
    if (it != m_floatingWidgets.end()) {
        m_floatingWidgets.erase(it);
    }
}

void DockManager::onDockAreaCreated(DockArea* dockArea) {
    registerDockArea(dockArea);
}

void DockManager::onDockAreaAboutToClose(DockArea* dockArea) {
    // Handle dock area closing
}

void DockManager::onFloatingWidgetCreated(FloatingDockContainer* floatingWidget) {
    registerFloatingWidget(floatingWidget);
}

void DockManager::onFloatingWidgetAboutToClose(FloatingDockContainer* floatingWidget) {
    unregisterFloatingWidget(floatingWidget);
}

void DockManager::setAutoHide(DockWidget* widget, DockWidgetArea area) {
    if (!widget || !d->autoHideManager) {
        return;
    }
    
    // Remove from current location
    if (widget->dockAreaWidget()) {
        widget->dockAreaWidget()->removeDockWidget(widget);
    }
    
    // Convert area to auto-hide location
    AutoHideSideBarLocation location;
    switch (area) {
    case LeftDockWidgetArea:
        location = SideBarLeft;
        break;
    case RightDockWidgetArea:
        location = SideBarRight;
        break;
    case TopDockWidgetArea:
        location = SideBarTop;
        break;
    case BottomDockWidgetArea:
        location = SideBarBottom;
        break;
    default:
        location = SideBarLeft; // Default
        break;
    }
    
    // Add to auto-hide
    d->autoHideManager->addAutoHideWidget(widget, location);
}

void DockManager::restoreFromAutoHide(DockWidget* widget) {
    if (!widget || !d->autoHideManager) {
        return;
    }
    
    d->autoHideManager->restoreDockWidget(widget);
}

bool DockManager::isAutoHide(DockWidget* widget) const {
    if (!widget || !d->autoHideManager) {
        return false;
    }
    
    return d->autoHideManager->autoHideContainer(widget) != nullptr;
}

std::vector<DockWidget*> DockManager::autoHideWidgets() const {
    if (!d->autoHideManager) {
        return std::vector<DockWidget*>();
    }
    
    return d->autoHideManager->autoHideWidgets();
}

PerspectiveManager* DockManager::perspectiveManager() const {
    return d->perspectiveManager;
}

DockOverlay* DockManager::dockAreaOverlay() const {
    return d->dockAreaOverlay;
}

DockOverlay* DockManager::containerOverlay() const {
    return d->containerOverlay;
}

// Performance optimization methods
void DockManager::beginBatchOperation() {
    m_batchOperationCount++;
    if (m_batchOperationCount == 1) {
        // Pause layout updates during batch operations
        if (m_layoutUpdateTimer && m_layoutUpdateTimer->IsRunning()) {
            m_layoutUpdateTimer->Stop();
        }
    }
}

void DockManager::endBatchOperation() {
    if (m_batchOperationCount > 0) {
        m_batchOperationCount--;
        if (m_batchOperationCount == 0) {
            // Resume layout updates and trigger a refresh
            updateLayout();
        }
    }
}

void DockManager::updateLayout() {
    if (m_batchOperationCount > 0) {
        // Defer layout update until batch operation completes
        return;
    }

    if (m_containerWidget) {
        m_containerWidget->Layout();
        m_containerWidget->Refresh();
    }
}

void DockManager::onLayoutUpdateTimer(wxTimerEvent& event) {
    updateLayout();
}

// Enhanced drag and drop optimization
void DockManager::optimizeDragOperation(DockWidget* draggedWidget) {
    if (!draggedWidget || m_isProcessingDrag) {
        return;
    }

    m_isProcessingDrag = true;

    // Cache frequently used values
    m_lastMousePos = wxGetMousePosition();

    // Use cached window hierarchy for faster lookups
    updateDragTargets();

    m_isProcessingDrag = false;
}

void DockManager::updateDragTargets() {
    // Cache potential drop targets to avoid repeated window hierarchy traversals
    m_cachedDropTargets.clear();

    if (m_containerWidget) {
        collectDropTargets(m_containerWidget);
    }
}

void DockManager::collectDropTargets(wxWindow* window) {
    if (!window) return;

    // Add dock areas and containers as potential drop targets
    if (dynamic_cast<DockArea*>(window)) {
        m_cachedDropTargets.push_back(window);
    } else if (dynamic_cast<DockContainerWidget*>(window)) {
        m_cachedDropTargets.push_back(window);
    }

    // Recursively collect from children
    wxWindowList& children = window->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        collectDropTargets(*it);
    }
}

// Memory management optimization
void DockManager::cleanupUnusedResources() {
    // Clean up any unused floating containers
    auto it = m_floatingWidgets.begin();
    while (it != m_floatingWidgets.end()) {
        FloatingDockContainer* floating = *it;
        if (floating && floating->dockWidgets().empty()) {
            floating->Destroy();
            it = m_floatingWidgets.erase(it);
        } else {
            ++it;
        }
    }
}

void DockManager::optimizeMemoryUsage() {
    cleanupUnusedResources();

    // Optimize overlay rendering
    if (d->dockAreaOverlay) {
        d->dockAreaOverlay->optimizeRendering();
    }
    if (d->containerOverlay) {
        d->containerOverlay->optimizeRendering();
    }
}

// Initialize performance variables
void DockManager::initializePerformanceVariables() {
    m_batchOperationCount = 0;
    m_isProcessingDrag = false;
    m_dragState = DragInactive;
    m_lastMousePos = wxDefaultPosition;
    m_cachedDropTargets.clear();
}

void DockManager::setLayoutConfig(const DockLayoutConfig& config) {
    if (m_layoutConfig) {
        *m_layoutConfig = config;
        m_layoutConfig->SaveToConfig();
    }
}

const DockLayoutConfig& DockManager::getLayoutConfig() const {
    if (!m_layoutConfig) {
        // Create a default config if not initialized
        const_cast<DockManager*>(this)->m_layoutConfig = std::make_unique<DockLayoutConfig>();
    }
    return *m_layoutConfig;
}

} // namespace ads
