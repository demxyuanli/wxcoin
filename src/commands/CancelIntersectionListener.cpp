#include "CancelIntersectionListener.h"
#include "ShowOriginalEdgesListener.h"
#include "edges/AsyncIntersectionManager.h"
#include "logger/Logger.h"

CancelIntersectionListener::CancelIntersectionListener(
	std::shared_ptr<ShowOriginalEdgesListener> edgeListener)
	: m_edgeListener(edgeListener)
{
}

CommandResult CancelIntersectionListener::executeCommand(
	const std::string& commandType,
	const std::unordered_map<std::string, std::string>&)
{
	if (!m_edgeListener) {
		return CommandResult(false, "Edge listener not available", commandType);
	}
	
	auto manager = m_edgeListener->getIntersectionManager();
	if (!manager) {
		return CommandResult(false, "Intersection manager not available", commandType);
	}
	
	if (!manager->isComputationRunning()) {
		LOG_INF_S("CancelIntersectionListener: no computation running");
		return CommandResult(true, "No intersection computation is currently running", commandType);
	}
	
	LOG_INF_S("CancelIntersectionListener: cancelling intersection computation");
	manager->cancelCurrentComputation();
	
	return CommandResult(true, "Intersection computation cancelled", commandType);
}

bool CancelIntersectionListener::canHandleCommand(const std::string& commandType) const {
	return commandType == "ID_CANCEL_INTERSECTION_COMPUTATION";
}



