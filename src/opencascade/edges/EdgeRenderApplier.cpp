#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "edges/EdgeRenderApplier.h"
#include "edges/ModularEdgeComponent.h"

void EdgeRenderApplier::applyFlagsAndAttach(std::shared_ptr<OCCGeometry>& geom, const EdgeDisplayFlags& flags) {
	if (!geom) return;
	if (!geom->modularEdgeComponent) return;
	geom->modularEdgeComponent->edgeFlags = flags;
	geom->updateEdgeDisplay();
}

void EdgeRenderApplier::applyFeatureAppearance(std::shared_ptr<OCCGeometry>& geom, const Quantity_Color& color, double width, bool edgesOnly) {
	if (!geom) return;
	geom->setEdgeColor(color);
	geom->setEdgeWidth(width);
	geom->setFacesVisible(!edgesOnly);
	if (geom->modularEdgeComponent) {
		geom->modularEdgeComponent->applyAppearanceToEdgeNode(EdgeType::Feature, color, width);
	}
}