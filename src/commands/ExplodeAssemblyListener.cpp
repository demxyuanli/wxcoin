#include "ExplodeAssemblyListener.h"
#include "ExplodeConfigListener.h"
#include "OCCViewer.h"
#include "viewer/ExplodeTypes.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/spinctrl.h>
#include "ExplodeConfigDialog.h"

ExplodeAssemblyListener::ExplodeAssemblyListener(wxFrame* frame, OCCViewer* viewer)
	: m_frame(frame), m_viewer(viewer) {
}

CommandResult ExplodeAssemblyListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_frame || !m_viewer) return CommandResult(false, "OCCViewer not available", commandType);

	auto geoms = m_viewer->getAllGeometry();
	if (geoms.size() <= 1) {
		wxMessageBox("Explode view requires an assembly (2+ parts)", "Explode View", wxOK | wxICON_INFORMATION);
		return CommandResult(false, "Not an assembly", commandType);
	}

	// If already enabled, toggle off immediately
	if (m_viewer->isExplodeEnabled()) {
		m_viewer->setExplodeEnabled(false, 1.0);
		return CommandResult(true, "Explode cleared", commandType);
	}

	// Popup config dialog first
	OCCViewer::ExplodeMode mode; double factor; m_viewer->getExplodeParams(mode, factor);
	ExplodeConfigDialog dlg(m_frame, mode, factor);
	if (dlg.ShowModal() != wxID_OK) {
		return CommandResult(false, "Explode cancelled", commandType);
	}
	// Advanced params
	ExplodeParams params = dlg.getParams();
	m_viewer->setExplodeParamsAdvanced(params);

	// Enable explode using selected params
	m_viewer->setExplodeEnabled(true, params.baseFactor);
	return CommandResult(true, "Explode applied", commandType);
}

bool ExplodeAssemblyListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ExplodeAssembly);
}