#pragma once

#include <Inventor/SbLinear.h>

class ISliceApi {
public:
    virtual ~ISliceApi() = default;

    virtual void setSliceEnabled(bool enabled) = 0;
    virtual bool isSliceEnabled() const = 0;
    virtual void setSlicePlane(const SbVec3f& normal, float offset) = 0;
    virtual void moveSliceAlongNormal(float delta) = 0;
    virtual SbVec3f getSliceNormal() const = 0;
    virtual float getSliceOffset() const = 0;
};


