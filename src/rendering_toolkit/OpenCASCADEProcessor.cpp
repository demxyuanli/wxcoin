#include "rendering/OpenCASCADEProcessor.h"
#include "logger/Logger.h"
#include <BRepMesh_IncrementalMesh.hxx>
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
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

OpenCASCADEProcessor::OpenCASCADEProcessor()
	: m_showEdges(true)
	, m_featureEdgeAngle(45.0)
	, m_smoothingEnabled(true)
	, m_subdivisionEnabled(false)
	, m_subdivisionLevels(2)
	, m_creaseAngle(30.0) {
	LOG_INF_S("OpenCASCADEProcessor created");
}

OpenCASCADEProcessor::~OpenCASCADEProcessor() {
	LOG_INF_S("OpenCASCADEProcessor destroyed");
}

TriangleMesh OpenCASCADEProcessor::convertToMesh(const TopoDS_Shape& shape,
	const MeshParameters& params) {
	TriangleMesh mesh;

	if (shape.IsNull()) {
		LOG_WRN_S("Cannot convert null shape to mesh");
		return mesh;
	}

	try {
		// Create incremental mesh
		BRepMesh_IncrementalMesh meshGen(shape, params.deflection, params.relative,
			params.angularDeflection, params.inParallel);

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
		if (m_smoothingEnabled) {
			mesh = smoothNormals(mesh, m_creaseAngle, 2);
		}

		// Apply subdivision if enabled
		if (m_subdivisionEnabled) {
			mesh = createSubdivisionSurface(mesh, m_subdivisionLevels);
		}

		LOG_INF_S("Generated mesh with " + std::to_string(mesh.getVertexCount()) +
			" vertices and " + std::to_string(mesh.getTriangleCount()) + " triangles");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception in mesh conversion: " + std::string(e.what()));
		mesh.clear();
	}

	return mesh;
}

void OpenCASCADEProcessor::calculateNormals(TriangleMesh& mesh) {
	if (mesh.vertices.empty() || mesh.triangles.empty()) {
		LOG_WRN_S("Cannot calculate normals for empty mesh");
		return;
	}

	// Initialize normals vector with zero vectors
	mesh.normals.resize(mesh.vertices.size(), gp_Vec(0, 0, 0));

	// Calculate face normals and accumulate at vertices
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

			// Calculate face normal
			gp_Vec v1(p1, p2);
			gp_Vec v2(p1, p3);
			gp_Vec faceNormal = v1.Crossed(v2);

			// Normalize the face normal
			double length = faceNormal.Magnitude();
			if (length > Precision::Confusion()) {
				faceNormal.Scale(1.0 / length);
			}

			// Accumulate face normal at each vertex
			mesh.normals[idx1] += faceNormal;
			mesh.normals[idx2] += faceNormal;
			mesh.normals[idx3] += faceNormal;
		}
	}

	// Normalize vertex normals
	for (auto& normal : mesh.normals) {
		double length = normal.Magnitude();
		if (length > Precision::Confusion()) {
			normal.Scale(1.0 / length);
		}
		else {
			// If normal is zero, set to default up vector
			normal = gp_Vec(0, 0, 1);
		}
	}

	LOG_INF_S("Calculated normals for " + std::to_string(mesh.normals.size()) + " vertices");
}

TriangleMesh OpenCASCADEProcessor::smoothNormals(const TriangleMesh& mesh,
	double creaseAngle,
	int iterations) {
	if (mesh.vertices.empty() || mesh.triangles.empty() || mesh.normals.empty()) {
		LOG_WRN_S("Cannot smooth normals for empty mesh");
		return mesh;
	}

	TriangleMesh result = mesh;
	double creaseAngleRad = creaseAngle * M_PI / 180.0;
	double cosCreaseAngle = cos(creaseAngleRad);

	// Perform multiple iterations of normal smoothing
	for (int iter = 0; iter < iterations; ++iter) {
		std::vector<gp_Vec> newNormals(result.vertices.size(), gp_Vec(0, 0, 0));
		std::vector<int> normalCounts(result.vertices.size(), 0);

		// For each triangle, check if normals should be averaged
		for (size_t i = 0; i < result.triangles.size(); i += 3) {
			if (i + 2 >= result.triangles.size()) break;

			int idx1 = result.triangles[i];
			int idx2 = result.triangles[i + 1];
			int idx3 = result.triangles[i + 2];

			if (idx1 >= 0 && idx1 < static_cast<int>(result.vertices.size()) &&
				idx2 >= 0 && idx2 < static_cast<int>(result.vertices.size()) &&
				idx3 >= 0 && idx3 < static_cast<int>(result.vertices.size())) {
				const gp_Vec& n1 = result.normals[idx1];
				const gp_Vec& n2 = result.normals[idx2];
				const gp_Vec& n3 = result.normals[idx3];

				// Check if normals are similar enough to smooth
				double dot12 = n1.Dot(n2);
				double dot13 = n1.Dot(n3);
				double dot23 = n2.Dot(n3);

				// If all normals are similar, average them
				if (dot12 > cosCreaseAngle && dot13 > cosCreaseAngle && dot23 > cosCreaseAngle) {
					gp_Vec avgNormal = n1 + n2 + n3;
					double length = avgNormal.Magnitude();
					if (length > Precision::Confusion()) {
						avgNormal.Scale(1.0 / length);
					}

					newNormals[idx1] += avgNormal;
					newNormals[idx2] += avgNormal;
					newNormals[idx3] += avgNormal;
					normalCounts[idx1]++;
					normalCounts[idx2]++;
					normalCounts[idx3]++;
				}
				else {
					// Keep original normals for sharp edges
					newNormals[idx1] += n1;
					newNormals[idx2] += n2;
					newNormals[idx3] += n3;
					normalCounts[idx1]++;
					normalCounts[idx2]++;
					normalCounts[idx3]++;
				}
			}
		}

		// Average the accumulated normals
		for (size_t i = 0; i < result.normals.size(); ++i) {
			if (normalCounts[i] > 0) {
				gp_Vec& normal = newNormals[i];
				normal.Scale(1.0 / normalCounts[i]);
				double length = normal.Magnitude();
				if (length > Precision::Confusion()) {
					normal.Scale(1.0 / length);
				}
				result.normals[i] = normal;
			}
		}
	}

	LOG_INF_S("Smoothed normals with " + std::to_string(iterations) + " iterations, crease angle: " + std::to_string(creaseAngle));
	return result;
}

TriangleMesh OpenCASCADEProcessor::createSubdivisionSurface(const TriangleMesh& mesh, int levels) {
	// This is a placeholder implementation
	LOG_WRN_S("OpenCASCADEProcessor::createSubdivisionSurface not fully implemented yet");

	TriangleMesh result = mesh;
	return result;
}

void OpenCASCADEProcessor::flipNormals(TriangleMesh& mesh) {
	// This is a placeholder implementation
	LOG_WRN_S("OpenCASCADEProcessor::flipNormals not fully implemented yet");
}

void OpenCASCADEProcessor::setShowEdges(bool show) {
	m_showEdges = show;
}

void OpenCASCADEProcessor::setFeatureEdgeAngle(double angleDegrees) {
	m_featureEdgeAngle = angleDegrees;
}

void OpenCASCADEProcessor::setSmoothingEnabled(bool enabled) {
	m_smoothingEnabled = enabled;
}

void OpenCASCADEProcessor::setSubdivisionEnabled(bool enabled) {
	m_subdivisionEnabled = enabled;
}

void OpenCASCADEProcessor::setSubdivisionLevels(int levels) {
	m_subdivisionLevels = levels;
}

void OpenCASCADEProcessor::setCreaseAngle(double angle) {
	m_creaseAngle = angle;
}

void OpenCASCADEProcessor::meshFace(const TopoDS_Shape& face, TriangleMesh& mesh, const MeshParameters& params) {
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

void OpenCASCADEProcessor::extractTriangulation(const Handle(Poly_Triangulation)& triangulation,
	const TopLoc_Location& location,
	TriangleMesh& mesh, TopAbs_Orientation orientation) {
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

gp_Vec OpenCASCADEProcessor::calculateTriangleNormalVec(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3) {
	gp_Vec v1(p1, p2);
	gp_Vec v2(p1, p3);
	gp_Vec normal = v1.Crossed(v2);

	double length = normal.Magnitude();
	if (length > Precision::Confusion()) {
		normal = normal / length;
	}

	return normal;
}
