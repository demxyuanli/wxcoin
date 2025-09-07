#pragma once

#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include <wx/wx.h>

namespace ads {
	/**
	 * @brief Helper class to integrate the Qt-Advanced-Docking-System style docking with FlatFrame
	 *
	 * This class provides an easy way to replace the ModernDockAdapter with our docking system.
	 *
	 * Example usage in FlatFrame::createPanels():
	 *
	 * @code
	 * // Replace ModernDockAdapter with our docking system
	 * auto* dockManager = new ads::DockManager(this);
	 * mainSizer->Add(dockManager->containerWidget(), 1, wxEXPAND | wxALL, 2);
	 *
	 * // Create dock widgets
	 * auto* treeWidget = new ads::DockWidget("Object Tree");
	 * treeWidget->setWidget(m_objectTreePanel);
	 * treeWidget->setIcon(SVG_ICON("tree", wxSize(16, 16)));
	 *
	 * auto* propWidget = new ads::DockWidget("Properties");
	 * propWidget->setWidget(m_propertyPanel);
	 * propWidget->setIcon(SVG_ICON("properties", wxSize(16, 16)));
	 *
	 * auto* canvasWidget = new ads::DockWidget("3D View");
	 * canvasWidget->setWidget(m_canvas);
	 * canvasWidget->setIcon(SVG_ICON("view3d", wxSize(16, 16)));
	 * canvasWidget->setFeature(ads::DockWidgetClosable, false); // Main view can't be closed
	 *
	 * auto* messageWidget = new ads::DockWidget("Message");
	 * messageWidget->setWidget(messageText);
	 *
	 * auto* perfWidget = new ads::DockWidget("Performance");
	 * perfWidget->setWidget(perfPage);
	 *
	 * // Add dock widgets to the manager
	 * dockManager->addDockWidget(ads::LeftDockWidgetArea, treeWidget);
	 * dockManager->addDockWidget(ads::LeftDockWidgetArea, propWidget,
	 *                           treeWidget->dockAreaWidget()); // Tab with tree
	 * dockManager->addDockWidget(ads::CenterDockWidgetArea, canvasWidget);
	 * dockManager->addDockWidget(ads::BottomDockWidgetArea, messageWidget);
	 * dockManager->addDockWidget(ads::BottomDockWidgetArea, perfWidget,
	 *                           messageWidget->dockAreaWidget()); // Tab with message
	 *
	 * // Configure features
	 * dockManager->setConfigFlag(ads::OpaqueSplitterResize, true);
	 * dockManager->setConfigFlag(ads::FocusHighlighting, true);
	 * dockManager->setConfigFlag(ads::DockAreaHasCloseButton, true);
	 * dockManager->setConfigFlag(ads::AllTabsHaveCloseButton, true);
	 *
	 * // Save/restore layout
	 * wxString layoutData;
	 * dockManager->saveState(layoutData);
	 * // ... save layoutData to config
	 *
	 * // Later: restore layout
	 * wxString savedLayout = config.GetLayoutData();
	 * dockManager->restoreState(savedLayout);
	 * @endcode
	 */
	class DockingIntegration {
	public:
		/**
		 * @brief Create a standard CAD application layout
		 *
		 * @param parent Parent window
		 * @param canvas Main 3D canvas widget
		 * @param objectTree Object tree panel
		 * @param properties Properties panel
		 * @param messageOutput Message output widget
		 * @param performancePanel Performance panel (optional)
		 * @return Configured DockManager instance
		 */
		static DockManager* CreateStandardCADLayout(
			wxWindow* parent,
			wxWindow* canvas,
			wxWindow* objectTree,
			wxWindow* properties,
			wxWindow* messageOutput,
			wxWindow* performancePanel = nullptr);

		/**
		 * @brief Create example dock widgets for testing
		 *
		 * @param dockManager The dock manager to add widgets to
		 */
		static void CreateExampleDockWidgets(DockManager* dockManager);

		/**
		 * @brief Set up menu items for dock widget visibility
		 *
		 * @param menu The view menu to add items to
		 * @param dockManager The dock manager
		 */
		static void SetupViewMenu(wxMenu* menu, DockManager* dockManager);
	};
} // namespace ads
