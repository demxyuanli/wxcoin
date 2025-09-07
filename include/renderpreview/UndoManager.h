#pragma once

#include <vector>
#include <memory>
#include <string>
#include "renderpreview/RenderLightSettings.h"

// Configuration snapshot for undo/redo
struct ConfigSnapshot {
    std::vector<RenderLightSettings> lights;
    int antiAliasingMethod;
    int msaaSamples;
    bool fxaaEnabled;
    int renderingMode;
    float materialAmbient;
    float materialDiffuse;
    float materialSpecular;
    float materialShininess;
    float materialTransparency;
    bool textureEnabled;
    int textureMode;
    float textureScale;
    std::string description;
    
    ConfigSnapshot()
        : antiAliasingMethod(1)
        , msaaSamples(4)
        , fxaaEnabled(false)
        , renderingMode(4)
        , materialAmbient(0.2f)
        , materialDiffuse(0.8f)
        , materialSpecular(0.6f)
        , materialShininess(32.0f)
        , materialTransparency(0.0f)
        , textureEnabled(false)
        , textureMode(0)
        , textureScale(1.0f)
    {}
};

class UndoManager
{
public:
    UndoManager(size_t maxHistorySize = 50);
    ~UndoManager();
    
    // Save current state
    void saveState(const ConfigSnapshot& snapshot, const std::string& description = "");
    
    // Undo/Redo operations
    bool canUndo() const;
    bool canRedo() const;
    ConfigSnapshot undo();
    ConfigSnapshot redo();
    
    // Get current state
    ConfigSnapshot getCurrentState() const;
    
    // Clear history
    void clear();
    
    // Get history info
    size_t getUndoCount() const;
    size_t getRedoCount() const;
    std::string getUndoDescription() const;
    std::string getRedoDescription() const;

private:
    std::vector<ConfigSnapshot> m_history;
    size_t m_currentIndex;
    size_t m_maxHistorySize;
    
    void trimHistory();
}; 