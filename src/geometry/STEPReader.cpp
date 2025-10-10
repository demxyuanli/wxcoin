#include "STEPReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"
#include "rendering/GeometryProcessor.h"
#include "config/RenderingConfig.h"

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

// Palette helper for decomposition coloring
static std::vector<Quantity_Color> getPaletteForScheme(GeometryReader::ColorScheme scheme) {
	using CS = GeometryReader::ColorScheme;
	switch (scheme) {
	case CS::WARM_COLORS:
		// High-contrast warm palette (reds/oranges/yellows with large luminance gaps)
		return {
			Quantity_Color(0.90, 0.12, 0.14, Quantity_TOC_RGB), // strong red
			Quantity_Color(1.00, 0.45, 0.00, Quantity_TOC_RGB), // vivid orange
			Quantity_Color(0.99, 0.76, 0.07, Quantity_TOC_RGB), // bright yellow
			Quantity_Color(0.60, 0.00, 0.00, Quantity_TOC_RGB), // dark red
			Quantity_Color(0.95, 0.30, 0.55, Quantity_TOC_RGB), // pink
			Quantity_Color(0.70, 0.35, 0.00, Quantity_TOC_RGB)  // brownish orange
		};
	case CS::RAINBOW:
		// Saturated rainbow with perceptual spacing
		return {
			Quantity_Color(0.90, 0.12, 0.14, Quantity_TOC_RGB), // red
			Quantity_Color(1.00, 0.50, 0.00, Quantity_TOC_RGB), // orange
			Quantity_Color(0.99, 0.76, 0.07, Quantity_TOC_RGB), // yellow
			Quantity_Color(0.20, 0.70, 0.00, Quantity_TOC_RGB), // green
			Quantity_Color(0.00, 0.65, 0.75, Quantity_TOC_RGB), // cyan
			Quantity_Color(0.12, 0.47, 0.71, Quantity_TOC_RGB), // blue
			Quantity_Color(0.42, 0.24, 0.60, Quantity_TOC_RGB)  // purple
		};
	case CS::MONOCHROME_BLUE:
		// Wide-spread blues from dark to light
		return {
			Quantity_Color(0.10, 0.18, 0.30, Quantity_TOC_RGB),
			Quantity_Color(0.12, 0.47, 0.71, Quantity_TOC_RGB),
			Quantity_Color(0.17, 0.63, 0.88, Quantity_TOC_RGB),
			Quantity_Color(0.40, 0.76, 1.00, Quantity_TOC_RGB),
			Quantity_Color(0.70, 0.86, 1.00, Quantity_TOC_RGB)
		};
	case CS::MONOCHROME_GREEN:
		// Wide-spread greens from dark to light
		return {
			Quantity_Color(0.05, 0.30, 0.10, Quantity_TOC_RGB),
			Quantity_Color(0.20, 0.60, 0.20, Quantity_TOC_RGB),
			Quantity_Color(0.33, 0.75, 0.29, Quantity_TOC_RGB),
			Quantity_Color(0.60, 0.85, 0.35, Quantity_TOC_RGB),
			Quantity_Color(0.80, 0.93, 0.60, Quantity_TOC_RGB)
		};
	case CS::MONOCHROME_GRAY:
		// High-contrast grays
		return {
			Quantity_Color(0.15, 0.15, 0.15, Quantity_TOC_RGB),
			Quantity_Color(0.35, 0.35, 0.35, Quantity_TOC_RGB),
			Quantity_Color(0.55, 0.55, 0.55, Quantity_TOC_RGB),
			Quantity_Color(0.75, 0.75, 0.75, Quantity_TOC_RGB),
			Quantity_Color(0.90, 0.90, 0.90, Quantity_TOC_RGB)
		};
	case CS::DISTINCT_COLORS:
	default:
		// High-contrast categorical palette (inspired by Tableau/Tol)
		return {
			Quantity_Color(0.12, 0.47, 0.71, Quantity_TOC_RGB), // blue
			Quantity_Color(1.00, 0.50, 0.05, Quantity_TOC_RGB), // orange
			Quantity_Color(0.17, 0.63, 0.17, Quantity_TOC_RGB), // green
			Quantity_Color(0.84, 0.15, 0.16, Quantity_TOC_RGB), // red
			Quantity_Color(0.58, 0.40, 0.74, Quantity_TOC_RGB), // purple
			Quantity_Color(0.55, 0.34, 0.29, Quantity_TOC_RGB), // brown
			Quantity_Color(0.89, 0.47, 0.76, Quantity_TOC_RGB), // pink
			Quantity_Color(0.50, 0.50, 0.50, Quantity_TOC_RGB), // gray
			Quantity_Color(0.74, 0.74, 0.13, Quantity_TOC_RGB), // olive
			Quantity_Color(0.09, 0.75, 0.81, Quantity_TOC_RGB), // cyan
			Quantity_Color(0.35, 0.31, 0.64, Quantity_TOC_RGB), // indigo
			Quantity_Color(0.95, 0.90, 0.25, Quantity_TOC_RGB)  // bright yellow
		};
	}
}

static std::vector<TopoDS_Shape> decomposeByLevelUsingTopo(const TopoDS_Shape& shape, GeometryReader::DecompositionLevel level) {
	std::vector<TopoDS_Shape> out;
	if (shape.IsNull()) return out;
	using DL = GeometryReader::DecompositionLevel;
	if (level == DL::NO_DECOMPOSITION || level == DL::SHAPE_LEVEL) {
		out.push_back(shape);
		return out;
	}
	TopAbs_ShapeEnum target = TopAbs_SHAPE;
	switch (level) {
	case DL::SOLID_LEVEL: target = TopAbs_SOLID; break;
	case DL::SHELL_LEVEL: target = TopAbs_SHELL; break;
	case DL::FACE_LEVEL: target = TopAbs_FACE; break;
	default: target = TopAbs_SHAPE; break;
	}
	for (TopExp_Explorer exp(shape, target); exp.More(); exp.Next()) {
		out.push_back(exp.Current());
	}
	if (out.empty()) out.push_back(shape);
	return out;
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
		// Try CAF reader first, but use it only if it contains useful color/material info
		bool cafSuccess = false;
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
			
			ReadResult cafResult = readSTEPFileWithCAF(filePath, options, progress);
			
			if (cafResult.success && !cafResult.geometries.empty()) {
				// Check if CAF contains useful color/material information
				bool hasColors = hasValidColorInfo(cafResult.geometries);
				
				if (hasColors) {
					// Use CAF results - they contain valuable color information
					result.geometries = cafResult.geometries;
					result.entityMetadata = cafResult.entityMetadata;
					result.assemblyStructure = cafResult.assemblyStructure;
					cafSuccess = true;
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
					LOG_INF_S("Found " + std::to_string(coloredCount) + " components with custom colors");
				} else {
					LOG_INF_S("CAF reader returned only default colors - falling back to standard reader with decomposition");
				}
			} else {
				LOG_WRN_S("CAF reader failed: " + (cafResult.errorMessage.empty() ? "Unknown error" : cafResult.errorMessage));
				LOG_WRN_S("Falling back to standard reader with decomposition");
			}
		} catch (const std::exception& e) {
			LOG_WRN_S("CAF reader exception: " + std::string(e.what()));
			LOG_INF_S("Falling back to standard reader with decomposition");
		}

		// Convert to geometry objects with simplified processing (fallback if CAF failed)
		if (!cafSuccess) {
			std::string baseName = std::filesystem::path(filePath).stem().string();
			
			// Log that we're using decomposition settings
			if (options.decomposition.enableDecomposition) {
				LOG_INF_S("Using standard reader with decomposition (level: " + 
						 std::to_string(static_cast<int>(options.decomposition.level)) + 
						 ", color scheme: " + std::to_string(static_cast<int>(options.decomposition.colorScheme)) + ")");
			} else {
				LOG_INF_S("Using standard reader without decomposition");
			}
			
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
		std::vector<TopoDS_Shape> shapes;

		// Check if decomposition is enabled
		if (options.decomposition.enableDecomposition) {
			// Use decomposition logic
			shapes = decomposeByLevelUsingTopo(shape, options.decomposition.level);

			// Apply heuristic component detection when decomposition yields a single part
			if (shapes.size() == 1) {
				std::vector<TopoDS_Shape> heuristics;
				// 1) Try shell grouping (multi-shell solids)
				decomposeByShellGroups(shape, heuristics);
				if (heuristics.size() <= 1) {
					// 2) FreeCAD-like strategy (solids>shells>features)
					heuristics.clear();
					decomposeShapeFreeCADLike(shape, heuristics);
				}
				if (heuristics.size() <= 1) {
					// 3) Geometric features grouping (planes/cylinders etc.)
					heuristics.clear();
					decomposeByGeometricFeatures(shape, heuristics);
				}
				if (heuristics.size() <= 1) {
					// 4) Connectivity-based grouping (last resort)
					heuristics.clear();
					decomposeByConnectivity(shape, heuristics);
				}
				if (heuristics.size() > 1) {
					shapes = std::move(heuristics);
				}
			}

			LOG_INF_S("Decomposed shape into " + std::to_string(shapes.size()) + " components using level: " +
				std::to_string(static_cast<int>(options.decomposition.level)));
		} else {
			// No decomposition - extract individual shapes as before
			extractShapes(shape, shapes);
		}

		LOG_INF_S("Converting " + std::to_string(shapes.size()) + " shapes to geometries for: " + baseName);

		// Get color palette for the selected scheme
		auto palette = getPaletteForScheme(options.decomposition.colorScheme);
		std::hash<std::string> hasher;
		size_t colorIndex = 0;

		// Sequential processing with progress (simplified from parallel)
		size_t total = shapes.size();
		size_t successCount = 0;
		size_t failCount = 0;

		for (size_t i = 0; i < shapes.size(); ++i) {
			if (!shapes[i].IsNull()) {
				std::string name = baseName + "_" + std::to_string(i);
				auto geometry = processSingleShape(shapes[i], name, baseName, options, palette, hasher, colorIndex);
				if (geometry) {
					LOG_INF_S("STEPReader: Created geometry '" + name + "' with filename '" + geometry->getFileName() + "'");
					geometries.push_back(geometry);
					successCount++;
					colorIndex++;
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
	const std::string& baseName,
	const OptimizationOptions& options)
{
	// Get color palette for the selected scheme
	auto palette = getPaletteForScheme(options.decomposition.colorScheme);
	std::hash<std::string> hasher;

	// Use sequential coloring if decomposition is enabled, otherwise use hash-based coloring
	size_t colorIndex = 0;
	if (options.decomposition.enableDecomposition && options.decomposition.useConsistentColoring) {
		// Use hash-based consistent coloring for decomposed components
		colorIndex = hasher(name) % palette.size();
	} else {
		// Use sequential coloring from the palette
		static size_t globalColorIndex = 0;
		colorIndex = globalColorIndex++ % palette.size();
	}

	return processSingleShape(shape, name, baseName, options, palette, hasher, colorIndex);
}

std::shared_ptr<OCCGeometry> STEPReader::processSingleShape(
	const TopoDS_Shape& shape,
	const std::string& name,
	const std::string& baseName,
	const OptimizationOptions& options,
	const std::vector<Quantity_Color>& palette,
	const std::hash<std::string>& hasher,
	size_t colorIndex)
{
	if (shape.IsNull()) {
		LOG_WRN_S("Skipping null shape for: " + name);
		return nullptr;
	}

	try {
		// Use OCCT raw shape without active fixing (simplified approach)
		auto geometry = std::make_shared<OCCGeometry>(name);
		geometry->setShape(shape);
		geometry->setFileName(baseName);

		// Set color based on the provided palette and index
		Quantity_Color componentColor = palette[colorIndex % palette.size()];
		geometry->setColor(componentColor);

		// Detect if this is a shell model and apply appropriate settings
		bool isShellModel = detectShellModel(shape);
		if (isShellModel) {
			LOG_INF_S("Detected shell model for: " + name + " - applying shell-specific rendering settings");
			// For shell models, disable backface culling to ensure all faces are visible from both sides
			geometry->setCullFace(false);
			// Shell models should be opaque for better visibility
			geometry->setTransparency(0.0);
			// Enable depth testing but ensure proper depth write for shells
			geometry->setDepthTest(true);
			geometry->setDepthWrite(true);
			// Set enhanced material properties for shell models with better contrast
			// Use the component color for ambient and diffuse, but adjust intensity for better shell rendering
			Standard_Real r, g, b;
			componentColor.Values(r, g, b, Quantity_TOC_RGB);
			geometry->setMaterialAmbientColor(Quantity_Color(r * 0.3, g * 0.3, b * 0.3, Quantity_TOC_RGB));
			geometry->setMaterialDiffuseColor(Quantity_Color(r * 0.8, g * 0.8, b * 0.8, Quantity_TOC_RGB));
			geometry->setMaterialShininess(50.0);
			// Enable smooth normals for better shell rendering
			geometry->setSmoothNormals(true);
		} else {
			// Regular solid models use standard settings
			geometry->setTransparency(0.0);
		}

		// Only analyze shape if explicitly enabled (disabled by default for speed)
		if (options.enableShapeAnalysis) {
			OCCShapeBuilder::analyzeShapeTopology(shape, name);
		}

		// Build face index mapping to enable face picking for all geometries
		MeshParameters meshParams;
		meshParams.deflection = 0.001;  // High quality mesh for face mapping
		meshParams.angularDeflection = 0.5;
		meshParams.relative = true;
		meshParams.inParallel = true;

		LOG_INF_S("Building face index mapping for geometry: " + name);
		geometry->buildFaceIndexMapping(meshParams);

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

		// Traverse assembly tree and build components with proper locations
		std::string baseName = std::filesystem::path(filePath).stem().string();
		auto palette = getPaletteForScheme(options.decomposition.colorScheme);
		std::hash<std::string> hasher;
		int componentIndex = 0;

		auto makeColorForName = [&](const std::string& name, const Quantity_Color* cafColor) -> Quantity_Color {
			if (options.decomposition.useConsistentColoring) {
				size_t idx = hasher(name) % palette.size();
				return palette[idx];
			}
			if (cafColor) return *cafColor;
			return palette[componentIndex % palette.size()];
		};

		std::function<void(const TDF_Label&, const TopLoc_Location&, int)> processLabel =
			[&](const TDF_Label& label, const TopLoc_Location& parentLoc, int level) {
				TopLoc_Location ownLoc = shapeTool->GetLocation(label);
				TopLoc_Location globLoc = parentLoc * ownLoc;

				// Debug: Log label information
				LOG_INF_S("Processing label at level " + std::to_string(level) + 
					" - IsAssembly: " + std::string(shapeTool->IsAssembly(label) ? "true" : "false") +
					", IsShape: " + std::string(shapeTool->IsShape(label) ? "true" : "false") +
					", IsReference: " + std::string(shapeTool->IsReference(label) ? "true" : "false"));

				if (shapeTool->IsAssembly(label)) {
					TDF_LabelSequence children;
					shapeTool->GetComponents(label, children);
					LOG_INF_S("Found assembly with " + std::to_string(children.Length()) + " components");
					for (int k = 1; k <= children.Length(); ++k) {
						processLabel(children.Value(k), globLoc, level + 1);
					}
					return;
				}

				if (!shapeTool->IsShape(label)) {
					LOG_INF_S("Label is not a shape, skipping");
					return;
				}

				// Resolve referenced shape (instance) and compose full location
				TDF_Label srcLabel = label;
				TopLoc_Location srcLoc; // identity by default
				if (shapeTool->IsReference(label)) {
					TDF_Label referred;
					if (shapeTool->GetReferredShape(label, referred)) {
						srcLabel = referred;
						srcLoc = shapeTool->GetLocation(srcLabel);
					}
				}

				TopoDS_Shape shape = shapeTool->GetShape(srcLabel);
				if (shape.IsNull()) {
					LOG_INF_S("Shape is null, skipping");
					return;
				}
				TopLoc_Location finalLoc = globLoc * srcLoc;
				TopoDS_Shape located = finalLoc.IsIdentity() ? shape : shape.Moved(finalLoc);

				std::string compName = baseName + "_Component_" + std::to_string(componentIndex);
				LOG_INF_S("Processing shape: " + compName + ", ShapeType: " + std::to_string(located.ShapeType()) + 
					" (TopAbs_COMPOUND=" + std::to_string(TopAbs_COMPOUND) + ")");
				Handle(TDataStd_Name) nameAttr;
				if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr)) {
					TCollection_ExtendedString extStr = nameAttr->Get();
					std::string converted = safeConvertExtendedString(extStr);
					if (!converted.empty() && converted != "UnnamedComponent") compName = converted;
				}
				// Fallback: try name on referenced/origin label
				if (compName == baseName + "_Component_" + std::to_string(componentIndex)) {
					Handle(TDataStd_Name) refNameAttr;
					if (srcLabel.FindAttribute(TDataStd_Name::GetID(), refNameAttr)) {
						TCollection_ExtendedString extStr = refNameAttr->Get();
						std::string converted = safeConvertExtendedString(extStr);
						if (!converted.empty() && converted != "UnnamedComponent") compName = converted;
					}
				}

				Quantity_Color cafColor;
				bool hasCafColor = false;
				if (!colorTool.IsNull()) {
					hasCafColor = colorTool->GetColor(label, XCAFDoc_ColorGen, cafColor) ||
								   colorTool->GetColor(label, XCAFDoc_ColorSurf, cafColor) ||
								   colorTool->GetColor(label, XCAFDoc_ColorCurv, cafColor);
					// Fallback: try color on referenced/origin label
					if (!hasCafColor) {
						hasCafColor = colorTool->GetColor(srcLabel, XCAFDoc_ColorGen, cafColor) ||
									   colorTool->GetColor(srcLabel, XCAFDoc_ColorSurf, cafColor) ||
									   colorTool->GetColor(srcLabel, XCAFDoc_ColorCurv, cafColor);
					}
				}

				// Check if this is a compound shape that might represent an assembly
				std::vector<TopoDS_Shape> parts;
				LOG_INF_S("Checking shape type: " + std::to_string(located.ShapeType()) + " (TopAbs_COMPOUND=" + std::to_string(TopAbs_COMPOUND) + ", TopAbs_SOLID=" + std::to_string(TopAbs_SOLID) + ")");
				
				// For assembly detection, we need to check if this shape contains multiple sub-components
				// regardless of whether it's a compound or a solid with multiple parts
				if (located.ShapeType() == TopAbs_COMPOUND) {
					LOG_INF_S("Detected compound shape - treating as potential assembly");
					// For compounds, try to extract individual solids/shells
					for (TopExp_Explorer exp(located, TopAbs_SOLID); exp.More(); exp.Next()) {
						parts.push_back(exp.Current());
					}
					// If no solids, try shells
					if (parts.empty()) {
						for (TopExp_Explorer exp(located, TopAbs_SHELL); exp.More(); exp.Next()) {
							parts.push_back(exp.Current());
						}
					}
					LOG_INF_S("Compound decomposition found " + std::to_string(parts.size()) + " parts");
				} else if (located.ShapeType() == TopAbs_SOLID) {
					// For solids, check if they contain multiple sub-components that could be assembly parts
					LOG_INF_S("Detected solid shape - checking for multiple sub-components");
					
					// Count shells within this solid
					int shellCount = 0;
					for (TopExp_Explorer exp(located, TopAbs_SHELL); exp.More(); exp.Next()) {
						shellCount++;
					}
					
					// If solid has multiple shells, treat each shell as a potential component
					if (shellCount > 1) {
						LOG_INF_S("Solid contains " + std::to_string(shellCount) + " shells - treating as assembly");
						for (TopExp_Explorer exp(located, TopAbs_SHELL); exp.More(); exp.Next()) {
							parts.push_back(exp.Current());
						}
					} else {
						// Single shell solid - use normal decomposition
						LOG_INF_S("Single shell solid - using normal decomposition");
						parts = decomposeByLevelUsingTopo(located, options.decomposition.level);
					}
				} else {
					// Use normal decomposition for other shape types
					parts = decomposeByLevelUsingTopo(located, options.decomposition.level);
				}

				// Apply user-configured decomposition options
				if (options.decomposition.enableDecomposition && parts.size() == 1) {
					LOG_INF_S("Decomposition enabled - applying user-configured decomposition level: " + 
						std::to_string(static_cast<int>(options.decomposition.level)));
					
					std::vector<TopoDS_Shape> heuristics;
					
					// Apply decomposition based on user-selected level
					switch (options.decomposition.level) {
						case GeometryReader::DecompositionLevel::NO_DECOMPOSITION:
							LOG_INF_S("No decomposition requested - keeping single part");
							break;
							
						case GeometryReader::DecompositionLevel::SHAPE_LEVEL:
							// Try to decompose into individual shapes
							decomposeByShellGroups(located, heuristics);
							if (heuristics.size() <= 1) {
								heuristics.clear();
								decomposeShapeFreeCADLike(located, heuristics);
							}
							break;
							
						case GeometryReader::DecompositionLevel::SOLID_LEVEL:
							// Decompose into individual solids
							decomposeShapeFreeCADLike(located, heuristics);
							if (heuristics.size() <= 1) {
								heuristics.clear();
								decomposeByGeometricFeatures(located, heuristics);
							}
							break;
							
						case GeometryReader::DecompositionLevel::SHELL_LEVEL:
							// Decompose into individual shells
							decomposeByShellGroups(located, heuristics);
							if (heuristics.size() <= 1) {
								heuristics.clear();
								decomposeByGeometricFeatures(located, heuristics);
							}
							break;
							
						case GeometryReader::DecompositionLevel::FACE_LEVEL:
							// Decompose into individual faces (most detailed)
							decomposeByFaceGroups(located, heuristics);
							if (heuristics.size() <= 1) {
								heuristics.clear();
								decomposeByConnectivity(located, heuristics);
							}
							// Try intelligent decomposition if still single component
							if (heuristics.size() <= 1) {
								heuristics.clear();
								decomposeByFeatureRecognition(located, heuristics);
								if (heuristics.size() <= 1) {
									heuristics.clear();
									decomposeByAdjacentFacesClustering(located, heuristics);
								}
							}
							break;
					}
					
					if (heuristics.size() > 1) {
						parts = std::move(heuristics);
						LOG_INF_S("User-configured decomposition found " + std::to_string(parts.size()) + " components");
					} else {
						LOG_INF_S("User-configured decomposition did not find additional components - keeping single part");
					}
				} else if (parts.size() == 1) {
					LOG_INF_S("Decomposition disabled - keeping single part");
				}
				int localIdx = 0;
				LOG_INF_S("Creating " + std::to_string(parts.size()) + " geometry components from shape");
				for (const auto& part : parts) {
					std::string partName = parts.size() > 1 ? (compName + "_Part_" + std::to_string(localIdx)) : compName;
					Quantity_Color color = makeColorForName(partName, hasCafColor ? &cafColor : nullptr);

					LOG_INF_S("Creating geometry: " + partName + " (part " + std::to_string(localIdx) + "/" + std::to_string(parts.size()) + ")");
					auto geom = std::make_shared<OCCGeometry>(partName);
					geom->setShape(part);
					geom->setColor(color);
					geom->setFileName(baseName);
					
					// Detect if this is a shell model and apply appropriate settings
					bool isShellModel = detectShellModel(part);
					if (isShellModel) {
						LOG_INF_S("CAF: Detected shell model for: " + partName + " - applying shell-specific rendering settings");
						// For shell models, disable backface culling to ensure all faces are visible from both sides
						geom->setCullFace(false);
						// Shell models should be opaque for better visibility
						geom->setTransparency(0.0);
						// Enable depth testing but ensure proper depth write for shells
						geom->setDepthTest(true);
						geom->setDepthWrite(true);
						// Set enhanced material properties for shell models with better contrast
						// Use the component color for ambient and diffuse, but adjust intensity for better shell rendering
						Standard_Real r, g, b;
						color.Values(r, g, b, Quantity_TOC_RGB);
						geom->setMaterialAmbientColor(Quantity_Color(r * 0.3, g * 0.3, b * 0.3, Quantity_TOC_RGB));
						geom->setMaterialDiffuseColor(Quantity_Color(r * 0.8, g * 0.8, b * 0.8, Quantity_TOC_RGB));
						geom->setMaterialShininess(50.0);
						// Enable smooth normals for better shell rendering
						geom->setSmoothNormals(true);
					} else {
						// Regular solid models use standard settings
						geom->setTransparency(0.0);
					}
					
				geom->setAssemblyLevel(level);

				// Build face index mapping for all geometries to enable face query
				// This allows face picking to work regardless of decomposition level
				MeshParameters meshParams;
				meshParams.deflection = 0.001;  // High quality mesh for face mapping
				meshParams.angularDeflection = 0.5;
				meshParams.relative = true;
				meshParams.inParallel = true;

				LOG_INF_S("Building face index mapping for geometry: " + partName);
				geom->buildFaceIndexMapping(meshParams);

					result.geometries.push_back(geom);

					STEPEntityInfo info;
					info.name = partName;
					info.type = "COMPONENT";
					info.color = color;
					info.hasColor = true;
					info.entityId = componentIndex;
					info.shapeIndex = componentIndex;
					result.entityMetadata.push_back(info);

					componentIndex++;
					localIdx++;
				}
			};

		for (int i = 1; i <= freeShapes.Length(); ++i) {
			processLabel(freeShapes.Value(i), TopLoc_Location(), 0);
		}

		if (progress) progress(80, "process components");

		// Build assembly structure summary
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

// Helper function to detect if a shape is a shell model
bool STEPReader::detectShellModel(const TopoDS_Shape& shape)
{
	try {
		if (shape.IsNull()) {
			return false;
		}

		// Check shape type - if it's a shell, it's definitely a shell model
		if (shape.ShapeType() == TopAbs_SHELL) {
			LOG_INF_S("Shape is a shell (TopAbs_SHELL)");
			return true;
		}

		// Check if the shape contains shells but no solids
		int solidCount = 0;
		int shellCount = 0;
		int faceCount = 0;
		int openShellCount = 0;

		for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
			solidCount++;
		}
		
		for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
			shellCount++;
			// Check if shell is closed (solid) or open (surface)
			TopoDS_Shell shell = TopoDS::Shell(exp.Current());
			if (!shell.Closed()) {
				openShellCount++;
			}
		}
		
		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
			faceCount++;
		}

		LOG_INF_S("Shape analysis - Solids: " + std::to_string(solidCount) + 
			", Shells: " + std::to_string(shellCount) + 
			", Open shells: " + std::to_string(openShellCount) +
			", Faces: " + std::to_string(faceCount));

		// If we have shells but no solids, it's likely a shell model
		if (shellCount > 0 && solidCount == 0) {
			LOG_INF_S("Detected shell model: has shells but no solids");
			return true;
		}

		// If we have open shells, it's definitely a shell model requiring double-sided rendering
		if (openShellCount > 0) {
			LOG_INF_S("Detected open shell model: has " + std::to_string(openShellCount) + " open shells");
			return true;
		}

		// If we have only faces and no solids/shells, check if it's a surface model
		if (solidCount == 0 && shellCount == 0 && faceCount > 0) {
			LOG_INF_S("Detected surface model: only faces, no solids or shells");
			return true;
		}

		// Additional check: if we have a compound with only shells
		if (shape.ShapeType() == TopAbs_COMPOUND) {
			TopExp_Explorer exp(shape, TopAbs_SOLID);
			if (!exp.More()) { // No solids found
				TopExp_Explorer shellExp(shape, TopAbs_SHELL);
				if (shellExp.More()) { // Has shells
					LOG_INF_S("Detected shell model: compound with shells but no solids");
					return true;
				}
			}
		}

		// Check for thin-walled solids (solids with very thin walls that might need double-sided rendering)
		if (solidCount > 0 && shellCount > 0) {
			// Additional heuristic: if solid has many shells relative to its size, it might be thin-walled
			double shellToSolidRatio = static_cast<double>(shellCount) / static_cast<double>(solidCount);
			if (shellToSolidRatio > 2.0) {
				LOG_INF_S("Detected potentially thin-walled model: shell/solid ratio = " + std::to_string(shellToSolidRatio));
				return true;
			}
		}

		return false;
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Error detecting shell model: " + std::string(e.what()));
		return false;
	}
}

// Feature-based intelligent decomposition (FreeCAD-style) - Optimized
void STEPReader::decomposeByFeatureRecognition(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& components) {
	try {
		LOG_INF_S("Starting optimized feature-based intelligent decomposition");

		std::vector<TopoDS_Face> faces;
		std::vector<Bnd_Box> faceBounds;

		// Extract all faces and pre-compute bounding boxes for spatial optimization
		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
			TopoDS_Face face = TopoDS::Face(exp.Current());
			faces.push_back(face);

			// Pre-compute bounding box for spatial filtering
			Bnd_Box box;
			BRepBndLib::Add(face, box);
			faceBounds.push_back(box);
		}

		if (faces.empty()) {
			LOG_INF_S("No faces found for feature-based decomposition");
			return;
		}

		LOG_INF_S("Analyzing " + std::to_string(faces.size()) + " faces for feature recognition");

		// Parallel feature extraction with spatial pre-filtering
		std::vector<FaceFeature> faceFeatures = extractFaceFeaturesParallel(faces, faceBounds);

		// Optimized clustering with spatial partitioning
		std::vector<std::vector<int>> featureGroups;
		clusterFacesByFeaturesOptimized(faceFeatures, faceBounds, featureGroups);

		LOG_INF_S("Feature-based clustering found " + std::to_string(featureGroups.size()) + " potential components");

		// Create components from face groups with validation
		createComponentsFromGroups(faceFeatures, featureGroups, components);

		// Post-process: merge similar small components
		mergeSmallComponents(components);

		LOG_INF_S("Feature-based decomposition created " + std::to_string(components.size()) + " components");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Feature-based decomposition failed: " + std::string(e.what()));
	}
}

// Adjacent faces clustering decomposition - Optimized
void STEPReader::decomposeByAdjacentFacesClustering(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& components) {
	try {
		LOG_INF_S("Starting optimized adjacent faces clustering decomposition");

		std::vector<TopoDS_Face> faces;
		std::vector<Bnd_Box> faceBounds;

		// Extract all faces and pre-compute bounding boxes
		for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
			TopoDS_Face face = TopoDS::Face(exp.Current());
			faces.push_back(face);

			Bnd_Box box;
			BRepBndLib::Add(face, box);
			faceBounds.push_back(box);
		}

		if (faces.empty()) {
			LOG_INF_S("No faces found for adjacent faces clustering");
			return;
		}

		LOG_INF_S("Analyzing " + std::to_string(faces.size()) + " faces for adjacent clustering");

		// Optimized adjacency graph building with spatial filtering
		std::vector<std::vector<int>> adjacencyGraph(faces.size());
		buildFaceAdjacencyGraphOptimized(faces, faceBounds, adjacencyGraph);

		// Advanced clustering with geometric validation
		std::vector<std::vector<int>> clusters;
		clusterAdjacentFacesOptimized(faces, adjacencyGraph, clusters);

		LOG_INF_S("Adjacent faces clustering found " + std::to_string(clusters.size()) + " clusters");

		// Create validated components from clusters
		createValidatedComponentsFromClusters(faces, clusters, components);

		// Post-process: filter and refine components
		refineComponents(components);

		LOG_INF_S("Adjacent faces clustering created " + std::to_string(components.size()) + " components");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Adjacent faces clustering failed: " + std::string(e.what()));
	}
}

// Helper methods for intelligent decomposition

std::string STEPReader::classifyFaceType(const TopoDS_Face& face) {
	try {
		// Get the underlying surface
		Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
		
		if (surface.IsNull()) {
			return "UNKNOWN";
		}
		
		// Classify surface type
		if (surface->DynamicType() == STANDARD_TYPE(Geom_Plane)) {
			return "PLANE";
		}
		else if (surface->DynamicType() == STANDARD_TYPE(Geom_CylindricalSurface)) {
			return "CYLINDER";
		}
		else if (surface->DynamicType() == STANDARD_TYPE(Geom_SphericalSurface)) {
			return "SPHERE";
		}
		else if (surface->DynamicType() == STANDARD_TYPE(Geom_ConicalSurface)) {
			return "CONE";
		}
		else if (surface->DynamicType() == STANDARD_TYPE(Geom_ToroidalSurface)) {
			return "TORUS";
		}
		else {
			return "SURFACE";
		}
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Failed to classify face type: " + std::string(e.what()));
		return "UNKNOWN";
	}
}

double STEPReader::calculateFaceArea(const TopoDS_Face& face) {
	try {
		GProp_GProps props;
		BRepGProp::SurfaceProperties(face, props);
		return props.Mass();
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Failed to calculate face area: " + std::string(e.what()));
		return 0.0;
	}
}

gp_Pnt STEPReader::calculateFaceCentroid(const TopoDS_Face& face) {
	try {
		GProp_GProps props;
		BRepGProp::SurfaceProperties(face, props);
		return props.CentreOfMass();
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Failed to calculate face centroid: " + std::string(e.what()));
		return gp_Pnt(0, 0, 0);
	}
}

gp_Dir STEPReader::calculateFaceNormal(const TopoDS_Face& face) {
	try {
		Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
		if (surface.IsNull()) {
			return gp_Dir(0, 0, 1);
		}
		
		// Get parameter bounds
		double uMin, uMax, vMin, vMax;
		surface->Bounds(uMin, uMax, vMin, vMax);
		
		// Calculate normal at center of parameter space
		double u = (uMin + uMax) * 0.5;
		double v = (vMin + vMax) * 0.5;
		
		gp_Pnt point = surface->Value(u, v);
		gp_Vec du, dv;
		surface->D1(u, v, point, du, dv);
		
		gp_Vec normal = du.Crossed(dv);
		if (normal.Magnitude() > 1e-12) {
			normal.Normalize();
		}
		
		return gp_Dir(normal);
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Failed to calculate face normal: " + std::string(e.what()));
		return gp_Dir(0, 0, 1);
	}
}

void STEPReader::clusterFacesByFeatures(const std::vector<FaceFeature>& faceFeatures, std::vector<std::vector<int>>& featureGroups) {
	try {
		std::unordered_map<std::string, std::vector<int>> typeGroups;
		
		// Group faces by type
		for (size_t i = 0; i < faceFeatures.size(); ++i) {
			typeGroups[faceFeatures[i].type].push_back(i);
		}
		
		// For each type group, further cluster by geometric similarity
		for (auto& [type, indices] : typeGroups) {
			if (indices.size() <= 1) {
				if (!indices.empty()) {
					featureGroups.push_back({indices[0]});
				}
				continue;
			}
			
			// Cluster by area similarity and centroid proximity
			std::vector<std::vector<int>> subGroups;
			std::vector<bool> assigned(indices.size(), false);
			
			for (size_t i = 0; i < indices.size(); ++i) {
				if (assigned[i]) continue;
				
				std::vector<int> group = {indices[i]};
				assigned[i] = true;
				
				const FaceFeature& refFeature = faceFeatures[indices[i]];
				
				for (size_t j = i + 1; j < indices.size(); ++j) {
					if (assigned[j]) continue;
					
					const FaceFeature& testFeature = faceFeatures[indices[j]];
					
					// Check area similarity (within 20% tolerance)
					double areaRatio = std::min(refFeature.area, testFeature.area) / std::max(refFeature.area, testFeature.area);
					
					// Check centroid distance
					double distance = refFeature.centroid.Distance(testFeature.centroid);
					
					if (areaRatio > 0.8 && distance < 10.0) { // Configurable thresholds
						group.push_back(indices[j]);
						assigned[j] = true;
					}
				}
				
				featureGroups.push_back(group);
			}
		}
		
		LOG_INF_S("Feature clustering created " + std::to_string(featureGroups.size()) + " groups");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Feature clustering failed: " + std::string(e.what()));
	}
}

void STEPReader::buildFaceAdjacencyGraph(const std::vector<TopoDS_Face>& faces, std::vector<std::vector<int>>& adjacencyGraph) {
	try {
		adjacencyGraph.clear();
		adjacencyGraph.resize(faces.size());
		
		// For each pair of faces, check if they share an edge
		for (size_t i = 0; i < faces.size(); ++i) {
			for (size_t j = i + 1; j < faces.size(); ++j) {
				if (areFacesAdjacent(faces[i], faces[j])) {
					adjacencyGraph[i].push_back(j);
					adjacencyGraph[j].push_back(i);
				}
			}
		}
		
		LOG_INF_S("Built adjacency graph for " + std::to_string(faces.size()) + " faces");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Failed to build adjacency graph: " + std::string(e.what()));
	}
}

void STEPReader::clusterAdjacentFaces(const std::vector<TopoDS_Face>& faces, const std::vector<std::vector<int>>& adjacencyGraph, std::vector<std::vector<int>>& clusters) {
	try {
		clusters.clear();
		std::vector<bool> visited(faces.size(), false);
		
		// Use DFS to find connected components
		for (size_t i = 0; i < faces.size(); ++i) {
			if (visited[i]) continue;
			
			std::vector<int> cluster;
			std::stack<int> stack;
			stack.push(i);
			
			while (!stack.empty()) {
				int current = stack.top();
				stack.pop();
				
				if (visited[current]) continue;
				visited[current] = true;
				cluster.push_back(current);
				
				// Add adjacent faces to stack
				for (int adjacent : adjacencyGraph[current]) {
					if (!visited[adjacent]) {
						stack.push(adjacent);
					}
				}
			}
			
			if (cluster.size() >= 3) { // Only keep substantial clusters
				clusters.push_back(cluster);
			}
		}
		
		LOG_INF_S("Adjacent clustering found " + std::to_string(clusters.size()) + " clusters");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Adjacent clustering failed: " + std::string(e.what()));
	}
}

TopoDS_Shape STEPReader::tryCreateSolidFromFaces(const TopoDS_Compound& compound, const std::vector<FaceFeature>& faceFeatures, const std::vector<int>& group) {
	try {
		// Simple approach: try to create a shell from the faces
		BRep_Builder builder;
		TopoDS_Shell shell;
		builder.MakeShell(shell);
		
		for (int faceId : group) {
			builder.Add(shell, faceFeatures[faceId].face);
		}
		
		// Try to close the shell
		ShapeFix_Shell shellFixer;
		shellFixer.Init(shell);
		shellFixer.SetPrecision(1e-6);
		shellFixer.Perform();
		
		TopoDS_Shell closedShell = shellFixer.Shell();
		
		// Try to create solid from closed shell
		BRepBuilderAPI_MakeSolid solidMaker(closedShell);
		if (solidMaker.IsDone()) {
			return solidMaker.Solid();
		}
		
		return closedShell; // Return shell if solid creation fails
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Failed to create solid from faces: " + std::string(e.what()));
		return TopoDS_Shape();
	}
}

TopoDS_Shape STEPReader::tryCreateSolidFromFaceCluster(const TopoDS_Compound& compound, const std::vector<TopoDS_Face>& faces, const std::vector<int>& cluster) {
	try {
		// Simple approach: try to create a shell from the faces
		BRep_Builder builder;
		TopoDS_Shell shell;
		builder.MakeShell(shell);
		
		for (int faceId : cluster) {
			builder.Add(shell, faces[faceId]);
		}
		
		// Try to close the shell
		ShapeFix_Shell shellFixer;
		shellFixer.Init(shell);
		shellFixer.SetPrecision(1e-6);
		shellFixer.Perform();
		
		TopoDS_Shell closedShell = shellFixer.Shell();
		
		// Try to create solid from closed shell
		BRepBuilderAPI_MakeSolid solidMaker(closedShell);
		if (solidMaker.IsDone()) {
			return solidMaker.Solid();
		}
		
		return closedShell; // Return shell if solid creation fails
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Failed to create solid from face cluster: " + std::string(e.what()));
		return TopoDS_Shape();
	}
}

bool STEPReader::areFacesAdjacent(const TopoDS_Face& face1, const TopoDS_Face& face2) {
	try {
		// Check if faces share any edges
		for (TopExp_Explorer exp1(face1, TopAbs_EDGE); exp1.More(); exp1.Next()) {
			TopoDS_Edge edge1 = TopoDS::Edge(exp1.Current());

			for (TopExp_Explorer exp2(face2, TopAbs_EDGE); exp2.More(); exp2.Next()) {
				TopoDS_Edge edge2 = TopoDS::Edge(exp2.Current());

				if (edge1.IsSame(edge2)) {
					return true;
				}
			}
		}

		return false;
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Failed to check face adjacency: " + std::string(e.what()));
		return false;
	}
}

// ===== OPTIMIZED DECOMPOSITION FUNCTIONS =====

// Parallel face feature extraction
std::vector<STEPReader::FaceFeature> STEPReader::extractFaceFeaturesParallel(
	const std::vector<TopoDS_Face>& faces,
	const std::vector<Bnd_Box>& faceBounds)
{
	std::vector<FaceFeature> faceFeatures(faces.size());

	// Parallel feature extraction
	std::for_each(std::execution::par, faces.begin(), faces.end(),
		[&](const TopoDS_Face& face) {
			size_t index = &face - &faces[0]; // Calculate index

			FaceFeature feature;
			feature.face = face;
			feature.id = static_cast<int>(index);
			feature.type = classifyFaceType(face);
			feature.area = calculateFaceArea(face);
			feature.centroid = calculateFaceCentroid(face);
			feature.normal = calculateFaceNormal(face);

			faceFeatures[index] = feature;
		});

	return faceFeatures;
}

// Optimized face clustering with spatial partitioning
void STEPReader::clusterFacesByFeaturesOptimized(
	const std::vector<FaceFeature>& faceFeatures,
	const std::vector<Bnd_Box>& faceBounds,
	std::vector<std::vector<int>>& featureGroups)
{
	try {
		// Calculate global bounding box for spatial partitioning
		Bnd_Box globalBox;
		for (const auto& box : faceBounds) {
			globalBox.Add(box);
		}

		// Create spatial grid for efficient neighbor search
		const int gridSize = 8; // 8x8x8 grid
		std::vector<std::vector<int>> spatialGrid(gridSize * gridSize * gridSize);

		// Assign faces to grid cells
		for (size_t i = 0; i < faceFeatures.size(); ++i) {
			if (faceBounds[i].IsVoid()) continue;

			Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
			faceBounds[i].Get(xMin, yMin, zMin, xMax, yMax, zMax);

			// Calculate grid cell
			Standard_Real globalXMin, globalYMin, globalZMin, globalXMax, globalYMax, globalZMax;
			globalBox.Get(globalXMin, globalYMin, globalZMin, globalXMax, globalYMax, globalZMax);

			int cellX = static_cast<int>((xMin - globalXMin) / (globalXMax - globalXMin) * gridSize);
			int cellY = static_cast<int>((yMin - globalYMin) / (globalYMax - globalYMin) * gridSize);
			int cellZ = static_cast<int>((zMin - globalZMin) / (globalZMax - globalZMin) * gridSize);

			cellX = std::max(0, std::min(gridSize - 1, cellX));
			cellY = std::max(0, std::min(gridSize - 1, cellY));
			cellZ = std::max(0, std::min(gridSize - 1, cellZ));

			int cellIndex = cellX + cellY * gridSize + cellZ * gridSize * gridSize;
			spatialGrid[cellIndex].push_back(static_cast<int>(i));
		}

		// Group faces by type first (fast pre-filtering)
		std::unordered_map<std::string, std::vector<int>> typeGroups;
		for (size_t i = 0; i < faceFeatures.size(); ++i) {
			typeGroups[faceFeatures[i].type].push_back(static_cast<int>(i));
		}

		// For each type group, perform spatial-aware clustering
		for (const auto& [type, indices] : typeGroups) {
			if (indices.size() <= 1) {
				if (!indices.empty()) {
					featureGroups.push_back({indices[0]});
				}
				continue;
			}

			// Spatial-aware clustering within type groups
			std::vector<bool> processed(indices.size(), false);

			for (size_t i = 0; i < indices.size(); ++i) {
				if (processed[i]) continue;

				std::vector<int> group = {indices[i]};
				processed[i] = true;

				const FaceFeature& refFeature = faceFeatures[indices[i]];

				// Find spatially nearby faces for comparison
				std::vector<int> nearbyFaces = findNearbyFaces(indices[i], spatialGrid, faceBounds, gridSize);

				for (int nearbyIdx : nearbyFaces) {
					// Find the position in the indices array
					auto it = std::find(indices.begin(), indices.end(), nearbyIdx);
					if (it == indices.end()) continue;

					size_t localIdx = it - indices.begin();
					if (processed[localIdx]) continue;

					const FaceFeature& testFeature = faceFeatures[nearbyIdx];

					// Enhanced similarity check with multiple criteria
					if (areFeaturesSimilar(refFeature, testFeature, faceBounds[indices[i]], faceBounds[nearbyIdx])) {
						group.push_back(indices[localIdx]);
						processed[localIdx] = true;
					}
				}

				featureGroups.push_back(group);
			}
		}

		LOG_INF_S("Optimized clustering created " + std::to_string(featureGroups.size()) + " groups");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Optimized feature clustering failed: " + std::string(e.what()));
	}
}

// Find faces in nearby grid cells
std::vector<int> STEPReader::findNearbyFaces(
	int faceIndex,
	const std::vector<std::vector<int>>& spatialGrid,
	const std::vector<Bnd_Box>& faceBounds,
	int gridSize)
{
	std::vector<int> nearbyFaces;

	if (faceBounds[faceIndex].IsVoid()) return nearbyFaces;

	Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
	faceBounds[faceIndex].Get(xMin, yMin, zMin, xMax, yMax, zMax);

	// Calculate grid cell for this face
	Bnd_Box globalBox;
	for (const auto& box : faceBounds) {
		globalBox.Add(box);
	}

	Standard_Real globalXMin, globalYMin, globalZMin, globalXMax, globalYMax, globalZMax;
	globalBox.Get(globalXMin, globalYMin, globalZMin, globalXMax, globalYMax, globalZMax);

	int cellX = static_cast<int>((xMin - globalXMin) / (globalXMax - globalXMin) * gridSize);
	int cellY = static_cast<int>((yMin - globalYMin) / (globalYMax - globalYMin) * gridSize);
	int cellZ = static_cast<int>((zMin - globalZMin) / (globalZMax - globalZMin) * gridSize);

	cellX = std::max(0, std::min(gridSize - 1, cellX));
	cellY = std::max(0, std::min(gridSize - 1, cellY));
	cellZ = std::max(0, std::min(gridSize - 1, cellZ));

	// Check neighboring cells (3x3x3 neighborhood)
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dz = -1; dz <= 1; ++dz) {
				int nx = cellX + dx;
				int ny = cellY + dy;
				int nz = cellZ + dz;

				if (nx >= 0 && nx < gridSize && ny >= 0 && ny < gridSize && nz >= 0 && nz < gridSize) {
					int cellIndex = nx + ny * gridSize + nz * gridSize * gridSize;
					for (int faceIdx : spatialGrid[cellIndex]) {
						if (faceIdx != faceIndex) {
							nearbyFaces.push_back(faceIdx);
						}
					}
				}
			}
		}
	}

	return nearbyFaces;
}

// Enhanced feature similarity check
bool STEPReader::areFeaturesSimilar(
	const FaceFeature& f1,
	const FaceFeature& f2,
	const Bnd_Box& b1,
	const Bnd_Box& b2)
{
	// Basic type check
	if (f1.type != f2.type) return false;

	// Area similarity (within 25% tolerance)
	double areaRatio = std::min(f1.area, f2.area) / std::max(f1.area, f2.area);
	if (areaRatio < 0.75) return false;

	// Centroid distance check (relative to bounding box size)
	Standard_Real xMin1, yMin1, zMin1, xMax1, yMax1, zMax1;
	Standard_Real xMin2, yMin2, zMin2, xMax2, yMax2, zMax2;
	b1.Get(xMin1, yMin1, zMin1, xMax1, yMax1, zMax1);
	b2.Get(xMin2, yMin2, zMin2, xMax2, yMax2, zMax2);

	double boxSize1 = std::max({xMax1 - xMin1, yMax1 - yMin1, zMax1 - zMin1});
	double boxSize2 = std::max({xMax2 - xMin2, yMax2 - yMin2, zMax2 - zMin2});
	double avgBoxSize = (boxSize1 + boxSize2) * 0.5;

	double distance = f1.centroid.Distance(f2.centroid);
	if (distance > avgBoxSize * 2.0) return false; // Too far apart

	// Normal similarity (for planes and similar surfaces)
	if (f1.type == "PLANE" || f1.type == "CYLINDER") {
		double dotProduct = f1.normal.Dot(f2.normal);
		if (std::abs(dotProduct) < 0.9) return false; // Not parallel enough
	}

	return true;
}

// Create components from face groups with validation
void STEPReader::createComponentsFromGroups(
	const std::vector<FaceFeature>& faceFeatures,
	const std::vector<std::vector<int>>& featureGroups,
	std::vector<TopoDS_Shape>& components)
{
	for (const auto& group : featureGroups) {
		if (group.size() < 2) continue; // Skip single-face groups

		try {
			TopoDS_Compound compound;
			BRep_Builder builder;
			builder.MakeCompound(compound);

			for (int faceId : group) {
				TopoDS_Shape shapeToAdd = faceFeatures[faceId].face;
				builder.Add(compound, shapeToAdd);
			}

			// Try to create a solid from the face group
			TopoDS_Shape component = tryCreateSolidFromFaces(compound, faceFeatures, group);
			if (!component.IsNull()) {
				components.push_back(component);
			}
		}
		catch (const std::exception& e) {
			LOG_WRN_S("Failed to create component from face group: " + std::string(e.what()));
		}
	}
}

// Merge small similar components
void STEPReader::mergeSmallComponents(std::vector<TopoDS_Shape>& components) {
	try {
		if (components.size() <= 1) return;

		std::vector<TopoDS_Shape> mergedComponents;
		std::vector<bool> merged(components.size(), false);

		// Calculate volumes for size comparison
		std::vector<double> volumes(components.size(), 0.0);
		for (size_t i = 0; i < components.size(); ++i) {
			try {
				GProp_GProps props;
				BRepGProp::VolumeProperties(components[i], props);
				volumes[i] = props.Mass();
			} catch (...) {
				volumes[i] = 0.0;
			}
		}

		// Find global volume statistics
		std::vector<double> validVolumes;
		for (double vol : volumes) {
			if (vol > 1e-12) validVolumes.push_back(vol);
		}

		if (validVolumes.empty()) return;

		std::sort(validVolumes.begin(), validVolumes.end());
		double medianVolume = validVolumes[validVolumes.size() / 2];
		double smallThreshold = medianVolume * 0.01; // 1% of median volume

		for (size_t i = 0; i < components.size(); ++i) {
			if (merged[i]) continue;

			std::vector<TopoDS_Shape> mergeGroup = {components[i]};
			merged[i] = true;

			// Find similar small components to merge
			for (size_t j = i + 1; j < components.size(); ++j) {
				if (merged[j] || volumes[j] > smallThreshold) continue;

				if (areShapesSimilar(components[i], components[j])) {
					mergeGroup.push_back(components[j]);
					merged[j] = true;
				}
			}

			// Create merged component if multiple shapes
			if (mergeGroup.size() > 1) {
				TopoDS_Compound compound;
				BRep_Builder builder;
				builder.MakeCompound(compound);

			for (const auto& shape : mergeGroup) {
				TopoDS_Shape shapeToAdd = shape;
				builder.Add(compound, shapeToAdd);
			}

				mergedComponents.push_back(compound);
			} else {
				mergedComponents.push_back(components[i]);
			}
		}

		components = std::move(mergedComponents);
		LOG_INF_S("Component merging reduced count from " + std::to_string(merged.size()) +
			" to " + std::to_string(components.size()));
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Component merging failed: " + std::string(e.what()));
	}
}

// Optimized adjacency graph building with spatial filtering
void STEPReader::buildFaceAdjacencyGraphOptimized(
	const std::vector<TopoDS_Face>& faces,
	const std::vector<Bnd_Box>& faceBounds,
	std::vector<std::vector<int>>& adjacencyGraph)
{
	try {
		adjacencyGraph.assign(faces.size(), std::vector<int>());

		// Create spatial index for efficient neighbor finding
		const int gridSize = 4; // Smaller grid for adjacency
		std::vector<std::vector<int>> spatialGrid(gridSize * gridSize * gridSize);

		// Build spatial grid
		Bnd_Box globalBox;
		for (const auto& box : faceBounds) {
			globalBox.Add(box);
		}

		for (size_t i = 0; i < faces.size(); ++i) {
			if (faceBounds[i].IsVoid()) continue;

			Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
			faceBounds[i].Get(xMin, yMin, zMin, xMax, yMax, zMax);

			int cellX = static_cast<int>((xMin - globalBox.CornerMin().X()) /
				(globalBox.CornerMax().X() - globalBox.CornerMin().X()) * gridSize);
			int cellY = static_cast<int>((yMin - globalBox.CornerMin().Y()) /
				(globalBox.CornerMax().Y() - globalBox.CornerMin().Y()) * gridSize);
			int cellZ = static_cast<int>((zMin - globalBox.CornerMin().Z()) /
				(globalBox.CornerMax().Z() - globalBox.CornerMin().Z()) * gridSize);

			cellX = std::max(0, std::min(gridSize - 1, cellX));
			cellY = std::max(0, std::min(gridSize - 1, cellY));
			cellZ = std::max(0, std::min(gridSize - 1, cellZ));

			int cellIndex = cellX + cellY * gridSize + cellZ * gridSize * gridSize;
			spatialGrid[cellIndex].push_back(static_cast<int>(i));
		}

		// Build adjacency using spatial filtering
		for (size_t i = 0; i < faces.size(); ++i) {
			std::vector<int> nearbyFaces = findNearbyFaces(static_cast<int>(i), spatialGrid, faceBounds, gridSize);

			// Check adjacency only with nearby faces
			for (int nearbyIdx : nearbyFaces) {
				if (nearbyIdx <= static_cast<int>(i)) continue; // Avoid duplicates

				if (areFacesAdjacent(faces[i], faces[nearbyIdx])) {
					adjacencyGraph[i].push_back(nearbyIdx);
					adjacencyGraph[nearbyIdx].push_back(static_cast<int>(i));
				}
			}
		}

		// Log adjacency statistics
		size_t totalConnections = 0;
		for (const auto& connections : adjacencyGraph) {
			totalConnections += connections.size();
		}

		LOG_INF_S("Built optimized adjacency graph: " + std::to_string(totalConnections / 2) + " connections");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Optimized adjacency graph building failed: " + std::string(e.what()));
	}
}

// Optimized adjacent face clustering
void STEPReader::clusterAdjacentFacesOptimized(
	const std::vector<TopoDS_Face>& faces,
	const std::vector<std::vector<int>>& adjacencyGraph,
	std::vector<std::vector<int>>& clusters)
{
	try {
		clusters.clear();
		std::vector<bool> visited(faces.size(), false);

		// Use DFS to find connected components with size filtering
		for (size_t i = 0; i < faces.size(); ++i) {
			if (visited[i]) continue;

			std::vector<int> cluster;
			std::stack<int> stack;
			stack.push(static_cast<int>(i));

			while (!stack.empty()) {
				int current = stack.top();
				stack.pop();

				if (visited[current]) continue;
				visited[current] = true;
				cluster.push_back(current);

				// Add adjacent faces to stack
				for (int adjacent : adjacencyGraph[current]) {
					if (!visited[adjacent]) {
						stack.push(adjacent);
					}
				}
			}

			// Enhanced filtering: check cluster quality
			if (isValidCluster(cluster, faces)) {
				clusters.push_back(cluster);
			}
		}

		LOG_INF_S("Optimized clustering found " + std::to_string(clusters.size()) + " valid clusters");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Optimized adjacent clustering failed: " + std::string(e.what()));
	}
}

// Validate cluster quality
bool STEPReader::isValidCluster(const std::vector<int>& cluster, const std::vector<TopoDS_Face>& faces) {
	if (cluster.size() < 3) return false;

	try {
		// Check if cluster forms a closed surface (basic check)
		int totalEdges = 0;
		std::set<TopoDS_Edge, EdgeComparator> uniqueEdges;

		for (int faceId : cluster) {
			for (TopExp_Explorer exp(faces[faceId], TopAbs_EDGE); exp.More(); exp.Next()) {
				TopoDS_Edge edge = TopoDS::Edge(exp.Current());
				uniqueEdges.insert(edge);
				totalEdges++;
			}
		}

		// Simple heuristic: cluster should have reasonable edge-to-face ratio
		double edgeToFaceRatio = static_cast<double>(uniqueEdges.size()) / cluster.size();
		if (edgeToFaceRatio < 2.5 || edgeToFaceRatio > 6.0) {
			return false; // Unreasonable topology
		}

		// Check bounding box aspect ratio
		Bnd_Box clusterBox;
		for (int faceId : cluster) {
			BRepBndLib::Add(faces[faceId], clusterBox);
		}

		if (clusterBox.IsVoid()) return false;

		Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
		clusterBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

		double sizeX = xMax - xMin;
		double sizeY = yMax - yMin;
		double sizeZ = zMax - zMin;

		if (sizeX < 1e-6 || sizeY < 1e-6 || sizeZ < 1e-6) {
			return false; // Degenerate cluster
		}

		return true;
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Cluster validation failed: " + std::string(e.what()));
		return false;
	}
}

// Create validated components from clusters
void STEPReader::createValidatedComponentsFromClusters(
	const std::vector<TopoDS_Face>& faces,
	const std::vector<std::vector<int>>& clusters,
	std::vector<TopoDS_Shape>& components)
{
	for (const auto& cluster : clusters) {
		try {
			TopoDS_Compound compound;
			BRep_Builder builder;
			builder.MakeCompound(compound);

			for (int faceId : cluster) {
				TopoDS_Shape shapeToAdd = faces[faceId];
				builder.Add(compound, shapeToAdd);
			}

			// Try to create a solid from the face cluster
			TopoDS_Shape component = tryCreateSolidFromFaceCluster(compound, faces, cluster);
			if (!component.IsNull()) {
				components.push_back(component);
			}
		}
		catch (const std::exception& e) {
			LOG_WRN_S("Failed to create component from validated cluster: " + std::string(e.what()));
		}
	}
}

// Refine and filter components
void STEPReader::refineComponents(std::vector<TopoDS_Shape>& components) {
	try {
		std::vector<TopoDS_Shape> refinedComponents;

		for (const auto& component : components) {
			try {
				// Check component validity
				if (component.IsNull()) continue;

				// Get component properties
				GProp_GProps props;
				BRepGProp::VolumeProperties(component, props);
				double volume = props.Mass();

				// Filter out invalid or too small components
				if (volume > 1e-12) {
					refinedComponents.push_back(component);
				}
			}
			catch (const std::exception& e) {
				LOG_WRN_S("Component refinement failed for one component: " + std::string(e.what()));
			}
		}

		components = std::move(refinedComponents);
		LOG_INF_S("Component refinement kept " + std::to_string(components.size()) + " valid components");
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Component refinement failed: " + std::string(e.what()));
	}
}

// Check if two shapes are similar for merging
bool STEPReader::areShapesSimilar(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2) {
	try {
		// Get bounding boxes
		Bnd_Box box1, box2;
		BRepBndLib::Add(shape1, box1);
		BRepBndLib::Add(shape2, box2);

		if (box1.IsVoid() || box2.IsVoid()) return false;

		// Check size similarity
		Standard_Real x1, y1, z1, X1, Y1, Z1;
		Standard_Real x2, y2, z2, X2, Y2, Z2;

		box1.Get(x1, y1, z1, X1, Y1, Z1);
		box2.Get(x2, y2, z2, X2, Y2, Z2);

		double vol1 = (X1 - x1) * (Y1 - y1) * (Z1 - z1);
		double vol2 = (X2 - x2) * (Y2 - y2) * (Z2 - z2);

		if (vol1 < 1e-12 || vol2 < 1e-12) return false;

		double volRatio = std::min(vol1, vol2) / std::max(vol1, vol2);
		return volRatio > 0.8; // Within 20% volume similarity

	} catch (const std::exception& e) {
		return false;
	}
}
