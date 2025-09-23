#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/timer.h>
#include <vector>
#include <memory>
#include <map>

namespace ads {
	// Forward declarations
	class DockManager;
	class DockWidget;
	class DockArea;
	class DockContainerWidget;
	class AutoHideTab;

	/**
	 * @brief Side bar positions for auto-hide functionality
	 */
	enum AutoHideSideBarLocation {
		SideBarLeft = 0,
		SideBarRight = 1,
		SideBarTop = 2,
		SideBarBottom = 3,
		SideBarCount = 4
	};

	/**
	 * @brief Tab representing an auto-hidden dock widget
	 */
	class AutoHideTab : public wxPanel {
	public:
		AutoHideTab(DockWidget* dockWidget, AutoHideSideBarLocation location);
		virtual ~AutoHideTab();

		DockWidget* dockWidget() const { return m_dockWidget; }
		void updateIcon();
		void updateTitle();
		bool isActiveTab() const { return m_isActive; }
		void setActive(bool active);

	protected:
		void OnPaint(wxPaintEvent& event);
		void OnMouseEnter(wxMouseEvent& event);
		void OnMouseLeave(wxMouseEvent& event);
		void OnLeftDown(wxMouseEvent& event);

	private:
		DockWidget* m_dockWidget;
		AutoHideSideBarLocation m_location;
		bool m_isActive;
		bool m_isHovered;
		wxBitmap m_icon;

		// Graphics context caching for performance optimization
		wxGraphicsContext* m_cachedGraphicsContext;
		wxSize m_lastPaintSize;
		bool m_needsRedraw;

		wxDECLARE_EVENT_TABLE();
	};

	/**
	 * @brief Side bar containing auto-hide tabs
	 */
	class AutoHideSideBar : public wxPanel {
	public:
		AutoHideSideBar(DockContainerWidget* container, AutoHideSideBarLocation location);
		virtual ~AutoHideSideBar();

		void addAutoHideWidget(DockWidget* dockWidget);
		void removeAutoHideWidget(DockWidget* dockWidget);

		AutoHideSideBarLocation location() const { return m_location; }
		int tabCount() const { return m_tabs.size(); }
		AutoHideTab* tab(int index) const;
		DockContainerWidget* getContainer() const { return m_container; }

		void showDockWidget(DockWidget* dockWidget);
		void hideDockWidget(DockWidget* dockWidget);

		bool hasVisibleTabs() const;

	protected:
		void OnSize(wxSizeEvent& event);
		void updateLayout();

	private:
		DockContainerWidget* m_container;
		AutoHideSideBarLocation m_location;
		std::vector<AutoHideTab*> m_tabs;
		wxBoxSizer* m_sizer;

		wxDECLARE_EVENT_TABLE();
	};

	/**
	 * @brief Container for auto-hidden dock widget content
	 */
	class AutoHideDockContainer : public wxPanel {
	public:
		AutoHideDockContainer(DockWidget* dockWidget, AutoHideSideBarLocation location, DockContainerWidget* parent);
		virtual ~AutoHideDockContainer();

		DockWidget* dockWidget() const { return m_dockWidget; }
		AutoHideSideBarLocation sideBarLocation() const { return m_sideBarLocation; }

		void slideIn();
		void slideOut();
		bool isAnimating() const { return m_isAnimating; }

		void setSize(const wxSize& size);
		wxSize autoHideSize() const { return m_size; }

	protected:
		void OnPaint(wxPaintEvent& event);
		void OnTimer(wxTimerEvent& event);
		void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
		void OnKillFocus(wxFocusEvent& event);
		void OnPinButtonClick(wxCommandEvent& event);

	private:
		DockWidget* m_dockWidget;
		AutoHideSideBarLocation m_sideBarLocation;
		DockContainerWidget* m_container;

		wxTimer m_animationTimer;
		bool m_isAnimating;
		bool m_isVisible;
		int m_animationProgress; // 0-100
		wxSize m_size;

		void updateAnimation();
		wxRect calculateGeometry(int progress);

		wxDECLARE_EVENT_TABLE();
	};

	/**
	 * @brief Manager for auto-hide functionality
	 */
	class AutoHideManager {
	public:
		AutoHideManager(DockContainerWidget* container);
		~AutoHideManager();

		// Pin/unpin dock widgets
		void addAutoHideWidget(DockWidget* dockWidget, AutoHideSideBarLocation location);
		void removeAutoHideWidget(DockWidget* dockWidget);
		void restoreDockWidget(DockWidget* dockWidget);

		// Side bar access
		AutoHideSideBar* sideBar(AutoHideSideBarLocation location) const;

		// Container management
		void showAutoHideWidget(DockWidget* dockWidget);
		void hideAutoHideWidget(DockWidget* dockWidget);
		AutoHideDockContainer* autoHideContainer(DockWidget* dockWidget) const;

		// State
		bool hasAutoHideWidgets() const;
		std::vector<DockWidget*> autoHideWidgets() const;

		// Serialization
		void saveState(wxString& xmlData) const;
		bool restoreState(const wxString& xmlData);

	private:
		DockContainerWidget* m_container;
		AutoHideSideBar* m_sideBars[SideBarCount];
		std::map<DockWidget*, AutoHideDockContainer*> m_autoHideContainers;
		AutoHideDockContainer* m_activeContainer;

		void createSideBars();
		void updateSideBarVisibility();
	};
} // namespace ads
