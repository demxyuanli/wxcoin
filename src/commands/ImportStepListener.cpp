#include "ImportStepListener.h"
#include "CommandDispatcher.h"
#include "STEPReader.h"
#include "OCCViewer.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include "logger/Logger.h"
#include <chrono>

ImportStepListener::ImportStepListener(wxFrame* frame, Canvas* canvas, OCCViewer* occViewer)
    : m_frame(frame), m_canvas(canvas), m_occViewer(occViewer)
{
    if (!m_frame) {
        LOG_ERR_S("ImportStepListener: frame pointer is null");
    }
}

CommandResult ImportStepListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>&) {
    auto totalImportStartTime = std::chrono::high_resolution_clock::now();
    
    // File dialog
    auto fileDialogStartTime = std::chrono::high_resolution_clock::now();
    wxFileDialog openFileDialog(m_frame, "Import STEP Files", "", "",
                               "STEP files (*.step;*.stp)|*.step;*.stp|All files (*.*)|*.*",
                               wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return CommandResult(false, "STEP import cancelled", commandType);
    }
    
    wxArrayString filePaths;
    openFileDialog.GetPaths(filePaths);
    auto fileDialogEndTime = std::chrono::high_resolution_clock::now();
    auto fileDialogDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fileDialogEndTime - fileDialogStartTime);
    
    LOG_INF_S("=== BATCH STEP IMPORT START ===");
    LOG_INF_S("Files selected: " + std::to_string(filePaths.size()) + ", Dialog time: " + std::to_string(fileDialogDuration.count()) + "ms");
    
    try {
        // Setup optimization options
        STEPReader::OptimizationOptions options;
        options.enableParallelProcessing = true;
        options.enableShapeAnalysis = false;
        options.enableCaching = true;
        options.enableBatchOperations = true;
        options.maxThreads = std::thread::hardware_concurrency();
        options.precision = 0.01;
        
        // Process all selected files
        std::vector<std::shared_ptr<OCCGeometry>> allGeometries;
        int successfulFiles = 0;
        int totalGeometries = 0;
        double totalImportTime = 0.0;
        
        for (size_t i = 0; i < filePaths.size(); ++i) {
            const wxString& filePath = filePaths[i];
            auto stepReadStartTime = std::chrono::high_resolution_clock::now();
            auto result = STEPReader::readSTEPFile(filePath.ToStdString(), options);
            auto stepReadEndTime = std::chrono::high_resolution_clock::now();
            auto stepReadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(stepReadEndTime - stepReadStartTime);
            
            if (result.success && !result.geometries.empty()) {
                allGeometries.insert(allGeometries.end(), result.geometries.begin(), result.geometries.end());
                successfulFiles++;
                totalGeometries += result.geometries.size();
                totalImportTime += result.importTime;
                
                LOG_INF_S("File " + std::to_string(i + 1) + "/" + std::to_string(filePaths.size()) + 
                         ": " + std::to_string(result.geometries.size()) + " geometries in " + 
                         std::to_string(stepReadDuration.count()) + "ms");
            } else {
                LOG_WRN_S("File " + std::to_string(i + 1) + "/" + std::to_string(filePaths.size()) + 
                         " failed: " + (result.success ? "No geometries" : result.errorMessage));
            }
        }
        
        if (!allGeometries.empty() && m_occViewer) {
            // Add all geometries using batch operations
            auto geometryAddStartTime = std::chrono::high_resolution_clock::now();
            // Temporarily use rough deflection to accelerate initial meshing
            double prevDeflection = m_occViewer->getMeshDeflection();
            double roughDeflection = m_occViewer->getLODRoughDeflection();
            if (roughDeflection <= 0.0) {
                roughDeflection = 0.1; // fallback
            }

            m_occViewer->beginBatchOperation();
            m_occViewer->setMeshDeflection(roughDeflection, false);
            m_occViewer->addGeometries(allGeometries);
            m_occViewer->endBatchOperation();

            // Restore previous deflection without immediate remesh to avoid long stall
            m_occViewer->setMeshDeflection(prevDeflection, false);
            auto geometryAddEndTime = std::chrono::high_resolution_clock::now();
            auto geometryAddDuration = std::chrono::duration_cast<std::chrono::milliseconds>(geometryAddEndTime - geometryAddStartTime);
            
            // Show performance summary
            wxString performanceMsg = wxString::Format(
                "STEP files imported successfully!\n\n"
                "Files processed: %d/%zu\n"
                "Total geometries: %d\n"
                "Total import time: %.1f ms\n"
                "Performance: %.1f geometries/second",
                successfulFiles, filePaths.size(),
                totalGeometries,
                totalImportTime,
                totalGeometries / (totalImportTime / 1000.0)
            );
            
            wxMessageDialog dialog(m_frame, performanceMsg, "Batch Import Complete", 
                                 wxOK | wxICON_INFORMATION);
            dialog.ShowModal();
            
            auto totalImportEndTime = std::chrono::high_resolution_clock::now();
            auto totalImportDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalImportEndTime - totalImportStartTime);
            
            LOG_INF_S("=== BATCH IMPORT COMPLETE ===");
            LOG_INF_S("Success: " + std::to_string(successfulFiles) + "/" + std::to_string(filePaths.size()) + 
                     " files, " + std::to_string(totalGeometries) + " geometries");
            LOG_INF_S("Timing: Import=" + std::to_string(totalImportTime) + "ms, Add=" + 
                     std::to_string(geometryAddDuration.count()) + "ms, Total=" + 
                     std::to_string(totalImportDuration.count()) + "ms");
            LOG_INF_S("Performance: " + std::to_string(totalGeometries / (totalImportDuration.count() / 1000.0)) + " geometries/second");
            LOG_INF_S("=============================");
            
            // Auto-fit all geometries after import
            if (m_occViewer) {
                LOG_INF_S("Auto-executing fitAll after STEP import");
                m_occViewer->fitAll();
            }
            
            return CommandResult(true, "STEP files imported successfully", commandType);
        } else {
            wxString warningMsg = wxString::Format(
                "No valid geometries found in selected files.\n\n"
                "Files processed: %d/%zu\n"
                "Successful files: %d",
                filePaths.size(), filePaths.size(), successfulFiles
            );
            wxMessageDialog dialog(m_frame, warningMsg, "Import Warning", wxOK | wxICON_WARNING);
            dialog.ShowModal();
            return CommandResult(false, "No valid geometries found in selected files", commandType);
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception during STEP import: " + std::string(e.what()));
        wxMessageDialog dialog(m_frame, "Error importing STEP files: " + std::string(e.what()), 
                             "Import Error", wxOK | wxICON_ERROR);
        dialog.ShowModal();
        return CommandResult(false, "Error importing STEP files: " + std::string(e.what()), commandType);
    }
}

bool ImportStepListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ImportSTEP);
} 
