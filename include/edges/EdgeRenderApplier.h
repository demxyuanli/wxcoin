#pragma once

#include "EdgeTypes.h"
#include "OCCGeometry.h"

// Applies appearance/flags and attaches generated nodes for rendering
class EdgeRenderApplier {
public:
	EdgeRenderApplier() = default;

	void applyFlagsAndAttach(std::shared_ptr<OCCGeometry>& geom, const EdgeDisplayFlags& flags);
	void applyFeatureAppearance(std::shared_ptr<OCCGeometry>& geom, const Quantity_Color& color, double width, bool edgesOnly);
};
