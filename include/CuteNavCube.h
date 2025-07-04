#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <map>
#include <string>
#include <functional>
#include <Inventor/SbLinear.h>
#include <memory>

class CuteNavCube {
public:
    CuteNavCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight);
    ~CuteNavCube();

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

    // CuteNavCube specific methods
    void setPosition(int x, int y) { m_positionX = x; m_positionY = y; }
    void getPosition(int& x, int& y) const { x = m_positionX; y = m_positionY; }
    void setSize(int size) { m_cubeSize = size; }
    int getSize() const { return m_cubeSize; }

private:
    void setupGeometry();
    std::string pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize);
    void updateCameraRotation();
    bool generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height, const wxColour& bgColor);

    // Texture cache entry
    struct TextureData {
        unsigned char* data;
        int width, height;
        TextureData(unsigned char* d, int w, int h) : data(d), width(w), height(h) {}
        ~TextureData() { delete[] data; }
    };

    SoSeparator* m_root;
    SoCamera* m_orthoCamera;
    SoDirectionalLight* m_mainLight;
    SoDirectionalLight* m_fillLight;
    SoDirectionalLight* m_sideLight;
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

    // CuteNavCube specific members
    int m_positionX;
    int m_positionY;
    int m_cubeSize;

    // Static texture cache
    static std::map<std::string, std::shared_ptr<TextureData>> s_textureCache;
}; 