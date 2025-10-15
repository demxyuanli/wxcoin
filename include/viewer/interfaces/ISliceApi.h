#pragma once

#include <Inventor/SbLinear.h>
#include <Inventor/SbViewportRegion.h>
#include <vector>

class OCCGeometry;

class ISliceApi {
public:
	virtual ~ISliceApi() = default;

	virtual void setSliceEnabled(bool enabled) = 0;
	virtual bool isSliceEnabled() const = 0;
	virtual void setSlicePlane(const SbVec3f& normal, float offset) = 0;
	virtual void moveSliceAlongNormal(float delta) = 0;
	virtual SbVec3f getSliceNormal() const = 0;
	virtual float getSliceOffset() const = 0;

	// Enhanced slice features
	virtual void setShowSectionContours(bool show) = 0;
	virtual bool isShowSectionContours() const = 0;
	virtual void setSlicePlaneColor(const SbVec3f& color) = 0;
	virtual const SbVec3f& getSlicePlaneColor() const = 0;
	virtual void setSlicePlaneOpacity(float opacity) = 0;
	virtual float getSlicePlaneOpacity() const = 0;
	virtual void setSliceGeometries(const std::vector<OCCGeometry*>& geometries) = 0;

	// Mouse interaction
	virtual bool handleSliceMousePress(const SbVec2s* mousePos, const SbViewportRegion* vp) = 0;
	virtual bool handleSliceMouseMove(const SbVec2s* mousePos, const SbViewportRegion* vp) = 0;
	virtual bool handleSliceMouseRelease(const SbVec2s* mousePos, const SbViewportRegion* vp) = 0;
	virtual bool isSliceInteracting() const = 0;
	
	// Drag mode control
	virtual void setSliceDragEnabled(bool enabled) = 0;
	virtual bool isSliceDragEnabled() const = 0;
};
