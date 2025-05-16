#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <map>
#include <string>

class Canvas;

class NavigationCube {
public:
    NavigationCube(Canvas* canvas);
    ~NavigationCube();

    void initialize();
    SoSeparator* getRoot() const { return m_root; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    void handleMouseClick(const wxMouseEvent& event, const wxSize& viewportSize);

private:
    void setupGeometry();
    void setupInteraction();
    void switchToView(const std::string& region);
    std::string pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize);

    Canvas* m_canvas;
    SoSeparator* m_root;
    SoCamera* m_orthoCamera;
    bool m_enabled;
    std::map<std::string, std::pair<SbVec3f, SbVec3f>> m_viewDirections; // region -> (direction, up)
};