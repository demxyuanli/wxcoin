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
#include "GeometryDecompositionDialog.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include <wx/timer.h>
#include "logger/Logger.h"
#include <chrono>
#include <filesystem>
#include "FlatFrame.h"
#include "flatui/FlatUIStatusBar.h"

ImportGeometryListener::ImportGeometryListener(wxFrame* frame, Canvas* canvas, OCCViewer* occViewer)
    : m_frame(frame), m_canvas(canvas), m_occViewer(occViewer), m_statusBar(nullptr)
    , m_decompositionOptions()
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

        // Show statistics dialog even for empty results
        ImportOverallStatistics overallStats;
        overallStats.totalFilesSelected = filePaths.size();
        overallStats.totalFilesProcessed = 0;
        overallStats.totalSuccessfulFiles = 0;
        overallStats.totalFailedFiles = filePaths.size();
        overallStats.totalGeometriesCreated = 0;
        overallStats.totalImportTime = std::chrono::milliseconds(0);

        // Add failed files to statistics
        for (size_t i = 0; i < filePaths.size(); ++i) {
            const auto& filePath = filePaths[i];
            ImportFileStatistics fileStat;
            wxFileName wxFile(filePath);
            fileStat.fileName = wxFile.GetFullName().ToStdString();
            fileStat.filePath = filePath.ToStdString();
            fileStat.format = "Unsupported";
            fileStat.success = false;
            fileStat.errorMessage = "Unsupported file format";
            fileStat.geometriesCreated = 0;
            fileStat.importTime = std::chrono::milliseconds(0);

            overallStats.fileStats.push_back(fileStat);
        }

        wxWindow* topWindow = wxTheApp->GetTopWindow();
        if (!topWindow) {
            topWindow = m_frame;
        }

        ImportStatisticsDialog statsDialog(topWindow, overallStats);
        statsDialog.ShowModal();

        return CommandResult(false, "No supported geometry files found", commandType);
    }

    // Check if we have non-mesh formats that might benefit from decomposition
    bool hasNonMeshFormats = false;
    for (const auto& formatGroup : filesByFormat) {
        const std::string& formatName = formatGroup.first;
        if (formatName == "STEP" || formatName == "IGES" || formatName == "BREP") {
            hasNonMeshFormats = true;
            break;
        }
    }

    // Show geometry decomposition dialog for non-mesh formats
    if (hasNonMeshFormats) {
        GeometryDecompositionDialog dialog(m_frame, m_decompositionOptions);
        if (dialog.ShowModal() == wxID_OK) {
            LOG_INF_S("Geometry decomposition configured: enabled=" +
                std::string(m_decompositionOptions.enableDecomposition ? "true" : "false") +
                ", level=" + std::to_string(static_cast<int>(m_decompositionOptions.level)));
        } else {
            LOG_INF_S("Geometry decomposition dialog cancelled, using default settings");
        }
    }

    auto fileDialogEndTime = std::chrono::high_resolution_clock::now();
    auto fileDialogDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fileDialogEndTime - fileDialogStartTime);

    LOG_INF_S("=== BATCH GEOMETRY IMPORT START ===");
    LOG_INF_S("Files selected: " + std::to_string(filePaths.size()) + ", Dialog time: " + std::to_string(fileDialogDuration.count()) + "ms");
    if (flatFrame) {
        flatFrame->appendMessage(wxString::Format("Files selected: %zu", filePaths.size()));
    }

    try {
        // Initialize detailed statistics collection
        ImportOverallStatistics overallStats;
        overallStats.totalFilesSelected = filePaths.size();
        overallStats.totalDialogTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - fileDialogStartTime);

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
            auto reader = GeometryReaderFactory::getReaderForFile(formatFiles[0]);
            if (!reader) {
                LOG_ERR_S("Failed to get reader for format: " + formatName + ", file: " + formatFiles[0]);
                continue;
            }

            // Use balanced default settings for import (no dialog needed)
            GeometryReader::OptimizationOptions options;
            setupBalancedImportOptions(options);

            // Apply decomposition options
            options.decomposition = m_decompositionOptions;

            // Import files for this format
            auto formatStartTime = std::chrono::high_resolution_clock::now();
            auto result = importFilesWithStats(std::move(reader), formatFiles, options, overallStats, formatName, allGeometries);
            auto formatEndTime = std::chrono::high_resolution_clock::now();

            if (result.success) {
                totalSuccessfulFiles += formatFiles.size();
                // Update format statistics
                auto& formatStat = overallStats.formatStats[formatName];
                formatStat.totalImportTime += std::chrono::duration_cast<std::chrono::milliseconds>(formatEndTime - formatStartTime);
            }
        }

        // Add all geometries to viewer
        if (!allGeometries.empty() && m_occViewer) {
            LOG_INF_S("Adding " + std::to_string(allGeometries.size()) + " geometries to viewer");
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

        }

        // Always show detailed statistics dialog (moved outside the condition)
        wxWindow* topWindow = wxTheApp->GetTopWindow();
        if (!topWindow) {
            topWindow = m_frame;
        }

        // Update overall statistics
        overallStats.totalFilesProcessed = filePaths.size();
        overallStats.totalSuccessfulFiles = totalSuccessfulFiles;
        overallStats.totalFailedFiles = filePaths.size() - totalSuccessfulFiles;
        // Use the accumulated geometries count from statistics
        overallStats.totalGeometriesCreated = overallStats.totalGeometriesCreated;
        overallStats.totalImportTime = std::chrono::milliseconds(static_cast<long long>(totalImportTime));

        LOG_INF_S("Showing import statistics dialog - Files: " + std::to_string(overallStats.totalFilesProcessed) +
                 ", Successful: " + std::to_string(overallStats.totalSuccessfulFiles) +
                 ", Geometries: " + std::to_string(overallStats.totalGeometriesCreated));

        ImportStatisticsDialog statsDialog(topWindow, overallStats);
        int result = statsDialog.ShowModal();
        LOG_INF_S("Statistics dialog closed with result: " + std::to_string(result));

        // Ensure progress is complete before finishing
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

        // Auto-fit all geometries after import
        if (m_occViewer && !allGeometries.empty()) {
            LOG_INF_S("Auto-executing fitAll after geometry import");
            m_occViewer->fitAll();
        }

        if (!allGeometries.empty()) {
            return CommandResult(true, "Geometry files imported successfully", commandType);
        } else {
            return CommandResult(false, "No valid geometries found in selected files", commandType);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception during geometry import: " + std::string(e.what()));
        cleanupProgress();

        // Show statistics dialog with error information
        ImportOverallStatistics overallStats;
        overallStats.totalFilesSelected = filePaths.size();
        overallStats.totalFilesProcessed = 0; // Exception occurred before processing
        overallStats.totalSuccessfulFiles = 0;
        overallStats.totalFailedFiles = 0;
        overallStats.totalGeometriesCreated = 0;
        overallStats.totalImportTime = std::chrono::milliseconds(0);

        // Add all files as failed due to exception
        for (size_t i = 0; i < filePaths.size(); ++i) {
            const auto& filePath = filePaths[i];
            ImportFileStatistics fileStat;
            wxFileName wxFile(filePath);
            fileStat.fileName = wxFile.GetFullName().ToStdString();
            fileStat.filePath = filePath.ToStdString();
            fileStat.format = "Unknown (Exception)";
            fileStat.success = false;
            fileStat.errorMessage = "Import failed due to exception: " + std::string(e.what());
            fileStat.geometriesCreated = 0;
            fileStat.importTime = std::chrono::milliseconds(0);

            overallStats.fileStats.push_back(fileStat);
        }

        wxWindow* topWindow = wxTheApp->GetTopWindow();
        if (!topWindow) {
            topWindow = m_frame;
        }

        ImportStatisticsDialog statsDialog(topWindow, overallStats);
        statsDialog.ShowModal();

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
    // Initialize statistics and geometries vector
    ImportOverallStatistics dummyStats;
    std::vector<std::shared_ptr<OCCGeometry>> dummyGeometries;
    return importFilesWithStats(std::move(reader), filePaths, options, dummyStats, "", dummyGeometries);
}

CommandResult ImportGeometryListener::importFilesWithStats(std::unique_ptr<GeometryReader> reader,
    const std::vector<std::string>& filePaths,
    const GeometryReader::OptimizationOptions& options,
    ImportOverallStatistics& overallStats,
    const std::string& formatName,
    std::vector<std::shared_ptr<OCCGeometry>>& allGeometries)
{
    try {
        int successfulFiles = 0;
        size_t totalFileSize = 0;

        for (size_t i = 0; i < filePaths.size(); ++i) {
            const std::string& filePath = filePaths[i];

            // Create file statistics
            ImportFileStatistics fileStat;
            fileStat.filePath = filePath;
            wxFileName wxFile(filePath);
            fileStat.fileName = wxFile.GetFullName().ToStdString();
            fileStat.format = formatName.empty() ? "Unknown" : formatName;

            // Get file size
            try {
                std::filesystem::path fsPath(filePath.c_str());
                if (std::filesystem::exists(fsPath)) {
                    fileStat.fileSize = std::filesystem::file_size(fsPath);
                    totalFileSize += fileStat.fileSize;
                }
            } catch (const std::exception&) {
                fileStat.fileSize = 0;
            }

            auto fileStartTime = std::chrono::high_resolution_clock::now();

            auto result = reader->readFile(filePath, options,
                [this, i, &filePaths](int percent, const std::string& stage) {
                    updateProgress(percent, stage, nullptr);
                });

            auto fileEndTime = std::chrono::high_resolution_clock::now();
            fileStat.importTime = std::chrono::duration_cast<std::chrono::milliseconds>(fileEndTime - fileStartTime);

            if (result.success && !result.geometries.empty()) {
                allGeometries.insert(allGeometries.end(), result.geometries.begin(), result.geometries.end());
                fileStat.success = true;
                fileStat.geometriesCreated = result.geometries.size();
                successfulFiles++;
            } else {
                fileStat.success = false;
                fileStat.errorMessage = result.errorMessage;
                fileStat.geometriesCreated = 0;
            }

            // Add to overall statistics
            overallStats.fileStats.push_back(fileStat);
            overallStats.totalGeometriesCreated += fileStat.geometriesCreated;
            overallStats.totalFileSize += fileStat.fileSize;
        }

        // Update format statistics
        if (!formatName.empty()) {
            auto& formatStat = overallStats.formatStats[formatName];
            formatStat.formatName = formatName;
            formatStat.totalFiles += filePaths.size();
            formatStat.successfulFiles += successfulFiles;
            formatStat.failedFiles += (filePaths.size() - successfulFiles);
            formatStat.totalGeometries += allGeometries.size();
            formatStat.totalFileSize += totalFileSize;
        }

        return CommandResult(successfulFiles > 0,
            "Imported " + std::to_string(successfulFiles) + "/" + std::to_string(filePaths.size()) + " files",
            "ImportGeometry");
    }
    catch (const std::exception& e) {
        return CommandResult(false, "Error importing files: " + std::string(e.what()), "ImportGeometry");
    }
}

void ImportGeometryListener::setupBalancedImportOptions(GeometryReader::OptimizationOptions& options)
{
    // Use balanced default settings for optimal import performance and quality
    // Balanced preset: Good balance between quality and performance
    options.meshDeflection = 1.0;          // Balanced mesh precision
    options.angularDeflection = 1.0;       // Balanced curve approximation
    options.enableParallelProcessing = true; // Enable parallel processing
    options.enableShapeAnalysis = false;    // Disable adaptive meshing for consistency
    options.enableCaching = true;           // Enable caching for better performance
    options.enableBatchOperations = true;   // Enable batch operations
    options.maxThreads = std::thread::hardware_concurrency();
    options.precision = 0.01;              // Standard precision
    options.enableNormalProcessing = false; // Disable normal processing for performance
    
    // Tessellation settings for balanced quality
    options.enableFineTessellation = true;   // Enable fine tessellation
    options.tessellationDeflection = 0.01;   // Fine tessellation precision
    options.tessellationAngle = 0.1;         // Tessellation angle
    options.tessellationMinPoints = 3;       // Minimum tessellation points
    options.tessellationMaxPoints = 100;     // Maximum tessellation points
    options.enableAdaptiveTessellation = true; // Enable adaptive tessellation
    
    LOG_INF_S("Balanced import settings applied: Deflection=1.0, Angular=1.0, Parallel=On");
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
