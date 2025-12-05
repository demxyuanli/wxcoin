#include "ShowPointViewListener.h"
#include "PointViewDialog.h"
#include "OCCViewer.h"
#include "RenderingEngine.h"
#include "CommandDispatcher.h"
#include "geometry/VertexExtractor.h"
#include "logger/Logger.h"
#include <wx/wx.h>
#include <wx/colour.h>

ShowPointViewListener::ShowPointViewListener(OCCViewer* occViewer, RenderingEngine* renderingEngine)
    : m_occViewer(occViewer)
    , m_renderingEngine(renderingEngine)
{
}

ShowPointViewListener::~ShowPointViewListener()
{
}

CommandResult ShowPointViewListener::executeCommand(const std::string& commandType,
                                                  const std::unordered_map<std::string, std::string>& parameters)
{
    if (commandType == "SHOW_POINT_VIEW")
    {
        if (!m_occViewer) {
            return CommandResult(false, "OCCViewer not available", commandType);
        }

        // Toggle logic: if currently enabled -> disable; if disabled -> render from cache
        const bool currentlyEnabled = m_occViewer->isPointViewEnabled();
        if (!currentlyEnabled) {
            try {
                // Get all geometries and render cached vertices as points
                auto geometries = m_occViewer->getAllGeometry();
                int totalPointsRendered = 0;
                size_t totalVertices = 0;
                
                for (auto& geom : geometries) {
                    if (geom) {
                        // Get independent vertex extractor
                        auto vertexExtractor = geom->getVertexExtractor();
                        
                        if (vertexExtractor && vertexExtractor->hasCache()) {
                            // Default point settings
                            Quantity_Color pointColor(1.0, 1.0, 0.0, Quantity_TOC_RGB); // Yellow
                            double pointSize = 5.0;
                            
                            // Create point node from cached vertices using independent extractor
                            auto pointNode = vertexExtractor->createPointNode(pointColor, pointSize);
                            
                            if (pointNode) {
                                // Add point node to geometry's coin node
                                if (geom->getCoinNode()) {
                                    geom->getCoinNode()->addChild(pointNode);
                                }
                                totalPointsRendered++;
                                totalVertices += vertexExtractor->getCachedCount();
                            }
                        } else {
                            LOG_WRN_S("ShowPointViewListener: No cached vertices for geometry: " + geom->getName());
                        }
                    }
                }
                
                // Enable point view in display settings
                auto displaySettings = m_occViewer->getDisplaySettings();
                displaySettings.showPointView = true;
                m_occViewer->setDisplaySettings(displaySettings);
                
                // Force refresh
                m_occViewer->requestViewRefresh();
                
                return CommandResult(true, "Point view enabled", commandType);
                
            } catch (const std::exception& e) {
                LOG_ERR_S("ShowPointViewListener: Error enabling point view: " + std::string(e.what()));
                return CommandResult(false, "Failed to enable point view: " + std::string(e.what()), commandType);
            }
        }
        else {
            // Disable point view
            auto displaySettings = m_occViewer->getDisplaySettings();
            displaySettings.showPointView = false;
            m_occViewer->setDisplaySettings(displaySettings);
            
            // Rebuild geometries to remove point nodes
            auto geometries = m_occViewer->getAllGeometry();
            for (auto& geom : geometries) {
                if (geom) {
                    // Trigger rebuild to remove point nodes
                    geom->forceCoinRepresentationRebuild(m_occViewer->getMeshParameters());
                }
            }
            
            // Force refresh to hide points
            m_occViewer->requestViewRefresh();
            
            return CommandResult(true, "Point view disabled", commandType);
        }
    }

    return CommandResult(false, "Unknown command type", "SHOW_POINT_VIEW");
}

CommandResult ShowPointViewListener::executeCommand(cmd::CommandType commandType,
                                                  const std::unordered_map<std::string, std::string>& parameters)
{
    return executeCommand(cmd::to_string(commandType), parameters);
}

bool ShowPointViewListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == "SHOW_POINT_VIEW";
}

std::string ShowPointViewListener::getListenerName() const
{
    return "ShowPointViewListener";
}
