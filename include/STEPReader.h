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
#include <OpenCASCADE/Quantity_Color.hxx>
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




	// Static members
	static bool s_initialized;
};