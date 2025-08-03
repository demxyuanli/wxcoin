#include "renderpreview/UndoManager.h"
#include <algorithm>

UndoManager::UndoManager(size_t maxHistorySize)
    : m_currentIndex(0)
    , m_maxHistorySize(maxHistorySize)
{
    // Initialize with default state
    ConfigSnapshot defaultState;
    m_history.push_back(defaultState);
}

UndoManager::~UndoManager()
{
}

void UndoManager::saveState(const ConfigSnapshot& snapshot, const std::string& description)
{
    // Remove any redo history when saving new state
    if (m_currentIndex < m_history.size() - 1) {
        m_history.erase(m_history.begin() + m_currentIndex + 1, m_history.end());
    }
    
    // Create a copy of the snapshot with description
    ConfigSnapshot newSnapshot = snapshot;
    newSnapshot.description = description;
    
    // Add to history
    m_history.push_back(newSnapshot);
    m_currentIndex = m_history.size() - 1;
    
    // Trim history if needed
    trimHistory();
}

bool UndoManager::canUndo() const
{
    return m_currentIndex > 0;
}

bool UndoManager::canRedo() const
{
    return m_currentIndex < m_history.size() - 1;
}

ConfigSnapshot UndoManager::undo()
{
    if (!canUndo()) {
        return getCurrentState();
    }
    
    m_currentIndex--;
    return m_history[m_currentIndex];
}

ConfigSnapshot UndoManager::redo()
{
    if (!canRedo()) {
        return getCurrentState();
    }
    
    m_currentIndex++;
    return m_history[m_currentIndex];
}

ConfigSnapshot UndoManager::getCurrentState() const
{
    if (m_history.empty()) {
        return ConfigSnapshot();
    }
    
    return m_history[m_currentIndex];
}

void UndoManager::clear()
{
    m_history.clear();
    m_currentIndex = 0;
    
    // Add default state
    ConfigSnapshot defaultState;
    m_history.push_back(defaultState);
}

size_t UndoManager::getUndoCount() const
{
    return m_currentIndex;
}

size_t UndoManager::getRedoCount() const
{
    return m_history.size() - m_currentIndex - 1;
}

std::string UndoManager::getUndoDescription() const
{
    if (canUndo()) {
        return m_history[m_currentIndex - 1].description;
    }
    return "";
}

std::string UndoManager::getRedoDescription() const
{
    if (canRedo()) {
        return m_history[m_currentIndex + 1].description;
    }
    return "";
}

void UndoManager::trimHistory()
{
    if (m_history.size() <= m_maxHistorySize) {
        return;
    }
    
    // Remove oldest entries while keeping current index valid
    size_t removeCount = m_history.size() - m_maxHistorySize;
    m_history.erase(m_history.begin(), m_history.begin() + removeCount);
    m_currentIndex = std::max<size_t>(0, m_currentIndex - removeCount);
} 