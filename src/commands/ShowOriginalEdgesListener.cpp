#include "ShowOriginalEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "OriginalEdgesParamDialog.h"
#include "edges/EdgeExtractionUIHelper.h"
#include "edges/AsyncIntersectionManager.h"
#include "async/AsyncEngineIntegration.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "logger/AsyncLogger.h"
#include <wx/frame.h>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <memory>

ShowOriginalEdgesListener::ShowOriginalEdgesListener(OCCViewer* viewer, IAsyncEngine* asyncEngine, wxFrame* frame)
	: m_viewer(viewer), m_asyncEngine(asyncEngine), m_frame(frame)
{
	if (m_frame) {
		// Note: AsyncIntersectionManager is deprecated and should be removed
		// The async engine is now injected directly
		LOG_INF_S_ASYNC("ShowOriginalEdgesListener initialized with async engine");
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

			LOG_INF_S_ASYNC("Original edges parameters: density=" + std::to_string(samplingDensity) +
				", minLength=" + std::to_string(minLength) +
				", linesOnly=" + std::string(showLinesOnly ? "true" : "false") +
				", width=" + std::to_string(edgeWidth) +
				", highlightNodes=" + std::string(highlightIntersectionNodes ? "true" : "false") +
				", nodeSize=" + std::to_string(intersectionNodeSize));

			// Create UI helper for progress tracking
			// Use shared_ptr to ensure lifetime extends to async callbacks
			auto uiHelper = std::make_shared<EdgeExtractionUIHelper>(m_frame);
			uiHelper->beginOperation("Extracting Original Edges");

			try {
				// Apply parameters to viewer (without intersection highlighting initially)
				m_viewer->setOriginalEdgesParameters(samplingDensity, minLength, showLinesOnly, edgeColor, edgeWidth,
					highlightIntersectionNodes, intersectionNodeColor, intersectionNodeSize, intersectionNodeShape);

			// Step 1: Extract and display original edges only (without intersections)
			uiHelper->updateProgress(20, "Extracting original edges...");
			LOG_INF_S_ASYNC("ShowOriginalEdgesListener: Extracting original edges without intersections");

			// Extract edges without intersection highlighting (async)
			auto edgeDisplayManager = m_viewer->getEdgeDisplayManager();
			if (edgeDisplayManager) {
				// Convert wxColour to Quantity_Color
				Quantity_Color occColor(edgeColor.Red() / 255.0, edgeColor.Green() / 255.0, edgeColor.Blue() / 255.0, Quantity_TOC_RGB);
				Quantity_Color intersectionNodeOccColor(intersectionNodeColor.Red() / 255.0, intersectionNodeColor.Green() / 255.0, intersectionNodeColor.Blue() / 255.0, Quantity_TOC_RGB);

				// Async extraction with completion callback
				// Note: highlightIntersectionNodes and edgeDisplayManager must be captured by value, not by reference,
				// because the lambda is executed asynchronously and the local variable may be out of scope
				edgeDisplayManager->extractOriginalEdgesOnly(samplingDensity, minLength, showLinesOnly,
					occColor, edgeWidth, intersectionNodeOccColor, intersectionNodeSize, intersectionNodeShape,
					[this, uiHelper, highlightIntersectionNodes, edgeDisplayManager,
					 samplingDensity, minLength, showLinesOnly](bool success, const std::string& error) {

						if (success) {
							// Enable original edges display
							m_viewer->setShowOriginalEdges(true);

							// Force immediate refresh to show edges
							LOG_INF_S_ASYNC("ShowOriginalEdgesListener: Refreshing to show original edges");
							if (m_viewer->getSceneManager() && m_viewer->getSceneManager()->getCanvas()) {
								m_viewer->getSceneManager()->getCanvas()->Refresh();
							}
								m_viewer->requestViewRefresh();
							uiHelper->updateProgress(40, "Original edges displayed");
							LOG_INF_S_ASYNC("ShowOriginalEdgesListener: Original edges displayed successfully");

							// Continue to step 2 if intersection computation is needed
							LOG_INF_S_ASYNC("ShowOriginalEdgesListener: Checking intersection computation - highlightIntersectionNodes=" + 
								std::string(highlightIntersectionNodes ? "true" : "false") + 
								", edgeDisplayManager=" + (edgeDisplayManager ? "valid" : "null"));
							if (highlightIntersectionNodes && edgeDisplayManager) {
								LOG_INF_S_ASYNC("Progressive display: starting async intersection computation with batch display");

								// Use injected async engine (dependency injection pattern)
								IAsyncEngine* asyncEngine = m_asyncEngine;
								if (!asyncEngine) {
									// Fallback to OCCViewer's engine if not injected (for backward compatibility)
									if (m_viewer) {
										asyncEngine = m_viewer->getAsyncEngine();
									}
									if (asyncEngine) {
										LOG_WRN_S("Progressive display: Using OCCViewer's async engine (fallback mode)");
									}
								} else {
									LOG_INF_S_ASYNC("Progressive display: Using injected async engine");
								}

								// Double-check edgeDisplayManager is still valid before using it
								if (edgeDisplayManager && asyncEngine && m_viewer) {
									// Verify edgeDisplayManager is still the same instance
									auto currentEdgeDisplayManager = m_viewer->getEdgeDisplayManager();
									if (currentEdgeDisplayManager != edgeDisplayManager) {
										LOG_WRN_S_ASYNC("ShowOriginalEdgesListener: EdgeDisplayManager changed, skipping intersection computation");
										uiHelper->updateProgress(100, "EdgeDisplayManager changed, skipping intersection computation");
										return;
									}
									// Show indeterminate progress bar immediately when starting computation
									uiHelper->setIndeterminateProgress(true, "Computing intersections...");
									LOG_INF_S_ASYNC("Progressive display: Indeterminate progress bar shown, starting computation");

									// Completion callback for all geometries
									// Note: uiHelper is captured by value (shared_ptr) to ensure lifetime extends to async callbacks
									auto onComplete = [this, uiHelper](size_t totalPoints, bool success) {
										if (!uiHelper) return; // Safety check
										
										if (success) {
											LOG_INF_S_ASYNC("Multi-geometry intersection computation completed: " +
											         std::to_string(totalPoints) + " total intersections found");

											// Hide progress bar when computation is complete
											uiHelper->setIndeterminateProgress(false);
											uiHelper->updateProgress(100, "Intersection computation completed");
											LOG_INF_S_ASYNC("Progressive display: Progress bar hidden, computation completed");

											if (m_viewer) {
												m_viewer->requestViewRefresh();
											}
										} else {
											LOG_ERR_S("Multi-geometry intersection computation failed");
											uiHelper->setIndeterminateProgress(false);
											uiHelper->updateProgress(100, "Intersection computation failed");
										}
									};

									// Progress callback - shows computation progress and batch updates
									// Note: uiHelper is captured by value (shared_ptr) to ensure lifetime extends to async callbacks
									auto onProgress = [this, uiHelper](int progress, const std::string& message) {
										if (!uiHelper) return; // Safety check
										
										LOG_INF_S_ASYNC("Intersection progress: " + std::to_string(progress) + "% - " + message);

										// Keep indeterminate progress animation during computation
										// Only update to determinate when computation is complete
										if (progress >= 100) {
											uiHelper->setIndeterminateProgress(false);
											uiHelper->updateProgress(100, message);
										}
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
									uiHelper->updateProgress(100, "Intersection computation skipped");
								}
							} else {
								// No intersection computation, just show edges
								if (!highlightIntersectionNodes) {
									LOG_INF_S_ASYNC("ShowOriginalEdgesListener: Intersection computation skipped - highlightIntersectionNodes is false");
								} else if (!edgeDisplayManager) {
									LOG_WRN_S_ASYNC("ShowOriginalEdgesListener: Intersection computation skipped - edgeDisplayManager is null");
								}
								uiHelper->updateProgress(100, "Edge display completed");
							}
						} else {
							LOG_ERR_S_ASYNC("Failed to extract original edges: " + error);
							uiHelper->updateProgress(100, "Edge extraction failed: " + error);
						}
					});
			}

			// Intersection computation is now handled in the async completion callback above

			// Step 3: Collect statistics from all geometries
			uiHelper->updateProgress(90, "Collecting statistics...");
			LOG_INF_S_ASYNC("ShowOriginalEdgesListener: Collecting edge statistics");

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
				uiHelper->setStatistics(totalStats);
				
				uiHelper->endOperation();

				return CommandResult(true, "Original edges shown with progressive intersection display", commandType);
			}
			catch (const std::exception& e) {
				uiHelper->endOperation();
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