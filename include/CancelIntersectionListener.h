#pragma once
#include "CommandListener.h"
#include "CommandType.h"
#include <memory>

class IAsyncEngine;

/**
 * @brief Listener for canceling intersection computation
 *
 * Cancels any ongoing asynchronous intersection computation
 * through the injected async engine.
 */
class CancelIntersectionListener : public CommandListener {
public:
	explicit CancelIntersectionListener(IAsyncEngine* asyncEngine);

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;

	bool canHandleCommand(const std::string& commandType) const override;

	std::string getListenerName() const override { return "CancelIntersectionListener"; }

private:
	IAsyncEngine* m_asyncEngine;
};



