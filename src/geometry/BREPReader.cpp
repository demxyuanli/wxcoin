#include "BREPReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE BREP import includes
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

// Static member initialization
std::unordered_map<std::string, BREPReader::ReadResult> BREPReader::s_cache;
std::mutex BREPReader::s_cacheMutex;
bool BREPReader::s_initialized = false;

BREPReader::ReadResult BREPReader::readFile(const std::string& filePath,
    const OptimizationOptions& options,
    ProgressCallback progress)
{
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    ReadResult result;
    result.formatName = "BREP";

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
            result.errorMessage = "File is not a BREP file: " + filePath;
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

        // Initialize BREP reader
        initialize();
        if (progress) progress(5, "Initializing BREP reader");

        // Read BREP file - simplified implementation
        // TODO: Fix BRepTools::Read API compatibility
        result.errorMessage = "BREP format support is temporarily disabled due to API compatibility issues";
        LOG_ERR_S(result.errorMessage);
        return result;
    }
    catch (const std::exception& e) {
        result.errorMessage = "Exception in BREP reader: " + std::string(e.what());
        LOG_ERR_S(result.errorMessage);
        return result;
    }
}

bool BREPReader::isValidFile(const std::string& filePath) const
{
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".brep";
}

std::vector<std::string> BREPReader::getSupportedExtensions() const
{
    return {".brep"};
}

std::string BREPReader::getFormatName() const
{
    return "BREP";
}

std::string BREPReader::getFileFilter() const
{
    return "BREP files (*.brep)|*.brep";
}

void BREPReader::initialize()
{
    if (s_initialized) {
        return;
    }
    
    try {
        // Initialize BREP reader if needed
        s_initialized = true;
        LOG_INF_S("BREP reader initialized");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to initialize BREP reader: " + std::string(e.what()));
        throw;
    }
}

void BREPReader::extractShapes(const TopoDS_Shape& compound, std::vector<TopoDS_Shape>& shapes)
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

std::vector<std::shared_ptr<OCCGeometry>> BREPReader::processShapesParallel(
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
            futures.push_back(std::async(std::launch::async, [this, &shapes, i, name, baseName, &options]() {
                return processSingleShape(shapes[i], name, baseName, options);
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
            auto geometry = processSingleShape(shapes[i], name, baseName, options);
            if (geometry) {
                geometries.push_back(geometry);
            }
            
            if (progress) {
                int percent = 40 + static_cast<int>((i + 1) * 40.0 / shapes.size());
                progress(percent, "Processing shape " + std::to_string(i + 1) + "/" + std::to_string(shapes.size()));
            }
        }
    }
    
    return geometries;
}

std::shared_ptr<OCCGeometry> BREPReader::processSingleShape(
    const TopoDS_Shape& shape,
    const std::string& name,
    const std::string& baseName,
    const OptimizationOptions& options)
{
    try {
        return createGeometryFromShape(shape, name, baseName, options);
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to process shape " + name + ": " + std::string(e.what()));
        return nullptr;
    }
}

TopoDS_Shape BREPReader::fixShape(const TopoDS_Shape& shape)
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
