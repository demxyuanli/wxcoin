#pragma once

#include <wx/wx.h>
#include <wx/frame.h>
#include <vector>

#ifndef wxByte
typedef unsigned char wxByte;
#endif
#include <memory>

// Include DockManager.h to get enum definitions
#include "DockManager.h"

namespace ads {
	// Forward declarations
	class DockWidget;
	class DockArea;
	class DockContainerWidget;
	class FloatingDragPreview;

<<<<<<< Current (Your changes)
	/**
	 * @brief Container for floating dock widgets
	 */
	class FloatingDockContainer : public wxFrame {
	public:
		// Internal state - moved here to be available for method declarations
		enum eDragState {
			DraggingInactive,
			DraggingMousePressed,
			DraggingTab,
			DraggingFloatingWidget
		};

		FloatingDockContainer(DockManager* dockManager);
		FloatingDockContainer(DockArea* dockArea);
		FloatingDockContainer(DockWidget* dockWidget);
		virtual ~FloatingDockContainer();

		// Dock container access
		DockContainerWidget* dockContainer() const { return m_dockContainer; }

		// Widget management
		void addDockWidget(DockWidget* dockWidget);
		void removeDockWidget(DockWidget* dockWidget);

		// State
		bool isClosable() const;
		bool hasTopLevelDockWidget() const;
		DockWidget* topLevelDockWidget() const;
		std::vector<DockWidget*> dockWidgets() const;

		// Title handling
		void updateWindowTitle();

		// Drag & drop
		void startDragging(const wxPoint& dragOffset);

		// Native window handling
		void setNativeTitleBar(bool native);
		bool hasNativeTitleBar() const { return m_hasNativeTitleBar; }

		// State persistence
		void saveState(wxString& xmlData) const;
		bool restoreState(const wxString& xmlData);

		// Dragging
		void startFloating(const wxPoint& dragStartPos, const wxSize& size,
			eDragState dragState, wxWindow* mouseEventHandler);
		void moveFloating();
		void finishDragging();

		// Testing
		bool isInTitleBar(const wxPoint& pos) const;

		// Events
		static wxEventTypeTag<wxCommandEvent> EVT_FLOATING_CONTAINER_CLOSING;
		static wxEventTypeTag<wxCommandEvent> EVT_FLOATING_CONTAINER_CLOSED;

	protected:
		// Event handlers
		void onClose(wxCloseEvent& event);
		void onMouseLeftDown(wxMouseEvent& event);
		void onMouseLeftUp(wxMouseEvent& event);
		void onMouseMove(wxMouseEvent& event);
		void onMouseDoubleClick(wxMouseEvent& event);
		void onNonClientHitTest(wxMouseEvent& event);
		void onMaximize(wxMaximizeEvent& event);

		// Internal methods
		void setupCustomTitleBar();
		bool testConfigFlag(DockManagerFeature flag) const;

	private:
		// Private implementation
		class Private;
		std::unique_ptr<Private> d;

		// Member variables
		DockManager* m_dockManager;
		DockContainerWidget* m_dockContainer;
		bool m_hasNativeTitleBar;
		eDragState m_dragState;
		wxPoint m_dragStartPos;
		FloatingDragPreview* m_dragPreview;

		// Initialization
		void init();

		friend class DockManager;
		friend class DockArea;
		friend class DockWidget;

		wxDECLARE_EVENT_TABLE();
	};

	/**
	 * @brief Preview widget shown while dragging
	 */
	class FloatingDragPreview : public wxFrame {
	public:
		FloatingDragPreview(DockWidget* content, wxWindow* parent);
		FloatingDragPreview(DockArea* content, wxWindow* parent);
		virtual ~FloatingDragPreview();

		// Content management
		void setContent(DockWidget* content);
		void setContent(DockArea* content);

		// Dragging
		void startDrag(const wxPoint& globalPos);
		void moveFloating(const wxPoint& globalPos);
		void finishDrag();

		// State
		bool isAnimated() const { return m_animated; }
		void setAnimated(bool animated) { m_animated = animated; }

	protected:
		void onPaint(wxPaintEvent& event);
		void onTimer(wxTimerEvent& event);

	private:
		wxWindow* m_content;
		wxPoint m_dragStartPos;
		bool m_animated;
		wxBitmap m_contentBitmap;
		wxTimer* m_animationTimer;
		int m_fadeAlpha;
		bool m_fadingIn;

		void updateContentBitmap();

		wxDECLARE_EVENT_TABLE();
	};
=======
// Forward declarations
class DockWidget;
class DockArea;
class DockContainerWidget;
class FloatingDragPreview;

/**
 * @brief Container for floating dock widgets
 */
class FloatingDockContainer : public wxFrame {
public:
    // Internal state - moved here to be available for method declarations
    enum eDragState {
        DraggingInactive,
        DraggingMousePressed,
        DraggingTab,
        DraggingFloatingWidget
    };
    
    FloatingDockContainer(DockManager* dockManager);
    FloatingDockContainer(DockArea* dockArea);
    FloatingDockContainer(DockWidget* dockWidget);
    virtual ~FloatingDockContainer();

    // Dock container access
    DockContainerWidget* dockContainer() const { return m_dockContainer; }
    
    // Widget management
    void addDockWidget(DockWidget* dockWidget);
    void removeDockWidget(DockWidget* dockWidget);
    
    // State
    bool isClosable() const;
    bool hasTopLevelDockWidget() const;
    DockWidget* topLevelDockWidget() const;
    std::vector<DockWidget*> dockWidgets() const;
    
    // Title handling
    void updateWindowTitle();
    
    // Drag & drop
    void startDragging(const wxPoint& dragOffset);
    
    // Native window handling
    void setNativeTitleBar(bool native);
    bool hasNativeTitleBar() const { return m_hasNativeTitleBar; }
    
    // State persistence
    void saveState(wxString& xmlData) const;
    bool restoreState(const wxString& xmlData);
    
    // Dragging
    void startFloating(const wxPoint& dragStartPos, const wxSize& size, 
                      eDragState dragState, wxWindow* mouseEventHandler);
    void moveFloating();
    void finishDragging();
    
    // Testing
    bool isInTitleBar(const wxPoint& pos) const;
    
    // Events
    static wxEventTypeTag<wxCommandEvent> EVT_FLOATING_CONTAINER_CLOSING;
    static wxEventTypeTag<wxCommandEvent> EVT_FLOATING_CONTAINER_CLOSED;

protected:
    // Event handlers
    void onClose(wxCloseEvent& event);
    void onMouseLeftDown(wxMouseEvent& event);
    void onMouseLeftUp(wxMouseEvent& event);
    void onMouseMove(wxMouseEvent& event);
    void onMouseDoubleClick(wxMouseEvent& event);
    void onNonClientHitTest(wxMouseEvent& event);
    void onMaximize(wxMaximizeEvent& event);
    
    // Internal methods
    void setupCustomTitleBar();
    bool testConfigFlag(DockManagerFeature flag) const;
    
private:
    // Private implementation
    class Private;
    std::unique_ptr<Private> d;
    
    // Member variables
    DockManager* m_dockManager;
    DockContainerWidget* m_dockContainer;
    bool m_hasNativeTitleBar;
    eDragState m_dragState;
    wxPoint m_dragStartPos;
    FloatingDragPreview* m_dragPreview;
    
    // Initialization
    void init();
    
    friend class DockManager;
    friend class DockArea;
    friend class DockWidget;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Preview widget shown while dragging
 */
class FloatingDragPreview : public wxFrame {
public:
    FloatingDragPreview(DockWidget* content, wxWindow* parent);
    FloatingDragPreview(DockArea* content, wxWindow* parent);
    virtual ~FloatingDragPreview();
    
    // Content management
    void setContent(DockWidget* content);
    void setContent(DockArea* content);
    
    // Dragging
    void startDrag(const wxPoint& globalPos);
    void moveFloating(const wxPoint& globalPos);
    void finishDrag();
    
    // State
    bool isAnimated() const { return m_animated; }
    void setAnimated(bool animated) { m_animated = animated; }
    
protected:
    void onPaint(wxPaintEvent& event);
    void onTimer(wxTimerEvent& event);
    
private:
    wxWindow* m_content;
    wxPoint m_dragStartPos;
    bool m_animated;
    wxBitmap m_contentBitmap;
    wxTimer* m_animationTimer;
    int m_fadeAlpha;
    bool m_fadingIn;
    
    void updateContentBitmap();
    
    wxDECLARE_EVENT_TABLE();
};

<<<<<<< Current (Your changes)
<<<<<<< Current (Your changes)
>>>>>>> Incoming (Background Agent changes)
=======
>>>>>>> Incoming (Background Agent changes)
=======
>>>>>>> Incoming (Background Agent changes)
} // namespace ads
