#include "STLReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE includes
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeFace.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakePolygon.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
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
#include <cstring>

// Static member initialization
std::unordered_map<std::string, STLReader::ReadResult> STLReader::s_cache;
std::mutex STLReader::s_cacheMutex;

STLReader::ReadResult STLReader::readFile(const std::string& filePath,
    const OptimizationOptions& options,
    ProgressCallback progress)
{
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    ReadResult result;
    result.formatName = "STL";

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
            result.errorMessage = "File is not an STL file: " + filePath;
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

        if (progress) progress(10, "Detecting STL format");

        // Detect file format
        STLFormat format = detectFormat(filePath);
        if (format == STLFormat::Unknown) {
            result.errorMessage = "Unknown STL file format: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        if (progress) progress(20, "Parsing STL file");

        // Parse STL file
        std::vector<Triangle> triangles;
        bool parseSuccess = false;

        if (format == STLFormat::ASCII) {
            parseSuccess = parseASCIISTL(filePath, triangles, progress);
        } else if (format == STLFormat::Binary) {
            parseSuccess = parseBinarySTL(filePath, triangles, progress);
        }

        if (!parseSuccess) {
            result.errorMessage = "Failed to parse STL file: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        LOG_INF_S("STL file parsed successfully: " + std::to_string(triangles.size()) + " triangles");

        if (progress) progress(60, "Creating geometry");

        if (triangles.empty()) {
            result.errorMessage = "No triangles found in STL file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Create shape from parsed data
        std::string baseName = std::filesystem::path(filePath).stem().string();
        TopoDS_Shape shape = createShapeFromSTLData(triangles, baseName, options);

        if (shape.IsNull()) {
            result.errorMessage = "Failed to create geometry from STL data";
            LOG_ERR_S("STL shape creation failed: shape is null");
            return result;
        }

        LOG_INF_S("STL shape created successfully");

        if (progress) progress(80, "Creating OCCGeometry");

        // Create OCCGeometry
        auto geometry = createGeometryFromShape(shape, baseName, options);
        if (!geometry) {
            result.errorMessage = "Failed to create OCCGeometry from shape";
            LOG_ERR_S("STL OCCGeometry creation failed: geometry is null");
            return result;
        }

        LOG_INF_S("STL OCCGeometry created successfully");

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

        LOG_INF_S("STL file imported successfully: " + std::to_string(triangles.size()) + 
                 " triangles in " + std::to_string(result.importTime) + "ms");

        return result;
    }
    catch (const std::exception& e) {
        result.errorMessage = "Exception during STL import: " + std::string(e.what());
        LOG_ERR_S(result.errorMessage);
        return result;
    }
}

bool STLReader::isValidFile(const std::string& filePath) const
{
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".stl";
}

std::vector<std::string> STLReader::getSupportedExtensions() const
{
    return {".stl"};
}

std::string STLReader::getFormatName() const
{
    return "STL";
}

std::string STLReader::getFileFilter() const
{
    return "STL files (*.stl)|*.stl";
}

STLReader::STLFormat STLReader::detectFormat(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return STLFormat::Unknown;
    }

    // Read first 80 bytes to check for ASCII signature
    char header[81] = {0};
    file.read(header, 80);
    
    // Check if it starts with "solid" (ASCII STL)
    std::string headerStr(header);
    std::transform(headerStr.begin(), headerStr.end(), headerStr.begin(), ::tolower);
    
    if (headerStr.find("solid") == 0) {
        // Check if it's really ASCII by looking for "facet" keyword
        file.seekg(0);
        std::string line;
        std::getline(file, line);
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        
        if (line.find("solid") == 0) {
            // Read a few more lines to confirm ASCII format
            for (int i = 0; i < 5 && std::getline(file, line); ++i) {
                std::transform(line.begin(), line.end(), line.begin(), ::tolower);
                if (line.find("facet") != std::string::npos) {
                    file.close();
                    return STLFormat::ASCII;
                }
            }
        }
    }

    file.close();
    return STLFormat::Binary;
}

bool STLReader::parseASCIISTL(const std::string& filePath,
    std::vector<Triangle>& triangles,
    ProgressCallback progress)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERR_S("Cannot open STL file: " + filePath);
        return false;
    }

    LOG_INF_S("Successfully opened STL ASCII file: " + filePath);

    std::string line;
    Triangle currentTriangle;
    int triangleCount = 0;
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

        // Debug: log first few lines
        if (lineNumber <= 5) {
            LOG_INF_S("STL ASCII line " + std::to_string(lineNumber) + ": " + line);
        }

        if (progress && lineNumber % 1000 == 0) {
            int percent = 20 + static_cast<int>((lineNumber * 30.0) / totalLines);
            progress(percent, "Parsing line " + std::to_string(lineNumber) + "/" + std::to_string(totalLines));
        }

        // Remove leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "facet") {
            // Read normal
            std::string normalKeyword;
            iss >> normalKeyword;
            if (normalKeyword == "normal") {
                double nx, ny, nz;
                if (iss >> nx >> ny >> nz) {
                    currentTriangle.normal = gp_Vec(nx, ny, nz);
                }
            }
        }
        else if (keyword == "vertex") {
            // Read vertex
            double x, y, z;
            if (iss >> x >> y >> z) {
                gp_Pnt vertex(x, y, z);
                
                // Determine which vertex this is (0, 1, or 2)
                int vertexIndex = triangleCount % 3;
                currentTriangle.vertices[vertexIndex] = vertex;
                
                triangleCount++;
                
                // If we've read 3 vertices, we have a complete triangle
                if (triangleCount % 3 == 0) {
                    triangles.push_back(currentTriangle);
                }
            }
        }
    }

    file.close();
    return true;
}

bool STLReader::parseBinarySTL(const std::string& filePath,
    std::vector<Triangle>& triangles,
    ProgressCallback progress)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERR_S("Cannot open STL file: " + filePath);
        return false;
    }

    LOG_INF_S("Successfully opened STL file: " + filePath);

    // Read header (80 bytes)
    char header[81] = {0};
    if (!readBinaryData(file, header, 80)) {
        LOG_ERR_S("Failed to read STL header");
        return false;
    }

    // Read triangle count (4 bytes)
    uint32_t triangleCount;
    if (!readBinaryData(file, &triangleCount, sizeof(uint32_t))) {
        LOG_ERR_S("Failed to read triangle count");
        return false;
    }


    triangles.reserve(triangleCount);

    // Read triangles
    for (uint32_t i = 0; i < triangleCount; ++i) {
        if (progress && i % 1000 == 0) {
            int percent = 20 + static_cast<int>((i * 30.0) / triangleCount);
            progress(percent, "Reading triangle " + std::to_string(i + 1) + "/" + std::to_string(triangleCount));
        }

        Triangle triangle;

        // Read normal (3 floats)
        float normal[3];
        if (!readBinaryData(file, normal, 3 * sizeof(float))) {
            LOG_ERR_S("Failed to read triangle normal");
            return false;
        }
        triangle.normal = gp_Vec(normal[0], normal[1], normal[2]);

        // Read vertices (3 vertices * 3 floats each)
        float vertices[9];
        if (!readBinaryData(file, vertices, 9 * sizeof(float))) {
            LOG_ERR_S("Failed to read triangle vertices");
            return false;
        }

        triangle.vertices[0] = gp_Pnt(vertices[0], vertices[1], vertices[2]);
        triangle.vertices[1] = gp_Pnt(vertices[3], vertices[4], vertices[5]);
        triangle.vertices[2] = gp_Pnt(vertices[6], vertices[7], vertices[8]);

        triangles.push_back(triangle);

        // Skip attribute byte count (2 bytes) - not used
        uint16_t attributeByteCount;
        if (!readBinaryData(file, &attributeByteCount, sizeof(uint16_t))) {
            LOG_ERR_S("Failed to read attribute byte count");
            return false;
        }
    }

    file.close();
    return true;
}

TopoDS_Shape STLReader::createShapeFromSTLData(
    const std::vector<Triangle>& triangles,
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
        int trianglesWithValidNormals = 0;
        int trianglesWithInvalidNormals = 0;

        for (const auto& triangle : triangles) {
            // Check STL triangle normal direction
            if (triangle.normal.Magnitude() > 1e-6) {
                trianglesWithValidNormals++;
                
                // Calculate triangle center
                gp_Pnt triangleCenter(
                    (triangle.vertices[0].X() + triangle.vertices[1].X() + triangle.vertices[2].X()) / 3.0,
                    (triangle.vertices[0].Y() + triangle.vertices[1].Y() + triangle.vertices[2].Y()) / 3.0,
                    (triangle.vertices[0].Z() + triangle.vertices[1].Z() + triangle.vertices[2].Z()) / 3.0
                );
                
                // Calculate vector from triangle center to origin
                gp_Vec centerToOrigin(-triangleCenter.X(), -triangleCenter.Y(), -triangleCenter.Z());
                
                // Check if STL normal points away from origin (outward)
                double dotProduct = triangle.normal.Dot(centerToOrigin);
                if (dotProduct > 0) {
                    correctNormals++;
                } else {
                    incorrectNormals++;
                }
                
                // Also check calculated normal for comparison
                gp_Vec edge1(triangle.vertices[1].X() - triangle.vertices[0].X(),
                            triangle.vertices[1].Y() - triangle.vertices[0].Y(),
                            triangle.vertices[1].Z() - triangle.vertices[0].Z());
                gp_Vec edge2(triangle.vertices[2].X() - triangle.vertices[0].X(),
                            triangle.vertices[2].Y() - triangle.vertices[0].Y(),
                            triangle.vertices[2].Z() - triangle.vertices[0].Z());
                gp_Vec calculatedNormal = edge1.Crossed(edge2);
                
                if (calculatedNormal.Magnitude() > 1e-6) {
                    calculatedNormal.Normalize();
                    // Check consistency between STL normal and calculated normal
                    double consistencyDot = calculatedNormal.Dot(triangle.normal);
                    if (consistencyDot < 0) {
                        // Normals are inconsistent - this indicates potential issues
                        LOG_WRN_S("STL normal inconsistent with calculated normal (dot product: " + 
                                 std::to_string(consistencyDot) + ")");
                    }
                }
            } else {
                trianglesWithInvalidNormals++;
            }

            TopoDS_Shape faceShape = createFaceFromTriangle(triangle);
            if (!faceShape.IsNull()) {
                builder.Add(compound, faceShape);
                validFaces++;
            }
        }

        // Output normal statistics
        LOG_INF_S("=== STL Normal Statistics ===");
        LOG_INF_S("Total triangles processed: " + std::to_string(triangles.size()));
        LOG_INF_S("Valid faces created: " + std::to_string(validFaces));
        LOG_INF_S("Triangles with valid normals: " + std::to_string(trianglesWithValidNormals));
        LOG_INF_S("Triangles with invalid normals: " + std::to_string(trianglesWithInvalidNormals));
        LOG_INF_S("Total normals analyzed: " + std::to_string(correctNormals + incorrectNormals));
        LOG_INF_S("Correct normals (outward): " + std::to_string(correctNormals));
        LOG_INF_S("Incorrect normals (inward): " + std::to_string(incorrectNormals));
        
        if (correctNormals + incorrectNormals > 0) {
            double correctPercentage = (static_cast<double>(correctNormals) / (correctNormals + incorrectNormals)) * 100.0;
            LOG_INF_S("Normal correctness: " + std::to_string(correctPercentage) + "%");
        }
        LOG_INF_S("=============================");

        if (validFaces == 0) {
            LOG_ERR_S("No valid faces could be created from STL data");
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
        LOG_ERR_S("Failed to create shape from STL data: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape STLReader::createFaceFromTriangle(const Triangle& triangle)
{
    try {
        // Create polygon from triangle vertices
        BRepBuilderAPI_MakePolygon polygon;
        polygon.Add(triangle.vertices[0]);
        polygon.Add(triangle.vertices[1]);
        polygon.Add(triangle.vertices[2]);
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

        // Use the triangle normal to ensure correct face orientation
        // STL files provide explicit normals for each triangle
        if (triangle.normal.Magnitude() > 1e-6) {
            LOG_INF_S("Triangle normal from STL: (" +
                     std::to_string(triangle.normal.X()) + ", " +
                     std::to_string(triangle.normal.Y()) + ", " +
                     std::to_string(triangle.normal.Z()) + ")");
            
            // Calculate face normal from vertices for comparison
            gp_Vec edge1(triangle.vertices[1].X() - triangle.vertices[0].X(),
                        triangle.vertices[1].Y() - triangle.vertices[0].Y(),
                        triangle.vertices[1].Z() - triangle.vertices[0].Z());
            gp_Vec edge2(triangle.vertices[2].X() - triangle.vertices[0].X(),
                        triangle.vertices[2].Y() - triangle.vertices[0].Y(),
                        triangle.vertices[2].Z() - triangle.vertices[0].Z());
            gp_Vec calculatedNormal = edge1.Crossed(edge2);
            
            if (calculatedNormal.Magnitude() > 1e-6) {
                calculatedNormal.Normalize();
                LOG_INF_S("Calculated triangle normal: (" +
                         std::to_string(calculatedNormal.X()) + ", " +
                         std::to_string(calculatedNormal.Y()) + ", " +
                         std::to_string(calculatedNormal.Z()) + ")");
                
                // Check if normals are consistent
                double dotProduct = calculatedNormal.Dot(triangle.normal);
                LOG_INF_S("Normal consistency check - dot product: " + std::to_string(dotProduct));
                
                // If dot product is negative, the face is oriented incorrectly
                if (dotProduct < 0) {
                    LOG_INF_S("Reversing STL face orientation");
                    face.Reverse();
                }
            }
        }

        return face;
    }
    catch (const std::exception& e) {
        LOG_WRN_S("Failed to create face from triangle: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

bool STLReader::readBinaryData(std::ifstream& file, void* data, size_t size)
{
    file.read(static_cast<char*>(data), size);
    return file.good();
}
