#pragma once

class IViewRefresher {
public:
    enum class Reason {
        GEOMETRY_CHANGED,
        NORMALS_TOGGLED,
        EDGES_TOGGLED,
        MATERIAL_CHANGED,
        CAMERA_MOVED,
        SELECTION_CHANGED,
        RENDERING_CHANGED,
        LIGHTING_CHANGED,
        MANUAL_REQUEST
    };

    virtual ~IViewRefresher() = default;
    virtual void requestRefresh(Reason reason, bool immediate) = 0;
};


