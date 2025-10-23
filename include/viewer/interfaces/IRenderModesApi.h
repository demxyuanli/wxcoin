#pragma once

#include "config/RenderingConfig.h"

class IRenderModesApi {
public:
	virtual ~IRenderModesApi() = default;

	virtual void setWireframeMode(bool wireframe) = 0;
	virtual void setShowEdges(bool showEdges) = 0;
	virtual void setAntiAliasing(bool enabled) = 0;

	virtual bool isWireframeMode() const = 0;
	virtual bool isShowEdges() const = 0;

	// Display settings
	virtual void setDisplaySettings(const RenderingConfig::DisplaySettings& settings) = 0;
	virtual const RenderingConfig::DisplaySettings& getDisplaySettings() const = 0;
};
