#include "ImportStepListener.h"
#include "CommandDispatcher.h"
#include "STEPReader.h"
#include "GeometryReader.h"
#include "OCCViewer.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include <wx/timer.h>
#include "logger/Logger.h"
#include <chrono>
#include "FlatFrame.h"
#include "ImportStatisticsDialog.h"
#include "flatui/FlatUIStatusBar.h"

// OpenCASCADE headers for topology analysis
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopAbs_ShapeEnum.hxx>
#include <OpenCASCADE/BRepCheck_Analyzer.hxx>
#include <OpenCASCADE/ShapeAnalysis_ShapeContents.hxx>

ImportStepListener::ImportStepListener(wxFrame* frame, Canvas* canvas, OCCViewer* occViewer)
	: m_frame(frame), m_canvas(canvas), m_occViewer(occViewer), m_statusBar(nullptr)
{
	if (!m_frame) {
		LOG_ERR_S("ImportStepListener: frame pointer is null");
		return;
	}

	// Try to get the status bar from the frame
	FlatFrame* flatFrame = dynamic_cast<FlatFrame*>(frame);
	if (flatFrame) {
		m_statusBar = flatFrame->GetFlatUIStatusBar();
	}
	
	if (!m_statusBar) {
		LOG_WRN_S("ImportStepListener: Could not find FlatUIStatusBar, progress will not be shown");
	}
}

CommandResult ImportStepListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	auto totalImportStartTime = std::chrono::high_resolution_clock::now();

	// Initialize overall statistics
	ImportOverallStatistics overallStats;

	// Initialize progress using status bar
	if (m_statusBar) {
		m_statusBar->EnableProgressGauge(true);
		m_statusBar->SetGaugeRange(100);
		m_statusBar->SetGaugeValue(0);
		m_statusBar->SetStatusText("STEP import started...", 0);
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
		if (m_statusBar) {
			m_statusBar->EnableProgressGauge(false);
			m_statusBar->SetStatusText("Ready", 0);
		}
		return CommandResult(false, "STEP import cancelled", commandType);
	}

	wxArrayString filePaths;
	openFileDialog.GetPaths(filePaths);

	// Set loading cursor for STEP import process
	wxWindow* topWindow = wxTheApp->GetTopWindow();
	if (topWindow) {
		topWindow->SetCursor(wxCursor(wxCURSOR_WAIT));
		LOG_INF_S("Set loading cursor for STEP import");
	}

	// Use balanced default settings for import (no dialog needed)
	// Balanced preset: Good balance between quality and performance
	double meshDeflection = 1.0;          // Balanced mesh precision
	double angularDeflection = 1.0;       // Balanced curve approximation
	bool enableLOD = true;                // Enable LOD for better interaction
	bool parallelProcessing = true;       // Enable parallel processing
	bool adaptiveMeshing = false;         // Disable adaptive meshing for consistency
	bool autoOptimize = true;             // Enable auto optimization
	bool normalProcessing = false;        // Disable normal processing for performance
	int importMode = 0;                   // Standard import mode
	
	// Tessellation settings for balanced quality
	bool enableFineTessellation = true;   // Enable fine tessellation
	double tessellationDeflection = 0.01; // Fine tessellation precision
	double tessellationAngle = 0.1;       // Tessellation angle
	int tessellationMinPoints = 3;        // Minimum tessellation points
	int tessellationMaxPoints = 100;      // Maximum tessellation points
	bool enableAdaptiveTessellation = true; // Enable adaptive tessellation

	LOG_INF_S(wxString::Format("Import settings: Deflection=%.2f, Angular=%.2f, LOD=%s, Parallel=%s",
		meshDeflection, angularDeflection,
		enableLOD ? "On" : "Off",
		parallelProcessing ? "On" : "Off"));
	auto fileDialogEndTime = std::chrono::high_resolution_clock::now();
	auto fileDialogDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fileDialogEndTime - fileDialogStartTime);

	LOG_INF_S("=== BATCH GEOMETRY IMPORT START ===");
	LOG_INF_S("Files selected: " + std::to_string(filePaths.size()) + ", Using balanced default settings");
	LOG_INF_S("Balanced settings applied: Deflection=1.0, Angular=1.0, LOD=On, Parallel=On");
	if (flatFrame) {
		flatFrame->appendMessage(wxString::Format("Files selected: %zu, using balanced quality settings", filePaths.size()));
	}

	try {
		// Setup optimization options will be created per reader below

		// Process all selected files
		std::vector<std::shared_ptr<OCCGeometry>> allGeometries;
		int successfulFiles = 0;
		int totalGeometries = 0;
		double totalImportTime = 0.0;
		const int totalPhases = (int)filePaths.size() + 1; // +1 for scene add phase

		for (size_t i = 0; i < filePaths.size(); ++i) {
			const wxString& filePath = filePaths[i];
			std::string filePathStr = filePath.ToStdString();

			// Get appropriate reader for this file
			auto reader = GeometryReaderFactory::getReaderForFile(filePathStr);
			if (!reader) {
				LOG_ERR_S("No suitable reader found for file: " + filePathStr);
				if (flatFrame) {
					flatFrame->appendMessage(wxString::Format("Unsupported file format: %s", filePath));
				}
				continue;
			}

			std::string formatName = reader->getFormatName();
			if (flatFrame) {
				flatFrame->appendMessage(wxString::Format("Reading %s file (%d/%d): %s",
					formatName, static_cast<int>(i + 1), static_cast<int>(filePaths.size()), filePath));
			}

			auto stepReadStartTime = std::chrono::high_resolution_clock::now();

			// Create optimization options for the reader
			GeometryReader::OptimizationOptions readerOptions;
			readerOptions.enableParallelProcessing = parallelProcessing;
			readerOptions.enableShapeAnalysis = adaptiveMeshing;
			readerOptions.enableCaching = true;
			readerOptions.enableBatchOperations = true;
			readerOptions.maxThreads = std::thread::hardware_concurrency();
			readerOptions.precision = 0.01;
			readerOptions.meshDeflection = meshDeflection;
			readerOptions.angularDeflection = angularDeflection;
			readerOptions.enableNormalProcessing = normalProcessing;
			
			// Set new tessellation options
			readerOptions.enableFineTessellation = enableFineTessellation;
			readerOptions.tessellationDeflection = tessellationDeflection;
			readerOptions.tessellationAngle = tessellationAngle;
			readerOptions.tessellationMinPoints = tessellationMinPoints;
			readerOptions.tessellationMaxPoints = tessellationMaxPoints;
			readerOptions.enableAdaptiveTessellation = enableAdaptiveTessellation;

			auto result = reader->readFile(filePathStr, readerOptions,
				[this, flatFrame, i, &filePaths](int percent, const std::string& stage) {
					try {
						int base = (int)std::round(((double)i / (double)(filePaths.size() + 1)) * 100.0);
						int next = (int)std::round(((double)(i + 1) / (double)(filePaths.size() + 1)) * 100.0);
						int mapped = base + (int)std::round((percent / 100.0) * (next - base));
						mapped = std::max(0, std::min(95, mapped));

						// Update status bar progress
						if (m_statusBar) {
							wxString progressMsg = wxString::Format("File %d/%d: %s",
								static_cast<int>(i + 1), static_cast<int>(filePaths.size()), stage.c_str());
							m_statusBar->SetGaugeValue(mapped);
							m_statusBar->SetStatusText(progressMsg, 0);
							// Force UI update
							m_statusBar->Refresh();
							wxYield(); // Allow UI to update

							
							// Small delay to make progress visible
							wxMilliSleep(50);
						}

						if (flatFrame) {
							try {
								flatFrame->appendMessage(wxString::Format("[%d%%] Import stage: %s", mapped, stage));
							}
							catch (...) {
								// Ignore message append errors
							}
						}
					}
					catch (...) {
						// Ignore all errors in progress callback
					}
				}
			);
			auto stepReadEndTime = std::chrono::high_resolution_clock::now();
			auto stepReadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(stepReadEndTime - stepReadStartTime);

			if (result.success && !result.geometries.empty()) {
				allGeometries.insert(allGeometries.end(), result.geometries.begin(), result.geometries.end());
				successfulFiles++;
				totalGeometries += result.geometries.size();
				totalImportTime += result.importTime;

				// Collect detailed statistics for this file
				ImportFileStatistics fileStat;
				wxFileName wxFile(filePaths[i]);
				fileStat.fileName = wxFile.GetFullName().ToStdString();
				fileStat.filePath = filePaths[i].ToStdString();
				fileStat.format = formatName;
				fileStat.success = true;
				fileStat.geometriesCreated = result.geometries.size();
				fileStat.importTime = stepReadDuration;
				fileStat.fileSize = wxFileName(filePaths[i]).GetSize().ToULong();

				// Extract detailed information from the geometry
				if (!result.geometries.empty()) {
					auto& geometry = result.geometries[0];
					collectGeometryDetails(geometry, fileStat);
				}

				// Store file statistics
				overallStats.fileStats.push_back(fileStat);

				// Update overall statistics
				overallStats.totalTransferableRoots += fileStat.transferableRoots;
				overallStats.totalTransferredShapes += fileStat.transferredShapes;
				overallStats.totalFacesProcessed += fileStat.facesProcessed;
				overallStats.totalSolids += fileStat.solids;
				overallStats.totalShells += fileStat.shells;
				overallStats.totalFaces += fileStat.faces;
				overallStats.totalWires += fileStat.wires;
				overallStats.totalEdges += fileStat.edges;
				overallStats.totalVertices += fileStat.vertices;
				overallStats.totalMeshVertices += fileStat.meshVertices;
				overallStats.totalMeshTriangles += fileStat.meshTriangles;

				LOG_INF_S("File " + std::to_string(i + 1) + "/" + std::to_string(filePaths.size()) +
					": " + std::to_string(result.geometries.size()) + " geometries in " +
					std::to_string(stepReadDuration.count()) + "ms");
				if (flatFrame) {
					flatFrame->appendMessage(wxString::Format("Parsed %zu geometries in %lld ms",
						result.geometries.size(), (long long)stepReadDuration.count()));
					// Assembly detection: if more than one geometry extracted, treat as assembly
					if (result.geometries.size() > 1) {
						flatFrame->appendMessage(wxString::Format("Assembly detected: %zu parts", result.geometries.size()));
					}
					else {
						flatFrame->appendMessage("Single part detected");
					}
				}
			}
			else {
				LOG_WRN_S("File " + std::to_string(i + 1) + "/" + std::to_string(filePaths.size()) +
					" failed: " + (result.success ? "No geometries" : result.errorMessage));
				if (flatFrame) {
					wxString errorMsg = result.success ? "No geometries" : result.errorMessage;
					flatFrame->appendMessage(wxString::Format("Failed to parse: %s", errorMsg));

					// Provide helpful tips for common errors
					if (errorMsg.Contains("Construction") || errorMsg.Contains("construction")) {
						flatFrame->appendMessage("  Tip: The file may contain invalid or degenerate geometry.");
						flatFrame->appendMessage("  Try checking the file in the original CAD software.");
					}
				}
			}
			// Update coarse progress after each file
			if (m_statusBar) {
				int percent = (int)std::round(((double)(i + 1) / (double)totalPhases) * 100.0);
				percent = std::max(0, std::min(95, percent)); // cap before add phase
				m_statusBar->SetGaugeValue(percent);
				m_statusBar->SetStatusText(wxString::Format("Processed %d/%d files",
					static_cast<int>(i + 1), static_cast<int>(filePaths.size())), 0);
				// Force UI update
				m_statusBar->Refresh();
				wxYield(); // Allow UI to update
			}
			
			// Also update message panel with progress
			if (flatFrame) {
				int percent = (int)std::round(((double)(i + 1) / (double)totalPhases) * 100.0);
				percent = std::max(0, std::min(95, percent)); // cap before add phase
				flatFrame->appendMessage(wxString::Format("[%d%%] Processed %d/%d files", percent,
					static_cast<int>(i + 1), static_cast<int>(filePaths.size())));
			}
		}

		if (!allGeometries.empty() && m_occViewer) {
			// Add all geometries using batch operations
			auto geometryAddStartTime = std::chrono::high_resolution_clock::now();
			// Use intelligent deflection based on geometry size
			double prevDeflection = m_occViewer->getMeshDeflection();

			// Calculate bounding box to determine appropriate deflection
			gp_Pnt minPt, maxPt;
			bool hasBounds = STEPReader::calculateCombinedBoundingBox(allGeometries, minPt, maxPt);

			double optimalDeflection = meshDeflection; // Use user-selected deflection

			// If auto-optimize is enabled, adjust based on geometry size
			if (autoOptimize && hasBounds) {
				// Calculate diagonal of bounding box
				double dx = maxPt.X() - minPt.X();
				double dy = maxPt.Y() - minPt.Y();
				double dz = maxPt.Z() - minPt.Z();
				double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);

				// Adjust user's deflection based on size
				double sizeFactor = 1.0;
				if (diagonal < 10.0) {
					sizeFactor = 0.5; // Finer for small objects
				}
				else if (diagonal > 1000.0) {
					sizeFactor = 2.0; // Coarser for very large objects
				}

				optimalDeflection = meshDeflection * sizeFactor;

				// Clamp to reasonable range
				optimalDeflection = std::max(0.001, std::min(10.0, optimalDeflection));

				LOG_INF_S("Auto-optimization: Bounding box diagonal: " + std::to_string(diagonal) +
					", adjusted deflection: " + std::to_string(optimalDeflection) +
					" (base: " + std::to_string(meshDeflection) + ")");
			}

			m_occViewer->beginBatchOperation();
			m_occViewer->setMeshDeflection(optimalDeflection, false);

			// Apply user settings
			m_occViewer->setLODEnabled(enableLOD);
			if (parallelProcessing) {
				m_occViewer->setParallelProcessing(true);
			}
			if (adaptiveMeshing) {
				m_occViewer->setAdaptiveMeshing(true);
			}

			// Add geometries
			m_occViewer->addGeometries(allGeometries);
			m_occViewer->endBatchOperation();
			m_occViewer->updateObjectTreeDeferred();
			if (m_statusBar) {
				m_statusBar->SetGaugeValue(98);
				m_statusBar->SetStatusText("Adding geometries to scene...", 0);
			}
			
			// Also update message panel with progress
			if (flatFrame) {
				flatFrame->appendMessage("[98%] Adding geometries to scene...");
			}
			// Optional: request a background-style refresh to refine mesh gradually (next user action can trigger remeshAllGeometries)
			auto geometryAddEndTime = std::chrono::high_resolution_clock::now();
			auto geometryAddDuration = std::chrono::duration_cast<std::chrono::milliseconds>(geometryAddEndTime - geometryAddStartTime);
			if (flatFrame) {
				flatFrame->appendMessage(wxString::Format("Added %zu geometries to scene in %lld ms",
					allGeometries.size(), (long long)geometryAddDuration.count()));
			}

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

			// Ensure progress is complete before showing dialog
			if (m_statusBar) {
				m_statusBar->SetGaugeValue(100);
				m_statusBar->SetStatusText("Import completed!", 0);
				// Hide progress bar after a short delay
				wxTimer* hideTimer = new wxTimer();
				hideTimer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
					if (m_statusBar) {
						m_statusBar->EnableProgressGauge(false);
						m_statusBar->SetStatusText("Ready", 0);
					}
				});
				hideTimer->StartOnce(2000); // Hide after 2 seconds
			}
			
			// Also update message panel with progress
			if (flatFrame) {
				flatFrame->appendMessage("[100%] Import completed!");
			}
			if (flatFrame) {
				flatFrame->appendMessage("STEP import completed.");
			}

			// Use the top-level window for the dialog to ensure it's visible
			wxWindow* topWindow = wxTheApp->GetTopWindow();
			if (!topWindow) {
				topWindow = m_frame;
			}

			// Restore arrow cursor after STEP import completion
			if (topWindow) {
				topWindow->SetCursor(wxCursor(wxCURSOR_ARROW));
				LOG_INF_S("Restored arrow cursor after STEP import");
			}

			// Update overall statistics with final summary information
			overallStats.totalFilesSelected = filePaths.size();
			overallStats.totalFilesProcessed = filePaths.size();
			overallStats.totalSuccessfulFiles = successfulFiles;
			overallStats.totalFailedFiles = filePaths.size() - successfulFiles;
			overallStats.totalGeometriesCreated = totalGeometries;
			overallStats.totalImportTime = std::chrono::milliseconds(static_cast<long long>(totalImportTime));
			overallStats.totalGeometryAddTime = geometryAddDuration.count();
			overallStats.averageGeometriesPerSecond = totalGeometries / (totalImportTime / 1000.0);

			// Get system settings
			overallStats.lodEnabled = enableLOD;
			overallStats.adaptiveMeshingEnabled = adaptiveMeshing;
			overallStats.meshDeflection = optimalDeflection;

			// Add format statistics for STEP
			ImportFormatStatistics& formatStat = overallStats.formatStats["STEP"];
			formatStat.totalFiles = filePaths.size();
			formatStat.successfulFiles = successfulFiles;
			formatStat.failedFiles = filePaths.size() - successfulFiles;
			formatStat.totalGeometries = totalGeometries;
			formatStat.totalImportTime = std::chrono::milliseconds(static_cast<long long>(totalImportTime));

			ImportStatisticsDialog statsDialog(topWindow, overallStats);
			int result = statsDialog.ShowModal();
			LOG_INF_S("Import statistics dialog closed with result: " + std::to_string(result));

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

			return CommandResult(true, "Geometry files imported successfully", commandType);
		}
		else {
			wxString warningMsg = wxString::Format(
				"No valid geometries found in selected files.\n\n"
				"Files processed: %zu/%zu\n"
				"Successful files: %d",
				static_cast<size_t>(filePaths.size()), static_cast<size_t>(filePaths.size()), successfulFiles
			); 

			// Ensure progress is cleaned up
			if (m_statusBar) {  
				try { 
					m_statusBar->EnableProgressGauge(false);
					m_statusBar->SetStatusText("Ready", 0);
				}
				catch (...) {}
			}

			// Use top-level window for dialog
			wxWindow* topWindow = wxTheApp->GetTopWindow();
			if (!topWindow) {
				topWindow = m_frame;
			}

			// Show detailed statistics dialog for failed imports
			ImportOverallStatistics overallStats;
			overallStats.totalFilesSelected = filePaths.size();
			overallStats.totalFilesProcessed = filePaths.size();
			overallStats.totalSuccessfulFiles = successfulFiles;
			overallStats.totalFailedFiles = filePaths.size() - successfulFiles;
			overallStats.totalGeometriesCreated = totalGeometries;
			overallStats.totalImportTime = std::chrono::milliseconds(static_cast<long long>(totalImportTime));

			// Add file statistics for all processed files (marking them as failed)
			for (size_t i = 0; i < filePaths.size(); ++i) {
				ImportFileStatistics fileStat;
				wxFileName wxFile(filePaths[i]);
				fileStat.fileName = wxFile.GetFullName().ToStdString();
				fileStat.filePath = filePaths[i].ToStdString();
				fileStat.format = "STEP";
				fileStat.success = false;
				fileStat.errorMessage = "No valid geometries found";
				fileStat.geometriesCreated = 0;
				fileStat.importTime = std::chrono::milliseconds(static_cast<long long>(totalImportTime / filePaths.size()));

				overallStats.fileStats.push_back(fileStat);
			}

			// Add format statistics for STEP
			ImportFormatStatistics& formatStat = overallStats.formatStats["STEP"];
			formatStat.totalFiles = filePaths.size();
			formatStat.successfulFiles = successfulFiles;
			formatStat.failedFiles = filePaths.size() - successfulFiles;
			formatStat.totalGeometries = totalGeometries;
			formatStat.totalImportTime = std::chrono::milliseconds(static_cast<long long>(totalImportTime));

			ImportStatisticsDialog statsDialog(topWindow, overallStats);
			int result = statsDialog.ShowModal();
			LOG_INF_S("Import statistics dialog closed with result: " + std::to_string(result));

			return CommandResult(false, "No valid geometries found in selected files", commandType);
		}
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception during STEP import: " + std::string(e.what()));

		// Restore arrow cursor on exception
		wxWindow* topWindow = wxTheApp->GetTopWindow();
		if (topWindow) {
			topWindow->SetCursor(wxCursor(wxCURSOR_ARROW));
			LOG_INF_S("Restored arrow cursor after STEP import exception");
		}

		// Ensure progress is cleaned up
		if (m_statusBar) {
			try {
				m_statusBar->EnableProgressGauge(false);
				m_statusBar->SetStatusText("Ready", 0);
			}
			catch (...) {}
		}

		// Use top-level window for dialog
		topWindow = wxTheApp->GetTopWindow();
		if (!topWindow) {
			topWindow = m_frame;
		}

		// Show detailed statistics dialog for exception errors
		ImportOverallStatistics overallStats;
		overallStats.totalFilesSelected = filePaths.size();
		overallStats.totalFilesProcessed = 0; // Exception occurred before processing
		overallStats.totalSuccessfulFiles = 0;
		overallStats.totalFailedFiles = 0;
		overallStats.totalGeometriesCreated = 0;
		overallStats.totalImportTime = std::chrono::milliseconds(0);

		// Add all files as failed due to exception
		for (size_t i = 0; i < filePaths.size(); ++i) {
			ImportFileStatistics fileStat;
			wxFileName wxFile(filePaths[i]);
			fileStat.fileName = wxFile.GetFullName().ToStdString();
			fileStat.filePath = filePaths[i].ToStdString();
			fileStat.format = "STEP";
			fileStat.success = false;
			fileStat.errorMessage = "Import failed due to exception: " + std::string(e.what());
			fileStat.geometriesCreated = 0;
			fileStat.importTime = std::chrono::milliseconds(0);

			overallStats.fileStats.push_back(fileStat);
		}

		ImportStatisticsDialog statsDialog(topWindow, overallStats);
		int result = statsDialog.ShowModal();
		LOG_INF_S("Import statistics dialog closed with result: " + std::to_string(result));

		return CommandResult(false, "Error importing STEP files: " + std::string(e.what()), commandType);
	}
}

void ImportStepListener::collectGeometryDetails(std::shared_ptr<OCCGeometry>& geometry, ImportFileStatistics& fileStat) {
	if (!geometry) return;

	try {
		// Get basic material information from OCCGeometry
		auto diffuseColor = geometry->getMaterialDiffuseColor();
		auto ambientColor = geometry->getMaterialAmbientColor();
		auto transparency = geometry->getTransparency();

		// Format color information
		fileStat.materialDiffuse = wxString::Format("%.3f,%.3f,%.3f",
			diffuseColor.Red(), diffuseColor.Green(), diffuseColor.Blue()).ToStdString();
		fileStat.materialAmbient = wxString::Format("%.3f,%.3f,%.3f",
			ambientColor.Red(), ambientColor.Green(), ambientColor.Blue()).ToStdString();
		fileStat.materialTransparency = transparency;
		fileStat.textureEnabled = geometry->isTextureEnabled();
		fileStat.blendMode = "Default";

		// Try to analyze the shape topology
		const TopoDS_Shape& shape = geometry->getShape();
		if (!shape.IsNull()) {
			// Count topology elements
			int solidCount = 0, shellCount = 0, faceCount = 0, wireCount = 0, edgeCount = 0, vertexCount = 0;

			// Use TopExp_Explorer to count elements
			TopExp_Explorer explorer;

			// Count solids
			for (explorer.Init(shape, TopAbs_SOLID); explorer.More(); explorer.Next()) {
				solidCount++;
			}

			// Count shells
			for (explorer.Init(shape, TopAbs_SHELL); explorer.More(); explorer.Next()) {
				shellCount++;
			}

			// Count faces
			for (explorer.Init(shape, TopAbs_FACE); explorer.More(); explorer.Next()) {
				faceCount++;
			}

			// Count wires
			for (explorer.Init(shape, TopAbs_WIRE); explorer.More(); explorer.Next()) {
				wireCount++;
			}

			// Count edges
			for (explorer.Init(shape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
				edgeCount++;
			}

			// Count vertices
			for (explorer.Init(shape, TopAbs_VERTEX); explorer.More(); explorer.Next()) {
				vertexCount++;
			}

			// Update file statistics
			fileStat.solids = solidCount;
			fileStat.shells = shellCount;
			fileStat.faces = faceCount;
			fileStat.wires = wireCount;
			fileStat.edges = edgeCount;
			fileStat.vertices = vertexCount;

			// Validate shape
			try {
				BRepCheck_Analyzer analyzer(shape);
				fileStat.shapeValid = analyzer.IsValid();
			} catch (...) {
				fileStat.shapeValid = false;
			}

			// Check if shape is closed (simplified check)
			fileStat.shapeClosed = (solidCount > 0); // If we have solids, assume closed
		} else {
			// Set defaults if shape is null
			fileStat.solids = 0;
			fileStat.shells = 0;
			fileStat.faces = 0;
			fileStat.wires = 0;
			fileStat.edges = 0;
			fileStat.vertices = 0;
			fileStat.shapeValid = false;
			fileStat.shapeClosed = false;
		}

		// Try to get mesh information from geometry's Coin3D representation
		try {
			auto coinNode = geometry->getCoinNode();
			if (coinNode) {
				// This is a simplified way to estimate mesh complexity
				// In a real implementation, we'd traverse the Coin3D scene graph
				// For now, we'll use some reasonable estimates based on face count
				if (fileStat.faces > 0) {
					// Estimate vertices and triangles based on face count
					// This is a rough approximation
					fileStat.meshVertices = fileStat.faces * 4; // Assume quad faces on average
					fileStat.meshTriangles = fileStat.faces * 2; // Assume each quad becomes 2 triangles
				}
			}
		} catch (...) {
			// Ignore errors when accessing mesh data
		}

		// Set STEP-specific information (these would come from STEP reader)
		fileStat.transferableRoots = 1;
		fileStat.transferredShapes = 1;
		fileStat.facesProcessed = fileStat.faces;
		fileStat.facesReversed = 0;

		// Performance information (would be measured during actual processing)
		fileStat.meshBuildTime = 0.0;
		fileStat.normalCalculationTime = 0.0;
		fileStat.normalSmoothingTime = 0.0;

	} catch (const std::exception& e) {
		LOG_WRN_S("Failed to collect geometry details: " + std::string(e.what()));
		// Set safe default values
		fileStat.solids = 0;
		fileStat.shells = 0;
		fileStat.faces = 0;
		fileStat.wires = 0;
		fileStat.edges = 0;
		fileStat.vertices = 0;
		fileStat.shapeValid = false;
		fileStat.shapeClosed = false;
		fileStat.meshVertices = 0;
		fileStat.meshTriangles = 0;
		fileStat.materialDiffuse = "0.950,0.950,0.950";
		fileStat.materialAmbient = "0.400,0.400,0.400";
		fileStat.materialTransparency = 0.0;
		fileStat.textureEnabled = false;
		fileStat.blendMode = "Default";
	}
}

bool ImportStepListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ImportSTEP);
}