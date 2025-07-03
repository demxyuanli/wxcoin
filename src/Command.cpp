#include "Command.h"
#include <iostream>
#include <memory>

CommandManager::CommandManager()
    : m_maxStackSize(100)
{
}

CommandManager::~CommandManager()
{
    clearHistory();
}

void CommandManager::executeCommand(std::shared_ptr<Command> command)
{
    if (!command)
        return;
    
    command->execute();
    
    m_undoStack.push_back(command);
    
    m_redoStack.clear();
    
    if (m_undoStack.size() > m_maxStackSize)
    {
        m_undoStack.erase(m_undoStack.begin());
    }
}

bool CommandManager::canUndo() const
{
    return !m_undoStack.empty();
}

bool CommandManager::canRedo() const
{
    return !m_redoStack.empty();
}

void CommandManager::undo()
{
    if (!canUndo())
        return;
    

    std::shared_ptr<Command> command = m_undoStack.back();
    m_undoStack.pop_back();
    

    command->undo();
    

    m_redoStack.push_back(command);
    
    if (m_redoStack.size() > m_maxStackSize)
    {
        m_redoStack.erase(m_redoStack.begin());
    }
}

void CommandManager::redo()
{
    if (!canRedo())
        return;
    

    std::shared_ptr<Command> command = m_redoStack.back();
    m_redoStack.pop_back();
    

    command->redo();
    

    m_undoStack.push_back(command);
    

    if (m_undoStack.size() > m_maxStackSize)
    {
        m_undoStack.erase(m_undoStack.begin());
    }
}

void CommandManager::clearHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

std::string CommandManager::getUndoCommandDescription() const
{
    if (canUndo())
        return m_undoStack.back()->getDescription();
    return "";
}

std::string CommandManager::getRedoCommandDescription() const
{
    if (canRedo())
        return m_redoStack.back()->getDescription();
    return "";
}