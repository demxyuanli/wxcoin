#include "STEPCAFProcessor.h"
#include "STEPGeometryDecomposer.h"
#include "STEPGeometryConverter.h"
#include "STEPColorManager.h"
#include "STEPMetadataExtractor.h"
#include "OCCGeometry.h"
#include "STEPReader.h"
#include "logger/Logger.h"
#include "rendering/GeometryProcessor.h"

// OpenCASCADE includes
#include <STEPCAFControl_Reader.hxx>
#include <XCAFApp_Application.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <TDataStd_Name.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorType.hxx>
#include <TopLoc_Location.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <ShapeFix_Shell.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <functional>

STEPReader::ReadResult STEPCAFProcessor::processSTEPFileWithCAF(
    const std::string& filePath,
    const GeometryReader::OptimizationOptions& options,
    ProgressCallback progress)
{
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    STEPReader::ReadResult result;

    try {
        // Check if file exists and is valid STEP file
        if (!std::filesystem::exists(filePath)) {
            result.errorMessage = "File does not exist: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        if (!STEPReader::isSTEPFile(filePath)) {
            result.errorMessage = "File is not a STEP file: " + filePath;
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        if (progress) progress(5, "initialize CAF");

        // Initialize CAF reader and document
        Handle(TDocStd_Document) doc;
        STEPCAFControl_Reader cafReader;
        std::string errorMessage;

        if (!initializeCAFReader(filePath, doc, cafReader, errorMessage)) {
            result.errorMessage = errorMessage;
            return result;
        }

        if (progress) progress(10, "create document");

        // Read and transfer the STEP file
        if (!readAndTransferCAF(filePath, cafReader, doc, progress, errorMessage)) {
            result.errorMessage = errorMessage;
            return result;
        }

        // Get shape and color tools
        Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
        Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

        if (shapeTool.IsNull()) {
            result.errorMessage = "Failed to get shape tool from CAF document";
            LOG_ERR_S(result.errorMessage);
            return result;
        }

        if (progress) progress(60, "extract shapes");

        // Process assembly tree and extract components
        std::string baseName = std::filesystem::path(filePath).stem().string();
        auto palette = STEPColorManager::getPaletteForScheme(options.decomposition.colorScheme);
        std::hash<std::string> hasher;
        int componentIndex = 0;

        auto makeColorForName = [&](const std::string& name, const Quantity_Color* cafColor) -> Quantity_Color {
            // Always prioritize CAF color from STEP file if available
            if (cafColor) return *cafColor;
            // Fallback to consistent coloring or palette
            if (options.decomposition.useConsistentColoring) {
                size_t idx = hasher(name) % palette.size();
                return palette[idx];
            }
            return palette[componentIndex % palette.size()];
        };

        componentIndex = processAssemblyTree(shapeTool, colorTool, baseName, options,
                                           result.geometries, result.entityMetadata, componentIndex);

        if (progress) progress(80, "process components");

        // Build assembly structure summary
        result.assemblyStructure.name = baseName;
        result.assemblyStructure.type = "ASSEMBLY";
        for (const auto& entity : result.entityMetadata) {
            result.assemblyStructure.components.push_back(entity);
        }

        // Apply automatic scaling
        if (!result.geometries.empty()) {
            STEPGeometryConverter::scaleGeometriesToReasonableSize(result.geometries);
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
        result.errorMessage = "OpenCASCADE CAF exception: " + std::string(e.GetMessageString());
        LOG_ERR_S(result.errorMessage);
    }
    catch (const std::exception& e) {
        result.errorMessage = "Exception reading STEP file with CAF: " + std::string(e.what());
        LOG_ERR_S(result.errorMessage);
    }

    return result;
}

bool STEPCAFProcessor::initializeCAFReader(
    const std::string& filePath,
    Handle(TDocStd_Document)& doc,
    STEPCAFControl_Reader& cafReader,
    std::string& errorMessage)
{
    try {
        // Create XCAF application
        Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
        if (app.IsNull()) {
            errorMessage = "Failed to create XCAF application";
            LOG_ERR_S(errorMessage);
            return false;
        }

        // Create document
        app->NewDocument("MDTV-XCAF", doc);
        if (doc.IsNull()) {
            errorMessage = "Failed to create XCAF document";
            LOG_ERR_S(errorMessage);
            return false;
        }

        // Configure CAF reader - following FreeCAD approach for better color extraction
        cafReader.SetColorMode(true);   // Enable color reading
        cafReader.SetNameMode(true);    // Enable name reading
        cafReader.SetMatMode(true);     // Enable material reading
        cafReader.SetGDTMode(true);     // Enable GD&T reading
        cafReader.SetLayerMode(true);   // Enable layer reading
        cafReader.SetSHUOMode(true);    // Enable SHUO (Shape Usage Occurrence) for assembly instance colors

        return true;
    }
    catch (const std::exception& e) {
        errorMessage = "Exception initializing CAF reader: " + std::string(e.what());
        LOG_ERR_S(errorMessage);
        return false;
    }
}

bool STEPCAFProcessor::readAndTransferCAF(
    const std::string& filePath,
    STEPCAFControl_Reader& cafReader,
    Handle(TDocStd_Document)& doc,
    ProgressCallback progress,
    std::string& errorMessage)
{
    try {
        // Read the file
        IFSelect_ReturnStatus status = cafReader.ReadFile(filePath.c_str());
        if (status != IFSelect_RetDone) {
            errorMessage = "Failed to read STEP file with CAF: " + filePath +
                " (Status: " + std::to_string(static_cast<int>(status)) + ")";
            LOG_ERR_S(errorMessage);
            return false;
        }

        if (progress) progress(30, "read CAF");

        // Transfer all roots
        cafReader.Transfer(doc);
        if (progress) progress(50, "transfer CAF");

        return true;
    }
    catch (const std::exception& e) {
        errorMessage = "Exception reading and transferring CAF: " + std::string(e.what());
        LOG_ERR_S(errorMessage);
        return false;
    }
}

int STEPCAFProcessor::processAssemblyTree(
    const Handle(XCAFDoc_ShapeTool)& shapeTool,
    const Handle(XCAFDoc_ColorTool)& colorTool,
    const std::string& baseName,
    const GeometryReader::OptimizationOptions& options,
    std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    std::vector<STEPReader::STEPEntityInfo>& entityMetadata,
    int componentIndex)
{
    // Get all free shapes (top-level shapes)
    TDF_LabelSequence freeShapes;
    shapeTool->GetFreeShapes(freeShapes);

    if (freeShapes.Length() == 0) {
        return componentIndex;
    }


    // Create color assignment function
    auto palette = STEPColorManager::getPaletteForScheme(options.decomposition.colorScheme);
    std::hash<std::string> hasher;

    auto makeColorForName = [&](const std::string& name, const Quantity_Color* cafColor) -> Quantity_Color {
        // Always prioritize CAF color from STEP file if available
        if (cafColor) return *cafColor;
        // Fallback to consistent coloring or palette
        if (options.decomposition.useConsistentColoring) {
            size_t idx = hasher(name) % palette.size();
            return palette[idx];
        }
        return palette[componentIndex % palette.size()];
    };

    // Process each free shape
    for (int i = 1; i <= freeShapes.Length(); ++i) {
        componentIndex = processLabel(freeShapes.Value(i), TopLoc_Location(), 0,
            shapeTool, colorTool, baseName, options, makeColorForName,
            geometries, entityMetadata, componentIndex);
    }

    return componentIndex;
}

std::vector<TopoDS_Shape> STEPCAFProcessor::extractAndDecomposeShapes(
    const TopoDS_Shape& located,
    const std::string& compName,
    const GeometryReader::OptimizationOptions& options)
{
    std::vector<TopoDS_Shape> parts;


    // For FACE_LEVEL, skip preprocessing and go directly to face extraction
    // This ensures face-level decomposition is always applied regardless of shape structure
    bool isFaceLevel = options.decomposition.enableDecomposition && 
                       options.decomposition.level == GeometryReader::DecompositionLevel::FACE_LEVEL;
    
    // For assembly detection, we need to check if this shape contains multiple sub-components
    // regardless of whether it's a compound or a solid with multiple parts
    // BUT skip this for FACE_LEVEL to ensure face extraction is always applied
    if (!isFaceLevel) {
        if (located.ShapeType() == TopAbs_COMPOUND) {
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
        } else if (located.ShapeType() == TopAbs_SOLID) {
            // For solids, check if they contain multiple sub-components that could be assembly parts
            // Count shells within this solid
            int shellCount = 0;
            for (TopExp_Explorer exp(located, TopAbs_SHELL); exp.More(); exp.Next()) {
                shellCount++;
            }

            // If solid has multiple shells, treat each shell as a potential component
            if (shellCount > 1) {
                for (TopExp_Explorer exp(located, TopAbs_SHELL); exp.More(); exp.Next()) {
                    parts.push_back(exp.Current());
                }
            } else {
                // Single shell solid - use normal decomposition
                std::vector<TopoDS_Shape> tempParts;
                if (located.ShapeType() == TopAbs_SOLID || located.ShapeType() == TopAbs_SHELL ||
                    located.ShapeType() == TopAbs_FACE) {
                    tempParts.push_back(located);
                }
                parts = tempParts;
            }
        } else {
            // Use normal decomposition for other shape types
            std::vector<TopoDS_Shape> tempParts;
            if (located.ShapeType() == TopAbs_SOLID || located.ShapeType() == TopAbs_SHELL ||
                located.ShapeType() == TopAbs_FACE) {
                tempParts.push_back(located);
            }
            parts = tempParts;
        }
    } else {
        // For FACE_LEVEL, initialize with single shape to trigger decomposition
        parts.push_back(located);
    }

    // Apply user-configured decomposition options
    // For FACE_LEVEL, always apply decomposition regardless of parts.size()
    if (options.decomposition.enableDecomposition && (parts.size() == 1 || isFaceLevel)) {
        std::vector<TopoDS_Shape> heuristics;

        // Apply decomposition based on user-selected level
        switch (options.decomposition.level) {
            case GeometryReader::DecompositionLevel::NO_DECOMPOSITION:
                break;

            case GeometryReader::DecompositionLevel::SHAPE_LEVEL: {
                // SHAPE_LEVEL: Extract all top-level shapes (solids, shells, faces, etc.)
                // Try FreeCAD-like decomposition first (extracts all meaningful shapes)
                auto freeCADShapes = STEPGeometryDecomposer::decomposeShapeFreeCADLike(located);
                heuristics.insert(heuristics.end(), freeCADShapes.begin(), freeCADShapes.end());
                if (heuristics.size() <= 1) {
                    // If that fails, try feature recognition
                    heuristics.clear();
                    auto featureShapes = STEPGeometryDecomposer::decomposeByFeatureRecognition(located);
                    heuristics.insert(heuristics.end(), featureShapes.begin(), featureShapes.end());
                }
                if (heuristics.size() <= 1) {
                    // Last resort: try shell groups
                    heuristics.clear();
                    auto shellGroups = STEPGeometryDecomposer::decomposeByShellGroups(located);
                    heuristics.insert(heuristics.end(), shellGroups.begin(), shellGroups.end());
                }
                break;
            }

            case GeometryReader::DecompositionLevel::SOLID_LEVEL: {
                // Decompose into individual solids
                auto freeCADShapes = STEPGeometryDecomposer::decomposeShapeFreeCADLike(located);
                heuristics.insert(heuristics.end(), freeCADShapes.begin(), freeCADShapes.end());
                if (heuristics.size() <= 1) {
                    heuristics.clear();
                    auto geometricFeatures = STEPGeometryDecomposer::decomposeByGeometricFeatures(located);
                    heuristics.insert(heuristics.end(), geometricFeatures.begin(), geometricFeatures.end());
                }
                break;
            }

            case GeometryReader::DecompositionLevel::SHELL_LEVEL: {
                // SHELL_LEVEL: Directly extract all shells
                // First try direct shell extraction
                auto directShells = STEPGeometryDecomposer::decomposeByLevelUsingTopo(
                    located, GeometryReader::DecompositionLevel::SHELL_LEVEL);
                heuristics.insert(heuristics.end(), directShells.begin(), directShells.end());
                if (heuristics.size() <= 1) {
                    // If direct extraction fails, try shell groups
                    heuristics.clear();
                    auto shellGroups = STEPGeometryDecomposer::decomposeByShellGroups(located);
                    heuristics.insert(heuristics.end(), shellGroups.begin(), shellGroups.end());
                }
                if (heuristics.size() <= 1) {
                    // Last resort: try geometric features
                    heuristics.clear();
                    auto geometricFeatures = STEPGeometryDecomposer::decomposeByGeometricFeatures(located);
                    heuristics.insert(heuristics.end(), geometricFeatures.begin(), geometricFeatures.end());
                }
                break;
            }

            case GeometryReader::DecompositionLevel::FACE_LEVEL: {
                // For FACE_LEVEL, ALWAYS extract ALL individual faces directly
                // This ensures face-level decomposition regardless of shape structure
                heuristics.clear();
                
                // Force direct face extraction - extract ALL faces from the shape
                for (TopExp_Explorer exp(located, TopAbs_FACE); exp.More(); exp.Next()) {
                    TopoDS_Face face = TopoDS::Face(exp.Current());
                    if (!face.IsNull()) {
                        heuristics.push_back(face);
                    }
                }
                
                if (heuristics.empty()) {
                    // Fallback: if no faces found, try to extract from nested structures
                    
                    // Try to extract faces from shells if shape is a compound/solid
                    for (TopExp_Explorer shellExp(located, TopAbs_SHELL); shellExp.More(); shellExp.Next()) {
                        TopoDS_Shell shell = TopoDS::Shell(shellExp.Current());
                        for (TopExp_Explorer faceExp(shell, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                            TopoDS_Face face = TopoDS::Face(faceExp.Current());
                            if (!face.IsNull()) {
                                heuristics.push_back(face);
                            }
                        }
                    }
                    
                    // If still no faces, try from solids
                    if (heuristics.empty()) {
                        for (TopExp_Explorer solidExp(located, TopAbs_SOLID); solidExp.More(); solidExp.Next()) {
                            TopoDS_Solid solid = TopoDS::Solid(solidExp.Current());
                            for (TopExp_Explorer faceExp(solid, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                                TopoDS_Face face = TopoDS::Face(faceExp.Current());
                                if (!face.IsNull()) {
                                    heuristics.push_back(face);
                                }
                            }
                        }
                    }
                    
                    if (heuristics.empty()) {
                        LOG_WRN_S("CAF: Face-level decomposition failed - no faces found, keeping original shape");
                        heuristics.push_back(located);
                    }
                }
                break;
            }
        }

        if (heuristics.size() > 1) {
            parts = std::move(heuristics);
        }
    }

    return parts;
}

bool STEPCAFProcessor::detectShellModel(const TopoDS_Shape& shape)
{
    try {
        if (shape.IsNull()) {
            return false;
        }

        // Check shape type - if it's a shell, it's definitely a shell model
        if (shape.ShapeType() == TopAbs_SHELL) {
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


        // If we have shells but no solids, it's likely a shell model
        if (shellCount > 0 && solidCount == 0) {
            return true;
        }

        // If we have open shells, it's definitely a shell model requiring double-sided rendering
        if (openShellCount > 0) {
            return true;
        }

        // If we have only faces and no solids/shells, check if it's a surface model
        if (solidCount == 0 && shellCount == 0 && faceCount > 0) {
            return true;
        }

        // Additional check: if we have a compound with only shells
        if (shape.ShapeType() == TopAbs_COMPOUND) {
            TopExp_Explorer exp(shape, TopAbs_SOLID);
            if (!exp.More()) { // No solids found
                TopExp_Explorer shellExp(shape, TopAbs_SHELL);
                if (shellExp.More()) { // Has shells
                    return true;
                }
            }
        }

        // Check for thin-walled solids (solids with very thin walls that might need double-sided rendering)
        if (solidCount > 0 && shellCount > 0) {
            // Additional heuristic: if solid has many shells relative to its size, it might be thin-walled
            double shellToSolidRatio = static_cast<double>(shellCount) / static_cast<double>(solidCount);
            if (shellToSolidRatio > 2.0) {
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

int STEPCAFProcessor::createGeometriesFromParts(
    const std::vector<TopoDS_Shape>& parts,
    const std::string& compName,
    bool hasCafColor,
    const Quantity_Color& cafColor,
    int level,
    const std::string& baseName,
    const GeometryReader::OptimizationOptions& options,
    const std::function<Quantity_Color(const std::string&, const Quantity_Color*)>& makeColorForName,
    std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    std::vector<STEPReader::STEPEntityInfo>& entityMetadata,
    int componentIndex,
    const Handle(XCAFDoc_ColorTool)& colorTool,
    const Handle(XCAFDoc_ShapeTool)& shapeTool)
{
    int localIdx = 0;

    for (const auto& part : parts) {
        std::string partName = parts.size() > 1 ? (compName + "_Part_" + std::to_string(localIdx)) : compName;
        
        // Try to extract shape-level or face-level color from CAF
        Quantity_Color partColor = cafColor;
        bool hasPartColor = hasCafColor;
        
        if (!colorTool.IsNull() && !shapeTool.IsNull()) {
            // Try to find the label for this shape/face and get its color
            TDF_Label label;
            if (shapeTool->FindShape(part, label, false)) {
                // Try to get color for this shape/face
                Quantity_Color shapeColor;
                if (colorTool->GetColor(label, XCAFDoc_ColorSurf, shapeColor) ||
                    colorTool->GetColor(label, XCAFDoc_ColorGen, shapeColor) ||
                    colorTool->GetColor(label, XCAFDoc_ColorCurv, shapeColor)) {
                    partColor = shapeColor;
                    hasPartColor = true;
                    LOG_INF_S("Extracted shape-level color for " + partName);
                } else {
                    // If direct color not found, try to get color from parent shapes
                    // This handles cases where color is assigned to parent solid/shell but not individual faces
                    TDF_Label parentLabel = label.Father();
                    while (!parentLabel.IsNull() && !hasPartColor) {
                        if (colorTool->GetColor(parentLabel, XCAFDoc_ColorSurf, shapeColor) ||
                            colorTool->GetColor(parentLabel, XCAFDoc_ColorGen, shapeColor) ||
                            colorTool->GetColor(parentLabel, XCAFDoc_ColorCurv, shapeColor)) {
                            partColor = shapeColor;
                            hasPartColor = true;
                            LOG_INF_S("Extracted parent-level color for " + partName);
                            break;
                        }
                        parentLabel = parentLabel.Father();
                    }
                }
            } else {
                // If FindShape fails, try to search through all shapes to find matching face
                // This handles cases where face was extracted during decomposition
                if (part.ShapeType() == TopAbs_FACE) {
                    TDF_LabelSequence allLabels;
                    shapeTool->GetShapes(allLabels);
                    
                    for (int i = 1; i <= allLabels.Length(); ++i) {
                        TDF_Label searchLabel = allLabels.Value(i);
                        TopoDS_Shape labelShape = shapeTool->GetShape(searchLabel);
                        
                        // Check if this label's shape contains our face
                        for (TopExp_Explorer exp(labelShape, TopAbs_FACE); exp.More(); exp.Next()) {
                            if (exp.Current().IsSame(part)) {
                                // Found matching face, try to get its color
                                Quantity_Color faceColor;
                                if (colorTool->GetColor(searchLabel, XCAFDoc_ColorSurf, faceColor) ||
                                    colorTool->GetColor(searchLabel, XCAFDoc_ColorGen, faceColor) ||
                                    colorTool->GetColor(searchLabel, XCAFDoc_ColorCurv, faceColor)) {
                                    partColor = faceColor;
                                    hasPartColor = true;
                                    break;
                                }
                            }
                        }
                        if (hasPartColor) break;
                    }
                }
            }
        }
        
        // Assign color: when decomposition is enabled, prioritize palette colors
        // Otherwise use CAF color if available
        Quantity_Color color;
        if (options.decomposition.enableDecomposition) {
            // When decomposition is enabled, always use palette colors from color scheme
            auto palette = STEPColorManager::getPaletteForScheme(options.decomposition.colorScheme);
            std::hash<std::string> hasher;
            
            if (options.decomposition.useConsistentColoring) {
                // Hash-based consistent coloring
                size_t idx = hasher(partName) % palette.size();
                color = palette[idx];
            } else {
                // Sequential coloring using componentIndex + localIdx
                color = palette[(componentIndex + localIdx) % palette.size()];
            }
            
            LOG_INF_S("Applied decomposition color for " + partName + 
                     " (R:" + std::to_string(color.Red()) + 
                     " G:" + std::to_string(color.Green()) + 
                     " B:" + std::to_string(color.Blue()) + 
                     ", ComponentIndex:" + std::to_string(componentIndex) + 
                     ", LocalIdx:" + std::to_string(localIdx) + ")");
        } else if (hasPartColor) {
            // Use color from CAF only when decomposition is disabled
            color = partColor;
        } else {
            // Default color when no decomposition and no CAF color
            Quantity_Color defaultColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
            color = defaultColor;
        }

        auto geom = std::make_shared<OCCGeometry>(partName);
        geom->setShape(part);
        geom->setColor(color);
        geom->setFileName(baseName);

        // Detect if this is a shell model and apply appropriate settings
        bool isShellModel = detectShellModel(part);
        if (isShellModel) {
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
            // Also apply material colors for regular solid models
            // This ensures CAF colors are correctly applied when decomposition is disabled
            Standard_Real r, g, b;
            color.Values(r, g, b, Quantity_TOC_RGB);
            geom->setMaterialAmbientColor(Quantity_Color(r * 0.3, g * 0.3, b * 0.3, Quantity_TOC_RGB));
            geom->setMaterialDiffuseColor(color);
        }

        geom->setAssemblyLevel(level);

        // Build face index mapping for all geometries to enable face query
        // This allows face picking to work regardless of decomposition level
        MeshParameters meshParams;
        meshParams.deflection = 0.001;  // High quality mesh for face mapping
        meshParams.angularDeflection = 0.5;
        meshParams.relative = true;
        meshParams.inParallel = true;

        geom->buildFaceIndexMapping(meshParams);

        geometries.push_back(geom);

        STEPReader::STEPEntityInfo info;
        info.name = partName;
        info.type = "COMPONENT";
        info.color = color;
        info.hasColor = true;
        info.entityId = componentIndex;
        info.shapeIndex = componentIndex;
        entityMetadata.push_back(info);

        componentIndex++;
        localIdx++;
    }

    return componentIndex;
}

int STEPCAFProcessor::processLabel(
    const TDF_Label& label,
    const TopLoc_Location& parentLoc,
    int level,
    const Handle(XCAFDoc_ShapeTool)& shapeTool,
    const Handle(XCAFDoc_ColorTool)& colorTool,
    const std::string& baseName,
    const GeometryReader::OptimizationOptions& options,
    const std::function<Quantity_Color(const std::string&, const Quantity_Color*)>& makeColorForName,
    std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    std::vector<STEPReader::STEPEntityInfo>& entityMetadata,
    int& componentIndex)
{
    TopLoc_Location ownLoc = shapeTool->GetLocation(label);
    TopLoc_Location globLoc = parentLoc * ownLoc;

    // Debug: Log label information

    if (shapeTool->IsAssembly(label)) {
        TDF_LabelSequence children;
        shapeTool->GetComponents(label, children);
        for (int k = 1; k <= children.Length(); ++k) {
            componentIndex = processLabel(children.Value(k), globLoc, level + 1,
                shapeTool, colorTool, baseName, options, makeColorForName,
                geometries, entityMetadata, componentIndex);
        }
        return componentIndex;
    }

    if (!shapeTool->IsShape(label)) {
        return componentIndex;
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
        return componentIndex;
    }

    TopLoc_Location finalLoc = globLoc * srcLoc;
    TopoDS_Shape located = finalLoc.IsIdentity() ? shape : shape.Moved(finalLoc);

    std::string compName = baseName + "_Component_" + std::to_string(componentIndex);

    Handle(TDataStd_Name) nameAttr;
    if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr)) {
        TCollection_ExtendedString extStr = nameAttr->Get();
        std::string converted = STEPMetadataExtractor::safeConvertExtendedString(extStr);
        if (!converted.empty() && converted != "UnnamedComponent") compName = converted;
    }
    // Fallback: try name on referenced/origin label
    if (compName == baseName + "_Component_" + std::to_string(componentIndex)) {
        Handle(TDataStd_Name) refNameAttr;
        if (srcLabel.FindAttribute(TDataStd_Name::GetID(), refNameAttr)) {
            TCollection_ExtendedString extStr = refNameAttr->Get();
            std::string converted = STEPMetadataExtractor::safeConvertExtendedString(extStr);
            if (!converted.empty() && converted != "UnnamedComponent") compName = converted;
        }
    }

    Quantity_Color cafColor;
    bool hasCafColor = false;
    if (!colorTool.IsNull()) {
        // Priority 1: Try to get instance color from the located shape (SHUO-based colors)
        // This is critical for assembly instance colors where the same part may have different colors
        hasCafColor = colorTool->GetInstanceColor(located, XCAFDoc_ColorSurf, cafColor) ||
                       colorTool->GetInstanceColor(located, XCAFDoc_ColorGen, cafColor) ||
                       colorTool->GetInstanceColor(located, XCAFDoc_ColorCurv, cafColor);
        
        // Priority 2: Try to get color from label (general, surface, or curve color)
        if (!hasCafColor) {
            hasCafColor = colorTool->GetColor(label, XCAFDoc_ColorSurf, cafColor) ||
                           colorTool->GetColor(label, XCAFDoc_ColorGen, cafColor) ||
                           colorTool->GetColor(label, XCAFDoc_ColorCurv, cafColor);
        }
        
        // Priority 3: Try color on referenced/origin label
        if (!hasCafColor) {
            hasCafColor = colorTool->GetColor(srcLabel, XCAFDoc_ColorSurf, cafColor) ||
                           colorTool->GetColor(srcLabel, XCAFDoc_ColorGen, cafColor) ||
                           colorTool->GetColor(srcLabel, XCAFDoc_ColorCurv, cafColor);
        }
        
        // Priority 4: Try to get instance color from original shape (before location transform)
        if (!hasCafColor && !shape.IsNull()) {
            hasCafColor = colorTool->GetInstanceColor(shape, XCAFDoc_ColorSurf, cafColor) ||
                           colorTool->GetInstanceColor(shape, XCAFDoc_ColorGen, cafColor) ||
                           colorTool->GetInstanceColor(shape, XCAFDoc_ColorCurv, cafColor);
        }
        
        // Priority 5: Try to get color from sub-shapes (faces) - some STEP files define colors at face level
        if (!hasCafColor && !located.IsNull()) {
            for (TopExp_Explorer faceExp(located, TopAbs_FACE); faceExp.More() && !hasCafColor; faceExp.Next()) {
                TopoDS_Shape face = faceExp.Current();
                if (colorTool->GetInstanceColor(face, XCAFDoc_ColorSurf, cafColor) ||
                    colorTool->GetInstanceColor(face, XCAFDoc_ColorGen, cafColor)) {
                    hasCafColor = true;
                    LOG_INF_S("Extracted color from sub-face for component: " + compName);
                    break;
                }
            }
        }
        
        // Priority 6: Try to get color from child labels in the document hierarchy
        if (!hasCafColor && !shapeTool.IsNull()) {
            TDF_LabelSequence childLabels;
            shapeTool->GetSubShapes(label, childLabels);
            for (int i = 1; i <= childLabels.Length() && !hasCafColor; ++i) {
                TDF_Label childLabel = childLabels.Value(i);
                if (colorTool->GetColor(childLabel, XCAFDoc_ColorSurf, cafColor) ||
                    colorTool->GetColor(childLabel, XCAFDoc_ColorGen, cafColor)) {
                    hasCafColor = true;
                    LOG_INF_S("Extracted color from child label for component: " + compName);
                    break;
                }
            }
        }
        
        if (hasCafColor) {
            LOG_INF_S("Extracted CAF color for component: " + compName + 
                     " (R:" + std::to_string(cafColor.Red()) + 
                     " G:" + std::to_string(cafColor.Green()) + 
                     " B:" + std::to_string(cafColor.Blue()) + ")");
        } else {
            LOG_INF_S("No CAF color found for component: " + compName + ", will use default/palette color");
        }
    }

    // Extract and decompose shapes
    std::vector<TopoDS_Shape> parts = extractAndDecomposeShapes(located, compName, options);

    // Create geometries from parts
    componentIndex = createGeometriesFromParts(parts, compName, hasCafColor, cafColor, level,
        baseName, options, makeColorForName, geometries, entityMetadata, componentIndex,
        colorTool, shapeTool);

    return componentIndex;
}
