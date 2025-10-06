#include "EdgeComponent.h"
#include "EdgeTypes.h"
#include "logger/Logger.h"
#include "config/SelectionColorConfig.h"
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
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTranslation.h>
#include <rendering/GeometryProcessor.h>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
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
	if (intersectionNodesNode) intersectionNodesNode->unref();
}

// Structure to hold edge processing data
struct EdgeProcessingData {
	TopoDS_Edge edge;
	Handle(Geom_Curve) curve;
	Standard_Real first, last;
	GeomAbs_CurveType curveType;
	bool isValid;
	bool passesLengthFilter;
	size_t pointCount;
	std::vector<gp_Pnt> sampledPoints;
};

void EdgeComponent::extractOriginalEdges(const TopoDS_Shape& shape, double samplingDensity, double minLength, bool showLinesOnly, const Quantity_Color& color, double width,
	bool highlightIntersectionNodes, const Quantity_Color& intersectionNodeColor, double intersectionNodeSize) {

	auto startTime = std::chrono::steady_clock::now();

	// Step 1: Pre-collect all edges and their basic information
	std::vector<EdgeProcessingData> edgeData;
	std::vector<TopoDS_Edge> allEdges;

	for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
		allEdges.push_back(TopoDS::Edge(exp.Current()));
	}

	edgeData.reserve(allEdges.size());

	LOG_INF_S("Optimizing original edge extraction for " + std::to_string(allEdges.size()) + " edges");

	// Step 2: Precompute edge data (parallel)
	std::mutex edgeDataMutex;
	std::atomic<size_t> validEdges{0};

	std::for_each(std::execution::par, allEdges.begin(), allEdges.end(), [&](const TopoDS_Edge& edge) {
		EdgeProcessingData data;
		data.edge = edge;

		Standard_Real first, last;
		Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

		if (curve.IsNull()) {
			data.isValid = false;
		} else {
			data.curve = curve;
			data.first = first;
			data.last = last;

			BRepAdaptor_Curve adaptor(edge);
			data.curveType = adaptor.GetType();
			data.isValid = true;

			validEdges++;
		}

		std::lock_guard<std::mutex> lock(edgeDataMutex);
		edgeData.push_back(data);
	});

	// Step 3: Parallel length filtering
	std::atomic<size_t> edgesPassingFilter{0};

	std::for_each(std::execution::par, edgeData.begin(), edgeData.end(), [&](EdgeProcessingData& data) {
		if (!data.isValid) return;

		// Quick length check using parameter range (much faster)
		gp_Pnt startPoint = data.curve->Value(data.first);
		gp_Pnt endPoint = data.curve->Value(data.last);
		double edgeLength = startPoint.Distance(endPoint);

		// For most cases, parameter range gives good approximation
		if (edgeLength >= minLength) {
			data.passesLengthFilter = true;
			edgesPassingFilter++;
		} else {
			// Only do expensive computation for borderline cases
			Standard_Real curveLength = data.last - data.first;
			if (curveLength > 0.01) { // Only for significant parameter ranges
				int numSamples = std::min(20, std::max(5, static_cast<int>(curveLength * 25)));
				double approximateLength = 0.0;
				gp_Pnt prev = data.curve->Value(data.first);
				for (int i = 1; i <= numSamples; ++i) {
					Standard_Real t = data.first + (data.last - data.first) * i / numSamples;
					gp_Pnt cur = data.curve->Value(t);
					approximateLength += prev.Distance(cur);
					prev = cur;
				}
				if (approximateLength >= minLength) {
					data.passesLengthFilter = true;
					edgesPassingFilter++;
				}
			}
		}
	});

	// Step 4: Parallel sampling
	std::atomic<size_t> totalPoints{0};

	std::for_each(std::execution::par, edgeData.begin(), edgeData.end(), [&](EdgeProcessingData& data) {
		if (!data.isValid || !data.passesLengthFilter) return;

		if (data.curveType == GeomAbs_Line || showLinesOnly) {
			// For lines, just use start and end points
			data.sampledPoints.push_back(data.curve->Value(data.first));
			data.sampledPoints.push_back(data.curve->Value(data.last));
			data.pointCount = 2;
		} else {
			// Adaptive sampling based on curve complexity
			Standard_Real curveLength = data.last - data.first;
			int baseSamples = std::max(4, static_cast<int>(curveLength * samplingDensity * 0.5)); // Reduced density

			// Adjust sampling based on curve type
			switch (data.curveType) {
				case GeomAbs_Circle:
				case GeomAbs_Ellipse:
					baseSamples = std::max(baseSamples, 16); // Circles need more points
					break;
				case GeomAbs_BSplineCurve:
				case GeomAbs_BezierCurve:
					baseSamples = std::max(baseSamples, 12); // Complex curves need more points
					break;
				default:
					break;
			}

			int numSamples = std::min(100, baseSamples); // Cap at 100 for performance

			data.sampledPoints.reserve(numSamples + 1);
			for (int i = 0; i <= numSamples; ++i) {
				Standard_Real t = data.first + (data.last - data.first) * i / numSamples;
				data.sampledPoints.push_back(data.curve->Value(t));
			}
			data.pointCount = data.sampledPoints.size();
		}

		totalPoints += data.pointCount;
	});

	// Step 5: Build final geometry data with optimized memory allocation
	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;

	size_t estimatedTotalPoints = totalPoints.load();
	size_t estimatedTotalIndices = estimatedTotalPoints * 3; // Each segment needs 3 indices

	points.reserve(estimatedTotalPoints);
	indices.reserve(estimatedTotalIndices);

	// Use more efficient batch insertion
	size_t pointIndex = 0;
	size_t processedEdges = 0;

	for (const auto& data : edgeData) {
		if (!data.isValid || !data.passesLengthFilter || data.sampledPoints.empty()) continue;

		size_t currentPointCount = data.sampledPoints.size();

		// Batch insert points
		points.insert(points.end(), data.sampledPoints.begin(), data.sampledPoints.end());

		// Batch insert indices for this edge
		size_t segmentCount = currentPointCount - 1;
		size_t startIndex = pointIndex;

		for (size_t i = 0; i < segmentCount; ++i) {
			indices.push_back(static_cast<int32_t>(startIndex + i));
			indices.push_back(static_cast<int32_t>(startIndex + i + 1));
			indices.push_back(SO_END_LINE_INDEX);
		}

		pointIndex += currentPointCount;
		processedEdges++;
	}

	// Trim excess capacity to save memory
	points.shrink_to_fit();
	indices.shrink_to_fit();

	auto endTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	LOG_INF_S("Optimized edge extraction completed in " + std::to_string(duration.count()) + "ms");
	LOG_INF_S("Statistics: " + std::to_string(validEdges.load()) + " valid edges, " +
			  std::to_string(edgesPassingFilter.load()) + " passed filter, " +
			  std::to_string(processedEdges) + " processed, " +
			  std::to_string(points.size()) + " points, " +
			  std::to_string(indices.size() / 3) + " line segments");

	if (originalEdgeNode) originalEdgeNode->unref();

	// Create material for original edges with custom color
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(color.Red(), color.Green(), color.Blue());

	// Create draw style for lines with custom width
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->lineWidth = static_cast<float>(width);

	// Create coordinates with optimized memory allocation
	SoCoordinate3* coords = new SoCoordinate3;
	if (!points.empty()) {
		coords->point.setNum(points.size());
		// Use batch setting for better performance
		SbVec3f* coordArray = coords->point.startEditing();
		for (size_t i = 0; i < points.size(); ++i) {
			coordArray[i].setValue(points[i].X(), points[i].Y(), points[i].Z());
		}
		coords->point.finishEditing();
	}

	// Create line set with optimized memory allocation
	SoIndexedLineSet* lineSet = nullptr;
	if (!indices.empty()) {
		lineSet = new SoIndexedLineSet;
		lineSet->coordIndex.setNum(indices.size());
		lineSet->coordIndex.setValues(0, indices.size(), indices.data());
	}

	// Create separator and add all components (optimized order for rendering)
	SoSeparator* sep = new SoSeparator;
	sep->addChild(mat);
	sep->addChild(drawStyle);
	sep->addChild(coords);
	if (lineSet) {
		sep->addChild(lineSet);
	}

	// Store the separator as originalEdgeNode for proper display management
	originalEdgeNode = sep;
	originalEdgeNode->ref();

	LOG_INF_S("Original edge extraction complete: " + std::to_string(points.size()) + " points, " +
		std::to_string(indices.size() / 3) + " line segments");

	// Generate intersection nodes if requested
	if (highlightIntersectionNodes) {
		std::vector<gp_Pnt> intersectionPoints;

		// Use the filtered edges that actually passed length filtering for intersection detection
		std::vector<TopoDS_Edge> filteredEdges;
		for (const auto& data : edgeData) {
			if (data.isValid && data.passesLengthFilter) {
				filteredEdges.push_back(data.edge);
			}
		}

		findEdgeIntersectionsFromEdges(filteredEdges, intersectionPoints);

		if (!intersectionPoints.empty()) {
			generateIntersectionNodesNode(intersectionPoints, intersectionNodeColor, intersectionNodeSize);
			LOG_INF_S("Generated " + std::to_string(intersectionPoints.size()) + " intersection nodes");
		} else {
			LOG_INF_S("No intersection points found");
		}
	}
}
// Structure to hold precomputed face data
struct FaceData {
	TopoDS_Face face;
	GeomAbs_SurfaceType surfaceType;
	gp_Vec normal;
	Standard_Real uMin, uMax, vMin, vMax;
	bool isValid;

	FaceData() : surfaceType(GeomAbs_Plane), isValid(false) {}
};

// Structure to hold edge processing data for features
struct FeatureEdgeData {
	TopoDS_Edge edge;
	std::vector<size_t> faceIndices; // Indices into precomputed face data
	Handle(Geom_Curve) curve;
	Standard_Real first, last;
	GeomAbs_CurveType curveType;
	bool isValid;
	bool isFeature;
	std::vector<gp_Pnt> sampledPoints;
};

void EdgeComponent::extractFeatureEdges(const TopoDS_Shape& shape, double featureAngle, double minLength, bool onlyConvex, bool onlyConcave) {

	auto startTime = std::chrono::steady_clock::now();

	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;
	int pointIndex = 0;
	double cosThreshold = std::cos(featureAngle * M_PI / 180.0);

	// Step 1: Precompute face data (parallel)
	std::vector<TopoDS_Face> allFaces;
	for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
		allFaces.push_back(TopoDS::Face(exp.Current()));
	}

	std::vector<FaceData> faceData(allFaces.size());
	std::mutex faceDataMutex;

	std::for_each(std::execution::par, allFaces.begin(), allFaces.end(), [&](const TopoDS_Face& face) {
		size_t faceIdx = &face - &allFaces[0]; // Calculate index
		FaceData& data = faceData[faceIdx];
		data.face = face;

		try {
			// Get surface type
			BRepAdaptor_Surface surf(face);
			data.surfaceType = surf.GetType();

			// Get UV bounds
			BRepTools::UVBounds(face, data.uMin, data.uMax, data.vMin, data.vMax);

			// Precompute normal at center of UV bounds
			Standard_Real midU = (data.uMin + data.uMax) / 2.0;
			Standard_Real midV = (data.vMin + data.vMax) / 2.0;
			gp_Pnt centerPoint;
			gp_Vec dU, dV;
			surf.D1(midU, midV, centerPoint, dU, dV);
			data.normal = dU.Crossed(dV);
			data.normal.Normalize();
			data.isValid = true;
		} catch (...) {
			data.isValid = false;
		}
	});

	// Step 2: Build edge->faces adjacency map
	TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
	TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

	// Step 3: Precompute edge data (parallel)
	std::vector<TopoDS_Edge> allEdges;
	allEdges.reserve(edgeFaceMap.Extent());
	for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
		allEdges.push_back(TopoDS::Edge(exp.Current()));
	}

	std::vector<FeatureEdgeData> edgeData(allEdges.size());

	std::for_each(std::execution::par, allEdges.begin(), allEdges.end(), [&](const TopoDS_Edge& edge) {
		size_t edgeIdx = &edge - &allEdges[0];
		FeatureEdgeData& data = edgeData[edgeIdx];
		data.edge = edge;

		// Get faces connected to this edge
		const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
		for (const auto& face : faces) {
			// Find face index
			for (size_t i = 0; i < allFaces.size(); ++i) {
				if (allFaces[i].IsSame(face)) {
					data.faceIndices.push_back(i);
					break;
				}
			}
		}

		// Get curve data
		Standard_Real first, last;
		Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
		if (!curve.IsNull()) {
			data.curve = curve;
			data.first = first;
			data.last = last;
			BRepAdaptor_Curve adaptor(edge);
			data.curveType = adaptor.GetType();
			data.isValid = true;
		}
	});

	size_t validEdges = 0;
	for (const auto& data : edgeData) {
		if (data.isValid) validEdges++;
	}

	LOG_INF_S("Optimizing feature edge extraction for " + std::to_string(validEdges) + " valid edges from " + std::to_string(allEdges.size()) + " total edges");
	LOG_INF_S("Precomputed data for " + std::to_string(allFaces.size()) + " faces and " + std::to_string(validEdges) + " edges");

	// Step 4: Parallel feature detection
	std::atomic<size_t> featureEdgesFound{0};

	std::for_each(std::execution::par, edgeData.begin(), edgeData.end(), [&](FeatureEdgeData& data) {
		if (!data.isValid || data.faceIndices.empty()) return;

		bool isFeatureEdge = false;
		double angleDegrees = 0.0;

		if (data.faceIndices.size() >= 2) {
			// Two faces case - use angle between face normals
			size_t faceIdx1 = data.faceIndices[0];
			size_t faceIdx2 = data.faceIndices[1];

			const FaceData& faceData1 = faceData[faceIdx1];
			const FaceData& faceData2 = faceData[faceIdx2];

			if (faceData1.isValid && faceData2.isValid) {
				// Use precomputed normals
				double cosAngle = faceData1.normal.Dot(faceData2.normal);
				angleDegrees = std::acos(std::abs(cosAngle)) * 180.0 / M_PI;

				if (cosAngle < cosThreshold) {
					isFeatureEdge = true;
				}

				// Check convexity/concavity for two-face edges
				if (isFeatureEdge && (onlyConvex || onlyConcave)) {
					gp_Pnt p1 = data.curve->Value(data.first);
					gp_Pnt p2 = data.curve->Value(data.last);
					gp_Vec edgeTangent(p1, p2);
					edgeTangent.Normalize();
					double cross = faceData1.normal.Crossed(faceData2.normal).Dot(edgeTangent);

					if (onlyConvex && cross <= 0) {
						isFeatureEdge = false;
					}
					if (onlyConcave && cross >= 0) {
						isFeatureEdge = false;
					}
				}
			}
		} else {
			// Single face case - use comprehensive feature detection
			size_t faceIdx = data.faceIndices[0];
			const FaceData& faceData1 = faceData[faceIdx];

			// Strategy 1: All curved edges are potential features
			if (data.curveType != GeomAbs_Line) {
				isFeatureEdge = true;
				angleDegrees = 45.0;
			}

			// Strategy 2: Check surface type
			if (!isFeatureEdge && faceData1.isValid) {
				if (faceData1.surfaceType == GeomAbs_Cylinder ||
					faceData1.surfaceType == GeomAbs_Cone ||
					faceData1.surfaceType == GeomAbs_Torus) {
					isFeatureEdge = true;
					angleDegrees = 30.0;
				}
			}

			// Strategy 3: Check edge length
			if (!isFeatureEdge) {
				gp_Pnt p1 = data.curve->Value(data.first);
				gp_Pnt p2 = data.curve->Value(data.last);
				double edgeLength = p1.Distance(p2);

				if (edgeLength > minLength) { // Use minLength instead of hardcoded 1.0
					isFeatureEdge = true;
					angleDegrees = 25.0;
				}
			}
		}

		data.isFeature = isFeatureEdge;
		if (isFeatureEdge) {
			featureEdgesFound++;
		}
	});

	// Step 5: Parallel sampling for feature edges
	std::atomic<size_t> totalSamples{0};

	std::for_each(std::execution::par, edgeData.begin(), edgeData.end(), [&](FeatureEdgeData& data) {
		if (!data.isValid || !data.isFeature) return;

		// Quick length check (reuse precomputed data where possible)
		gp_Pnt p1 = data.curve->Value(data.first);
		gp_Pnt p2 = data.curve->Value(data.last);
		double edgeLength = p1.Distance(p2);

		// For closed curves, use approximate length if needed
		if (edgeLength < minLength && std::abs(data.last - data.first) > 0.01) {
			const int samples = std::min(20, std::max(5, static_cast<int>((data.last - data.first) * 25)));
			edgeLength = 0.0;
			gp_Pnt prev = data.curve->Value(data.first);
			for (int i = 1; i <= samples; ++i) {
				Standard_Real t = data.first + (data.last - data.first) * i / samples;
				gp_Pnt cur = data.curve->Value(t);
				edgeLength += prev.Distance(cur);
				prev = cur;
			}
		}

		if (edgeLength < minLength) return;

		// Adaptive sampling based on curve type
		if (data.curveType == GeomAbs_Line) {
			data.sampledPoints.push_back(p1);
			data.sampledPoints.push_back(p2);
			totalSamples += 2;
		} else {
			Standard_Real curveLength = data.last - data.first;
			int baseSamples = std::max(12, static_cast<int>(curveLength * 80)); // Slightly less dense than before

			// Adjust based on curve type
			switch (data.curveType) {
				case GeomAbs_Circle:
				case GeomAbs_Ellipse:
					baseSamples = std::max(baseSamples, 24);
					break;
				case GeomAbs_BSplineCurve:
				case GeomAbs_BezierCurve:
					baseSamples = std::max(baseSamples, 16);
					break;
				default:
					break;
			}

			int numSamples = std::min(150, baseSamples); // Cap lower than original

			data.sampledPoints.reserve(numSamples + 1);
			for (int i = 0; i <= numSamples; ++i) {
				Standard_Real t = data.first + (data.last - data.first) * i / numSamples;
				data.sampledPoints.push_back(data.curve->Value(t));
			}
			totalSamples += data.sampledPoints.size();
		}
	});

	// Step 6: Build final geometry (sequential)
	points.reserve(totalSamples.load());
	indices.reserve(totalSamples.load() * 3);

	for (const auto& data : edgeData) {
		if (!data.isValid || !data.isFeature || data.sampledPoints.empty()) continue;

		// Add points
		for (const auto& point : data.sampledPoints) {
			points.push_back(point);
		}

		// Add indices
		for (size_t i = 0; i < data.sampledPoints.size() - 1; ++i) {
			indices.push_back(static_cast<int32_t>(pointIndex + i));
			indices.push_back(static_cast<int32_t>(pointIndex + i + 1));
			indices.push_back(SO_END_LINE_INDEX);
		}

		pointIndex += data.sampledPoints.size();
	}

	auto endTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	LOG_INF_S("Optimized feature edge extraction completed in " + std::to_string(duration.count()) + "ms");
	LOG_INF_S("Statistics: " + std::to_string(featureEdgesFound.load()) + " feature edges found, " +
			  std::to_string(points.size()) + " points, " +
			  std::to_string(indices.size() / 3) + " line segments");

	if (featureEdgeNode) featureEdgeNode->unref();

	// Create material for feature edges - use default red color instead of hardcoded blue
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(1, 0, 0); // Red (will be overridden by applyAppearanceToEdgeNode)

	// Create draw style for feature edges
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->lineWidth = 2.0f; // Thicker lines for better visibility
	drawStyle->linePattern = 0xFFFF; // Default to solid - will be overridden by applyAppearanceToEdgeNode

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
	case EdgeType::IntersectionNodes: return intersectionNodesNode;
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

	// Handle intersection nodes - they are always shown when original edges are shown and intersection nodes exist
	if (edgeFlags.showOriginalEdges && intersectionNodesNode) {
		if (parentNode->findChild(intersectionNodesNode) < 0) {
			parentNode->addChild(intersectionNodesNode);
			LOG_INF_S("Added intersection nodes to parent");
		}
	}
	else if (intersectionNodesNode) {
		int idx = parentNode->findChild(intersectionNodesNode);
		if (idx >= 0) {
			parentNode->removeChild(idx);
			LOG_INF_S("Removed intersection nodes from parent");
		}
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

void EdgeComponent::applyAppearanceToEdgeNode(EdgeType type, const Quantity_Color& color, double width, int style) {
	std::lock_guard<std::mutex> lock(m_nodeMutex);
	SoSeparator* node = getEdgeNode(type);
	if (!node) {
		LOG_WRN_S("applyAppearanceToEdgeNode: node is null for type " + std::to_string(static_cast<int>(type)));
		return;
	}

	LOG_INF_S("applyAppearanceToEdgeNode: type=" + std::to_string(static_cast<int>(type)) +
		", width=" + std::to_string(width) + ", style=" + std::to_string(style));

	// Update material color if present
	const int childCount = node->getNumChildren();
	if (childCount <= 0) {
		LOG_WRN_S("applyAppearanceToEdgeNode: node has no children");
		return;
	}

	for (int i = 0; i < childCount; ++i) {
		SoNode* child = node->getChild(i);
		if (!child) continue;
		if (SoMaterial* mat = dynamic_cast<SoMaterial*>(child)) {
			Standard_Real r, g, b;
			color.Values(r, g, b, Quantity_TOC_RGB);
			mat->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
			continue;
		}
		if (SoDrawStyle* drawStyle = dynamic_cast<SoDrawStyle*>(child)) {
			drawStyle->lineWidth = static_cast<float>(std::max(0.1, std::min(10.0, width)));

			// Set line pattern based on style
			// Coin3D line patterns: 0xFFFF = solid, 0xAAAA = dashed (alternating pixels), 0xCCCC = dotted (2 pixels on/off)
			switch (style) {
				case 0: // Solid
					drawStyle->linePattern = 0xFFFF;
					LOG_INF_S("Setting line pattern to SOLID (0xFFFF)");
					break;
				case 1: // Dashed
					drawStyle->linePattern = 0xAAAA; // Alternating pixels for dash
					LOG_INF_S("Setting line pattern to DASHED (0xAAAA)");
					break;
				case 2: // Dotted
					drawStyle->linePattern = 0xCCCC; // 2 pixels on, 2 pixels off
					LOG_INF_S("Setting line pattern to DOTTED (0xCCCC)");
					break;
				case 3: // Dash-Dot
					drawStyle->linePattern = 0xA9A9; // Dash-dot pattern
					LOG_INF_S("Setting line pattern to DASH-DOT (0xA9A9)");
					break;
				default:
					drawStyle->linePattern = 0xFFFF; // Default to solid
					LOG_INF_S("Setting line pattern to DEFAULT SOLID (0xFFFF)");
					break;
			}
			continue;
		}
	}
}
void EdgeComponent::generateHighlightEdgeNode() {
	// Generate highlight edge node for selected edges/faces
	// TODO: Implement highlight edge node generation based on selection state
	if (highlightEdgeNode) highlightEdgeNode->unref();

	// Create material for highlight edges
	SoMaterial* mat = new SoMaterial;
	if (SelectionColorConfig::getInstance().isInitialized()) {
		float r, g, b;
		SelectionColorConfig::getInstance().getSelectedHighlightEdgeColor(r, g, b);
		mat->diffuseColor.setValue(r, g, b);
	}
	else {
		mat->diffuseColor.setValue(1.0f, 1.0f, 0.6f); // Fallback to light yellow
	}

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

	// Check if we have valid data
	if (mesh.vertices.empty()) {
		LOG_WRN_S("Cannot generate normal lines: no vertices in mesh");
		return;
	}
	if (mesh.normals.empty()) {
		LOG_WRN_S("Cannot generate normal lines: no normals in mesh");
		return;
	}
	if (mesh.normals.size() != mesh.vertices.size()) {
		LOG_WRN_S("Cannot generate normal lines: normals count (" + std::to_string(mesh.normals.size()) + 
			") does not match vertices count (" + std::to_string(mesh.vertices.size()) + ")");
		return;
	}

	std::vector<gp_Pnt> points;
	std::vector<int32_t> indices;
	std::vector<float> colors; // Store colors for each line
	int pointIndex = 0;
	int normalCount = 0;
	int correctNormalCount = 0;
	int incorrectNormalCount = 0;
	int zeroNormalCount = 0;

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
		} else {
			zeroNormalCount++;
		}
	}

	LOG_INF_S("Generated " + std::to_string(normalCount) + " normal lines from " +
		std::to_string(mesh.vertices.size()) + " vertices");
	LOG_INF_S("Correct normals: " + std::to_string(correctNormalCount) + 
		", Incorrect normals: " + std::to_string(incorrectNormalCount) +
		", Zero normals: " + std::to_string(zeroNormalCount));

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

// Axis-Aligned Bounding Box structure
struct AABB {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;

    AABB() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {}

    bool intersects(const AABB& other) const {
        return !(maxX < other.minX || other.maxX < minX ||
                 maxY < other.minY || other.maxY < minY ||
                 maxZ < other.minZ || other.maxZ < minZ);
    }

    void expand(const gp_Pnt& point) {
        minX = std::min(minX, point.X());
        minY = std::min(minY, point.Y());
        minZ = std::min(minZ, point.Z());
        maxX = std::max(maxX, point.X());
        maxY = std::max(maxY, point.Y());
        maxZ = std::max(maxZ, point.Z());
    }

    void expand(double margin) {
        minX -= margin;
        minY -= margin;
        minZ -= margin;
        maxX += margin;
        maxY += margin;
        maxZ += margin;
    }
};

// Edge data structure with bounding box
struct EdgeData {
    TopoDS_Edge edge;
    AABB bbox;
    Handle(Geom_Curve) curve;
    Standard_Real first, last;
    int gridX, gridY, gridZ; // Grid cell coordinates
};

// Helper function to find intersections from a pre-filtered edge list
void EdgeComponent::findEdgeIntersectionsFromEdges(const std::vector<class TopoDS_Edge>& edges, std::vector<gp_Pnt>& intersectionPoints) {
    auto startTime = std::chrono::steady_clock::now();

    LOG_INF_S("Finding intersections from " + std::to_string(edges.size()) + " filtered edges");

    // For small number of edges, use simpler approach
    if (edges.size() < 50) {
        findEdgeIntersectionsSimple(edges, intersectionPoints);
        return;
    }

    // For larger models, use the optimized spatial approach
    // Calculate global bounding box from edges
    Bnd_Box globalBbox;
    for (const auto& edge : edges) {
        BRepBndLib::Add(edge, globalBbox);
    }
    double xmin, ymin, zmin, xmax, ymax, zmax;
    globalBbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double diagonal = sqrt((xmax - xmin) * (xmax - xmin) + (ymax - ymin) * (ymax - ymin) + (zmax - zmin) * (zmax - zmin));
    const double tolerance = diagonal * 0.005; // 0.5% of diagonal as tolerance
    const double bboxMargin = tolerance * 2.0; // Add margin to bounding boxes

    // Create edge data with bounding boxes
    std::vector<EdgeData> edgeData;
    edgeData.reserve(edges.size());

    // Determine grid size for spatial partitioning (aim for ~10 edges per cell)
    const int targetEdgesPerCell = 10;
    int gridSize = std::max(1, static_cast<int>(std::cbrt(edges.size() / targetEdgesPerCell)));
    double gridSizeX = (xmax - xmin) / gridSize;
    double gridSizeY = (ymax - ymin) / gridSize;
    double gridSizeZ = (zmax - zmin) / gridSize;

    LOG_INF_S("Using " + std::to_string(gridSize) + "x" + std::to_string(gridSize) + "x" + std::to_string(gridSize) +
              " grid for spatial partitioning, tolerance: " + std::to_string(tolerance));

    // Precompute edge data
    for (const auto& edge : edges) {
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

        if (curve.IsNull()) continue;

        EdgeData data;
        data.edge = edge;
        data.curve = curve;
        data.first = first;
        data.last = last;

        // Compute bounding box by sampling the curve
        const int bboxSamples = std::min(20, std::max(5, static_cast<int>((last - first) * 50)));
        data.bbox = AABB();

        for (int i = 0; i <= bboxSamples; ++i) {
            Standard_Real t = first + (last - first) * i / bboxSamples;
            gp_Pnt point = curve->Value(t);
            if (i == 0) {
                data.bbox.minX = data.bbox.maxX = point.X();
                data.bbox.minY = data.bbox.maxY = point.Y();
                data.bbox.minZ = data.bbox.maxZ = point.Z();
            } else {
                data.bbox.expand(point);
            }
        }

        // Expand bounding box with margin
        data.bbox.expand(bboxMargin);

        // Assign to grid cell
        data.gridX = std::max(0, std::min(gridSize - 1, static_cast<int>((data.bbox.minX - xmin) / gridSizeX)));
        data.gridY = std::max(0, std::min(gridSize - 1, static_cast<int>((data.bbox.minY - ymin) / gridSizeY)));
        data.gridZ = std::max(0, std::min(gridSize - 1, static_cast<int>((data.bbox.minZ - zmin) / gridSizeZ)));

        edgeData.push_back(data);
    }

    LOG_INF_S("Precomputed " + std::to_string(edgeData.size()) + " edge bounding boxes");

    // Create spatial grid
    std::vector<std::vector<std::vector<std::vector<size_t>>>> grid(
        gridSize, std::vector<std::vector<std::vector<size_t>>>(
        gridSize, std::vector<std::vector<size_t>>(
        gridSize, std::vector<size_t>())));

    // Assign edges to grid cells
    for (size_t i = 0; i < edgeData.size(); ++i) {
        const auto& data = edgeData[i];
        grid[data.gridX][data.gridY][data.gridZ].push_back(i);
    }

    // Count total potential comparisons
    size_t totalPotentialComparisons = 0;
    for (const auto& gridX : grid) {
        for (const auto& gridY : gridX) {
            for (const auto& gridZ : gridY) {
                size_t cellSize = gridZ.size();
                totalPotentialComparisons += cellSize * (cellSize - 1) / 2;
            }
        }
    }

    LOG_INF_S("Grid contains " + std::to_string(totalPotentialComparisons) + " potential edge pair comparisons");

    // Check intersections using spatial partitioning
    std::mutex intersectionMutex; // For thread-safe access to intersectionPoints

    // Process grid cells in parallel
    std::atomic<size_t> processedComparisons{0};
    std::atomic<size_t> bboxFiltered{0};
    std::atomic<size_t> distanceFiltered{0};

    auto processCellRange = [&](size_t startCell, size_t endCell) {
        for (size_t cellIdx = startCell; cellIdx < endCell; ++cellIdx) {
            size_t gx = cellIdx / (gridSize * gridSize);
            size_t gy = (cellIdx / gridSize) % gridSize;
            size_t gz = cellIdx % gridSize;

            const auto& cellEdges = grid[gx][gy][gz];
            if (cellEdges.size() < 2) continue;

            // Check all pairs within this cell
            for (size_t i = 0; i < cellEdges.size(); ++i) {
                for (size_t j = i + 1; j < cellEdges.size(); ++j) {
                    size_t edgeIdx1 = cellEdges[i];
                    size_t edgeIdx2 = cellEdges[j];

                    processedComparisons++;

                    const auto& data1 = edgeData[edgeIdx1];
                    const auto& data2 = edgeData[edgeIdx2];

                    // AABB intersection test
                    if (!data1.bbox.intersects(data2.bbox)) {
                        bboxFiltered++;
                        continue;
                    }

                    // Compute minimum distance between curves
                    double minDistance = computeMinDistanceBetweenCurves(data1, data2);

                    if (minDistance > tolerance) {
                        distanceFiltered++;
                        continue;
                    }

                    // Found intersection
                    gp_Pnt intersectionPoint = computeIntersectionPoint(data1, data2);

                    // Thread-safe addition to result
                    std::lock_guard<std::mutex> lock(intersectionMutex);

                    // Check if this intersection point is already found
                    bool alreadyFound = false;
                    for (const auto& existingPoint : intersectionPoints) {
                        if (intersectionPoint.Distance(existingPoint) < tolerance) {
                            alreadyFound = true;
                            break;
                        }
                    }

                    if (!alreadyFound) {
                        intersectionPoints.push_back(intersectionPoint);
                    }
                }
            }
        }
    };

    // Use parallel processing for large models
    const size_t totalCells = gridSize * gridSize * gridSize;
    const size_t numThreads = std::min(8u, std::max(1u, std::thread::hardware_concurrency()));
    const size_t cellsPerThread = (totalCells + numThreads - 1) / numThreads;

    std::vector<std::thread> threads;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startCell = t * cellsPerThread;
        size_t endCell = std::min(startCell + cellsPerThread, totalCells);
        threads.emplace_back(processCellRange, startCell, endCell);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    LOG_INF_S("Intersection detection completed in " + std::to_string(duration.count()) + "ms");
    LOG_INF_S("Statistics: " + std::to_string(processedComparisons.load()) + " comparisons, " +
              std::to_string(bboxFiltered.load()) + " filtered by AABB, " +
              std::to_string(distanceFiltered.load()) + " filtered by distance");
    LOG_INF_S("Found " + std::to_string(intersectionPoints.size()) + " intersection points");
}

void EdgeComponent::findEdgeIntersections(const TopoDS_Shape& shape, std::vector<gp_Pnt>& intersectionPoints) {
    auto startTime = std::chrono::steady_clock::now();

    // Collect all edges
    std::vector<TopoDS_Edge> edges;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edges.push_back(TopoDS::Edge(exp.Current()));
    }

    findEdgeIntersectionsFromEdges(edges, intersectionPoints);
}

// Simple intersection detection for small number of edges
void EdgeComponent::findEdgeIntersectionsSimple(const std::vector<class TopoDS_Edge>& edges, std::vector<gp_Pnt>& intersectionPoints) {
    LOG_INF_S("Using simple intersection detection for " + std::to_string(edges.size()) + " edges");

    // Calculate bounding box for tolerance
    Bnd_Box bbox;
    for (const auto& edge : edges) {
        BRepBndLib::Add(edge, bbox);
    }
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    double diagonal = sqrt((xmax - xmin) * (xmax - xmin) + (ymax - ymin) * (ymax - ymin) + (zmax - zmin) * (zmax - zmin));
    const double tolerance = diagonal * 0.01; // 1% of diagonal as tolerance

    // Simple pairwise comparison
    for (size_t i = 0; i < edges.size(); ++i) {
        for (size_t j = i + 1; j < edges.size(); ++j) {
            const TopoDS_Edge& edge1 = edges[i];
            const TopoDS_Edge& edge2 = edges[j];

            Standard_Real first1, last1, first2, last2;
            Handle(Geom_Curve) curve1 = BRep_Tool::Curve(edge1, first1, last1);
            Handle(Geom_Curve) curve2 = BRep_Tool::Curve(edge2, first2, last2);

            if (curve1.IsNull() || curve2.IsNull()) continue;

            // Use simplified intersection detection - check only endpoints and midpoints
            gp_Pnt start1 = curve1->Value(first1);
            gp_Pnt end1 = curve1->Value(last1);
            gp_Pnt mid1 = curve1->Value((first1 + last1) / 2.0);

            gp_Pnt start2 = curve2->Value(first2);
            gp_Pnt end2 = curve2->Value(last2);
            gp_Pnt mid2 = curve2->Value((first2 + last2) / 2.0);

            // Check distances between key points
            double minDistance = std::numeric_limits<double>::max();
            gp_Pnt closestPoint1, closestPoint2;

            std::vector<gp_Pnt> points1 = {start1, mid1, end1};
            std::vector<gp_Pnt> points2 = {start2, mid2, end2};

            for (const auto& p1 : points1) {
                for (const auto& p2 : points2) {
                    double dist = p1.Distance(p2);
                    if (dist < minDistance) {
                        minDistance = dist;
                        closestPoint1 = p1;
                        closestPoint2 = p2;
                    }
                }
            }

            // If the curves are close enough, consider it an intersection
            if (minDistance < tolerance) {
                gp_Pnt intersectionPoint = gp_Pnt(
                    (closestPoint1.X() + closestPoint2.X()) / 2.0,
                    (closestPoint1.Y() + closestPoint2.Y()) / 2.0,
                    (closestPoint1.Z() + closestPoint2.Z()) / 2.0
                );

                // Check if this intersection point is already found
                bool alreadyFound = false;
                for (const auto& existingPoint : intersectionPoints) {
                    if (intersectionPoint.Distance(existingPoint) < tolerance) {
                        alreadyFound = true;
                        break;
                    }
                }

                if (!alreadyFound) {
                    intersectionPoints.push_back(intersectionPoint);
                }
            }
        }
    }

    LOG_INF_S("Simple intersection detection found " + std::to_string(intersectionPoints.size()) + " intersection points");
}

// Compute minimum distance between two curves using sampling
double EdgeComponent::computeMinDistanceBetweenCurves(const EdgeData& data1, const EdgeData& data2) {
    const int samples = 15; // Reduced sampling for speed
    double minDistance = std::numeric_limits<double>::max();

    for (int i = 0; i <= samples; ++i) {
        Standard_Real t1 = data1.first + (data1.last - data1.first) * i / samples;
        gp_Pnt p1 = data1.curve->Value(t1);

        for (int j = 0; j <= samples; ++j) {
            Standard_Real t2 = data2.first + (data2.last - data2.first) * j / samples;
            gp_Pnt p2 = data2.curve->Value(t2);

            double dist = p1.Distance(p2);
            if (dist < minDistance) {
                minDistance = dist;
            }
        }
    }

    return minDistance;
}

// Compute intersection point as midpoint of closest points
gp_Pnt EdgeComponent::computeIntersectionPoint(const EdgeData& data1, const EdgeData& data2) {
    const int samples = 10;
    double minDistance = std::numeric_limits<double>::max();
    gp_Pnt closest1, closest2;

    for (int i = 0; i <= samples; ++i) {
        Standard_Real t1 = data1.first + (data1.last - data1.first) * i / samples;
        gp_Pnt p1 = data1.curve->Value(t1);

        for (int j = 0; j <= samples; ++j) {
            Standard_Real t2 = data2.first + (data2.last - data2.first) * j / samples;
            gp_Pnt p2 = data2.curve->Value(t2);

            double dist = p1.Distance(p2);
            if (dist < minDistance) {
                minDistance = dist;
                closest1 = p1;
                closest2 = p2;
            }
        }
    }

    return gp_Pnt(
        (closest1.X() + closest2.X()) / 2.0,
        (closest1.Y() + closest2.Y()) / 2.0,
        (closest1.Z() + closest2.Z()) / 2.0
    );
}

void EdgeComponent::generateIntersectionNodesNode(const std::vector<gp_Pnt>& intersectionPoints, const Quantity_Color& color, double size) {
	if (intersectionPoints.empty()) {
		LOG_INF_S("No intersection points to render");
		return;
	}
	
	LOG_INF_S("Generating " + std::to_string(intersectionPoints.size()) + " intersection nodes");
	
	// Clear existing intersection nodes
	if (intersectionNodesNode) {
		intersectionNodesNode->unref();
		intersectionNodesNode = nullptr;
	}
	
	// Create material for intersection nodes
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(color.Red(), color.Green(), color.Blue());
	
	// Create coordinates for all intersection points
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(intersectionPoints.size());
	for (size_t i = 0; i < intersectionPoints.size(); ++i) {
		const gp_Pnt& point = intersectionPoints[i];
		coords->point.set1Value(i, point.X(), point.Y(), point.Z());
	}
	
	// Create sphere geometry for each intersection point
	SoSeparator* sep = new SoSeparator;
	sep->addChild(mat);
	
	for (size_t i = 0; i < intersectionPoints.size(); ++i) {
		SoSeparator* nodeSep = new SoSeparator;
		
		// Create translation to position the sphere
		SoTranslation* translation = new SoTranslation;
		const gp_Pnt& point = intersectionPoints[i];
		translation->translation.setValue(point.X(), point.Y(), point.Z());
		
		// Create sphere
		SoSphere* sphere = new SoSphere;
		sphere->radius = static_cast<float>(size * 0.01); // Make spheres more visible
		
		nodeSep->addChild(translation);
		nodeSep->addChild(sphere);
		sep->addChild(nodeSep);
	}
	
	intersectionNodesNode = sep;
	intersectionNodesNode->ref();
	
	LOG_INF_S("Successfully created intersection nodes node with " + std::to_string(intersectionPoints.size()) + " spheres");
}