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

// STEP metadata includes - simplified for basic functionality
#include <StepData_StepModel.hxx>
#include <StepRepr_RepresentationItem.hxx>
#include <TCollection_HAsciiString.hxx>
#include <StepVisual_Colour.hxx>
#include <StepVisual_ColourRgb.hxx>
#include <StepVisual_SurfaceStyleUsage.hxx>
#include <StepVisual_StyledItem.hxx>

// Tessellation includes for smooth surfaces
#include <BRepMesh_IncrementalMesh.hxx>

// File system includes
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <chrono>

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

		// Use enhanced STEP reader settings for smooth surfaces
		STEPControl_Reader reader;
		
		// Set precision mode with fine tessellation
		Interface_Static::SetIVal("read.precision.mode", 1);
		Interface_Static::SetRVal("read.precision.val", options.precision);
		
		// Configure tessellation parameters for smooth surfaces
		if (options.enableFineTessellation) {
			Interface_Static::SetRVal("mesh.deflection", options.tessellationDeflection);
			Interface_Static::SetRVal("mesh.angular_deflection", options.tessellationAngle);
			Interface_Static::SetIVal("mesh.minimum_points", options.tessellationMinPoints);
			Interface_Static::SetIVal("mesh.maximum_points", options.tessellationMaxPoints);
			
			// Enable adaptive tessellation if requested
			if (options.enableAdaptiveTessellation) {
				Interface_Static::SetIVal("mesh.adaptive", 1);
			}
		}
		
		// Use balanced settings for better surface quality
		Interface_Static::SetIVal("read.step.optimize", 0);  // Disable aggressive optimization
		Interface_Static::SetIVal("read.step.fast_mode", 0); // Disable fast mode for better quality
		
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

		// Convert to geometry objects with simplified processing
		std::string baseName = std::filesystem::path(filePath).stem().string();
		result.geometries = shapeToGeometries(result.rootShape, baseName, options, progress, 70, 15);

		// Apply fine tessellation for smooth surfaces
		if (!result.geometries.empty() && options.enableFineTessellation) {
			applyFineTessellation(result.geometries, options);
		}
		if (progress) progress(80, "tessellation");

		// Apply colors to geometries for assembly visualization
		if (!result.geometries.empty()) {
			applyColorsToGeometries(result.geometries, result.entityMetadata, result.assemblyStructure);
		}
		if (progress) progress(85, "colors");

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

		// Set default color for imported STEP models
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
	
	// Enhanced surface and curve tessellation settings for smooth display
	// These settings improve the quality of surface tessellation similar to FreeCAD
	
	// Enable fine surface curve tessellation
	Interface_Static::SetIVal("read.surfacecurve.3d", 1);
	Interface_Static::SetIVal("read.surfacecurve.2d", 1);
	
	// Set tessellation parameters for smooth surfaces
	Interface_Static::SetRVal("read.maxprecision.mode", 1);
	Interface_Static::SetRVal("read.maxprecision.val", 0.001);
	
	// Enable comprehensive surface reading
	Interface_Static::SetIVal("read.step.face", 1);
	Interface_Static::SetIVal("read.step.surface_curve", 1);
	Interface_Static::SetIVal("read.step.curve_2d", 1);
	
	// Enable advanced surface processing
	Interface_Static::SetIVal("read.step.surface", 1);
	Interface_Static::SetIVal("read.step.geometric_curve", 1);
	
	// Set mesh generation parameters for better visualization
	Interface_Static::SetRVal("mesh.deflection", 0.01);
	Interface_Static::SetRVal("mesh.angular_deflection", 0.1);
	Interface_Static::SetIVal("mesh.minimum_points", 3);
	Interface_Static::SetIVal("mesh.maximum_points", 100);
	
	LOG_INF_S("Enhanced STEP reader initialized with fine tessellation settings");
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

				// Try to extract color information
				extractColorFromEntity(entity, info);
				
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
				component.shapeIndex = i - 1; // Convert to 0-based index
				
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

void STEPReader::extractColorFromEntity(
	const Handle(Standard_Transient)& entity,
	STEPEntityInfo& info)
{
	try {
		// Try to extract color from styled item
		Handle(StepVisual_StyledItem) styledItem = 
			Handle(StepVisual_StyledItem)::DownCast(entity);
		
		if (!styledItem.IsNull()) {
			// Get the styles
			const Handle(StepVisual_HArray1OfPresentationStyleAssignment)& styles = 
				styledItem->Styles();
			
			if (!styles.IsNull() && styles->Length() > 0) {
				// For now, use a simple approach to extract color
				// This is a simplified implementation - in practice, 
				// color extraction from STEP files can be quite complex
				info.hasColor = true;
				info.color = Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB); // Default gray
			}
		}
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Exception extracting color from entity: " + std::string(e.what()));
	}
}

std::vector<Quantity_Color> STEPReader::generateDistinctColors(int componentCount)
{
	std::vector<Quantity_Color> colors;
	colors.reserve(componentCount);

	// Predefined distinct colors for assembly components
	std::vector<Quantity_Color> predefinedColors = {
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

	// Use predefined colors first
	for (int i = 0; i < componentCount && i < (int)predefinedColors.size(); i++) {
		colors.push_back(predefinedColors[i]);
	}

	// Generate additional colors if needed
	if (componentCount > (int)predefinedColors.size()) {
		for (int i = predefinedColors.size(); i < componentCount; i++) {
			// Generate colors using HSV color space for better distribution
			double hue = (double)(i - predefinedColors.size()) / (componentCount - predefinedColors.size());
			double saturation = 0.8;
			double value = 0.9;
			
			// Convert HSV to RGB
			double c = value * saturation;
			double x = c * (1.0 - std::abs(std::fmod(hue * 6.0, 2.0) - 1.0));
			double m = value - c;
			
			double r, g, b;
			if (hue < 1.0/6.0) {
				r = c; g = x; b = 0;
			} else if (hue < 2.0/6.0) {
				r = x; g = c; b = 0;
			} else if (hue < 3.0/6.0) {
				r = 0; g = c; b = x;
			} else if (hue < 4.0/6.0) {
				r = 0; g = x; b = c;
			} else if (hue < 5.0/6.0) {
				r = x; g = 0; b = c;
			} else {
				r = c; g = 0; b = x;
			}
			
			colors.push_back(Quantity_Color(r + m, g + m, b + m, Quantity_TOC_RGB));
		}
	}

	return colors;
}

void STEPReader::applyColorsToGeometries(
	std::vector<std::shared_ptr<OCCGeometry>>& geometries,
	const std::vector<STEPEntityInfo>& entityMetadata,
	const STEPAssemblyInfo& assemblyInfo)
{
	if (geometries.empty()) {
		return;
	}

	try {
		// Generate distinct colors for components
		std::vector<Quantity_Color> distinctColors = generateDistinctColors(geometries.size());
		
		// Apply colors to geometries
		for (size_t i = 0; i < geometries.size(); i++) {
			if (geometries[i]) {
				Quantity_Color colorToUse;
				bool hasCustomColor = false;
				
				// Try to find color from entity metadata
				for (const auto& entity : entityMetadata) {
					if (entity.shapeIndex == (int)i && entity.hasColor) {
						colorToUse = entity.color;
						hasCustomColor = true;
						break;
					}
				}
				
				// Use distinct color if no custom color found
				if (!hasCustomColor && i < distinctColors.size()) {
					colorToUse = distinctColors[i];
				} else if (!hasCustomColor) {
					// Fallback to default color
					colorToUse = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
				}
				
				geometries[i]->setColor(colorToUse);
				geometries[i]->setTransparency(0.0);
				
				LOG_INF_S("Applied color to geometry " + std::to_string(i) + 
					": R=" + std::to_string(colorToUse.Red()) + 
					" G=" + std::to_string(colorToUse.Green()) + 
					" B=" + std::to_string(colorToUse.Blue()));
			}
		}
		
		LOG_INF_S("Applied colors to " + std::to_string(geometries.size()) + " geometries");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception applying colors to geometries: " + std::string(e.what()));
	}
}

void STEPReader::applyFineTessellation(
	std::vector<std::shared_ptr<OCCGeometry>>& geometries,
	const OptimizationOptions& options)
{
	if (geometries.empty()) {
		return;
	}

	try {
		LOG_INF_S("Applying fine tessellation to " + std::to_string(geometries.size()) + " geometries");
		
		for (auto& geometry : geometries) {
			if (!geometry || geometry->getShape().IsNull()) {
				continue;
			}

			TopoDS_Shape shape = geometry->getShape();
			
			// Create incremental mesh for fine tessellation
			BRepMesh_IncrementalMesh mesh(shape, 
				options.tessellationDeflection,
				Standard_False,  // Relative deflection
				options.tessellationAngle,
				Standard_True);  // In parallel
			
			// Perform the meshing
			mesh.Perform();
			
			if (mesh.IsDone()) {
				LOG_INF_S("Fine tessellation completed for geometry: " + geometry->getName());
			} else {
				LOG_WRN_S("Fine tessellation failed for geometry: " + geometry->getName());
			}
		}
		
		LOG_INF_S("Fine tessellation completed for all geometries");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception applying fine tessellation: " + std::string(e.what()));
	}
}
