#pragma once

#include <string>
#include <unordered_map>

struct CommandResult;

/**
 * @brief Interface for command listeners
 * All command handlers must implement this interface
 */
class CommandListener {
public:
    virtual ~CommandListener() = default;
    
    /**
     * @brief Execute the command and return result
     * @param commandType Type of command to execute
     * @param parameters Command parameters
     * @return CommandResult with execution status
     */
    virtual CommandResult executeCommand(const std::string& commandType, 
                                       const std::unordered_map<std::string, std::string>& parameters) = 0;
    
    /**
     * @brief Check if this listener can handle the command
     * @param commandType Command type to check
     * @return True if this listener can handle the command
     */
    virtual bool canHandleCommand(const std::string& commandType) const = 0;
    
    /**
     * @brief Get listener name for debugging and logging
     * @return Name of the listener
     */
    virtual std::string getListenerName() const = 0;
};