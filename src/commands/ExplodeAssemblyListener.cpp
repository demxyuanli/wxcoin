#include "ExplodeAssemblyListener.h"
#include "ExplodeConfigListener.h"
#include "OCCViewer.h"
#include "viewer/ExplodeTypes.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/spinctrl.h>

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
	class ExplodeConfigDialog : public wxDialog {
	public:
		ExplodeConfigDialog(wxWindow* parent, OCCViewer* viewer)
			: wxDialog(parent, wxID_ANY, "Explode", wxDefaultPosition, wxSize(320, 200)), m_viewer(viewer) {
			wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
			wxArrayString modes; modes.Add("Radial"); modes.Add("Axis X"); modes.Add("Axis Y"); modes.Add("Axis Z"); modes.Add("Stack X"); modes.Add("Stack Y"); modes.Add("Stack Z"); modes.Add("Diagonal");
			m_mode = new wxRadioBox(this, wxID_ANY, "Mode", wxDefaultPosition, wxDefaultSize, modes, 1, wxRA_SPECIFY_ROWS);
			m_factor = new wxSpinCtrlDouble(this, wxID_ANY);
			m_factor->SetRange(0.01, 10.0); m_factor->SetIncrement(0.05);
			ExplodeMode mode; double factor; viewer->getExplodeParams(mode, factor);
			int sel = 0; if (mode == ExplodeMode::AxisX) sel = 1; else if (mode == ExplodeMode::AxisY) sel = 2; else if (mode == ExplodeMode::AxisZ) sel = 3; else if (mode == ExplodeMode::StackX) sel = 4; else if (mode == ExplodeMode::StackY) sel = 5; else if (mode == ExplodeMode::StackZ) sel = 6; else if (mode == ExplodeMode::Diagonal) sel = 7;
			m_mode->SetSelection(sel); m_factor->SetValue(factor);
			wxFlexGridSizer* grid = new wxFlexGridSizer(2, 8, 8);
			grid->Add(new wxStaticText(this, wxID_ANY, "Distance Factor:"), 0, wxALIGN_CENTER_VERTICAL);
			grid->Add(m_factor, 1, wxEXPAND);
			top->Add(m_mode, 0, wxALL | wxEXPAND, 10);
			top->Add(grid, 0, wxALL | wxEXPAND, 10);
			wxStdDialogButtonSizer* btns = new wxStdDialogButtonSizer();
			btns->AddButton(new wxButton(this, wxID_OK));
			btns->AddButton(new wxButton(this, wxID_CANCEL));
			btns->Realize(); top->Add(btns, 0, wxALL | wxALIGN_RIGHT, 8);
			SetSizerAndFit(top);
		}
		ExplodeMode getMode() const {
			int sel = m_mode->GetSelection();
			if (sel == 1) return ExplodeMode::AxisX;
			if (sel == 2) return ExplodeMode::AxisY;
			if (sel == 3) return ExplodeMode::AxisZ;
			if (sel == 4) return ExplodeMode::StackX;
			if (sel == 5) return ExplodeMode::StackY;
			if (sel == 6) return ExplodeMode::StackZ;
			if (sel == 7) return ExplodeMode::Diagonal;
			return ExplodeMode::Radial;
		}
		double getFactor() const { return m_factor->GetValue(); }
	private:
		OCCViewer* m_viewer;
		wxRadioBox* m_mode{ nullptr };
		wxSpinCtrlDouble* m_factor{ nullptr };
	};

	ExplodeConfigDialog dlg(m_frame, m_viewer);
	if (dlg.ShowModal() != wxID_OK) {
		return CommandResult(false, "Explode cancelled", commandType);
	}
	m_viewer->setExplodeParams(dlg.getMode(), dlg.getFactor());

	// Enable explode using selected params
	m_viewer->setExplodeEnabled(true, dlg.getFactor());
	return CommandResult(true, "Explode applied", commandType);
}

bool ExplodeAssemblyListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ExplodeAssembly);
}