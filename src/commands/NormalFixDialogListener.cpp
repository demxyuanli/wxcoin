#include "NormalFixDialogListener.h"
#include "OCCViewer.h"
#include "NormalFixDialog.h"
#include "NormalValidator.h"
#include "logger/Logger.h"
#include <wx/app.h>
#include <wx/frame.h>

NormalFixDialogListener::NormalFixDialogListener(wxFrame* frame, OCCViewer* viewer) 
    : m_frame(frame), m_viewer(viewer) {}

CommandResult NormalFixDialogListener::executeCommand(const std::string& commandType,
                                                    const std::unordered_map<std::string, std::string>& parameters) {
    LOG_INF_S("NormalFixDialogListener::executeCommand called with commandType: " + commandType);
    
    if (!m_frame || !m_viewer) {
        LOG_ERR_S("Frame or OCCViewer not available");
        return CommandResult(false, "Frame or OCCViewer not available", commandType);
    }
    
    try {
        LOG_INF_S("Creating NormalFixDialog");
        // Create and show the normal fix dialog
        NormalFixDialog dialog(m_frame, m_viewer);
        
        LOG_INF_S("Showing NormalFixDialog modal");
        int result = dialog.ShowModal();
        
        LOG_INF_S("NormalFixDialog result: " + std::to_string(result));
        
        if (result == wxID_OK || result == wxID_APPLY) {
            // Get settings from dialog
            auto settings = dialog.getSettings();
            
            // Determine which geometries to process
            std::vector<std::shared_ptr<OCCGeometry>> geometries;
            
            if (settings.applyToSelected) {
                geometries = m_viewer->getSelectedGeometries();
                if (geometries.empty()) {
                    return CommandResult(false, "No geometries selected", commandType);
                }
            } else if (settings.applyToAll) {
                geometries = m_viewer->getAllGeometry();
                if (geometries.empty()) {
                    return CommandResult(false, "No geometries available", commandType);
                }
            } else {
                return CommandResult(false, "No application scope selected", commandType);
            }
            
            LOG_INF_S("Starting normal correction for " + std::to_string(geometries.size()) + " geometries");
            
            int correctedCount = 0;
            int totalCount = geometries.size();
            
            // Apply normal correction to each geometry
            for (auto& geometry : geometries) {
                if (!geometry) continue;
                
                const TopoDS_Shape& originalShape = static_cast<GeometryRenderer*>(geometry.get())->getShape();
                if (originalShape.IsNull()) continue;
                
                // Check if correction is needed based on quality threshold
                if (settings.autoCorrect) {
                    double quality = NormalValidator::getNormalQualityScore(originalShape);
                    if (quality < settings.qualityThreshold) {
                        // Apply normal correction
                        TopoDS_Shape correctedShape = NormalValidator::autoCorrectNormals(originalShape, geometry->getName());
                        
                        // Update the geometry with corrected shape
                        geometry->setShape(correctedShape);
                        correctedCount++;
                        
                        LOG_INF_S("Corrected normals for geometry: " + geometry->getName());
                    } else {
                        LOG_INF_S("Geometry " + geometry->getName() + " already has good normals (quality: " + 
                                std::to_string(quality) + ")");
                    }
                }
            }
            
            // Refresh the viewer to show changes
            m_viewer->requestViewRefresh();
            
            LOG_INF_S("Normal correction completed: " + std::to_string(correctedCount) + "/" + 
                     std::to_string(totalCount) + " geometries processed");
            
            std::string message = "Normal fix applied to " + std::to_string(correctedCount) + 
                                 " out of " + std::to_string(totalCount) + " geometries";
            
            return CommandResult(true, message, commandType);
        }
        
        return CommandResult(false, "Operation cancelled", commandType);
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception during normal fix dialog: " + std::string(e.what()));
        return CommandResult(false, "Error: " + std::string(e.what()), commandType);
    }
}

bool NormalFixDialogListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::NormalFixDialog);
}

std::string NormalFixDialogListener::getListenerName() const {
    return "NormalFixDialogListener";
}
