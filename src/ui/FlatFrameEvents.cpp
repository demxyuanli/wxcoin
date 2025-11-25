#include "FlatFrame.h"
#include "OCCViewer.h"
#include "Canvas.h"
#include "InputManager.h"
#include "CommandType.h"
#include "CommandListenerManager.h"
#include "logger/Logger.h"
#include "ui/OutlineSettingsDialog.h"
#include <unordered_map>
#include <wx/splitter.h>

// Local event ID mapping (duplicated to avoid cross-TU static linkage issues)
static const std::unordered_map<int, cmd::CommandType> kEventTable = {
	{wxID_NEW, cmd::CommandType::FileNew},
	{wxID_OPEN, cmd::CommandType::FileOpen},
	{wxID_SAVE, cmd::CommandType::FileSave},
	{ID_SAVE_AS, cmd::CommandType::FileSaveAs},
	{ID_IMPORT_STEP, cmd::CommandType::ImportSTEP},
	{wxID_EXIT, cmd::CommandType::FileExit},
	{ID_CREATE_BOX, cmd::CommandType::CreateBox},
	{ID_CREATE_SPHERE, cmd::CommandType::CreateSphere},
	{ID_CREATE_CYLINDER, cmd::CommandType::CreateCylinder},
	{ID_CREATE_CONE, cmd::CommandType::CreateCone},
	{ID_CREATE_TORUS, cmd::CommandType::CreateTorus},
	{ID_CREATE_TRUNCATED_CYLINDER, cmd::CommandType::CreateTruncatedCylinder},
	{ID_CREATE_WRENCH, cmd::CommandType::CreateWrench},
	{ID_CREATE_NAV_CUBE, cmd::CommandType::CreateNavCube},
	{ID_VIEW_ALL, cmd::CommandType::ViewAll},
	{ID_VIEW_TOP, cmd::CommandType::ViewTop},
	{ID_VIEW_FRONT, cmd::CommandType::ViewFront},
	{ID_VIEW_RIGHT, cmd::CommandType::ViewRight},
	{ID_VIEW_ISOMETRIC, cmd::CommandType::ViewIsometric},

	// View Bookmarks
	{ID_VIEW_BOOKMARK_SAVE, cmd::CommandType::ViewBookmarkSave},
	{ID_VIEW_BOOKMARK_FRONT, cmd::CommandType::ViewBookmarkFront},
	{ID_VIEW_BOOKMARK_BACK, cmd::CommandType::ViewBookmarkBack},
	{ID_VIEW_BOOKMARK_LEFT, cmd::CommandType::ViewBookmarkLeft},
	{ID_VIEW_BOOKMARK_RIGHT, cmd::CommandType::ViewBookmarkRight},
	{ID_VIEW_BOOKMARK_TOP, cmd::CommandType::ViewBookmarkTop},
	{ID_VIEW_BOOKMARK_BOTTOM, cmd::CommandType::ViewBookmarkBottom},
	{ID_VIEW_BOOKMARK_ISOMETRIC, cmd::CommandType::ViewBookmarkIsometric},
	{ID_VIEW_BOOKMARK_MANAGER, cmd::CommandType::ViewBookmarkManager},

	// Animation Types
	{ID_ANIMATION_TYPE_LINEAR, cmd::CommandType::AnimationTypeLinear},
	{ID_ANIMATION_TYPE_SMOOTH, cmd::CommandType::AnimationTypeSmooth},
	{ID_ANIMATION_TYPE_EASE_IN, cmd::CommandType::AnimationTypeEaseIn},
	{ID_ANIMATION_TYPE_EASE_OUT, cmd::CommandType::AnimationTypeEaseOut},
	{ID_ANIMATION_TYPE_BOUNCE, cmd::CommandType::AnimationTypeBounce},

	// Zoom Controls
	{ID_ZOOM_IN, cmd::CommandType::ZoomIn},
	{ID_ZOOM_OUT, cmd::CommandType::ZoomOut},
	{ID_ZOOM_RESET, cmd::CommandType::ZoomReset},
	{ID_ZOOM_SETTINGS, cmd::CommandType::ZoomSettings},
	{ID_ZOOM_LEVEL_25, cmd::CommandType::ZoomLevel25},
	{ID_ZOOM_LEVEL_50, cmd::CommandType::ZoomLevel50},
	{ID_ZOOM_LEVEL_100, cmd::CommandType::ZoomLevel100},
	{ID_ZOOM_LEVEL_200, cmd::CommandType::ZoomLevel200},
	{ID_ZOOM_LEVEL_400, cmd::CommandType::ZoomLevel400},
	{ID_SHOW_NORMALS, cmd::CommandType::ShowNormals},
	{ID_FIX_NORMALS, cmd::CommandType::FixNormals},
	{ID_NORMAL_FIX_DIALOG, cmd::CommandType::NormalFixDialog},
	{ID_SET_TRANSPARENCY, cmd::CommandType::SetTransparency},
	{ID_TOGGLE_WIREFRAME, cmd::CommandType::ToggleWireframe},
	{ID_TOGGLE_EDGES, cmd::CommandType::ToggleEdges},
	{ID_VIEW_SHOW_ORIGINAL_EDGES, cmd::CommandType::ShowOriginalEdges},
	{ID_SHOW_FEATURE_EDGES, cmd::CommandType::ShowFeatureEdges},
	{ID_SHOW_MESH_EDGES, cmd::CommandType::ShowMeshEdges},
	{ID_SHOW_FACE_NORMALS, cmd::CommandType::ShowFaceNormals},
	{ID_FACE_QUERY_TOOL, cmd::CommandType::FaceQueryTool},
	{ID_FACE_SELECTION_TOOL, cmd::CommandType::FaceSelectionTool},
	{ID_EDGE_SELECTION_TOOL, cmd::CommandType::EdgeSelectionTool},
	{ID_VERTEX_SELECTION_TOOL, cmd::CommandType::VertexSelectionTool},
	{ID_SHOW_POINT_VIEW, cmd::CommandType::ShowPointView},
	{ID_TOGGLE_SLICE, cmd::CommandType::SliceToggle},
	{ID_TEXTURE_MODE_DECAL, cmd::CommandType::TextureModeDecal},
	{ID_TEXTURE_MODE_MODULATE, cmd::CommandType::TextureModeModulate},
	{ID_TEXTURE_MODE_REPLACE, cmd::CommandType::TextureModeReplace},
	{ID_TEXTURE_MODE_BLEND, cmd::CommandType::TextureModeBlend},
	{ID_TOGGLE_COORDINATE_SYSTEM, cmd::CommandType::ToggleCoordinateSystem},
	{ID_TOGGLE_REFERENCE_GRID, cmd::CommandType::ToggleReferenceGrid},
	{ID_TOGGLE_CHESSBOARD_GRID, cmd::CommandType::ToggleChessboardGrid},
	{ID_EXPLODE_ASSEMBLY, cmd::CommandType::ExplodeAssembly},
	{ID_UNDO, cmd::CommandType::Undo},
	{ID_REDO, cmd::CommandType::Redo},
	{ID_NAVIGATION_CUBE_CONFIG, cmd::CommandType::NavCubeConfig},
	{ID_ZOOM_SPEED, cmd::CommandType::ZoomSpeed},
	{ID_NAVIGATION_MODE, cmd::CommandType::NavigationMode},
	{ID_MESH_QUALITY_DIALOG, cmd::CommandType::MeshQualityDialog},
	{ID_RENDERING_SETTINGS, cmd::CommandType::RenderingSettings},
	{ID_EDGE_SETTINGS, cmd::CommandType::EdgeSettings},
	{ID_LIGHTING_SETTINGS, cmd::CommandType::LightingSettings},
	{ID_SELECTION_HIGHLIGHT_CONFIG, cmd::CommandType::SelectionHighlightConfig},
	{ID_DOCK_LAYOUT_CONFIG, cmd::CommandType::DockLayoutConfig},
	{ID_RENDER_PREVIEW_SYSTEM, cmd::CommandType::RenderPreviewSystem},
	{ID_SPLIT_VIEW_SINGLE, cmd::CommandType::SplitViewSingle},
	{ID_SPLIT_VIEW_HORIZONTAL_2, cmd::CommandType::SplitViewHorizontal2},
	{ID_SPLIT_VIEW_VERTICAL_2, cmd::CommandType::SplitViewVertical2},
	{ID_SPLIT_VIEW_QUAD, cmd::CommandType::SplitViewQuad},
	{ID_SPLIT_VIEW_SIX, cmd::CommandType::SplitViewSix},
	{ID_SPLIT_VIEW_TOGGLE_SYNC, cmd::CommandType::SplitViewToggleSync},
	{ID_RENDER_MODE_NO_SHADING, cmd::CommandType::RenderModeNoShading},
	{ID_RENDER_MODE_POINTS, cmd::CommandType::RenderModePoints},
	{ID_RENDER_MODE_WIREFRAME, cmd::CommandType::RenderModeWireframe},
	{ID_RENDER_MODE_FLAT_LINES, cmd::CommandType::RenderModeFlatLines},
	{ID_RENDER_MODE_SHADED, cmd::CommandType::RenderModeShaded},
	{ID_RENDER_MODE_SHADED_WIREFRAME, cmd::CommandType::RenderModeShadedWireframe},
	{ID_RENDER_MODE_HIDDEN_LINE, cmd::CommandType::RenderModeHiddenLine},
	{ID_SHOW_FLAT_WIDGETS_EXAMPLE, cmd::CommandType::ShowFlatWidgetsExample},
	{wxID_ABOUT, cmd::CommandType::HelpAbout}
};
#include <wx/msgdlg.h>

void FlatFrame::OnLeftDown(wxMouseEvent& event)
{
	if (m_homeMenu && m_homeMenu->IsShown()) {
		event.Skip();
		return;
	}
	FlatUIFrame::OnLeftDown(event);
}

void FlatFrame::OnMotion(wxMouseEvent& event)
{
	if (m_homeMenu && m_homeMenu->IsShown()) {
		SetCursor(wxCursor(wxCURSOR_ARROW));
		event.Skip();
		return;
	}
	FlatUIFrame::OnMotion(event);
}

void FlatFrame::onCommand(wxCommandEvent& event) {
	auto it = kEventTable.find(event.GetId());
	if (it == kEventTable.end()) {
		// Handle non-command mapped UI toggles here
		if (event.GetId() == ID_TOGGLE_OUTLINE) {
			// Route through command system for consistency
			std::unordered_map<std::string, std::string> params; params["toggle"] = "true";
			if (m_listenerManager) {
				auto result = m_listenerManager->dispatch(cmd::CommandType::ToggleOutline, params);
				onCommandFeedback(result);
			}
			else if (m_occViewer) {
				m_occViewer->setOutlineEnabled(!m_occViewer->isOutlineEnabled());
			}
			return;
		}
		if (event.GetId() == ID_OUTLINE_SETTINGS) {
			// Open outline settings dialog
			auto params = m_occViewer ? m_occViewer->getOutlineParams() : ImageOutlineParams{};
			OutlineSettingsDialog dlg(this, params);
			if (dlg.ShowModal() == wxID_OK) {
				auto p = dlg.getParams();
				if (m_occViewer) m_occViewer->setOutlineParams(p);
			}
			return;
		}
		// For unknown command IDs, skip to allow event propagation to derived classes
		// This is crucial for docking system buttons handled by FlatFrameDocking
		LOG_INF_S("Unknown command ID, skipping event propagation: " + std::to_string(event.GetId()));
		event.Skip();
		return;
	}
	cmd::CommandType commandType = it->second;
	std::unordered_map<std::string, std::string> parameters;
	if (commandType == cmd::CommandType::ShowNormals) { 
		parameters["toggle"] = "true"; 
		LOG_INF_S("FlatFrame::onCommand - ShowNormals command detected, will dispatch");
	}
	if (m_listenerManager && m_listenerManager->hasListener(commandType)) {
		LOG_INF_S("FlatFrame::onCommand - Dispatching command: " + cmd::to_string(commandType));
		CommandResult result = m_listenerManager->dispatch(commandType, parameters);
		onCommandFeedback(result);
		// Handle explode slider dialog creation and destruction
		if (commandType == cmd::CommandType::ExplodeAssembly && m_occViewer) {
			if (m_occViewer->isExplodeEnabled()) {
				// Create a lightweight modeless slider window centered at the bottom of canvas
				wxWindow* canvas = m_canvas;
				m_explodeSliderDialog = new wxDialog(this, wxID_ANY, "Explode Factor", wxDefaultPosition, wxSize(400, 30), wxBORDER_NONE | wxSTAY_ON_TOP);
				m_explodeSliderDialog->SetBackgroundColour(GetBackgroundColour());
				m_explodeSliderDialog->SetTransparent(180);
				wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
				ExplodeMode tmpMode; double tmpFactor; m_occViewer->getExplodeParams(tmpMode, tmpFactor);
				wxSlider* slider = new wxSlider(m_explodeSliderDialog, wxID_ANY, int(tmpFactor * 100), 1, 1000);
				sizer->Add(slider, 1, wxEXPAND | wxALL, 2);
				m_explodeSliderDialog->SetSizerAndFit(sizer);
				if (canvas) {
					wxSize cs = canvas->GetClientSize();
					int w = 400;
					int h = 30;
					// Use screen coordinates anchored at canvas client origin for precise viewport positioning
					wxPoint sp = canvas->ClientToScreen(wxPoint(0, 0));
					int x = sp.x + (cs.GetWidth() - w) / 2;
					int y = sp.y + cs.GetHeight() - h;
					m_explodeSliderDialog->SetSize(wxRect(x, y, w, h));
				}
				slider->Bind(wxEVT_SLIDER, [this](wxCommandEvent& e) {
					double f = std::max(0.01, e.GetInt() / 100.0);
					m_occViewer->setExplodeEnabled(true, f);
				});
				m_explodeSliderDialog->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&){ 
					if (m_explodeSliderDialog) {
						m_explodeSliderDialog->Destroy();
						m_explodeSliderDialog = nullptr;
					}
				});
				m_explodeSliderDialog->Show();
			} else {
				// Destroy the slider dialog when explode is disabled
				if (m_explodeSliderDialog) {
					m_explodeSliderDialog->Destroy();
					m_explodeSliderDialog = nullptr;
				}
			}
		}
	}
	else {
		SetStatusText("Error: No listener registered", 0);
		LOG_ERR_S("No listener registered for command");
	}
}

void FlatFrame::onCommandFeedback(const CommandResult& result) {
	if (result.success) {
		SetStatusText(result.message.empty() ? "Command executed successfully" : result.message, 0);
		LOG_INF_S("Command executed: " + result.commandId);
	}
	else {
		SetStatusText("Error: " + result.message, 0);
		LOG_ERR_S("Command failed: " + result.commandId + " - " + result.message);
		if (!result.message.empty() && result.commandId != "UNKNOWN") { wxMessageBox(result.message, "Command Error", wxOK | wxICON_ERROR, this); }
	}

	if (result.commandId == cmd::to_string(cmd::CommandType::ShowNormals) && result.success && m_occViewer) {
		LOG_INF_S("Show normals state updated: " + std::string(m_occViewer->isShowNormals() ? "shown" : "hidden"));
	}
	// Outline toggle handled directly; add optional feedback if needed

	// Handle face query tool toggle state
	if (result.commandId == cmd::to_string(cmd::CommandType::FaceQueryTool) && result.success) {
		bool isActive = m_canvas && m_canvas->getInputManager() && m_canvas->getInputManager()->isCustomInputStateActive();
		// TODO: Update button state in ribbon when FlatUIBar supports SetToggleButtonState
		// if (m_ribbon) {
		//     m_ribbon->SetToggleButtonState(ID_FACE_QUERY_TOOL, isActive);
		// }
		LOG_INF_S("Face query tool state updated: " + std::string(isActive ? "active" : "inactive"));
	}

	if (m_canvas && (
		result.commandId.find("VIEW_") == 0 ||
		result.commandId.find("SHOW_") == 0 ||
		result.commandId == "FIX_NORMALS" ||
		result.commandId.find("CREATE_") == 0 ||
		result.commandId == "TOGGLE_COORDINATE_SYSTEM"
		)) {
		m_canvas->Refresh();
		LOG_INF_S("Canvas refreshed for command: " + result.commandId);
	}
}

void FlatFrame::onClose(wxCloseEvent& event) {
	LOG_INF_S("Closing application"); Destroy();
}

void FlatFrame::onActivate(wxActivateEvent& event) {
	if (event.GetActive() && m_isFirstActivate) {
		m_isFirstActivate = false;
		if (m_occViewer) {
		}
	}
	event.Skip();
}

void FlatFrame::onSize(wxSizeEvent& event) {
	event.Skip();
	static bool firstSize = true;
	if (firstSize && m_mainSplitter && m_mainSplitter->IsShown()) {
		firstSize = false;
		if (m_mainSplitter->GetSize().GetWidth() > 160) {
			m_mainSplitter->SetSashPosition(160);
		}
		if (m_leftSplitter && m_leftSplitter->IsShown()) {
			int leftHeight = m_leftSplitter->GetSize().GetHeight();
			if (leftHeight > 200) {
				int treeHeight = leftHeight - 200;
				m_leftSplitter->SetSashPosition(treeHeight);
			}
		}
	}
}