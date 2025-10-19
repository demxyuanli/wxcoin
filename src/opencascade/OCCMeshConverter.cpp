#include "OCCMeshConverter.h"
#include "logger/Logger.h"
#include "config/RenderingConfig.h"
#include "config/EdgeSettingsConfig.h"

// OpenCASCADE includes
#include <BRepMesh_IncrementalMesh.hxx>
#include <IMeshTools_Parameters.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>

// Coin3D includes
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoNormalBinding.h>

// STL includes
#include <cmath>
#include <set>
#include <vector>
#include <map>
#include <algorithm>

// Mathematical constants
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Static member initialization
bool OCCMeshConverter::s_showEdges = false; // Changed to false to avoid conflicts with new EdgeComponent system
double OCCMeshConverter::s_featureEdgeAngle = 45.0;  // Increased threshold for better feature edge detection
bool OCCMeshConverter::s_smoothingEnabled = true;
bool OCCMeshConverter::s_subdivisionEnabled = false;
int OCCMeshConverter::s_subdivisionLevels = 2;
double OCCMeshConverter::s_creaseAngle = 30.0;

void OCCMeshConverter::setShowEdges(bool show)
{
	s_showEdges = show;
	EdgeSettingsConfig& edgeConfig = EdgeSettingsConfig::getInstance();
	edgeConfig.setGlobalShowEdges(show);
}

void OCCMeshConverter::setFeatureEdgeAngle(double angleDegrees)
{
	s_featureEdgeAngle = angleDegrees;
}

void OCCMeshConverter::setSmoothingEnabled(bool enabled)
{
	s_smoothingEnabled = enabled;
}

void OCCMeshConverter::setSubdivisionEnabled(bool enabled)
{
	s_subdivisionEnabled = enabled;
}

void OCCMeshConverter::setSubdivisionLevels(int levels)
{
	s_subdivisionLevels = levels;
}

void OCCMeshConverter::setCreaseAngle(double angle)
{
	s_creaseAngle = angle;
}

TriangleMesh OCCMeshConverter::convertToMesh(const TopoDS_Shape& shape,
	const MeshParameters& params)
{
	TriangleMesh mesh;

	if (shape.IsNull()) {
		LOG_WRN_S("Cannot convert null shape to mesh");
		return mesh;
	}

	try {
		// Create incremental mesh with proper parameters for seam edges
		IMeshTools_Parameters meshParams;
		meshParams.Deflection = params.deflection;
		meshParams.Angle = params.angularDeflection;
		meshParams.Relative = params.relative;
		meshParams.InParallel = params.inParallel;
		meshParams.MinSize = Precision::Confusion();
		meshParams.InternalVerticesMode = Standard_True;  // Critical: ensure internal vertices are created for seam edges
		meshParams.ControlSurfaceDeflection = Standard_True;  // Better surface approximation
		
		BRepMesh_IncrementalMesh meshGen;
		meshGen.SetShape(shape);
		meshGen.ChangeParameters() = meshParams;
		meshGen.Perform();

		if (!meshGen.IsDone()) {
			LOG_ERR_S("Failed to generate mesh for shape");
			return mesh;
		}

		// Extract triangles from all faces
		TopExp_Explorer faceExplorer(shape, TopAbs_FACE);
		for (; faceExplorer.More(); faceExplorer.Next()) {
			const TopoDS_Face& face = TopoDS::Face(faceExplorer.Current());
			meshFace(face, mesh, params);
		}

		// Calculate normals if not already done
		if (mesh.normals.empty() && !mesh.vertices.empty()) {
			calculateNormals(mesh);
		}

		// Apply smoothing if enabled
		if (s_smoothingEnabled) {
			mesh = smoothNormals(mesh, s_creaseAngle, 2);  // Reduced iterations for better performance
		}

		// Apply subdivision if enabled
		if (s_subdivisionEnabled) {
			mesh = createSubdivisionSurface(mesh, s_subdivisionLevels);
		}

		LOG_DBG_S("Generated mesh with " + std::to_string(mesh.getVertexCount()) +
			" vertices and " + std::to_string(mesh.getTriangleCount()) + " triangles");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception in mesh conversion: " + std::string(e.what()));
		mesh.clear();
	}

	return mesh;
}

TriangleMesh OCCMeshConverter::convertToMesh(const TopoDS_Shape& shape, double deflection)
{
	MeshParameters params;
	params.deflection = deflection;
	return convertToMesh(shape, params);
}

void OCCMeshConverter::calculateNormals(TriangleMesh& mesh)
{
	if (mesh.vertices.empty() || mesh.triangles.empty()) {
		return;
	}

	// Initialize normals array
	mesh.normals.resize(mesh.vertices.size());
	for (auto& normal : mesh.normals) {
		normal = gp_Vec(0, 0, 0);
	}

	// Calculate face normals and accumulate at vertices
	for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
		int i0 = mesh.triangles[i];
		int i1 = mesh.triangles[i + 1];
		int i2 = mesh.triangles[i + 2];

		if (i0 >= 0 && i0 < static_cast<int>(mesh.vertices.size()) &&
			i1 >= 0 && i1 < static_cast<int>(mesh.vertices.size()) &&
			i2 >= 0 && i2 < static_cast<int>(mesh.vertices.size())) {
			const gp_Pnt& v0 = mesh.vertices[i0];
			const gp_Pnt& v1 = mesh.vertices[i1];
			const gp_Pnt& v2 = mesh.vertices[i2];

			gp_Vec normal = calculateTriangleNormalVec(v0, v1, v2);

			// Accumulate normal at each vertex
			mesh.normals[i0] += normal;
			mesh.normals[i1] += normal;
			mesh.normals[i2] += normal;
		}
	}

	// Normalize accumulated normals
	for (auto& normal : mesh.normals) {
		double length = normal.Magnitude();
		if (length > 1e-6) {
			normal = normal / length;
		}
	}
}

TriangleMesh OCCMeshConverter::smoothNormals(const TriangleMesh& mesh, double creaseAngle, int iterations)
{
	TriangleMesh result = mesh;

	if (result.vertices.empty() || result.triangles.empty()) {
		return result;
	}

	// Step 1: Build adjacency relationships
	// Data structure: vertex -> adjacent faces -> face normals mapping
	std::vector<std::vector<int>> vertexToFaces(result.vertices.size());
	std::vector<gp_Vec> faceNormals;
	std::set<int> boundaryVertices;

	// Build vertex-to-faces mapping and calculate face normals
	for (size_t faceIdx = 0; faceIdx < result.triangles.size(); faceIdx += 3) {
		int v0 = result.triangles[faceIdx];
		int v1 = result.triangles[faceIdx + 1];
		int v2 = result.triangles[faceIdx + 2];

		// Add face to vertex adjacency lists
		vertexToFaces[v0].push_back(static_cast<int>(faceIdx / 3));
		vertexToFaces[v1].push_back(static_cast<int>(faceIdx / 3));
		vertexToFaces[v2].push_back(static_cast<int>(faceIdx / 3));

		// Step 2: Calculate face normal using cross product
		gp_Vec faceNormal = calculateTriangleNormalVec(
			result.vertices[v0], result.vertices[v1], result.vertices[v2]);
		faceNormals.push_back(faceNormal);
	}

	// Identify boundary edges and boundary vertices
	std::set<std::pair<int, int>> boundaryEdges = findBoundaryEdges(mesh);
	for (const auto& edge : boundaryEdges) {
		boundaryVertices.insert(edge.first);
		boundaryVertices.insert(edge.second);
	}

	// Convert angle threshold to radians and cosine
	double creaseAngleRad = creaseAngle * M_PI / 180.0;
	double cosThreshold = cos(creaseAngleRad);

	// Step 4: Iterative smoothing (Taubin/Laplace style)
	for (int iteration = 0; iteration < iterations; ++iteration) {
		std::vector<gp_Vec> newNormals(result.vertices.size(), gp_Vec(0, 0, 0));
		std::vector<double> normalWeights(result.vertices.size(), 0.0);

		// Process each vertex
		for (size_t vertexIdx = 0; vertexIdx < result.vertices.size(); ++vertexIdx) {
			// Skip boundary vertices - they don't participate in smoothing
			if (boundaryVertices.find(static_cast<int>(vertexIdx)) != boundaryVertices.end()) {
				continue;
			}

			const std::vector<int>& adjacentFaces = vertexToFaces[vertexIdx];
			if (adjacentFaces.empty()) {
				continue;
			}

			// Get current vertex normal (normalized)
			gp_Vec currentNormal = result.normals[vertexIdx];
			if (currentNormal.Magnitude() < 1e-6) {
				continue;
			}
			currentNormal = currentNormal / currentNormal.Magnitude();

			// Step 3: Angle threshold filtering and weighted averaging
			double totalWeight = 0.0;
			gp_Vec accumulatedNormal(0, 0, 0);

			for (int faceIdx : adjacentFaces) {
				if (faceIdx >= 0 && faceIdx < static_cast<int>(faceNormals.size())) {
					gp_Vec faceNormal = faceNormals[faceIdx];

					// Normalize face normal
					double faceNormalLength = faceNormal.Magnitude();
					if (faceNormalLength < 1e-6) {
						continue;
					}
					faceNormal = faceNormal / faceNormalLength;

					// Calculate angle between current vertex normal and face normal
					double cosAngle = currentNormal.Dot(faceNormal);
					cosAngle = std::max(-1.0, std::min(1.0, cosAngle)); // Clamp to [-1, 1]

					// Only include faces within the angle threshold
					if (cosAngle >= cosThreshold) {
						// Equal weight averaging (can be extended to area or angle weighted)
						double weight = 1.0;
						accumulatedNormal += faceNormal * weight;
						totalWeight += weight;
					}
				}
			}

			// Apply weighted average if we have valid contributions
			if (totalWeight > 0.0) {
				accumulatedNormal = accumulatedNormal / totalWeight;
				double length = accumulatedNormal.Magnitude();
				if (length > 1e-6) {
					newNormals[vertexIdx] = accumulatedNormal / length;
					normalWeights[vertexIdx] = totalWeight;
				}
			}
		}

		// Update vertex normals for non-boundary vertices
		for (size_t i = 0; i < result.vertices.size(); ++i) {
			if (boundaryVertices.find(static_cast<int>(i)) == boundaryVertices.end() &&
				normalWeights[i] > 0.0) {
				result.normals[i] = newNormals[i];
			}
			// Boundary vertices keep their original normals
		}
	}

	return result;
}

TriangleMesh OCCMeshConverter::createSubdivisionSurface(const TriangleMesh& mesh, int levels)
{
	TriangleMesh result = mesh;

	for (int level = 0; level < levels; ++level) {
		TriangleMesh subdivided;

		// Create edge points
		std::map<std::pair<int, int>, int> edgePointMap;
		std::vector<gp_Pnt> edgePoints;

		for (size_t i = 0; i < result.triangles.size(); i += 3) {
			int v0 = result.triangles[i];
			int v1 = result.triangles[i + 1];
			int v2 = result.triangles[i + 2];

			// Process each edge
			auto processEdge = [&](int a, int b) {
				if (a > b) std::swap(a, b);
				auto edge = std::make_pair(a, b);

				if (edgePointMap.find(edge) == edgePointMap.end()) {
					// Create edge midpoint
					gp_Pnt edgePoint = gp_Pnt(
						(result.vertices[a].X() + result.vertices[b].X()) / 2.0,
						(result.vertices[a].Y() + result.vertices[b].Y()) / 2.0,
						(result.vertices[a].Z() + result.vertices[b].Z()) / 2.0
					);
					edgePoints.push_back(edgePoint);
					edgePointMap[edge] = edgePoints.size() - 1;
				}
				};

			processEdge(v0, v1);
			processEdge(v1, v2);
			processEdge(v2, v0);
		}

		// Create new vertices (Loop subdivision vertex rule)
		std::vector<gp_Pnt> newVertices;
		for (size_t i = 0; i < result.vertices.size(); ++i) {
			// Find connected vertices
			std::vector<int> connectedVertices;
			for (size_t j = 0; j < result.triangles.size(); j += 3) {
				int t0 = result.triangles[j];
				int t1 = result.triangles[j + 1];
				int t2 = result.triangles[j + 2];

				if (t0 == static_cast<int>(i)) {
					if (t1 != static_cast<int>(i)) connectedVertices.push_back(t1);
					if (t2 != static_cast<int>(i)) connectedVertices.push_back(t2);
				}
				else if (t1 == static_cast<int>(i)) {
					if (t0 != static_cast<int>(i)) connectedVertices.push_back(t0);
					if (t2 != static_cast<int>(i)) connectedVertices.push_back(t2);
				}
				else if (t2 == static_cast<int>(i)) {
					if (t0 != static_cast<int>(i)) connectedVertices.push_back(t0);
					if (t1 != static_cast<int>(i)) connectedVertices.push_back(t1);
				}
			}

			// Calculate new vertex position using Loop vertex rule
			if (connectedVertices.size() >= 3) {
				double beta = 0.0;
				if (connectedVertices.size() == 3) {
					beta = 3.0 / 16.0;
				}
				else {
					beta = 3.0 / (8.0 * connectedVertices.size());
				}

				gp_Pnt sum(0, 0, 0);
				for (int connectedVertex : connectedVertices) {
					sum = gp_Pnt(sum.X() + result.vertices[connectedVertex].X(),
						sum.Y() + result.vertices[connectedVertex].Y(),
						sum.Z() + result.vertices[connectedVertex].Z());
				}

				double newX = (1.0 - connectedVertices.size() * beta) * result.vertices[i].X() + beta * sum.X();
				double newY = (1.0 - connectedVertices.size() * beta) * result.vertices[i].Y() + beta * sum.Y();
				double newZ = (1.0 - connectedVertices.size() * beta) * result.vertices[i].Z() + beta * sum.Z();

				newVertices.push_back(gp_Pnt(newX, newY, newZ));
			}
			else {
				newVertices.push_back(result.vertices[i]);
			}
		}

		// Create subdivided mesh
		subdivided.vertices = newVertices;

		// Add edge points to vertices
		size_t edgePointOffset = subdivided.vertices.size();
		for (const auto& ep : edgePoints) {
			subdivided.vertices.push_back(ep);
		}

		// Create new triangles
		for (size_t i = 0; i < result.triangles.size(); i += 3) {
			int v0 = result.triangles[i];
			int v1 = result.triangles[i + 1];
			int v2 = result.triangles[i + 2];

			// Get edge points
			int e0 = edgePointOffset + edgePointMap[std::make_pair(std::min(v0, v1), std::max(v0, v1))];
			int e1 = edgePointOffset + edgePointMap[std::make_pair(std::min(v1, v2), std::max(v1, v2))];
			int e2 = edgePointOffset + edgePointMap[std::make_pair(std::min(v2, v0), std::max(v2, v0))];

			// Create 4 new triangles
			subdivided.triangles.push_back(v0); subdivided.triangles.push_back(e0); subdivided.triangles.push_back(e2);
			subdivided.triangles.push_back(e0); subdivided.triangles.push_back(v1); subdivided.triangles.push_back(e1);
			subdivided.triangles.push_back(e2); subdivided.triangles.push_back(e1); subdivided.triangles.push_back(v2);
			subdivided.triangles.push_back(e0); subdivided.triangles.push_back(e1); subdivided.triangles.push_back(e2);
		}

		result = subdivided;
	}

	// Calculate normals for the final mesh
	calculateNormals(result);

	return result;
}

void OCCMeshConverter::flipNormals(TriangleMesh& mesh)
{
	// Flip triangle winding order
	for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
		std::swap(mesh.triangles[i + 1], mesh.triangles[i + 2]);
	}

	// Flip normal directions
	for (auto& normal : mesh.normals) {
		normal = -normal;
	}
}

gp_Vec OCCMeshConverter::calculateTriangleNormalVec(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3)
{
	gp_Vec v1(p1, p2);
	gp_Vec v2(p1, p3);
	gp_Vec normal = v1.Crossed(v2);

	double length = normal.Magnitude();
	if (length > Precision::Confusion()) {
		normal = normal / length;
	}

	return normal;
}

void OCCMeshConverter::meshFace(const TopoDS_Shape& face, TriangleMesh& mesh, const MeshParameters& params)
{
	if (face.ShapeType() != TopAbs_FACE) {
		return;
	}

	const TopoDS_Face& topoFace = TopoDS::Face(face);
	TopLoc_Location location;
	Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(topoFace, location);

	// If triangulation exists, extract it
	if (!triangulation.IsNull()) {
		extractTriangulation(triangulation, location, mesh, topoFace.Orientation());
	}
	else {
		// If no triangulation exists, create one
		BRepMesh_IncrementalMesh mesher(topoFace, params.deflection, params.relative, params.angularDeflection,
			params.inParallel);
		triangulation = BRep_Tool::Triangulation(topoFace, location);
		if (!triangulation.IsNull()) {
			extractTriangulation(triangulation, location, mesh, topoFace.Orientation());
		}
	}
}

void OCCMeshConverter::extractTriangulation(const Handle(Poly_Triangulation)& triangulation,
	const TopLoc_Location& location,
	TriangleMesh& mesh, TopAbs_Orientation orientation)
{
	if (triangulation.IsNull()) {
		return;
	}

	// Get transformation
	gp_Trsf transform = location.Transformation();

	// Extract vertices
	int vertexOffset = static_cast<int>(mesh.vertices.size());

	for (int i = 1; i <= triangulation->NbNodes(); i++) {
		gp_Pnt point = triangulation->Node(i);
		point.Transform(transform);
		mesh.vertices.push_back(point);
	}

	// Extract triangles with proper orientation handling
	const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
	for (int i = triangles.Lower(); i <= triangles.Upper(); i++) {
		int n1, n2, n3;
		triangles.Value(i).Get(n1, n2, n3);

		// Adjust indices to be 0-based and add vertex offset
		int idx1 = vertexOffset + n1 - 1;
		int idx2 = vertexOffset + n2 - 1;
		int idx3 = vertexOffset + n3 - 1;

		// Handle face orientation - reverse triangle winding if face is reversed
		if (orientation == TopAbs_REVERSED) {
			mesh.triangles.push_back(idx1);
			mesh.triangles.push_back(idx3);  // Swap n2 and n3 to reverse winding
			mesh.triangles.push_back(idx2);
		}
		else {
			mesh.triangles.push_back(idx1);
			mesh.triangles.push_back(idx2);
			mesh.triangles.push_back(idx3);
		}
	}
}

void OCCMeshConverter::subdivideTriangle(TriangleMesh& mesh, const gp_Pnt& p0, const gp_Pnt& p1, const gp_Pnt& p2, int levels)
{
	if (levels <= 0) {
		// Add original triangle
		mesh.vertices.push_back(p0);
		mesh.vertices.push_back(p1);
		mesh.vertices.push_back(p2);
		mesh.triangles.push_back(mesh.vertices.size() - 3);
		mesh.triangles.push_back(mesh.vertices.size() - 2);
		mesh.triangles.push_back(mesh.vertices.size() - 1);
		return;
	}

	// Calculate midpoints
	gp_Pnt mid01((p0.X() + p1.X()) / 2.0, (p0.Y() + p1.Y()) / 2.0, (p0.Z() + p1.Z()) / 2.0);
	gp_Pnt mid12((p1.X() + p2.X()) / 2.0, (p1.Y() + p2.Y()) / 2.0, (p1.Z() + p2.Z()) / 2.0);
	gp_Pnt mid20((p2.X() + p0.X()) / 2.0, (p2.Y() + p0.Y()) / 2.0, (p2.Z() + p0.Z()) / 2.0);

	// Recursively subdivide the four smaller triangles
	subdivideTriangle(mesh, p0, mid01, mid20, levels - 1);
	subdivideTriangle(mesh, mid01, p1, mid12, levels - 1);
	subdivideTriangle(mesh, mid20, mid12, p2, levels - 1);
	subdivideTriangle(mesh, mid01, mid12, mid20, levels - 1);
}

std::set<std::pair<int, int>> OCCMeshConverter::findBoundaryEdges(const TriangleMesh& mesh)
{
	std::set<std::pair<int, int>> boundaryEdges;
	std::map<std::pair<int, int>, int> edgeCounts;

	// Count edge occurrences
	for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
		int v0 = mesh.triangles[i];
		int v1 = mesh.triangles[i + 1];
		int v2 = mesh.triangles[i + 2];

		auto edge1 = std::make_pair(std::min(v0, v1), std::max(v0, v1));
		auto edge2 = std::make_pair(std::min(v1, v2), std::max(v1, v2));
		auto edge3 = std::make_pair(std::min(v2, v0), std::max(v2, v0));

		edgeCounts[edge1]++;
		edgeCounts[edge2]++;
		edgeCounts[edge3]++;
	}

	// Edges that appear only once are boundary edges
	for (const auto& edgeCount : edgeCounts) {
		if (edgeCount.second == 1) {
			boundaryEdges.insert(edgeCount.first);
		}
	}

	return boundaryEdges;
}
