#ifndef MODERN_DOCK_ADAPTER_H
#define MODERN_DOCK_ADAPTER_H

#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <map>
#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockPanel.h"

// Migration adapter to provide FlatDockManager compatible interface
// while using the new ModernDockManager internally
class ModernDockAdapter : public wxPanel {
public:
	// Keep compatible enum for existing code
	enum class DockPos {
		LeftTop,
		LeftBottom,
		Center,
		Bottom
	};

	enum class DockRegion {
		Center,
		Left,
		Right,
		Top,
		Bottom
	};

	explicit ModernDockAdapter(wxWindow* parent);
	~ModernDockAdapter() override = default;

	// Compatible API methods for existing code
	void AddPane(wxWindow* pane, DockPos pos, int sizePx = -1);

	// Preview methods (simplified for compatibility)
	void ShowDockPreview(const wxPoint& screenPt);
	void HideDockPreview();

	// Access to underlying modern dock manager
	ModernDockManager* GetModernManager() const { return m_modernManager; }

	// Legacy splitter access (returns nullptr - modern system doesn't use splitters)
	wxSplitterWindow* GetMainSplitter() const { return nullptr; }
	wxSplitterWindow* GetLeftSplitter() const { return nullptr; }

private:
	void CreateDefaultLayout();
	DockArea ConvertDockPos(DockPos pos);

	ModernDockManager* m_modernManager;
	std::map<DockPos, ModernDockPanel*> m_panelMap;
	std::map<DockPos, int> m_pendingSizes;
};

#endif // MODERN_DOCK_ADAPTER_H
