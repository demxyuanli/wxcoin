#pragma once

#include "config/RenderingConfig.h"
#include "geometry/GeometryRenderContext.h"
#include "geometry/helper/DisplayModeHandler.h"
#include <OpenCASCADE/Quantity_Color.hxx>

class DisplayModeStateManager {
public:
    DisplayModeStateManager() = default;
    ~DisplayModeStateManager() = default;

    void setRenderStateForMode(DisplayModeRenderState& state, 
                               RenderingConfig::DisplayMode displayMode,
                               const GeometryRenderContext& context);
};


