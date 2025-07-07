#pragma once

#include <Inventor/nodes/SoSeparator.h>

class CoordinateSystemRenderer {
public:
    CoordinateSystemRenderer(SoSeparator* objectRoot);
    ~CoordinateSystemRenderer();

    void createCoordinateSystem();
    void updateCoordinateSystemSize(float sceneSize);
    void setCoordinateSystemScale(float scale);
    float getCoordinateSystemSize() const { return m_currentPlaneSize; }

private:
    void rebuildCoordinateSystem();
    
    static const float DEFAULT_COORD_PLANE_SIZE;
    static const float COORD_PLANE_TRANSPARENCY;
    SoSeparator* m_objectRoot;
    SoSeparator* m_coordSystemSeparator;
    float m_currentPlaneSize;
};