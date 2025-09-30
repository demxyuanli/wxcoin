#include "IGESReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE IGES import includes
#include <IGESControl_Reader.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <Interface_Static.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Label.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ConstructionError.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <ShapeFix_Shape.hxx>
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
std::unordered_map<std::string, IGESReader::ReadResult> IGESReader::s_cache;
std::mutex IGESReader::s_cacheMutex;
bool IGESReader::s_initialized = false;

IGESReader::ReadResult IGESReader::readFile(const std::string& filePath,
    const OptimizationOptions& options,
    ProgressCallback progress)
{
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    ReadResult result;
    result.formatName = "IGES";

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
            result.errorMessage = "File is not an IGES file: " + filePath;
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

        // Initialize IGES reader
        initialize();
        if (progress) progress(5, "Initializing IGES reader");

        // Use optimized IGES reader settings
        IGESControl_Reader reader;
        Interface_Static::SetIVal("read.precision.mode", 1);
        Interface_Static::SetRVal("read.precision.val", options.precision);

        // Set additional optimization parameters
        Interface_Static::SetIVal("read.iges.optimize", 1);
        Interface_Static::SetIVal("read.iges.fast_mode", 1);

        IFSelect_ReturnStatus status = reader.ReadFile(filePath.c_str());

        if (status != IFSelect_RetDone) {
            result.errorMessage = "Failed to read IGES file: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }
        if (progress) progress(20, "Reading IGES file");

        // Transfer shapes
        Standard_Integer nbRoots = reader.NbRootsForTransfer();
        if (nbRoots == 0) {
            result.errorMessage = "No transferable entities found in IGES file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        reader.TransferRoots();
        Standard_Integer nbShapes = reader.NbShapes();
        if (progress) progress(35, "Transferring shapes");

        if (nbShapes == 0) {
            result.errorMessage = "No shapes found in IGES file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Extract shapes
        std::vector<TopoDS_Shape> shapes;
        for (Standard_Integer i = 1; i <= nbShapes; ++i) {
            TopoDS_Shape shape = reader.Shape(i);
            if (!shape.IsNull()) {
                shapes.push_back(shape);
            }
        }

        if (progress) progress(50, "Processing shapes");

        // Process shapes
        std::string baseName = std::filesystem::path(filePath).stem().string();
        result.geometries = processShapesParallel(shapes, baseName, options, progress);

        if (result.geometries.empty()) {
            result.errorMessage = "No valid geometries could be created from IGES file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Create compound shape for root
        if (shapes.size() > 1) {
            BRep_Builder builder;
            TopoDS_Compound compound;
            builder.MakeCompound(compound);
            for (const auto& shape : shapes) {
                builder.Add(compound, shape);
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

        LOG_INF_S("IGES file imported successfully: " + std::to_string(result.geometries.size()) + 
                 " geometries in " + std::to_string(result.importTime) + "ms");

        return result;
    }
    catch (const std::exception& e) {
        result.errorMessage = "Exception during IGES import: " + std::string(e.what());
        LOG_ERR_S(result.errorMessage);
        return result;
    }
}

bool IGESReader::isValidFile(const std::string& filePath) const
{
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".iges" || extension == ".igs";
}

std::vector<std::string> IGESReader::getSupportedExtensions() const
{
    return {".iges", ".igs"};
}

std::string IGESReader::getFormatName() const
{
    return "IGES";
}

std::string IGESReader::getFileFilter() const
{
    return "IGES files (*.iges;*.igs)|*.iges;*.igs";
}

void IGESReader::initialize()
{
    if (s_initialized) {
        return;
    }
    
    try {
        // Initialize IGES reader if needed
        s_initialized = true;
        LOG_INF_S("IGES reader initialized");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to initialize IGES reader: " + std::string(e.what()));
        throw;
    }
}

void IGESReader::extractShapes(const TopoDS_Shape& compound, std::vector<TopoDS_Shape>& shapes)
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
    } else {
        shapes.push_back(compound);
    }
}

std::vector<std::shared_ptr<OCCGeometry>> IGESReader::processShapesParallel(
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
                LOG_INF_S("IGESReader: Created geometry '" + geometry->getName() + "' with filename '" + geometry->getFileName() + "'");
                geometries.push_back(geometry);
            }
        }
    } else {
        // Process shapes sequentially
        for (size_t i = 0; i < shapes.size(); ++i) {
            std::string name = baseName + "_" + std::to_string(i + 1);
            auto geometry = processSingleShape(shapes[i], name, baseName, options);
            if (geometry) {
                LOG_INF_S("IGESReader: Created geometry '" + name + "' with filename '" + geometry->getFileName() + "'");
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

std::shared_ptr<OCCGeometry> IGESReader::processSingleShape(
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
