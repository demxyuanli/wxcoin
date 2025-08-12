#pragma once

#include <Inventor/SbLinear.h>

// Forward declarations
class SceneManager;
class SoSeparator;
class SoClipPlane;
class SoTransform;

/**
 * SliceController encapsulates clipping plane (slice) logic and visuals.
 * It owns the Coin3D nodes required to implement a slice plane and provides
 * a small API for enabling/disabling and configuring the plane.
 */
class SliceController {
public:
    SliceController(SceneManager* sceneManager, SoSeparator* root);

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setPlane(const SbVec3f& normal, float offset);
    void moveAlongNormal(float delta);

    SbVec3f normal() const { return m_normal; }
    float offset() const { return m_offset; }

    // If the viewer root changes, allow re-attachment
    void attachRoot(SoSeparator* root);

private:
    void ensureNodes();
    void updateNodes();
    void removeNodes();

private:
    SceneManager* m_sceneManager{nullptr};
    SoSeparator* m_root{nullptr};

    bool m_enabled{false};
    SbVec3f m_normal{0,0,1};
    float m_offset{0.0f};

    // Coin3D implementation nodes
    SoClipPlane* m_clipPlane{nullptr};
    SoSeparator* m_sliceVisual{nullptr};
    SoTransform* m_sliceTransform{nullptr};
};


