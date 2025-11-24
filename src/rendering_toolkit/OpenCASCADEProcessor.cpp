#include "rendering/OpenCASCADEProcessor.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
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
		// Get config reference for reading parameters
		auto& configRef = RenderingToolkitAPI::getConfig();

		// Read all tessellation parameters from config
		int tessellationQuality = 2;
		bool adaptiveMeshing = false;
		int tessellationMethod = 0;
		double featurePreservation = 0.5;
		bool parallelProcessingConfig = true;

		try {
			tessellationQuality = std::stoi(configRef.getParameter("tessellation_quality", "2"));
			adaptiveMeshing = (configRef.getParameter("adaptive_meshing", "false") == "true");
			tessellationMethod = std::stoi(configRef.getParameter("tessellation_method", "0"));
			featurePreservation = std::stod(configRef.getParameter("feature_preservation", "0.5"));
			parallelProcessingConfig = (configRef.getParameter("parallel_processing", "true") == "true");
		} catch (...) {
			// Use defaults if parsing fails
			tessellationQuality = 2;
			adaptiveMeshing = false;
			tessellationMethod = 0;
			featurePreservation = 0.5;
			parallelProcessingConfig = true;
		}

		// Use config setting if available, otherwise use parameter setting
		bool useParallel = parallelProcessingConfig && params.inParallel;

		// Log additional tessellation parameters for debugging
		LOG_DBG_S("Tessellation parameters: quality=" + std::to_string(tessellationQuality) +
			", adaptive=" + std::string(adaptiveMeshing ? "true" : "false") +
			", method=" + std::to_string(tessellationMethod) +
			", featurePreservation=" + std::to_string(featurePreservation) +
			", parallel=" + std::string(useParallel ? "true" : "false"));

		// Adjust basic parameters based on advanced settings
		double adjustedDeflection = params.deflection;
		double adjustedAngularDeflection = params.angularDeflection;
		
		// Only adjust parameters if user has explicitly set high quality settings
		// Default tessellationQuality=2 should not trigger aggressive parameter adjustment
		if (tessellationQuality >= 3) {
			// Only apply aggressive quality adjustments for very high quality settings
			// Quality 3: 0.25x deflection (very detailed)
			// Quality 4+: 0.1x deflection (extremely detailed)
			double qualityFactor = 1.0 / (1.0 + (tessellationQuality - 2));
			adjustedDeflection *= qualityFactor;
			adjustedAngularDeflection *= qualityFactor;
			LOG_DBG_S("Applied high quality tessellation adjustment: factor=" + std::to_string(qualityFactor));
		}
		
		// Only apply adaptive meshing adjustment if explicitly enabled AND quality is high
		if (adaptiveMeshing && tessellationQuality >= 3) {
			// Adaptive meshing uses even smaller deflection for better quality
			adjustedDeflection *= 0.7; // Less aggressive than 0.5
			adjustedAngularDeflection *= 0.7;
			LOG_DBG_S("Applied adaptive meshing adjustment");
		}
		
		LOG_DBG_S("Adjusted mesh parameters: deflection=" + std::to_string(adjustedDeflection) +
			", angularDeflection=" + std::to_string(adjustedAngularDeflection) +
			" (original: " + std::to_string(params.deflection) + ", " + std::to_string(params.angularDeflection) + ")");

		// Create incremental mesh with adjusted parameters
		// Use IMeshTools_Parameters for better control over meshing
		IMeshTools_Parameters meshParams;
		meshParams.Deflection = adjustedDeflection;
		meshParams.Angle = adjustedAngularDeflection;
		meshParams.Relative = params.relative;
		meshParams.InParallel = useParallel;
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

		// Only apply smoothing/subdivision if mesh is not empty
		if (!mesh.vertices.empty() && !mesh.triangles.empty()) {
			// Apply smoothing if enabled - read from RenderingToolkitAPI config
			auto& config = configRef; // Reuse the config reference from earlier
			auto& smoothingSettings = config.getSmoothingSettings();
			auto& subdivisionSettings = config.getSubdivisionSettings();

			// Read custom parameters
			double smoothingStrength = 0.5;
			try {
				smoothingStrength = std::stod(config.getParameter("smoothing_strength", "0.5"));
			} catch (...) {
				smoothingStrength = 0.5;
			}

			if (smoothingSettings.enabled) {
				// Use smoothing strength to modify iterations based on strength
				int adjustedIterations = smoothingSettings.iterations;
				if (smoothingStrength > 0.7) {
					adjustedIterations = std::max(adjustedIterations + 1, 1);
				} else if (smoothingStrength < 0.3) {
					adjustedIterations = std::max(adjustedIterations - 1, 1);
				}

				mesh = smoothNormals(mesh, smoothingSettings.creaseAngle, adjustedIterations);
				LOG_DBG_S("Applied mesh smoothing: creaseAngle=" + std::to_string(smoothingSettings.creaseAngle) +
					", iterations=" + std::to_string(adjustedIterations) +
					", strength=" + std::to_string(smoothingStrength));
			}

			// Apply subdivision if enabled - read from RenderingToolkitAPI config
			if (subdivisionSettings.enabled) {
				mesh = createSubdivisionSurface(mesh, subdivisionSettings.levels);
				LOG_DBG_S("Applied mesh subdivision: levels=" + std::to_string(subdivisionSettings.levels));
			}
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

// Convert to mesh with face index mapping
TriangleMesh OpenCASCADEProcessor::convertToMeshWithFaceMapping(const TopoDS_Shape& shape,
	const MeshParameters& params, std::vector<std::pair<int, std::vector<int>>>& faceMappings) {

	TriangleMesh mesh;

	if (shape.IsNull()) {
		LOG_WRN_S("Cannot convert null shape to mesh");
		return mesh;
	}

	try {
		// Get config reference for reading parameters
		auto& configRef = RenderingToolkitAPI::getConfig();

		// Read all tessellation parameters from config
		int tessellationQuality = 2;
		bool adaptiveMeshing = false;
		int tessellationMethod = 0;
		double featurePreservation = 0.5;
		bool parallelProcessingConfig = true;

		try {
			tessellationQuality = std::stoi(configRef.getParameter("tessellation_quality", "2"));
			adaptiveMeshing = (configRef.getParameter("adaptive_meshing", "false") == "true");
			tessellationMethod = std::stoi(configRef.getParameter("tessellation_method", "0"));
			featurePreservation = std::stod(configRef.getParameter("feature_preservation", "0.5"));
			parallelProcessingConfig = (configRef.getParameter("parallel_processing", "true") == "true");
		} catch (...) {
			// Use defaults if parsing fails
			tessellationQuality = 2;
			adaptiveMeshing = false;
			tessellationMethod = 0;
			featurePreservation = 0.5;
			parallelProcessingConfig = true;
		}

		// Use config setting if available, otherwise use parameter setting
		bool useParallel = parallelProcessingConfig && params.inParallel;

		// Log additional tessellation parameters for debugging
		LOG_DBG_S("Tessellation parameters: quality=" + std::to_string(tessellationQuality) +
			", adaptive=" + std::string(adaptiveMeshing ? "true" : "false") +
			", method=" + std::to_string(tessellationMethod) +
			", featurePreservation=" + std::to_string(featurePreservation) +
			", parallel=" + std::string(useParallel ? "true" : "false"));

		// Adjust basic parameters based on advanced settings
		double adjustedDeflection = params.deflection;
		double adjustedAngularDeflection = params.angularDeflection;

		// Only adjust parameters if user has explicitly set high quality settings
		// Default tessellationQuality=2 should not trigger aggressive parameter adjustment
		if (tessellationQuality >= 3) {
			// Only apply aggressive quality adjustments for very high quality settings
			// Quality 3: 0.25x deflection (very detailed)
			// Quality 4+: 0.1x deflection (extremely detailed)
			double qualityFactor = 1.0 / (1.0 + (tessellationQuality - 2));
			adjustedDeflection *= qualityFactor;
			adjustedAngularDeflection *= qualityFactor;
			LOG_DBG_S("Applied high quality tessellation adjustment: factor=" + std::to_string(qualityFactor));
		}

		// Only apply adaptive meshing adjustment if explicitly enabled AND quality is high
		if (adaptiveMeshing && tessellationQuality >= 3) {
			// Adaptive meshing uses even smaller deflection for better quality
			adjustedDeflection *= 0.7; // Less aggressive than 0.5
		}

		// Generate mesh with BRepMesh using IMeshTools_Parameters for better control
		IMeshTools_Parameters meshParams;
		meshParams.Deflection = adjustedDeflection;
		meshParams.Angle = adjustedAngularDeflection;
		meshParams.Relative = params.relative;
		meshParams.InParallel = useParallel;
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

		// Extract triangles from all faces with face index tracking
		// Use recursive traversal to ensure all faces are found, including nested ones
		std::vector<TopoDS_Face> allFaces;
		extractAllFacesRecursive(shape, allFaces);

		LOG_INF_S("OpenCASCADEProcessor::convertToMeshWithFaceMapping - Extracted " + 
		          std::to_string(allFaces.size()) + " faces from shape");

		faceMappings.clear();
		faceMappings.reserve(allFaces.size());

		int currentTriangleIndex = 0;
		int facesWithTriangles = 0;

		for (size_t faceIndex = 0; faceIndex < allFaces.size(); ++faceIndex) {
			const TopoDS_Face& face = allFaces[faceIndex];
			std::vector<int> faceTriangleIndices;

			// Mesh the face and track triangle indices
			meshFaceWithIndexTracking(face, mesh, params, faceTriangleIndices, currentTriangleIndex);

			// Store face-triangle mapping
			faceMappings.emplace_back(static_cast<int>(faceIndex), faceTriangleIndices);

			if (!faceTriangleIndices.empty()) {
				facesWithTriangles++;
			}
		}

		LOG_INF_S("OpenCASCADEProcessor::convertToMeshWithFaceMapping - Built mappings for " +
		          std::to_string(facesWithTriangles) + " faces with triangles out of " +
		          std::to_string(allFaces.size()) + " total faces");

		// Calculate normals if not already done
		if (mesh.normals.empty() && !mesh.vertices.empty()) {
			calculateNormals(mesh);
		}

		// Only apply smoothing/subdivision if mesh is not empty
		if (!mesh.vertices.empty() && !mesh.triangles.empty()) {
			// Apply smoothing if enabled - read from RenderingToolkitAPI config
			auto& smoothingSettings = configRef.getSmoothingSettings();
			auto& subdivisionSettings = configRef.getSubdivisionSettings();

			// Read custom parameters
			double smoothingStrength = 0.5;
			try {
				smoothingStrength = std::stod(configRef.getParameter("smoothing_strength", "0.5"));
			} catch (...) {
				smoothingStrength = 0.5;
			}

			if (smoothingSettings.enabled) {
				// Use smoothing strength to modify iterations based on strength
				int adjustedIterations = smoothingSettings.iterations;
				if (smoothingStrength > 0.7) {
					adjustedIterations = std::max(adjustedIterations + 1, 1);
				} else if (smoothingStrength < 0.3) {
					adjustedIterations = std::max(adjustedIterations - 1, 1);
				}

				mesh = smoothNormals(mesh, smoothingSettings.creaseAngle, adjustedIterations);
				LOG_DBG_S("Applied mesh smoothing: creaseAngle=" + std::to_string(smoothingSettings.creaseAngle) +
					", iterations=" + std::to_string(adjustedIterations) +
					", strength=" + std::to_string(smoothingStrength));
			}

			// Apply subdivision if enabled - read from RenderingToolkitAPI config
			if (subdivisionSettings.enabled) {
				mesh = createSubdivisionSurface(mesh, subdivisionSettings.levels);
				LOG_DBG_S("Applied mesh subdivision: levels=" + std::to_string(subdivisionSettings.levels));
			}
		}

	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception in mesh conversion with face mapping: " + std::string(e.what()));
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

	LOG_DBG_S("Calculated normals for " + std::to_string(mesh.normals.size()) + " vertices");
}

TriangleMesh OpenCASCADEProcessor::smoothNormals(const TriangleMesh& mesh,
	double creaseAngle,
	int iterations) {
	if (mesh.vertices.empty() || mesh.triangles.empty() || mesh.normals.empty()) {
		// Silent return - this is expected when called with empty mesh (caller should check first)
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

	LOG_DBG_S("Smoothed normals with " + std::to_string(iterations) + " iterations, crease angle: " + std::to_string(creaseAngle));
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

// Mesh face with triangle index tracking for face mapping
void OpenCASCADEProcessor::meshFaceWithIndexTracking(const TopoDS_Face& face, TriangleMesh& mesh,
	const MeshParameters& params, std::vector<int>& triangleIndices, int& currentTriangleIndex) {

	TopLoc_Location location;
	Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

	// If triangulation exists, extract it
	if (!triangulation.IsNull()) {
		int startTriangleIndex = currentTriangleIndex;
		extractTriangulationWithIndexTracking(triangulation, location, mesh, face.Orientation(), triangleIndices);
		currentTriangleIndex += triangulation->NbTriangles();
	}
	else {
		// If no triangulation exists, create one
		try {
			BRepMesh_IncrementalMesh mesher(face, params.deflection, params.relative, params.angularDeflection,
				params.inParallel);
			triangulation = BRep_Tool::Triangulation(face, location);
			if (!triangulation.IsNull()) {
				int startTriangleIndex = currentTriangleIndex;
				extractTriangulationWithIndexTracking(triangulation, location, mesh, face.Orientation(), triangleIndices);
				currentTriangleIndex += triangulation->NbTriangles();
			} else {
				LOG_WRN_S("meshFaceWithIndexTracking - Failed to create triangulation for face");
			}
		} catch (const std::exception& e) {
			LOG_WRN_S("meshFaceWithIndexTracking - Exception creating triangulation: " + std::string(e.what()));
		} catch (...) {
			LOG_WRN_S("meshFaceWithIndexTracking - Unknown exception creating triangulation");
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

// Extract triangulation with triangle index tracking
void OpenCASCADEProcessor::extractTriangulationWithIndexTracking(const Handle(Poly_Triangulation)& triangulation,
	const TopLoc_Location& location, TriangleMesh& mesh, TopAbs_Orientation orientation, std::vector<int>& triangleIndices) {

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

	// Extract triangles with proper orientation handling and index tracking
	const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
	int startTriangleIndex = static_cast<int>(mesh.triangles.size()) / 3;

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

		// Track triangle index for this face
		int triangleIndex = startTriangleIndex + (i - triangles.Lower());
		triangleIndices.push_back(triangleIndex);
	}
}

// Recursive face extraction to handle nested compounds, solids, and shells
// TopExp_Explorer recursively traverses sub-shapes, but for nested compounds we need explicit recursion
void OpenCASCADEProcessor::extractAllFacesRecursive(const TopoDS_Shape& shape, std::vector<TopoDS_Face>& faces) {
	if (shape.IsNull()) {
		LOG_WRN_S("extractAllFacesRecursive - Shape is null");
		return;
	}

	// Log shape type for debugging
	const char* shapeTypeName = "UNKNOWN";
	switch (shape.ShapeType()) {
		case TopAbs_COMPOUND: shapeTypeName = "COMPOUND"; break;
		case TopAbs_COMPSOLID: shapeTypeName = "COMPSOLID"; break;
		case TopAbs_SOLID: shapeTypeName = "SOLID"; break;
		case TopAbs_SHELL: shapeTypeName = "SHELL"; break;
		case TopAbs_FACE: shapeTypeName = "FACE"; break;
		case TopAbs_WIRE: shapeTypeName = "WIRE"; break;
		case TopAbs_EDGE: shapeTypeName = "EDGE"; break;
		case TopAbs_VERTEX: shapeTypeName = "VERTEX"; break;
		case TopAbs_SHAPE: shapeTypeName = "SHAPE"; break;
	}
	LOG_INF_S("extractAllFacesRecursive - Shape type: " + std::string(shapeTypeName));

	// Use IsSame() to track already added faces to avoid duplicates
	// IsSame() compares both TShape and Location, which is more accurate than just TShape pointer
	// This ensures each topological face gets its own unique index, even if they share the same geometry
	auto addFaceIfNew = [&](const TopoDS_Face& face) {
		if (!face.IsNull()) {
			// Check if this exact face (same TShape and Location) is already added
			bool alreadyAdded = false;
			for (const auto& existingFace : faces) {
				if (face.IsSame(existingFace)) {
					alreadyAdded = true;
					break;
				}
			}
			if (!alreadyAdded) {
				faces.push_back(face);
			}
		}
	};

	// Extract all faces from the shape
	// TopExp_Explorer automatically handles recursion for COMPOUND, SOLID, SHELL, etc.
	// For COMPOUND, we can directly traverse FACE and it will recursively find all faces
	int faceCountBefore = static_cast<int>(faces.size());
	for (TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
		addFaceIfNew(TopoDS::Face(faceExp.Current()));
	}
	int faceCountAfter = static_cast<int>(faces.size());
	
	if (faceCountAfter > faceCountBefore) {
		LOG_INF_S("extractAllFacesRecursive - Extracted " + std::to_string(faceCountAfter - faceCountBefore) +
		          " faces from " + std::string(shapeTypeName));
	} else if (shape.ShapeType() == TopAbs_COMPOUND) {
		// For COMPOUND, if no faces found with direct traversal, try alternative approach
		LOG_WRN_S("extractAllFacesRecursive - No faces found with direct traversal, trying alternative approach");
		
		// Try traversing sub-shapes explicitly
		int subShapeCount = 0;
		for (TopExp_Explorer exp(shape, TopAbs_SHAPE); exp.More(); exp.Next()) {
			subShapeCount++;
			const TopoDS_Shape& subShape = exp.Current();
			
			const char* subShapeTypeName = "UNKNOWN";
			switch (subShape.ShapeType()) {
				case TopAbs_COMPOUND: subShapeTypeName = "COMPOUND"; break;
				case TopAbs_COMPSOLID: subShapeTypeName = "COMPSOLID"; break;
				case TopAbs_SOLID: subShapeTypeName = "SOLID"; break;
				case TopAbs_SHELL: subShapeTypeName = "SHELL"; break;
				case TopAbs_FACE: subShapeTypeName = "FACE"; break;
				case TopAbs_WIRE: subShapeTypeName = "WIRE"; break;
				case TopAbs_EDGE: subShapeTypeName = "EDGE"; break;
				case TopAbs_VERTEX: subShapeTypeName = "VERTEX"; break;
				case TopAbs_SHAPE: subShapeTypeName = "SHAPE"; break;
			}
			
			int facesBeforeAlt = static_cast<int>(faces.size());
			
			if (subShape.ShapeType() == TopAbs_COMPOUND) {
				// Recursively process nested compounds
				extractAllFacesRecursive(subShape, faces);
			} else if (subShape.ShapeType() == TopAbs_FACE) {
				// Add face directly
				addFaceIfNew(TopoDS::Face(subShape));
			} else if (subShape.ShapeType() == TopAbs_SOLID) {
				// Extract faces from solid
				for (TopExp_Explorer faceExp(subShape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
					addFaceIfNew(TopoDS::Face(faceExp.Current()));
				}
			} else if (subShape.ShapeType() == TopAbs_SHELL) {
				// Extract faces from shell
				for (TopExp_Explorer faceExp(subShape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
					addFaceIfNew(TopoDS::Face(faceExp.Current()));
				}
			}
			
			int facesAfterAlt = static_cast<int>(faces.size());
			if (facesAfterAlt > facesBeforeAlt) {
				LOG_INF_S("extractAllFacesRecursive - Sub-shape " + std::to_string(subShapeCount) + 
				          " (type: " + std::string(subShapeTypeName) + ") added " + 
				          std::to_string(facesAfterAlt - facesBeforeAlt) + " faces");
			}
		}
		LOG_INF_S("extractAllFacesRecursive - Alternative approach processed " + std::to_string(subShapeCount) + " sub-shapes");
	}
	
	if (shape.ShapeType() != TopAbs_COMPOUND) {
		// For non-compound shapes, TopExp_Explorer handles recursion automatically
		int faceCountBefore = static_cast<int>(faces.size());
		for (TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
			addFaceIfNew(TopoDS::Face(faceExp.Current()));
		}
		int faceCountAfter = static_cast<int>(faces.size());
		LOG_INF_S("extractAllFacesRecursive - Extracted " + std::to_string(faceCountAfter - faceCountBefore) +
		          " faces from " + std::string(shapeTypeName));
	}
	
	LOG_INF_S("extractAllFacesRecursive - Total faces extracted: " + std::to_string(faces.size()));
}
