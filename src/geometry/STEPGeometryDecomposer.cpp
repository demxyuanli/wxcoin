#include "STEPGeometryDecomposer.h"
#include "STEPReaderUtils.h"
#include "logger/Logger.h"
#include <OpenCASCADE/TopoDS_Builder.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include <OpenCASCADE/Geom_Surface.hxx>
#include <OpenCASCADE/Geom_Plane.hxx>
#include <OpenCASCADE/Geom_CylindricalSurface.hxx>
#include <OpenCASCADE/Geom_SphericalSurface.hxx>
#include <OpenCASCADE/Geom_ConicalSurface.hxx>
#include <OpenCASCADE/Geom_ToroidalSurface.hxx>
#include <OpenCASCADE/GProp_GProps.hxx>
#include <OpenCASCADE/BRepGProp.hxx>
#include <OpenCASCADE/BRepBndLib.hxx>
#include <OpenCASCADE/ShapeFix_Shell.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeSolid.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopAbs_ShapeEnum.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Dir.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <algorithm>
#include <set>
#include <stack>
#include <unordered_map>
#include <cmath>
#include <tbb/tbb.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

// Import decomposition functions from STEPReader.cpp
// These will be moved here to create a cleaner separation of concerns

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeShape(
    const TopoDS_Shape& shape,
    const GeometryReader::OptimizationOptions& options)
{
    std::vector<TopoDS_Shape> result;

    if (shape.IsNull()) {
        return result;
    }

    try {
        // Check if decomposition is enabled
        if (options.decomposition.enableDecomposition) {
            // For FACE_LEVEL, force direct face extraction - extract ALL individual faces
            if (options.decomposition.level == GeometryReader::DecompositionLevel::FACE_LEVEL) {
                // Force direct face extraction - extract all individual faces regardless of shape structure
                result.clear();
                
                // Extract all faces from the shape (including nested ones)
                for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                    TopoDS_Face face = TopoDS::Face(exp.Current());
                    if (!face.IsNull()) {
                        result.push_back(face);
                    }
                }
                
                if (result.empty()) {
                    // No faces found - try intelligent face grouping as fallback
                    std::vector<TopoDS_Shape> heuristics;
                    
                    // Try face group decomposition
                    heuristics = decomposeByFaceGroups(shape);
                    if (heuristics.size() <= 1) {
                        heuristics = decomposeByConnectivity(shape);
                    }
                    if (heuristics.size() <= 1) {
                        heuristics = decomposeByAdjacentFacesClustering(shape);
                    }
                    if (heuristics.size() <= 1) {
                        heuristics = decomposeByFeatureRecognition(shape);
                    }
                    
                    // For FACE_LEVEL, we must extract individual faces from any grouped results
                    // (heuristics may return shells/compounds, but we need individual faces)
                    result.clear();
                    for (const auto& heuristicShape : heuristics) {
                        if (heuristicShape.IsNull()) continue;
                        
                        // If it's already a face, add it directly
                        if (heuristicShape.ShapeType() == TopAbs_FACE) {
                            result.push_back(heuristicShape);
                        } else {
                            // Extract all faces from this shape (could be shell, solid, or compound)
                            for (TopExp_Explorer faceExp(heuristicShape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                                TopoDS_Face face = TopoDS::Face(faceExp.Current());
                                if (!face.IsNull()) {
                                    result.push_back(face);
                                }
                            }
                        }
                    }
                    
                    if (result.empty()) {
                        // Last resort: try to extract faces from shells
                        for (TopExp_Explorer shellExp(shape, TopAbs_SHELL); shellExp.More(); shellExp.Next()) {
                            TopoDS_Shell shell = TopoDS::Shell(shellExp.Current());
                            for (TopExp_Explorer faceExp(shell, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                                TopoDS_Face face = TopoDS::Face(faceExp.Current());
                                if (!face.IsNull()) {
                                    result.push_back(face);
                                }
                            }
                        }
                        
                        if (result.empty()) {
                            result.push_back(shape);
                        }
                    }
                }
            } else {
                // For other levels, use standard decomposition logic
                result = decomposeByLevelUsingTopo(shape, options.decomposition.level);

                // Apply heuristic component detection when decomposition yields a single part
                if (result.size() == 1) {
                    std::vector<TopoDS_Shape> heuristics;

                    // Apply decomposition based on user-selected level
                    switch (options.decomposition.level) {
                        case GeometryReader::DecompositionLevel::NO_DECOMPOSITION:
                            break;

                        case GeometryReader::DecompositionLevel::SHAPE_LEVEL:
                            // SHAPE_LEVEL: Try to extract all top-level shapes (solids, shells, etc.)
                            // First try FreeCAD-like decomposition which extracts all meaningful shapes
                            heuristics = decomposeShapeFreeCADLike(shape);
                            if (heuristics.size() <= 1) {
                                // If that fails, try feature recognition
                                heuristics = decomposeByFeatureRecognition(shape);
                            }
                            if (heuristics.size() <= 1) {
                                // Last resort: try shell groups
                                heuristics = decomposeByShellGroups(shape);
                            }
                            break;

                        case GeometryReader::DecompositionLevel::SOLID_LEVEL:
                            heuristics = decomposeByFeatureRecognition(shape);
                            if (heuristics.size() <= 1) {
                                heuristics = decomposeByGeometricFeatures(shape);
                            }
                            break;

                        case GeometryReader::DecompositionLevel::SHELL_LEVEL:
                            // SHELL_LEVEL: Directly extract all shells
                            // First use direct shell extraction via decomposeByLevelUsingTopo
                            heuristics = decomposeByLevelUsingTopo(shape, GeometryReader::DecompositionLevel::SHELL_LEVEL);
                            if (heuristics.size() <= 1) {
                                // If direct extraction fails, try shell groups
                                heuristics = decomposeByShellGroups(shape);
                            }
                            if (heuristics.size() <= 1) {
                                // Last resort: try geometric features
                                heuristics = decomposeByGeometricFeatures(shape);
                            }
                            break;

                        default:
                            break;
                    }

                    if (heuristics.size() > 1) {
                        result = std::move(heuristics);
                    }
                }
            }

        } else {
            // No decomposition - use original shape
            result.push_back(shape);
        }
    }
    catch (const std::exception& e) {
        result.push_back(shape); // Fallback to original shape
    }

    return result;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeByLevelUsingTopo(
    const TopoDS_Shape& shape,
    GeometryReader::DecompositionLevel level)
{
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

// Helper functions for face analysis
bool STEPGeometryDecomposer::areFacesConnected(const TopoDS_Face& face1, const TopoDS_Face& face2) {
    try {
        // Optimized connectivity check using direct edge comparison
        // Collect edges from both faces and check for shared edges

        std::vector<TopoDS_Edge> edges1;
        edges1.reserve(6); // Typical face has 3-6 edges

        // Collect edges from first face
        for (TopExp_Explorer exp(face1, TopAbs_EDGE); exp.More(); exp.Next()) {
            edges1.push_back(TopoDS::Edge(exp.Current()));
        }

        // Check if any edge from face2 matches any edge from face1
        for (TopExp_Explorer exp2(face2, TopAbs_EDGE); exp2.More(); exp2.Next()) {
            const TopoDS_Edge& edge2 = TopoDS::Edge(exp2.Current());

            for (const auto& edge1 : edges1) {
                if (edge1.IsSame(edge2)) {
                    return true;
                }
            }
        }

        return false;
    } catch (const std::exception& e) {
        return false;
    }
}

bool STEPGeometryDecomposer::areFacesSimilar(const TopoDS_Face& face1, const TopoDS_Face& face2) {
    try {
        // Get surface types
        Handle(Geom_Surface) surf1 = BRep_Tool::Surface(face1);
        Handle(Geom_Surface) surf2 = BRep_Tool::Surface(face2);

        if (surf1.IsNull() || surf2.IsNull()) {
            return false;
        }

        std::string type1 = surf1->DynamicType()->Name();
        std::string type2 = surf2->DynamicType()->Name();

        // Check if surfaces are of the same type
        if (surf1->DynamicType() != surf2->DynamicType()) {
            return false;
        }

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
                return isParallel;
            }
        }

        // For other surface types, consider them similar if they're the same type
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string STEPGeometryDecomposer::classifyFaceType(const TopoDS_Face& face) {
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
        return "UNKNOWN";
    }
}

double STEPGeometryDecomposer::calculateFaceArea(const TopoDS_Face& face) {
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

gp_Pnt STEPGeometryDecomposer::calculateFaceCentroid(const TopoDS_Face& face) {
    try {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        return props.CentreOfMass();
    }
    catch (const std::exception& e) {
        return gp_Pnt(0, 0, 0);
    }
}

gp_Dir STEPGeometryDecomposer::calculateFaceNormal(const TopoDS_Face& face) {
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
        return gp_Dir(0, 0, 1);
    }
}

bool STEPGeometryDecomposer::areFacesAdjacent(const TopoDS_Face& face1, const TopoDS_Face& face2) {
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
        return false;
    }
}

// Implementation of decomposition functions migrated from STEPReader.cpp

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeByFaceGroups(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> subShapes;

    try {
        // Group faces by geometric properties
        std::vector<std::vector<TopoDS_Face>> faceGroups;
        std::vector<TopoDS_Face> currentGroup;

        // Collect all faces
        std::vector<TopoDS_Face> allFaces;
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            allFaces.push_back(TopoDS::Face(exp.Current()));
        }

        if (allFaces.empty()) {
            return subShapes;
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
                    addedToExistingGroup = true;
                    break;
                }
            }

            // If not added to existing group, start a new group
            if (!addedToExistingGroup) {
                if (currentGroup.empty()) {
                    currentGroup.push_back(face);
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
                    } else {
                        // Start a new group
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
            faceGroups.push_back(currentGroup);
        }

        // Convert face groups to shapes
        for (const auto& group : faceGroups) {
            if (group.size() > 0) {

                // Create a compound from the face group
                TopoDS_Compound compound;
                BRep_Builder builder;
                builder.MakeCompound(compound);

                for (const auto& face : group) {
                    builder.Add(compound, face);
                }

                subShapes.push_back(compound);
            }
        }
    } catch (const std::exception& e) {
        LOG_WRN_S("Face group decomposition failed: " + std::string(e.what()));
        subShapes.push_back(shape); // Fallback
    }

    return subShapes;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeShapeFreeCADLike(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> subShapes;
    subShapes.clear();

    try {

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
            for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
                subShapes.push_back(exp.Current());
            }
        } else if (solidCount == 1 && shellCount > 1) {
            // Single solid with multiple shells - try to group shells into logical bodies
            auto shellGroups = decomposeByShellGroups(shape);
            subShapes.insert(subShapes.end(), shellGroups.begin(), shellGroups.end());
        } else if (solidCount == 1 && shellCount == 1 && faceCount > 20) {
            // Single solid/shell with many faces - try to group by geometric features
            auto geometricFeatures = decomposeByGeometricFeatures(shape);
            subShapes.insert(subShapes.end(), geometricFeatures.begin(), geometricFeatures.end());
        } else {
            // Single shape - use as is (no decomposition needed)
            subShapes.push_back(shape);
        }

    } catch (const std::exception& e) {
        LOG_WRN_S("Failed FreeCAD-like decomposition: " + std::string(e.what()));
        subShapes.push_back(shape);
    }

    return subShapes;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeByConnectivity(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> subShapes;

    try {
        // Collect all faces
        std::vector<TopoDS_Face> allFaces;
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            allFaces.push_back(TopoDS::Face(exp.Current()));
        }

        // Cache string conversion to avoid repeated allocations
        std::string facesCountStr = std::to_string(allFaces.size());

        if (allFaces.empty()) {
            subShapes.push_back(shape);
            return subShapes;
        }

        // Group faces by connectivity using optimized approach
        // Pre-allocate and reserve memory for better performance
        std::vector<std::vector<TopoDS_Face>> faceGroups;
        faceGroups.reserve(allFaces.size() / 4); // Estimate initial capacity

        std::vector<bool> processed(allFaces.size(), false);

        // Use a more efficient flood-fill approach
        for (size_t i = 0; i < allFaces.size(); i++) {
            if (processed[i]) continue;

            std::vector<TopoDS_Face> currentGroup;
            currentGroup.reserve(allFaces.size() / 8); // Estimate group size

            // Use queue for breadth-first search to find connected components
            std::vector<size_t> queue;
            queue.reserve(allFaces.size() / 4);
            queue.push_back(i);
            processed[i] = true;

            while (!queue.empty()) {
                size_t currentIdx = queue.back();
                queue.pop_back();
                const TopoDS_Face& currentFace = allFaces[currentIdx];
                currentGroup.push_back(currentFace);

                // Check remaining unprocessed faces for connections
                for (size_t j = i + 1; j < allFaces.size(); j++) { // Start from i+1 to avoid rechecking
                    if (processed[j]) continue;

                    // Quick adjacency check using cached edge information
                    if (areFacesConnected(currentFace, allFaces[j])) {
                        queue.push_back(j);
                        processed[j] = true;
                    }
                }
            }

            if (!currentGroup.empty()) {
                faceGroups.push_back(std::move(currentGroup));
            }
        }


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

    } catch (const std::exception& e) {
        LOG_WRN_S("Connectivity decomposition failed: " + std::string(e.what()));
        subShapes.push_back(shape);
    }

    return subShapes;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeByGeometricFeatures(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> subShapes;

    try {
        // Collect all faces
        std::vector<TopoDS_Face> allFaces;
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            allFaces.push_back(TopoDS::Face(exp.Current()));
        }

        if (allFaces.empty()) {
            subShapes.push_back(shape);
            return subShapes;
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
            }
        }


        // Pre-allocate string buffer to avoid repeated allocations
        std::string logBuffer;
        logBuffer.reserve(256);
        for (const auto& group : surfaceTypeGroups) {
            logBuffer = "  " + group.first + ": " + std::to_string(group.second.size()) + " faces";
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
                    }
                }

                for (const auto& planeGroup : planeGroups) {
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
                // Creating shape from group

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

            // Try decomposing by face area (large vs small faces)
            std::vector<TopoDS_Face> largeFaces, smallFaces;
            std::vector<double> faceAreas;
            faceAreas.reserve(allFaces.size());

            double totalArea = 0.0;

            // Pre-calculate all face areas in one pass
            for (const auto& face : allFaces) {
                double area = 0.0;
                try {
                    GProp_GProps props;
                    BRepGProp::SurfaceProperties(face, props);
                    area = props.Mass();
                    totalArea += area;
                } catch (const std::exception& e) {
                    area = 0.0; // Default area for failed calculations
                }
                faceAreas.push_back(area);
            }

            double averageArea = totalArea / allFaces.size();
            std::string avgAreaStr = std::to_string(averageArea);

            // Classify faces based on pre-calculated areas
            for (size_t i = 0; i < allFaces.size(); ++i) {
                if (faceAreas[i] > averageArea * 2.0) {
                    largeFaces.push_back(allFaces[i]);
                } else {
                    smallFaces.push_back(allFaces[i]);
                }
            }


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

    } catch (const std::exception& e) {
        LOG_WRN_S("Geometric feature decomposition failed: " + std::string(e.what()));
        subShapes.push_back(shape);
    }

    return subShapes;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeByShellGroups(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> subShapes;

    try {
        // Collect all shells
        std::vector<TopoDS_Shell> allShells;
        for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
            allShells.push_back(TopoDS::Shell(exp.Current()));
        }

        if (allShells.empty()) {
            subShapes.push_back(shape);
            return subShapes;
        }

        // If we have few shells, try to group them by volume and connectivity
        if (allShells.size() <= 3) {

            // Group shells by volume (large vs small shells)
            std::vector<TopoDS_Shell> largeShells, smallShells;
            double totalVolume = 0.0;

            for (const auto& shell : allShells) {
                try {
                    GProp_GProps props;
                    BRepGProp::VolumeProperties(shell, props);
                    totalVolume += props.Mass();
                } catch (const std::exception& e) {
                }
            }

            double averageVolume = totalVolume / allShells.size();

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

            std::vector<TopoDS_Shell> complexShells, simpleShells;

            // Pre-calculate face counts for all shells to avoid repeated traversals
            std::vector<std::pair<TopoDS_Shell, int>> shellFaceCounts;
            shellFaceCounts.reserve(allShells.size());

            for (const auto& shell : allShells) {
                int faceCount = 0;
                for (TopExp_Explorer exp(shell, TopAbs_FACE); exp.More(); exp.Next()) {
                    faceCount++;
                }
                shellFaceCounts.emplace_back(shell, faceCount);
            }

            // Now classify shells based on pre-calculated face counts
            for (const auto& [shell, faceCount] : shellFaceCounts) {
                if (faceCount > 10) { // Complex shell threshold
                    complexShells.push_back(shell);
                } else {
                    simpleShells.push_back(shell);
                }
            }


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

    } catch (const std::exception& e) {
        LOG_WRN_S("Shell group decomposition failed: " + std::string(e.what()));
        subShapes.push_back(shape);
    }

    return subShapes;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeShape(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> subShapes;
    subShapes.clear();

    try {
        // Try to decompose into solids first (most common case)
        for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
            subShapes.push_back(exp.Current());
        }

        // If no solids found, try shells
        if (subShapes.empty()) {
            for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
                subShapes.push_back(exp.Current());
            }
        }

        // If still no sub-shapes, try to decompose by face groups
        if (subShapes.empty()) {
            auto faceGroupShapes = STEPGeometryDecomposer::decomposeByFaceGroups(shape);
            subShapes.insert(subShapes.end(), faceGroupShapes.begin(), faceGroupShapes.end());
        }

        // If still empty, try faces (for very complex shapes)
        if (subShapes.empty()) {
            for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                subShapes.push_back(exp.Current());
            }
        }

        // If still empty, use the original shape
        if (subShapes.empty()) {
            subShapes.push_back(shape);
        }
    } catch (const std::exception& e) {
        subShapes.push_back(shape);
    }

    return subShapes;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeByFeatureRecognition(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> components;
    try {

        std::vector<TopoDS_Face> faces;
        std::vector<Bnd_Box> faceBounds;

        // Extract all faces and pre-compute bounding boxes for spatial optimization
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            TopoDS_Face face = TopoDS::Face(exp.Current());
            faces.push_back(face);

            // Pre-compute bounding box for spatial filtering
            faceBounds.push_back(STEPReaderUtils::safeCalculateBoundingBox(face));
        }

        if (faces.empty()) {
            components.push_back(shape);
            return components;
        }

        STEPReaderUtils::logCount("Analyzing ", faces.size(), " faces for feature recognition");

        // Parallel feature extraction with spatial pre-filtering
        std::vector<FaceFeature> faceFeatures = extractFaceFeaturesParallel(faces, faceBounds);

        // Optimized clustering with spatial partitioning
        std::vector<std::vector<int>> featureGroups;
        clusterFacesByFeaturesOptimized(faceFeatures, faceBounds, featureGroups);

        STEPReaderUtils::logCount("Feature-based clustering found ", featureGroups.size(), " potential components");

        // Create components from face groups with validation
        createComponentsFromGroups(faceFeatures, featureGroups, components);

        // Post-process: merge similar small components
        mergeSmallComponents(components);

        STEPReaderUtils::logCount("Feature-based decomposition created ", components.size(), " components");
        
        if (components.empty()) {
            components.push_back(shape);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Feature-based decomposition failed: " + std::string(e.what()));
        components.push_back(shape);
    }
    return components;
}

std::vector<TopoDS_Shape> STEPGeometryDecomposer::decomposeByAdjacentFacesClustering(
    const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> components;
    try {

        std::vector<TopoDS_Face> faces;
        std::vector<Bnd_Box> faceBounds;

        // Extract all faces and pre-compute bounding boxes
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            TopoDS_Face face = TopoDS::Face(exp.Current());
            faces.push_back(face);

            faceBounds.push_back(STEPReaderUtils::safeCalculateBoundingBox(face));
        }

        if (faces.empty()) {
            components.push_back(shape);
            return components;
        }


        // Adjacency graph building (optimized with spatial filtering for large datasets)
        std::vector<std::vector<int>> adjacencyGraph(faces.size());
        buildFaceAdjacencyGraphOptimized(faces, faceBounds, adjacencyGraph);

        // Advanced clustering with geometric validation
        std::vector<std::vector<int>> clusters;
        clusterAdjacentFacesOptimized(faces, adjacencyGraph, clusters);


        // Create validated components from clusters
        createValidatedComponentsFromClusters(faces, clusters, components);

        // Post-process: filter and refine components
        refineComponents(components);

        
        if (components.empty()) {
            components.push_back(shape);
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Adjacent faces clustering failed: " + std::string(e.what()));
        components.push_back(shape);
    }
    return components;
}

// ===== OPTIMIZED DECOMPOSITION FUNCTIONS =====

// Parallel face feature extraction using TBB
// Uses parallel processing only when face count exceeds threshold to avoid overhead
std::vector<STEPGeometryDecomposer::FaceFeature> STEPGeometryDecomposer::extractFaceFeaturesParallel(
    const std::vector<TopoDS_Face>& faces,
    const std::vector<Bnd_Box>& faceBounds)
{
    const size_t PARALLEL_THRESHOLD = 100; // Use parallel processing for 100+ faces
    std::vector<FaceFeature> faceFeatures(faces.size());

    if (faces.size() >= PARALLEL_THRESHOLD) {
        // TBB parallel processing for large datasets (better load balancing than std::execution::par)
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, faces.size()),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i < range.end(); ++i) {
                    FaceFeature feature;
                    feature.face = faces[i];
                    feature.id = static_cast<int>(i);
                    feature.type = classifyFaceType(faces[i]);
                    feature.area = calculateFaceArea(faces[i]);
                    feature.centroid = calculateFaceCentroid(faces[i]);
                    feature.normal = calculateFaceNormal(faces[i]);

                    faceFeatures[i] = feature;
                }
            },
            tbb::auto_partitioner() // Let TBB choose optimal partition size
        );
    } else {
        // Sequential processing for small datasets (avoids thread overhead)
        for (size_t i = 0; i < faces.size(); ++i) {
            FaceFeature feature;
            feature.face = faces[i];
            feature.id = static_cast<int>(i);
            feature.type = classifyFaceType(faces[i]);
            feature.area = calculateFaceArea(faces[i]);
            feature.centroid = calculateFaceCentroid(faces[i]);
            feature.normal = calculateFaceNormal(faces[i]);

            faceFeatures[i] = feature;
        }
    }

    return faceFeatures;
}

// Optimized face clustering with spatial partitioning
void STEPGeometryDecomposer::clusterFacesByFeaturesOptimized(
    const std::vector<FaceFeature>& faceFeatures,
    const std::vector<Bnd_Box>& faceBounds,
    std::vector<std::vector<int>>& featureGroups)
{
    try {
        Bnd_Box globalBox;
        for (const auto& box : faceBounds) {
            globalBox.Add(box);
        }

        const int gridSize = 8;
        std::vector<std::vector<int>> spatialGrid(gridSize * gridSize * gridSize);

        for (size_t i = 0; i < faceFeatures.size(); ++i) {
            if (faceBounds[i].IsVoid()) continue;

            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            faceBounds[i].Get(xMin, yMin, zMin, xMax, yMax, zMax);

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

        std::unordered_map<std::string, std::vector<int>> typeGroups;
        for (size_t i = 0; i < faceFeatures.size(); ++i) {
            typeGroups[faceFeatures[i].type].push_back(static_cast<int>(i));
        }

        for (const auto& [type, indices] : typeGroups) {
            if (indices.size() <= 1) {
                if (!indices.empty()) {
                    featureGroups.push_back({indices[0]});
                }
                continue;
            }

            std::vector<bool> processed(indices.size(), false);

            for (size_t i = 0; i < indices.size(); ++i) {
                if (processed[i]) continue;

                std::vector<int> group = {indices[i]};
                processed[i] = true;

                const FaceFeature& refFeature = faceFeatures[indices[i]];

                std::vector<int> nearbyFaces = findNearbyFaces(indices[i], spatialGrid, faceBounds, gridSize);

                for (int nearbyIdx : nearbyFaces) {
                    auto it = std::find(indices.begin(), indices.end(), nearbyIdx);
                    if (it == indices.end()) continue;

                    size_t localIdx = it - indices.begin();
                    if (processed[localIdx]) continue;

                    const FaceFeature& testFeature = faceFeatures[nearbyIdx];

                    if (areFeaturesSimilar(refFeature, testFeature, faceBounds[indices[i]], faceBounds[nearbyIdx])) {
                        group.push_back(indices[localIdx]);
                        processed[localIdx] = true;
                    }
                }

                featureGroups.push_back(group);
            }
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Optimized feature clustering failed: " + std::string(e.what()));
    }
}

// Find faces in nearby grid cells
std::vector<int> STEPGeometryDecomposer::findNearbyFaces(
    int faceIndex,
    const std::vector<std::vector<int>>& spatialGrid,
    const std::vector<Bnd_Box>& faceBounds,
    int gridSize)
{
    std::vector<int> nearbyFaces;

    if (faceBounds[faceIndex].IsVoid()) return nearbyFaces;

    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    faceBounds[faceIndex].Get(xMin, yMin, zMin, xMax, yMax, zMax);

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
bool STEPGeometryDecomposer::areFeaturesSimilar(
    const FaceFeature& f1,
    const FaceFeature& f2,
    const Bnd_Box& b1,
    const Bnd_Box& b2)
{
    if (f1.type != f2.type) return false;

    double areaRatio = std::min(f1.area, f2.area) / std::max(f1.area, f2.area);
    if (areaRatio < 0.75) return false;

    Standard_Real xMin1, yMin1, zMin1, xMax1, yMax1, zMax1;
    Standard_Real xMin2, yMin2, zMin2, xMax2, yMax2, zMax2;
    b1.Get(xMin1, yMin1, zMin1, xMax1, yMax1, zMax1);
    b2.Get(xMin2, yMin2, zMin2, xMax2, yMax2, zMax2);

    double boxSize1 = std::max({xMax1 - xMin1, yMax1 - yMin1, zMax1 - zMin1});
    double boxSize2 = std::max({xMax2 - xMin2, yMax2 - yMin2, zMax2 - zMin2});
    double avgBoxSize = (boxSize1 + boxSize2) * 0.5;

    double distance = f1.centroid.Distance(f2.centroid);
    if (distance > avgBoxSize * 2.0) return false;

    if (f1.type == "PLANE" || f1.type == "CYLINDER") {
        double dotProduct = f1.normal.Dot(f2.normal);
        if (std::abs(dotProduct) < 0.9) return false;
    }

    return true;
}

// Create components from face groups with validation
void STEPGeometryDecomposer::createComponentsFromGroups(
    const std::vector<FaceFeature>& faceFeatures,
    const std::vector<std::vector<int>>& featureGroups,
    std::vector<TopoDS_Shape>& components)
{
    for (const auto& group : featureGroups) {
        if (group.size() < 2) continue;

        try {
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);

            for (int faceId : group) {
                TopoDS_Shape shapeToAdd = faceFeatures[faceId].face;
                builder.Add(compound, shapeToAdd);
            }

            TopoDS_Shape component = tryCreateSolidFromFaces(compound, faceFeatures, group);
            if (!component.IsNull()) {
                components.push_back(component);
            }
        }
        catch (const std::exception& e) {
        }
    }
}

// Try to create solid from faces
TopoDS_Shape STEPGeometryDecomposer::tryCreateSolidFromFaces(
    const TopoDS_Compound& compound,
    const std::vector<FaceFeature>& faceFeatures,
    const std::vector<int>& group)
{
    try {
        BRep_Builder builder;
        TopoDS_Shell shell;
        builder.MakeShell(shell);

        for (int faceId : group) {
            builder.Add(shell, faceFeatures[faceId].face);
        }

        ShapeFix_Shell shellFixer;
        shellFixer.Init(shell);
        shellFixer.SetPrecision(1e-6);
        shellFixer.Perform();

        TopoDS_Shell closedShell = shellFixer.Shell();

        BRepBuilderAPI_MakeSolid solidMaker(closedShell);
        if (solidMaker.IsDone()) {
            return solidMaker.Solid();
        }

        return closedShell;
    }
    catch (const std::exception& e) {
        return TopoDS_Shape();
    }
}

// Merge small similar components
void STEPGeometryDecomposer::mergeSmallComponents(std::vector<TopoDS_Shape>& components) {
    try {
        if (components.size() <= 1) return;

        std::vector<TopoDS_Shape> mergedComponents;
        std::vector<bool> merged(components.size(), false);

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

        std::vector<double> validVolumes;
        for (double vol : volumes) {
            if (vol > 1e-12) validVolumes.push_back(vol);
        }

        if (validVolumes.empty()) return;

        std::sort(validVolumes.begin(), validVolumes.end());
        double medianVolume = validVolumes[validVolumes.size() / 2];
        double smallThreshold = medianVolume * 0.01;

        for (size_t i = 0; i < components.size(); ++i) {
            if (merged[i]) continue;

            std::vector<TopoDS_Shape> mergeGroup = {components[i]};
            merged[i] = true;

            for (size_t j = i + 1; j < components.size(); ++j) {
                if (merged[j] || volumes[j] > smallThreshold) continue;

                if (areShapesSimilar(components[i], components[j])) {
                    mergeGroup.push_back(components[j]);
                    merged[j] = true;
                }
            }

            if (mergeGroup.size() > 1) {
                TopoDS_Compound compound;
                BRep_Builder builder;
                builder.MakeCompound(compound);

                for (const auto& shape : mergeGroup) {
                    builder.Add(compound, shape);
                }

                mergedComponents.push_back(compound);
            } else {
                mergedComponents.push_back(components[i]);
            }
        }

        components = std::move(mergedComponents);

    }
    catch (const std::exception& e) {
        LOG_WRN_S("Component merging failed: " + std::string(e.what()));
    }
}

// Check if two shapes are similar for merging
bool STEPGeometryDecomposer::areShapesSimilar(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2) {
    try {
        Bnd_Box box1, box2;
        BRepBndLib::Add(shape1, box1);
        BRepBndLib::Add(shape2, box2);

        if (box1.IsVoid() || box2.IsVoid()) return false;

        Standard_Real x1, y1, z1, X1, Y1, Z1;
        Standard_Real x2, y2, z2, X2, Y2, Z2;

        box1.Get(x1, y1, z1, X1, Y1, Z1);
        box2.Get(x2, y2, z2, X2, Y2, Z2);

        double vol1 = (X1 - x1) * (Y1 - y1) * (Z1 - z1);
        double vol2 = (X2 - x2) * (Y2 - y2) * (Z2 - z2);

        if (vol1 < 1e-12 || vol2 < 1e-12) return false;

        double volRatio = std::min(vol1, vol2) / std::max(vol1, vol2);
        return volRatio > 0.8;

    } catch (const std::exception& e) {
        return false;
    }
}

// Optimized adjacency graph building with spatial filtering
void STEPGeometryDecomposer::buildFaceAdjacencyGraphOptimized(
    const std::vector<TopoDS_Face>& faces,
    const std::vector<Bnd_Box>& faceBounds,
    std::vector<std::vector<int>>& adjacencyGraph)
{
    try {
        adjacencyGraph.assign(faces.size(), std::vector<int>());

        const int gridSize = 4;
        std::vector<std::vector<int>> spatialGrid(gridSize * gridSize * gridSize);

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

        for (size_t i = 0; i < faces.size(); ++i) {
            std::vector<int> nearbyFaces = findNearbyFaces(static_cast<int>(i), spatialGrid, faceBounds, gridSize);

            for (int nearbyIdx : nearbyFaces) {
                if (nearbyIdx <= static_cast<int>(i)) continue;

                if (areFacesAdjacent(faces[i], faces[nearbyIdx])) {
                    adjacencyGraph[i].push_back(nearbyIdx);
                    adjacencyGraph[nearbyIdx].push_back(static_cast<int>(i));
                }
            }
        }

        size_t totalConnections = 0;
        for (const auto& connections : adjacencyGraph) {
            totalConnections += connections.size();
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Optimized adjacency graph building failed: " + std::string(e.what()));
    }
}

// Optimized adjacent face clustering
void STEPGeometryDecomposer::clusterAdjacentFacesOptimized(
    const std::vector<TopoDS_Face>& faces,
    const std::vector<std::vector<int>>& adjacencyGraph,
    std::vector<std::vector<int>>& clusters)
{
    try {
        clusters.clear();
        std::vector<bool> visited(faces.size(), false);

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

                for (int adjacent : adjacencyGraph[current]) {
                    if (!visited[adjacent]) {
                        stack.push(adjacent);
                    }
                }
            }

            if (isValidCluster(cluster, faces)) {
                clusters.push_back(cluster);
            }
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Optimized adjacent clustering failed: " + std::string(e.what()));
    }
}

// Validate cluster quality
bool STEPGeometryDecomposer::isValidCluster(const std::vector<int>& cluster, const std::vector<TopoDS_Face>& faces) {
    if (cluster.size() < 3) return false;

    try {
        int totalEdges = 0;
        std::set<TopoDS_Edge, EdgeComparator> uniqueEdges;

        for (int faceId : cluster) {
            for (TopExp_Explorer exp(faces[faceId], TopAbs_EDGE); exp.More(); exp.Next()) {
                TopoDS_Edge edge = TopoDS::Edge(exp.Current());
                uniqueEdges.insert(edge);
                totalEdges++;
            }
        }

        double edgeToFaceRatio = static_cast<double>(uniqueEdges.size()) / cluster.size();
        if (edgeToFaceRatio < 2.5 || edgeToFaceRatio > 6.0) {
            return false;
        }

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
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

// Create validated components from clusters
void STEPGeometryDecomposer::createValidatedComponentsFromClusters(
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
                builder.Add(compound, faces[faceId]);
            }

            TopoDS_Shape component = tryCreateSolidFromFaceCluster(compound, faces, cluster);
            if (!component.IsNull()) {
                components.push_back(component);
            }
        }
        catch (const std::exception& e) {
        }
    }
}

// Try to create solid from face cluster
TopoDS_Shape STEPGeometryDecomposer::tryCreateSolidFromFaceCluster(
    const TopoDS_Compound& compound,
    const std::vector<TopoDS_Face>& faces,
    const std::vector<int>& cluster)
{
    try {
        BRep_Builder builder;
        TopoDS_Shell shell;
        builder.MakeShell(shell);

        for (int faceId : cluster) {
            builder.Add(shell, faces[faceId]);
        }

        ShapeFix_Shell shellFixer;
        shellFixer.Init(shell);
        shellFixer.SetPrecision(1e-6);
        shellFixer.Perform();

        TopoDS_Shell closedShell = shellFixer.Shell();

        BRepBuilderAPI_MakeSolid solidMaker(closedShell);
        if (solidMaker.IsDone()) {
            return solidMaker.Solid();
        }

        return closedShell;
    }
    catch (const std::exception& e) {
        return TopoDS_Shape();
    }
}

// Refine and filter components
void STEPGeometryDecomposer::refineComponents(std::vector<TopoDS_Shape>& components) {
    try {
        std::vector<TopoDS_Shape> refinedComponents;

        for (const auto& component : components) {
            try {
                if (component.IsNull()) continue;

                GProp_GProps props;
                BRepGProp::VolumeProperties(component, props);
                double volume = props.Mass();

                if (volume > 1e-12) {
                    refinedComponents.push_back(component);
                }
            }
            catch (const std::exception& e) {
            }
        }

        components = std::move(refinedComponents);
    }
    catch (const std::exception& e) {
        LOG_WRN_S("Component refinement failed: " + std::string(e.what()));
    }
}

// Basic clustering functions (non-optimized versions)
void STEPGeometryDecomposer::clusterFacesByFeatures(
    const std::vector<FaceFeature>& faceFeatures,
    std::vector<std::vector<int>>& featureGroups)
{
    try {
        std::unordered_map<std::string, std::vector<int>> typeGroups;

        for (size_t i = 0; i < faceFeatures.size(); ++i) {
            typeGroups[faceFeatures[i].type].push_back(i);
        }

        for (auto& [type, indices] : typeGroups) {
            if (indices.size() <= 1) {
                if (!indices.empty()) {
                    featureGroups.push_back({indices[0]});
                }
                continue;
            }

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

                    double areaRatio = std::min(refFeature.area, testFeature.area) / std::max(refFeature.area, testFeature.area);
                    double distance = refFeature.centroid.Distance(testFeature.centroid);

                    if (areaRatio > 0.8 && distance < 10.0) {
                        group.push_back(indices[j]);
                        assigned[j] = true;
                    }
                }

                featureGroups.push_back(group);
            }
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Feature clustering failed: " + std::string(e.what()));
    }
}

void STEPGeometryDecomposer::buildFaceAdjacencyGraph(
    const std::vector<TopoDS_Face>& faces,
    std::vector<std::vector<int>>& adjacencyGraph)
{
    try {
        adjacencyGraph.clear();
        adjacencyGraph.resize(faces.size());

        for (size_t i = 0; i < faces.size(); ++i) {
            for (size_t j = i + 1; j < faces.size(); ++j) {
                if (areFacesAdjacent(faces[i], faces[j])) {
                    adjacencyGraph[i].push_back(j);
                    adjacencyGraph[j].push_back(i);
                }
            }
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to build adjacency graph: " + std::string(e.what()));
    }
}

void STEPGeometryDecomposer::clusterAdjacentFaces(
    const std::vector<TopoDS_Face>& faces,
    const std::vector<std::vector<int>>& adjacencyGraph,
    std::vector<std::vector<int>>& clusters)
{
    try {
        clusters.clear();
        std::vector<bool> visited(faces.size(), false);

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

                for (int adjacent : adjacencyGraph[current]) {
                    if (!visited[adjacent]) {
                        stack.push(adjacent);
                    }
                }
            }

            if (cluster.size() >= 3) {
                clusters.push_back(cluster);
            }
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Adjacent clustering failed: " + std::string(e.what()));
    }
}
