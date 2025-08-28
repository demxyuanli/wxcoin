#include "docking/DockContainerWidget.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/FloatingDockContainer.h"
#include <algorithm>

namespace ads {
<<<<<<< Current (Your changes)
	// Define custom events
	wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_ADDED(wxNewEventType());
	wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_REMOVED(wxNewEventType());

	// Event table
	wxBEGIN_EVENT_TABLE(DockContainerWidget, wxPanel)
		EVT_SIZE(DockContainerWidget::onSize)
		wxEND_EVENT_TABLE()

		wxBEGIN_EVENT_TABLE(DockSplitter, wxSplitterWindow)
		EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, DockSplitter::OnSplitterSashPosChanging)
		EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, DockSplitter::OnSplitterSashPosChanged)
		wxEND_EVENT_TABLE()

		// Private implementation
		class DockContainerWidget::Private {
		public:
			Private(DockContainerWidget* parent) : q(parent) {}

			DockContainerWidget* q;
			DockSplitter* rootSplitter = nullptr;
			wxWindow* centralWidget = nullptr;
	};

	// DockContainerWidget implementation
	DockContainerWidget::DockContainerWidget(DockManager* dockManager, wxWindow* parent)
		: wxPanel(parent)
		, d(std::make_unique<Private>(this))
		, m_dockManager(dockManager)
		, m_floatingWidget(nullptr)
		, m_lastAddedArea(nullptr)
	{
		// Create layout
		m_layout = new wxBoxSizer(wxVERTICAL);
		SetSizer(m_layout);

		// Create root splitter
		m_rootSplitter = new DockSplitter(this);
		m_layout->Add(m_rootSplitter, 1, wxEXPAND);
	}

	DockContainerWidget::~DockContainerWidget() {
		// Clear all dock areas first to ensure proper cleanup order
		m_dockAreas.clear();

		// The splitter and other child windows will be destroyed automatically
		// by wxWidgets parent-child mechanism
	}

	DockArea* DockContainerWidget::addDockWidget(DockWidgetArea area, DockWidget* dockWidget,
		DockArea* targetDockArea, int index) {
		wxLogDebug("DockContainerWidget::addDockWidget - area: %d, widget: %p, targetArea: %p",
			area, dockWidget, targetDockArea);

		if (!dockWidget) {
			wxLogDebug("  -> dockWidget is null");
			return nullptr;
		}

		// If we have a target dock area, add to it
		if (targetDockArea) {
			wxLogDebug("  -> Adding to existing target area");
			targetDockArea->addDockWidget(dockWidget);
			return targetDockArea;
		}

		// If we already have many areas, add as tab to avoid complex layouts
		if (m_dockAreas.size() >= 4) {
			wxLogDebug("  -> Too many areas (%d), adding as tab", (int)m_dockAreas.size());
			// Find the best area to add to
			DockArea* bestArea = nullptr;

			// Try to find an area in the same general position
			for (auto* existingArea : m_dockAreas) {
				if (existingArea && existingArea->GetParent()) {
					// For now, just use the first valid area
					bestArea = existingArea;
					break;
				}
			}

			if (bestArea) {
				bestArea->addDockWidget(dockWidget);
				return bestArea;
			}
		}

		// Create new dock area
		wxLogDebug("  -> Creating new dock area");
		DockArea* newDockArea = new DockArea(m_dockManager, this);
		newDockArea->addDockWidget(dockWidget);

		// Add dock area to container
		addDockArea(newDockArea, area);

		m_lastAddedArea = newDockArea;

		return newDockArea;
	}

	void DockContainerWidget::removeDockArea(DockArea* area) {
		if (!area) {
			return;
		}

		wxLogDebug("DockContainerWidget::removeDockArea - removing area %p", area);

		// Remove from list
		auto it = std::find(m_dockAreas.begin(), m_dockAreas.end(), area);
		if (it != m_dockAreas.end()) {
			m_dockAreas.erase(it);
		}

		// Handle splitter removal properly
		wxWindow* parent = area->GetParent();
		if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(parent)) {
			wxLogDebug("  -> Parent is a splitter");

			// Get the other window in the splitter
			wxWindow* otherWindow = nullptr;
			if (splitter->GetWindow1() == area) {
				otherWindow = splitter->GetWindow2();
			}
			else if (splitter->GetWindow2() == area) {
				otherWindow = splitter->GetWindow1();
			}

			// First, remove the area from the splitter
			splitter->Unsplit(area);

			// Detach the area from its parent to prevent double deletion
			area->Reparent(this);
			area->Hide();

			// If the splitter now has only one child, we may need to simplify the layout
			if (otherWindow && splitter->GetParent()) {
				wxWindow* splitterParent = splitter->GetParent();

				// If parent is also a splitter, replace this splitter with the remaining window
				if (DockSplitter* parentSplitter = dynamic_cast<DockSplitter*>(splitterParent)) {
					wxLogDebug("  -> Parent is also a splitter, simplifying layout");

					// Reparent the other window to the parent splitter
					otherWindow->Reparent(parentSplitter);

					// Replace this splitter with the other window
					if (parentSplitter->GetWindow1() == splitter) {
						parentSplitter->ReplaceWindow(splitter, otherWindow);
					}
					else if (parentSplitter->GetWindow2() == splitter) {
						parentSplitter->ReplaceWindow(splitter, otherWindow);
					}

					// Now we can safely destroy the empty splitter
					splitter->Destroy();
				}
				else if (splitter == m_rootSplitter && otherWindow) {
					wxLogDebug("  -> This is the root splitter with one child");
					// For root splitter, just leave it with one window
					// The splitter will handle drawing correctly
				}
			}
		}
		else {
			wxLogDebug("  -> Parent is not a splitter");
			// Just detach from parent
			if (parent) {
				if (wxSizer* sizer = parent->GetSizer()) {
					sizer->Detach(area);
				}
			}
		}

		// Update layout
		Layout();

		// Notify
		wxCommandEvent event(EVT_DOCK_AREAS_REMOVED);
		event.SetEventObject(this);
		ProcessWindowEvent(event);

		// Now safely destroy the area
		area->Destroy();
	}

	void DockContainerWidget::removeDockWidget(DockWidget* widget) {
		if (!widget || !widget->dockAreaWidget()) {
			return;
		}

		widget->dockAreaWidget()->removeDockWidget(widget);
	}

	DockArea* DockContainerWidget::dockAreaAt(const wxPoint& globalPos) const {
		wxPoint localPos = ScreenToClient(globalPos);

		for (auto* area : m_dockAreas) {
			wxRect rect = area->GetRect();
			if (rect.Contains(localPos)) {
				return area;
			}
		}

		return nullptr;
	}

	DockArea* DockContainerWidget::dockArea(int index) const {
		if (index >= 0 && index < static_cast<int>(m_dockAreas.size())) {
			return m_dockAreas[index];
		}
		return nullptr;
	}

	void DockContainerWidget::addDockArea(DockArea* dockArea, DockWidgetArea area) {
		wxLogDebug("DockContainerWidget::addDockArea - area: %d, dockArea: %p", area, dockArea);

		if (!dockArea) {
			wxLogDebug("  -> dockArea is null");
			return;
		}

		// Add to list
		m_dockAreas.push_back(dockArea);
		wxLogDebug("  -> Added to list, now have %d areas", (int)m_dockAreas.size());

		// For now, just add to the root splitter
		if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(m_rootSplitter)) {
			wxLogDebug("  -> Got root splitter: %p", splitter);
			if (splitter->GetWindow1() == nullptr) {
				wxLogDebug("  -> Splitter window1 is null, initializing with dock area");
				// Reparent the dock area to the splitter before initializing
				dockArea->Reparent(splitter);
				splitter->Initialize(dockArea);
			}
			else if (splitter->GetWindow2() == nullptr) {
				wxLogDebug("  -> Splitter window1 exists, splitting");
				// Reparent the dock area to the splitter before splitting
				dockArea->Reparent(splitter);
				// Split based on area
				// Ensure minimum sizes
				splitter->GetWindow1()->SetMinSize(wxSize(100, 100));
				dockArea->SetMinSize(wxSize(100, 100));

				if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
					wxLogDebug("    -> Splitting vertically");
					if (area == LeftDockWidgetArea) {
						splitter->SplitVertically(dockArea, splitter->GetWindow1());
					}
					else {
						splitter->SplitVertically(splitter->GetWindow1(), dockArea);
					}
				}
				else {
					wxLogDebug("    -> Splitting horizontally");
					if (area == TopDockWidgetArea) {
						splitter->SplitHorizontally(dockArea, splitter->GetWindow1());
					}
					else {
						splitter->SplitHorizontally(splitter->GetWindow1(), dockArea);
					}
				}
			}
			else {
				wxLogDebug("  -> Both splitter windows occupied, need complex layout");
				// Both windows are occupied, need more complex layout
				// For now, try to find existing dock area in the desired position
				wxWindow* targetWindow = nullptr;

				if (area == LeftDockWidgetArea || area == TopDockWidgetArea) {
					targetWindow = splitter->GetWindow1();
				}
				else {
					targetWindow = splitter->GetWindow2();
				}

				// Check if target is already a splitter
				DockSplitter* targetSplitter = dynamic_cast<DockSplitter*>(targetWindow);
				if (targetSplitter) {
					wxLogDebug("  -> Target is already a splitter, recursing");
					// Recursively add to the sub-splitter
					addDockAreaToSplitter(targetSplitter, dockArea, area);
				}
				else {
					wxLogDebug("  -> Creating new sub-splitter");
					// Create a new sub-splitter
					DockSplitter* newSplitter = new DockSplitter(splitter);

					if (targetWindow) {
						// Reparent the old window to the new splitter
						targetWindow->Reparent(newSplitter);

						// Replace the window in the parent splitter
						splitter->ReplaceWindow(targetWindow, newSplitter);

						// Reparent the dock area to the new splitter
						dockArea->Reparent(newSplitter);

						// Split based on area
						if (area == LeftDockWidgetArea) {
							newSplitter->SplitVertically(dockArea, targetWindow);
						}
						else if (area == RightDockWidgetArea) {
							newSplitter->SplitVertically(targetWindow, dockArea);
						}
						else if (area == TopDockWidgetArea) {
							newSplitter->SplitHorizontally(dockArea, targetWindow);
						}
						else {
							newSplitter->SplitHorizontally(targetWindow, dockArea);
						}
					}
					else {
						wxLogDebug("  -> ERROR: targetWindow is null!");
						// Just add to parent splitter
						dockArea->Reparent(splitter);
					}
				}
			}
		}

		// Notify
		wxCommandEvent event(EVT_DOCK_AREAS_ADDED);
		event.SetEventObject(this);
		ProcessWindowEvent(event);
	}

	void DockContainerWidget::splitDockArea(DockArea* dockArea, DockArea* newDockArea,
		DockWidgetArea area, int splitRatio) {
		// TODO: Implement dock area splitting
		addDockArea(newDockArea, area);
	}

	void DockContainerWidget::setFloatingWidget(FloatingDockContainer* floatingWidget) {
		m_floatingWidget = floatingWidget;
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

	void DockContainerWidget::updateSplitterHandles(wxWindow* splitter) {
		// TODO: Update splitter appearance
	}

	wxWindow* DockContainerWidget::createSplitter(wxOrientation orientation) {
		DockSplitter* splitter = new DockSplitter(this);
		splitter->setOrientation(orientation);
		return splitter;
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

	void DockContainerWidget::onSize(wxSizeEvent& event) {
		event.Skip();
	}

	void DockContainerWidget::onDockAreaDestroyed(wxWindowDestroyEvent& event) {
		// Handle dock area destruction
	}

	void DockContainerWidget::dropFloatingWidget(FloatingDockContainer* floatingWidget, const wxPoint& targetPos) {
		// TODO: Implement floating widget dropping
	}

	void DockContainerWidget::dropDockArea(DockArea* dockArea, DockWidgetArea area) {
		addDockArea(dockArea, area);
	}

	void DockContainerWidget::addDockAreaToContainer(DockWidgetArea area, DockArea* dockArea) {
		addDockArea(dockArea, area);
	}

	void DockContainerWidget::addDockAreaToSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area) {
		if (!splitter || !dockArea) {
			return;
		}

		wxLogDebug("DockContainerWidget::addDockAreaToSplitter - area: %d", area);

		if (splitter->GetWindow1() == nullptr) {
			wxLogDebug("  -> Window1 is null, initializing");
			dockArea->Reparent(splitter);
			splitter->Initialize(dockArea);
		}
		else if (splitter->GetWindow2() == nullptr) {
			wxLogDebug("  -> Window2 is null, splitting");
			dockArea->Reparent(splitter);
			// Ensure minimum sizes
			splitter->GetWindow1()->SetMinSize(wxSize(100, 100));
			dockArea->SetMinSize(wxSize(100, 100));

			if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
				if (area == LeftDockWidgetArea) {
					splitter->SplitVertically(dockArea, splitter->GetWindow1());
				}
				else {
					splitter->SplitVertically(splitter->GetWindow1(), dockArea);
				}
			}
			else {
				if (area == TopDockWidgetArea) {
					splitter->SplitHorizontally(dockArea, splitter->GetWindow1());
				}
				else {
					splitter->SplitHorizontally(splitter->GetWindow1(), dockArea);
				}
			}
		}
		else {
			wxLogDebug("  -> Both windows occupied, creating sub-splitter");
			// Both windows occupied, same logic as in addDockArea
			wxWindow* targetWindow = nullptr;

			if (area == LeftDockWidgetArea || area == TopDockWidgetArea) {
				targetWindow = splitter->GetWindow1();
			}
			else {
				targetWindow = splitter->GetWindow2();
			}

			if (targetWindow) {
				DockSplitter* newSplitter = new DockSplitter(splitter);
				targetWindow->Reparent(newSplitter);
				splitter->ReplaceWindow(targetWindow, newSplitter);
				dockArea->Reparent(newSplitter);

				// Ensure minimum size for both windows
				dockArea->SetMinSize(wxSize(100, 100));
				targetWindow->SetMinSize(wxSize(100, 100));

				if (area == LeftDockWidgetArea) {
					newSplitter->SplitVertically(dockArea, targetWindow);
				}
				else if (area == RightDockWidgetArea) {
					newSplitter->SplitVertically(targetWindow, dockArea);
				}
				else if (area == TopDockWidgetArea) {
					newSplitter->SplitHorizontally(dockArea, targetWindow);
				}
				else {
					newSplitter->SplitHorizontally(targetWindow, dockArea);
				}

				// Set splitter properties
				newSplitter->SetSashGravity(0.5);
				newSplitter->SetMinimumPaneSize(50);
			}
		}
	}

	DockSplitter* DockContainerWidget::newSplitter(wxOrientation orientation) {
		return new DockSplitter(this);
	}

	// DockSplitter implementation
	DockSplitter::DockSplitter(wxWindow* parent)
		: wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
			wxSP_3D | wxSP_LIVE_UPDATE)
		, m_orientation(wxHORIZONTAL)
	{
		SetSashGravity(0.5);
		SetMinimumPaneSize(50);
	}

	DockSplitter::~DockSplitter() {
	}

	void DockSplitter::setOrientation(wxOrientation orientation) {
		m_orientation = orientation;
	}

	void DockSplitter::insertWidget(int index, wxWindow* widget, bool stretch) {
		if (index < 0 || index > widgetCount()) {
			addWidget(widget, stretch);
			return;
		}

		// Ensure widget has this splitter as parent
		if (widget->GetParent() != this) {
			widget->Reparent(this);
		}

		m_widgets.insert(m_widgets.begin() + index, widget);
		updateSplitter();
	}

	void DockSplitter::addWidget(wxWindow* widget, bool stretch) {
		// Ensure widget has this splitter as parent
		if (widget->GetParent() != this) {
			widget->Reparent(this);
		}

		m_widgets.push_back(widget);
		updateSplitter();
	}

	wxWindow* DockSplitter::replaceWidget(wxWindow* from, wxWindow* to) {
		// Ensure 'to' widget has this splitter as parent
		if (to && to->GetParent() != this) {
			to->Reparent(this);
		}

		ReplaceWindow(from, to);

		auto it = std::find(m_widgets.begin(), m_widgets.end(), from);
		if (it != m_widgets.end()) {
			*it = to;
		}

		return from;
	}

	wxWindow* DockSplitter::widget(int index) const {
		if (index >= 0 && index < static_cast<int>(m_widgets.size())) {
			return m_widgets[index];
		}
		return nullptr;
	}

	int DockSplitter::indexOf(wxWindow* widget) const {
		auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);
		if (it != m_widgets.end()) {
			return std::distance(m_widgets.begin(), it);
		}
		return -1;
	}

	int DockSplitter::widgetCount() const {
		return m_widgets.size();
	}

	bool DockSplitter::hasVisibleContent() const {
		for (auto* widget : m_widgets) {
			if (widget && widget->IsShown()) {
				return true;
			}
		}
		return false;
	}

	void DockSplitter::setSizes(const std::vector<int>& sizes) {
		m_sizes = sizes;

		if (IsSplit() && !sizes.empty()) {
			SetSashPosition(sizes[0]);
		}
	}

	std::vector<int> DockSplitter::sizes() const {
		std::vector<int> result;

		if (IsSplit()) {
			result.push_back(GetSashPosition());
			result.push_back(GetSize().GetWidth() - GetSashPosition() - GetSashSize());
		}

		return result;
	}

	void DockSplitter::OnSplitterSashPosChanging(wxSplitterEvent& event) {
		// Allow sash position changes
	}

	void DockSplitter::OnSplitterSashPosChanged(wxSplitterEvent& event) {
		// Update sizes
		m_sizes = sizes();
	}

	void DockSplitter::updateSplitter() {
		if (m_widgets.size() >= 2 && !IsSplit()) {
			// Ensure both widgets have this splitter as parent
			if (m_widgets[0]->GetParent() != this) {
				m_widgets[0]->Reparent(this);
			}
			if (m_widgets[1]->GetParent() != this) {
				m_widgets[1]->Reparent(this);
			}

			if (m_orientation == wxVERTICAL) {
				SplitVertically(m_widgets[0], m_widgets[1]);
			}
			else {
				SplitHorizontally(m_widgets[0], m_widgets[1]);
			}
		}
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
} // namespace ads
=======

// Define custom events
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_ADDED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_REMOVED(wxNewEventType());

// Event table
wxBEGIN_EVENT_TABLE(DockContainerWidget, wxPanel)
    EVT_SIZE(DockContainerWidget::onSize)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(DockSplitter, wxSplitterWindow)
    EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, DockSplitter::OnSplitterSashPosChanging)
    EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, DockSplitter::OnSplitterSashPosChanged)
wxEND_EVENT_TABLE()

// Private implementation
class DockContainerWidget::Private {
public:
    Private(DockContainerWidget* parent) : q(parent) {}
    
    DockContainerWidget* q;
    DockSplitter* rootSplitter = nullptr;
    wxWindow* centralWidget = nullptr;
};

// DockContainerWidget implementation
DockContainerWidget::DockContainerWidget(DockManager* dockManager, wxWindow* parent)
    : wxPanel(parent)
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockManager)
    , m_floatingWidget(nullptr)
    , m_lastAddedArea(nullptr)
{
    // Create layout
    m_layout = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_layout);
    
    // Create root splitter
    m_rootSplitter = new DockSplitter(this);
    m_layout->Add(m_rootSplitter, 1, wxEXPAND);
}

DockContainerWidget::~DockContainerWidget() {
    // Clear all dock areas first to ensure proper cleanup order
    m_dockAreas.clear();
    
    // The splitter and other child windows will be destroyed automatically
    // by wxWidgets parent-child mechanism
}

DockArea* DockContainerWidget::addDockWidget(DockWidgetArea area, DockWidget* dockWidget, 
                                            DockArea* targetDockArea, int index) {
    wxLogDebug("DockContainerWidget::addDockWidget - area: %d, widget: %p, targetArea: %p", 
        area, dockWidget, targetDockArea);
    wxLogDebug("  -> Current dock areas count: %d", (int)m_dockAreas.size());
    
    if (!dockWidget) {
        wxLogDebug("  -> dockWidget is null");
        return nullptr;
    }
    
    // If we have a target dock area, add to it
    if (targetDockArea) {
        wxLogDebug("  -> Adding to existing target area");
        targetDockArea->addDockWidget(dockWidget);
        return targetDockArea;
    }
    
    // If we already have many areas, add as tab to avoid complex layouts
    if (m_dockAreas.size() >= 4) {
        wxLogDebug("  -> Too many areas (%d), adding as tab", (int)m_dockAreas.size());
        // Find the best area to add to
        DockArea* bestArea = nullptr;
        
        // Try to find an area in the same general position
        for (auto* existingArea : m_dockAreas) {
            if (existingArea && existingArea->GetParent()) {
                // For now, just use the first valid area
                bestArea = existingArea;
                break;
            }
        }
        
        if (bestArea) {
            bestArea->addDockWidget(dockWidget);
            return bestArea;
        }
    }
    
    // Create new dock area
    wxLogDebug("  -> Creating new dock area");
    DockArea* newDockArea = new DockArea(m_dockManager, this);
    newDockArea->addDockWidget(dockWidget);
    
    // Add dock area to container
    addDockArea(newDockArea, area);
    
    m_lastAddedArea = newDockArea;
    
    return newDockArea;
}

void DockContainerWidget::removeDockArea(DockArea* area) {
    if (!area) {
        return;
    }
    
    wxLogDebug("DockContainerWidget::removeDockArea - removing area %p", area);
    
    // Remove from list
    auto it = std::find(m_dockAreas.begin(), m_dockAreas.end(), area);
    if (it != m_dockAreas.end()) {
        m_dockAreas.erase(it);
    }
    
    // Handle splitter removal properly
    wxWindow* parent = area->GetParent();
    if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(parent)) {
        wxLogDebug("  -> Parent is a splitter");
        
        // Get the other window in the splitter
        wxWindow* otherWindow = nullptr;
        if (splitter->GetWindow1() == area) {
            otherWindow = splitter->GetWindow2();
        } else if (splitter->GetWindow2() == area) {
            otherWindow = splitter->GetWindow1();
        }
        
        // First, remove the area from the splitter
        bool wasUnsplit = splitter->Unsplit(area);
        if (!wasUnsplit) {
            wxLogDebug("  -> Failed to unsplit area!");
        }
        
        // Detach the area from its parent to prevent double deletion
        area->Reparent(this);
        area->Hide();
        
        // If the splitter now has only one child, we may need to simplify the layout
        if (otherWindow && splitter->GetParent()) {
            wxWindow* splitterParent = splitter->GetParent();
            
            // If parent is also a splitter, replace this splitter with the remaining window
            if (DockSplitter* parentSplitter = dynamic_cast<DockSplitter*>(splitterParent)) {
                wxLogDebug("  -> Parent is also a splitter, simplifying layout");
                
                // Reparent the other window to the parent splitter
                otherWindow->Reparent(parentSplitter);
                
                // Replace this splitter with the other window
                if (parentSplitter->GetWindow1() == splitter) {
                    parentSplitter->ReplaceWindow(splitter, otherWindow);
                } else if (parentSplitter->GetWindow2() == splitter) {
                    parentSplitter->ReplaceWindow(splitter, otherWindow);
                }
                
                // Now we can safely destroy the empty splitter
                splitter->Destroy();
            } else if (splitter == m_rootSplitter) {
                wxLogDebug("  -> This is the root splitter with one child");
                if (!otherWindow) {
                    wxLogDebug("    -> No other window in root splitter!");
                    // Root splitter is now empty, this shouldn't happen
                    // but if it does, we need to handle it
                }
                // For root splitter, just leave it with one window
                // The splitter will handle drawing correctly
            }
        }
    } else {
        wxLogDebug("  -> Parent is not a splitter");
        // Just detach from parent
        if (parent) {
            if (wxSizer* sizer = parent->GetSizer()) {
                sizer->Detach(area);
            }
        }
    }
    
    // Update layout
    Layout();
    
    // Notify
    wxCommandEvent event(EVT_DOCK_AREAS_REMOVED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
    
    // Now safely destroy the area
    area->Destroy();
}

void DockContainerWidget::removeDockWidget(DockWidget* widget) {
    if (!widget || !widget->dockAreaWidget()) {
        return;
    }
    
    widget->dockAreaWidget()->removeDockWidget(widget);
}

DockArea* DockContainerWidget::dockAreaAt(const wxPoint& globalPos) const {
    wxPoint localPos = ScreenToClient(globalPos);
    
    for (auto* area : m_dockAreas) {
        wxRect rect = area->GetRect();
        if (rect.Contains(localPos)) {
            return area;
        }
    }
    
    return nullptr;
}

DockArea* DockContainerWidget::dockArea(int index) const {
    if (index >= 0 && index < static_cast<int>(m_dockAreas.size())) {
        return m_dockAreas[index];
    }
    return nullptr;
}

void DockContainerWidget::addDockArea(DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::addDockArea - area: %d, dockArea: %p", area, dockArea);
    
    if (!dockArea) {
        wxLogDebug("  -> dockArea is null");
        return;
    }
    
    // Add to list
    m_dockAreas.push_back(dockArea);
    wxLogDebug("  -> Added to list, now have %d areas", (int)m_dockAreas.size());
    
    // For now, just add to the root splitter
    if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(m_rootSplitter)) {
        wxLogDebug("  -> Got root splitter: %p", splitter);
        wxWindow* window1 = splitter->GetWindow1();
        wxWindow* window2 = splitter->GetWindow2();
        
        wxLogDebug("  -> Splitter windows: window1=%p, window2=%p", window1, window2);
        
        if (window1 == nullptr && window2 == nullptr) {
            wxLogDebug("  -> Both splitter windows are null, initializing with dock area");
            // Reparent the dock area to the splitter before initializing
            dockArea->Reparent(splitter);
            splitter->Initialize(dockArea);
        } else if (window1 && window2 == nullptr) {
            wxLogDebug("  -> Splitter window1 exists, window2 is null, splitting");
            // Reparent the dock area to the splitter before splitting
            dockArea->Reparent(splitter);
            // Split based on area
            // Ensure minimum sizes
            window1->SetMinSize(wxSize(100, 100));
            dockArea->SetMinSize(wxSize(100, 100));
            
            if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
                wxLogDebug("    -> Splitting vertically");
                if (area == LeftDockWidgetArea) {
                    splitter->SplitVertically(dockArea, window1);
                } else {
                    splitter->SplitVertically(window1, dockArea);
                }
            } else {
                wxLogDebug("    -> Splitting horizontally");
                if (area == TopDockWidgetArea) {
                    splitter->SplitHorizontally(dockArea, window1);
                } else {
                    splitter->SplitHorizontally(window1, dockArea);
                }
            }
        } else if (window2 && window1 == nullptr) {
            wxLogDebug("  -> Splitter window2 exists, window1 is null, splitting");
            // This is unusual but can happen after unsplitting
            // Reparent the dock area to the splitter before splitting
            dockArea->Reparent(splitter);
            // Ensure minimum sizes
            window2->SetMinSize(wxSize(100, 100));
            dockArea->SetMinSize(wxSize(100, 100));
            
            if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
                wxLogDebug("    -> Splitting vertically");
                if (area == LeftDockWidgetArea) {
                    splitter->SplitVertically(dockArea, window2);
                } else {
                    splitter->SplitVertically(window2, dockArea);
                }
            } else {
                wxLogDebug("    -> Splitting horizontally");
                if (area == TopDockWidgetArea) {
                    splitter->SplitHorizontally(dockArea, window2);
                } else {
                    splitter->SplitHorizontally(window2, dockArea);
                }
            }
        } else {
            wxLogDebug("  -> Both splitter windows occupied, need complex layout");
            // Both windows are occupied, need more complex layout
            // For now, try to find existing dock area in the desired position
            wxWindow* targetWindow = nullptr;
            
            if (area == LeftDockWidgetArea || area == TopDockWidgetArea) {
                targetWindow = splitter->GetWindow1();
            } else {
                targetWindow = splitter->GetWindow2();
            }
            
            // Check if target is already a splitter
            DockSplitter* targetSplitter = dynamic_cast<DockSplitter*>(targetWindow);
            if (targetSplitter) {
                wxLogDebug("  -> Target is already a splitter, recursing");
                // Recursively add to the sub-splitter
                addDockAreaToSplitter(targetSplitter, dockArea, area);
            } else {
                wxLogDebug("  -> Creating new sub-splitter");
                // Create a new sub-splitter
                DockSplitter* newSplitter = new DockSplitter(splitter);
                
                if (targetWindow) {
                    // Reparent the old window to the new splitter
                    targetWindow->Reparent(newSplitter);
                    
                    // Replace the window in the parent splitter
                    splitter->ReplaceWindow(targetWindow, newSplitter);
                    
                    // Reparent the dock area to the new splitter
                    dockArea->Reparent(newSplitter);
                    
                    // Split based on area
                    if (area == LeftDockWidgetArea) {
                        newSplitter->SplitVertically(dockArea, targetWindow);
                    } else if (area == RightDockWidgetArea) {
                        newSplitter->SplitVertically(targetWindow, dockArea);
                    } else if (area == TopDockWidgetArea) {
                        newSplitter->SplitHorizontally(dockArea, targetWindow);
                    } else {
                        newSplitter->SplitHorizontally(targetWindow, dockArea);
                    }
                } else {
                    wxLogDebug("  -> ERROR: targetWindow is null!");
                    // Just add to parent splitter
                    dockArea->Reparent(splitter);
                }
            }
        }
    }
    
    // Notify
    wxCommandEvent event(EVT_DOCK_AREAS_ADDED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}

void DockContainerWidget::splitDockArea(DockArea* dockArea, DockArea* newDockArea, 
                                       DockWidgetArea area, int splitRatio) {
    // TODO: Implement dock area splitting
    addDockArea(newDockArea, area);
}

void DockContainerWidget::setFloatingWidget(FloatingDockContainer* floatingWidget) {
    m_floatingWidget = floatingWidget;
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

void DockContainerWidget::updateSplitterHandles(wxWindow* splitter) {
    // TODO: Update splitter appearance
}

wxWindow* DockContainerWidget::createSplitter(wxOrientation orientation) {
    DockSplitter* splitter = new DockSplitter(this);
    splitter->setOrientation(orientation);
    return splitter;
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

void DockContainerWidget::onSize(wxSizeEvent& event) {
    event.Skip();
}

void DockContainerWidget::onDockAreaDestroyed(wxWindowDestroyEvent& event) {
    // Handle dock area destruction
}

void DockContainerWidget::dropFloatingWidget(FloatingDockContainer* floatingWidget, const wxPoint& targetPos) {
    // TODO: Implement floating widget dropping
}

void DockContainerWidget::dropDockArea(DockArea* dockArea, DockWidgetArea area) {
    addDockArea(dockArea, area);
}

void DockContainerWidget::addDockAreaToContainer(DockWidgetArea area, DockArea* dockArea) {
    addDockArea(dockArea, area);
}

void DockContainerWidget::addDockAreaToSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area) {
    if (!splitter || !dockArea) {
        return;
    }
    
    wxLogDebug("DockContainerWidget::addDockAreaToSplitter - area: %d", area);
    
    if (splitter->GetWindow1() == nullptr) {
        wxLogDebug("  -> Window1 is null, initializing");
        dockArea->Reparent(splitter);
        splitter->Initialize(dockArea);
    } else if (splitter->GetWindow2() == nullptr) {
        wxLogDebug("  -> Window2 is null, splitting");
        dockArea->Reparent(splitter);
        // Ensure minimum sizes
        splitter->GetWindow1()->SetMinSize(wxSize(100, 100));
        dockArea->SetMinSize(wxSize(100, 100));
        
        if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
            if (area == LeftDockWidgetArea) {
                splitter->SplitVertically(dockArea, splitter->GetWindow1());
            } else {
                splitter->SplitVertically(splitter->GetWindow1(), dockArea);
            }
        } else {
            if (area == TopDockWidgetArea) {
                splitter->SplitHorizontally(dockArea, splitter->GetWindow1());
            } else {
                splitter->SplitHorizontally(splitter->GetWindow1(), dockArea);
            }
        }
    } else {
        wxLogDebug("  -> Both windows occupied, creating sub-splitter");
        // Both windows occupied, same logic as in addDockArea
        wxWindow* targetWindow = nullptr;
        
        if (area == LeftDockWidgetArea || area == TopDockWidgetArea) {
            targetWindow = splitter->GetWindow1();
        } else {
            targetWindow = splitter->GetWindow2();
        }
        
        if (targetWindow) {
            DockSplitter* newSplitter = new DockSplitter(splitter);
            targetWindow->Reparent(newSplitter);
            splitter->ReplaceWindow(targetWindow, newSplitter);
            dockArea->Reparent(newSplitter);
            
            // Ensure minimum size for both windows
            dockArea->SetMinSize(wxSize(100, 100));
            targetWindow->SetMinSize(wxSize(100, 100));
            
            if (area == LeftDockWidgetArea) {
                newSplitter->SplitVertically(dockArea, targetWindow);
            } else if (area == RightDockWidgetArea) {
                newSplitter->SplitVertically(targetWindow, dockArea);
            } else if (area == TopDockWidgetArea) {
                newSplitter->SplitHorizontally(dockArea, targetWindow);
            } else {
                newSplitter->SplitHorizontally(targetWindow, dockArea);
            }
            
            // Set splitter properties
            newSplitter->SetSashGravity(0.5);
            newSplitter->SetMinimumPaneSize(50);
        }
    }
}

DockSplitter* DockContainerWidget::newSplitter(wxOrientation orientation) {
    return new DockSplitter(this);
}

// DockSplitter implementation
DockSplitter::DockSplitter(wxWindow* parent)
    : wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                      wxSP_3D | wxSP_LIVE_UPDATE)
    , m_orientation(wxHORIZONTAL)
{
    SetSashGravity(0.5);
    SetMinimumPaneSize(50);
}

DockSplitter::~DockSplitter() {
}

void DockSplitter::setOrientation(wxOrientation orientation) {
    m_orientation = orientation;
}

void DockSplitter::insertWidget(int index, wxWindow* widget, bool stretch) {
    if (index < 0 || index > widgetCount()) {
        addWidget(widget, stretch);
        return;
    }
    
    // Ensure widget has this splitter as parent
    if (widget->GetParent() != this) {
        widget->Reparent(this);
    }
    
    m_widgets.insert(m_widgets.begin() + index, widget);
    updateSplitter();
}

void DockSplitter::addWidget(wxWindow* widget, bool stretch) {
    // Ensure widget has this splitter as parent
    if (widget->GetParent() != this) {
        widget->Reparent(this);
    }
    
    m_widgets.push_back(widget);
    updateSplitter();
}

wxWindow* DockSplitter::replaceWidget(wxWindow* from, wxWindow* to) {
    // Ensure 'to' widget has this splitter as parent
    if (to && to->GetParent() != this) {
        to->Reparent(this);
    }
    
    ReplaceWindow(from, to);
    
    auto it = std::find(m_widgets.begin(), m_widgets.end(), from);
    if (it != m_widgets.end()) {
        *it = to;
    }
    
    return from;
}

wxWindow* DockSplitter::widget(int index) const {
    if (index >= 0 && index < static_cast<int>(m_widgets.size())) {
        return m_widgets[index];
    }
    return nullptr;
}

int DockSplitter::indexOf(wxWindow* widget) const {
    auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);
    if (it != m_widgets.end()) {
        return std::distance(m_widgets.begin(), it);
    }
    return -1;
}

int DockSplitter::widgetCount() const {
    return m_widgets.size();
}

bool DockSplitter::hasVisibleContent() const {
    for (auto* widget : m_widgets) {
        if (widget && widget->IsShown()) {
            return true;
        }
    }
    return false;
}

void DockSplitter::setSizes(const std::vector<int>& sizes) {
    m_sizes = sizes;
    
    if (IsSplit() && !sizes.empty()) {
        SetSashPosition(sizes[0]);
    }
}

std::vector<int> DockSplitter::sizes() const {
    std::vector<int> result;
    
    if (IsSplit()) {
        result.push_back(GetSashPosition());
        result.push_back(GetSize().GetWidth() - GetSashPosition() - GetSashSize());
    }
    
    return result;
}

void DockSplitter::OnSplitterSashPosChanging(wxSplitterEvent& event) {
    // Allow sash position changes
}

void DockSplitter::OnSplitterSashPosChanged(wxSplitterEvent& event) {
    // Update sizes
    m_sizes = sizes();
}

void DockSplitter::updateSplitter() {
    if (m_widgets.size() >= 2 && !IsSplit()) {
        // Ensure both widgets have this splitter as parent
        if (m_widgets[0]->GetParent() != this) {
            m_widgets[0]->Reparent(this);
        }
        if (m_widgets[1]->GetParent() != this) {
            m_widgets[1]->Reparent(this);
        }
        
        if (m_orientation == wxVERTICAL) {
            SplitVertically(m_widgets[0], m_widgets[1]);
        } else {
            SplitHorizontally(m_widgets[0], m_widgets[1]);
        }
    }
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

} // namespace ads
<<<<<<< Current (Your changes)
<<<<<<< Current (Your changes)
>>>>>>> Incoming (Background Agent changes)
=======
>>>>>>> Incoming (Background Agent changes)
=======
>>>>>>> Incoming (Background Agent changes)
