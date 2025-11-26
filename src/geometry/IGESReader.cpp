#include "IGESReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"
#include "STEPGeometryConverter.h"
#include "STEPGeometryDecomposer.h"
#include "STEPColorManager.h"

// OpenCASCADE IGES import includes
#include <IGESControl_Reader.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <IGESData_IGESModel.hxx>
#include <IGESToBRep_Actor.hxx>
#include <Interface_Static.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Label.hxx>
#include <TDataStd_Name.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TCollection_AsciiString.hxx>
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

        // Use IGESCAFControl_Reader following FreeCAD approach for color/name/layer support
        IGESCAFControl_Reader aReader;
        
        // Configure reader settings following FreeCAD implementation
        // Skip blank entities (default: true)
        aReader.SetReadVisible(true);
        // Enable color, name, and layer modes
        aReader.SetColorMode(true);
        aReader.SetNameMode(true);
        aReader.SetLayerMode(true);

        // Set precision options
        Interface_Static::SetIVal("read.precision.mode", 1);
        Interface_Static::SetRVal("read.precision.val", options.precision);

        // Read the file
        std::string name8bit = filePath;  // On Windows, UTF-8 paths should work directly
        IFSelect_ReturnStatus status = aReader.ReadFile(name8bit.c_str());

        if (status != IFSelect_RetDone) {
            result.errorMessage = "Cannot read IGES file: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }
        if (progress) progress(20, "Reading IGES file");

        // Create XCAF document for transfer
        Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
        if (app.IsNull()) {
            result.errorMessage = "Failed to create XCAF application";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        Handle(TDocStd_Document) hDoc;
        app->NewDocument("MDTV-XCAF", hDoc);
        if (hDoc.IsNull()) {
            result.errorMessage = "Failed to create XCAF document";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        if (progress) progress(30, "Creating document");

        // Transfer to document
        aReader.Transfer(hDoc);

        // Memory leak fix following FreeCAD approach
        // http://opencascade.blogspot.de/2009/03/unnoticeable-memory-leaks-part-2.html
        Handle(IGESToBRep_Actor) actor = Handle(IGESToBRep_Actor)::DownCast(aReader.WS()->TransferReader()->Actor());
        if (!actor.IsNull()) {
            actor->SetModel(new IGESData_IGESModel);
        }

        if (progress) progress(40, "Transferring shapes");

        // Get shape and color tools
        Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
        Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(hDoc->Main());

        if (shapeTool.IsNull()) {
            result.errorMessage = "Failed to get shape tool from CAF document";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Extract all free shapes (top-level shapes)
        TDF_LabelSequence labels;
        shapeTool->GetFreeShapes(labels);

        if (labels.Length() == 0) {
            result.errorMessage = "No shapes found in IGES file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        if (progress) progress(50, "Extracting shapes");

        // Extract shapes with colors and names
        std::vector<TopoDS_Shape> shapes;
        std::string baseName = std::filesystem::path(filePath).stem().string();
        
        for (Standard_Integer i = 1; i <= labels.Length(); ++i) {
            TDF_Label label = labels.Value(i);
            TopoDS_Shape shape;
            if (shapeTool->GetShape(label, shape) && !shape.IsNull()) {
                shapes.push_back(shape);
            }
        }

        if (shapes.empty()) {
            result.errorMessage = "No valid shapes could be extracted from IGES file";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        // Process shapes with color and name information
        result.geometries = processShapesWithCAF(shapes, labels, shapeTool, colorTool, 
                                                  baseName, options, progress);

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
        // Initialize IGES controller following FreeCAD approach
        IGESControl_Controller::Init();
        s_initialized = true;
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

std::vector<std::shared_ptr<OCCGeometry>> IGESReader::processShapesWithCAF(
    const std::vector<TopoDS_Shape>& shapes,
    const TDF_LabelSequence& labels,
    const Handle(XCAFDoc_ShapeTool)& shapeTool,
    const Handle(XCAFDoc_ColorTool)& colorTool,
    const std::string& baseName,
    const OptimizationOptions& options,
    ProgressCallback progress)
{
    std::vector<std::shared_ptr<OCCGeometry>> geometries;
    
    // Get color palette for decomposed components
    auto palette = STEPColorManager::getPaletteForScheme(options.decomposition.colorScheme);
    std::hash<std::string> hasher;
    size_t globalColorIndex = 0;  // Global counter across all shapes

    for (size_t i = 0; i < shapes.size() && i < static_cast<size_t>(labels.Length()); ++i) {
        TDF_Label label = labels.Value(static_cast<Standard_Integer>(i + 1));
        const TopoDS_Shape& shape = shapes[i];

        // Extract name from label
        std::string name = baseName + "_" + std::to_string(i + 1);
        Handle(TDataStd_Name) nameAttr;
        if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr)) {
            TCollection_ExtendedString extName = nameAttr->Get();
            // Convert extended string to std::string (simplified conversion)
            TCollection_AsciiString asciiName(extName);
            if (!asciiName.IsEmpty()) {
                name = std::string(asciiName.ToCString());
            }
        }

        // Extract color from label (following STEPCAFProcessor approach)
        Quantity_Color color;
        bool hasColor = false;
        if (!colorTool.IsNull()) {
            // Try to get color for the shape (general, surface, curve colors)
            hasColor = colorTool->GetColor(label, XCAFDoc_ColorGen, color) ||
                       colorTool->GetColor(label, XCAFDoc_ColorSurf, color) ||
                       colorTool->GetColor(label, XCAFDoc_ColorCurv, color);
            
            // If not found on label, try to get color from sub-shapes
            if (!hasColor) {
                // Try to get color from sub-shapes recursively
                TDF_LabelSequence subLabels;
                if (shapeTool->GetSubShapes(label, subLabels)) {
                    for (Standard_Integer j = 1; j <= subLabels.Length() && !hasColor; ++j) {
                        TDF_Label subLabel = subLabels.Value(j);
                        hasColor = colorTool->GetColor(subLabel, XCAFDoc_ColorGen, color) ||
                                   colorTool->GetColor(subLabel, XCAFDoc_ColorSurf, color) ||
                                   colorTool->GetColor(subLabel, XCAFDoc_ColorCurv, color);
                    }
                }
            }
            
            // Also try to get color directly from shape (not from label)
            if (!hasColor) {
                // Try to get color from the shape itself using XCAFDoc_ColorTool
                TDF_Label shapeLabel;
                if (shapeTool->FindShape(shape, shapeLabel)) {
                    hasColor = colorTool->GetColor(shapeLabel, XCAFDoc_ColorGen, color) ||
                               colorTool->GetColor(shapeLabel, XCAFDoc_ColorSurf, color) ||
                               colorTool->GetColor(shapeLabel, XCAFDoc_ColorCurv, color);
                }
            }
        }
        
        // Apply decomposition if enabled (including FACE_LEVEL)
        std::vector<TopoDS_Shape> decomposedShapes;
        if (options.decomposition.enableDecomposition) {
            decomposedShapes = STEPGeometryDecomposer::decomposeShape(shape, options);
        } else {
            decomposedShapes.push_back(shape);
        }

        // Process each decomposed shape
        for (size_t j = 0; j < decomposedShapes.size(); ++j) {
            const TopoDS_Shape& decomposedShape = decomposedShapes[j];
            
            // Generate name for decomposed component
            std::string componentName = name;
            if (decomposedShapes.size() > 1) {
                componentName = name + "_part_" + std::to_string(j + 1);
            }
            
            // Create geometry from decomposed shape
            auto geometry = createGeometryFromShape(decomposedShape, componentName, baseName, options);
            if (geometry) {
                // Apply color: use file color if found, otherwise use palette
                Quantity_Color componentColor;
                if (hasColor) {
                    // Use color from file
                    componentColor = color;
                } else {
                    // Use palette color for decomposed components
                    if (options.decomposition.enableDecomposition && options.decomposition.useConsistentColoring) {
                        // Hash-based consistent coloring
                        size_t idx = hasher(componentName) % palette.size();
                        componentColor = palette[idx];
                    } else {
                        // Sequential coloring - use global counter
                        componentColor = palette[globalColorIndex % palette.size()];
                    }
                }
                geometry->setColor(componentColor);
                globalColorIndex++;
                geometries.push_back(geometry);
            }
        }

        if (progress) {
            int percent = 50 + static_cast<int>((i + 1) * 40.0 / shapes.size());
            progress(percent, "Processing shape " + std::to_string(i + 1) + "/" + std::to_string(shapes.size()));
        }
    }

    return geometries;
}
