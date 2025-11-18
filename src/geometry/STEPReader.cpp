#include "STEPReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"
#include "rendering/GeometryProcessor.h"
#include "config/RenderingConfig.h"
#include "STEPGeometryDecomposer.h"
#include "STEPColorManager.h"
#include "STEPMetadataExtractor.h"
#include "STEPCAFProcessor.h"
#include "STEPReaderUtils.h"
#include "STEPGeometryConverter.h"

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
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <ShapeFix_Shell.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <Bnd_Box.hxx>
#include <stack>
#include <execution>
#include <numeric>
#include <set>

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
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <TopLoc_Location.hxx>

// File system includes
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>
#include <locale>
#include <codecvt>
#include <functional>

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

// Utility functions moved to STEPReaderUtils
// Decomposition functions moved to STEPGeometryDecomposer
// Geometry conversion functions moved to STEPGeometryConverter

bool STEPReader::validateFile(const std::string& filePath, ReadResult& result) {
		// Check if file exists
		if (!std::filesystem::exists(filePath)) {
			result.errorMessage = "File does not exist: " + filePath;
			LOG_ERR_S(result.errorMessage);
		return false;
		}

		// Check file extension
		if (!isSTEPFile(filePath)) {
			result.errorMessage = "File is not a STEP file: " + filePath;
			LOG_ERR_S(result.errorMessage);
		return false;
	}

	return true;
}

bool STEPReader::readSTEPFileCore(const std::string& filePath, const OptimizationOptions& options,
	ReadResult& result, ProgressCallback progress) {

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
		return false;
		}
		if (progress) progress(20, "read");

		// Check for transferable roots
		Standard_Integer nbRoots = reader.NbRootsForTransfer();
		if (nbRoots == 0) {
			result.errorMessage = "No transferable entities found in STEP file";
			LOG_ERR_S(result.errorMessage);
		return false;
		}
		
		STEPReaderUtils::logCount("Found ", nbRoots, " transferable roots");

		// Transfer all roots
		reader.TransferRoots();
		Standard_Integer nbShapes = reader.NbShapes();
		if (progress) progress(35, "transfer");
		
		STEPReaderUtils::logCount("Transferred ", nbShapes, " shapes");

		if (nbShapes == 0) {
			result.errorMessage = "No shapes could be transferred from STEP file";
			LOG_ERR_S(result.errorMessage);
		return false;
	}

	return assembleShapes(reader, result, progress);
}

bool STEPReader::assembleShapes(const STEPControl_Reader& reader, ReadResult& result, ProgressCallback progress) {
	Standard_Integer nbShapes = reader.NbShapes();

		// Handle single shape or multiple shapes
		if (nbShapes == 1) {
			// Single shape - use it directly
			result.rootShape = reader.Shape(1);
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
			return false;
			}
			
			STEPReaderUtils::logCount("Created compound with ", validShapes, " shapes");
			result.rootShape = compound;
		}

		if (progress) progress(45, "assemble");
	return true;
}

void STEPReader::extractMetadata(const STEPControl_Reader& reader, ReadResult& result, ProgressCallback progress) {
		try {
			result.entityMetadata = readSTEPMetadata(reader);
			result.assemblyStructure = buildAssemblyStructure(reader);
			if (progress) progress(60, "metadata");
		} catch (const std::exception& e) {
			LOG_WRN_S("Failed to read metadata: " + std::string(e.what()));
		}
}

bool STEPReader::tryCAFReader(const std::string& filePath, const OptimizationOptions& options,
	ReadResult& result, ProgressCallback progress) {

		size_t fileSize = std::filesystem::file_size(filePath);
		
		// Helper lambda to check if CAF results contain meaningful color information
		auto hasValidColorInfo = [](const std::vector<std::shared_ptr<OCCGeometry>>& geometries) -> bool {
			if (geometries.empty()) return false;
			
			// Default gray color that OCCT uses when no color is specified
			const Quantity_Color defaultGray(0.7, 0.7, 0.7, Quantity_TOC_RGB);
			const double colorTolerance = 0.01; // Small tolerance for floating point comparison
			
			// Check if any geometry has a non-default color
			int nonDefaultColorCount = 0;
			for (const auto& geom : geometries) {
				if (!geom) continue;
				Quantity_Color color = geom->getColor();
				
				// Check if color is significantly different from default gray
				bool isDifferent = (std::abs(color.Red() - defaultGray.Red()) > colorTolerance ||
								  std::abs(color.Green() - defaultGray.Green()) > colorTolerance ||
								  std::abs(color.Blue() - defaultGray.Blue()) > colorTolerance);
				
				if (isDifferent) {
					nonDefaultColorCount++;
				}
			}
			
			// Consider CAF valid if at least one component has a non-default color
			return nonDefaultColorCount > 0;
		};
		
		try {
			LOG_INF_S("Attempting to read STEP file with CAF reader: " + filePath + 
					 " (size: " + std::to_string(fileSize / (1024 * 1024)) + " MB)");
			
			ReadResult cafResult = STEPCAFProcessor::processSTEPFileWithCAF(filePath, options, progress);
			
			if (cafResult.success && !cafResult.geometries.empty()) {
				// Check if CAF contains useful color/material information
				bool hasColors = hasValidColorInfo(cafResult.geometries);
				
				if (hasColors) {
					// Use CAF results - they contain valuable color information
					result.geometries = cafResult.geometries;
					result.entityMetadata = cafResult.entityMetadata;
					result.assemblyStructure = cafResult.assemblyStructure;
					LOG_INF_S("CAF reader successful with color information - using CAF results (" + 
						std::to_string(result.geometries.size()) + " colored components)");
					
					// Log color information for debugging
					int coloredCount = 0;
					for (size_t i = 0; i < result.geometries.size(); i++) {
						Quantity_Color color = result.geometries[i]->getColor();
						if (std::abs(color.Red() - 0.7) > 0.01 || 
							std::abs(color.Green() - 0.7) > 0.01 || 
							std::abs(color.Blue() - 0.7) > 0.01) {
							LOG_DBG_S("Component " + std::to_string(i) + " color: R=" + 
								std::to_string(color.Red()) + " G=" + std::to_string(color.Green()) + 
								" B=" + std::to_string(color.Blue()));
							coloredCount++;
						}
					}
					STEPReaderUtils::logCount("Found ", coloredCount, " components with custom colors");
		return true;
				} else {
					LOG_INF_S("CAF reader returned only default colors - falling back to standard reader with decomposition");
				return false;
				}
			} else {
				LOG_WRN_S("CAF reader failed: " + (cafResult.errorMessage.empty() ? "Unknown error" : cafResult.errorMessage));
				LOG_WRN_S("Falling back to standard reader with decomposition");
			return false;
			}
		} catch (const std::exception& e) {
			LOG_WRN_S("CAF reader exception: " + std::string(e.what()));
			LOG_INF_S("Falling back to standard reader with decomposition");
		return false;
	}
}

void STEPReader::postProcessGeometries(ReadResult& result, ProgressCallback progress) {
		// Apply automatic scaling to make geometries reasonable size
		if (!result.geometries.empty()) {
			double scaleFactor = STEPGeometryConverter::scaleGeometriesToReasonableSize(result.geometries);
		}
		if (progress) progress(95, "postprocess");
}

STEPReader::ReadResult STEPReader::readSTEPFile(const std::string& filePath,
	const OptimizationOptions& options,
	ProgressCallback progress)
{
	auto totalStartTime = std::chrono::high_resolution_clock::now();
	ReadResult result;

	try {
		// Step 1: Validate input file
		if (!validateFile(filePath, result)) {
			return result;
		}

		// Step 2: Read and parse STEP file using standard OCCT reader
		STEPControl_Reader reader;
		if (!readSTEPFileCore(filePath, options, result, progress)) {
			return result;
		}

		// Step 3: Extract metadata from STEP file
		extractMetadata(reader, result, progress);

		// Step 4: Try CAF reader for enhanced color/material support
		bool cafSuccess = tryCAFReader(filePath, options, result, progress);

		// Step 5: Convert shape to geometries (fallback if CAF failed)
		if (!cafSuccess) {
			std::string baseName = std::filesystem::path(filePath).stem().string();
			result.geometries = STEPGeometryConverter::shapeToGeometries(
				result.rootShape, baseName, options, progress, 70, 25);
		}

		// Step 6: Apply post-processing to geometries
		postProcessGeometries(result, progress);

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

	// Pre-allocate vector for better performance
	shapes.reserve(1000); // Reasonable initial capacity

	// If it's a compound, extract its children
	if (compound.ShapeType() == TopAbs_COMPOUND) {
		// Count shapes first for better memory allocation
		int solidCount = 0, shellCount = 0, faceCount = 0;
		for (TopExp_Explorer counter(compound, TopAbs_SOLID); counter.More(); counter.Next()) {
			solidCount++;
		}
		
		if (solidCount > 0) {
			shapes.reserve(solidCount);
			for (TopExp_Explorer exp(compound, TopAbs_SOLID); exp.More(); exp.Next()) {
				shapes.push_back(exp.Current());
			}
		} else {
			// If no solids found, try shells
			for (TopExp_Explorer counter(compound, TopAbs_SHELL); counter.More(); counter.Next()) {
				shellCount++;
			}
			
			if (shellCount > 0) {
				shapes.reserve(shellCount);
				for (TopExp_Explorer exp(compound, TopAbs_SHELL); exp.More(); exp.Next()) {
					shapes.push_back(exp.Current());
				}
			} else {
				// If no shells found, try faces
				for (TopExp_Explorer counter(compound, TopAbs_FACE); counter.More(); counter.Next()) {
					faceCount++;
				}
				
				if (faceCount > 0) {
					shapes.reserve(faceCount);
					for (TopExp_Explorer exp(compound, TopAbs_FACE); exp.More(); exp.Next()) {
						shapes.push_back(exp.Current());
					}
				} else {
					// If still no shapes found, try any sub-shapes
					for (TopExp_Explorer exp(compound, TopAbs_SHAPE); exp.More(); exp.Next()) {
						if (exp.Current().ShapeType() != TopAbs_COMPOUND) {
							shapes.push_back(exp.Current());
						}
					}
				}
			}
		}
	}
	else {
		// It's a single shape
		shapes.push_back(compound);
	}
	
	// Shrink to fit to release unused memory
	shapes.shrink_to_fit();
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

		STEPReaderUtils::logCount("Extracted metadata for ", metadata.size(), " entities");
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

		STEPReaderUtils::logCount("Built assembly structure with ", assemblyInfo.components.size(), " components");
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

// readSTEPFileWithCAF implementation moved to STEPCAFProcessor
STEPReader::ReadResult STEPReader::readSTEPFileWithCAF(const std::string& filePath,
	const OptimizationOptions& options,
	ProgressCallback progress)
{
	// Delegate to STEPCAFProcessor
	return STEPCAFProcessor::processSTEPFileWithCAF(filePath, options, progress);
}
