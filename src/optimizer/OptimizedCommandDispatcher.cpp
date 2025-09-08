#include "optimizer/OptimizedCommandDispatcher.h"
#include "CommandListener.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

OptimizedCommandDispatcher::OptimizedCommandDispatcher() {
    // Initialize string to ID cache with common commands
    m_stringToIdCache = {
        {"FILE_NEW", static_cast<uint32_t>(cmd::CommandType::FileNew)},
        {"FILE_OPEN", static_cast<uint32_t>(cmd::CommandType::FileOpen)},
        {"FILE_SAVE", static_cast<uint32_t>(cmd::CommandType::FileSave)},
        {"FILE_SAVE_AS", static_cast<uint32_t>(cmd::CommandType::FileSaveAs)},
        {"IMPORT_STEP", static_cast<uint32_t>(cmd::CommandType::ImportSTEP)},
        {"FILE_EXIT", static_cast<uint32_t>(cmd::CommandType::FileExit)},
        {"CREATE_BOX", static_cast<uint32_t>(cmd::CommandType::CreateBox)},
        {"CREATE_SPHERE", static_cast<uint32_t>(cmd::CommandType::CreateSphere)},
        {"CREATE_CYLINDER", static_cast<uint32_t>(cmd::CommandType::CreateCylinder)},
        {"CREATE_CONE", static_cast<uint32_t>(cmd::CommandType::CreateCone)},
        {"CREATE_TORUS", static_cast<uint32_t>(cmd::CommandType::CreateTorus)},
        {"CREATE_TRUNCATED_CYLINDER", static_cast<uint32_t>(cmd::CommandType::CreateTruncatedCylinder)},
        {"CREATE_WRENCH", static_cast<uint32_t>(cmd::CommandType::CreateWrench)},
        {"VIEW_ALL", static_cast<uint32_t>(cmd::CommandType::ViewAll)},
        {"VIEW_TOP", static_cast<uint32_t>(cmd::CommandType::ViewTop)},
        {"VIEW_FRONT", static_cast<uint32_t>(cmd::CommandType::ViewFront)},
        {"VIEW_RIGHT", static_cast<uint32_t>(cmd::CommandType::ViewRight)},
        {"VIEW_ISOMETRIC", static_cast<uint32_t>(cmd::CommandType::ViewIsometric)},
        {"SHOW_NORMALS", static_cast<uint32_t>(cmd::CommandType::ShowNormals)},
        {"FIX_NORMALS", static_cast<uint32_t>(cmd::CommandType::FixNormals)},
        {"TOGGLE_EDGES", static_cast<uint32_t>(cmd::CommandType::ToggleEdges)},
        {"SET_TRANSPARENCY", static_cast<uint32_t>(cmd::CommandType::SetTransparency)},
        {"UNDO", static_cast<uint32_t>(cmd::CommandType::Undo)},
        {"REDO", static_cast<uint32_t>(cmd::CommandType::Redo)},
        {"NAV_CUBE_CONFIG", static_cast<uint32_t>(cmd::CommandType::NavCubeConfig)},
        {"ZOOM_SPEED", static_cast<uint32_t>(cmd::CommandType::ZoomSpeed)},
        {"MESH_QUALITY_DIALOG", static_cast<uint32_t>(cmd::CommandType::MeshQualityDialog)},
        {"RENDERING_SETTINGS", static_cast<uint32_t>(cmd::CommandType::RenderingSettings)},
        {"HELP_ABOUT", static_cast<uint32_t>(cmd::CommandType::HelpAbout)}
    };
}


void OptimizedCommandDispatcher::registerListener(uint32_t commandId, std::shared_ptr<CommandListener> listener) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_listeners[commandId].push_back(listener);
}

void OptimizedCommandDispatcher::registerListener(cmd::CommandType commandType, std::shared_ptr<CommandListener> listener) {
    registerListener(static_cast<uint32_t>(commandType), listener);
}

void OptimizedCommandDispatcher::unregisterListener(uint32_t commandId, std::shared_ptr<CommandListener> listener) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_listeners.find(commandId);
    if (it != m_listeners.end()) {
        auto& listeners = it->second;
        listeners.erase(
            std::remove(listeners.begin(), listeners.end(), listener),
            listeners.end()
        );
    }
}

CommandResult OptimizedCommandDispatcher::dispatchCommand(uint32_t commandId, const CommandParameters& parameters) {
    m_dispatchCount++;
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_listeners.find(commandId);
    
    if (it == m_listeners.end()) {
        return CommandResult(false, "No handler found for command ID: " + std::to_string(commandId), commandIdToString(commandId));
    }
    
    // Execute all listeners for this command
    for (auto& listener : it->second) {
        auto result = listener->executeCommand(commandIdToString(commandId), parameters);
        if (!result.success) {
            return result;
        }
    }

    return CommandResult(true, "Command executed successfully", commandIdToString(commandId));
}

CommandResult OptimizedCommandDispatcher::dispatchCommand(cmd::CommandType commandType, const CommandParameters& parameters) {
    return dispatchCommand(static_cast<uint32_t>(commandType), parameters);
}

std::vector<CommandResult> OptimizedCommandDispatcher::dispatchCommands(
    const std::vector<std::pair<uint32_t, CommandParameters>>& commands) {
    
    std::vector<CommandResult> results;
    results.reserve(commands.size());
    
    // Batch dispatch for better performance
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (const auto& [commandId, parameters] : commands) {
        m_dispatchCount++;
        
        auto it = m_listeners.find(commandId);
        if (it == m_listeners.end()) {
            results.emplace_back(false, "No handler found for command ID: " + std::to_string(commandId), commandIdToString(commandId));
            continue;
        }
        
        bool success = true;
        std::string message = "Command executed successfully";
        
        // Execute all listeners for this command
        for (auto& listener : it->second) {
            auto result = listener->executeCommand(commandIdToString(commandId), parameters);
            if (!result.success) {
                success = false;
                message = result.message;
                break;
            }
        }

        results.emplace_back(success, message, commandIdToString(commandId));
    }
    
    return results;
}

void OptimizedCommandDispatcher::setUIFeedbackHandler(std::function<void(const CommandResult&)> handler) {
    m_uiFeedbackHandler = std::move(handler);
}

bool OptimizedCommandDispatcher::hasHandler(uint32_t commandId) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_listeners.find(commandId);
    return it != m_listeners.end() && !it->second.empty();
}

uint32_t OptimizedCommandDispatcher::getCommandId(const std::string& commandString) const {
    return stringToCommandId(commandString);
}

std::vector<uint32_t> OptimizedCommandDispatcher::precompileCommands(
    const std::vector<std::string>& commandStrings) const {
    
    std::vector<uint32_t> commandIds;
    commandIds.reserve(commandStrings.size());
    
    for (const auto& commandString : commandStrings) {
        commandIds.push_back(stringToCommandId(commandString));
    }
    
    return commandIds;
}

std::string OptimizedCommandDispatcher::getPerformanceStats() const {
    std::ostringstream oss;
    oss << "Command Dispatcher Performance Stats:\n";
    oss << "  Total dispatches: " << m_dispatchCount.load() << "\n";
    oss << "  Cache hits: " << m_cacheHits.load() << "\n";
    oss << "  Cache misses: " << m_cacheMisses.load() << "\n";
    
    if (m_dispatchCount.load() > 0) {
        double hitRate = static_cast<double>(m_cacheHits.load()) / m_dispatchCount.load() * 100.0;
        oss << "  Cache hit rate: " << std::fixed << std::setprecision(2) << hitRate << "%\n";
    }
    
    oss << "  Registered command types: " << m_listeners.size() << "\n";
    oss << "  Cached string mappings: " << m_stringToIdCache.size() << "\n";
    
    return oss.str();
}

uint32_t OptimizedCommandDispatcher::stringToCommandId(const std::string& commandString) const {
    // Check cache first
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_stringToIdCache.find(commandString);
        if (it != m_stringToIdCache.end()) {
            updatePerformanceStats(true);
            return it->second;
        }
    }
    
    updatePerformanceStats(false);
    
    // Fallback to enum conversion
    try {
        auto commandType = cmd::from_string(commandString);
        return static_cast<uint32_t>(commandType);
    } catch (...) {
        return static_cast<uint32_t>(cmd::CommandType::Unknown);
    }
}

void OptimizedCommandDispatcher::updatePerformanceStats(bool cacheHit) const {
    if (cacheHit) {
        m_cacheHits++;
    } else {
        m_cacheMisses++;
    }
}

std::string OptimizedCommandDispatcher::commandIdToString(uint32_t commandId) const {
    // Try to find in cache first
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (const auto& [str, id] : m_stringToIdCache) {
        if (id == commandId) {
            return str;
        }
    }

    // Fallback to enum conversion
    try {
        cmd::CommandType cmdType = static_cast<cmd::CommandType>(commandId);
        return cmd::to_string(cmdType);
    } catch (...) {
        return "UNKNOWN_COMMAND_" + std::to_string(commandId);
    }
} 