#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/SliceController.h"
#include "SceneManager.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoCube.h>

SliceController::SliceController(SceneManager* sceneManager, SoSeparator* root)
    : m_sceneManager(sceneManager), m_root(root) {}

void SliceController::attachRoot(SoSeparator* root) {
    if (m_root == root) return;
    // Detach current nodes from old root
    removeNodes();
    m_root = root;
    if (m_enabled) {
        ensureNodes();
        updateNodes();
    }
}

void SliceController::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (m_enabled) {
        // Initialize plane at scene center for immediate visible effect
        if (m_sceneManager) {
            SbVec3f bbMin, bbMax;
            m_sceneManager->getSceneBoundingBoxMinMax(bbMin, bbMax);
            SbVec3f center = (bbMin + bbMax) * 0.5f;
            SbVec3f n = m_normal; n.normalize(); if (n.length() < 1e-6f) n = SbVec3f(0,0,1);
            m_offset = n.dot(center);
            // Align visual transform to the world axes, not to any object transforms
            if (m_sliceTransform) {
                m_sliceTransform->rotation.setValue(SbRotation::identity());
            }
        }
        ensureNodes();
        updateNodes();
    } else {
        removeNodes();
    }
}

void SliceController::setPlane(const SbVec3f& normal, float offset) {
    m_normal = normal;
    m_offset = offset;
    if (m_enabled) {
        ensureNodes();
        updateNodes();
    }
}

void SliceController::moveAlongNormal(float delta) {
    m_offset += delta;
    if (m_enabled) updateNodes();
}

void SliceController::ensureNodes() {
    if (!m_root) return;
    if (!m_clipPlane) {
        m_clipPlane = new SoClipPlane;
        // Insert at the beginning so it affects subsequent geometry
        m_root->insertChild(m_clipPlane, 0);
    }
    if (!m_sliceVisual) {
        m_sliceVisual = new SoSeparator;
        m_sliceTransform = new SoTransform;
        m_sliceVisual->addChild(m_sliceTransform);
        // A large quad proxy using a very thin cube scaled
        SoScale* scale = new SoScale;
        scale->scaleFactor.setValue(1000.0f, 0.001f, 1000.0f);
        m_sliceVisual->addChild(scale);
        SoMaterial* mat = new SoMaterial;
        mat->diffuseColor.setValue(0.9f, 0.6f, 0.1f);
        mat->transparency.setValue(0.5f);
        m_sliceVisual->addChild(mat);
        SoCube* quad = new SoCube;
        m_sliceVisual->addChild(quad);
        m_root->addChild(m_sliceVisual);
    }
}

void SliceController::updateNodes() {
    if (!m_clipPlane) return;
    SbVec3f n = m_normal;
    n.normalize();
    SbVec3f point = n * m_offset;
    m_clipPlane->plane.setValue(SbPlane(n, point));
    if (m_sliceTransform) {
        SbVec3f z(0,0,1);
        SbRotation rot = SbRotation(z, n);
        // Keep plane aligned to world axes to avoid perceived rotations on adds
        m_sliceTransform->rotation.setValue(SbRotation::identity());
        m_sliceTransform->translation.setValue(point);
    }
}

void SliceController::removeNodes() {
    if (!m_root) return;
    if (m_clipPlane) {
        m_root->removeChild(m_clipPlane);
        m_clipPlane = nullptr;
    }
    if (m_sliceVisual) {
        m_root->removeChild(m_sliceVisual);
        m_sliceVisual = nullptr;
        m_sliceTransform = nullptr;
    }
}


