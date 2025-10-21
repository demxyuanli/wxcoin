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
					false, intersectionNodeColor, intersectionNodeSize, intersectionNodeShape);

				// Enable original edges display first
				uiHelper.updateProgress(30, "Displaying original edges...");
				m_viewer->setShowOriginalEdges(true);

				// Progressive display: now enabled with OCCGeometry incremental API
				if (highlightIntersectionNodes) {
					LOG_INF_S("Progressive display: enabling async intersection computation");
					
					// Clear any existing intersection nodes
					auto geometries = m_viewer->getAllGeometry();
					for (auto& geom : geometries) {
						geom->clearIntersectionNodes();
					}
					
					// Start async intersection computation with progressive display
					for (auto& geom : geometries) {
						if (!geom->getShape().IsNull()) {
						// Capture node size for lambda
						double nodeSize = intersectionNodeSize;
						
						// Partial results callback: progressive rendering
						auto onPartialResults = [this, geom, nodeSize](const std::vector<gp_Pnt>& points, size_t totalSoFar) {
							if (!points.empty()) {
								Quantity_Color intersectionColor(1.0, 0.0, 0.0, Quantity_TOC_RGB);
								geom->addBatchIntersectionNodes(points, intersectionColor, nodeSize);
								
								// Update edge display to add intersection nodes to scene graph
								geom->updateEdgeDisplay();
								
								// Request view refresh to render the new nodes
								m_viewer->requestViewRefresh();
								
								LOG_INF_S("Progressive display: rendered " + std::to_string(points.size()) + 
										 " points, total so far: " + std::to_string(totalSoFar));
							}
						};
							
							// Completion callback: final status update
							auto onCompleted = [this, geom](const std::vector<gp_Pnt>& points) {
								LOG_INF_S("Progressive display: intersection computation completed for " + geom->getName() + 
										 " (" + std::to_string(points.size()) + " points)");
							};
							
							// Error callback
							auto onError = [this, geom](const std::string& error) {
								LOG_ERR_S("Progressive display: intersection computation failed for " + geom->getName() + ": " + error);
							};
							
							// Start async computation with progressive display
							// Parameters: shape, tolerance, completionCallback, partialResultsCallback, batchSize
							m_intersectionManager->startIntersectionComputation(
								geom->getShape(),
								0.0,  // adaptive tolerance
								onCompleted,
								onPartialResults,
								50  // Batch size for progressive display
							);
						}
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