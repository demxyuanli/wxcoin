#include "FixNormalsListener.h"
#include "OCCViewer.h"
#include "NormalValidator.h"
#include "logger/Logger.h"

FixNormalsListener::FixNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult FixNormalsListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
	
	try {
		// Get all geometries from the viewer
		auto geometries = m_viewer->getAllGeometry();
		if (geometries.empty()) {
			return CommandResult(false, "No geometries found to fix", commandType);
		}
		
		LOG_INF_S("Starting normal correction for " + std::to_string(geometries.size()) + " geometries");
		
		int correctedCount = 0;
		int totalCount = geometries.size();
		
		// Apply normal correction to each geometry
		for (auto& geometry : geometries) {
			if (!geometry) continue;
			
			const TopoDS_Shape& originalShape = static_cast<GeometryRenderer*>(geometry.get())->getShape();
			if (originalShape.IsNull()) continue;
			
			// Apply normal correction
			TopoDS_Shape correctedShape = NormalValidator::autoCorrectNormals(originalShape, geometry->getName());
			
			// Update the geometry with corrected shape
			geometry->setShape(correctedShape);
			correctedCount++;
		}
		
		// Refresh the viewer to show changes
		m_viewer->requestViewRefresh();
		
		LOG_INF_S("Normal correction completed: " + std::to_string(correctedCount) + "/" + std::to_string(totalCount) + " geometries processed");
		
		return CommandResult(true, "Face normals fixed for " + std::to_string(correctedCount) + " geometries", commandType);
		
	} catch (const std::exception& e) {
		LOG_ERR_S("Exception during normal correction: " + std::string(e.what()));
		return CommandResult(false, "Error fixing normals: " + std::string(e.what()), commandType);
	}
}

bool FixNormalsListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::FixNormals);
}