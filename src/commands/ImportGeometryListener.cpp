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
#include "SceneManager.h"
#include "Canvas.h"
#include "RenderingEngine.h"
#include "GeometryDecompositionDialog.h"
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include "GeometryImportOptimizer.h"
#include "ProgressiveGeometryLoader.h"
#include "StreamingFileReader.h"
#include "STEPGeometryDecomposer.h"
#include "STEPColorManager.h"
#include "STEPGeometryConverter.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include <wx/timer.h>
#include "logger/Logger.h"
#include <chrono>
#include <filesystem>
#include "FlatFrame.h"
#include "flatui/FlatUIStatusBar.h"
#include <GL/gl.h>

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

    // Set loading cursor for import process
    wxWindow* topWindow = wxTheApp->GetTopWindow();
    if (topWindow) {
        topWindow->SetCursor(wxCursor(wxCURSOR_WAIT));
        LOG_INF_S("Set loading cursor for geometry import");
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

    // Check if we have BRep-compatible formats that support decomposition
    // Only formats that can be converted to TopoDS_Shape (BRep) support decomposition
    // Mesh-only formats (STL, OBJ) do not support decomposition
    bool hasBRepFormats = false;
    for (const auto& formatGroup : filesByFormat) {
        const std::string& formatName = formatGroup.first;
        // BRep-compatible formats: STEP, IGES, BREP, X_T (Parasolid)
        // Mesh-only formats (excluded): STL, OBJ
        if (formatName == "STEP" || formatName == "IGES" || formatName == "BREP" || formatName == "X_T") {
            hasBRepFormats = true;
            break;
        }
    }

    // Show geometry decomposition dialog only for BRep-compatible formats
    if (hasBRepFormats) {
        // Force parent frame to complete all pending paint operations
        // This prevents DC handle conflicts on Windows (error 0x00000006)
        if (m_frame) {
            m_frame->Update();  // Force immediate processing of pending paint events
        }
        
        // Process all pending events to ensure all DC handles are released
        if (wxTheApp) {
            wxTheApp->Yield(true);
        }
        
        // Small delay to ensure Windows GDI system completes all operations
        wxMilliSleep(10);
        
        // Check if geometry is large/complex before showing dialog
        std::vector<std::string> filePathsForCheck;
        for (const auto& formatGroup : filesByFormat) {
            for (const auto& filePath : formatGroup.second) {
                filePathsForCheck.push_back(filePath);
            }
        }
        bool isLargeComplex = GeometryDecompositionDialog::isLargeComplexGeometry(filePathsForCheck);
        
        if (isLargeComplex && flatFrame) {
            flatFrame->appendMessage("Large/complex geometry detected - using balanced settings");
        }
        
        GeometryDecompositionDialog dialog(m_frame, m_decompositionOptions, isLargeComplex);
        if (dialog.ShowModal() == wxID_OK) {
            LOG_INF_S("Geometry decomposition configured: enabled=" +
                std::string(m_decompositionOptions.enableDecomposition ? "true" : "false") +
                ", level=" + std::to_string(static_cast<int>(m_decompositionOptions.level)));
        } else {
            LOG_INF_S("Geometry decomposition dialog cancelled, using default settings (no decomposition)");
            // Reset to default settings when dialog is cancelled
            m_decompositionOptions.enableDecomposition = false;
            m_decompositionOptions.level = GeometryReader::DecompositionLevel::NO_DECOMPOSITION;
            m_decompositionOptions.colorScheme = GeometryReader::ColorScheme::DISTINCT_COLORS;
            m_decompositionOptions.useConsistentColoring = true;
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
        
        // First pass: check for large files that need progressive loading
        std::vector<std::string> largeFiles;
        for (const auto& formatGroup : filesByFormat) {
            for (const auto& filePath : formatGroup.second) {
                try {
                    size_t fileSize = std::filesystem::file_size(filePath);
                    if (shouldUseProgressiveLoading(filePath, fileSize)) {
                        largeFiles.push_back(filePath);
                        LOG_INF_S("Large file detected (" + std::to_string(fileSize / (1024*1024)) + 
                                 " MB), will use progressive loading: " + filePath);
                        
                        if (flatFrame) {
                            flatFrame->appendMessage(wxString::Format("Large file (%zu MB) - will use progressive loading mode", 
                                                    fileSize / (1024*1024)));
                        }
                    }
                } catch (const std::exception& e) {
                    LOG_WRN_S("Failed to check file size: " + std::string(e.what()));
                }
            }
        }
        
        // Handle large files with progressive loading first
        for (const auto& filePath : largeFiles) {
            auto fileStartTime = std::chrono::high_resolution_clock::now();
            
            GeometryReader::OptimizationOptions opts;
            // Apply decomposition options from dialog FIRST, before setupBalancedImportOptions
            opts.decomposition = m_decompositionOptions;
            // Now setup options based on the decomposition settings (including mesh quality preset)
            setupBalancedImportOptions(opts);
            
            std::vector<std::shared_ptr<OCCGeometry>> progressiveGeoms;
            if (importWithProgressiveLoading(filePath, opts, progressiveGeoms)) {
                allGeometries.insert(allGeometries.end(), 
                                   progressiveGeoms.begin(), 
                                   progressiveGeoms.end());
                totalSuccessfulFiles++;
                
                // Create file statistics for progressive loading
                auto fileEndTime = std::chrono::high_resolution_clock::now();
                ImportFileStatistics fileStat;
                wxFileName wxFile(filePath);
                fileStat.fileName = wxFile.GetFullName().ToStdString();
                fileStat.filePath = filePath;
                fileStat.format = "STEP (Progressive)";
                fileStat.success = true;
                fileStat.geometriesCreated = progressiveGeoms.size();
                fileStat.importTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    fileEndTime - fileStartTime);
                
                try {
                    fileStat.fileSize = std::filesystem::file_size(filePath);
                } catch (...) {
                    fileStat.fileSize = 0;
                }
                
                overallStats.fileStats.push_back(fileStat);
                overallStats.totalGeometriesCreated += fileStat.geometriesCreated;
                overallStats.totalFileSize += fileStat.fileSize;
                
                LOG_INF_S("Progressive loading stats: " + std::to_string(progressiveGeoms.size()) + 
                         " geometries in " + std::to_string(fileStat.importTime.count()) + "ms");
                
                if (flatFrame) {
                    flatFrame->appendMessage(wxString::Format("Progressive loading completed: %zu geometries", 
                                            progressiveGeoms.size()));
                }
            }
        }

        // Process each format group (skip files already processed with progressive loading)
        for (const auto& formatGroup : filesByFormat) {
            const std::string& formatName = formatGroup.first;
            std::vector<std::string> formatFiles = formatGroup.second;
            
            // Remove large files from normal processing
            formatFiles.erase(
                std::remove_if(formatFiles.begin(), formatFiles.end(),
                    [&largeFiles](const std::string& file) {
                        return std::find(largeFiles.begin(), largeFiles.end(), file) != largeFiles.end();
                    }),
                formatFiles.end()
            );
            
            if (formatFiles.empty()) {
                continue; // All files in this format were large files
            }

            if (flatFrame) {
                flatFrame->appendMessage(wxString::Format("Processing %s files: %zu files", formatName, formatFiles.size()));
            }

            // Get reader for this format
            auto reader = GeometryReaderFactory::getReaderForFile(formatFiles[0]);
            if (!reader) {
                LOG_ERR_S("Failed to get reader for format: " + formatName + ", file: " + formatFiles[0]);
                continue;
            }

            // Use enhanced optimization options
            GeometryImportOptimizer::EnhancedOptions enhancedOptions;
            
            // Configure threading based on file count and size
            enhancedOptions.threading.maxThreads = std::thread::hardware_concurrency();
            enhancedOptions.threading.enableParallelReading = true;
            enhancedOptions.threading.enableParallelParsing = true;
            enhancedOptions.threading.enableParallelTessellation = true;
            enhancedOptions.threading.useMemoryMapping = true;
            
            // Configure progressive loading for better responsiveness
            enhancedOptions.progressive.enabled = true;
            enhancedOptions.progressive.streamLargeFiles = true;
            
            // Enable caching for repeated imports
            enhancedOptions.enableCache = true;
            
            // Apply decomposition options FIRST, before setupBalancedImportOptions
            enhancedOptions.decomposition = m_decompositionOptions;
            // Now setup options based on the decomposition settings (including mesh quality preset)
            setupBalancedImportOptions(enhancedOptions);
            
            // Enable profiling for performance monitoring
            GeometryImportOptimizer::enableProfiling(true);

            // Import files for this format
            auto formatStartTime = std::chrono::high_resolution_clock::now();
            auto result = importFilesWithStats(std::move(reader), formatFiles, enhancedOptions, overallStats, formatName, allGeometries);
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
            
            // CRITICAL FIX: Update OCCViewer mesh parameters from import options BEFORE adding geometries
            // This ensures imported geometries use the correct mesh quality settings
            GeometryReader::OptimizationOptions tempOpts;
            tempOpts.decomposition = m_decompositionOptions;
            setupBalancedImportOptions(tempOpts);
            
            // Update viewer mesh parameters to match import settings
            m_occViewer->setMeshDeflection(tempOpts.meshDeflection, false); // false = don't remesh existing geometries
            m_occViewer->setAngularDeflection(tempOpts.angularDeflection, false);
            
            // CRITICAL FIX: Apply subdivision and smoothing parameters from decomposition options
            // This ensures imported geometries use the smooth surface settings from the dialog
            m_occViewer->setSubdivisionEnabled(m_decompositionOptions.subdivisionEnabled);
            m_occViewer->setSubdivisionLevel(m_decompositionOptions.subdivisionLevel);
            m_occViewer->setSubdivisionMethod(0); // Catmull-Clark (default)
            m_occViewer->setSubdivisionCreaseAngle(30.0); // Default crease angle
            
            m_occViewer->setSmoothingEnabled(m_decompositionOptions.smoothingEnabled);
            m_occViewer->setSmoothingMethod(0); // Laplacian (default)
            m_occViewer->setSmoothingIterations(m_decompositionOptions.smoothingIterations);
            m_occViewer->setSmoothingStrength(m_decompositionOptions.smoothingStrength);
            m_occViewer->setSmoothingCreaseAngle(m_decompositionOptions.smoothingCreaseAngle);
            
            // Apply LOD settings
            m_occViewer->setLODEnabled(m_decompositionOptions.lodEnabled);
            m_occViewer->setLODFineDeflection(m_decompositionOptions.lodFineDeflection);
            m_occViewer->setLODRoughDeflection(m_decompositionOptions.lodRoughDeflection);
            
            // Apply tessellation settings
            m_occViewer->setTessellationQuality(m_decompositionOptions.tessellationQuality);
            m_occViewer->setFeaturePreservation(m_decompositionOptions.featurePreservation);
            
            LOG_INF_S(wxString::Format("Updated OCCViewer mesh parameters from import options: Deflection=%.4f, Angular=%.4f",
                tempOpts.meshDeflection, tempOpts.angularDeflection));
            LOG_INF_S(wxString::Format("Applied subdivision: enabled=%d, level=%d",
                m_decompositionOptions.subdivisionEnabled ? 1 : 0,
                m_decompositionOptions.subdivisionLevel));
            LOG_INF_S(wxString::Format("Applied smoothing: enabled=%d, iterations=%d, strength=%.2f, creaseAngle=%.2f",
                m_decompositionOptions.smoothingEnabled ? 1 : 0,
                m_decompositionOptions.smoothingIterations,
                m_decompositionOptions.smoothingStrength,
                m_decompositionOptions.smoothingCreaseAngle));
            LOG_INF_S(wxString::Format("Applied LOD: enabled=%d, fine=%.2f, rough=%.2f",
                m_decompositionOptions.lodEnabled ? 1 : 0,
                m_decompositionOptions.lodFineDeflection,
                m_decompositionOptions.lodRoughDeflection));
            LOG_INF_S(wxString::Format("Applied tessellation: quality=%d, featurePreservation=%.2f",
                m_decompositionOptions.tessellationQuality,
                m_decompositionOptions.featurePreservation));
            
            m_occViewer->beginBatchOperation();
            m_occViewer->addGeometries(allGeometries);
            m_occViewer->endBatchOperation();
            m_occViewer->updateObjectTreeDeferred();
            
            // Check geometry complexity after import (face count and assembly count)
            int totalFaceCount = 0;
            int assemblyCount = static_cast<int>(allGeometries.size()); // Each geometry is considered an assembly component
            
            for (const auto& geometry : allGeometries) {
                if (geometry && !geometry->getShape().IsNull()) {
                    // Count faces in this geometry
                    for (TopExp_Explorer exp(geometry->getShape(), TopAbs_FACE); exp.More(); exp.Next()) {
                        totalFaceCount++;
                    }
                }
            }
            
            // Check if geometry is complex based on counts
            bool isComplexByCounts = GeometryDecompositionDialog::isComplexGeometryByCounts(totalFaceCount, assemblyCount);
            
            if (isComplexByCounts) {
                LOG_INF_S("Complex geometry detected after import: faces=" + std::to_string(totalFaceCount) + 
                         ", assemblies=" + std::to_string(assemblyCount) + " - applying restrictions");
                
                if (flatFrame) {
                    flatFrame->appendMessage(wxString::Format("Complex geometry detected (%d faces, %d components) - using balanced settings", 
                        totalFaceCount, assemblyCount));
                }
                
                // Force balanced settings for complex geometry
                m_occViewer->setMeshDeflection(1.0, false);
                m_occViewer->setAngularDeflection(1.0, false);
                
                // Apply basic smooth parameters (not high quality)
                m_occViewer->setSubdivisionEnabled(true);
                m_occViewer->setSubdivisionLevel(2);  // Limit to 2
                m_occViewer->setSmoothingEnabled(true);
                m_occViewer->setSmoothingIterations(2);  // Limit to 2
                m_occViewer->setSmoothingStrength(0.5);  // Limit to 0.5
                
                // Enable LOD for performance
                m_occViewer->setLODEnabled(true);
                m_occViewer->setLODFineDeflection(0.2);
                m_occViewer->setLODRoughDeflection(0.5);
                
                // Use balanced tessellation quality
                m_occViewer->setTessellationQuality(2);  // Limit to 2
                m_occViewer->setFeaturePreservation(0.5);  // Limit to 0.5
                
                LOG_INF_S("Applied balanced settings for complex geometry");
            }
            
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

        // CRITICAL FIX: After ShowModal(), GL context may be invalidated
        // ShowModal() creates its own message loop which can cause context loss on Windows
        // Must reactivate GL context before any rendering operations
        if (m_occViewer && !allGeometries.empty()) {
            SceneManager* sceneManager = m_occViewer->getSceneManager();
            if (sceneManager) {
                Canvas* canvas = sceneManager->getCanvas();
                if (canvas) {
                    // Get rendering engine to check/reactivate GL context
                    RenderingEngine* renderEngine = canvas->getRenderingEngine();
                    if (renderEngine) {
                        // Check if GL context is still valid after dialog
                        if (!renderEngine->isGLContextValid()) {
                            LOG_WRN_S("GL context invalid after ShowModal, attempting reinitialize");
                            if (!renderEngine->reinitialize()) {
                                LOG_ERR_S("Failed to reinitialize GL context after dialog - rendering may fail");
                            } else {
                                LOG_INF_S("Successfully reinitialized GL context after ShowModal");
                                // After reinit, invalidate Coin3D cache
                                sceneManager->invalidateCoin3DCache();
                            }
                        } else {
                            LOG_INF_S("GL context still valid after ShowModal");
                        }
                    }
                }
            }
        }

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

        // CRITICAL FIX: Force an immediate render after batch geometry import
        // This ensures Coin3D creates GL resources (display lists, VBOs) while context is valid
        // Without this, first render after long idle may find stale/invalid GL context
        if (m_occViewer && !allGeometries.empty()) {
            LOG_INF_S("Forcing immediate render after batch import to establish GL resources");
            
            // Get canvas through SceneManager and trigger immediate render
            SceneManager* sceneManager = m_occViewer->getSceneManager();
            if (sceneManager) {
                Canvas* canvas = sceneManager->getCanvas();
                if (canvas) {
                    // Force a full quality render to build all GL resources
                    canvas->render(false);
                    
                    // Ensure GL operations complete
                    glFinish();
                    
                    LOG_INF_S("Immediate post-import render completed - GL resources established");
                }
            }
            
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
        
        // Restore arrow cursor on exception
        wxWindow* topWindow = wxTheApp->GetTopWindow();
        if (topWindow) {
            topWindow->SetCursor(wxCursor(wxCURSOR_ARROW));
            LOG_INF_S("Restored arrow cursor after import exception");
        }
        
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

        topWindow = wxTheApp->GetTopWindow();
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
    // Convert to enhanced options
    GeometryImportOptimizer::EnhancedOptions enhancedOptions;
    // Copy base options
    static_cast<GeometryReader::OptimizationOptions&>(enhancedOptions) = options;
    
    // Initialize statistics and geometries vector
    ImportOverallStatistics dummyStats;
    std::vector<std::shared_ptr<OCCGeometry>> dummyGeometries;
    return importFilesWithStats(std::move(reader), filePaths, enhancedOptions, dummyStats, "", dummyGeometries);
}

CommandResult ImportGeometryListener::importFilesWithStats(std::unique_ptr<GeometryReader> reader,
    const std::vector<std::string>& filePaths,
    const GeometryImportOptimizer::EnhancedOptions& options,
    ImportOverallStatistics& overallStats,
    const std::string& formatName,
    std::vector<std::shared_ptr<OCCGeometry>>& allGeometries)
{
    try {
        int successfulFiles = 0;
        size_t totalFileSize = 0;

        // Use batch optimization for multiple files
        if (filePaths.size() > 1 && options.threading.enableParallelReading) {
            LOG_INF_S("Using batch optimization for " + std::to_string(filePaths.size()) + " files");
            
            // Progress callback for batch import
            auto batchProgress = [this](size_t current, size_t total, const std::string& file) {
                int percent = static_cast<int>((100.0 * current) / total);
                std::string msg = "Processing file " + std::to_string(current) + "/" + 
                                 std::to_string(total) + ": " + std::filesystem::path(file).filename().string();
                updateProgress(percent, msg, nullptr);
            };
            
            // Import all files in parallel
            auto results = GeometryImportOptimizer::importBatchOptimized(filePaths, options, batchProgress);
            
            // Process results
            for (size_t i = 0; i < results.size(); ++i) {
                const auto& result = results[i];
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
                
                fileStat.importTime = std::chrono::milliseconds(static_cast<long long>(result.importTime));
                
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
        } else {
            // Single file or sequential processing
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

                // Estimate import time for progress
                double estimatedTime = GeometryImportOptimizer::estimateImportTime(filePath);
                if (estimatedTime > 0 && m_statusBar) {
                    m_statusBar->SetStatusText(wxString::Format("Importing %s (estimated: %.1fs)", 
                        wxFile.GetFullName(), estimatedTime / 1000.0), 0);
                }

                auto fileStartTime = std::chrono::high_resolution_clock::now();

                // Use optimized import
                auto result = GeometryImportOptimizer::importOptimized(filePath, options,
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

        // Log performance report
        LOG_INF_S("\n" + GeometryImportOptimizer::getPerformanceReport());

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
    // Apply mesh quality based on preset from decomposition options
    double meshDeflection = 1.0;
    double angularDeflection = 1.0;
    
    switch (options.decomposition.meshQualityPreset) {
        case GeometryReader::MeshQualityPreset::FAST:
            meshDeflection = 2.0;
            angularDeflection = 2.0;
            break;
        case GeometryReader::MeshQualityPreset::BALANCED:
            meshDeflection = 1.0;
            angularDeflection = 1.0;
            break;
        case GeometryReader::MeshQualityPreset::HIGH_QUALITY:
            // Match MeshQualityDialog "quality" preset parameters
            meshDeflection = 0.5;
            angularDeflection = 0.5;
            break;
        case GeometryReader::MeshQualityPreset::ULTRA_QUALITY:
            // Match MeshQualityDialog "ultra" preset parameters
            meshDeflection = 0.2;
            angularDeflection = 0.3;
            break;
        case GeometryReader::MeshQualityPreset::CUSTOM:
            meshDeflection = options.decomposition.customMeshDeflection;
            angularDeflection = options.decomposition.customAngularDeflection;
            break;
        default:
            meshDeflection = 1.0;
            angularDeflection = 1.0;
            break;
    }
    
    options.meshDeflection = meshDeflection;
    options.angularDeflection = angularDeflection;
    options.enableParallelProcessing = true; // Enable parallel processing
    options.enableShapeAnalysis = false;    // Disable adaptive meshing for consistency
    options.enableCaching = true;           // Enable caching for better performance
    options.enableBatchOperations = true;   // Enable batch operations
    options.maxThreads = std::thread::hardware_concurrency();
    options.precision = 0.01;              // Standard precision
    options.enableNormalProcessing = false; // Disable normal processing for performance
    
    // Tessellation settings based on mesh quality
    options.enableFineTessellation = true;   // Enable fine tessellation
    options.tessellationDeflection = meshDeflection * 0.01;   // Fine tessellation precision
    options.tessellationAngle = angularDeflection * 0.1;       // Tessellation angle
    options.tessellationMinPoints = 3;       // Minimum tessellation points
    options.tessellationMaxPoints = 100;     // Maximum tessellation points
    options.enableAdaptiveTessellation = true; // Enable adaptive tessellation
    
    LOG_INF_S(wxString::Format("Import settings applied: Deflection=%.4f, Angular=%.4f, Preset=%d, Parallel=On",
        meshDeflection, angularDeflection, static_cast<int>(options.decomposition.meshQualityPreset)));
}

void ImportGeometryListener::setupBalancedImportOptions(GeometryImportOptimizer::EnhancedOptions& options)
{
    // Copy base optimization options
    setupBalancedImportOptions(static_cast<GeometryReader::OptimizationOptions&>(options));
    
    // Set enhanced threading options
    options.threading.maxThreads = std::thread::hardware_concurrency();
    options.threading.enableParallelReading = true;
    options.threading.enableParallelParsing = true;
    options.threading.enableParallelTessellation = true;
    options.threading.useMemoryMapping = true;
    options.threading.chunkSize = 2 * 1024 * 1024; // 2MB chunks
    
    // Configure progressive loading for smooth interaction
    options.progressive.enabled = true;
    options.progressive.lodDistances[0] = 10.0;   // Very close - highest quality
    options.progressive.lodDistances[1] = 50.0;   // Close - high quality
    options.progressive.lodDistances[2] = 100.0;  // Medium distance - balanced
    options.progressive.lodDistances[3] = 500.0;  // Far - low quality
    options.progressive.lodDeflections[0] = 0.1;  // Highest quality deflection
    options.progressive.lodDeflections[1] = 0.5;  // High quality
    options.progressive.lodDeflections[2] = 1.0;  // Balanced
    options.progressive.lodDeflections[3] = 2.0;  // Low quality
    options.progressive.streamLargeFiles = true;
    options.progressive.streamThreshold = 50 * 1024 * 1024; // 50MB
    
    // Enable caching for repeated imports
    options.enableCache = true;
    options.maxCacheSize = 512 * 1024 * 1024; // 512MB cache
    
    // Enable prefetching for smaller files
    options.enablePrefetch = true;
    
    // Enable compression for cache storage
    options.enableCompression = false; // Disabled for now, can be implemented later
    
    // GPU acceleration disabled by default (requires additional setup)
    options.enableGPUAcceleration = false;
    
    LOG_INF_S("Enhanced import settings applied with multi-threading and progressive loading");
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
    // Restore arrow cursor after import completion
    wxWindow* topWindow = wxTheApp->GetTopWindow();
    if (topWindow) {
        topWindow->SetCursor(wxCursor(wxCURSOR_ARROW));
        LOG_INF_S("Restored arrow cursor after geometry import");
    }

    if (m_statusBar) {
        try {
            m_statusBar->EnableProgressGauge(false);
            m_statusBar->SetStatusText("Ready", 0);
        }
        catch (...) {}
    }
}

bool ImportGeometryListener::shouldUseProgressiveLoading(const std::string& filePath, size_t fileSize) {
    // Progressive loading threshold
    const size_t PROGRESSIVE_THRESHOLD = 50 * 1024 * 1024; // 50MB
    
    if (fileSize <= PROGRESSIVE_THRESHOLD) {
        return false;
    }
    
    return StreamingFileReader::supportsStreaming(filePath);
}

bool ImportGeometryListener::importWithProgressiveLoading(const std::string& filePath,
    const GeometryReader::OptimizationOptions& options,
    std::vector<std::shared_ptr<OCCGeometry>>& allGeometries)
{
    auto loader = std::make_unique<ProgressiveGeometryLoader>();
    
    ProgressiveGeometryLoader::LoadingConfiguration config;
    config.filePath = filePath;
    config.streamConfig.maxMemoryUsage = 1024 * 1024 * 1024;
    config.streamConfig.chunkSize = StreamingFileReader::getOptimalChunkSize(
        std::filesystem::file_size(filePath));
    config.streamConfig.maxShapesPerChunk = 100;
    config.maxConcurrentChunks = 2;
    config.renderBatchSize = 50;
    config.autoStartRendering = true;
    config.enableMemoryManagement = true;
    config.targetFrameRate = 30.0;
    
    ProgressiveGeometryLoader::Callbacks callbacks;
    
    callbacks.onProgress = [this](double progress) {
        if (m_statusBar) {
            int percent = static_cast<int>(progress * 100.0);
            m_statusBar->SetGaugeValue(percent);
            wxYield();
        }
    };
    
    callbacks.onStateChanged = [this](ProgressiveGeometryLoader::LoadingState state, 
                                     const std::string& message) {
        if (m_statusBar) {
            m_statusBar->SetStatusText(message, 0);
            wxYield();
        }
    };
    
    callbacks.onChunkRendered = [this, &allGeometries, &filePath, &options]
        (const ProgressiveGeometryLoader::RenderChunk& chunk) {
        std::string baseName = std::filesystem::path(filePath).stem().string();
        
        for (size_t i = 0; i < chunk.shapes.size(); ++i) {
            const auto& shape = chunk.shapes[i];
            if (shape.IsNull()) {
                continue;
            }
            
            // Apply decomposition if enabled
            std::vector<TopoDS_Shape> shapesToProcess;
            if (options.decomposition.enableDecomposition) {
                // Decompose the shape according to options
                shapesToProcess = STEPGeometryDecomposer::decomposeShape(shape, options);
            } else {
                // No decomposition - use original shape
                shapesToProcess.push_back(shape);
            }
            
            // Process each decomposed shape
            for (size_t j = 0; j < shapesToProcess.size(); ++j) {
                const auto& shapeToProcess = shapesToProcess[j];
                if (shapeToProcess.IsNull()) {
                    continue;
                }
                
                // Generate name for geometry
                std::string name = baseName + "_chunk" + std::to_string(chunk.chunkIndex) +
                                 "_" + std::to_string(i);
                if (shapesToProcess.size() > 1) {
                    name += "_part" + std::to_string(j + 1);
                }

                // Create geometry using converter - it will apply decomposition color scheme automatically
                auto geometry = STEPGeometryConverter::processSingleShape(
                    shapeToProcess, name, baseName, options);

                if (geometry) {
                allGeometries.push_back(geometry);
                }
            }
        }

        LOG_INF_S("Progressive loading: rendered chunk " + std::to_string(chunk.chunkIndex) +
                 " with " + std::to_string(chunk.shapes.size()) + " shapes, total geometries: " +
                 std::to_string(allGeometries.size()));
    };
    
    callbacks.onError = [](const std::string& error) {
        LOG_ERR_S("Progressive loading error: " + error);
    };
    
    bool success = loader->startLoading(config, callbacks);
    
    if (!success) {
        LOG_ERR_S("Failed to start progressive loading for: " + filePath);
        return false;
    }
    
    LOG_INF_S("Progressive loading started, waiting for completion");
    
    // Wait for loading to complete with timeout
    auto startWait = std::chrono::steady_clock::now();
    const auto MAX_WAIT_TIME = std::chrono::minutes(10); // 10 minute timeout
    
    while (loader->getState() == ProgressiveGeometryLoader::LoadingState::Loading ||
           loader->getState() == ProgressiveGeometryLoader::LoadingState::Preparing ||
           loader->getState() == ProgressiveGeometryLoader::LoadingState::Rendering) {
        
        wxYield();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - startWait;
        if (elapsed > MAX_WAIT_TIME) {
            LOG_ERR_S("Progressive loading timeout after " + 
                     std::to_string(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()) + 
                     " seconds");
            loader->cancelLoading();
            return false;
        }
        
        // Log state every 5 seconds for debugging
        static auto lastLog = std::chrono::steady_clock::now();
        if (std::chrono::steady_clock::now() - lastLog > std::chrono::seconds(5)) {
            LOG_INF_S("Still loading... State: " + std::to_string(static_cast<int>(loader->getState())) +
                     ", Progress: " + std::to_string(loader->getProgress() * 100.0) + "%");
            lastLog = std::chrono::steady_clock::now();
        }
    }
    
    auto finalState = loader->getState();
    LOG_INF_S("Progressive loading finished with state: " + std::to_string(static_cast<int>(finalState)));

    // Don't add to viewer here - let the main import flow handle it
    // This prevents duplicate addition and ensures consistent batch handling
    LOG_INF_S("Progressive loading completed with " + std::to_string(allGeometries.size()) + " geometries");

    return finalState == ProgressiveGeometryLoader::LoadingState::Completed;
}
