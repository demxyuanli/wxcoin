#pragma once

#include <memory>
#include <vector>
#include <string>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>

/**
 * @brief Triangle mesh data structure
 */
struct TriangleMesh {
	std::vector<gp_Pnt> vertices;       // vertex coordinates
	std::vector<int> triangles;         // triangle indices (3 per triangle)
	std::vector<gp_Vec> normals;        // vertex normals

	// Statistics
	int getVertexCount() const { return static_cast<int>(vertices.size()); }
	int getTriangleCount() const { return static_cast<int>(triangles.size() / 3); }

	void clear() {
		vertices.clear();
		triangles.clear();
		normals.clear();
	}

	bool isEmpty() const {
		return vertices.empty() || triangles.empty();
	}
};

/**
 * @brief Meshing parameters
 */
struct MeshParameters {
	double deflection;          // mesh deflection
	double angularDeflection;   // angular deflection
	bool relative;              // relative deflection
	bool inParallel;            // parallel computation

	MeshParameters()
		: deflection(0.5)  // Increased from 0.1 for better performance
		, angularDeflection(1.0)  // Increased from 0.5 for coarser mesh
		, relative(false)
		, inParallel(true) {
	}
};

/**
 * @brief Geometry processing interface
 */
class GeometryProcessor {
public:
	virtual ~GeometryProcessor() = default;

	/**
	 * @brief Convert shape to triangle mesh
	 * @param shape Input shape
	 * @param params Meshing parameters
	 * @return Triangle mesh
	 */
	virtual TriangleMesh convertToMesh(const TopoDS_Shape& shape,
		const MeshParameters& params = MeshParameters()) = 0;

	/**
	 * @brief Calculate normals for mesh
	 * @param mesh Input/output mesh
	 */
	virtual void calculateNormals(TriangleMesh& mesh) = 0;

	/**
	 * @brief Smooth mesh normals
	 * @param mesh Input mesh
	 * @param creaseAngle Angle threshold in degrees
	 * @param iterations Number of smoothing iterations
	 * @return Smoothed mesh
	 */
	virtual TriangleMesh smoothNormals(const TriangleMesh& mesh,
		double creaseAngle = 30.0,
		int iterations = 2) = 0;

	/**
	 * @brief Create subdivision surface
	 * @param mesh Input mesh
	 * @param levels Subdivision levels
	 * @return Subdivided mesh
	 */
	virtual TriangleMesh createSubdivisionSurface(const TriangleMesh& mesh, int levels = 2) = 0;

	/**
	 * @brief Flip mesh normals
	 * @param mesh Input/output mesh
	 */
	virtual void flipNormals(TriangleMesh& mesh) = 0;

	/**
	 * @brief Get processor name
	 * @return Processor identifier
	 */
	virtual std::string getName() const = 0;
};