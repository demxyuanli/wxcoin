#pragma once

#include "CommandDispatcher.h"
#include <string>
#include <memory>

/**
 * @brief Unified command error handling helper class
 *
 * Provides standardized error handling methods to eliminate duplicate error handling code
 */
class CommandErrorHelper {
public:
    /**
     * @brief Check if pointer is null and return appropriate error result
     * @param ptr Pointer to check
     * @param serviceName Service name (used in error message)
     * @param commandType Command type
     * @return Error CommandResult if pointer is null; otherwise empty CommandResult (indicates check passed)
     */
    template<typename T>
    static CommandResult checkPointer(T* ptr, const std::string& serviceName, const std::string& commandType) {
        if (!ptr) {
            return CommandResult(false, serviceName + " not available", commandType);
        }
        return CommandResult(); // Return default constructed success result, but won't be used
    }

    /**
     * @brief Create error result for unavailable service
     * @param serviceName Service name
     * @param commandType Command type
     * @return Error CommandResult
     */
    static CommandResult serviceNotAvailable(const std::string& serviceName, const std::string& commandType) {
        return CommandResult(false, serviceName + " not available", commandType);
    }

    /**
     * @brief Create generic error result
     * @param message Error message
     * @param commandType Command type
     * @return Error CommandResult
     */
    static CommandResult error(const std::string& message, const std::string& commandType) {
        return CommandResult(false, message, commandType);
    }

    /**
     * @brief Create success result
     * @param message Success message
     * @param commandType Command type
     * @return Success CommandResult
     */
    static CommandResult success(const std::string& message, const std::string& commandType) {
        return CommandResult(true, message, commandType);
    }

    /**
     * @brief Create success result (using default message)
     * @param commandType Command type
     * @return Success CommandResult
     */
    static CommandResult success(const std::string& commandType) {
        return CommandResult(true, "Command executed successfully", commandType);
    }

    /**
     * @brief Wrap function execution with automatic exception handling
     * @param func Function to execute
     * @param commandType Command type
     * @return CommandResult
     */
    template<typename Func>
    static CommandResult executeSafely(Func&& func, const std::string& commandType) {
        try {
            return func();
        } catch (const std::exception& e) {
            return CommandResult(false, "Exception occurred: " + std::string(e.what()), commandType);
        } catch (...) {
            return CommandResult(false, "Unknown exception occurred", commandType);
        }
    }
};

// Convenience macro definitions to simplify common error handling patterns

// Check pointer and return error if null
#define CHECK_PTR_RETURN(ptr, serviceName, commandType) \
    { auto __result = CommandErrorHelper::checkPointer(ptr, serviceName, commandType); \
      if (!__result.success) return __result; }

// Return error results
#define RETURN_SERVICE_ERROR(serviceName, commandType) \
    return CommandErrorHelper::serviceNotAvailable(serviceName, commandType);

#define RETURN_ERROR(message, commandType) \
    return CommandErrorHelper::error(message, commandType);

// Return success results
#define RETURN_SUCCESS(message, commandType) \
    return CommandErrorHelper::success(message, commandType);

#define RETURN_SUCCESS_DEFAULT(commandType) \
    return CommandErrorHelper::success(commandType);

// DEPRECATED: Use direct try-catch or TRY_EXECUTE instead
// This macro has a design flaw: it executes the statement but doesn't handle return values properly
// Keep for backward compatibility, but don't use in new code
// 
// WARNING: Using this macro will generate a deprecation warning
// See docs/CommandErrorHelper_Usage.md for migration guide
#ifdef _MSC_VER
    #define EXECUTE_SAFELY_DEPRECATED_WARNING __pragma(message("Warning: EXECUTE_SAFELY is deprecated. Use TRY_EXECUTE or direct try-catch instead. See docs/CommandErrorHelper_Usage.md"))
#else
    #define EXECUTE_SAFELY_DEPRECATED_WARNING _Pragma("GCC warning \"EXECUTE_SAFELY is deprecated. Use TRY_EXECUTE or direct try-catch instead.\"")
#endif

#define EXECUTE_SAFELY(func, commandType) \
    EXECUTE_SAFELY_DEPRECATED_WARNING \
    { try { func; } catch (const std::exception& e) { \
        return CommandErrorHelper::error("Exception occurred: " + std::string(e.what()), commandType); \
      } catch (...) { \
        return CommandErrorHelper::error("Unknown exception occurred", commandType); \
      } }

// NEW: Properly handle void operations with exception handling
// Usage: TRY_EXECUTE(myFunction(), commandType);
#define TRY_EXECUTE(operation, commandType) \
    do { \
        try { \
            operation; \
        } catch (const std::exception& e) { \
            return CommandErrorHelper::error("Exception in " #operation ": " + std::string(e.what()), commandType); \
        } catch (...) { \
            return CommandErrorHelper::error("Unknown exception in " #operation, commandType); \
        } \
    } while(0)

// NEW: Execute and return result with exception handling
// Usage: return EXECUTE_AND_RETURN([&]() { ... return result; }, commandType);
#define EXECUTE_AND_RETURN(func, commandType) \
    CommandErrorHelper::executeSafely(func, commandType)
