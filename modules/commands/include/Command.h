#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <memory>

class Command
{
public:
    virtual ~Command() {}
    
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    
    virtual std::string getDescription() const = 0;
};

class CommandManager
{
public:
    CommandManager();
    ~CommandManager();
    
    void executeCommand(std::shared_ptr<Command> command);
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    
    void clearHistory();
    
    std::string getUndoCommandDescription() const;
    std::string getRedoCommandDescription() const;

private:
    std::vector<std::shared_ptr<Command>> m_undoStack;
    std::vector<std::shared_ptr<Command>> m_redoStack;
    size_t m_maxStackSize;
};

#endif // COMMAND_H