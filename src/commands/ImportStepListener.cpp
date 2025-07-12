#include "ImportStepListener.h"
#include "CommandDispatcher.h"
#include "STEPReader.h"
#include "OCCViewer.h"
#include <wx/filedlg.h>
#include "logger/Logger.h"

ImportStepListener::ImportStepListener(wxFrame* frame, Canvas* canvas, OCCViewer* occViewer)
    : m_frame(frame), m_canvas(canvas), m_occViewer(occViewer)
{
    if (!m_frame) {
        LOG_ERR_S("ImportStepListener: frame pointer is null");
    }
}

CommandResult ImportStepListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>&) {
    wxFileDialog openFileDialog(m_frame, "Import STEP File", "", "",
                               "STEP files (*.step;*.stp)|*.step;*.stp|All files (*.*)|*.*",
                               wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return CommandResult(false, "STEP import cancelled", commandType);
    
    wxString filePath = openFileDialog.GetPath();
    LOG_INF_S("Importing STEP file: " + filePath.ToStdString());
    
    try {
        // Use the static method from STEPReader
        auto result = STEPReader::readSTEPFile(filePath.ToStdString());
        
        if (result.success && !result.geometries.empty() && m_occViewer) {
            // Add geometries to the OCC viewer (auto-updates scene bounds and view)
            for (const auto& geometry : result.geometries) {
                m_occViewer->addGeometry(geometry);
            }
            
            LOG_INF_S("Successfully imported " + std::to_string(result.geometries.size()) + " geometries from STEP file");
            return CommandResult(true, "STEP file imported successfully", commandType);
        } else if (!result.success) {
            LOG_ERR_S("Failed to read STEP file: " + result.errorMessage);
            return CommandResult(false, "Failed to read STEP file: " + result.errorMessage, commandType);
        } else {
            return CommandResult(false, "No valid geometries found in STEP file", commandType);
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception during STEP import: " + std::string(e.what()));
        return CommandResult(false, "Error importing STEP file: " + std::string(e.what()), commandType);
    }
}

bool ImportStepListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ImportSTEP);
} 
