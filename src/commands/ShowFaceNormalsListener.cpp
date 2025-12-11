#include "ShowFaceNormalsListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"

ShowFaceNormalsListener::ShowFaceNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowFaceNormalsListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
	bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::FaceNormal);
	m_viewer->setShowFaceNormals(show);
	return CommandResult(true, show ? "Face normals shown" : "Face normals hidden", commandType);
}

bool ShowFaceNormalsListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ShowFaceNormals);
}

std::string ShowFaceNormalsListener::getListenerName() const {
	return "ShowFaceNormalsListener";
}