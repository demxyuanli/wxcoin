#include "ExplodeConfigListener.h"
#include "OCCViewer.h"
#include <wx/wx.h>
#include <wx/radiobox.h>
#include <wx/spinctrl.h>

// Forward declare dialog
class ExplodeConfigDialog;

ExplodeConfigListener::ExplodeConfigListener(wxFrame* frame, OCCViewer* viewer)
    : m_frame(frame), m_viewer(viewer) {}

CommandResult ExplodeConfigListener::executeCommand(const std::string& commandType,
                                                  const std::unordered_map<std::string, std::string>&) {
    if (!m_frame || !m_viewer) return CommandResult(false, "Viewer not available", commandType);

    // Create dialog lazily (inline simple implementation)
    class ExplodeConfigDialog : public wxDialog {
    public:
        ExplodeConfigDialog(wxWindow* parent, OCCViewer* viewer)
            : wxDialog(parent, wxID_ANY, "Explode Configuration", wxDefaultPosition, wxSize(320, 200)), m_viewer(viewer) {
            wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
            wxArrayString modes; modes.Add("Radial"); modes.Add("Axis X"); modes.Add("Axis Y"); modes.Add("Axis Z");
            m_mode = new wxRadioBox(this, wxID_ANY, "Mode", wxDefaultPosition, wxDefaultSize, modes, 1, wxRA_SPECIFY_ROWS);
            m_factor = new wxSpinCtrlDouble(this, wxID_ANY);
            m_factor->SetRange(0.01, 10.0); m_factor->SetIncrement(0.05);
            // Load current
            OCCViewer::ExplodeMode mode; double factor; viewer->getExplodeParams(mode, factor);
            int sel = 0; if (mode==OCCViewer::ExplodeMode::AxisX) sel=1; else if (mode==OCCViewer::ExplodeMode::AxisY) sel=2; else if (mode==OCCViewer::ExplodeMode::AxisZ) sel=3;
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
        OCCViewer::ExplodeMode getMode() const {
            int sel = m_mode->GetSelection();
            if (sel==1) return OCCViewer::ExplodeMode::AxisX;
            if (sel==2) return OCCViewer::ExplodeMode::AxisY;
            if (sel==3) return OCCViewer::ExplodeMode::AxisZ;
            return OCCViewer::ExplodeMode::Radial;
        }
        double getFactor() const { return m_factor->GetValue(); }
    private:
        OCCViewer* m_viewer;
        wxRadioBox* m_mode{nullptr};
        wxSpinCtrlDouble* m_factor{nullptr};
    };

    ExplodeConfigDialog dlg(m_frame, m_viewer);
    if (dlg.ShowModal() == wxID_OK) {
        m_viewer->setExplodeParams(dlg.getMode(), dlg.getFactor());
        return CommandResult(true, "Explode parameters updated", commandType);
    }
    return CommandResult(false, "Explode configuration cancelled", commandType);
}

bool ExplodeConfigListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ExplodeConfig);
}


