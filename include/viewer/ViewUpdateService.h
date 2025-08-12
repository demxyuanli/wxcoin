#pragma once

#include <memory>

class SceneManager;
class Canvas;

class ViewUpdateService {
public:
    explicit ViewUpdateService(SceneManager* sceneManager);

    void updateSceneBounds() const;
    void resetView() const;
    void requestRefresh(int reasonEnumValue, bool immediate) const; // IViewRefresher::Reason
    void requestMaterialChanged(bool immediate) const;
    void requestGeometryChanged(bool immediate) const;
    void requestNormalsToggled(bool immediate) const;
    void requestEdgesToggled(bool immediate) const;
    void requestCameraMoved(bool immediate) const;
    void refreshCanvas(bool eraseBackground) const;

private:
    SceneManager* m_sceneManager;
};


