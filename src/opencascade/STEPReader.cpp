#include "STEPReader.h"
#include "Logger.h"
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

// Add include at top
#include <XCAFApp_Application.hxx>

STEPReader::ReadResult STEPReader::readSTEPFile(const std::string& filePath)
{
    ReadResult result;
    
    try {
        LOG_INF("Reading STEP file: " + filePath);
        
        // Check if file exists
        if (!std::filesystem::exists(filePath)) {
            result.errorMessage = "File does not exist: " + filePath;
            LOG_ERR(result.errorMessage);
            return result;
        }
        
        // Check file extension
        if (!isSTEPFile(filePath)) {
            result.errorMessage = "File is not a STEP file: " + filePath;
            LOG_ERR(result.errorMessage);
            return result;
        }
        
        // Initialize STEP reader
        initialize();
        
        // Try CAF (Color and Assembly) reader first for better support
        STEPCAFControl_Reader cafReader;
        
        // Set precision
        Interface_Static::SetIVal("read.precision.mode", 1);
        Interface_Static::SetRVal("read.precision.val", 0.01);
        
        // Read the file
        IFSelect_ReturnStatus status = cafReader.ReadFile(filePath.c_str());
        
        if (status != IFSelect_RetDone) {
            // Fall back to basic STEP reader
            LOG_WRN("CAF reader failed, trying basic STEP reader");
            
            STEPControl_Reader basicReader;
            status = basicReader.ReadFile(filePath.c_str());
            
            if (status != IFSelect_RetDone) {
                result.errorMessage = "Failed to read STEP file: " + filePath;
                LOG_ERR(result.errorMessage);
                return result;
            }
            
            // Transfer shapes
            Standard_Integer nbRoots = basicReader.NbRootsForTransfer();
            LOG_INF("Found " + std::to_string(nbRoots) + " root entities");
            
            if (nbRoots == 0) {
                result.errorMessage = "No transferable entities found in STEP file";
                LOG_ERR(result.errorMessage);
                return result;
            }
            
            basicReader.TransferRoots();
            Standard_Integer nbShapes = basicReader.NbShapes();
            LOG_INF("Transferred " + std::to_string(nbShapes) + " shapes");
            
            if (nbShapes == 0) {
                result.errorMessage = "No shapes could be transferred from STEP file";
                LOG_ERR(result.errorMessage);
                return result;
            }
            
            // Create compound shape containing all shapes
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            
            for (Standard_Integer i = 1; i <= nbShapes; i++) {
                TopoDS_Shape shape = basicReader.Shape(i);
                if (!shape.IsNull()) {
                    builder.Add(compound, shape);
                }
            }
            
            result.rootShape = compound;
            
        } else {
            // CAF reader succeeded
            LOG_INF("CAF reader succeeded");
            
            // Create new XCAF document for transfer
            Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) doc;
            app->NewDocument("MDTV-XCAF", doc);
            if (doc.IsNull()) {
                result.errorMessage = "Failed to create XCAF document";
                LOG_ERR(result.errorMessage);
                return result;
            }
            
            // Transfer data
            if (!cafReader.Transfer(doc)) {
                result.errorMessage = "Failed to transfer data from CAF reader";
                LOG_ERR(result.errorMessage);
                return result;
            }
            
            // Get shape tool
            Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
            
            if (shapeTool.IsNull()) {
                result.errorMessage = "Failed to get shape tool from document";
                LOG_ERR(result.errorMessage);
                return result;
            }
            
            // Get free shapes (top-level shapes)
            TDF_LabelSequence freeShapes;
            shapeTool->GetFreeShapes(freeShapes);
            
            LOG_INF("Found " + std::to_string(freeShapes.Length()) + " free shapes");
            
            if (freeShapes.Length() == 0) {
                result.errorMessage = "No free shapes found in STEP file";
                LOG_ERR(result.errorMessage);
                return result;
            }
            
            // Create compound shape
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            
            for (Standard_Integer i = 1; i <= freeShapes.Length(); i++) {
                TDF_Label label = freeShapes.Value(i);
                TopoDS_Shape shape;
                if (shapeTool->GetShape(label, shape) && !shape.IsNull()) {
                    builder.Add(compound, shape);
                }
            }
            
            result.rootShape = compound;
        }
        
        // Convert to geometry objects
        std::string baseName = std::filesystem::path(filePath).stem().string();
        result.geometries = shapeToGeometries(result.rootShape, baseName);
        
        // Apply automatic scaling to make geometries reasonable size
        if (!result.geometries.empty()) {
            double scaleFactor = scaleGeometriesToReasonableSize(result.geometries);
            if (scaleFactor != 1.0) {
                LOG_INF("Applied automatic scaling factor: " + std::to_string(scaleFactor));
            }
        }
        
        // Analyze the imported shape
        OCCShapeBuilder::analyzeShapeTopology(result.rootShape, baseName);
        
        result.success = true;
        LOG_INF("Successfully imported STEP file with " + std::to_string(result.geometries.size()) + " geometry objects");
        
    } catch (const Standard_Failure& e) {
        result.errorMessage = "OpenCASCADE exception: " + std::string(e.GetMessageString());
        LOG_ERR(result.errorMessage);
    } catch (const std::exception& e) {
        result.errorMessage = "Exception reading STEP file: " + std::string(e.what());
        LOG_ERR(result.errorMessage);
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
    const std::string& baseName)
{
    std::vector<std::shared_ptr<OCCGeometry>> geometries;
    
    if (shape.IsNull()) {
        LOG_WRN("Cannot convert null shape to geometries");
        return geometries;
    }
    
    try {
        // Extract individual shapes
        std::vector<TopoDS_Shape> shapes;
        extractShapes(shape, shapes);
        
        LOG_INF("Extracted " + std::to_string(shapes.size()) + " individual shapes");
        
        // Create geometry objects
        int shapeIndex = 0;
        for (const auto& individualShape : shapes) {
            if (!individualShape.IsNull()) {
                std::string name = baseName + "_" + std::to_string(shapeIndex++);
                auto geometry = std::make_shared<OCCGeometry>(name);
                geometry->setShape(individualShape);
                
                // Set better default color for imported STEP models
                // Use a neutral light gray color that is bright and clear
                Quantity_Color defaultColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
                geometry->setColor(defaultColor);
                
                // Remove transparency for a solid appearance
                geometry->setTransparency(0.0);
                
                // Analyze each shape
                OCCShapeBuilder::analyzeShapeTopology(individualShape, name);
                
                geometries.push_back(geometry);
            }
        }
        
        // If no individual shapes were found, create one geometry from the whole shape
        if (geometries.empty() && !shape.IsNull()) {
            auto geometry = std::make_shared<OCCGeometry>(baseName);
            geometry->setShape(shape);
            geometries.push_back(geometry);
        }
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception converting shape to geometries: " + std::string(e.what()));
    }
    
    return geometries;
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
    
    LOG_INF("STEP reader initialized");
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

double STEPReader::scaleGeometriesToReasonableSize(
    std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    double targetSize)
{
    if (geometries.empty()) {
        return 1.0;
    }
    
    try {
        // Calculate overall bounding box
        gp_Pnt overallMin(DBL_MAX, DBL_MAX, DBL_MAX);
        gp_Pnt overallMax(-DBL_MAX, -DBL_MAX, -DBL_MAX);
        bool hasValidBounds = false;
        
        for (const auto& geometry : geometries) {
            if (!geometry || geometry->getShape().IsNull()) {
                continue;
            }
            
            gp_Pnt minPt, maxPt;
            OCCShapeBuilder::getBoundingBox(geometry->getShape(), minPt, maxPt);
            
            if (minPt.X() < overallMin.X()) overallMin.SetX(minPt.X());
            if (minPt.Y() < overallMin.Y()) overallMin.SetY(minPt.Y());
            if (minPt.Z() < overallMin.Z()) overallMin.SetZ(minPt.Z());
            
            if (maxPt.X() > overallMax.X()) overallMax.SetX(maxPt.X());
            if (maxPt.Y() > overallMax.Y()) overallMax.SetY(maxPt.Y());
            if (maxPt.Z() > overallMax.Z()) overallMax.SetZ(maxPt.Z());
            
            hasValidBounds = true;
        }
        
        if (!hasValidBounds) {
            LOG_WRN("No valid bounds found for scaling");
            return 1.0;
        }
        
        // Calculate current size
        double currentSizeX = overallMax.X() - overallMin.X();
        double currentSizeY = overallMax.Y() - overallMin.Y();
        double currentSizeZ = overallMax.Z() - overallMin.Z();
        double currentMaxSize = std::max({currentSizeX, currentSizeY, currentSizeZ});
        
        LOG_INF("Current geometry size: " + std::to_string(currentMaxSize));
        
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
        
        LOG_INF("Applying scale factor: " + std::to_string(scaleFactor));
        
        // Apply scaling to all geometries
        for (auto& geometry : geometries) {
            if (!geometry || geometry->getShape().IsNull()) {
                continue;
            }
            
            // Scale the shape
            TopoDS_Shape scaledShape = OCCShapeBuilder::scale(
                geometry->getShape(), 
                gp_Pnt(0, 0, 0), 
                scaleFactor
            );
            
            if (!scaledShape.IsNull()) {
                geometry->setShape(scaledShape);
            }
        }
        
        return scaleFactor;
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception scaling geometries: " + std::string(e.what()));
        return 1.0;
    }
} 