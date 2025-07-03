#pragma once

#include <Inventor/nodes/SoSeparator.h>

class CoordinateSystemRenderer {
public:
    CoordinateSystemRenderer(SoSeparator* objectRoot);
    ~CoordinateSystemRenderer();

    void createCoordinateSystem();

private:
    static const float COORD_PLANE_SIZE;
    static const float COORD_PLANE_TRANSPARENCY;
    SoSeparator* m_objectRoot;
};