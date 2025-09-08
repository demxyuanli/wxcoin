#include "STEPReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE STEP import includes
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <Interface_Static.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <APIHeaderSection_MakeHeader.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Label.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ConstructionError.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <ShapeFix_Shape.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <GeomLProp_SLProps.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

// File system includes
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cfloat>
#include <chrono>
#include <thread>
#include <execution>

// Add include at top
#include <XCAFApp_Application.hxx>

// GeometryReader interface implementation
GeometryReader::ReadResult STEPReader::readFile(const std::string& filePath,
	const OptimizationOptions& options,
	ProgressCallback progress)
{
	ReadResult result = readSTEPFile(filePath, options, progress);
	GeometryReader::ReadResult baseResult;
	baseResult.success = result.success;
	baseResult.errorMessage = result.errorMessage;
	baseResult.geometries = result.geometries;
	baseResult.rootShape = result.rootShape;
	baseResult.importTime = result.importTime;
	baseResult.formatName = "STEP";
	return baseResult;
}

bool STEPReader::isValidFile(const std::string& filePath) const
{
	return isSTEPFile(filePath);
}

std::vector<std::string> STEPReader::getSupportedExtensions() const
{
	return { ".step", ".stp" };
}

std::string STEPReader::getFormatName() const
{
	return "STEP";
}

std::string STEPReader::getFileFilter() const
{
	return "STEP files (*.step;*.stp)|*.step;*.stp";
}

// Static member initialization
std::unordered_map<std::string, STEPReader::ReadResult> STEPReader::s_cache;
std::mutex STEPReader::s_cacheMutex;
STEPReader::OptimizationOptions STEPReader::s_globalOptions;
bool STEPReader::s_initialized = false;

STEPReader::ReadResult STEPReader::readSTEPFile(const std::string& filePath,
	const OptimizationOptions& options,
	ProgressCallback progress)
{
	auto totalStartTime = std::chrono::high_resolution_clock::now();
	ReadResult result;

	try {
		// Check if file exists
		if (!std::filesystem::exists(filePath)) {
			result.errorMessage = "File does not exist: " + filePath;
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		// Check file extension
		if (!isSTEPFile(filePath)) {
			result.errorMessage = "File is not a STEP file: " + filePath;
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		// Check cache if enabled
		if (options.enableCaching) {
			std::lock_guard<std::mutex> lock(s_cacheMutex);
			auto cacheIt = s_cache.find(filePath);
			if (cacheIt != s_cache.end()) {
				result = cacheIt->second;
				return result;
			}
		}

		// Initialize STEP reader
		initialize();
		if (progress) progress(5, "initialize");

		// Use standard STEP reader settings following OpenCASCADE best practices
		STEPControl_Reader reader;
		
		// Set precision mode and value
		Interface_Static::SetIVal("read.precision.mode", 1);
		Interface_Static::SetRVal("read.precision.val", options.precision);
		
		// Disable aggressive optimizations that might cause issues
		Interface_Static::SetIVal("read.step.optimize", 0);
		Interface_Static::SetIVal("read.step.fast_mode", 0);
		
		// Enable comprehensive reading
		Interface_Static::SetIVal("read.step.nonmanifold", 1);
		Interface_Static::SetIVal("read.step.face", 1);
		Interface_Static::SetIVal("read.step.surface_curve", 1);
		
		// Read the file
		IFSelect_ReturnStatus status = reader.ReadFile(filePath.c_str());
		
		if (status != IFSelect_RetDone) {
			result.errorMessage = "Failed to read STEP file: " + filePath + 
				" (Status: " + std::to_string(static_cast<int>(status)) + ")";
			LOG_ERR_S(result.errorMessage);
			return result;
		}
		if (progress) progress(20, "read");

		// Check for transferable roots
		Standard_Integer nbRoots = reader.NbRootsForTransfer();
		if (nbRoots == 0) {
			result.errorMessage = "No transferable entities found in STEP file";
			LOG_ERR_S(result.errorMessage);
			return result;
		}
		
		LOG_INF_S("Found " + std::to_string(nbRoots) + " transferable roots");

		// Transfer all roots
		reader.TransferRoots();
		Standard_Integer nbShapes = reader.NbShapes();
		if (progress) progress(35, "transfer");
		
		LOG_INF_S("Transferred " + std::to_string(nbShapes) + " shapes");

		if (nbShapes == 0) {
			result.errorMessage = "No shapes could be transferred from STEP file";
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		// Handle single shape or multiple shapes
		if (nbShapes == 1) {
			// Single shape - use it directly
			result.rootShape = reader.Shape(1);
			LOG_INF_S("Using single shape directly");
		} else {
			// Multiple shapes - create compound
			TopoDS_Compound compound;
			BRep_Builder builder;
			builder.MakeCompound(compound);
			
			Standard_Integer validShapes = 0;
			for (Standard_Integer i = 1; i <= nbShapes; i++) {
				TopoDS_Shape shape = reader.Shape(i);
				if (!shape.IsNull()) {
					builder.Add(compound, shape);
					validShapes++;
				}
			}
			
			if (validShapes == 0) {
				result.errorMessage = "No valid shapes found in STEP file";
				LOG_ERR_S(result.errorMessage);
				return result;
			}
			
			LOG_INF_S("Created compound with " + std::to_string(validShapes) + " shapes");
			result.rootShape = compound;
		}
		if (progress) progress(45, "assemble");

		// Convert to geometry objects with optimization
		std::string baseName = std::filesystem::path(filePath).stem().string();
		result.geometries = shapeToGeometries(result.rootShape, baseName, options, progress, 50, 40);

		// Apply automatic scaling to make geometries reasonable size
		if (!result.geometries.empty()) {
			double scaleFactor = scaleGeometriesToReasonableSize(result.geometries);
		}
		if (progress) progress(92, "postprocess");

		// Cache result if enabled
		if (options.enableCaching) {
			std::lock_guard<std::mutex> lock(s_cacheMutex);
			s_cache[filePath] = result;
		}

		result.success = true;

		// Calculate total import time
		auto totalEndTime = std::chrono::high_resolution_clock::now();
		auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEndTime - totalStartTime);
		result.importTime = static_cast<double>(totalDuration.count());
		if (progress) progress(100, "done");
	}
	catch (const Standard_Failure& e) {
		result.errorMessage = "OpenCASCADE exception: " + std::string(e.GetMessageString());
		LOG_ERR_S(result.errorMessage);
	}
	catch (const std::exception& e) {
		result.errorMessage = "Exception reading STEP file: " + std::string(e.what());
		LOG_ERR_S(result.errorMessage);
	}

	return result;
}

TopoDS_Shape STEPReader::readSTEPShape(const std::string& filePath)
{
	ReadResult result = readSTEPFile(filePath);
	return result.success ? result.rootShape : TopoDS_Shape();
}

bool STEPReader::isSTEPFile(const std::string& filePath)
{
	std::string extension = std::filesystem::path(filePath).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	return extension == ".step" || extension == ".stp";
}

std::vector<std::shared_ptr<OCCGeometry>> STEPReader::shapeToGeometries(
	const TopoDS_Shape& shape,
	const std::string& baseName,
	const OptimizationOptions& options,
	ProgressCallback progress,
	int progressStart,
	int progressSpan)
{
	std::vector<std::shared_ptr<OCCGeometry>> geometries;

	if (shape.IsNull()) {
		LOG_WRN_S("Cannot convert null shape to geometries");
		return geometries;
	}

	try {
		// Extract individual shapes
		std::vector<TopoDS_Shape> shapes;
		extractShapes(shape, shapes);

		LOG_INF_S("Converting " + std::to_string(shapes.size()) + " shapes to geometries for: " + baseName);

		// Use parallel processing if enabled and there are multiple shapes
		if (options.enableParallelProcessing && shapes.size() > 1) {
			geometries = processShapesParallel(shapes, baseName, options, progress, progressStart, progressSpan);
		}
		else {
			// Sequential processing with progress
			size_t total = shapes.size();
			size_t successCount = 0;
			size_t failCount = 0;
			
			for (size_t i = 0; i < shapes.size(); ++i) {
				if (!shapes[i].IsNull()) {
					std::string name = baseName + "_" + std::to_string(i);
					auto geometry = processSingleShape(shapes[i], name, options);
					if (geometry) {
						geometries.push_back(geometry);
						successCount++;
					} else {
						failCount++;
					}
				}
				if (progress && total > 0) {
					int pct = progressStart + (int)std::round(((double)(i + 1) / (double)total) * progressSpan);
					pct = std::max(progressStart, std::min(progressStart + progressSpan, pct));
					progress(pct, "convert");
				}
			}
			
			if (failCount > 0) {
				LOG_WRN_S("Failed to process " + std::to_string(failCount) + " out of " + 
					std::to_string(total) + " shapes for: " + baseName);
			}
		}
	}
	catch (const Standard_ConstructionError& e) {
		LOG_ERR_S("Construction error converting shapes: " + std::string(e.GetMessageString()));
		LOG_ERR_S("This typically indicates invalid or degenerate geometry in the STEP file");
	}
	catch (const Standard_Failure& e) {
		LOG_ERR_S("OpenCASCADE error converting shapes: " + std::string(e.GetMessageString()));
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception converting shape to geometries: " + std::string(e.what()));
	}

	return geometries;
}

std::vector<std::shared_ptr<OCCGeometry>> STEPReader::processShapesParallel(
	const std::vector<TopoDS_Shape>& shapes,
	const std::string& baseName,
	const OptimizationOptions& options,
	ProgressCallback progress,
	int progressStart,
	int progressSpan)
{
	std::vector<std::shared_ptr<OCCGeometry>> geometries;
	geometries.reserve(shapes.size());

	// Create futures for parallel processing
	std::vector<std::future<std::shared_ptr<OCCGeometry>>> futures;
	futures.reserve(shapes.size());

	// Submit tasks to thread pool with proper value capture to avoid thread safety issues
	for (size_t i = 0; i < shapes.size(); ++i) {
		if (!shapes[i].IsNull()) {
			std::string name = baseName + "_" + std::to_string(i);
			// Capture by value to avoid thread safety issues with references
			futures.push_back(std::async(std::launch::async,
				[shape = shapes[i], name, options]() {
					return processSingleShape(shape, name, options);
				}));
		}
	}

	// Collect results with progress
	size_t total = futures.size();
	size_t successCount = 0;
	size_t failCount = 0;
	
	for (size_t idx = 0; idx < futures.size(); ++idx) {
		try {
			auto geometry = futures[idx].get();
			if (geometry) {
				geometries.push_back(geometry);
				successCount++;
			} else {
				failCount++;
			}
		}
		catch (const Standard_ConstructionError& e) {
			LOG_ERR_S("Construction error in parallel shape processing: " + std::string(e.GetMessageString()));
			failCount++;
		}
		catch (const Standard_Failure& e) {
			LOG_ERR_S("OpenCASCADE error in parallel shape processing: " + std::string(e.GetMessageString()));
			failCount++;
		}
		catch (const std::exception& e) {
			LOG_ERR_S("Error in parallel shape processing: " + std::string(e.what()));
			failCount++;
		}
		catch (...) {
			LOG_ERR_S("Unknown error in parallel shape processing");
			failCount++;
		}
		
		if (progress && total > 0) {
			int pct = progressStart + (int)std::round(((double)(idx + 1) / (double)total) * progressSpan);
			pct = std::max(progressStart, std::min(progressStart + progressSpan, pct));
			progress(pct, "convert");
		}
	}
	
	if (failCount > 0) {
		LOG_WRN_S("Parallel processing: Failed " + std::to_string(failCount) + 
			" out of " + std::to_string(total) + " shapes");
	}

	return geometries;
}

std::shared_ptr<OCCGeometry> STEPReader::processSingleShape(
	const TopoDS_Shape& shape,
	const std::string& name,
	const OptimizationOptions& options)
{
	if (shape.IsNull()) {
		LOG_WRN_S("Skipping null shape for: " + name);
		return nullptr;
	}

	try {
		// Validate the shape before processing
		BRepCheck_Analyzer analyzer(shape);
		if (!analyzer.IsValid()) {
			LOG_WRN_S("Invalid shape detected for: " + name + ", attempting to fix...");
			
			// Note: Detailed validation status logging removed due to API compatibility issues
			// The analyzer.IsValid() check is sufficient for our purposes
			
			// Try to fix the shape
			ShapeFix_Shape shapeFixer(shape);
			shapeFixer.SetPrecision(options.precision);
			shapeFixer.SetMaxTolerance(options.precision * 10);
			shapeFixer.Perform();
			
			TopoDS_Shape fixedShape = shapeFixer.Shape();
			if (!fixedShape.IsNull()) {
				LOG_INF_S("Shape fixed successfully for: " + name);
				// Continue with fixed shape
				auto geometry = std::make_shared<OCCGeometry>(name);
				geometry->setShape(fixedShape);
				
				// Set better default color for imported STEP models
				Quantity_Color defaultColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
				geometry->setColor(defaultColor);
				
				// Remove transparency for a solid appearance
				geometry->setTransparency(0.0);
				
				return geometry;
			} else {
				LOG_ERR_S("Failed to fix invalid shape for: " + name);
				return nullptr;
			}
		}

		// Apply normal direction consistency fix
		TopoDS_Shape consistentShape = ensureConsistentNormalDirections(shape, name);
		
		auto geometry = std::make_shared<OCCGeometry>(name);
		geometry->setShape(consistentShape);

		// Set better default color for imported STEP models
		Quantity_Color defaultColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
		geometry->setColor(defaultColor);

		// Remove transparency for a solid appearance
		geometry->setTransparency(0.0);

		// Only analyze shape if explicitly enabled (disabled by default for speed)
		if (options.enableShapeAnalysis) {
			OCCShapeBuilder::analyzeShapeTopology(shape, name);
		}

		return geometry;
	}
	catch (const Standard_ConstructionError& e) {
		LOG_ERR_S("Construction error processing shape " + name + ": " + std::string(e.GetMessageString()));
		LOG_ERR_S("This often happens with degenerate or invalid geometry. Skipping this shape.");
		return nullptr;
	}
	catch (const Standard_Failure& e) {
		LOG_ERR_S("OpenCASCADE error processing shape " + name + ": " + std::string(e.GetMessageString()));
		return nullptr;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception processing shape " + name + ": " + std::string(e.what()));
		return nullptr;
	}
}

void STEPReader::initialize()
{
	// Set up STEP reader parameters
	Interface_Static::SetIVal("read.step.ideas", 1);
	Interface_Static::SetIVal("read.step.nonmanifold", 1);
	Interface_Static::SetIVal("read.step.product.mode", 1);
	Interface_Static::SetIVal("read.step.product.context", 1);
	Interface_Static::SetIVal("read.step.shape.repr", 1);
	Interface_Static::SetIVal("read.step.assembly.level", 1);

	// Set precision
	Interface_Static::SetRVal("read.precision.val", 0.01);
	Interface_Static::SetIVal("read.precision.mode", 1);
}

void STEPReader::extractShapes(const TopoDS_Shape& compound, std::vector<TopoDS_Shape>& shapes)
{
	if (compound.IsNull()) {
		return;
	}

	// If it's a compound, extract its children
	if (compound.ShapeType() == TopAbs_COMPOUND) {
		for (TopExp_Explorer exp(compound, TopAbs_SOLID); exp.More(); exp.Next()) {
			shapes.push_back(exp.Current());
		}

		// If no solids found, try shells
		if (shapes.empty()) {
			for (TopExp_Explorer exp(compound, TopAbs_SHELL); exp.More(); exp.Next()) {
				shapes.push_back(exp.Current());
			}
		}

		// If no shells found, try faces
		if (shapes.empty()) {
			for (TopExp_Explorer exp(compound, TopAbs_FACE); exp.More(); exp.Next()) {
				shapes.push_back(exp.Current());
			}
		}

		// If still no shapes found, try any sub-shapes
		if (shapes.empty()) {
			for (TopExp_Explorer exp(compound, TopAbs_SHAPE); exp.More(); exp.Next()) {
				if (exp.Current().ShapeType() != TopAbs_COMPOUND) {
					shapes.push_back(exp.Current());
				}
			}
		}
	}
	else {
		// It's a single shape
		shapes.push_back(compound);
	}
}

void STEPReader::clearCache()
{
	std::lock_guard<std::mutex> lock(s_cacheMutex);
	s_cache.clear();
	LOG_INF_S("STEP import cache cleared");
}

std::string STEPReader::getCacheStats()
{
	std::lock_guard<std::mutex> lock(s_cacheMutex);
	return "Cache entries: " + std::to_string(s_cache.size());
}

void STEPReader::setGlobalOptimizationOptions(const OptimizationOptions& options)
{
	s_globalOptions = options;
	LOG_INF_S("Global STEP optimization options updated");
}

STEPReader::OptimizationOptions STEPReader::getGlobalOptimizationOptions()
{
	return s_globalOptions;
}

bool STEPReader::calculateCombinedBoundingBox(
	const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
	gp_Pnt& minPt,
	gp_Pnt& maxPt)
{
	if (geometries.empty()) {
		return false;
	}

	minPt = gp_Pnt(DBL_MAX, DBL_MAX, DBL_MAX);
	maxPt = gp_Pnt(-DBL_MAX, -DBL_MAX, -DBL_MAX);
	bool hasValidBounds = false;

	// Use parallel processing for large geometry sets
	if (geometries.size() > 10) {
		std::vector<gp_Pnt> minPoints(geometries.size());
		std::vector<gp_Pnt> maxPoints(geometries.size());

		// Initialize with invalid bounds
		for (size_t i = 0; i < geometries.size(); ++i) {
			minPoints[i] = gp_Pnt(DBL_MAX, DBL_MAX, DBL_MAX);
			maxPoints[i] = gp_Pnt(-DBL_MAX, -DBL_MAX, -DBL_MAX);
		}

		// Process in parallel
		std::for_each(std::execution::par, geometries.begin(), geometries.end(),
			[&](const auto& geometry) {
				size_t index = &geometry - &geometries[0];
				if (geometry && !geometry->getShape().IsNull()) {
					gp_Pnt localMin, localMax;
					OCCShapeBuilder::getBoundingBox(geometry->getShape(), localMin, localMax);
					minPoints[index] = localMin;
					maxPoints[index] = localMax;
				}
			});

		// Combine results
		for (size_t i = 0; i < geometries.size(); ++i) {
			if (minPoints[i].X() != DBL_MAX) { // Valid bounds
				if (minPoints[i].X() < minPt.X()) minPt.SetX(minPoints[i].X());
				if (minPoints[i].Y() < minPt.Y()) minPt.SetY(minPoints[i].Y());
				if (minPoints[i].Z() < minPt.Z()) minPt.SetZ(minPoints[i].Z());

				if (maxPoints[i].X() > maxPt.X()) maxPt.SetX(maxPoints[i].X());
				if (maxPoints[i].Y() > maxPt.Y()) maxPt.SetY(maxPoints[i].Y());
				if (maxPoints[i].Z() > maxPt.Z()) maxPt.SetZ(maxPoints[i].Z());

				hasValidBounds = true;
			}
		}
	}
	else {
		// Sequential processing for small geometry sets
		for (const auto& geometry : geometries) {
			if (!geometry || geometry->getShape().IsNull()) {
				continue;
			}

			gp_Pnt localMin, localMax;
			OCCShapeBuilder::getBoundingBox(geometry->getShape(), localMin, localMax);

			if (localMin.X() < minPt.X()) minPt.SetX(localMin.X());
			if (localMin.Y() < minPt.Y()) minPt.SetY(localMin.Y());
			if (localMin.Z() < minPt.Z()) minPt.SetZ(localMin.Z());

			if (localMax.X() > maxPt.X()) maxPt.SetX(localMax.X());
			if (localMax.Y() > maxPt.Y()) maxPt.SetY(localMax.Y());
			if (localMax.Z() > maxPt.Z()) maxPt.SetZ(localMax.Z());

			hasValidBounds = true;
		}
	}

	return hasValidBounds;
}

double STEPReader::scaleGeometriesToReasonableSize(
	std::vector<std::shared_ptr<OCCGeometry>>& geometries,
	double targetSize)
{
	if (geometries.empty()) {
		return 1.0;
	}

	try {
		// Use optimized bounding box calculation
		gp_Pnt overallMin, overallMax;
		if (!calculateCombinedBoundingBox(geometries, overallMin, overallMax)) {
			LOG_WRN_S("No valid bounds found for scaling");
			return 1.0;
		}

		// Calculate current size
		double currentSizeX = overallMax.X() - overallMin.X();
		double currentSizeY = overallMax.Y() - overallMin.Y();
		double currentSizeZ = overallMax.Z() - overallMin.Z();
		double currentMaxSize = (std::max)({ currentSizeX, currentSizeY, currentSizeZ });

		// Determine target size
		if (targetSize <= 0.0) {
			// Auto-detect reasonable size (10-50 units)
			if (currentMaxSize > 100.0) {
				targetSize = 20.0;  // Scale large models down
			}
			else if (currentMaxSize < 0.1) {
				targetSize = 10.0;  // Scale tiny models up
			}
			else {
				// Size is already reasonable
				return 1.0;
			}
		}

		double scaleFactor = targetSize / currentMaxSize;

		if (std::abs(scaleFactor - 1.0) < 0.01) {
			// No significant scaling needed
			return 1.0;
		}

		// Apply scaling in parallel for large geometry sets
		if (geometries.size() > 5) {
			std::for_each(std::execution::par, geometries.begin(), geometries.end(),
				[scaleFactor](auto& geometry) {
					if (geometry && !geometry->getShape().IsNull()) {
						TopoDS_Shape scaledShape = OCCShapeBuilder::scale(
							geometry->getShape(),
							gp_Pnt(0, 0, 0),
							scaleFactor
						);
						if (!scaledShape.IsNull()) {
							geometry->setShape(scaledShape);
						}
					}
				});
		}
		else {
			// Sequential scaling for small geometry sets
			for (auto& geometry : geometries) {
				if (!geometry || geometry->getShape().IsNull()) {
					continue;
				}

				TopoDS_Shape scaledShape = OCCShapeBuilder::scale(
					geometry->getShape(),
					gp_Pnt(0, 0, 0),
					scaleFactor
				);

				if (!scaledShape.IsNull()) {
					geometry->setShape(scaledShape);
				}
			}
		}

		return scaleFactor;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception scaling geometries: " + std::string(e.what()));
		return 1.0;
	}
}

TopoDS_Shape STEPReader::ensureConsistentNormalDirections(const TopoDS_Shape& shape, const std::string& name)
{
	if (shape.IsNull()) {
		return shape;
	}

	try {
		// Calculate overall bounding box to determine object center
		Bnd_Box bbox;
		BRepBndLib::Add(shape, bbox);

		if (bbox.IsVoid()) {
			LOG_WRN_S("Cannot determine bounding box for shape: " + name);
			return shape;
		}

		Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
		bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

		// Calculate object center
		gp_Pnt objectCenter(
			(xMin + xMax) / 2.0,
			(yMin + yMax) / 2.0,
			(zMin + zMax) / 2.0
		);

		LOG_INF_S("Object center for " + name + ": (" +
			std::to_string(objectCenter.X()) + ", " +
			std::to_string(objectCenter.Y()) + ", " +
			std::to_string(objectCenter.Z()) + ")");

		// Process each face to ensure consistent normal directions
		std::vector<TopoDS_Face> originalFaces;
		std::vector<TopoDS_Face> correctedFaces;

		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
			TopoDS_Face face = TopoDS::Face(exp.Current());
			originalFaces.push_back(face);
		}

		LOG_INF_S("Processing " + std::to_string(originalFaces.size()) + " faces for normal consistency");

		int reversedCount = 0;
		int processedCount = 0;
		int correctCount = 0;
		int incorrectCount = 0;

		for (const TopoDS_Face& face : originalFaces) {
			TopoDS_Face correctedFace = face;

			// Get face center
			Bnd_Box faceBox;
			BRepBndLib::Add(face, faceBox);

			if (!faceBox.IsVoid()) {
				Standard_Real fxMin, fyMin, fzMin, fxMax, fyMax, fzMax;
				faceBox.Get(fxMin, fyMin, fzMin, fxMax, fyMax, fzMax);

				gp_Pnt faceCenter(
					(fxMin + fxMax) / 2.0,
					(fyMin + fyMax) / 2.0,
					(fzMin + fzMax) / 2.0
				);

				// Calculate face normal at center
				Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
				if (!surface.IsNull()) {
					// Get UV parameters at face center
					Standard_Real uMin, uMax, vMin, vMax;
					BRepTools::UVBounds(face, uMin, uMax, vMin, vMax);

					Standard_Real uMid = (uMin + uMax) / 2.0;
					Standard_Real vMid = (vMin + vMax) / 2.0;

					// Get normal at face center
					GeomLProp_SLProps props(surface, uMid, vMid, 1, 1e-6);
					if (props.IsNormalDefined()) {
						gp_Vec faceNormal = props.Normal();

						// Apply face orientation
						if (face.Orientation() == TopAbs_REVERSED) {
							faceNormal.Reverse();
						}

						// Calculate vector from object center to face center
						// This is more accurate than center to center
						gp_Vec centerToFace(
							faceCenter.X() - objectCenter.X(),
							faceCenter.Y() - objectCenter.Y(),
							faceCenter.Z() - objectCenter.Z()
						);

						// Normalize the direction vector
						if (centerToFace.Magnitude() > 1e-6) {
							centerToFace.Normalize();

							// Check if normal points outward (away from object center)
							// A positive dot product means the normal points in the same direction as centerToFace
							// which means it points outward from the center
							double dotProduct = faceNormal.Dot(centerToFace);

							if (dotProduct < 0) {
								// Normal points inward, reverse the face
								correctedFace.Reverse();
								reversedCount++;
								incorrectCount++;
							} else {
								correctCount++;
							}
						}
						processedCount++;
					}
				}
			}

			correctedFaces.push_back(correctedFace);
		}

		LOG_INF_S("Processed " + std::to_string(processedCount) + " faces, reversed " +
			std::to_string(reversedCount) + " faces for consistent normals");
		LOG_INF_S("Normal consistency: " + std::to_string(correctCount) + " correct, " + 
			std::to_string(incorrectCount) + " corrected");

		// Rebuild the shape with corrected faces
		if (shape.ShapeType() == TopAbs_COMPOUND) {
			TopoDS_Compound compound;
			BRep_Builder builder;
			builder.MakeCompound(compound);

			// Add all solids from original shape
			for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
				builder.Add(compound, exp.Current());
			}

			// Add all shells from original shape
			for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
				builder.Add(compound, exp.Current());
			}

			// Add corrected faces
			for (const TopoDS_Face& face : correctedFaces) {
				builder.Add(compound, face);
			}

			// Add any other shapes that might exist
			for (TopExp_Explorer exp(shape, TopAbs_SHAPE); exp.More(); exp.Next()) {
				TopAbs_ShapeEnum shapeType = exp.Current().ShapeType();
				if (shapeType != TopAbs_SOLID && shapeType != TopAbs_SHELL && shapeType != TopAbs_FACE) {
					builder.Add(compound, exp.Current());
				}
			}

			return compound;
		} else if (shape.ShapeType() == TopAbs_SOLID) {
			// For solids, we need to rebuild the solid with corrected faces
			// This is more complex and might require ShapeFix operations
			// For now, return the original solid with a warning
			LOG_WRN_S("Solid shape normal correction not fully implemented, using original shape");
			return shape;
		} else {
			// For single shapes, return the first corrected face if it's a single face
			if (correctedFaces.size() == 1) {
				return correctedFaces[0];
			}
			return shape;
		}
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception ensuring consistent normal directions: " + std::string(e.what()));
		return shape;
	}
}
