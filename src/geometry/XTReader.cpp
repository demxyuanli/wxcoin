#include "XTReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE X_T import includes
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <ShapeFix_Shape.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepTools_ReShape.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ConstructionError.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

// File system includes
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cfloat>
#include <chrono>
#include <thread>
#include <execution>
#include <sstream>

// Static member initialization
std::unordered_map<std::string, XTReader::ReadResult> XTReader::s_cache;
std::mutex XTReader::s_cacheMutex;
bool XTReader::s_initialized = false;

XTReader::ReadResult XTReader::readFile(const std::string& filePath,
    const OptimizationOptions& options,
    ProgressCallback progress)
{
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    ReadResult result;
    result.formatName = "X_T";

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
            result.errorMessage = "File is not an X_T file: " + filePath;
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

        // Initialize X_T reader
        initialize();
        if (progress) progress(5, "Initializing X_T reader");

        // Parse X_T file
        std::vector<TopoDS_Shape> shapes;
        if (!parseXTFile(filePath, shapes, progress)) {
            result.errorMessage = "Failed to parse X_T file: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        if (shapes.empty()) {
            result.errorMessage = "No valid shapes found in X_T file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Process shapes
        std::string baseName = std::filesystem::path(filePath).stem().string();
        result.geometries = processShapesParallel(shapes, baseName, options, progress);

        if (result.geometries.empty()) {
            result.errorMessage = "No valid geometries could be created from X_T file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Set root shape
        if (shapes.size() > 1) {
            BRep_Builder builder;
            TopoDS_Compound compound;
            builder.MakeCompound(compound);
            for (const auto& s : shapes) {
                builder.Add(compound, s);
            }
            result.rootShape = compound;
        } else if (!shapes.empty()) {
            result.rootShape = shapes[0];
        }

        result.success = true;

        // Cache result if enabled
        if (options.enableCaching) {
            std::lock_guard<std::mutex> lock(s_cacheMutex);
            s_cache[filePath] = result;
        }

        auto totalEndTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEndTime - totalStartTime);
        result.importTime = static_cast<double>(totalDuration.count());

        LOG_INF_S("X_T file imported successfully: " + std::to_string(result.geometries.size()) + 
                 " geometries in " + std::to_string(result.importTime) + "ms");

        return result;
    }
    catch (const std::exception& e) {
        result.errorMessage = "Exception during X_T import: " + std::string(e.what());
        LOG_ERR_S(result.errorMessage);
        return result;
    }
}

bool XTReader::isValidFile(const std::string& filePath) const
{
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".x_t" || extension == ".xmt_txt";
}

std::vector<std::string> XTReader::getSupportedExtensions() const
{
    return {".x_t", ".xmt_txt"};
}

std::string XTReader::getFormatName() const
{
    return "X_T";
}

std::string XTReader::getFileFilter() const
{
    return "X_T files (*.x_t;*.xmt_txt)|*.x_t;*.xmt_txt";
}

void XTReader::initialize()
{
    if (s_initialized) {
        return;
    }
    
    try {
        // Initialize X_T reader if needed
        s_initialized = true;
        LOG_INF_S("X_T reader initialized");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to initialize X_T reader: " + std::string(e.what()));
        throw;
    }
}

bool XTReader::parseHeader(const std::string& filePath, ProgressCallback progress)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERR_S("Cannot open X_T file: " + filePath);
        return false;
    }

    std::string line;
    if (std::getline(file, line)) {
        // Check for X_T file signature
        if (line.find("Parasolid") != std::string::npos || 
            line.find("xmt_txt") != std::string::npos ||
            line.find("x_t") != std::string::npos) {
            LOG_INF_S("Detected X_T file format");
            return true;
        }
    }

    file.close();
    LOG_WRN_S("Could not identify X_T file format");
    return false;
}

bool XTReader::parseXTFile(const std::string& filePath,
    std::vector<TopoDS_Shape>& shapes,
    ProgressCallback progress)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERR_S("Cannot open X_T file: " + filePath);
        return false;
    }

    std::string line;
    int lineNumber = 0;
    int totalLines = 0;

    // First pass: count lines for progress
    std::ifstream countFile(filePath);
    while (std::getline(countFile, line)) {
        totalLines++;
    }
    countFile.close();

    file.open(filePath);
    if (!file.is_open()) {
        return false;
    }

    // Parse header
    if (!parseHeader(filePath, progress)) {
        LOG_WRN_S("Could not parse X_T header, continuing with basic parsing");
    }

    // Parse file content
    while (std::getline(file, line)) {
        lineNumber++;
        
        if (progress && lineNumber % 1000 == 0) {
            int percent = 10 + static_cast<int>((lineNumber * 40.0) / totalLines);
            progress(percent, "Parsing line " + std::to_string(lineNumber) + "/" + std::to_string(totalLines));
        }

        // Remove leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse line
        if (!parseLine(line, shapes)) {
            // Continue parsing even if one line fails
            continue;
        }
    }

    file.close();
    
    if (shapes.empty()) {
        LOG_WRN_S("No shapes found in X_T file - file may be corrupted or use unsupported format");
        return false;
    }
    
    return true;
}

void XTReader::extractShapes(const TopoDS_Shape& compound, std::vector<TopoDS_Shape>& shapes)
{
    if (compound.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer explorer(compound, TopAbs_SOLID);
        for (; explorer.More(); explorer.Next()) {
            shapes.push_back(explorer.Current());
        }
        
        // If no solids found, try shells
        if (shapes.empty()) {
            explorer.Init(compound, TopAbs_SHELL);
            for (; explorer.More(); explorer.Next()) {
                shapes.push_back(explorer.Current());
            }
        }
        
        // If still no shapes found, try faces
        if (shapes.empty()) {
            explorer.Init(compound, TopAbs_FACE);
            for (; explorer.More(); explorer.Next()) {
                shapes.push_back(explorer.Current());
            }
        }
        
        // If still no shapes found, try edges
        if (shapes.empty()) {
            explorer.Init(compound, TopAbs_EDGE);
            for (; explorer.More(); explorer.Next()) {
                shapes.push_back(explorer.Current());
            }
        }
        
        // If still no shapes found, try vertices
        if (shapes.empty()) {
            explorer.Init(compound, TopAbs_VERTEX);
            for (; explorer.More(); explorer.Next()) {
                shapes.push_back(explorer.Current());
            }
        }
    } else {
        shapes.push_back(compound);
    }
}

std::vector<std::shared_ptr<OCCGeometry>> XTReader::processShapesParallel(
    const std::vector<TopoDS_Shape>& shapes,
    const std::string& baseName,
    const OptimizationOptions& options,
    ProgressCallback progress)
{
    std::vector<std::shared_ptr<OCCGeometry>> geometries;
    
    if (options.enableParallelProcessing && shapes.size() > 1) {
        // Process shapes in parallel
        std::vector<std::future<std::shared_ptr<OCCGeometry>>> futures;
        
        for (size_t i = 0; i < shapes.size(); ++i) {
            std::string name = baseName + "_" + std::to_string(i + 1);
            futures.push_back(std::async(std::launch::async, [this, &shapes, i, name, &options]() {
                return processSingleShape(shapes[i], name, options);
            }));
        }
        
        // Collect results
        for (auto& future : futures) {
            auto geometry = future.get();
            if (geometry) {
                geometries.push_back(geometry);
            }
        }
    } else {
        // Process shapes sequentially
        for (size_t i = 0; i < shapes.size(); ++i) {
            std::string name = baseName + "_" + std::to_string(i + 1);
            auto geometry = processSingleShape(shapes[i], name, options);
            if (geometry) {
                geometries.push_back(geometry);
            }
            
            if (progress) {
                int percent = 50 + static_cast<int>((i + 1) * 40.0 / shapes.size());
                progress(percent, "Processing shape " + std::to_string(i + 1) + "/" + std::to_string(shapes.size()));
            }
        }
    }
    
    return geometries;
}

std::shared_ptr<OCCGeometry> XTReader::processSingleShape(
    const TopoDS_Shape& shape,
    const std::string& name,
    const OptimizationOptions& options)
{
    try {
        return createGeometryFromShape(shape, name, options);
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to process shape " + name + ": " + std::string(e.what()));
        return nullptr;
    }
}

TopoDS_Shape XTReader::fixShape(const TopoDS_Shape& shape)
{
    try {
        // Check if shape needs fixing
        BRepCheck_Analyzer analyzer(shape);
        if (analyzer.IsValid()) {
            return shape; // Shape is already valid
        }

        // Fix the shape
        ShapeFix_Shape fixer(shape);
        fixer.Perform();
        
        TopoDS_Shape fixedShape = fixer.Shape();
        if (!fixedShape.IsNull()) {
            LOG_INF_S("Shape fixed successfully");
            return fixedShape;
        } else {
            LOG_WRN_S("Shape fixing failed");
            return shape; // Return original shape
        }
    }
    catch (const std::exception& e) {
        LOG_WRN_S("Failed to fix shape: " + std::string(e.what()));
        return shape; // Return original shape if fixing fails
    }
}

bool XTReader::parseLine(const std::string& line, std::vector<TopoDS_Shape>& shapes)
{
    try {
        // This is a simplified X_T parser
        // Real X_T files are much more complex and require specialized parsing
        
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        
        // Look for geometric entities
        if (token == "body" || token == "solid" || token == "shell" || token == "face") {
            // For now, we'll create a placeholder shape
            // In a real implementation, you would parse the geometric data
            // and create appropriate OpenCASCADE shapes
            
            LOG_DBG_S("Found geometric entity: " + token);
            // Placeholder - in real implementation, parse and create shape
            return true;
        }
        
        return true; // Continue parsing
    }
    catch (const std::exception& e) {
        LOG_WRN_S("Error parsing X_T line: " + std::string(e.what()));
        return false;
    }
}
