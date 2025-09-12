#include "STEPReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE STEP import includes
#include <STEPControl_Reader.hxx>
#include <Interface_Static.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ConstructionError.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <GeomLProp_SLProps.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

// STEP metadata includes
#include <STEPCAFControl_Reader.hxx>
#include <XCAFApp_Application.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <TDataStd_Name.hxx>
#include <TCollection_AsciiString.hxx>
#include <StepData_StepModel.hxx>
#include <StepRepr_RepresentationItem.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorType.hxx>

// File system includes
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>
#include <locale>
#include <codecvt>

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

		// Initialize STEP reader
		initialize();
		if (progress) progress(5, "initialize");

		// Use standard STEP reader settings following FreeCAD approach
		STEPControl_Reader reader;
		
		// Set basic precision mode
		Interface_Static::SetIVal("read.precision.mode", 1);
		Interface_Static::SetRVal("read.precision.val", options.precision);
		
		// Use default OCCT settings for better compatibility
		Interface_Static::SetIVal("read.step.optimize", 1);
		Interface_Static::SetIVal("read.step.fast_mode", 1);
		
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

		// Read metadata using STEPCAFControl_Reader for better metadata support
		try {
			result.entityMetadata = readSTEPMetadata(reader);
			result.assemblyStructure = buildAssemblyStructure(reader);
			if (progress) progress(60, "metadata");
		} catch (const std::exception& e) {
			LOG_WRN_S("Failed to read metadata: " + std::string(e.what()));
		}

		// Use STEPCAFControl_Reader for advanced color and assembly information
		bool cafSuccess = false;
		try {
			LOG_INF_S("Attempting to read STEP file with CAF reader: " + filePath);
			ReadResult cafResult = readSTEPFileWithCAF(filePath, options, progress);
			if (cafResult.success && !cafResult.geometries.empty()) {
				// Use CAF results if successful
				result.geometries = cafResult.geometries;
				result.entityMetadata = cafResult.entityMetadata;
				result.assemblyStructure = cafResult.assemblyStructure;
				cafSuccess = true;
				LOG_INF_S("Successfully read STEP file with CAF reader, found " + 
					std::to_string(result.geometries.size()) + " colored components");
				
				// Log color information for debugging
				for (size_t i = 0; i < result.geometries.size(); i++) {
					Quantity_Color color = result.geometries[i]->getColor();
					LOG_INF_S("Component " + std::to_string(i) + " color: R=" + 
						std::to_string(color.Red()) + " G=" + std::to_string(color.Green()) + 
						" B=" + std::to_string(color.Blue()));
				}
			} else {
				LOG_WRN_S("CAF reader failed: " + (cafResult.errorMessage.empty() ? "Unknown error" : cafResult.errorMessage));
				LOG_WRN_S("Falling back to standard reader");
			}
		} catch (const std::exception& e) {
			LOG_WRN_S("CAF reader exception: " + std::string(e.what()));
		}

		// Convert to geometry objects with simplified processing (fallback if CAF failed)
		if (!cafSuccess) {
			std::string baseName = std::filesystem::path(filePath).stem().string();
			result.geometries = shapeToGeometries(result.rootShape, baseName, options, progress, 70, 25);
		}

		// Apply automatic scaling to make geometries reasonable size
		if (!result.geometries.empty()) {
			double scaleFactor = scaleGeometriesToReasonableSize(result.geometries);
		}
		if (progress) progress(95, "postprocess");

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

		// Sequential processing with progress (simplified from parallel)
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
		// Use OCCT raw shape without active fixing (simplified approach)
		auto geometry = std::make_shared<OCCGeometry>(name);
		geometry->setShape(shape);

		// Set distinct color for imported STEP models based on name hash
		static std::vector<Quantity_Color> distinctColors = {
			Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), // Red
			Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB), // Green
			Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB), // Blue
			Quantity_Color(1.0, 1.0, 0.0, Quantity_TOC_RGB), // Yellow
			Quantity_Color(1.0, 0.0, 1.0, Quantity_TOC_RGB), // Magenta
			Quantity_Color(0.0, 1.0, 1.0, Quantity_TOC_RGB), // Cyan
			Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB), // Orange
			Quantity_Color(0.5, 0.0, 1.0, Quantity_TOC_RGB), // Purple
			Quantity_Color(0.0, 0.5, 0.0, Quantity_TOC_RGB), // Dark Green
			Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB), // Gray
			Quantity_Color(1.0, 0.5, 0.5, Quantity_TOC_RGB), // Light Red
			Quantity_Color(0.5, 1.0, 0.5, Quantity_TOC_RGB), // Light Green
			Quantity_Color(0.5, 0.5, 1.0, Quantity_TOC_RGB), // Light Blue
			Quantity_Color(1.0, 1.0, 0.5, Quantity_TOC_RGB), // Light Yellow
			Quantity_Color(1.0, 0.5, 1.0, Quantity_TOC_RGB), // Light Magenta
		};
		
		// Generate color index based on name hash for consistent coloring
		std::hash<std::string> hasher;
		size_t hashValue = hasher(name);
		size_t colorIndex = hashValue % distinctColors.size();
		Quantity_Color componentColor = distinctColors[colorIndex];
		geometry->setColor(componentColor);

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
	// Set up basic STEP reader parameters (simplified)
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

std::vector<STEPReader::STEPEntityInfo> STEPReader::readSTEPMetadata(
	const STEPControl_Reader& reader)
{
	std::vector<STEPEntityInfo> metadata;
	
	try {
		// Get the STEP model
		Handle(StepData_StepModel) stepModel = reader.StepModel();
		if (stepModel.IsNull()) {
			LOG_WRN_S("No STEP model available for metadata extraction");
			return metadata;
		}

		// Extract entity information
		Standard_Integer nbEntities = stepModel->NbEntities();
		metadata.reserve(nbEntities);

		for (Standard_Integer i = 1; i <= nbEntities; i++) {
			Handle(Standard_Transient) entity = stepModel->Entity(i);
			if (!entity.IsNull()) {
				STEPEntityInfo info;
				info.entityId = i;
				
				// Get entity type name
				info.type = entity->DynamicType()->Name();
				
				// Try to get name if available
				Handle(StepRepr_RepresentationItem) reprItem = 
					Handle(StepRepr_RepresentationItem)::DownCast(entity);
				if (!reprItem.IsNull()) {
					Handle(TCollection_HAsciiString) name = reprItem->Name();
					if (!name.IsNull()) {
						info.name = name->ToCString();
					}
				}
				
				metadata.push_back(info);
			}
		}

		LOG_INF_S("Extracted metadata for " + std::to_string(metadata.size()) + " entities");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception reading STEP metadata: " + std::string(e.what()));
	}

	return metadata;
}

STEPReader::STEPAssemblyInfo STEPReader::buildAssemblyStructure(
	const STEPControl_Reader& reader)
{
	STEPAssemblyInfo assemblyInfo;
	
	try {
		// Get the STEP model
		Handle(StepData_StepModel) stepModel = reader.StepModel();
		if (stepModel.IsNull()) {
			LOG_WRN_S("No STEP model available for assembly structure");
			return assemblyInfo;
		}

		// Set basic assembly info
		assemblyInfo.name = "Root Assembly";
		assemblyInfo.type = "ASSEMBLY";

		// Extract components from transferred shapes
		Standard_Integer nbShapes = reader.NbShapes();
		for (Standard_Integer i = 1; i <= nbShapes; i++) {
			TopoDS_Shape shape = reader.Shape(i);
			if (!shape.IsNull()) {
				STEPEntityInfo component;
				component.name = "Component_" + std::to_string(i);
				component.type = "SHAPE";
				component.entityId = i;
				
				assemblyInfo.components.push_back(component);
			}
		}

		LOG_INF_S("Built assembly structure with " + std::to_string(assemblyInfo.components.size()) + " components");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception building assembly structure: " + std::string(e.what()));
	}

	return assemblyInfo;
}

STEPReader::STEPEntityInfo STEPReader::extractEntityInfo(
	const STEPControl_Reader& reader,
	int entityId)
{
	STEPEntityInfo info;
	info.entityId = entityId;
	
	try {
		Handle(StepData_StepModel) stepModel = reader.StepModel();
		if (!stepModel.IsNull() && entityId > 0 && entityId <= stepModel->NbEntities()) {
			Handle(Standard_Transient) entity = stepModel->Entity(entityId);
			if (!entity.IsNull()) {
				info.type = entity->DynamicType()->Name();
				
				// Try to get name if available
				Handle(StepRepr_RepresentationItem) reprItem = 
					Handle(StepRepr_RepresentationItem)::DownCast(entity);
				if (!reprItem.IsNull()) {
					Handle(TCollection_HAsciiString) name = reprItem->Name();
					if (!name.IsNull()) {
						info.name = name->ToCString();
					}
				}
			}
		}
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception extracting entity info: " + std::string(e.what()));
	}

	return info;
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

	// Sequential processing for simplicity
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
		// Use simplified bounding box calculation
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

		// Apply scaling sequentially for simplicity
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

		return scaleFactor;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception scaling geometries: " + std::string(e.what()));
		return 1.0;
	}
}

STEPReader::ReadResult STEPReader::readSTEPFileWithCAF(const std::string& filePath,
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

		if (progress) progress(5, "initialize CAF");

		// Create XCAF application
		Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
		if (app.IsNull()) {
			result.errorMessage = "Failed to create XCAF application";
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		// Create document
		Handle(TDocStd_Document) doc;
		app->NewDocument("MDTV-XCAF", doc);
		if (doc.IsNull()) {
			result.errorMessage = "Failed to create XCAF document";
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		if (progress) progress(10, "create document");

		// Create STEPCAFControl_Reader
		STEPCAFControl_Reader cafReader;
		cafReader.SetColorMode(true);
		cafReader.SetNameMode(true);
		cafReader.SetMatMode(true);
		cafReader.SetGDTMode(true);
		cafReader.SetLayerMode(true);

		// Read the file
		IFSelect_ReturnStatus status = cafReader.ReadFile(filePath.c_str());
		if (status != IFSelect_RetDone) {
			result.errorMessage = "Failed to read STEP file with CAF: " + filePath + 
				" (Status: " + std::to_string(static_cast<int>(status)) + ")";
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		if (progress) progress(30, "read CAF");

		// Transfer all roots
		cafReader.Transfer(doc);
		if (progress) progress(50, "transfer CAF");

		// Get shape tool and color tool
		Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
		Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());
		Handle(XCAFDoc_MaterialTool) materialTool = XCAFDoc_DocumentTool::MaterialTool(doc->Main());

		if (shapeTool.IsNull()) {
			result.errorMessage = "Failed to get shape tool from CAF document";
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		// Get all free shapes (top-level shapes)
		TDF_LabelSequence freeShapes;
		shapeTool->GetFreeShapes(freeShapes);

		if (freeShapes.Length() == 0) {
			result.errorMessage = "No free shapes found in CAF document";
			LOG_ERR_S(result.errorMessage);
			return result;
		}

		LOG_INF_S("Found " + std::to_string(freeShapes.Length()) + " free shapes in CAF document");

		if (progress) progress(60, "extract shapes");

		// Process each free shape
		std::string baseName = std::filesystem::path(filePath).stem().string();
		int componentIndex = 0;
		
		LOG_INF_S("Processing " + std::to_string(freeShapes.Length()) + " components with distinct colors");

		for (int i = 1; i <= freeShapes.Length(); i++) {
			TDF_Label label = freeShapes.Value(i);
			
			// Get shape from label
			TopoDS_Shape shape = shapeTool->GetShape(label);
			if (shape.IsNull()) {
				continue;
			}

			// Get name from label
			std::string componentName = baseName + "_Component_" + std::to_string(componentIndex);
			Handle(TDataStd_Name) nameAttr;
			if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr)) {
				TCollection_ExtendedString extStr = nameAttr->Get();
				// Convert ExtendedString to std::string using AsciiString as bridge
				TCollection_AsciiString asciiStr(extStr);
				componentName = asciiStr.ToCString();
			}

			// Get color from label
			Quantity_Color color;
			bool hasColor = false;
			if (!colorTool.IsNull()) {
				hasColor = colorTool->GetColor(label, XCAFDoc_ColorGen, color) ||
						   colorTool->GetColor(label, XCAFDoc_ColorSurf, color) ||
						   colorTool->GetColor(label, XCAFDoc_ColorCurv, color);
			}

			// Always generate distinct colors for better visualization (override any existing color)
			// Generate distinct colors for components
			static std::vector<Quantity_Color> distinctColors = {
				Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), // Red
				Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB), // Green
				Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB), // Blue
				Quantity_Color(1.0, 1.0, 0.0, Quantity_TOC_RGB), // Yellow
				Quantity_Color(1.0, 0.0, 1.0, Quantity_TOC_RGB), // Magenta
				Quantity_Color(0.0, 1.0, 1.0, Quantity_TOC_RGB), // Cyan
				Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB), // Orange
				Quantity_Color(0.5, 0.0, 1.0, Quantity_TOC_RGB), // Purple
				Quantity_Color(0.0, 0.5, 0.0, Quantity_TOC_RGB), // Dark Green
				Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB), // Gray
				Quantity_Color(1.0, 0.5, 0.5, Quantity_TOC_RGB), // Light Red
				Quantity_Color(0.5, 1.0, 0.5, Quantity_TOC_RGB), // Light Green
				Quantity_Color(0.5, 0.5, 1.0, Quantity_TOC_RGB), // Light Blue
				Quantity_Color(1.0, 1.0, 0.5, Quantity_TOC_RGB), // Light Yellow
				Quantity_Color(1.0, 0.5, 1.0, Quantity_TOC_RGB), // Light Magenta
			};
			
			// Use distinct color for each component (override any existing color for better visualization)
			color = distinctColors[componentIndex % distinctColors.size()];
			hasColor = true;
			
			LOG_INF_S("Assigned color to component " + std::to_string(componentIndex) + 
				" (" + componentName + "): R=" + std::to_string(color.Red()) + 
				" G=" + std::to_string(color.Green()) + " B=" + std::to_string(color.Blue()));

			// Create geometry object
			auto geometry = std::make_shared<OCCGeometry>(componentName);
			geometry->setShape(shape);
			geometry->setColor(color);
			geometry->setTransparency(0.0);

			// Create entity info
			STEPEntityInfo entityInfo;
			entityInfo.name = componentName;
			entityInfo.type = "COMPONENT";
			entityInfo.color = color;
			entityInfo.hasColor = hasColor;
			entityInfo.entityId = componentIndex;
			entityInfo.shapeIndex = componentIndex;

			result.geometries.push_back(geometry);
			result.entityMetadata.push_back(entityInfo);

			LOG_INF_S("Created colored component: " + componentName + 
				" (R=" + std::to_string(color.Red()) + 
				" G=" + std::to_string(color.Green()) + 
				" B=" + std::to_string(color.Blue()) + ")");

			componentIndex++;
		}

		if (progress) progress(80, "process components");

		// Build assembly structure
		result.assemblyStructure.name = baseName;
		result.assemblyStructure.type = "ASSEMBLY";
		for (const auto& entity : result.entityMetadata) {
			result.assemblyStructure.components.push_back(entity);
		}

		// Apply automatic scaling
		if (!result.geometries.empty()) {
			double scaleFactor = scaleGeometriesToReasonableSize(result.geometries);
			LOG_INF_S("Applied scaling factor: " + std::to_string(scaleFactor));
		}

		if (progress) progress(95, "postprocess");

		result.success = true;

		// Calculate total import time
		auto totalEndTime = std::chrono::high_resolution_clock::now();
		auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEndTime - totalStartTime);
		result.importTime = static_cast<double>(totalDuration.count());

		LOG_INF_S("CAF import completed successfully: " + std::to_string(result.geometries.size()) + 
			" colored components in " + std::to_string(result.importTime) + "ms");

		if (progress) progress(100, "done");
	}
	catch (const Standard_Failure& e) {
		result.errorMessage = "OpenCASCADE CAF exception: " + std::string(e.GetMessageString());
		LOG_ERR_S(result.errorMessage);
	}
	catch (const std::exception& e) {
		result.errorMessage = "Exception reading STEP file with CAF: " + std::string(e.what());
		LOG_ERR_S(result.errorMessage);
	}

	return result;
}
