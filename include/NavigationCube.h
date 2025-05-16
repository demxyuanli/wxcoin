#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <map>
#include <string>
#include <functional>
#include <Inventor/SbLinear.h>

class NavigationCube {
public:
    NavigationCube(std::function<void(const std::string&)> viewChangeCallback);
    ~NavigationCube();

    void initialize();
    SoSeparator* getRoot() const { return m_root; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    void handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize);
    void render(const wxSize& size);

private:
    void setupGeometry();
    std::string pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize);
    void updateCameraRotation();
    void generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height);

    SoSeparator* m_root;
    SoCamera* m_orthoCamera;
    SoTransform* m_cameraTransform;
    bool m_enabled;
    std::map<std::string, std::string> m_faceToView;
    std::function<void(const std::string&)> m_viewChangeCallback;
    bool m_isDragging;
    SbVec2s m_lastMousePos;
    float m_rotationX;
    float m_rotationY;
    wxLongLong m_lastDragTime;
};