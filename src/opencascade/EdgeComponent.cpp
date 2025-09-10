#include "EdgeComponent.h"
#include "EdgeTypes.h"
#include "logger/Logger.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <vector>
#include <cmath>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <rendering/GeometryProcessor.h>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <execution>
#include <numeric>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <algorithm>

EdgeComponent::EdgeComponent() {
}

EdgeComponent::~EdgeComponent() {
	// Release all SoIndexedLineSet nodes
	if (originalEdgeNode) originalEdgeNode->unref();
	if (featureEdgeNode) featureEdgeNode->unref();
	if (meshEdgeNode) meshEdgeNode->unref();
	if (highlightEdgeNode) highlightEdgeNode->unref();
	if (normalLineNode) normalLineNode->unref();
	if (faceNormalLineNode) faceNormalLineNode->unref();
	if (silhouetteEdgeNode) silhouetteEdgeNode->unref();
}

void EdgeComponent::extractOriginalEdges(const TopoDS_Shape& shape, double samplingDensity, double minLength, bool showLinesOnly, const Quantity_Color& color, double width) {
	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;
	int pointIndex = 0;

	LOG_INF_S("Extracting original edges from shape with sampling density: " + std::to_string(samplingDensity) + 
		", min length: " + std::to_string(minLength) + 
		", lines only: " + std::string(showLinesOnly ? "true" : "false") +
		", color: (" + std::to_string(color.Red()) + ", " + std::to_string(color.Green()) + ", " + std::to_string(color.Blue()) + ")" +
		", width: " + std::to_string(width));

	for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
		TopoDS_Edge edge = TopoDS::Edge(exp.Current());
		Standard_Real first, last;
		Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

		if (!curve.IsNull()) {
			// Get curve type using BRepAdaptor_Curve
			BRepAdaptor_Curve adaptor(edge);
			GeomAbs_CurveType curveType = adaptor.GetType();

			// Check minimum length filter
			gp_Pnt startPoint = curve->Value(first);
			gp_Pnt endPoint = curve->Value(last);
			double edgeLength = startPoint.Distance(endPoint);
			
			// For closed curves, use approximate arc length
			if (edgeLength < minLength) {
				Standard_Real curveLength = last - first;
				int numSamples = std::max(8, static_cast<int>(curveLength * 50));
				numSamples = std::min(numSamples, 100);
				double approximateLength = 0.0;
				gp_Pnt prev = curve->Value(first);
				for (int i = 1; i <= numSamples; ++i) {
					Standard_Real t = first + (last - first) * i / numSamples;
					gp_Pnt cur = curve->Value(t);
					approximateLength += prev.Distance(cur);
					prev = cur;
				}
				if (approximateLength < minLength) {
					continue; // Skip this edge
				}
			}

			std::vector<gp_Pnt> edgePoints;

			if (curveType == GeomAbs_Line || showLinesOnly) {
				// For lines or when lines-only mode is enabled, just use start and end points
				edgePoints.push_back(startPoint);
				edgePoints.push_back(endPoint);
			}
			else {
				// For curves, use adaptive sampling based on density parameter
				Standard_Real curveLength = last - first;
				int numSamples = std::max(8, static_cast<int>(curveLength * samplingDensity));
				numSamples = std::min(numSamples, 200);

				for (int i = 0; i <= numSamples; ++i) {
					Standard_Real t = first + (last - first) * i / numSamples;
					gp_Pnt point = curve->Value(t);
					edgePoints.push_back(point);
				}
			}

			// Add points to global array
			for (const auto& point : edgePoints) {
				points.push_back(point);
			}

			// Add indices for this edge - ensure continuous line segments
			for (size_t i = 0; i < edgePoints.size() - 1; ++i) {
				indices.push_back(pointIndex + i);
				indices.push_back(pointIndex + i + 1);
				indices.push_back(SO_END_LINE_INDEX);
			}

			pointIndex += edgePoints.size();
		}
	}

	if (originalEdgeNode) originalEdgeNode->unref();

	// Create material for original edges with custom color
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(color.Red(), color.Green(), color.Blue());

	// Create draw style for lines with custom width
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->lineWidth = static_cast<float>(width);

	// Create coordinates
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
	}

	// Create line set
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	lineSet->coordIndex.setValues(0, indices.size(), indices.data());

	// Create separator and add all components
	SoSeparator* sep = new SoSeparator;
	sep->addChild(mat);
	sep->addChild(drawStyle);
	sep->addChild(coords);
	sep->addChild(lineSet);

	// Store the separator as originalEdgeNode for proper display management
	originalEdgeNode = sep;
	originalEdgeNode->ref();

	LOG_INF_S("Original edge extraction complete: " + std::to_string(points.size()) + " points, " +
		std::to_string(indices.size() / 3) + " line segments");
}
void EdgeComponent::extractFeatureEdges(const TopoDS_Shape& shape, double featureAngle, double minLength, bool onlyConvex, bool onlyConcave) {
	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;
	int pointIndex = 0;
	double cosThreshold = std::cos(featureAngle * M_PI / 180.0);
	// Build edge->faces adjacency once for performance on large models
	TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
	TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

	// Collect all edges first
	std::vector<TopoDS_Edge> allEdges;
	allEdges.reserve(edgeFaceMap.Extent());
	for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
		allEdges.push_back(TopoDS::Edge(exp.Current()));
	}

	// Per-edge polyline results (to enable parallel processing without locks)
	struct EdgePolyline { std::vector<gp_Pnt> pts; };
	std::vector<EdgePolyline> polylines(allEdges.size());
	std::vector<size_t> edgeIndices(allEdges.size());
	std::iota(edgeIndices.begin(), edgeIndices.end(), 0);
	auto approximateCurveLength = [&](const Handle(Geom_Curve)& curve, Standard_Real first, Standard_Real last) -> double {
		if (curve.IsNull() || last <= first) return 0.0;
		// Sample-based approximation to handle periodic/closed curves (e.g., cylinder circle edges)
		const int samples = std::min(400, std::max(32, static_cast<int>((last - first) * 200)));
		gp_Pnt prev = curve->Value(first);
		double length = 0.0;
		for (int i = 1; i <= samples; ++i) {
			Standard_Real t = first + (last - first) * (static_cast<double>(i) / samples);
			gp_Pnt cur = curve->Value(t);
			length += prev.Distance(cur);
			prev = cur;
		}
		return length;
		};

	LOG_INF_S("Extracting feature edges with angle threshold: " + std::to_string(featureAngle) +
		" degrees, min length: " + std::to_string(minLength) +
		", only convex: " + std::string(onlyConvex ? "true" : "false") +
		", only concave: " + std::string(onlyConcave ? "true" : "false"));
	// Process edges in parallel via index array
	std::for_each(std::execution::par, edgeIndices.begin(), edgeIndices.end(), [&](size_t idxEdge) {
		// Safety: watchdog to avoid infinite loops on degenerate parameter ranges
		static const int kMaxSamples = 2000;
		const TopoDS_Edge& edge = allEdges[idxEdge];
		const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
		if (faces.Extent() >= 1) {
			TopoDS_Face face1 = TopoDS::Face(faces.First());
			TopoDS_Face face2 = faces.Extent() >= 2 ? TopoDS::Face(faces.Last()) : TopoDS_Face();

			//LOG_INF_S("Found edge shared by " + std::to_string(faceList.Extent()) + " faces, processing...");
			Standard_Real first, last;
			Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
			if (curve.IsNull()) return; // skip this edge in parallel loop
			Standard_Real mid = (first + last) / 2.0;
			gp_Pnt midPnt = curve->Value(mid);

			bool isFeatureEdge = false;
			double angleDegrees = 0.0;

			if (faces.Extent() >= 2) {
				// Two faces case - use angle between face normals
				// Get proper UV coordinates for the midpoint on both faces
				Standard_Real u1, v1, u2, v2;
				BRepTools::UVBounds(face1, u1, v1, u2, v2);
				Standard_Real u3, v3, u4, v4;
				BRepTools::UVBounds(face2, u3, v3, u4, v4);

				// Use midpoint of UV bounds for better surface normal calculation
				Standard_Real midU1 = (u1 + u2) / 2.0;
				Standard_Real midV1 = (v1 + v2) / 2.0;
				Standard_Real midU2 = (u3 + u4) / 2.0;
				Standard_Real midV2 = (v3 + v4) / 2.0;

				BRepAdaptor_Surface surf1(face1);
				BRepAdaptor_Surface surf2(face2);
				gp_Vec d1u1, d1v1, d1u2, d1v2;
				surf1.D1(midU1, midV1, midPnt, d1u1, d1v1);
				surf2.D1(midU2, midV2, midPnt, d1u2, d1v2);
				gp_Vec n1 = d1u1.Crossed(d1v1);
				gp_Vec n2 = d1u2.Crossed(d1v2);
				n1.Normalize();
				n2.Normalize();
				double cosAngle = n1.Dot(n2);
				angleDegrees = std::acos(std::abs(cosAngle)) * 180.0 / M_PI;

				//LOG_INF_S("Two faces case - Edge angle: " + std::to_string(angleDegrees) + " degrees, threshold: " + std::to_string(featureAngle));

				if (cosAngle < cosThreshold) {
					isFeatureEdge = true;
				}

				// Check convexity/concavity for two-face edges
				if (isFeatureEdge && (onlyConvex || onlyConcave)) {
					gp_Pnt p1 = curve->Value(first);
					gp_Pnt p2 = curve->Value(last);
					gp_Vec edgeTangent(p1, p2);
					edgeTangent.Normalize();
					double cross = n1.Crossed(n2).Dot(edgeTangent);

					LOG_INF_S("Cross product sign: " + std::to_string(cross) +
						" (convex: " + std::string(cross > 0 ? "true" : "false") +
						", concave: " + std::string(cross < 0 ? "true" : "false") + ")");

					if (onlyConvex && cross <= 0) {
						LOG_INF_S("Skipping non-convex edge");
						isFeatureEdge = false;
					}
					if (onlyConcave && cross >= 0) {
						LOG_INF_S("Skipping non-concave edge");
						isFeatureEdge = false;
					}
				}
			}
			else {
				// Single face case - use comprehensive feature detection
				BRepAdaptor_Curve adaptor(edge);
				GeomAbs_CurveType curveType = adaptor.GetType();

				// Strategy 1: All curved edges are potential features
				if (curveType != GeomAbs_Line) {
					isFeatureEdge = true;
					angleDegrees = 45.0; // Default angle for curved edges
					LOG_INF_S("Single face curved edge - curve type: " + std::to_string(static_cast<int>(curveType)) + ", considering as feature");
				}

				// Strategy 2: Check if this is a boundary edge (important for cylinders, cones, etc.)
				if (!isFeatureEdge) {
					// Check if this edge is on the boundary of the shape
					BRepAdaptor_Surface surf(face1);
					GeomAbs_SurfaceType surfaceType = surf.GetType();

					// For cylindrical, conical, and toroidal surfaces, edges are often features
					if (surfaceType == GeomAbs_Cylinder ||
						surfaceType == GeomAbs_Cone ||
						surfaceType == GeomAbs_Torus) {
						isFeatureEdge = true;
						angleDegrees = 30.0;
						LOG_INF_S("Boundary edge on " + std::string(surfaceType == GeomAbs_Cylinder ? "cylinder" :
							surfaceType == GeomAbs_Cone ? "cone" : "torus") + ", considering as feature");
					}
				}

				// Strategy 3: Check edge length and position
				if (!isFeatureEdge) {
					gp_Pnt p1 = curve->Value(first);
					gp_Pnt p2 = curve->Value(last);
					double edgeLength = p1.Distance(p2);

					// Long edges are more likely to be features
					if (edgeLength > 1.0) {
						isFeatureEdge = true;
						angleDegrees = 25.0;
						LOG_INF_S("Long edge (length: " + std::to_string(edgeLength) + "), considering as feature");
					}
				}
			}

			if (isFeatureEdge) {
				gp_Pnt p1 = curve->Value(first);
				gp_Pnt p2 = curve->Value(last);
				double edgeLength = p1.Distance(p2);
				// Fallback to approximate arc length for closed/periodic edges where start==end (e.g., cylinder rims)
				if (edgeLength < minLength) {
					edgeLength = approximateCurveLength(curve, first, last);
				}
				if (edgeLength < minLength) {
					return; // skip this edge
				}

				// Sample the curve densely like original edges
				std::vector<gp_Pnt> edgePoints;

				// Get curve type for adaptive sampling
				BRepAdaptor_Curve adaptor(edge);
				GeomAbs_CurveType curveType = adaptor.GetType();

				if (curveType == GeomAbs_Line) {
					// For lines, just use start and end points
					edgePoints.push_back(p1);
					edgePoints.push_back(p2);
				}
				else {
					// For curves, use dense sampling
					Standard_Real curveLength = last - first;
					int numSamples = std::max(50, static_cast<int>(curveLength * 100));
					numSamples = std::min(numSamples, 200);
					if (numSamples <= 0 || numSamples > kMaxSamples) numSamples = 200;

					for (int i = 0; i <= numSamples; ++i) {
						Standard_Real alpha = static_cast<Standard_Real>(i) / static_cast<Standard_Real>(numSamples);
						if (alpha < 0.0) alpha = 0.0; if (alpha > 1.0) alpha = 1.0;
						Standard_Real t = first + (last - first) * alpha;
						gp_Pnt point = curve->Value(t);
						edgePoints.push_back(point);
					}
				}

				// Store into the per-edge polyline container
				polylines[idxEdge].pts = std::move(edgePoints);

				//LOG_INF_S("Added feature edge: (" + std::to_string(p1.X()) + ", " + std::to_string(p1.Y()) + ", " + std::to_string(p1.Z()) +
				//          ") to (" + std::to_string(p2.X()) + ", " + std::to_string(p2.Y()) + ", " + std::to_string(p2.Z()) + ")");
			}
			else {
				// Not a feature edge; leave empty
			}
		}
		});

	// Merge per-edge polylines into SoIndexedLineSet buffers
	// Precompute total point count
	size_t totalPts = 0;
	for (const auto& pl : polylines) totalPts += pl.pts.size();
	points.reserve(totalPts);
	// Merge and create indices on the fly
	for (const auto& pl : polylines) {
		if (pl.pts.size() < 2) continue;
		size_t base = points.size();
		for (const auto& p : pl.pts) points.push_back(p);
		for (size_t i = 0; i + 1 < pl.pts.size(); ++i) {
			indices.push_back(static_cast<int32_t>(base + i));
			indices.push_back(static_cast<int32_t>(base + i + 1));
			indices.push_back(SO_END_LINE_INDEX);
		}
	}

	LOG_INF_S("Feature edge extraction complete: " + std::to_string(points.size()) + " points, " +
		std::to_string(indices.size() / 3) + " line segments");

	if (featureEdgeNode) featureEdgeNode->unref();

	// Create material for feature edges
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(0, 0, 1); // Blue

	// Create draw style for feature edges
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->lineWidth = 2.0f; // Thicker lines for better visibility

	// Create coordinates
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
	}

	// Create line set
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	lineSet->coordIndex.setValues(0, indices.size(), indices.data());

	// Create separator and add all components
	SoSeparator* sep = new SoSeparator;
	sep->addChild(mat);
	sep->addChild(drawStyle);
	sep->addChild(coords);
	sep->addChild(lineSet);

	// Store the separator as featureEdgeNode for proper display management
	featureEdgeNode = sep;
	featureEdgeNode->ref();
}
void EdgeComponent::extractMeshEdges(const TriangleMesh& mesh) {
	std::vector<gp_Pnt> points = mesh.vertices;
	std::vector<int32_t> indices;
	for (size_t i = 0; i + 2 < mesh.triangles.size(); i += 3) {
		int a = mesh.triangles[i];
		int b = mesh.triangles[i + 1];
		int c = mesh.triangles[i + 2];
		indices.push_back(a); indices.push_back(b); indices.push_back(SO_END_LINE_INDEX);
		indices.push_back(b); indices.push_back(c); indices.push_back(SO_END_LINE_INDEX);
		indices.push_back(c); indices.push_back(a); indices.push_back(SO_END_LINE_INDEX);
	}

	if (meshEdgeNode) meshEdgeNode->unref();

	// Create material for mesh edges
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(0, 1, 0); // Green

	// Create coordinates
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
	}

	// Create line set
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	lineSet->coordIndex.setValues(0, indices.size(), indices.data());

	// Create separator and add all components
	SoSeparator* sep = new SoSeparator;
	sep->addChild(mat);
	sep->addChild(coords);
	sep->addChild(lineSet);

	// Store the separator as meshEdgeNode for proper display management
	meshEdgeNode = sep;
	meshEdgeNode->ref();
}
void EdgeComponent::generateAllEdgeNodes() {
	if (originalEdgeNode) originalEdgeNode->unref();
	if (featureEdgeNode) featureEdgeNode->unref();
	if (meshEdgeNode) meshEdgeNode->unref();
	if (highlightEdgeNode) highlightEdgeNode->unref();
	if (normalLineNode) normalLineNode->unref();

	// Initialize all edge nodes as null - they will be created when needed
	originalEdgeNode = nullptr;
	featureEdgeNode = nullptr;
	meshEdgeNode = nullptr;
	highlightEdgeNode = nullptr;
	normalLineNode = nullptr;
}
SoSeparator* EdgeComponent::getEdgeNode(EdgeType type) {
	switch (type) {
	case EdgeType::Original: return originalEdgeNode;
	case EdgeType::Feature: return featureEdgeNode;
	case EdgeType::Mesh: return meshEdgeNode;
	case EdgeType::Highlight: return highlightEdgeNode;
	case EdgeType::NormalLine: return normalLineNode;
	case EdgeType::FaceNormalLine: return faceNormalLineNode;
	}
	return nullptr;
}
void EdgeComponent::setEdgeDisplayType(EdgeType type, bool show) {
	switch (type) {
	case EdgeType::Original: edgeFlags.showOriginalEdges = show; break;
	case EdgeType::Feature: edgeFlags.showFeatureEdges = show; break;
	case EdgeType::Mesh: edgeFlags.showMeshEdges = show; break;
	case EdgeType::Highlight: edgeFlags.showHighlightEdges = show; break;
	case EdgeType::NormalLine: edgeFlags.showNormalLines = show; break;
	case EdgeType::FaceNormalLine: edgeFlags.showFaceNormalLines = show; break;
	}
}
bool EdgeComponent::isEdgeDisplayTypeEnabled(EdgeType type) const {
	switch (type) {
	case EdgeType::Original: return edgeFlags.showOriginalEdges;
	case EdgeType::Feature: return edgeFlags.showFeatureEdges;
	case EdgeType::Mesh: return edgeFlags.showMeshEdges;
	case EdgeType::Highlight: return edgeFlags.showHighlightEdges;
	case EdgeType::NormalLine: return edgeFlags.showNormalLines;
	case EdgeType::FaceNormalLine: return edgeFlags.showFaceNormalLines;
	}
	return false;
}
void EdgeComponent::updateEdgeDisplay(SoSeparator* parentNode) {
	if (edgeFlags.showOriginalEdges && originalEdgeNode) {
		if (parentNode->findChild(originalEdgeNode) < 0) {
			parentNode->addChild(originalEdgeNode);
		}
	}
	else if (originalEdgeNode) {
		int idx = parentNode->findChild(originalEdgeNode);
		if (idx >= 0) parentNode->removeChild(idx);
	}
	if (edgeFlags.showFeatureEdges && featureEdgeNode) {
		if (parentNode->findChild(featureEdgeNode) < 0) {
			parentNode->addChild(featureEdgeNode);
		}
	}
	else if (featureEdgeNode) {
		int idx = parentNode->findChild(featureEdgeNode);
		if (idx >= 0) parentNode->removeChild(idx);
	}
	if (edgeFlags.showMeshEdges && meshEdgeNode) {
		if (parentNode->findChild(meshEdgeNode) < 0) {
			parentNode->addChild(meshEdgeNode);
		}
	}
	else if (meshEdgeNode) {
		int idx = parentNode->findChild(meshEdgeNode);
		if (idx >= 0) parentNode->removeChild(idx);
	}
	if (edgeFlags.showHighlightEdges && highlightEdgeNode) {
		if (parentNode->findChild(highlightEdgeNode) < 0) {
			parentNode->addChild(highlightEdgeNode);
		}
	}
	else if (highlightEdgeNode) {
		int idx = parentNode->findChild(highlightEdgeNode);
		if (idx >= 0) parentNode->removeChild(idx);
	}
	if (edgeFlags.showNormalLines && normalLineNode) {
		if (parentNode->findChild(normalLineNode) < 0) {
			parentNode->addChild(normalLineNode);
			LOG_INF_S("Added normal line node to parent");
		}
	}
	else if (normalLineNode) {
		int idx = parentNode->findChild(normalLineNode);
		if (idx >= 0) {
			parentNode->removeChild(idx);
			LOG_INF_S("Removed normal line node from parent");
		}
	}
	else if (edgeFlags.showNormalLines) {
		LOG_WRN_S("Normal lines enabled but normalLineNode is null");
	}

	if (edgeFlags.showFaceNormalLines && faceNormalLineNode) {
		if (parentNode->findChild(faceNormalLineNode) < 0) {
			parentNode->addChild(faceNormalLineNode);
			LOG_INF_S("Added face normal line node to parent");
		}
	}
	else if (faceNormalLineNode) {
		int idx = parentNode->findChild(faceNormalLineNode);
		if (idx >= 0) {
			parentNode->removeChild(idx);
			LOG_INF_S("Removed face normal line node from parent");
		}
	}
	else if (edgeFlags.showFaceNormalLines) {
		LOG_WRN_S("Face normal lines enabled but faceNormalLineNode is null");
	}

	if (false && silhouetteEdgeNode) {
		if (parentNode->findChild(silhouetteEdgeNode) < 0) {
			parentNode->addChild(silhouetteEdgeNode);
			LOG_INF_S("Added silhouette edge node to parent");
		}
	}
	else if (silhouetteEdgeNode) {
		int idx = parentNode->findChild(silhouetteEdgeNode);
		if (idx >= 0) {
			parentNode->removeChild(idx);
			LOG_INF_S("Removed silhouette edge node from parent");
		}
	}
	else if (false) {
		LOG_WRN_S("Silhouette edges enabled but silhouetteEdgeNode is null");
	}
}

void EdgeComponent::applyAppearanceToEdgeNode(EdgeType type, const Quantity_Color& color, double width) {
	SoSeparator* node = getEdgeNode(type);
	if (!node) return;

	// Update material color if present
	for (int i = 0; i < node->getNumChildren(); ++i) {
		SoMaterial* mat = dynamic_cast<SoMaterial*>(node->getChild(i));
		if (mat) {
			Standard_Real r, g, b;
			color.Values(r, g, b, Quantity_TOC_RGB);
			mat->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
		}
		SoDrawStyle* style = dynamic_cast<SoDrawStyle*>(node->getChild(i));
		if (style) {
			style->lineWidth = static_cast<float>(std::max(0.1, std::min(10.0, width)));
		}
	}
}
void EdgeComponent::generateHighlightEdgeNode() {
	// Generate highlight edge node for selected edges/faces
	// TODO: Implement highlight edge node generation based on selection state
	if (highlightEdgeNode) highlightEdgeNode->unref();

	// Create material for highlight edges
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(1, 0, 0); // Red

	// Create empty line set for now
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;

	// Create separator and add all components
	SoSeparator* sep = new SoSeparator;
	sep->addChild(mat);
	sep->addChild(lineSet);

	// Store the separator as highlightEdgeNode for proper display management
	highlightEdgeNode = sep;
	highlightEdgeNode->ref();
}
void EdgeComponent::generateNormalLineNode(const TriangleMesh& mesh, double length) {
	LOG_INF_S("Generating normal line node with " + std::to_string(mesh.vertices.size()) +
		" vertices and " + std::to_string(mesh.normals.size()) + " normals");

	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;
	std::vector<float> colors; // Store colors for each line
	int pointIndex = 0;
	int normalCount = 0;
	int correctNormalCount = 0;
	int incorrectNormalCount = 0;

	for (size_t i = 0; i < mesh.vertices.size() && i < mesh.normals.size(); ++i) {
		const gp_Pnt& v = mesh.vertices[i];
		const gp_Vec& n = mesh.normals[i];

		// Check if normal is valid (not zero)
		if (n.Magnitude() > 0.001) {
			gp_Pnt p2(v.X() + n.X() * length, v.Y() + n.Y() * length, v.Z() + n.Z() * length);
			points.push_back(v);
			points.push_back(p2);
			indices.push_back(pointIndex++);
			indices.push_back(pointIndex++);
			indices.push_back(SO_END_LINE_INDEX);
			
			// Check if normal direction is correct (pointing outward)
			// Calculate vector from vertex to origin
			gp_Vec vertexToOrigin(-v.X(), -v.Y(), -v.Z());
			
			// Check if normal points away from origin (outward)
			double dotProduct = n.Dot(vertexToOrigin);
			bool isCorrectDirection = (dotProduct > 0);
			
			// Add color for this line (RGB values)
			if (isCorrectDirection) {
				colors.push_back(0.0f);  // R - Green for correct normals
				colors.push_back(1.0f);  // G
				colors.push_back(0.0f);  // B
				correctNormalCount++;
			} else {
				colors.push_back(1.0f);  // R - Red for incorrect normals
				colors.push_back(0.0f);  // G
				colors.push_back(0.0f);  // B
				incorrectNormalCount++;
			}
			
			normalCount++;
		}
	}

	LOG_INF_S("Generated " + std::to_string(normalCount) + " normal lines from " +
		std::to_string(mesh.vertices.size()) + " vertices");
	LOG_INF_S("Correct normals: " + std::to_string(correctNormalCount) + 
		", Incorrect normals: " + std::to_string(incorrectNormalCount));

	if (normalLineNode) normalLineNode->unref();

	// Create material for normal lines (base color - will be overridden by per-vertex colors)
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(0.5, 0, 0.5); // Purple (fallback)

	// Create coordinates
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
	}

	// Create color node for per-vertex coloring
	SoMaterialBinding* matBinding = new SoMaterialBinding;
	matBinding->value = SoMaterialBinding::PER_VERTEX;

	// Create material array for colors
	SoMaterial* colorMat = new SoMaterial;
	colorMat->diffuseColor.setValues(0, colors.size() / 3, reinterpret_cast<const float(*)[3]>(colors.data()));

	// Create line set
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	lineSet->coordIndex.setValues(0, indices.size(), indices.data());

	// Create separator and add all components
	normalLineNode = new SoSeparator;
	normalLineNode->ref();
	normalLineNode->addChild(mat); // Base material
	normalLineNode->addChild(matBinding); // Per-vertex binding
	normalLineNode->addChild(colorMat); // Color array
	normalLineNode->addChild(coords);
	normalLineNode->addChild(lineSet);
}

void EdgeComponent::generateFaceNormalLineNode(const TriangleMesh& mesh, double length) {
	LOG_INF_S("Generating face normal line node with " + std::to_string(mesh.vertices.size()) +
		" vertices and " + std::to_string(mesh.triangles.size() / 3) + " triangles");

	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;
	std::vector<float> colors; // Store colors for each line
	int pointIndex = 0;
	int faceNormalCount = 0;
	int correctFaceNormalCount = 0;
	int incorrectFaceNormalCount = 0;

	// Calculate face normals for each triangle
	for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
		if (i + 2 >= mesh.triangles.size()) break;

		int idx1 = mesh.triangles[i];
		int idx2 = mesh.triangles[i + 1];
		int idx3 = mesh.triangles[i + 2];

		if (idx1 >= 0 && idx1 < static_cast<int>(mesh.vertices.size()) &&
			idx2 >= 0 && idx2 < static_cast<int>(mesh.vertices.size()) &&
			idx3 >= 0 && idx3 < static_cast<int>(mesh.vertices.size())) {
			const gp_Pnt& p1 = mesh.vertices[idx1];
			const gp_Pnt& p2 = mesh.vertices[idx2];
			const gp_Pnt& p3 = mesh.vertices[idx3];

			// Calculate face center
			gp_Pnt faceCenter(
				(p1.X() + p2.X() + p3.X()) / 3.0,
				(p1.Y() + p2.Y() + p3.Y()) / 3.0,
				(p1.Z() + p2.Z() + p3.Z()) / 3.0
			);

			// Calculate face normal using right-hand rule
			gp_Vec v1(p1, p2);
			gp_Vec v2(p1, p3);
			gp_Vec faceNormal = v1.Crossed(v2);

			// Normalize the face normal
			double normalLength = faceNormal.Magnitude();
			if (normalLength > 0.001) {
				faceNormal.Scale(1.0 / normalLength);

				// Check if face normal direction is correct (pointing outward)
				// Calculate vector from face center to origin
				gp_Vec centerToOrigin(-faceCenter.X(), -faceCenter.Y(), -faceCenter.Z());
				
				// Check if face normal points away from origin (outward)
				double dotProduct = faceNormal.Dot(centerToOrigin);
				bool isCorrectDirection = (dotProduct > 0);

				// Create line from face center to normal direction
				gp_Pnt normalEnd(
					faceCenter.X() + faceNormal.X() * length,
					faceCenter.Y() + faceNormal.Y() * length,
					faceCenter.Z() + faceNormal.Z() * length
				);

				points.push_back(faceCenter);
				points.push_back(normalEnd);
				indices.push_back(pointIndex++);
				indices.push_back(pointIndex++);
				indices.push_back(SO_END_LINE_INDEX);
				
				// Add color for this line (RGB values)
				if (isCorrectDirection) {
					colors.push_back(0.0f);  // R - Green for correct normals
					colors.push_back(1.0f);  // G
					colors.push_back(0.0f);  // B
					correctFaceNormalCount++;
				} else {
					colors.push_back(1.0f);  // R - Red for incorrect normals
					colors.push_back(0.0f);  // G
					colors.push_back(0.0f);  // B
					incorrectFaceNormalCount++;
				}
				
				faceNormalCount++;
			}
		}
	}

	LOG_INF_S("Generated " + std::to_string(faceNormalCount) + " face normal lines from " +
		std::to_string(mesh.triangles.size() / 3) + " triangles");
	LOG_INF_S("Correct face normals: " + std::to_string(correctFaceNormalCount) + 
		", Incorrect face normals: " + std::to_string(incorrectFaceNormalCount));

	if (faceNormalLineNode) faceNormalLineNode->unref();

	// Create material for face normal lines (base color - will be overridden by per-vertex colors)
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(0, 0.5, 0.5); // Cyan (fallback)

	// Create coordinates
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
	}

	// Create color node for per-vertex coloring
	SoMaterialBinding* matBinding = new SoMaterialBinding;
	matBinding->value = SoMaterialBinding::PER_VERTEX;

	// Create material array for colors
	SoMaterial* colorMat = new SoMaterial;
	colorMat->diffuseColor.setValues(0, colors.size() / 3, reinterpret_cast<const float(*)[3]>(colors.data()));

	// Create line set
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	lineSet->coordIndex.setValues(0, indices.size(), indices.data());

	// Create separator and add all components
	faceNormalLineNode = new SoSeparator;
	faceNormalLineNode->ref();
	faceNormalLineNode->addChild(mat); // Base material
	faceNormalLineNode->addChild(matBinding); // Per-vertex binding
	faceNormalLineNode->addChild(colorMat); // Color array
	faceNormalLineNode->addChild(coords);
	faceNormalLineNode->addChild(lineSet);
}

static gp_Vec getNormalAt(const TopoDS_Face& face, const gp_Pnt& p) {
	BRepAdaptor_Surface surf(face, true);
	Standard_Real u, v;
	Handle(Geom_Surface) hSurf = BRep_Tool::Surface(face);
	GeomAPI_ProjectPointOnSurf projector(p, hSurf);
	projector.LowerDistanceParameters(u, v);
	gp_Pnt surfPnt;
	gp_Vec dU, dV;
	surf.D1(u, v, surfPnt, dU, dV);
	gp_Vec n = dU.Crossed(dV);
	n.Normalize();
	if (face.Orientation() == TopAbs_REVERSED)
		n.Reverse();
	return n;
}

void EdgeComponent::generateSilhouetteEdgeNode(const TopoDS_Shape& shape, const gp_Pnt& cameraPos) {
	LOG_INF_S("[SilhouetteDebug] Camera position: " + std::to_string(cameraPos.X()) + ", " + std::to_string(cameraPos.Y()) + ", " + std::to_string(cameraPos.Z()));
	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;
	int pointIndex = 0;
	int silhouetteCount = 0;
	int totalEdges = 0;
	int edgesWithTwoFaces = 0;

	TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
	TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

	for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
		totalEdges++;
		TopoDS_Edge edge = TopoDS::Edge(exp.Current());
		const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);

		LOG_INF_S("[SilhouetteDebug] Edge " + std::to_string(totalEdges) + " has " + std::to_string(faces.Extent()) + " faces");

		if (faces.Extent() != 2) {
			LOG_INF_S("[SilhouetteDebug] Skipping edge with " + std::to_string(faces.Extent()) + " faces (need exactly 2)");
			continue;
		}
		edgesWithTwoFaces++;

		TopoDS_Face face1 = TopoDS::Face(faces.First());
		TopoDS_Face face2 = TopoDS::Face(faces.Last());

		Standard_Real first, last;
		Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
		if (curve.IsNull()) {
			LOG_INF_S("[SilhouetteDebug] Curve is null for edge " + std::to_string(totalEdges));
			continue;
		}

		Standard_Real mid = (first + last) / 2.0;
		gp_Pnt midPnt = curve->Value(mid);

		gp_Vec n1 = getNormalAt(face1, midPnt);
		gp_Vec n2 = getNormalAt(face2, midPnt);

		gp_Vec view = midPnt.XYZ() - cameraPos.XYZ();
		double viewLength = view.Magnitude();
		if (viewLength < 1e-6) {
			LOG_INF_S("[SilhouetteDebug] View vector too small, skipping edge");
			continue;
		}
		view.Normalize();

		bool f1Front = n1.Dot(view) > 0;
		bool f2Front = n2.Dot(view) > 0;

		LOG_INF_S("[SilhouetteDebug] Edge " + std::to_string(totalEdges) + ": (" + std::to_string(midPnt.X()) + ", " + std::to_string(midPnt.Y()) + ", " + std::to_string(midPnt.Z()) + ") "
			+ " n1: (" + std::to_string(n1.X()) + ", " + std::to_string(n1.Y()) + ", " + std::to_string(n1.Z()) + ") "
			+ " n2: (" + std::to_string(n2.X()) + ", " + std::to_string(n2.Y()) + ", " + std::to_string(n2.Z()) + ") "
			+ " view: (" + std::to_string(view.X()) + ", " + std::to_string(view.Y()) + ", " + std::to_string(view.Z()) + ") "
			+ " f1Front: " + (f1Front ? "true" : "false") + ", f2Front: " + (f2Front ? "true" : "false"));

		// Only draw silhouette edges where one face is front-facing and the other is back-facing
		if (f1Front != f2Front) {
			gp_Pnt p1 = curve->Value(first);
			gp_Pnt p2 = curve->Value(last);
			points.push_back(p1);
			points.push_back(p2);
			indices.push_back(pointIndex++);
			indices.push_back(pointIndex++);
			indices.push_back(SO_END_LINE_INDEX);
			silhouetteCount++;
			LOG_INF_S("[SilhouetteDebug] This edge is a silhouette edge.");
		}
	}

	LOG_INF_S("[SilhouetteDebug] Total edges: " + std::to_string(totalEdges) + ", Edges with 2 faces: " + std::to_string(edgesWithTwoFaces) + ", Total silhouette edges: " + std::to_string(silhouetteCount));

	if (silhouetteEdgeNode) silhouetteEdgeNode->unref();
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(1.0, 0.0, 0.0);  // Red color for better visibility
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->lineWidth = 2.0;
	drawStyle->style = SoDrawStyle::LINES;
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(points.size());
	for (size_t i = 0; i < points.size(); ++i)
		coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	lineSet->coordIndex.setValues(0, indices.size(), indices.data());
	silhouetteEdgeNode = new SoSeparator;
	silhouetteEdgeNode->ref();
	silhouetteEdgeNode->addChild(mat);
	silhouetteEdgeNode->addChild(drawStyle);
	silhouetteEdgeNode->addChild(coords);
	silhouetteEdgeNode->addChild(lineSet);
}

void EdgeComponent::clearSilhouetteEdgeNode() {
	if (silhouetteEdgeNode) {
		silhouetteEdgeNode->unref();
		silhouetteEdgeNode = nullptr;
	}
}