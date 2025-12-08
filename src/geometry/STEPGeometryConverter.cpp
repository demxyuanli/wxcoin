#include "STEPGeometryConverter.h"
#include "STEPReader.h"
#include "STEPReaderUtils.h"
#include "STEPColorManager.h"
#include "STEPGeometryDecomposer.h"
#include "OCCShapeBuilder.h"
#include "logger/Logger.h"
#include "rendering/GeometryProcessor.h"

#include <OpenCASCADE/Standard_ConstructionError.hxx>
#include <OpenCASCADE/Standard_Failure.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopAbs_ShapeEnum.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopoDS_Shell.hxx>
#include <OpenCASCADE/BRepBndLib.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <limits>

std::vector<std::shared_ptr<OCCGeometry>> STEPGeometryConverter::shapeToGeometries(
    const TopoDS_Shape& shape,
    const std::string& baseName,
    const GeometryReader::OptimizationOptions& options,
    ProgressCallback progress,
    int progressStart,
    int progressSpan)
{
    std::vector<std::shared_ptr<OCCGeometry>> geometries;

    if (shape.IsNull()) {
        return geometries;
    }

    try {
        // Step 1: Decompose shape according to options
        std::vector<TopoDS_Shape> shapes = STEPGeometryDecomposer::decomposeShape(shape, options);

        STEPReaderUtils::logCount("Converting ", shapes.size(), " shapes to geometries for: " + baseName);

        // Step 2: Get color palette for the selected scheme
        auto palette = STEPColorManager::getPaletteForScheme(options.decomposition.colorScheme);
        std::hash<std::string> hasher;

        // Step 3: Convert shapes to geometries with progress reporting
        size_t total = shapes.size();

        // Simple sequential processing (could be parallelized later)
        size_t successCount = 0;
        size_t failCount = 0;
        size_t colorIndex = 0;

        // Pre-allocate geometries vector for better performance
        geometries.reserve(shapes.size());

        // Cache frequently used values to avoid repeated computations
        const size_t paletteSize = palette.size();
        std::string baseNameWithUnderscore = baseName + "_";

        for (size_t i = 0; i < shapes.size(); ++i) {
            if (!shapes[i].IsNull()) {
                // Use stringstream for more efficient string building
                std::stringstream nameStream;
                nameStream << baseNameWithUnderscore << i;
                std::string name = nameStream.str();

                auto geometry = processSingleShape(shapes[i], name, baseName, options, palette, hasher, colorIndex % paletteSize);
                if (geometry) {
                    geometries.push_back(geometry);
                    successCount++;
                    colorIndex++;
                } else {
                    failCount++;
                }
            }

            // CRITICAL FIX: Update progress more frequently for large imports
            // This calls wxYield() in ImportGeometryListener to keep GL context alive
            if (progress && total > 0 && (i % 5 == 0 || i == total - 1)) {  // Changed from 10 to 5
                int pct = progressStart + (int)std::round(((double)(i + 1) / (double)total) * progressSpan);
                pct = std::max(progressStart, std::min(progressStart + progressSpan, pct));
                progress(pct, "convert");
            }
        }

        if (failCount > 0) {
            LOG_WRN_S("Failed to process " + std::to_string(failCount) + " out of " +
                std::to_string(total) + " shapes for: " + baseName);
        }
    }
    catch (const Standard_ConstructionError& e) {
        LOG_ERR_S("Construction error converting shapes: " + std::string(e.GetMessageString()));
        LOG_ERR_S("This typically indicates invalid or degenerate geometry in the STEP file");
    }
    catch (const Standard_Failure& e) {
        LOG_ERR_S("OpenCASCADE error converting shapes: " + std::string(e.GetMessageString()));
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception converting shape to geometries: " + std::string(e.what()));
    }

    return geometries;
}

std::shared_ptr<OCCGeometry> STEPGeometryConverter::processSingleShape(
    const TopoDS_Shape& shape,
    const std::string& name,
    const std::string& baseName,
    const GeometryReader::OptimizationOptions& options)
{
    // Get color palette for the selected scheme
    auto palette = STEPColorManager::getPaletteForScheme(options.decomposition.colorScheme);
    std::hash<std::string> hasher;

    // Use sequential coloring if decomposition is enabled, otherwise use hash-based coloring
    size_t colorIndex = 0;
    if (options.decomposition.enableDecomposition && options.decomposition.useConsistentColoring) {
        // Use hash-based consistent coloring for decomposed components
        colorIndex = hasher(name) % palette.size();
    } else {
        // Use sequential coloring from the palette
        static size_t globalColorIndex = 0;
        colorIndex = globalColorIndex++ % palette.size();
    }

    return processSingleShape(shape, name, baseName, options, palette, hasher, colorIndex);
}

std::shared_ptr<OCCGeometry> STEPGeometryConverter::processSingleShape(
    const TopoDS_Shape& shape,
    const std::string& name,
    const std::string& baseName,
    const GeometryReader::OptimizationOptions& options,
    const std::vector<Quantity_Color>& palette,
    const std::hash<std::string>& hasher,
    size_t colorIndex)
{
    if (shape.IsNull()) {
        return nullptr;
    }

    try {
        // Use OCCT raw shape without active fixing (simplified approach)
        auto geometry = std::make_shared<OCCGeometry>(name);
        geometry->setShape(shape);
        geometry->setFileName(baseName);

        // Set color based on the provided palette and index
        Quantity_Color componentColor = palette[colorIndex % palette.size()];
        geometry->setColor(componentColor);

        // Detect if this is a shell model and apply appropriate settings
        bool isShellModel = detectShellModel(shape);
        if (isShellModel) {
            // For shell models, disable backface culling to ensure all faces are visible from both sides
            geometry->setCullFace(false);
            // Shell models should be opaque for better visibility
            geometry->setTransparency(0.0);
            // Enable depth testing but ensure proper depth write for shells
            geometry->setDepthTest(true);
            geometry->setDepthWrite(true);
            // Set enhanced material properties for shell models with better contrast
            // Use the component color for ambient and diffuse, but adjust intensity for better shell rendering
            Standard_Real r, g, b;
            componentColor.Values(r, g, b, Quantity_TOC_RGB);
            geometry->setMaterialAmbientColor(Quantity_Color(r * 0.3, g * 0.3, b * 0.3, Quantity_TOC_RGB));
            geometry->setMaterialDiffuseColor(Quantity_Color(r * 0.8, g * 0.8, b * 0.8, Quantity_TOC_RGB));
            geometry->setMaterialShininess(50.0);
            // Enable smooth normals for better shell rendering
            geometry->setSmoothNormals(true);
        } else {
            // Regular solid models use standard settings
            geometry->setTransparency(0.0);
        }

        // Only analyze shape if explicitly enabled (disabled by default for speed)
        if (options.enableShapeAnalysis) {
            OCCShapeBuilder::analyzeShapeTopology(shape, name);
        }

        // Build face index mapping to enable face picking for all geometries
        MeshParameters meshParams;
        meshParams.deflection = 0.001;  // High quality mesh for face mapping
        meshParams.angularDeflection = 0.5;
        meshParams.relative = true;
        meshParams.inParallel = true;

        geometry->buildFaceIndexMapping(meshParams);

        return geometry;
    }
    catch (const Standard_ConstructionError& e) {
        LOG_ERR_S("Construction error processing shape " + name + ": " + std::string(e.GetMessageString()));
        LOG_ERR_S("This often happens with degenerate or invalid geometry. Skipping this shape.");
        return nullptr;
    }
    catch (const Standard_Failure& e) {
        LOG_ERR_S("OpenCASCADE error processing shape " + name + ": " + std::string(e.GetMessageString()));
        return nullptr;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception processing shape " + name + ": " + std::string(e.what()));
        return nullptr;
    }
}

std::vector<std::shared_ptr<OCCGeometry>> STEPGeometryConverter::createGeometriesFromShapes(
    const std::vector<TopoDS_Shape>& shapes,
    const std::string& baseName,
    const GeometryReader::OptimizationOptions& options,
    const std::vector<Quantity_Color>& palette,
    const std::hash<std::string>& hasher)
{
    std::vector<std::shared_ptr<OCCGeometry>> geometries;
    size_t colorIndex = 0;
    size_t successCount = 0;
    size_t failCount = 0;

    for (size_t i = 0; i < shapes.size(); ++i) {
        if (!shapes[i].IsNull()) {
            std::string name = baseName + "_" + std::to_string(i);
            auto geometry = processSingleShape(shapes[i], name, baseName, options, palette, hasher, colorIndex);
            if (geometry) {
                geometries.push_back(geometry);
                successCount++;
                colorIndex++;
            } else {
                failCount++;
            }
        }
    }

    if (successCount > 0) {
        STEPReaderUtils::logSuccess("Successfully converted", successCount, "shapes");
        if (failCount > 0) {
            LOG_WRN_S("Failed to convert " + std::to_string(failCount) + " shapes");
        }
    } else {
        LOG_WRN_S("Failed to convert any shapes to geometries");
    }

    return geometries;
}

bool STEPGeometryConverter::detectShellModel(const TopoDS_Shape& shape)
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

        // Shape analysis removed

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
        return false;
    }
}

double STEPGeometryConverter::scaleGeometriesToReasonableSize(
    std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    double targetSize)
{
    if (geometries.empty()) {
        return 1.0;
    }

    try {
        // Use simplified bounding box calculation
        gp_Pnt overallMin, overallMax;
        if (!calculateCombinedBoundingBox(geometries, overallMin, overallMax)) {
            return 1.0;
        }

        // Calculate current size
        double currentSizeX = overallMax.X() - overallMin.X();
        double currentSizeY = overallMax.Y() - overallMin.Y();
        double currentSizeZ = overallMax.Z() - overallMin.Z();
        double currentMaxSize = (std::max)({ currentSizeX, currentSizeY, currentSizeZ });

        // Determine target size
        if (targetSize <= 0.0) {
            // Auto-detect reasonable size (10-50 units)
            if (currentMaxSize > 100.0) {
                targetSize = 20.0;  // Scale large models down
            }
            else if (currentMaxSize < 0.1) {
                targetSize = 10.0;  // Scale tiny models up
            }
            else {
                // Size is already reasonable
                return 1.0;
            }
        }

        double scaleFactor = targetSize / currentMaxSize;

        if (std::abs(scaleFactor - 1.0) < 0.01) {
            // No significant scaling needed
            return 1.0;
        }

        // Apply scaling sequentially for simplicity
        for (auto& geometry : geometries) {
            if (!geometry || static_cast<GeometryRenderer*>(geometry.get())->getShape().IsNull()) {
                continue;
            }

            TopoDS_Shape scaledShape = OCCShapeBuilder::scale(
                static_cast<GeometryRenderer*>(geometry.get())->getShape(),
                gp_Pnt(0, 0, 0),
                scaleFactor
            );

            if (!scaledShape.IsNull()) {
                geometry->setShape(scaledShape);
            }
        }

        return scaleFactor;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception scaling geometries: " + std::string(e.what()));
        return 1.0;
    }
}

bool STEPGeometryConverter::calculateCombinedBoundingBox(
    const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    gp_Pnt& minPt,
    gp_Pnt& maxPt)
{
    if (geometries.empty()) {
        return false;
    }

    minPt = gp_Pnt(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    maxPt = gp_Pnt(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    bool hasValidBounds = false;

    // Sequential processing for simplicity
    for (const auto& geometry : geometries) {
        if (!geometry || static_cast<GeometryRenderer*>(geometry.get())->getShape().IsNull()) {
            continue;
        }

        gp_Pnt localMin, localMax;
        OCCShapeBuilder::getBoundingBox(static_cast<GeometryRenderer*>(geometry.get())->getShape(), localMin, localMax);

        if (localMin.X() < minPt.X()) minPt.SetX(localMin.X());
        if (localMin.Y() < minPt.Y()) minPt.SetY(localMin.Y());
        if (localMin.Z() < minPt.Z()) minPt.SetZ(localMin.Z());

        if (localMax.X() > maxPt.X()) maxPt.SetX(localMax.X());
        if (localMax.Y() > maxPt.Y()) maxPt.SetY(localMax.Y());
        if (localMax.Z() > maxPt.Z()) maxPt.SetZ(localMax.Z());

        hasValidBounds = true;
    }

    return hasValidBounds;
}

