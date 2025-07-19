#pragma once

#include <Inventor/nodes/SoSeparator.h>

class SoSwitch;

class CoordinateSystemRenderer {
public:
    CoordinateSystemRenderer(SoSeparator* objectRoot);
    ~CoordinateSystemRenderer();

    void createCoordinateSystem();
    void updateCoordinateSystemSize(float sceneSize);
    void setCoordinateSystemScale(float scale);
    float getCoordinateSystemSize() const { return m_currentPlaneSize; }
    
    // Visibility control
    void setVisible(bool visible);
    bool isVisible() const { return m_visible; }

private:
    void rebuildCoordinateSystem();
    
    static const float DEFAULT_COORD_PLANE_SIZE;
    static const float COORD_PLANE_TRANSPARENCY;
    SoSeparator* m_objectRoot;
    SoSeparator* m_coordSystemSeparator;
    SoSwitch* m_coordSystemSwitch;
    float m_currentPlaneSize;
    bool m_visible;
};