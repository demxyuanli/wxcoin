#pragma once

#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"

struct EdgeDisplayFlags;
struct MeshParameters;

class IEdgeDisplayApi {
public:
	virtual ~IEdgeDisplayApi() = default;

	virtual void setShowOriginalEdges(bool show) = 0;
	virtual void setShowFeatureEdges(bool show) = 0;
	virtual void setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave) = 0;
	virtual void setShowFeatureEdges(bool show, double featureAngleDeg, double minLength, bool onlyConvex, bool onlyConcave,
		const Quantity_Color& color, double width) = 0;
	virtual void setShowMeshEdges(bool show) = 0;
	virtual void setShowHighlightEdges(bool show) = 0;
	virtual void setShowNormalLines(bool show) = 0;
	virtual void setShowFaceNormalLines(bool show) = 0;
	virtual void setShowIntersectionNodes(bool show) = 0;
	virtual void toggleEdgeType(EdgeType type, bool show) = 0;
	virtual bool isEdgeTypeEnabled(EdgeType type) const = 0;
	virtual void updateAllEdgeDisplays() = 0;
	virtual void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, bool edgesOnly) = 0;
	virtual void applyFeatureEdgeAppearance(const Quantity_Color& color, double width, int style, bool edgesOnly) = 0;
};
