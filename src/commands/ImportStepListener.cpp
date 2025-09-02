#include "ImportStepListener.h"
#include "CommandDispatcher.h"
#include "STEPReader.h"
#include "OCCViewer.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include "logger/Logger.h"
#include <chrono>
#include "FlatFrame.h"

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

	// Resolve status bar and message output
	FlatUIStatusBar* statusBar = nullptr;
	FlatFrame* flatFrame = nullptr;
	
	// Try to get FlatFrame - could be the frame itself or in docking environment
	flatFrame = dynamic_cast<FlatFrame*>(m_frame);
	if (!flatFrame) {
		// Try to find FlatFrame through the app's top window
		wxWindow* topWindow = wxTheApp->GetTopWindow();
		if (topWindow) {
			flatFrame = dynamic_cast<FlatFrame*>(topWindow);
		}
	}
	
	if (flatFrame) {
		try {
			statusBar = flatFrame->GetFlatUIStatusBar();
			if (statusBar) {
				statusBar->SetGaugeRange(100);
				statusBar->SetGaugeValue(0);
				statusBar->EnableProgressGauge(true); // show on demand at start
			}
		} catch (...) {
			LOG_WRN_S("Failed to access status bar, continuing without progress display");
			statusBar = nullptr;
		}
		flatFrame->appendMessage("STEP import started...");
	} else {
		LOG_WRN_S("FlatFrame not found, progress will not be displayed");
	}

	// File dialog
	auto fileDialogStartTime = std::chrono::high_resolution_clock::now();
	wxFileDialog openFileDialog(m_frame, "Import STEP Files", "", "",
		"STEP files (*.step;*.stp)|*.step;*.stp|All files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	if (openFileDialog.ShowModal() == wxID_CANCEL) {
		if (statusBar) { statusBar->SetGaugeValue(0); }
		return CommandResult(false, "STEP import cancelled", commandType);
	}

	wxArrayString filePaths;
	openFileDialog.GetPaths(filePaths);
	auto fileDialogEndTime = std::chrono::high_resolution_clock::now();
	auto fileDialogDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fileDialogEndTime - fileDialogStartTime);

	LOG_INF_S("=== BATCH STEP IMPORT START ===");
	LOG_INF_S("Files selected: " + std::to_string(filePaths.size()) + ", Dialog time: " + std::to_string(fileDialogDuration.count()) + "ms");
	if (flatFrame) {
		flatFrame->appendMessage(wxString::Format("Files selected: %zu", filePaths.size()));
	}

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
		const int totalPhases = (int)filePaths.size() + 1; // +1 for scene add phase

		for (size_t i = 0; i < filePaths.size(); ++i) {
			const wxString& filePath = filePaths[i];
			if (flatFrame) {
				flatFrame->appendMessage(wxString::Format("Reading STEP file (%zu/%zu): %s", i + 1, filePaths.size(), filePath));
			}
			auto stepReadStartTime = std::chrono::high_resolution_clock::now();
			auto result = STEPReader::readSTEPFile(
				filePath.ToStdString(), options,
				[statusBar, flatFrame, i, &filePaths](int percent, const std::string& stage) {
					try {
						int base = (int)std::round(((double)i / (double)(filePaths.size() + 1)) * 100.0);
						int next = (int)std::round(((double)(i + 1) / (double)(filePaths.size() + 1)) * 100.0);
						int mapped = base + (int)std::round((percent / 100.0) * (next - base));
						mapped = std::max(0, std::min(95, mapped));
						if (statusBar) {
							try {
								statusBar->SetGaugeValue(mapped);
							} catch (...) {
								// Ignore status bar errors
							}
						}
						if (flatFrame) {
							try {
								flatFrame->appendMessage(wxString::Format("[%d%%] Import stage: %s", mapped, stage));
							} catch (...) {
								// Ignore message append errors
							}
						}
					} catch (...) {
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
			if (statusBar) {
				int percent = (int)std::round(((double)(i + 1) / (double)totalPhases) * 100.0);
				percent = std::max(0, std::min(95, percent)); // cap before add phase
				statusBar->SetGaugeValue(percent);
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
			
			double optimalDeflection = prevDeflection;
			if (hasBounds) {
				// Calculate diagonal of bounding box
				double dx = maxPt.X() - minPt.X();
				double dy = maxPt.Y() - minPt.Y();
				double dz = maxPt.Z() - minPt.Z();
				double diagonal = std::sqrt(dx*dx + dy*dy + dz*dz);
				
				// Use 0.1% to 0.5% of diagonal as deflection
				// Smaller objects get finer mesh, larger objects get coarser mesh
				if (diagonal < 10.0) {
					optimalDeflection = diagonal * 0.001; // 0.1% for small objects
				} else if (diagonal < 100.0) {
					optimalDeflection = diagonal * 0.002; // 0.2% for medium objects
				} else {
					optimalDeflection = diagonal * 0.005; // 0.5% for large objects
				}
				
				// Clamp to reasonable range
				optimalDeflection = std::max(0.001, std::min(1.0, optimalDeflection));
				
				LOG_INF_S("Bounding box diagonal: " + std::to_string(diagonal) + 
					", using deflection: " + std::to_string(optimalDeflection));
			}

			m_occViewer->beginBatchOperation();
			m_occViewer->setMeshDeflection(optimalDeflection, false);
			
			// Enable LOD for better performance
			m_occViewer->setLODEnabled(true);
			
			// Add geometries
			m_occViewer->addGeometries(allGeometries);
			m_occViewer->endBatchOperation();
			if (statusBar) { statusBar->SetGaugeValue(98); }
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

			// Ensure status bar is updated before showing dialog
			if (statusBar) { 
				statusBar->SetGaugeValue(100); 
				statusBar->EnableProgressGauge(false); 
			}
			if (flatFrame) { 
				flatFrame->appendMessage("STEP import completed."); 
			}
			
			// Use the top-level window for the dialog to ensure it's visible
			wxWindow* topWindow = wxTheApp->GetTopWindow();
			if (!topWindow) {
				topWindow = m_frame;
			}
			
			// Show dialog with explicit parent and ensure it's on top
			wxMessageDialog* dialog = new wxMessageDialog(topWindow, performanceMsg, 
				"Batch Import Complete", wxOK | wxICON_INFORMATION | wxCENTRE);
			dialog->ShowModal();
			delete dialog;

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
		}
		else {
			wxString warningMsg = wxString::Format(
				"No valid geometries found in selected files.\n\n"
				"Files processed: %d/%zu\n"
				"Successful files: %d",
				filePaths.size(), filePaths.size(), successfulFiles
			);
			
			// Ensure status bar is cleaned up
			if (statusBar) { 
				try {
					statusBar->SetGaugeValue(0); 
					statusBar->EnableProgressGauge(false);
				} catch (...) {}
			}
			
			// Use top-level window for dialog
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
		LOG_ERR_S("Exception during STEP import: " + std::string(e.what()));
		
		// Ensure status bar is cleaned up
		if (statusBar) { 
			try {
				statusBar->SetGaugeValue(0); 
				statusBar->EnableProgressGauge(false);
			} catch (...) {}
		}
		
		// Use top-level window for dialog
		wxWindow* topWindow = wxTheApp->GetTopWindow();
		if (!topWindow) {
			topWindow = m_frame;
		}
		
		wxMessageDialog* dialog = new wxMessageDialog(topWindow, 
			"Error importing STEP files: " + std::string(e.what()),
			"Import Error", wxOK | wxICON_ERROR | wxCENTRE);
		dialog->ShowModal();
		delete dialog;
		
		return CommandResult(false, "Error importing STEP files: " + std::string(e.what()), commandType);
	}
}

bool ImportStepListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ImportSTEP);
}