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
#include <memory>

class NavigationCube {
public:
    NavigationCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight);
    ~NavigationCube();

    void initialize();
    SoSeparator* getRoot() const { return m_root; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    void handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize);
    void render(int x, int y, const wxSize& size);
    void setWindowSize(int width, int height) {
        m_windowWidth = width;
        m_windowHeight = height;
    }
    SoCamera* getCamera() const { return m_orthoCamera; }
    void setCameraPosition(const SbVec3f& position);
    void setCameraOrientation(const SbRotation& orientation);
    void setRotationChangedCallback(std::function<void()> callback) { m_rotationChangedCallback = callback; }

private:
    void setupGeometry();
    std::string pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize);
    void updateCameraRotation();
    bool generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height);

    // Texture cache entry
    struct TextureData {
        unsigned char* data;
        int width, height;
        TextureData(unsigned char* d, int w, int h) : data(d), width(w), height(h) {}
        ~TextureData() { delete[] data; }
    };

    SoSeparator* m_root;
    SoCamera* m_orthoCamera;
    SoTransform* m_cameraTransform;
    bool m_enabled;
    float m_dpiScale;
    std::map<std::string, std::string> m_faceToView;
    std::function<void(const std::string&)> m_viewChangeCallback;
    std::function<void()> m_rotationChangedCallback;
    bool m_isDragging;
    SbVec2s m_lastMousePos;
    float m_rotationX;
    float m_rotationY;
    wxLongLong m_lastDragTime;
    int m_windowWidth;
    int m_windowHeight;

    // Static texture cache
    static std::map<std::string, std::shared_ptr<TextureData>> s_textureCache;
};