#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <future>
#include <functional>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Dir.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include "OCCGeometry.h"
#include "GeometryReader.h"

// Forward declarations
class STEPControl_Reader;
template<typename T> class Handle;

/**
 * @brief STEP file reader for importing CAD models
 *
 * Provides functionality to read STEP files and convert them to OCCGeometry objects
 * with optimized performance through parallel processing and caching
 */
class STEPReader : public GeometryReader {
public:
	using ProgressCallback = std::function<void(int /*percent*/, const std::string& /*stage*/)>;
	
	// GeometryReader interface implementation
	ReadResult readFile(const std::string& filePath,
		const OptimizationOptions& options = OptimizationOptions(),
		ProgressCallback progress = nullptr) override;
	
	bool isValidFile(const std::string& filePath) const override;
	std::vector<std::string> getSupportedExtensions() const override;
	std::string getFormatName() const override;
	std::string getFileFilter() const override;
	
	/**
	 * @brief STEP entity metadata structure
	 */
	struct STEPEntityInfo {
		std::string name;
		std::string type;
		Quantity_Color color;
		bool hasColor;
		std::string material;
		std::string description;
		int entityId;
		int shapeIndex; // Index in the transferred shapes
		
		STEPEntityInfo() : hasColor(false), entityId(0), shapeIndex(-1) {}
	};

	/**
	 * @brief STEP assembly structure
	 */
	struct STEPAssemblyInfo {
		std::string name;
		std::string type;
		std::vector<STEPEntityInfo> components;
		std::vector<STEPAssemblyInfo> subAssemblies;
		TopoDS_Shape shape;
		
		STEPAssemblyInfo() {}
	};

	/**
	 * @brief Result structure for STEP file reading
	 */
	struct ReadResult {
		bool success;
		std::string errorMessage;
		std::vector<std::shared_ptr<OCCGeometry>> geometries;
		TopoDS_Shape rootShape;
		double importTime; // Time taken for import in milliseconds
		std::vector<STEPEntityInfo> entityMetadata;
		STEPAssemblyInfo assemblyStructure;

		ReadResult() : success(false), importTime(0.0) {}
	};

	/**
	 * @brief Read a STEP file and return geometry objects with optimization
	 * @param filePath Path to the STEP file
	 * @param options Optimization options
	 * @return ReadResult containing success status and geometry objects
	 */
	static ReadResult readSTEPFile(const std::string& filePath,
		const OptimizationOptions& options = OptimizationOptions(),
		ProgressCallback progress = nullptr);

	/**
	 * @brief Read a STEP file using STEPCAFControl_Reader for color and assembly support
	 * @param filePath Path to the STEP file
	 * @param options Optimization options
	 * @param progress Progress callback
	 * @return ReadResult containing success status and geometry objects with colors
	 */
	static ReadResult readSTEPFileWithCAF(const std::string& filePath,
		const OptimizationOptions& options = OptimizationOptions(),
		ProgressCallback progress = nullptr);

	/**
	 * @brief Read a STEP file and return a single compound shape
	 * @param filePath Path to the STEP file
	 * @return TopoDS_Shape containing all geometry from the file
	 */
	static TopoDS_Shape readSTEPShape(const std::string& filePath);

	/**
	 * @brief Check if a file has a valid STEP extension
	 * @param filePath Path to check
	 * @return true if file has .step or .stp extension
	 */
	static bool isSTEPFile(const std::string& filePath);

	/**
	 * @brief Convert a TopoDS_Shape to OCCGeometry objects with optimization
	 * @param shape The shape to convert
	 * @param baseName Base name for the geometry objects
	 * @param options Optimization options
	 * @return Vector of OCCGeometry objects
	 */
	static std::vector<std::shared_ptr<OCCGeometry>> shapeToGeometries(
		const TopoDS_Shape& shape,
		const std::string& baseName = "ImportedGeometry",
		const OptimizationOptions& options = OptimizationOptions(),
		ProgressCallback progress = nullptr,
		int progressStart = 50,
		int progressSpan = 40
	);

	/**
	 * @brief Scale imported geometry to reasonable size
	 * @param geometries Vector of geometry objects to scale
	 * @param targetSize Target maximum dimension (0 = auto-detect)
	 * @return Scaling factor applied
	 */
	static double scaleGeometriesToReasonableSize(
		std::vector<std::shared_ptr<OCCGeometry>>& geometries,
		double targetSize = 0.0
	);

	/**
	 * @brief Calculate combined bounding box of multiple geometries
	 * @param geometries Vector of geometries
	 * @param minPt Output minimum point
	 * @param maxPt Output maximum point
	 * @return true if valid bounds found
	 */
	static bool calculateCombinedBoundingBox(
		const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
		gp_Pnt& minPt,
		gp_Pnt& maxPt
	);

public:
	STEPReader() = default;
	
private:

	/**
	 * @brief Initialize the STEP reader (if needed)
	 */
	static void initialize();

	/**
	 * @brief Extract individual shapes from a compound
	 * @param compound The compound shape
	 * @param shapes Output vector of shapes
	 */
	static void extractShapes(const TopoDS_Shape& compound, std::vector<TopoDS_Shape>& shapes);

	/**
	 * @brief Process shapes in parallel
	 * @param shapes Vector of shapes to process
	 * @param baseName Base name for geometry objects
	 * @param options Optimization options
	 * @return Vector of processed geometry objects
	 */
	static std::vector<std::shared_ptr<OCCGeometry>> processShapesParallel(
		const std::vector<TopoDS_Shape>& shapes,
		const std::string& baseName,
		const OptimizationOptions& options,
		ProgressCallback progress = nullptr,
		int progressStart = 50,
		int progressSpan = 40
	);

	/**
	 * @brief Process a single shape (for parallel processing)
	 * @param shape The shape to process
	 * @param name Name for the geometry object
	 * @param options Optimization options
	 * @return Processed geometry object
	 */
	static std::shared_ptr<OCCGeometry> processSingleShape(
		const TopoDS_Shape& shape,
		const std::string& name,
		const std::string& baseName,
		const OptimizationOptions& options
	);

	/**
	 * @brief Process a single shape with custom color palette and index
	 * @param shape The shape to process
	 * @param name Name for the geometry object
	 * @param baseName Base filename
	 * @param options Optimization options
	 * @param palette Color palette to use
	 * @param hasher Hash function for consistent coloring
	 * @param colorIndex Index in the color palette
	 * @return Processed geometry object
	 */
	static std::shared_ptr<OCCGeometry> processSingleShape(
		const TopoDS_Shape& shape,
		const std::string& name,
		const std::string& baseName,
		const OptimizationOptions& options,
		const std::vector<Quantity_Color>& palette,
		const std::hash<std::string>& hasher,
		size_t colorIndex
	);

	/**
	 * @brief Read STEP file metadata (colors, materials, names)
	 * @param reader The STEP reader instance
	 * @return Vector of entity metadata
	 */
	static std::vector<STEPEntityInfo> readSTEPMetadata(
		const STEPControl_Reader& reader
	);

	/**
	 * @brief Build assembly structure from STEP file
	 * @param reader The STEP reader instance
	 * @return Assembly structure information
	 */
	static STEPAssemblyInfo buildAssemblyStructure(
		const STEPControl_Reader& reader
	);

	/**
	 * @brief Extract entity information from STEP model
	 * @param reader The STEP reader instance
	 * @param entityId Entity ID in STEP file
	 * @return Entity information
	 */
	static STEPEntityInfo extractEntityInfo(
		const STEPControl_Reader& reader,
		int entityId
	);

	/**
	 * @brief Validate input file and check basic requirements
	 * @param filePath Path to the STEP file
	 * @param result ReadResult to store error messages
	 * @return true if file is valid, false otherwise
	 */
	static bool validateFile(const std::string& filePath, ReadResult& result);

	/**
	 * @brief Read and parse STEP file using standard OCCT reader
	 * @param filePath Path to the STEP file
	 * @param options Optimization options
	 * @param result ReadResult to store results
	 * @param progress Progress callback
	 * @return true if successful, false otherwise
	 */
	static bool readSTEPFileCore(const std::string& filePath, const OptimizationOptions& options,
		ReadResult& result, ProgressCallback progress);

	/**
	 * @brief Assemble shapes from STEP reader results
	 * @param reader The STEP reader instance
	 * @param result ReadResult to store assembled shape
	 * @param progress Progress callback
	 * @return true if successful, false otherwise
	 */
	static bool assembleShapes(const STEPControl_Reader& reader, ReadResult& result, ProgressCallback progress);

	/**
	 * @brief Extract metadata from STEP file
	 * @param reader The STEP reader instance
	 * @param result ReadResult to store metadata
	 * @param progress Progress callback
	 */
	static void extractMetadata(const STEPControl_Reader& reader, ReadResult& result, ProgressCallback progress);

	/**
	 * @brief Try to read file with CAF reader for enhanced color/material support
	 * @param filePath Path to the STEP file
	 * @param options Optimization options
	 * @param result ReadResult to store results
	 * @param progress Progress callback
	 * @return true if CAF reader succeeded and provided useful data, false otherwise
	 */
	static bool tryCAFReader(const std::string& filePath, const OptimizationOptions& options,
		ReadResult& result, ProgressCallback progress);

	/**
	 * @brief Convert shape to geometries using decomposition and coloring
	 * @param shape The root shape
	 * @param baseName Base name for geometries
	 * @param options Optimization options
	 * @param result ReadResult to store geometries
	 * @param progress Progress callback
	 * @param progressStart Starting progress percentage
	 * @param progressSpan Progress span for this operation
	 */
	static void convertToGeometries(const TopoDS_Shape& shape, const std::string& baseName,
		const OptimizationOptions& options, ReadResult& result,
		ProgressCallback progress, int progressStart, int progressSpan);

	/**
	 * @brief Apply post-processing to geometries
	 * @param result ReadResult containing geometries to post-process
	 * @param progress Progress callback
	 */
	static void postProcessGeometries(ReadResult& result, ProgressCallback progress);

	/**
	 * @brief Decompose shape according to specified options
	 * @param shape Input shape
	 * @param options Decomposition options
	 * @return Vector of decomposed shapes
	 */
	static std::vector<TopoDS_Shape> decomposeShape(const TopoDS_Shape& shape, const OptimizationOptions& options);

	/**
	 * @brief Apply decomposition heuristics when basic decomposition yields single component
	 * @param shapes Current shapes vector
	 * @param options Decomposition options
	 * @return Updated shapes vector with applied heuristics
	 */
	static std::vector<TopoDS_Shape> applyDecompositionHeuristics(std::vector<TopoDS_Shape> shapes, const OptimizationOptions& options);

	/**
	 * @brief Create geometries from shapes with coloring
	 * @param shapes Vector of shapes to convert
	 * @param baseName Base name for geometries
	 * @param options Optimization options
	 * @param palette Color palette
	 * @param hasher Hash function for consistent coloring
	 * @return Vector of created geometries
	 */
	static std::vector<std::shared_ptr<OCCGeometry>> createGeometriesFromShapes(
		const std::vector<TopoDS_Shape>& shapes,
		const std::string& baseName,
		const OptimizationOptions& options,
		const std::vector<Quantity_Color>& palette,
		const std::hash<std::string>& hasher);

	/**
	 * @brief Generate distinct colors for assembly components
	 * @param componentCount Number of components
	 * @return Vector of distinct colors
	 */
	static std::vector<Quantity_Color> generateDistinctColors(int componentCount);

	/**
	 * @brief Apply colors to geometries based on entity metadata
	 * @param geometries Vector of geometries to color
	 * @param entityMetadata Vector of entity metadata
	 * @param assemblyInfo Assembly structure information
	 */
	static void applyColorsToGeometries(
		std::vector<std::shared_ptr<OCCGeometry>>& geometries,
		const std::vector<STEPEntityInfo>& entityMetadata,
		const STEPAssemblyInfo& assemblyInfo
	);

	/**
	 * @brief Extract color information from STEP entity
	 * @param entity The STEP entity
	 * @param info Entity info to update with color
	 */
	static void extractColorFromEntity(
		const Handle(Standard_Transient)& entity,
		STEPEntityInfo& info
	);

	/**
	 * @brief Apply fine tessellation to geometries for smooth surfaces
	 * @param geometries Vector of geometries to tessellate
	 * @param options Tessellation options
	 */
	static void applyFineTessellation(
		std::vector<std::shared_ptr<OCCGeometry>>& geometries,
		const OptimizationOptions& options
	);

	/**
	 * @brief Detect if a shape is a shell model (surface model without volume)
	 * @param shape The shape to analyze
	 * @return true if the shape is a shell model
	 */
	static bool detectShellModel(const TopoDS_Shape& shape);

public:
	/**
	 * @brief Face feature structure for intelligent decomposition
	 */
	struct FaceFeature {
		TopoDS_Face face;
		int id;
		std::string type; // "PLANE", "CYLINDER", "SPHERE", "CONE", "TORUS", "SURFACE"
		double area;
		gp_Pnt centroid;
		gp_Dir normal;
		std::vector<int> adjacentFaces;
	};

	/**
	 * @brief Feature-based intelligent decomposition (FreeCAD-style)
	 * @param shape Input shape to decompose
	 * @param components Output vector of decomposed components
	 */
	static void decomposeByFeatureRecognition(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& components);

	/**
	 * @brief Adjacent faces clustering decomposition
	 * @param shape Input shape to decompose
	 * @param components Output vector of decomposed components
	 */
	static void decomposeByAdjacentFacesClustering(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& components);

	/**
	 * @brief Comparator for TopoDS_Edge to use in sets
	 */
	struct EdgeComparator {
		bool operator()(const TopoDS_Edge& a, const TopoDS_Edge& b) const {
			// Use TShape identity comparison
			return a.TShape().get() < b.TShape().get();
		}
	};

private:
	/**
	 * @brief Classify face type based on geometry
	 * @param face Input face
	 * @return Face type string
	 */
	static std::string classifyFaceType(const TopoDS_Face& face);

	/**
	 * @brief Calculate face area
	 * @param face Input face
	 * @return Face area
	 */
	static double calculateFaceArea(const TopoDS_Face& face);

	/**
	 * @brief Calculate face centroid
	 * @param face Input face
	 * @return Face centroid point
	 */
	static gp_Pnt calculateFaceCentroid(const TopoDS_Face& face);

	/**
	 * @brief Calculate face normal
	 * @param face Input face
	 * @return Face normal direction
	 */
	static gp_Dir calculateFaceNormal(const TopoDS_Face& face);

	/**
	 * @brief Cluster faces by similar features
	 * @param faceFeatures Input face features
	 * @param featureGroups Output feature groups
	 */
	static void clusterFacesByFeatures(const std::vector<FaceFeature>& faceFeatures, std::vector<std::vector<int>>& featureGroups);

	/**
	 * @brief Build face adjacency graph
	 * @param faces Input faces
	 * @param adjacencyGraph Output adjacency graph
	 */
	static void buildFaceAdjacencyGraph(const std::vector<TopoDS_Face>& faces, std::vector<std::vector<int>>& adjacencyGraph);

	/**
	 * @brief Cluster adjacent faces based on geometric similarity
	 * @param faces Input faces
	 * @param adjacencyGraph Face adjacency graph
	 * @param clusters Output face clusters
	 */
	static void clusterAdjacentFaces(const std::vector<TopoDS_Face>& faces, const std::vector<std::vector<int>>& adjacencyGraph, std::vector<std::vector<int>>& clusters);

	/**
	 * @brief Try to create solid from face group
	 * @param compound Input compound of faces
	 * @param faceFeatures Face features
	 * @param group Face group indices
	 * @return Created solid or null shape
	 */
	static TopoDS_Shape tryCreateSolidFromFaces(const TopoDS_Compound& compound, const std::vector<FaceFeature>& faceFeatures, const std::vector<int>& group);

	/**
	 * @brief Try to create solid from face cluster
	 * @param compound Input compound of faces
	 * @param faces Input faces
	 * @param cluster Face cluster indices
	 * @return Created solid or null shape
	 */
	static TopoDS_Shape tryCreateSolidFromFaceCluster(const TopoDS_Compound& compound, const std::vector<TopoDS_Face>& faces, const std::vector<int>& cluster);

	/**
	 * @brief Check if two faces are adjacent (share edges)
	 * @param face1 First face
	 * @param face2 Second face
	 * @return true if faces are adjacent
	 */
	static bool areFacesAdjacent(const TopoDS_Face& face1, const TopoDS_Face& face2);

private:
	/**
	 * @brief Parallel face feature extraction
	 * @param faces Input faces
	 * @param faceBounds Face bounding boxes
	 * @return Vector of face features
	 */
	static std::vector<FaceFeature> extractFaceFeaturesParallel(
		const std::vector<TopoDS_Face>& faces,
		const std::vector<Bnd_Box>& faceBounds);

	/**
	 * @brief Optimized face clustering with spatial partitioning
	 * @param faceFeatures Face features
	 * @param faceBounds Face bounding boxes
	 * @param featureGroups Output feature groups
	 */
	static void clusterFacesByFeaturesOptimized(
		const std::vector<FaceFeature>& faceFeatures,
		const std::vector<Bnd_Box>& faceBounds,
		std::vector<std::vector<int>>& featureGroups);

	/**
	 * @brief Find faces in nearby grid cells
	 * @param faceIndex Face index
	 * @param spatialGrid Spatial grid
	 * @param faceBounds Face bounding boxes
	 * @param gridSize Grid size
	 * @return Vector of nearby face indices
	 */
	static std::vector<int> findNearbyFaces(
		int faceIndex,
		const std::vector<std::vector<int>>& spatialGrid,
		const std::vector<Bnd_Box>& faceBounds,
		int gridSize);

	/**
	 * @brief Enhanced feature similarity check
	 * @param f1 First face feature
	 * @param f2 Second face feature
	 * @param b1 First bounding box
	 * @param b2 Second bounding box
	 * @return true if features are similar
	 */
	static bool areFeaturesSimilar(
		const FaceFeature& f1,
		const FaceFeature& f2,
		const Bnd_Box& b1,
		const Bnd_Box& b2);

	/**
	 * @brief Create components from face groups with validation
	 * @param faceFeatures Face features
	 * @param featureGroups Face groups
	 * @param components Output components
	 */
	static void createComponentsFromGroups(
		const std::vector<FaceFeature>& faceFeatures,
		const std::vector<std::vector<int>>& featureGroups,
		std::vector<TopoDS_Shape>& components);

	/**
	 * @brief Merge small similar components
	 * @param components Components to merge
	 */
	static void mergeSmallComponents(std::vector<TopoDS_Shape>& components);

	/**
	 * @brief Optimized adjacency graph building with spatial filtering
	 * @param faces Faces
	 * @param faceBounds Face bounding boxes
	 * @param adjacencyGraph Output adjacency graph
	 */
	static void buildFaceAdjacencyGraphOptimized(
		const std::vector<TopoDS_Face>& faces,
		const std::vector<Bnd_Box>& faceBounds,
		std::vector<std::vector<int>>& adjacencyGraph);

	/**
	 * @brief Optimized adjacent face clustering
	 * @param faces Faces
	 * @param adjacencyGraph Adjacency graph
	 * @param clusters Output clusters
	 */
	static void clusterAdjacentFacesOptimized(
		const std::vector<TopoDS_Face>& faces,
		const std::vector<std::vector<int>>& adjacencyGraph,
		std::vector<std::vector<int>>& clusters);

	/**
	 * @brief Validate cluster quality
	 * @param cluster Face cluster
	 * @param faces Face list
	 * @return true if cluster is valid
	 */
	static bool isValidCluster(const std::vector<int>& cluster, const std::vector<TopoDS_Face>& faces);

	/**
	 * @brief Create validated components from clusters
	 * @param faces Faces
	 * @param clusters Face clusters
	 * @param components Output components
	 */
	static void createValidatedComponentsFromClusters(
		const std::vector<TopoDS_Face>& faces,
		const std::vector<std::vector<int>>& clusters,
		std::vector<TopoDS_Shape>& components);

	/**
	 * @brief Refine and filter components
	 * @param components Components to refine
	 */
	static void refineComponents(std::vector<TopoDS_Shape>& components);

	/**
	 * @brief Check if two shapes are similar for merging
	 * @param shape1 First shape
	 * @param shape2 Second shape
	 * @return true if shapes are similar
	 */
	static bool areShapesSimilar(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);




	// Static members
	static bool s_initialized;
};