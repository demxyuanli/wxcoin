#include "STEPReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"

// OpenCASCADE STEP import includes
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <Interface_Static.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <APIHeaderSection_MakeHeader.hxx>
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

// File system includes
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cfloat>
#include <chrono>
#include <thread>
#include <execution>

// Add include at top
#include <XCAFApp_Application.hxx>

// Static member initialization
std::unordered_map<std::string, STEPReader::ReadResult> STEPReader::s_cache;
std::mutex STEPReader::s_cacheMutex;
STEPReader::OptimizationOptions STEPReader::s_globalOptions;
bool STEPReader::s_initialized = false;

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
        
        // Check cache if enabled
        if (options.enableCaching) {
            std::lock_guard<std::mutex> lock(s_cacheMutex);
            auto cacheIt = s_cache.find(filePath);
            if (cacheIt != s_cache.end()) {
                result = cacheIt->second;
                return result;
            }
        }
        
        // Initialize STEP reader
        initialize();
        if (progress) progress(5, "initialize");
        
        // Use optimized STEP reader settings
        STEPControl_Reader reader;
        Interface_Static::SetIVal("read.precision.mode", 1);
        Interface_Static::SetRVal("read.precision.val", options.precision);
        
        // Set additional optimization parameters
        Interface_Static::SetIVal("read.step.optimize", 1);
        Interface_Static::SetIVal("read.step.fast_mode", 1);
        
        IFSelect_ReturnStatus status = reader.ReadFile(filePath.c_str());
        
        if (status != IFSelect_RetDone) {
            result.errorMessage = "Failed to read STEP file: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }
        if (progress) progress(20, "read");
        
        // Transfer shapes
        Standard_Integer nbRoots = reader.NbRootsForTransfer();
        if (nbRoots == 0) {
            result.errorMessage = "No transferable entities found in STEP file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }
        
        reader.TransferRoots();
        Standard_Integer nbShapes = reader.NbShapes();
        if (progress) progress(35, "transfer");
        
        if (nbShapes == 0) {
            result.errorMessage = "No shapes could be transferred from STEP file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }
        
        // Create compound shape containing all shapes
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        for (Standard_Integer i = 1; i <= nbShapes; i++) {
            TopoDS_Shape shape = reader.Shape(i);
            if (!shape.IsNull()) {
                builder.Add(compound, shape);
            }
        }
        result.rootShape = compound;
        if (progress) progress(45, "assemble");
        
        // Convert to geometry objects with optimization
        std::string baseName = std::filesystem::path(filePath).stem().string();
        result.geometries = shapeToGeometries(result.rootShape, baseName, options, progress, 50, 40);
        
        // Apply automatic scaling to make geometries reasonable size
        if (!result.geometries.empty()) {
            double scaleFactor = scaleGeometriesToReasonableSize(result.geometries);
        }
        if (progress) progress(92, "postprocess");
        
        // Cache result if enabled
        if (options.enableCaching) {
            std::lock_guard<std::mutex> lock(s_cacheMutex);
            s_cache[filePath] = result;
        }
        
        result.success = true;
        
        // Calculate total import time
        auto totalEndTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEndTime - totalStartTime);
        result.importTime = static_cast<double>(totalDuration.count());
        if (progress) progress(100, "done");
        
    } catch (const Standard_Failure& e) {
        result.errorMessage = "OpenCASCADE exception: " + std::string(e.GetMessageString());
        LOG_ERR_S(result.errorMessage);
    } catch (const std::exception& e) {
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

std::vector<std::string> STEPReader::getSupportedExtensions()
{
    return {"*.step", "*.stp", "*.STEP", "*.STP"};
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
        
        // Use parallel processing if enabled and there are multiple shapes
        if (options.enableParallelProcessing && shapes.size() > 1) {
            geometries = processShapesParallel(shapes, baseName, options, progress, progressStart, progressSpan);
        } else {
            // Sequential processing with progress
            size_t total = shapes.size();
            for (size_t i = 0; i < shapes.size(); ++i) {
                if (!shapes[i].IsNull()) {
                    std::string name = baseName + "_" + std::to_string(i);
                    auto geometry = processSingleShape(shapes[i], name, options);
                    if (geometry) {
                        geometries.push_back(geometry);
                    }
                }
                if (progress && total > 0) {
                    int pct = progressStart + (int)std::round(((double)(i + 1) / (double)total) * progressSpan);
                    pct = std::max(progressStart, std::min(progressStart + progressSpan, pct));
                    progress(pct, "convert");
                }
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception converting shape to geometries: " + std::string(e.what()));
    }
    
    return geometries;
}

std::vector<std::shared_ptr<OCCGeometry>> STEPReader::processShapesParallel(
    const std::vector<TopoDS_Shape>& shapes,
    const std::string& baseName,
    const OptimizationOptions& options,
    ProgressCallback progress,
    int progressStart,
    int progressSpan)
{
    std::vector<std::shared_ptr<OCCGeometry>> geometries;
    geometries.reserve(shapes.size());
    
    // Create futures for parallel processing
    std::vector<std::future<std::shared_ptr<OCCGeometry>>> futures;
    futures.reserve(shapes.size());
    
    // Submit tasks to thread pool
    for (size_t i = 0; i < shapes.size(); ++i) {
        if (!shapes[i].IsNull()) {
            std::string name = baseName + "_" + std::to_string(i);
            futures.push_back(std::async(std::launch::async, 
                [&shapes, i, &name, &options]() {
                    return processSingleShape(shapes[i], name, options);
                }));
        }
    }
    
    // Collect results with progress
    size_t total = futures.size();
    for (size_t idx = 0; idx < futures.size(); ++idx) {
        auto geometry = futures[idx].get();
        if (geometry) {
            geometries.push_back(geometry);
        }
        if (progress && total > 0) {
            int pct = progressStart + (int)std::round(((double)(idx + 1) / (double)total) * progressSpan);
            pct = std::max(progressStart, std::min(progressStart + progressSpan, pct));
            progress(pct, "convert");
        }
    }
    
    return geometries;
}

std::shared_ptr<OCCGeometry> STEPReader::processSingleShape(
    const TopoDS_Shape& shape,
    const std::string& name,
    const OptimizationOptions& options)
{
    if (shape.IsNull()) {
        return nullptr;
    }
    
    try {
        auto geometry = std::make_shared<OCCGeometry>(name);
        geometry->setShape(shape);
        
        // Set better default color for imported STEP models
        Quantity_Color defaultColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        geometry->setColor(defaultColor);
        
        // Remove transparency for a solid appearance
        geometry->setTransparency(0.0);
        
        // Only analyze shape if explicitly enabled (disabled by default for speed)
        if (options.enableShapeAnalysis) {
            OCCShapeBuilder::analyzeShapeTopology(shape, name);
        }
        
        return geometry;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception processing shape " + name + ": " + std::string(e.what()));
        return nullptr;
    }
}

void STEPReader::initialize()
{
    // Set up STEP reader parameters
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
    } else {
        // It's a single shape
        shapes.push_back(compound);
    }
}

void STEPReader::clearCache()
{
    std::lock_guard<std::mutex> lock(s_cacheMutex);
    s_cache.clear();
    LOG_INF_S("STEP import cache cleared");
}

std::string STEPReader::getCacheStats()
{
    std::lock_guard<std::mutex> lock(s_cacheMutex);
    return "Cache entries: " + std::to_string(s_cache.size());
}

void STEPReader::setGlobalOptimizationOptions(const OptimizationOptions& options)
{
    s_globalOptions = options;
    LOG_INF_S("Global STEP optimization options updated");
}

STEPReader::OptimizationOptions STEPReader::getGlobalOptimizationOptions()
{
    return s_globalOptions;
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
    
    // Use parallel processing for large geometry sets
    if (geometries.size() > 10) {
        std::vector<gp_Pnt> minPoints(geometries.size());
        std::vector<gp_Pnt> maxPoints(geometries.size());
        
        // Initialize with invalid bounds
        for (size_t i = 0; i < geometries.size(); ++i) {
            minPoints[i] = gp_Pnt(DBL_MAX, DBL_MAX, DBL_MAX);
            maxPoints[i] = gp_Pnt(-DBL_MAX, -DBL_MAX, -DBL_MAX);
        }
        
        // Process in parallel
        std::for_each(std::execution::par, geometries.begin(), geometries.end(),
            [&](const auto& geometry) {
                size_t index = &geometry - &geometries[0];
                if (geometry && !geometry->getShape().IsNull()) {
                    gp_Pnt localMin, localMax;
                    OCCShapeBuilder::getBoundingBox(geometry->getShape(), localMin, localMax);
                    minPoints[index] = localMin;
                    maxPoints[index] = localMax;
                }
            });
        
        // Combine results
        for (size_t i = 0; i < geometries.size(); ++i) {
            if (minPoints[i].X() != DBL_MAX) { // Valid bounds
                if (minPoints[i].X() < minPt.X()) minPt.SetX(minPoints[i].X());
                if (minPoints[i].Y() < minPt.Y()) minPt.SetY(minPoints[i].Y());
                if (minPoints[i].Z() < minPt.Z()) minPt.SetZ(minPoints[i].Z());
                
                if (maxPoints[i].X() > maxPt.X()) maxPt.SetX(maxPoints[i].X());
                if (maxPoints[i].Y() > maxPt.Y()) maxPt.SetY(maxPoints[i].Y());
                if (maxPoints[i].Z() > maxPt.Z()) maxPt.SetZ(maxPoints[i].Z());
                
                hasValidBounds = true;
            }
        }
    } else {
        // Sequential processing for small geometry sets
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
        // Use optimized bounding box calculation
        gp_Pnt overallMin, overallMax;
        if (!calculateCombinedBoundingBox(geometries, overallMin, overallMax)) {
            LOG_WRN_S("No valid bounds found for scaling");
            return 1.0;
        }
        
        // Calculate current size
        double currentSizeX = overallMax.X() - overallMin.X();
        double currentSizeY = overallMax.Y() - overallMin.Y();
        double currentSizeZ = overallMax.Z() - overallMin.Z();
        double currentMaxSize = (std::max)({currentSizeX, currentSizeY, currentSizeZ});
        
        // Determine target size
        if (targetSize <= 0.0) {
            // Auto-detect reasonable size (10-50 units)
            if (currentMaxSize > 100.0) {
                targetSize = 20.0;  // Scale large models down
            } else if (currentMaxSize < 0.1) {
                targetSize = 10.0;  // Scale tiny models up
            } else {
                // Size is already reasonable
                return 1.0;
            }
        }
        
        double scaleFactor = targetSize / currentMaxSize;
        
        if (std::abs(scaleFactor - 1.0) < 0.01) {
            // No significant scaling needed
            return 1.0;
        }
        
        // Apply scaling in parallel for large geometry sets
        if (geometries.size() > 5) {
            std::for_each(std::execution::par, geometries.begin(), geometries.end(),
                [scaleFactor](auto& geometry) {
                    if (geometry && !geometry->getShape().IsNull()) {
                        TopoDS_Shape scaledShape = OCCShapeBuilder::scale(
                            geometry->getShape(), 
                            gp_Pnt(0, 0, 0), 
                            scaleFactor
                        );
                        if (!scaledShape.IsNull()) {
                            geometry->setShape(scaledShape);
                        }
                    }
                });
        } else {
            // Sequential scaling for small geometry sets
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
        }
        
        return scaleFactor;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception scaling geometries: " + std::string(e.what()));
        return 1.0;
    }
} 
