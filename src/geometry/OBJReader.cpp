#include "OBJReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE includes
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeFace.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakePolygon.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeVertex.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeWire.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeShell.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeSolid.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopoDS_Builder.hxx>
#include <OpenCASCADE/Geom_Plane.hxx>
#include <OpenCASCADE/GeomAPI_PointsToBSpline.hxx>
#include <OpenCASCADE/GeomAPI_Interpolate.hxx>
#include <OpenCASCADE/TColgp_Array1OfPnt.hxx>
#include <OpenCASCADE/ShapeFix_Shape.hxx>
#include <OpenCASCADE/BRepCheck_Analyzer.hxx>

// Standard includes
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <thread>

// Static member initialization
std::unordered_map<std::string, OBJReader::ReadResult> OBJReader::s_cache;
std::mutex OBJReader::s_cacheMutex;

OBJReader::ReadResult OBJReader::readFile(const std::string& filePath,
    const OptimizationOptions& options,
    ProgressCallback progress)
{
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    ReadResult result;
    result.formatName = "OBJ";

    try {
        // Validate file
        std::string errorMessage;
        if (!validateFile(filePath, errorMessage)) {
            result.errorMessage = errorMessage;
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Check file extension
        if (!isValidFile(filePath)) {
            result.errorMessage = "File is not an OBJ file: " + filePath;
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

        if (progress) progress(10, "Parsing OBJ file");

        // Parse OBJ file
        std::vector<Vertex> vertices;
        std::vector<Vertex> normals;
        std::vector<Face> faces;
        std::unordered_map<std::string, Material> materials;

        if (!parseOBJFile(filePath, vertices, faces, normals, materials, progress)) {
            result.errorMessage = "Failed to parse OBJ file: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        LOG_INF_S("OBJ file parsed successfully: " + std::to_string(vertices.size()) + " vertices, " + std::to_string(faces.size()) + " faces");

        if (progress) progress(60, "Creating geometry");

        if (vertices.empty() || faces.empty()) {
            result.errorMessage = "No valid geometry data found in OBJ file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Create shape from parsed data
        std::string baseName = std::filesystem::path(filePath).stem().string();
        TopoDS_Shape shape = createShapeFromOBJData(vertices, faces, normals, baseName, options);

        if (shape.IsNull()) {
            result.errorMessage = "Failed to create geometry from OBJ data";
            LOG_ERR_S("OBJ shape creation failed: shape is null");
            return result;
        }

        LOG_INF_S("OBJ shape created successfully");

        if (progress) progress(80, "Creating OCCGeometry");

        // Create OCCGeometry
        auto geometry = createGeometryFromShape(shape, baseName, options);
        if (!geometry) {
            result.errorMessage = "Failed to create OCCGeometry from shape";
            LOG_ERR_S("OBJ OCCGeometry creation failed: geometry is null");
            return result;
        }

        LOG_INF_S("OBJ OCCGeometry created successfully");

        result.geometries.push_back(geometry);
        result.rootShape = shape;
        result.success = true;

        // Cache result if enabled
        if (options.enableCaching) {
            std::lock_guard<std::mutex> lock(s_cacheMutex);
            s_cache[filePath] = result;
        }

        auto totalEndTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEndTime - totalStartTime);
        result.importTime = static_cast<double>(totalDuration.count());

        LOG_INF_S("OBJ file imported successfully: " + std::to_string(vertices.size()) + 
                 " vertices, " + std::to_string(faces.size()) + " faces in " + 
                 std::to_string(result.importTime) + "ms");

        return result;
    }
    catch (const std::exception& e) {
        result.errorMessage = "Exception during OBJ import: " + std::string(e.what());
        LOG_ERR_S(result.errorMessage);
        return result;
    }
}

bool OBJReader::isValidFile(const std::string& filePath) const
{
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".obj";
}

std::vector<std::string> OBJReader::getSupportedExtensions() const
{
    return {".obj"};
}

std::string OBJReader::getFormatName() const
{
    return "OBJ";
}

std::string OBJReader::getFileFilter() const
{
    return "OBJ files (*.obj)|*.obj";
}

bool OBJReader::parseOBJFile(const std::string& filePath,
    std::vector<Vertex>& vertices,
    std::vector<Face>& faces,
    std::vector<Vertex>& normals,
    std::unordered_map<std::string, Material>& materials,
    ProgressCallback progress)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERR_S("Cannot open OBJ file: " + filePath);
        return false;
    }

    LOG_INF_S("Successfully opened OBJ file: " + filePath);

    std::string line;
    std::string currentMaterial;
    int lineNumber = 0;
    int totalLines = 0;

    // First pass: count lines for progress
    std::ifstream countFile(filePath);
    std::string countLine;
    while (std::getline(countFile, countLine)) {
        totalLines++;
    }
    countFile.close();

    // Reset file position to beginning
    file.seekg(0, std::ios::beg);
    if (!file.good()) {
        LOG_ERR_S("Failed to reset file position for: " + filePath);
        return false;
    }

    while (std::getline(file, line)) {
        lineNumber++;

        if (progress && lineNumber % 1000 == 0) {
            int percent = 10 + static_cast<int>((lineNumber * 40.0) / totalLines);
            progress(percent, "Parsing line " + std::to_string(lineNumber) + "/" + std::to_string(totalLines));
        }

        // Debug: log first few lines
        if (lineNumber <= 5) {
            LOG_INF_S("OBJ line " + std::to_string(lineNumber) + ": " + line);
        }

        // Remove leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            // Vertex
            double x, y, z;
            if (iss >> x >> y >> z) {
                vertices.emplace_back(x, y, z);
            } else {
                LOG_WRN_S("Failed to parse vertex from line: " + line);
            }
        }
        else if (type == "vn") {
            // Vertex normal
            double x, y, z;
            if (iss >> x >> y >> z) {
                normals.emplace_back(x, y, z);
            } else {
                LOG_WRN_S("Failed to parse vertex normal from line: " + line);
            }
        }
        else if (type == "f") {
            // Face
            Face face;
            face.materialName = currentMaterial;

            std::string vertexStr;
            while (iss >> vertexStr) {
                // Parse vertex/normal indices (format: v/vt/vn, v//vn, or just v)
                std::string vertexPart, normalPart;
                size_t firstSlash = vertexStr.find('/');
                size_t secondSlash = std::string::npos;

                if (firstSlash != std::string::npos) {
                    vertexPart = vertexStr.substr(0, firstSlash);
                    secondSlash = vertexStr.find('/', firstSlash + 1);
                    if (secondSlash != std::string::npos) {
                        normalPart = vertexStr.substr(secondSlash + 1);
                    } else if (vertexStr.size() > firstSlash + 1) {
                        // Handle v//vn format
                        if (vertexStr[firstSlash + 1] != '/') {
                            // This is v/vt format (no normal)
                        } else {
                            // This is v//vn format
                            normalPart = vertexStr.substr(firstSlash + 2);
                        }
                    }
                } else {
                    vertexPart = vertexStr;
                }

                // Parse vertex index
                try {
                    int vertexIndex = std::stoi(vertexPart);
                    // OBJ indices are 1-based, convert to 0-based
                    if (vertexIndex > 0) {
                        face.vertexIndices.push_back(vertexIndex - 1);
                    } else if (vertexIndex < 0) {
                        // Negative indices are relative to end
                        face.vertexIndices.push_back(static_cast<int>(vertices.size()) + vertexIndex);
                    }
                } catch (const std::exception&) {
                    LOG_WRN_S("Invalid vertex index in face: " + vertexPart);
                }

                // Parse normal index if present
                if (!normalPart.empty()) {
                    try {
                        int normalIndex = std::stoi(normalPart);
                        // OBJ indices are 1-based, convert to 0-based
                        if (normalIndex > 0) {
                            face.normalIndices.push_back(normalIndex - 1);
                        } else if (normalIndex < 0) {
                            // Negative indices are relative to end
                            face.normalIndices.push_back(static_cast<int>(normals.size()) + normalIndex);
                        }
                    } catch (const std::exception&) {
                        LOG_WRN_S("Invalid normal index in face: " + normalPart);
                    }
                }
            }

            if (face.vertexIndices.size() >= 3) {
                faces.push_back(face);
            } else {
                LOG_WRN_S("Face with insufficient vertices: " + std::to_string(face.vertexIndices.size()));
            }
        }
        else if (type == "mtllib") {
            // Material library
            std::string mtlFileName;
            iss >> mtlFileName;
            
            // Look for MTL file in same directory as OBJ
            std::filesystem::path objPath(filePath);
            std::filesystem::path mtlPath = objPath.parent_path() / mtlFileName;
            
            if (std::filesystem::exists(mtlPath)) {
                parseMTLFile(mtlPath.string(), materials);
            }
        }
        else if (type == "usemtl") {
            // Use material
            iss >> currentMaterial;
        }
    }

    file.close();
    return true;
}

TopoDS_Shape OBJReader::createShapeFromOBJData(
    const std::vector<Vertex>& vertices,
    const std::vector<Face>& faces,
    const std::vector<Vertex>& normals,
    const std::string& baseName,
    const OptimizationOptions& options)
{
    try {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);

        int validFaces = 0;
        int correctNormals = 0;
        int incorrectNormals = 0;
        int facesWithExplicitNormals = 0;
        int facesWithCalculatedNormals = 0;

        // Calculate model center for better normal direction analysis
        gp_Pnt modelCenter(0, 0, 0);
        int validVerticesForCenter = 0;

        // Quick pass to calculate approximate model center
        for (const auto& face : faces) {
            if (face.vertexIndices.size() >= 3) {
                // Use first vertex of each face for center calculation
                int vertexIdx = face.vertexIndices[0];
                if (vertexIdx >= 0 && vertexIdx < static_cast<int>(vertices.size())) {
                    const Vertex& v = vertices[vertexIdx];
                    modelCenter.SetX(modelCenter.X() + v.x);
                    modelCenter.SetY(modelCenter.Y() + v.y);
                    modelCenter.SetZ(modelCenter.Z() + v.z);
                    validVerticesForCenter++;
                }
            }
        }

        if (validVerticesForCenter > 0) {
            modelCenter.SetX(modelCenter.X() / validVerticesForCenter);
            modelCenter.SetY(modelCenter.Y() / validVerticesForCenter);
            modelCenter.SetZ(modelCenter.Z() / validVerticesForCenter);
            LOG_INF_S("Calculated model center for OBJ normal analysis: (" +
                     std::to_string(modelCenter.X()) + ", " +
                     std::to_string(modelCenter.Y()) + ", " +
                     std::to_string(modelCenter.Z()) + ")");
        } else {
            LOG_WRN_S("Could not calculate model center for OBJ, using origin (0,0,0)");
        }

        for (const auto& face : faces) {
            if (face.vertexIndices.size() < 3) {
                continue;
            }

            // Count normals before processing
            bool hasExplicitNormals = !face.normalIndices.empty() && 
                                    face.normalIndices.size() == face.vertexIndices.size();
            
            if (hasExplicitNormals) {
                facesWithExplicitNormals++;
                
                // Check explicit normals direction
                for (int normalIdx : face.normalIndices) {
                    if (normalIdx >= 0 && normalIdx < static_cast<int>(normals.size())) {
                        const Vertex& normal = normals[normalIdx];
                        if (normal.z >= 0) {
                            correctNormals++;
                        } else {
                            incorrectNormals++;
                        }
                    }
                }
            } else {
                facesWithCalculatedNormals++;
                
                // For calculated normals, check the computed direction
                if (face.vertexIndices.size() >= 3) {
                    const Vertex& v0 = vertices[face.vertexIndices[0]];
                    const Vertex& v1 = vertices[face.vertexIndices[1]];
                    const Vertex& v2 = vertices[face.vertexIndices[2]];
                    
                    gp_Vec edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
                    gp_Vec edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
                    gp_Vec computedNormal = edge1.Crossed(edge2);
                    
                    if (computedNormal.Magnitude() > 1e-6) {
                        computedNormal.Normalize();
                        
                        // Calculate face center
                        gp_Pnt faceCenter(0, 0, 0);
                        for (int idx : face.vertexIndices) {
                            if (idx >= 0 && idx < static_cast<int>(vertices.size())) {
                                const Vertex& v = vertices[idx];
                                faceCenter.SetX(faceCenter.X() + v.x);
                                faceCenter.SetY(faceCenter.Y() + v.y);
                                faceCenter.SetZ(faceCenter.Z() + v.z);
                            }
                        }
                        faceCenter.SetX(faceCenter.X() / face.vertexIndices.size());
                        faceCenter.SetY(faceCenter.Y() / face.vertexIndices.size());
                        faceCenter.SetZ(faceCenter.Z() / face.vertexIndices.size());
                        
                        // Calculate vector from model center to face center
                        // This gives us the direction from center to the face surface
                        gp_Vec centerToFace(
                            faceCenter.X() - modelCenter.X(),
                            faceCenter.Y() - modelCenter.Y(),
                            faceCenter.Z() - modelCenter.Z()
                        );

                        // Check if normal points in the same direction as centerToFace (outward)
                        double dotProduct = computedNormal.Dot(centerToFace);
                        if (dotProduct > 0) {
                            correctNormals++;
                        } else {
                            incorrectNormals++;
                        }
                    }
                }
            }

            TopoDS_Shape faceShape = createFaceFromVertices(vertices, face.vertexIndices, normals, face.normalIndices, modelCenter);
            if (!faceShape.IsNull()) {
                builder.Add(compound, faceShape);
                validFaces++;
            }
        }

        // Enhanced normal statistics and diagnostics
        LOG_INF_S("=== OBJ Normal Analysis Report ===");
        LOG_INF_S("File: " + baseName);
        LOG_INF_S("Total faces in file: " + std::to_string(faces.size()));
        LOG_INF_S("Valid faces created: " + std::to_string(validFaces));
        LOG_INF_S("Failed face creation: " + std::to_string(faces.size() - validFaces));

        // Normal source analysis
        LOG_INF_S("--- Normal Source Analysis ---");
        LOG_INF_S("Faces with explicit normals (vn): " + std::to_string(facesWithExplicitNormals));
        LOG_INF_S("Faces with calculated normals: " + std::to_string(facesWithCalculatedNormals));
        LOG_INF_S("Faces without normals: " + std::to_string(faces.size() - facesWithExplicitNormals - facesWithCalculatedNormals));

        // Normal direction analysis
        LOG_INF_S("--- Normal Direction Analysis ---");
        int totalNormalsAnalyzed = correctNormals + incorrectNormals;
        LOG_INF_S("Total normals analyzed: " + std::to_string(totalNormalsAnalyzed));
        LOG_INF_S("Normals pointing outward: " + std::to_string(correctNormals));
        LOG_INF_S("Normals pointing inward: " + std::to_string(incorrectNormals));

        if (totalNormalsAnalyzed > 0) {
            double correctPercentage = (static_cast<double>(correctNormals) / totalNormalsAnalyzed) * 100.0;
            LOG_INF_S("Normal correctness ratio: " + std::to_string(correctPercentage) + "%");

            // Provide diagnostic feedback
            if (correctPercentage < 50.0) {
                LOG_WRN_S("WARNING: Low normal correctness ratio (" + std::to_string(correctPercentage) +
                         "%). This may indicate:");
                LOG_WRN_S("  - Incorrect winding order in OBJ file");
                LOG_WRN_S("  - Inconsistent normal definitions");
                LOG_WRN_S("  - Geometry with complex topology");
                LOG_WRN_S("  Consider checking the source OBJ file or enabling normal auto-correction.");
            } else if (correctPercentage < 80.0) {
                LOG_INF_S("NOTICE: Moderate normal correctness ratio (" + std::to_string(correctPercentage) +
                         "%). Some faces may need orientation correction.");
            } else {
                LOG_INF_S("GOOD: High normal correctness ratio (" + std::to_string(correctPercentage) +
                         "%). Face orientations appear consistent.");
            }
        } else {
            LOG_WRN_S("No normals were analyzed. This may indicate:");
            LOG_WRN_S("  - OBJ file lacks normal definitions (vn statements)");
            LOG_WRN_S("  - All faces are degenerate or invalid");
            LOG_WRN_S("  - Normal calculation failed for all faces");
        }

        // Additional quality metrics
        LOG_INF_S("--- Quality Metrics ---");
        if (faces.size() > 0) {
            double faceCreationRate = (static_cast<double>(validFaces) / faces.size()) * 100.0;
            LOG_INF_S("Face creation success rate: " + std::to_string(faceCreationRate) + "%");

            if (faceCreationRate < 90.0) {
                LOG_WRN_S("WARNING: Low face creation success rate. Check for:");
                LOG_WRN_S("  - Degenerate triangles (zero area)");
                LOG_WRN_S("  - Invalid vertex indices");
                LOG_WRN_S("  - Corrupted OBJ file data");
            }
        }

        if (facesWithExplicitNormals > 0 && facesWithCalculatedNormals > 0) {
            LOG_INF_S("MIXED: File contains both explicit and calculated normals");
            LOG_INF_S("This may indicate incomplete normal definitions in the source file.");
        }

        LOG_INF_S("=====================================");

        if (validFaces == 0) {
            LOG_ERR_S("No valid faces could be created from OBJ data");
            return TopoDS_Shape();
        }

        // If we have only one face, return it directly
        if (validFaces == 1) {
            TopExp_Explorer explorer(compound, TopAbs_FACE);
            if (explorer.More()) {
                return explorer.Current();
            }
        }

        return compound;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create shape from OBJ data: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OBJReader::createFaceFromVertices(
    const std::vector<Vertex>& vertices,
    const std::vector<int>& faceIndices,
    const std::vector<Vertex>& normals,
    const std::vector<int>& normalIndices,
    const gp_Pnt& modelCenter)
{
    try {
        if (faceIndices.size() < 3) {
            return TopoDS_Shape();
        }

        // Create polygon from vertices
        BRepBuilderAPI_MakePolygon polygon;
        
        for (int index : faceIndices) {
            if (index >= 0 && index < static_cast<int>(vertices.size())) {
                polygon.Add(vertices[index].toPoint());
            }
        }
        
        // Close the polygon
        polygon.Close();
        
        if (!polygon.IsDone()) {
            return TopoDS_Shape();
        }

        // Create wire from polygon
        TopoDS_Wire wire = polygon.Wire();

        // Create face from wire
        BRepBuilderAPI_MakeFace faceMaker(wire);

        if (!faceMaker.IsDone()) {
            return TopoDS_Shape();
        }

        TopoDS_Face face = faceMaker.Face();

        // Calculate face normal and ensure correct orientation
        gp_Vec faceNormal(0, 0, 0);
        bool hasValidNormal = false;
        
        if (normalIndices.empty() || normalIndices.size() != faceIndices.size()) {
            // Calculate face normal from vertices using cross product
            if (faceIndices.size() >= 3) {
                const Vertex& v0 = vertices[faceIndices[0]];
                const Vertex& v1 = vertices[faceIndices[1]];
                const Vertex& v2 = vertices[faceIndices[2]];
                
                gp_Vec edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
                gp_Vec edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
                faceNormal = edge1.Crossed(edge2);
                
                if (faceNormal.Magnitude() > 1e-6) {
                    faceNormal.Normalize();
                    hasValidNormal = true;
                    LOG_INF_S("Calculated face normal: (" +
                             std::to_string(faceNormal.X()) + ", " +
                             std::to_string(faceNormal.Y()) + ", " +
                             std::to_string(faceNormal.Z()) + ")");
                }
            }
        } else {
            // Use provided normals
            gp_Vec avgNormal(0, 0, 0);
            bool hasValidNormals = true;

            for (size_t i = 0; i < normalIndices.size(); ++i) {
                int normalIdx = normalIndices[i];
                if (normalIdx >= 0 && normalIdx < static_cast<int>(normals.size())) {
                    const Vertex& normal = normals[normalIdx];
                    avgNormal = gp_Vec(avgNormal.X() + normal.x,
                                     avgNormal.Y() + normal.y,
                                     avgNormal.Z() + normal.z);
                } else {
                    hasValidNormals = false;
                    break;
                }
            }

            if (hasValidNormals && avgNormal.Magnitude() > 1e-6) {
                avgNormal.Normalize();
                faceNormal = avgNormal;
                hasValidNormal = true;
                LOG_INF_S("Face normal from OBJ: (" +
                         std::to_string(faceNormal.X()) + ", " +
                         std::to_string(faceNormal.Y()) + ", " +
                         std::to_string(faceNormal.Z()) + ")");
            }
        }
        
        // Check face orientation and ensure normal points outward
        if (hasValidNormal) {
            if (faceIndices.size() >= 3) {
                const Vertex& v0 = vertices[faceIndices[0]];
                const Vertex& v1 = vertices[faceIndices[1]];
                const Vertex& v2 = vertices[faceIndices[2]];
                
                // Calculate the cross product of the first two edges
                gp_Vec edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
                gp_Vec edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
                gp_Vec calculatedNormal = edge1.Crossed(edge2);
                
                if (calculatedNormal.Magnitude() > 1e-6) {
                    calculatedNormal.Normalize();
                    
                    // Check if the calculated normal matches the provided normal
                    double dotProduct = calculatedNormal.Dot(faceNormal);
                    
                    // If dot product is negative, the face vertices are ordered clockwise
                    // and we need to reverse the face
                    if (dotProduct < 0) {
                        face.Reverse();
                    }
                }
            }
        } else {
            // If no valid normal provided, ensure face points outward
            if (faceIndices.size() >= 3) {
                const Vertex& v0 = vertices[faceIndices[0]];
                const Vertex& v1 = vertices[faceIndices[1]];
                const Vertex& v2 = vertices[faceIndices[2]];
                
                // Calculate face normal from vertices
                gp_Vec edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
                gp_Vec edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
                gp_Vec calculatedNormal = edge1.Crossed(edge2);
                
                if (calculatedNormal.Magnitude() > 1e-6) {
                    calculatedNormal.Normalize();
                    
                    // Calculate face center
                    gp_Pnt faceCenter(0, 0, 0);
                    for (int index : faceIndices) {
                        if (index >= 0 && index < static_cast<int>(vertices.size())) {
                            const Vertex& v = vertices[index];
                            faceCenter = gp_Pnt(faceCenter.X() + v.x, faceCenter.Y() + v.y, faceCenter.Z() + v.z);
                        }
                    }
                    faceCenter = gp_Pnt(faceCenter.X() / faceIndices.size(), 
                                      faceCenter.Y() / faceIndices.size(), 
                                      faceCenter.Z() / faceIndices.size());
                    
                    // Calculate vector from model center to face center
                    // This gives us the direction from center to the face surface
                    gp_Vec centerToFace(
                        faceCenter.X() - modelCenter.X(),
                        faceCenter.Y() - modelCenter.Y(),
                        faceCenter.Z() - modelCenter.Z()
                    );

                    // Check if calculated normal points in the same direction as centerToFace (outward)
                    double dotProduct = calculatedNormal.Dot(centerToFace);
                    if (dotProduct < 0) {
                        // Normal points inward, reverse the face
                        face.Reverse();
                    }
                }
            }
        }

        return face;
    }
    catch (const std::exception& e) {
        LOG_WRN_S("Failed to create face from vertices: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

bool OBJReader::parseMTLFile(const std::string& mtlFilePath,
    std::unordered_map<std::string, Material>& materials)
{
    std::ifstream file(mtlFilePath);
    if (!file.is_open()) {
        LOG_WRN_S("Cannot open MTL file: " + mtlFilePath);
        return false;
    }

    std::string line;
    Material currentMaterial;

    while (std::getline(file, line)) {
        // Remove leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "newmtl") {
            // Save previous material
            if (!currentMaterial.name.empty()) {
                materials[currentMaterial.name] = currentMaterial;
            }
            
            // Start new material
            iss >> currentMaterial.name;
            currentMaterial.r = currentMaterial.g = currentMaterial.b = 0.8; // Default gray
        }
        else if (type == "Kd") {
            // Diffuse color
            iss >> currentMaterial.r >> currentMaterial.g >> currentMaterial.b;
        }
    }

    // Save last material
    if (!currentMaterial.name.empty()) {
        materials[currentMaterial.name] = currentMaterial;
    }

    file.close();
    return true;
}
