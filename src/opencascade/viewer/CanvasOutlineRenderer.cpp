#include "viewer/IOutlineRenderer.h"
#include "SceneManager.h"
#include "Canvas.h"

// Adapter for Canvas to work with ImageOutlinePass2
class CanvasOutlineRenderer : public IOutlineRenderer {
public:
    CanvasOutlineRenderer(SceneManager* sceneManager) 
        : m_sceneManager(sceneManager) {}
    
    wxGLCanvas* getGLCanvas() const override {
        return m_sceneManager ? m_sceneManager->getCanvas() : nullptr;
    }
    
    SoCamera* getCamera() const override {
        return m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    }
    
    SoSeparator* getSceneRoot() const override {
        // Return the object root where overlays should be attached
        return m_sceneManager ? m_sceneManager->getObjectRoot() : nullptr;
    }
    
    void requestRedraw() override {
        if (m_sceneManager && m_sceneManager->getCanvas()) {
            m_sceneManager->getCanvas()->Refresh(false);
        }
    }
    
private:
    SceneManager* m_sceneManager;
};