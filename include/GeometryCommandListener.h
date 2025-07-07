#pragma once

#include "CommandListener.h"
#include <memory>
#include <set>

class GeometryFactory;
class MouseHandler;

/**
 * @brief Handles geometry creation and manipulation commands
 */
class GeometryCommandListener : public CommandListener {
public:
    /**
     * @brief Constructor
     * @param factory Geometry factory for creating objects
     * @param mouseHandler Mouse handler for interaction modes
     */
    GeometryCommandListener(GeometryFactory* factory, MouseHandler* mouseHandler);
    ~GeometryCommandListener() override;
    
    CommandResult executeCommand(const std::string& commandType, 
                               const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;
    
private:
    GeometryFactory* m_geometryFactory;
    MouseHandler* m_mouseHandler;
    std::set<std::string> m_supportedCommands;
    
    /**
     * @brief Initialize supported command types
     */
    void initializeSupportedCommands();
};