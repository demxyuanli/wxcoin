#include "FlatFrame.h"
#include "OCCViewer.h"
#include "Canvas.h"
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
	{ID_VIEW_ALL, cmd::CommandType::ViewAll},
	{ID_VIEW_TOP, cmd::CommandType::ViewTop},
	{ID_VIEW_FRONT, cmd::CommandType::ViewFront},
	{ID_VIEW_RIGHT, cmd::CommandType::ViewRight},
	{ID_VIEW_ISOMETRIC, cmd::CommandType::ViewIsometric},
	{ID_SHOW_NORMALS, cmd::CommandType::ShowNormals},
	{ID_FIX_NORMALS, cmd::CommandType::FixNormals},
	{ID_SET_TRANSPARENCY, cmd::CommandType::SetTransparency},
	{ID_TOGGLE_WIREFRAME, cmd::CommandType::ToggleWireframe},
	{ID_TOGGLE_EDGES, cmd::CommandType::ToggleEdges},
	{ID_VIEW_SHOW_ORIGINAL_EDGES, cmd::CommandType::ShowOriginalEdges},
	{ID_SHOW_FEATURE_EDGES, cmd::CommandType::ShowFeatureEdges},
	{ID_SHOW_MESH_EDGES, cmd::CommandType::ShowMeshEdges},
	{ID_SHOW_FACE_NORMALS, cmd::CommandType::ShowFaceNormals},
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
	{ID_MESH_QUALITY_DIALOG, cmd::CommandType::MeshQualityDialog},
	{ID_RENDERING_SETTINGS, cmd::CommandType::RenderingSettings},
	{ID_EDGE_SETTINGS, cmd::CommandType::EdgeSettings},
	{ID_LIGHTING_SETTINGS, cmd::CommandType::LightingSettings},
	{ID_RENDER_PREVIEW_SYSTEM, cmd::CommandType::RenderPreviewSystem},
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
		LOG_WRN_S("Unknown command ID: " + std::to_string(event.GetId()));
		return;
	}
	cmd::CommandType commandType = it->second;
	std::unordered_map<std::string, std::string> parameters;
	if (commandType == cmd::CommandType::ShowNormals) { parameters["toggle"] = "true"; }
	if (m_listenerManager && m_listenerManager->hasListener(commandType)) {
		CommandResult result = m_listenerManager->dispatch(commandType, parameters);
		onCommandFeedback(result);
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