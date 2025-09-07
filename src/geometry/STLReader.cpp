#include "STLReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE includes
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopoDS_Wire.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeFace.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakePolygon.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
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
#include <execution>
#include <future>
#include <numeric>

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
    // Read entire file into memory for faster processing
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERR_S("Cannot open STL file: " + filePath);
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read entire file into buffer
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    // Estimate triangle count (roughly 7 lines per triangle)
    size_t estimatedTriangles = fileSize / 200; // Conservative estimate
    triangles.reserve(estimatedTriangles);

    // Parse buffer
    const char* data = buffer.data();
    const char* end = data + fileSize;
    
    Triangle currentTriangle;
    int vertexCount = 0;
    size_t processedBytes = 0;
    
    while (data < end) {
        // Find next line
        const char* lineStart = data;
        while (data < end && *data != '\n' && *data != '\r') {
            data++;
        }
        
        // Skip line endings
        while (data < end && (*data == '\n' || *data == '\r')) {
            data++;
        }
        
        // Process line
        size_t lineLength = data - lineStart;
        if (lineLength == 0) continue;
        
        // Skip whitespace
        while (lineStart < data && (*lineStart == ' ' || *lineStart == '\t')) {
            lineStart++;
        }
        
        if (lineStart >= data) continue;
        
        // Parse keywords efficiently
        if (strncmp(lineStart, "facet normal", 12) == 0) {
            // Parse normal values directly
            const char* normalStart = lineStart + 12;
            double nx, ny, nz;
            if (sscanf(normalStart, "%lf %lf %lf", &nx, &ny, &nz) == 3) {
                currentTriangle.normal = gp_Vec(nx, ny, nz);
            }
        }
        else if (strncmp(lineStart, "vertex", 6) == 0) {
            // Parse vertex coordinates directly
            const char* vertexStart = lineStart + 6;
            double x, y, z;
            if (sscanf(vertexStart, "%lf %lf %lf", &x, &y, &z) == 3) {
                currentTriangle.vertices[vertexCount] = gp_Pnt(x, y, z);
                vertexCount++;
                
                if (vertexCount == 3) {
                    triangles.push_back(currentTriangle);
                    vertexCount = 0;
                }
            }
        }
        
        // Update progress
        processedBytes += lineLength;
        if (progress && processedBytes % (fileSize / 100) == 0) {
            int percent = 20 + static_cast<int>((processedBytes * 30.0) / fileSize);
            progress(percent, "Parsing ASCII STL");
        }
    }

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

    // Read entire file into memory for faster processing
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();
    
    const char* data = buffer.data();
    
    // Skip header (80 bytes)
    data += 80;
    
    // Read triangle count (4 bytes)
    uint32_t triangleCount;
    memcpy(&triangleCount, data, sizeof(uint32_t));
    data += sizeof(uint32_t);
    
    triangles.reserve(triangleCount);
    
    // Calculate triangle data size (normal + vertices + attribute)
    const size_t triangleDataSize = 3 * sizeof(float) + 9 * sizeof(float) + sizeof(uint16_t);
    
    // Process triangles in batches for better performance
    const size_t batchSize = (triangleCount < 1000) ? triangleCount : 1000;

    for (uint32_t i = 0; i < triangleCount; i += batchSize) {
        size_t currentBatchSize = (batchSize < triangleCount - i) ? batchSize : (triangleCount - i);
        
        // Process batch
        for (size_t j = 0; j < currentBatchSize; ++j) {
            Triangle triangle;
            
            // Read normal (3 floats)
            float normal[3];
            memcpy(normal, data, 3 * sizeof(float));
            data += 3 * sizeof(float);
            triangle.normal = gp_Vec(normal[0], normal[1], normal[2]);
            
            // Read vertices (9 floats)
            float vertices[9];
            memcpy(vertices, data, 9 * sizeof(float));
            data += 9 * sizeof(float);
            
            triangle.vertices[0] = gp_Pnt(vertices[0], vertices[1], vertices[2]);
            triangle.vertices[1] = gp_Pnt(vertices[3], vertices[4], vertices[5]);
            triangle.vertices[2] = gp_Pnt(vertices[6], vertices[7], vertices[8]);
            
            triangles.push_back(triangle);
            
            // Skip attribute byte count (2 bytes)
            data += sizeof(uint16_t);
        }
        
        // Update progress
        if (progress) {
            int percent = 20 + static_cast<int>(((i + currentBatchSize) * 30.0) / triangleCount);
            progress(percent, "Reading binary STL");
        }
    }
    
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

        // Pre-allocate face list for better performance
        std::vector<TopoDS_Face> faces;
        faces.reserve(triangles.size());
        
        int validFaces = 0;
        int correctNormals = 0;
        int incorrectNormals = 0;
        int trianglesWithValidNormals = 0;
        int trianglesWithInvalidNormals = 0;

        // Process triangles in parallel batches
        const size_t batchSize = (triangles.size() < 1000) ? triangles.size() : 1000;
        const size_t numBatches = (triangles.size() + batchSize - 1) / batchSize;
        
        std::vector<std::vector<TopoDS_Face>> batchFaces(numBatches);
        std::vector<std::vector<int>> batchStats(numBatches);
        
        // Process batches in parallel
        std::vector<std::future<void>> futures;
        
        for (size_t batchIdx = 0; batchIdx < numBatches; ++batchIdx) {
            futures.push_back(std::async(std::launch::async, [&, batchIdx]() {
                size_t startIdx = batchIdx * batchSize;
                size_t endIdx = (startIdx + batchSize < triangles.size()) ? startIdx + batchSize : triangles.size();
                
                batchFaces[batchIdx].reserve(endIdx - startIdx);
                batchStats[batchIdx].resize(5, 0); // 5 counters
                
                for (size_t i = startIdx; i < endIdx; ++i) {
                    const auto& triangle = triangles[i];
                    
                    // Check STL triangle normal direction
                    if (triangle.normal.Magnitude() > 1e-6) {
                        batchStats[batchIdx][0]++; // trianglesWithValidNormals
                        
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
                            batchStats[batchIdx][1]++; // correctNormals
                        } else {
                            batchStats[batchIdx][2]++; // incorrectNormals
                        }
                    } else {
                        batchStats[batchIdx][3]++; // trianglesWithInvalidNormals
                    }

                    TopoDS_Face faceShape = createFaceFromTriangle(triangle);
                    if (!faceShape.IsNull()) {
                        batchFaces[batchIdx].push_back(faceShape);
                        batchStats[batchIdx][4]++; // validFaces
                    }
                }
            }));
        }
        
        // Wait for all batches to complete
        for (auto& future : futures) {
            future.wait();
        }
        
        // Merge results
        for (size_t batchIdx = 0; batchIdx < numBatches; ++batchIdx) {
            trianglesWithValidNormals += batchStats[batchIdx][0];
            correctNormals += batchStats[batchIdx][1];
            incorrectNormals += batchStats[batchIdx][2];
            trianglesWithInvalidNormals += batchStats[batchIdx][3];
            validFaces += batchStats[batchIdx][4];
            
            // Add faces to compound
            for (const auto& face : batchFaces[batchIdx]) {
                builder.Add(compound, face);
            }
        }

        // Output normal statistics (reduced logging)
        LOG_INF_S("STL processed: " + std::to_string(triangles.size()) + " triangles, " + 
                 std::to_string(validFaces) + " valid faces");
        
        if (correctNormals + incorrectNormals > 0) {
            double correctPercentage = (static_cast<double>(correctNormals) / (correctNormals + incorrectNormals)) * 100.0;
            LOG_INF_S("Normal correctness: " + std::to_string(correctPercentage) + "%");
        }

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

TopoDS_Face STLReader::createFaceFromTriangle(const Triangle& triangle)
{
    try {
        // Create polygon from triangle vertices
        BRepBuilderAPI_MakePolygon polygon;
        polygon.Add(triangle.vertices[0]);
        polygon.Add(triangle.vertices[1]);
        polygon.Add(triangle.vertices[2]);
        polygon.Close();
        
        if (!polygon.IsDone()) {
            return TopoDS_Face();
        }

        // Create wire from polygon
        TopoDS_Wire wire = polygon.Wire();
        
        // Create face from wire
        BRepBuilderAPI_MakeFace faceMaker(wire);

        if (!faceMaker.IsDone()) {
            return TopoDS_Face();
        }

        TopoDS_Face face = faceMaker.Face();

        // Use the triangle normal to ensure correct face orientation
        // STL files provide explicit normals for each triangle
        if (triangle.normal.Magnitude() > 1e-6) {
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
                
                // Check if normals are consistent
                double dotProduct = calculatedNormal.Dot(triangle.normal);
                
                // If dot product is negative, the face is oriented incorrectly
                if (dotProduct < 0) {
                    face.Reverse();
                }
            }
        }

        return face;
    }
    catch (const std::exception& e) {
        // Reduced logging for performance
        return TopoDS_Face();
    }
}

bool STLReader::readBinaryData(std::ifstream& file, void* data, size_t size)
{
    file.read(static_cast<char*>(data), size);
    return file.good();
}

