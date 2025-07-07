#include "ImportStepListener.h"
#include "MainFrame.h"
#include "Canvas.h"
#include "OCCViewer.h"
#include "STEPReader.h"
#include "SceneManager.h"
#include <wx/filedlg.h>
#include "Logger.h"

ImportStepListener::ImportStepListener(MainFrame* mainFrame, Canvas* canvas, OCCViewer* viewer)
    : m_mainFrame(mainFrame), m_canvas(canvas), m_viewer(viewer) {}

CommandResult ImportStepListener::executeCommand(const std::string& commandType,
                                                 const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mainFrame || !m_canvas) {
        return CommandResult(false, "Main frame or canvas not available", commandType);
    }

    wxString wildcard = "STEP files (*.step;*.stp)|*.step;*.stp|All files (*.*)|*.*";
    wxFileDialog dlg(m_mainFrame, "Import STEP File", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_CANCEL) {
        LOG_INF("STEP import dialog cancelled");
        return CommandResult(false, "Import operation cancelled", commandType);
    }

    std::string filePath = dlg.GetPath().ToStdString();
    LOG_INF("Reading STEP file: " + filePath);

    auto res = STEPReader::readSTEPFile(filePath);
    if (!res.success) {
        LOG_ERR("STEP import failed: " + res.errorMessage);
        m_canvas->showErrorDialog(res.errorMessage);
        return CommandResult(false, res.errorMessage, commandType);
    }

    if (!m_viewer) {
        std::string err = "OCCViewer not available";
        LOG_ERR(err);
        m_canvas->showErrorDialog(err);
        return CommandResult(false, err, commandType);
    }
    for (auto& geom : res.geometries) {
        m_viewer->addGeometry(geom);
    }
    m_canvas->Refresh();

    if (m_canvas && m_canvas->getSceneManager()) {
        m_canvas->getSceneManager()->updateSceneBounds();
        m_canvas->getSceneManager()->resetView();
    }

    std::string msg = "Imported " + std::to_string(res.geometries.size()) + " objects";
    return CommandResult(true, msg, commandType);
}

bool ImportStepListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ImportSTEP);
} 