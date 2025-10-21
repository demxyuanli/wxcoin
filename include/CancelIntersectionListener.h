#pragma once
#include "CommandListener.h"
#include "CommandType.h"
#include <memory>

class ShowOriginalEdgesListener;

/**
 * @brief Listener for canceling intersection computation
 * 
 * Cancels any ongoing asynchronous intersection computation
 * through the AsyncIntersectionManager.
 */
class CancelIntersectionListener : public CommandListener {
public:
	explicit CancelIntersectionListener(std::shared_ptr<ShowOriginalEdgesListener> edgeListener);
	
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	
	bool canHandleCommand(const std::string& commandType) const override;
	
	std::string getListenerName() const override { return "CancelIntersectionListener"; }

private:
	std::shared_ptr<ShowOriginalEdgesListener> m_edgeListener;
};



