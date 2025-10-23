#include "CancelIntersectionListener.h"
#include "async/AsyncEngineIntegration.h"
#include "logger/Logger.h"

CancelIntersectionListener::CancelIntersectionListener(
	IAsyncEngine* asyncEngine)
	: m_asyncEngine(asyncEngine)
{
}

CommandResult CancelIntersectionListener::executeCommand(
	const std::string& commandType,
	const std::unordered_map<std::string, std::string>&)
{
	if (!m_asyncEngine) {
		return CommandResult(false, "Async engine not available", commandType);
	}

	LOG_INF_S("CancelIntersectionListener: cancelling all intersection computations");
	m_asyncEngine->cancelAllTasks();

	return CommandResult(true, "All intersection computations cancelled", commandType);
}

bool CancelIntersectionListener::canHandleCommand(const std::string& commandType) const {
	return commandType == "ID_CANCEL_INTERSECTION_COMPUTATION";
}



