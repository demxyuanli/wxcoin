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

// Helper function to safely convert ExtendedString to std::string
static std::string safeConvertExtendedString(const TCollection_ExtendedString& extStr) {
	try {
		// First try direct conversion
		TCollection_AsciiString asciiStr(extStr);
		const char* cStr = asciiStr.ToCString();
		if (cStr != nullptr) {
			std::string result(cStr);
			// Check if the result contains only printable ASCII characters
			bool isValid = true;
			for (char c : result) {
				if (c < 32 || c > 126) { // Not printable ASCII
					isValid = false;
					break;
				}
			}
			if (isValid && !result.empty()) {
				return result;
			}
		}
	} catch (const std::exception& e) {
		LOG_WRN_S("ExtendedString conversion failed: " + std::string(e.what()));
	}
	
	// Fallback: convert character by character, keeping only ASCII
	std::string result;
	const Standard_ExtString extCStr = extStr.ToExtString();
	if (extCStr != nullptr) {
		for (int i = 0; extCStr[i] != 0; i++) {
			wchar_t wc = extCStr[i];
			if (wc >= 32 && wc <= 126) { // Printable ASCII range
				result += static_cast<char>(wc);
			}
		}
	}
	
	return result.empty() ? "UnnamedComponent" : result;
}

// Forward declarations
static void decomposeByFaceGroups(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes);
static bool areFacesSimilar(const TopoDS_Face& face1, const TopoDS_Face& face2);
static void decomposeShapeFreeCADLike(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes);
static void decomposeByConnectivity(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes);
static bool areFacesConnected(const TopoDS_Face& face1, const TopoDS_Face& face2);
static void decomposeByGeometricFeatures(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes);
static void decomposeByShellGroups(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes);

// Helper function to decompose a shape into sub-shapes using FreeCAD-like strategy
static void decomposeShape(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
	subShapes.clear();
	
	try {
		LOG_INF_S("Starting shape decomposition for shape type: " + std::to_string(shape.ShapeType()));
		
		// Strategy 1: Try to decompose into solids first (most common case)
		int solidCount = 0;
		for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
			subShapes.push_back(exp.Current());
			solidCount++;
		}
		LOG_INF_S("Strategy 1 - Found " + std::to_string(solidCount) + " solids");
		
		// Strategy 2: If no solids found, try shells
		if (subShapes.empty()) {
			int shellCount = 0;
			for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
				subShapes.push_back(exp.Current());
				shellCount++;
			}
			LOG_INF_S("Strategy 2 - Found " + std::to_string(shellCount) + " shells");
		}
		
		// Strategy 3: If still no sub-shapes, try to decompose by face groups
		if (subShapes.empty()) {
			LOG_INF_S("Strategy 3 - Attempting face group decomposition");
			decomposeByFaceGroups(shape, subShapes);
		}
		
		// Strategy 4: If still empty, try faces (for very complex shapes)
		if (subShapes.empty()) {
			int faceCount = 0;
			for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
				subShapes.push_back(exp.Current());
				faceCount++;
			}
			LOG_INF_S("Strategy 4 - Found " + std::to_string(faceCount) + " faces");
		}
		
		// Strategy 5: If still empty, use the original shape
		if (subShapes.empty()) {
			LOG_INF_S("Strategy 5 - Using original shape as single component");
			subShapes.push_back(shape);
		}
		
		LOG_INF_S("Shape decomposition result: " + std::to_string(subShapes.size()) + " sub-shapes");
	} catch (const std::exception& e) {
		LOG_WRN_S("Failed to decompose shape: " + std::string(e.what()));
		subShapes.push_back(shape);
	}
}

// Helper function to decompose using FreeCAD-like approach (prioritize topology over geometry)
static void decomposeShapeFreeCADLike(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
	subShapes.clear();
	
	try {
		LOG_INF_S("Starting FreeCAD-like decomposition for shape type: " + std::to_string(shape.ShapeType()));
		
		// Count different shape types
		int solidCount = 0, shellCount = 0, faceCount = 0, edgeCount = 0, vertexCount = 0;
		
		for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) solidCount++;
		for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) shellCount++;
		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) faceCount++;
		for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) edgeCount++;
		for (TopExp_Explorer exp(shape, TopAbs_VERTEX); exp.More(); exp.Next()) vertexCount++;
		
		LOG_INF_S("Shape analysis - Solids: " + std::to_string(solidCount) + 
			", Shells: " + std::to_string(shellCount) + 
			", Faces: " + std::to_string(faceCount) + 
			", Edges: " + std::to_string(edgeCount) + 
			", Vertices: " + std::to_string(vertexCount));
		
		// FreeCAD-like decomposition strategy (prioritize complete bodies)
		if (solidCount > 1) {
			// Multiple solids - decompose into individual solids
			LOG_INF_S("FreeCAD Strategy: Decomposing " + std::to_string(solidCount) + " solids");
			for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
				subShapes.push_back(exp.Current());
			}
		} else if (solidCount == 1 && shellCount > 1) {
			// Single solid with multiple shells - try to group shells into logical bodies
			LOG_INF_S("FreeCAD Strategy: Single solid with " + std::to_string(shellCount) + " shells - attempting shell grouping");
			decomposeByShellGroups(shape, subShapes);
		} else if (solidCount == 1 && shellCount == 1 && faceCount > 20) {
			// Single solid/shell with many faces - try to group by geometric features
			LOG_INF_S("FreeCAD Strategy: Single solid/shell with " + std::to_string(faceCount) + " faces - attempting geometric feature grouping");
			decomposeByGeometricFeatures(shape, subShapes);
		} else {
			// Single shape - use as is (no decomposition needed)
			LOG_INF_S("FreeCAD Strategy: Single shape - no decomposition needed");
			subShapes.push_back(shape);
		}
		
		LOG_INF_S("FreeCAD-like decomposition result: " + std::to_string(subShapes.size()) + " sub-shapes");
	} catch (const std::exception& e) {
		LOG_WRN_S("Failed FreeCAD-like decomposition: " + std::string(e.what()));
		subShapes.push_back(shape);
	}
}

// Helper function to decompose by connectivity (group connected faces)
static void decomposeByConnectivity(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
	try {
		LOG_INF_S("Starting connectivity-based decomposition");
		
		// Collect all faces
		std::vector<TopoDS_Face> allFaces;
		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
			allFaces.push_back(TopoDS::Face(exp.Current()));
		}
		
		LOG_INF_S("Collected " + std::to_string(allFaces.size()) + " faces for connectivity analysis");
		
		if (allFaces.empty()) {
			subShapes.push_back(shape);
			return;
		}
		
		// Group faces by connectivity (simplified approach)
		std::vector<std::vector<TopoDS_Face>> faceGroups;
		std::vector<bool> processed(allFaces.size(), false);
		
		for (size_t i = 0; i < allFaces.size(); i++) {
			if (processed[i]) continue;
			
			std::vector<TopoDS_Face> currentGroup;
			currentGroup.push_back(allFaces[i]);
			processed[i] = true;
			
			// Find connected faces (simplified - faces sharing edges)
			bool foundMore = true;
			while (foundMore) {
				foundMore = false;
				for (size_t j = 0; j < allFaces.size(); j++) {
					if (processed[j]) continue;
					
					// Check if face j is connected to any face in current group
					bool connected = false;
					for (const auto& groupFace : currentGroup) {
						if (areFacesConnected(allFaces[j], groupFace)) {
							connected = true;
							break;
						}
					}
					
					if (connected) {
						currentGroup.push_back(allFaces[j]);
						processed[j] = true;
						foundMore = true;
					}
				}
			}
			
			faceGroups.push_back(currentGroup);
		}
		
		LOG_INF_S("Created " + std::to_string(faceGroups.size()) + " connectivity groups");
		
		// Convert face groups to shapes
		for (const auto& group : faceGroups) {
			if (group.size() > 0) {
				TopoDS_Compound compound;
				BRep_Builder builder;
				builder.MakeCompound(compound);
				
				for (const auto& face : group) {
					builder.Add(compound, face);
				}
				
				subShapes.push_back(compound);
			}
		}
		
		LOG_INF_S("Connectivity decomposition completed: " + std::to_string(subShapes.size()) + " shapes");
	} catch (const std::exception& e) {
		LOG_WRN_S("Connectivity decomposition failed: " + std::string(e.what()));
		subShapes.push_back(shape);
	}
}

// Helper function to check if two faces are connected (share an edge)
static bool areFacesConnected(const TopoDS_Face& face1, const TopoDS_Face& face2) {
	try {
		// Get edges of both faces
		std::vector<TopoDS_Edge> edges1, edges2;
		
		for (TopExp_Explorer exp(face1, TopAbs_EDGE); exp.More(); exp.Next()) {
			edges1.push_back(TopoDS::Edge(exp.Current()));
		}
		
		for (TopExp_Explorer exp(face2, TopAbs_EDGE); exp.More(); exp.Next()) {
			edges2.push_back(TopoDS::Edge(exp.Current()));
		}
		
		// Check if any edge is shared
		for (const auto& edge1 : edges1) {
			for (const auto& edge2 : edges2) {
				if (edge1.IsSame(edge2)) {
					return true;
				}
			}
		}
		
		return false;
	} catch (const std::exception& e) {
		LOG_WRN_S("Face connectivity check failed: " + std::string(e.what()));
		return false;
	}
}

// Helper function to decompose by face groups (FreeCAD-like approach)
static void decomposeByFaceGroups(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
	try {
		LOG_INF_S("Starting face group decomposition");
		
		// Group faces by geometric properties
		std::vector<std::vector<TopoDS_Face>> faceGroups;
		std::vector<TopoDS_Face> currentGroup;
		
		// Collect all faces
		std::vector<TopoDS_Face> allFaces;
		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
			allFaces.push_back(TopoDS::Face(exp.Current()));
		}
		
		LOG_INF_S("Collected " + std::to_string(allFaces.size()) + " faces for grouping");
		
		if (allFaces.empty()) {
			LOG_WRN_S("No faces found for decomposition");
			return;
		}
		
		// Group faces by surface type and normal direction using improved algorithm
		int faceIndex = 0;
		for (const auto& face : allFaces) {
			bool addedToExistingGroup = false;
			
			// Try to add to existing groups first
			for (auto& group : faceGroups) {
				bool belongsToGroup = false;
				int similarFaces = 0;
				for (const auto& groupFace : group) {
					if (areFacesSimilar(face, groupFace)) {
						belongsToGroup = true;
						similarFaces++;
					}
				}
				
				if (belongsToGroup) {
					group.push_back(face);
					LOG_INF_S("Face " + std::to_string(faceIndex) + " added to existing group (similar to " + 
						std::to_string(similarFaces) + " faces in group)");
					addedToExistingGroup = true;
					break;
				}
			}
			
			// If not added to existing group, start a new group
			if (!addedToExistingGroup) {
				if (currentGroup.empty()) {
					currentGroup.push_back(face);
					LOG_INF_S("Starting new group with face " + std::to_string(faceIndex));
				} else {
					// Check if this face belongs to the current group
					bool belongsToGroup = false;
					int similarFaces = 0;
					for (const auto& groupFace : currentGroup) {
						if (areFacesSimilar(face, groupFace)) {
							belongsToGroup = true;
							similarFaces++;
						}
					}
					
					if (belongsToGroup) {
						currentGroup.push_back(face);
						LOG_INF_S("Face " + std::to_string(faceIndex) + " added to current group (similar to " + 
							std::to_string(similarFaces) + " faces in group)");
					} else {
						// Start a new group
						LOG_INF_S("Face " + std::to_string(faceIndex) + " starts new group (current group has " + 
							std::to_string(currentGroup.size()) + " faces)");
						faceGroups.push_back(currentGroup);
						currentGroup.clear();
						currentGroup.push_back(face);
					}
				}
			}
			faceIndex++;
		}
		
		// Add the last group
		if (!currentGroup.empty()) {
			LOG_INF_S("Adding final group with " + std::to_string(currentGroup.size()) + " faces");
			faceGroups.push_back(currentGroup);
		}
		
		LOG_INF_S("Created " + std::to_string(faceGroups.size()) + " face groups");
		
		// Convert face groups to shapes
		int groupIndex = 0;
		for (const auto& group : faceGroups) {
			if (group.size() > 0) {
				LOG_INF_S("Processing group " + std::to_string(groupIndex) + " with " + 
					std::to_string(group.size()) + " faces");
				
				// Create a compound from the face group
				TopoDS_Compound compound;
				BRep_Builder builder;
				builder.MakeCompound(compound);
				
				for (const auto& face : group) {
					builder.Add(compound, face);
				}
				
				subShapes.push_back(compound);
				groupIndex++;
			}
		}
		
		LOG_INF_S("Face group decomposition completed: " + std::to_string(faceGroups.size()) + 
			" groups converted to " + std::to_string(subShapes.size()) + " shapes");
	} catch (const std::exception& e) {
		LOG_WRN_S("Face group decomposition failed: " + std::string(e.what()));
	}
}

// Helper function to check if two faces are similar (for grouping)
static bool areFacesSimilar(const TopoDS_Face& face1, const TopoDS_Face& face2) {
	try {
		// Get surface types
		Handle(Geom_Surface) surf1 = BRep_Tool::Surface(face1);
		Handle(Geom_Surface) surf2 = BRep_Tool::Surface(face2);
		
		if (surf1.IsNull() || surf2.IsNull()) {
			LOG_INF_S("Face similarity check: One or both surfaces are null");
			return false;
		}
		
		std::string type1 = surf1->DynamicType()->Name();
		std::string type2 = surf2->DynamicType()->Name();
		
		// Check if surfaces are of the same type
		if (surf1->DynamicType() != surf2->DynamicType()) {
			LOG_INF_S("Face similarity check: Different surface types - " + type1 + " vs " + type2);
			return false;
		}
		
		LOG_INF_S("Face similarity check: Both faces are " + type1);
		
		// For planes, check if they are parallel
		if (surf1->DynamicType() == STANDARD_TYPE(Geom_Plane)) {
			Handle(Geom_Plane) plane1 = Handle(Geom_Plane)::DownCast(surf1);
			Handle(Geom_Plane) plane2 = Handle(Geom_Plane)::DownCast(surf2);
			
			if (!plane1.IsNull() && !plane2.IsNull()) {
				gp_Dir normal1 = plane1->Axis().Direction();
				gp_Dir normal2 = plane2->Axis().Direction();
				
				// Check if normals are parallel (within tolerance)
				double dotProduct = normal1.Dot(normal2);
				bool isParallel = std::abs(dotProduct) > 0.7; // 70% parallel (more lenient)
				LOG_INF_S("Plane similarity check: dot product = " + std::to_string(dotProduct) + 
					", parallel = " + (isParallel ? "true" : "false"));
				return isParallel;
			}
		}
		
		// For cylinders, check if they have similar axis
		if (surf1->DynamicType() == STANDARD_TYPE(Geom_CylindricalSurface)) {
			Handle(Geom_CylindricalSurface) cyl1 = Handle(Geom_CylindricalSurface)::DownCast(surf1);
			Handle(Geom_CylindricalSurface) cyl2 = Handle(Geom_CylindricalSurface)::DownCast(surf2);
			
			if (!cyl1.IsNull() && !cyl2.IsNull()) {
				gp_Dir axis1 = cyl1->Axis().Direction();
				gp_Dir axis2 = cyl2->Axis().Direction();
				
				double dotProduct = axis1.Dot(axis2);
				bool isParallel = std::abs(dotProduct) > 0.7; // 70% parallel (more lenient)
				LOG_INF_S("Cylinder similarity check: dot product = " + std::to_string(dotProduct) + 
					", parallel = " + (isParallel ? "true" : "false"));
				return isParallel;
			}
		}
		
		// For other surface types, consider them similar if they're the same type
		LOG_INF_S("Surface similarity check: Same type (" + type1 + "), considering similar");
		return true;
	} catch (const std::exception& e) {
		LOG_WRN_S("Face similarity check failed: " + std::string(e.what()));
		return false;
	}
}

// Helper function to process a single component
static void processComponent(const TopoDS_Shape& shape, const std::string& componentName, 
	int componentIndex, std::vector<std::shared_ptr<OCCGeometry>>& geometries,
	std::vector<STEPReader::STEPEntityInfo>& entityMetadata) {
	
	// Generate distinct colors for components (cool tones and muted)
	static std::vector<Quantity_Color> distinctColors = {
		Quantity_Color(0.4, 0.5, 0.6, Quantity_TOC_RGB), // Cool Blue-Gray
		Quantity_Color(0.3, 0.5, 0.7, Quantity_TOC_RGB), // Steel Blue
		Quantity_Color(0.2, 0.4, 0.6, Quantity_TOC_RGB), // Deep Blue
		Quantity_Color(0.4, 0.6, 0.7, Quantity_TOC_RGB), // Light Blue-Gray
		Quantity_Color(0.3, 0.6, 0.5, Quantity_TOC_RGB), // Teal
		Quantity_Color(0.2, 0.5, 0.4, Quantity_TOC_RGB), // Dark Teal
		Quantity_Color(0.5, 0.4, 0.6, Quantity_TOC_RGB), // Cool Purple
		Quantity_Color(0.4, 0.3, 0.5, Quantity_TOC_RGB), // Muted Purple
		Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB), // Neutral Gray
		Quantity_Color(0.4, 0.4, 0.4, Quantity_TOC_RGB), // Dark Gray
		Quantity_Color(0.6, 0.5, 0.4, Quantity_TOC_RGB), // Cool Beige
		Quantity_Color(0.5, 0.6, 0.5, Quantity_TOC_RGB), // Cool Green-Gray
		Quantity_Color(0.3, 0.4, 0.5, Quantity_TOC_RGB), // Slate Blue
		Quantity_Color(0.4, 0.5, 0.4, Quantity_TOC_RGB), // Cool Green
		Quantity_Color(0.6, 0.4, 0.5, Quantity_TOC_RGB), // Cool Rose
	};
	
	// Use distinct color for each component
	Quantity_Color color = distinctColors[componentIndex % distinctColors.size()];
	
	// Create geometry object
	auto geometry = std::make_shared<OCCGeometry>(componentName);
	geometry->setShape(shape);
	geometry->setColor(color);
	geometry->setTransparency(0.0);

	// Create entity info
	STEPReader::STEPEntityInfo entityInfo;
	entityInfo.name = componentName;
	entityInfo.type = "COMPONENT";
	entityInfo.color = color;
	entityInfo.hasColor = true;
	entityInfo.entityId = componentIndex;
	entityInfo.shapeIndex = componentIndex;

	geometries.push_back(geometry);
	entityMetadata.push_back(entityInfo);

	LOG_INF_S("Created colored component: " + componentName + 
		" (R=" + std::to_string(color.Red()) + 
		" G=" + std::to_string(color.Green()) + 
		" B=" + std::to_string(color.Blue()) + ")");
}

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

		// Set distinct color for imported STEP models based on name hash (cool tones and muted)
		static std::vector<Quantity_Color> distinctColors = {
			Quantity_Color(0.4, 0.5, 0.6, Quantity_TOC_RGB), // Cool Blue-Gray
			Quantity_Color(0.3, 0.5, 0.7, Quantity_TOC_RGB), // Steel Blue
			Quantity_Color(0.2, 0.4, 0.6, Quantity_TOC_RGB), // Deep Blue
			Quantity_Color(0.4, 0.6, 0.7, Quantity_TOC_RGB), // Light Blue-Gray
			Quantity_Color(0.3, 0.6, 0.5, Quantity_TOC_RGB), // Teal
			Quantity_Color(0.2, 0.5, 0.4, Quantity_TOC_RGB), // Dark Teal
			Quantity_Color(0.5, 0.4, 0.6, Quantity_TOC_RGB), // Cool Purple
			Quantity_Color(0.4, 0.3, 0.5, Quantity_TOC_RGB), // Muted Purple
			Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB), // Neutral Gray
			Quantity_Color(0.4, 0.4, 0.4, Quantity_TOC_RGB), // Dark Gray
			Quantity_Color(0.6, 0.5, 0.4, Quantity_TOC_RGB), // Cool Beige
			Quantity_Color(0.5, 0.6, 0.5, Quantity_TOC_RGB), // Cool Green-Gray
			Quantity_Color(0.3, 0.4, 0.5, Quantity_TOC_RGB), // Slate Blue
			Quantity_Color(0.4, 0.5, 0.4, Quantity_TOC_RGB), // Cool Green
			Quantity_Color(0.6, 0.4, 0.5, Quantity_TOC_RGB), // Cool Rose
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
		
		// If only one component, try to decompose it into sub-components for better visualization
		bool tryDecomposition = (freeShapes.Length() == 1);
		if (tryDecomposition) {
			LOG_INF_S("Single component detected, attempting decomposition for better color visualization");
		}

		for (int i = 1; i <= freeShapes.Length(); i++) {
			TDF_Label label = freeShapes.Value(i);
			
			// Get shape from label
			TopoDS_Shape shape = shapeTool->GetShape(label);
			if (shape.IsNull()) {
				continue;
			}

			LOG_INF_S("Processing component " + std::to_string(i) + ", shape type: " + std::to_string(shape.ShapeType()));

			// For single component, try to decompose into sub-shapes for better visualization
			if (tryDecomposition && freeShapes.Length() == 1) {
				std::vector<TopoDS_Shape> subShapes;
				
				// Use FreeCAD-like decomposition strategy for better results
				LOG_INF_S("Using FreeCAD-like decomposition for single component");
				decomposeShapeFreeCADLike(shape, subShapes);
				
				if (subShapes.size() > 1) {
					LOG_INF_S("Decomposed single component into " + std::to_string(subShapes.size()) + " sub-components");
					
					// Process each sub-shape with different colors
					for (size_t j = 0; j < subShapes.size(); j++) {
						processComponent(subShapes[j], baseName + "_Part_" + std::to_string(j), 
							componentIndex, result.geometries, result.entityMetadata);
						componentIndex++;
					}
					continue; // Skip the original single component processing
				} else {
					LOG_INF_S("FreeCAD-like decomposition resulted in single component, using original");
				}
			}

			// Get name from label
			std::string componentName = baseName + "_Component_" + std::to_string(componentIndex);
			Handle(TDataStd_Name) nameAttr;
			if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr)) {
				TCollection_ExtendedString extStr = nameAttr->Get();
				// Convert ExtendedString to std::string safely
				std::string convertedName = safeConvertExtendedString(extStr);
				if (!convertedName.empty() && convertedName != "UnnamedComponent") {
					componentName = convertedName;
				}
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
			// Generate distinct colors for components (cool tones and muted)
			static std::vector<Quantity_Color> distinctColors = {
				Quantity_Color(0.4, 0.5, 0.6, Quantity_TOC_RGB), // Cool Blue-Gray
				Quantity_Color(0.3, 0.5, 0.7, Quantity_TOC_RGB), // Steel Blue
				Quantity_Color(0.2, 0.4, 0.6, Quantity_TOC_RGB), // Deep Blue
				Quantity_Color(0.4, 0.6, 0.7, Quantity_TOC_RGB), // Light Blue-Gray
				Quantity_Color(0.3, 0.6, 0.5, Quantity_TOC_RGB), // Teal
				Quantity_Color(0.2, 0.5, 0.4, Quantity_TOC_RGB), // Dark Teal
				Quantity_Color(0.5, 0.4, 0.6, Quantity_TOC_RGB), // Cool Purple
				Quantity_Color(0.4, 0.3, 0.5, Quantity_TOC_RGB), // Muted Purple
				Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB), // Neutral Gray
				Quantity_Color(0.4, 0.4, 0.4, Quantity_TOC_RGB), // Dark Gray
				Quantity_Color(0.6, 0.5, 0.4, Quantity_TOC_RGB), // Cool Beige
				Quantity_Color(0.5, 0.6, 0.5, Quantity_TOC_RGB), // Cool Green-Gray
				Quantity_Color(0.3, 0.4, 0.5, Quantity_TOC_RGB), // Slate Blue
				Quantity_Color(0.4, 0.5, 0.4, Quantity_TOC_RGB), // Cool Green
				Quantity_Color(0.6, 0.4, 0.5, Quantity_TOC_RGB), // Cool Rose
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

// Helper function to decompose by geometric features (more aggressive than connectivity)
static void decomposeByGeometricFeatures(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
	try {
		LOG_INF_S("Starting geometric feature decomposition");
		
		// Collect all faces
		std::vector<TopoDS_Face> allFaces;
		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
			allFaces.push_back(TopoDS::Face(exp.Current()));
		}
		
		LOG_INF_S("Collected " + std::to_string(allFaces.size()) + " faces for geometric feature analysis");
		
		if (allFaces.empty()) {
			subShapes.push_back(shape);
			return;
		}
		
		// Group faces by surface type and geometric properties
		std::map<std::string, std::vector<TopoDS_Face>> surfaceTypeGroups;
		std::map<std::string, std::vector<TopoDS_Face>> normalGroups;
		
		// First pass: group by surface type
		for (const auto& face : allFaces) {
			try {
				Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
				if (!surf.IsNull()) {
					std::string surfaceType = surf->DynamicType()->Name();
					surfaceTypeGroups[surfaceType].push_back(face);
				}
			} catch (const std::exception& e) {
				LOG_WRN_S("Failed to get surface type for face: " + std::string(e.what()));
			}
		}
		
		LOG_INF_S("Surface type groups: " + std::to_string(surfaceTypeGroups.size()));
		for (const auto& group : surfaceTypeGroups) {
			LOG_INF_S("  " + group.first + ": " + std::to_string(group.second.size()) + " faces");
		}
		
		// Second pass: for planes, group by normal direction
		for (const auto& group : surfaceTypeGroups) {
			if (group.first == "Geom_Plane" && group.second.size() > 1) {
				// Group planes by normal direction
				std::map<std::string, std::vector<TopoDS_Face>> planeGroups;
				
				for (const auto& face : group.second) {
					try {
						Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
						Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(surf);
						if (!plane.IsNull()) {
							gp_Dir normal = plane->Axis().Direction();
							// Create a key based on normal direction (rounded to avoid precision issues)
							std::string normalKey = std::to_string(round(normal.X() * 1000) / 1000) + "_" +
													std::to_string(round(normal.Y() * 1000) / 1000) + "_" +
													std::to_string(round(normal.Z() * 1000) / 1000);
							planeGroups[normalKey].push_back(face);
						}
					} catch (const std::exception& e) {
						LOG_WRN_S("Failed to process plane face: " + std::string(e.what()));
					}
				}
				
				LOG_INF_S("Plane normal groups: " + std::to_string(planeGroups.size()));
				for (const auto& planeGroup : planeGroups) {
					LOG_INF_S("  Normal " + planeGroup.first + ": " + std::to_string(planeGroup.second.size()) + " faces");
					normalGroups["Plane_" + planeGroup.first] = planeGroup.second;
				}
			} else {
				// Keep other surface types as single groups
				normalGroups[group.first] = group.second;
			}
		}
		
		// Create shapes from groups
		int groupIndex = 0;
		for (const auto& group : normalGroups) {
			if (group.second.size() > 0) {
				LOG_INF_S("Creating shape from group " + std::to_string(groupIndex) + 
					" (" + group.first + ") with " + std::to_string(group.second.size()) + " faces");
				
				if (group.second.size() == 1) {
					// Single face - use directly
					subShapes.push_back(group.second[0]);
				} else {
					// Multiple faces - create compound
					TopoDS_Compound compound;
					BRep_Builder builder;
					builder.MakeCompound(compound);
					
					for (const auto& face : group.second) {
						builder.Add(compound, face);
					}
					
					subShapes.push_back(compound);
				}
				groupIndex++;
			}
		}
		
		// If we still have too few groups, try more aggressive decomposition
		if (subShapes.size() <= 2 && allFaces.size() > 50) {
			LOG_INF_S("Too few groups (" + std::to_string(subShapes.size()) + "), trying aggressive decomposition");
			
			// Try decomposing by face area (large vs small faces)
			std::vector<TopoDS_Face> largeFaces, smallFaces;
			double totalArea = 0.0;
			
			for (const auto& face : allFaces) {
				try {
					GProp_GProps props;
					BRepGProp::SurfaceProperties(face, props);
					totalArea += props.Mass();
				} catch (const std::exception& e) {
					LOG_WRN_S("Failed to calculate face area: " + std::string(e.what()));
				}
			}
			
			double averageArea = totalArea / allFaces.size();
			LOG_INF_S("Average face area: " + std::to_string(averageArea));
			
			for (const auto& face : allFaces) {
				try {
					GProp_GProps props;
					BRepGProp::SurfaceProperties(face, props);
					if (props.Mass() > averageArea * 2.0) {
						largeFaces.push_back(face);
					} else {
						smallFaces.push_back(face);
					}
				} catch (const std::exception& e) {
					smallFaces.push_back(face); // Default to small
				}
			}
			
			LOG_INF_S("Area-based grouping: " + std::to_string(largeFaces.size()) + " large faces, " + 
				std::to_string(smallFaces.size()) + " small faces");
			
			// Create shapes from area groups
			subShapes.clear();
			
			if (!largeFaces.empty()) {
				if (largeFaces.size() == 1) {
					subShapes.push_back(largeFaces[0]);
				} else {
					TopoDS_Compound compound;
					BRep_Builder builder;
					builder.MakeCompound(compound);
					for (const auto& face : largeFaces) {
						builder.Add(compound, face);
					}
					subShapes.push_back(compound);
				}
			}
			
			if (!smallFaces.empty()) {
				if (smallFaces.size() == 1) {
					subShapes.push_back(smallFaces[0]);
				} else {
					TopoDS_Compound compound;
					BRep_Builder builder;
					builder.MakeCompound(compound);
					for (const auto& face : smallFaces) {
						builder.Add(compound, face);
					}
					subShapes.push_back(compound);
				}
			}
		}
		
		LOG_INF_S("Geometric feature decomposition completed: " + std::to_string(subShapes.size()) + " shapes");
	} catch (const std::exception& e) {
		LOG_WRN_S("Geometric feature decomposition failed: " + std::string(e.what()));
		subShapes.push_back(shape);
	}
}

// Helper function to decompose by shell groups (group shells into logical bodies)
static void decomposeByShellGroups(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
	try {
		LOG_INF_S("Starting shell group decomposition");
		
		// Collect all shells
		std::vector<TopoDS_Shell> allShells;
		for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
			allShells.push_back(TopoDS::Shell(exp.Current()));
		}
		
		LOG_INF_S("Collected " + std::to_string(allShells.size()) + " shells for grouping");
		
		if (allShells.empty()) {
			subShapes.push_back(shape);
			return;
		}
		
		// If we have few shells, try to group them by volume and connectivity
		if (allShells.size() <= 3) {
			LOG_INF_S("Few shells (" + std::to_string(allShells.size()) + "), grouping by volume and connectivity");
			
			// Group shells by volume (large vs small shells)
			std::vector<TopoDS_Shell> largeShells, smallShells;
			double totalVolume = 0.0;
			
			for (const auto& shell : allShells) {
				try {
					GProp_GProps props;
					BRepGProp::VolumeProperties(shell, props);
					totalVolume += props.Mass();
				} catch (const std::exception& e) {
					LOG_WRN_S("Failed to calculate shell volume: " + std::string(e.what()));
				}
			}
			
			double averageVolume = totalVolume / allShells.size();
			LOG_INF_S("Average shell volume: " + std::to_string(averageVolume));
			
			for (const auto& shell : allShells) {
				try {
					GProp_GProps props;
					BRepGProp::VolumeProperties(shell, props);
					if (props.Mass() > averageVolume * 0.5) { // 50% of average volume threshold
						largeShells.push_back(shell);
					} else {
						smallShells.push_back(shell);
					}
				} catch (const std::exception& e) {
					largeShells.push_back(shell); // Default to large
				}
			}
			
			LOG_INF_S("Volume-based grouping: " + std::to_string(largeShells.size()) + " large shells, " + 
				std::to_string(smallShells.size()) + " small shells");
			
			// Create shapes from volume groups
			if (!largeShells.empty()) {
				if (largeShells.size() == 1) {
					subShapes.push_back(largeShells[0]);
				} else {
					TopoDS_Compound compound;
					BRep_Builder builder;
					builder.MakeCompound(compound);
					for (const auto& shell : largeShells) {
						builder.Add(compound, shell);
					}
					subShapes.push_back(compound);
				}
			}
			
			if (!smallShells.empty()) {
				if (smallShells.size() == 1) {
					subShapes.push_back(smallShells[0]);
				} else {
					TopoDS_Compound compound;
					BRep_Builder builder;
					builder.MakeCompound(compound);
					for (const auto& shell : smallShells) {
						builder.Add(compound, shell);
					}
					subShapes.push_back(compound);
				}
			}
		} else {
			// Many shells - group by face count (complex vs simple shells)
			LOG_INF_S("Many shells (" + std::to_string(allShells.size()) + "), grouping by complexity");
			
			std::vector<TopoDS_Shell> complexShells, simpleShells;
			
			for (const auto& shell : allShells) {
				int faceCount = 0;
				for (TopExp_Explorer exp(shell, TopAbs_FACE); exp.More(); exp.Next()) {
					faceCount++;
				}
				
				if (faceCount > 10) { // Complex shell threshold
					complexShells.push_back(shell);
				} else {
					simpleShells.push_back(shell);
				}
			}
			
			LOG_INF_S("Complexity-based grouping: " + std::to_string(complexShells.size()) + " complex shells, " + 
				std::to_string(simpleShells.size()) + " simple shells");
			
			// Create shapes from complexity groups
			if (!complexShells.empty()) {
				if (complexShells.size() == 1) {
					subShapes.push_back(complexShells[0]);
				} else {
					TopoDS_Compound compound;
					BRep_Builder builder;
					builder.MakeCompound(compound);
					for (const auto& shell : complexShells) {
						builder.Add(compound, shell);
					}
					subShapes.push_back(compound);
				}
			}
			
			if (!simpleShells.empty()) {
				if (simpleShells.size() == 1) {
					subShapes.push_back(simpleShells[0]);
				} else {
					TopoDS_Compound compound;
					BRep_Builder builder;
					builder.MakeCompound(compound);
					for (const auto& shell : simpleShells) {
						builder.Add(compound, shell);
					}
					subShapes.push_back(compound);
				}
			}
		}
		
		LOG_INF_S("Shell group decomposition completed: " + std::to_string(subShapes.size()) + " shapes");
	} catch (const std::exception& e) {
		LOG_WRN_S("Shell group decomposition failed: " + std::string(e.what()));
		subShapes.push_back(shape);
	}
}
