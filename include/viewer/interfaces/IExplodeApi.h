#pragma once

#include "viewer/ExplodeTypes.h"

class IExplodeApi {
public:
    virtual ~IExplodeApi() = default;

    virtual void setExplodeEnabled(bool enabled, double factor = 1.0) = 0;
    virtual bool isExplodeEnabled() const = 0;
    virtual void setExplodeParams(ExplodeMode mode, double factor) = 0;
    virtual void getExplodeParams(ExplodeMode& mode, double& factor) const = 0;
};


