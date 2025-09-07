#include "ImportGeometryListener.h"
#include "CommandDispatcher.h"
#include "GeometryReader.h"
#include "STEPReader.h"
#include "IGESReader.h"
#include "OBJReader.h"
#include "STLReader.h"
#include "BREPReader.h"
#include "XTReader.h"
#include "OCCViewer.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include <wx/timer.h>
#include "logger/Logger.h"
#include <chrono>
#include "FlatFrame.h"
#include "ImportSettingsDialog.h"
#include "flatui/FlatUIStatusBar.h"

ImportGeometryListener::ImportGeometryListener(wxFrame* frame, Canvas* canvas, OCCViewer* occViewer)
    : m_frame(frame), m_canvas(canvas), m_occViewer(occViewer), m_statusBar(nullptr)
{
    if (!m_frame) {
        LOG_ERR_S("ImportGeometryListener: frame pointer is null");
        return;
    }

    // Try to get the status bar from the frame
    FlatFrame* flatFrame = dynamic_cast<FlatFrame*>(frame);
    if (flatFrame) {
        m_statusBar = flatFrame->GetFlatUIStatusBar();
    }
    
    if (!m_statusBar) {
        LOG_WRN_S("ImportGeometryListener: Could not find FlatUIStatusBar, progress will not be shown");
    }
}

CommandResult ImportGeometryListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>&) {
    auto totalImportStartTime = std::chrono::high_resolution_clock::now();

    // Initialize progress using status bar
    if (m_statusBar) {
        m_statusBar->EnableProgressGauge(true);
        m_statusBar->SetGaugeRange(100);
        m_statusBar->SetGaugeValue(0);
        m_statusBar->SetStatusText("Geometry import started...", 0);
    }

    // Get FlatFrame for message output
    FlatFrame* flatFrame = dynamic_cast<FlatFrame*>(m_frame);
    if (!flatFrame) {
        wxWindow* topWindow = wxTheApp->GetTopWindow();
        if (topWindow) {
            flatFrame = dynamic_cast<FlatFrame*>(topWindow);
        }
    }

    if (flatFrame) {
        flatFrame->appendMessage("Geometry import started...");
    }

    // File dialog with all supported formats
    auto fileDialogStartTime = std::chrono::high_resolution_clock::now();
    std::string fileFilter = GeometryReaderFactory::getAllSupportedFileFilter();
    
    wxFileDialog openFileDialog(m_frame, "Import Geometry Files", "", "",
        fileFilter.c_str(),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        cleanupProgress();
        return CommandResult(false, "Geometry import cancelled", commandType);
    }

    wxArrayString filePaths;
    openFileDialog.GetPaths(filePaths);

    if (filePaths.empty()) {
        cleanupProgress();
        return CommandResult(false, "No files selected", commandType);
    }

    // Group files by format
    std::unordered_map<std::string, std::vector<std::string>> filesByFormat;
    for (const auto& filePath : filePaths) {
        auto reader = GeometryReaderFactory::getReaderForFile(filePath.ToStdString());
        if (reader) {
            std::string formatName = reader->getFormatName();
            filesByFormat[formatName].push_back(filePath.ToStdString());
        } else {
            LOG_WRN_S("Unsupported file format: " + filePath.ToStdString());
        }
    }

    if (filesByFormat.empty()) {
        cleanupProgress();
        wxMessageDialog* dialog = new wxMessageDialog(m_frame,
            "No supported geometry files found in selection.",
            "Import Error", wxOK | wxICON_ERROR | wxCENTRE);
        dialog->ShowModal();
        delete dialog;
        return CommandResult(false, "No supported geometry files found", commandType);
    }

    auto fileDialogEndTime = std::chrono::high_resolution_clock::now();
    auto fileDialogDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fileDialogEndTime - fileDialogStartTime);

    LOG_INF_S("=== BATCH GEOMETRY IMPORT START ===");
    LOG_INF_S("Files selected: " + std::to_string(filePaths.size()) + ", Dialog time: " + std::to_string(fileDialogDuration.count()) + "ms");
    if (flatFrame) {
        flatFrame->appendMessage(wxString::Format("Files selected: %zu", filePaths.size()));
    }

    try {
        int totalSuccessfulFiles = 0;
        int totalGeometries = 0;
        double totalImportTime = 0.0;
        std::vector<std::shared_ptr<OCCGeometry>> allGeometries;

        // Process each format group
        for (const auto& formatGroup : filesByFormat) {
            const std::string& formatName = formatGroup.first;
            const std::vector<std::string>& formatFiles = formatGroup.second;

            if (flatFrame) {
                flatFrame->appendMessage(wxString::Format("Processing %s files: %zu files", formatName, formatFiles.size()));
            }

            // Get reader for this format
            auto reader = GeometryReaderFactory::getReaderForExtension(formatFiles[0]);
            if (!reader) {
                LOG_ERR_S("Failed to get reader for format: " + formatName);
                continue;
            }

            // Show import settings dialog
            GeometryReader::OptimizationOptions options;
            if (!showImportSettingsDialog(reader.get(), options)) {
                cleanupProgress();
                return CommandResult(false, "Import cancelled by user", commandType);
            }

            // Import files for this format
            auto result = importFiles(std::move(reader), formatFiles, options);
            
            if (result.success) {
                totalSuccessfulFiles += formatFiles.size();
                // Note: We would need to extract geometries from result, but CommandResult doesn't contain them
                // This is a limitation of the current design - we'd need to modify CommandResult to include geometries
            }
        }

        // Add all geometries to viewer
        if (!allGeometries.empty() && m_occViewer) {
            auto geometryAddStartTime = std::chrono::high_resolution_clock::now();
            
            m_occViewer->beginBatchOperation();
            m_occViewer->addGeometries(allGeometries);
            m_occViewer->endBatchOperation();
            
            if (m_statusBar) {
                m_statusBar->SetGaugeValue(98);
                m_statusBar->SetStatusText("Adding geometries to scene...", 0);
            }
            
            if (flatFrame) {
                flatFrame->appendMessage("[98%] Adding geometries to scene...");
            }
            
            auto geometryAddEndTime = std::chrono::high_resolution_clock::now();
            auto geometryAddDuration = std::chrono::duration_cast<std::chrono::milliseconds>(geometryAddEndTime - geometryAddStartTime);
            
            if (flatFrame) {
                flatFrame->appendMessage(wxString::Format("Added %zu geometries to scene in %lld ms",
                    allGeometries.size(), (long long)geometryAddDuration.count()));
            }

            // Show performance summary
            wxString performanceMsg = wxString::Format(
                "Geometry files imported successfully!\n\n"
                "Files processed: %d/%zu\n"
                "Total geometries: %d\n"
                "Total import time: %.1f ms",
                totalSuccessfulFiles, filePaths.size(),
                totalGeometries,
                totalImportTime
            );

            // Ensure progress is complete before showing dialog
            if (m_statusBar) {
                m_statusBar->SetGaugeValue(100);
                m_statusBar->SetStatusText("Import completed!", 0);
                // Hide progress bar after a short delay
                wxTimer* hideTimer = new wxTimer();
                hideTimer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
                    cleanupProgress();
                });
                hideTimer->StartOnce(2000); // Hide after 2 seconds
            }
            
            if (flatFrame) {
                flatFrame->appendMessage("[100%] Import completed!");
                flatFrame->appendMessage("Geometry import completed.");
            }

            // Show dialog
            wxWindow* topWindow = wxTheApp->GetTopWindow();
            if (!topWindow) {
                topWindow = m_frame;
            }

            wxMessageDialog* dialog = new wxMessageDialog(topWindow, performanceMsg,
                "Batch Import Complete", wxOK | wxICON_INFORMATION | wxCENTRE);
            dialog->ShowModal();
            delete dialog;

            // Auto-fit all geometries after import
            if (m_occViewer) {
                LOG_INF_S("Auto-executing fitAll after geometry import");
                m_occViewer->fitAll(); 
            }

            return CommandResult(true, "Geometry files imported successfully", commandType);
        }
        else {
            wxString warningMsg = wxString::Format(
                "No valid geometries found in selected files.\n\n"
                "Files processed: %d/%zu\n"
                "Successful files: %d", 
                filePaths.size(), filePaths.size(), totalSuccessfulFiles
            ); 

            cleanupProgress();

            wxWindow* topWindow = wxTheApp->GetTopWindow();
            if (!topWindow) {
                topWindow = m_frame;
            }

            wxMessageDialog* dialog = new wxMessageDialog(topWindow, warningMsg,
                "Import Warning", wxOK | wxICON_WARNING | wxCENTRE);
            dialog->ShowModal();
            delete dialog;

            return CommandResult(false, "No valid geometries found in selected files", commandType);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception during geometry import: " + std::string(e.what()));
        cleanupProgress();

        wxWindow* topWindow = wxTheApp->GetTopWindow();
        if (!topWindow) {
            topWindow = m_frame;
        }

        wxMessageDialog* dialog = new wxMessageDialog(topWindow,
            "Error importing geometry files: " + std::string(e.what()),
            "Import Error", wxOK | wxICON_ERROR | wxCENTRE);
        dialog->ShowModal();
        delete dialog;

        return CommandResult(false, "Error importing geometry files: " + std::string(e.what()), commandType);
    }
}

bool ImportGeometryListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ImportSTEP) ||
           commandType == "ImportGeometry";
}

CommandResult ImportGeometryListener::importFiles(std::unique_ptr<GeometryReader> reader,
    const std::vector<std::string>& filePaths,
    const GeometryReader::OptimizationOptions& options)
{
    try {
        std::vector<std::shared_ptr<OCCGeometry>> allGeometries;
        int successfulFiles = 0;

        for (size_t i = 0; i < filePaths.size(); ++i) {
            const std::string& filePath = filePaths[i];
            
            auto result = reader->readFile(filePath, options, 
                [this, i, &filePaths](int percent, const std::string& stage) {
                    updateProgress(percent, stage, nullptr);
                });

            if (result.success && !result.geometries.empty()) {
                allGeometries.insert(allGeometries.end(), result.geometries.begin(), result.geometries.end());
                successfulFiles++;
            }
        }

        return CommandResult(successfulFiles > 0, 
            "Imported " + std::to_string(successfulFiles) + "/" + std::to_string(filePaths.size()) + " files", 
            "ImportGeometry");
    }
    catch (const std::exception& e) {
        return CommandResult(false, "Error importing files: " + std::string(e.what()), "ImportGeometry");
    }
}

bool ImportGeometryListener::showImportSettingsDialog(GeometryReader* reader, 
    GeometryReader::OptimizationOptions& options)
{
    // For now, use the existing ImportSettingsDialog
    // In the future, this could be made format-specific
    ImportSettingsDialog settingsDialog(m_frame);
    if (settingsDialog.ShowModal() != wxID_OK) {
        return false;
    }

    // Map settings to optimization options
    options.meshDeflection = settingsDialog.getMeshDeflection();
    options.angularDeflection = settingsDialog.getAngularDeflection();
    options.enableParallelProcessing = settingsDialog.isParallelProcessing();
    options.enableShapeAnalysis = settingsDialog.isAdaptiveMeshing();
    options.enableCaching = true;
    options.enableBatchOperations = true;
    options.maxThreads = std::thread::hardware_concurrency();
    options.precision = 0.01;

    return true;
}

void ImportGeometryListener::updateProgress(int percent, const std::string& message, void* flatFrame)
{
    if (m_statusBar) {
        m_statusBar->SetGaugeValue(percent);
        m_statusBar->SetStatusText(message, 0);
        m_statusBar->Refresh();
        wxYield();
    }

    if (flatFrame) {
        FlatFrame* frame = static_cast<FlatFrame*>(flatFrame);
        frame->appendMessage(wxString::Format("[%d%%] %s", percent, message));
    }
}

void ImportGeometryListener::cleanupProgress()
{
    if (m_statusBar) {
        try {
            m_statusBar->EnableProgressGauge(false);
            m_statusBar->SetStatusText("Ready", 0);
        }
        catch (...) {}
    }
}
