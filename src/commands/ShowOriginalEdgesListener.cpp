#include "ShowOriginalEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "OriginalEdgesParamDialog.h"
#include "edges/EdgeExtractionUIHelper.h"
#include "edges/AsyncIntersectionManager.h"
#include "FlatFrame.h"
#include "logger/Logger.h"
#include <wx/frame.h>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

ShowOriginalEdgesListener::ShowOriginalEdgesListener(OCCViewer* viewer, wxFrame* frame) 
	: m_viewer(viewer), m_frame(frame) 
{
	if (m_frame) {
		FlatFrame* flatFrame = dynamic_cast<FlatFrame*>(m_frame);
		if (flatFrame) {
			m_intersectionManager = std::make_shared<AsyncIntersectionManager>(
				m_frame,
				flatFrame->GetFlatUIStatusBar(),
				nullptr
			);
			LOG_INF_S("AsyncIntersectionManager initialized for ShowOriginalEdgesListener");
		}
	}
}

CommandResult ShowOriginalEdgesListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) {
		return CommandResult(false, "OCCViewer not available", commandType);
	}

	// Toggle logic: if currently enabled -> disable; if disabled -> open param dialog
	const bool currentlyEnabled = m_viewer->isEdgeTypeEnabled(EdgeType::Original);
	if (!currentlyEnabled) {
		// Open parameter dialog to get parameters
		OriginalEdgesParamDialog dialog(m_frame);
		if (dialog.ShowModal() == wxID_OK) {
			// Get parameters from dialog
			double samplingDensity = dialog.getSamplingDensity();
			double minLength = dialog.getMinLength();
			bool showLinesOnly = dialog.getShowLinesOnly();
			wxColour edgeColor = dialog.getEdgeColor();
			double edgeWidth = dialog.getEdgeWidth();
			bool highlightIntersectionNodes = dialog.getHighlightIntersectionNodes();
			wxColour intersectionNodeColor = dialog.getIntersectionNodeColor();
			double intersectionNodeSize = dialog.getIntersectionNodeSize();
			IntersectionNodeShape intersectionNodeShape = dialog.getIntersectionNodeShape();

			LOG_INF_S("Original edges parameters: density=" + std::to_string(samplingDensity) +
				", minLength=" + std::to_string(minLength) +
				", linesOnly=" + std::string(showLinesOnly ? "true" : "false") +
				", width=" + std::to_string(edgeWidth) +
				", highlightNodes=" + std::string(highlightIntersectionNodes ? "true" : "false") +
				", nodeSize=" + std::to_string(intersectionNodeSize));

			// Create UI helper for progress tracking
			EdgeExtractionUIHelper uiHelper(m_frame);
			uiHelper.beginOperation("Extracting Original Edges");

			try {
				// Apply parameters to viewer (without intersection highlighting initially)
				m_viewer->setOriginalEdgesParameters(samplingDensity, minLength, showLinesOnly, edgeColor, edgeWidth,
					highlightIntersectionNodes, intersectionNodeColor, intersectionNodeSize, intersectionNodeShape);

				// Enable original edges display first
				uiHelper.updateProgress(30, "Displaying original edges...");
				m_viewer->setShowOriginalEdges(true);

				// Progressive display: now enabled with multi-geometry async computation
			if (highlightIntersectionNodes) {
				LOG_INF_S("Progressive display: enabling multi-geometry async intersection computation");
				
				// Use EdgeDisplayManager's multi-geometry async computation
				auto edgeDisplayManager = m_viewer->getEdgeDisplayManager();
				
				// Get async engine from FlatFrame (not OCCViewer, to ensure proper main thread event handling)
				async::AsyncEngineIntegration* asyncEngine = nullptr;
				if (m_frame) {
					// Cast to FlatFrame to access async engine
					auto* flatFrame = dynamic_cast<FlatFrame*>(m_frame);
					if (flatFrame) {
						asyncEngine = flatFrame->getAsyncEngine();
						LOG_INF_S("Progressive display: Using FlatFrame's async engine");
					}
				}
				
				if (!asyncEngine) {
					// Fallback to OCCViewer's engine (headless mode)
					asyncEngine = m_viewer->getAsyncEngine();
					LOG_WRN_S("Progressive display: Using OCCViewer's async engine (headless mode)");
				}
				
				if (edgeDisplayManager && asyncEngine) {
					// Completion callback for all geometries
					auto onComplete = [this](size_t totalPoints, bool success) {
						if (success) {
							LOG_INF_S("Multi-geometry intersection computation completed: " + 
							         std::to_string(totalPoints) + " total intersections found");
							m_viewer->requestViewRefresh();
						} else {
							LOG_ERR_S("Multi-geometry intersection computation failed");
						}
					};
					
					// Progress callback
					auto onProgress = [this](int progress, const std::string& message) {
						LOG_INF_S("Intersection progress: " + std::to_string(progress) + "% - " + message);
					};
					
					// Start multi-geometry async intersection computation
					edgeDisplayManager->computeIntersectionsAsync(
						0.001,  // tolerance
						asyncEngine,
						onComplete,
						onProgress
					);
				} else {
					LOG_ERR_S("EdgeDisplayManager or AsyncEngine not available, skipping intersection computation");
				}
			}

				uiHelper.updateProgress(90, "Finalizing edge display...");

				// Collect statistics from all geometries
				EdgeExtractionUIHelper::Statistics totalStats;
				auto geometries = m_viewer->getAllGeometry();
				
				auto startTime = std::chrono::high_resolution_clock::now();
				
				for (const auto& geom : geometries) {
					if (geom) {
						// Count edges using TopExp_Explorer
						int edgeCount = 0;
						for (TopExp_Explorer exp(geom->getShape(), TopAbs_EDGE); exp.More(); exp.Next()) {
							edgeCount++;
						}
						totalStats.totalEdges += edgeCount;
					}
				}
				
				auto endTime = std::chrono::high_resolution_clock::now();
				totalStats.extractionTime = std::chrono::duration<double>(endTime - startTime).count();
				totalStats.processedEdges = totalStats.totalEdges;
				uiHelper.setStatistics(totalStats);
				
				uiHelper.endOperation();

				return CommandResult(true, "Original edges shown with progressive intersection display", commandType);
			}
			catch (const std::exception& e) {
				uiHelper.endOperation();
				LOG_ERR_S("Error extracting original edges: " + std::string(e.what()));
				return CommandResult(false, "Failed to extract original edges: " + std::string(e.what()), commandType);
			}
		}
		else {
			// User cancelled, don't enable original edges
			return CommandResult(true, "Original edges display cancelled", commandType);
		}
	}
	else {
		// Disable original edges
		m_viewer->setShowOriginalEdges(false);
		return CommandResult(true, "Original edges hidden", commandType);
	}
}

bool ShowOriginalEdgesListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ShowOriginalEdges);
}