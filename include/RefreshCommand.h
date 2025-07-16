#pragma once

#include "Command.h"
#include "CommandType.h"
#include <string>
#include <memory>
#include <unordered_map>

class Canvas;
class OCCViewer;
class SceneManager;
class ViewRefreshManager;

/**
 * @brief Refresh target specification
 */
struct RefreshTarget {
    std::string objectId;       // Specific object ID (empty for all)
    std::string componentType;  // Component type filter
    bool immediate;             // Whether to refresh immediately or use debouncing
    
    RefreshTarget(const std::string& id = "", const std::string& type = "", bool imm = false)
        : objectId(id), componentType(type), immediate(imm) {}
};

/**
 * @brief Base class for all refresh commands
 */
class RefreshCommand : public Command {
public:
    RefreshCommand(cmd::CommandType type, const RefreshTarget& target = RefreshTarget());
    virtual ~RefreshCommand() = default;
    
    cmd::CommandType getCommandType() const { return m_commandType; }
    const RefreshTarget& getTarget() const { return m_target; }
    
    // Command interface
    void unexecute() override {} // Refresh commands are not undoable
    std::string getDescription() const override;

protected:
    cmd::CommandType m_commandType;
    RefreshTarget m_target;
};

/**
 * @brief Refresh view/viewport
 */
class RefreshViewCommand : public RefreshCommand {
public:
    RefreshViewCommand(const RefreshTarget& target = RefreshTarget());
    void execute() override;
    
    void setCanvas(Canvas* canvas) { m_canvas = canvas; }
    
private:
    Canvas* m_canvas;
};

/**
 * @brief Refresh scene/3D content
 */
class RefreshSceneCommand : public RefreshCommand {
public:
    RefreshSceneCommand(const RefreshTarget& target = RefreshTarget());
    void execute() override;
    
    void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }
    
private:
    SceneManager* m_sceneManager;
};

/**
 * @brief Refresh specific object(s)
 */
class RefreshObjectCommand : public RefreshCommand {
public:
    RefreshObjectCommand(const RefreshTarget& target = RefreshTarget());
    void execute() override;
    
    void setOCCViewer(OCCViewer* viewer) { m_occViewer = viewer; }
    
private:
    OCCViewer* m_occViewer;
};

/**
 * @brief Refresh material properties
 */
class RefreshMaterialCommand : public RefreshCommand {
public:
    RefreshMaterialCommand(const RefreshTarget& target = RefreshTarget());
    void execute() override;
    
    void setOCCViewer(OCCViewer* viewer) { m_occViewer = viewer; }
    
private:
    OCCViewer* m_occViewer;
};

/**
 * @brief Refresh geometry/mesh
 */
class RefreshGeometryCommand : public RefreshCommand {
public:
    RefreshGeometryCommand(const RefreshTarget& target = RefreshTarget());
    void execute() override;
    
    void setOCCViewer(OCCViewer* viewer) { m_occViewer = viewer; }
    
private:
    OCCViewer* m_occViewer;
};

/**
 * @brief Refresh UI components
 */
class RefreshUICommand : public RefreshCommand {
public:
    RefreshUICommand(const RefreshTarget& target = RefreshTarget());
    void execute() override;
    
    void setCanvas(Canvas* canvas) { m_canvas = canvas; }
    
private:
    Canvas* m_canvas;
};

/**
 * @brief Factory for creating refresh commands
 */
class RefreshCommandFactory {
public:
    static std::shared_ptr<RefreshCommand> createCommand(
        cmd::CommandType type,
        const RefreshTarget& target = RefreshTarget());
    
    static std::shared_ptr<RefreshCommand> createCommand(
        const std::string& commandString,
        const std::unordered_map<std::string, std::string>& parameters = {});
        
private:
    static RefreshTarget parseTarget(const std::unordered_map<std::string, std::string>& parameters);
}; 