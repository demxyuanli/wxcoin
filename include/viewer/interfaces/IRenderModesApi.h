#pragma once

class IRenderModesApi {
public:
	virtual ~IRenderModesApi() = default;

	virtual void setWireframeMode(bool wireframe) = 0;
	virtual void setShowEdges(bool showEdges) = 0;
	virtual void setAntiAliasing(bool enabled) = 0;

	virtual bool isWireframeMode() const = 0;
	virtual bool isShowEdges() const = 0;
};
