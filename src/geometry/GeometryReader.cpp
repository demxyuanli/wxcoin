#include "GeometryReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"
#include "STEPReader.h"
#include "IGESReader.h"
#include "OBJReader.h"
#include "STLReader.h"
#include "BREPReader.h"
#include "XTReader.h"
#include "rendering/RenderingToolkitAPI.h"
#include "geometry/helper/PointViewBuilder.h"
#include "geometry/helper/WireframeBuilder.h"
#include "geometry/GeometryRenderContext.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <mutex>
#include <future>
#include <thread>
#include <execution>

// OpenCASCADE includes for shape processing
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <ShapeFix_Shape.hxx>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPolygonOffset.h>

std::shared_ptr<OCCGeometry> GeometryReader::createGeometryFromShape(
    const TopoDS_Shape& shape,
    const std::string& name,
    const std::string& fileName,
    const OptimizationOptions& options)
{
    try {
        // Create OCCGeometry from shape
        auto geometry = std::make_shared<OCCGeometry>(name);
        geometry->setShape(shape);
        geometry->setFileName(fileName);
        
        // Apply optimization options
        if (options.enableShapeAnalysis) {
            // Perform shape analysis and fixing if needed
            BRepCheck_Analyzer analyzer(shape);
            if (!analyzer.IsValid()) {
                ShapeFix_Shape fixer(shape);
                fixer.Perform();
                // Use the fixed shape if available
                TopoDS_Shape fixedShape = fixer.Shape();
                if (!fixedShape.IsNull()) {
                    geometry = std::make_shared<OCCGeometry>(name);
                    geometry->setShape(fixedShape);
                    geometry->setFileName(fileName);
                }
            }
        }
        
        return geometry;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create geometry from shape: " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryReader::createGeometryFromMesh(
    const TriangleMesh& mesh,
    const std::string& name,
    const std::string& fileName,
    const OptimizationOptions& options)
{
    try {
        // Create OCCGeometry with empty shape (placeholder for mesh-only geometry)
        auto geometry = std::make_shared<OCCGeometry>(name);
        
        // Create a minimal dummy shape to satisfy OCCGeometry requirements
        // This is just a placeholder - the actual rendering uses the mesh directly
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        geometry->setShape(compound);
        geometry->setFileName(fileName);
        
        // Create complete Coin3D node structure with all display modes (FreeCAD-style fast path)
        SoSeparator* rootNode = new SoSeparator();
        rootNode->ref();
        rootNode->renderCaching.setValue(SoSeparator::OFF);
        rootNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        rootNode->pickCulling.setValue(SoSeparator::OFF);

        // Create SoSwitch for display mode switching
        SoSwitch* modeSwitch = new SoSwitch();
        modeSwitch->ref();

        // Default material colors
        Quantity_Color defaultDiffuse(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        Quantity_Color defaultAmbient(0.2, 0.2, 0.2, Quantity_TOC_RGB);
        Quantity_Color defaultSpecular(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        Quantity_Color defaultEmissive(0.0, 0.0, 0.0, Quantity_TOC_RGB);

        // Create surface geometry node (Solid mode)
        auto& manager = RenderingToolkitAPI::getManager();
        auto backend = manager.getRenderBackend("Coin3D");
        if (backend) {
            auto sceneNode = backend->createSceneNode(mesh, false, 
                defaultDiffuse, defaultAmbient, defaultSpecular, defaultEmissive, 0.5, 0.0);
            
            if (sceneNode) {
                SoSeparator* surfaceNode = sceneNode.get();
                surfaceNode->ref();
                modeSwitch->addChild(surfaceNode);
            }
        }

        // Create wireframe node (Wireframe mode)
        SoSeparator* wireframeNode = new SoSeparator();
        wireframeNode->ref();
        wireframeNode->renderCaching.setValue(SoSeparator::OFF);
        wireframeNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        wireframeNode->pickCulling.setValue(SoSeparator::OFF);
        
        SoDrawStyle* wireframeStyle = new SoDrawStyle();
        wireframeStyle->style.setValue(SoDrawStyle::LINES);
        wireframeStyle->lineWidth.setValue(1.0f);
        wireframeNode->addChild(wireframeStyle);
        
        SoMaterial* wireframeMaterial = new SoMaterial();
        wireframeMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.0f);
        wireframeMaterial->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
        wireframeNode->addChild(wireframeMaterial);
        
        SoLightModel* wireframeLightModel = new SoLightModel();
        wireframeLightModel->model.setValue(SoLightModel::BASE_COLOR);
        wireframeNode->addChild(wireframeLightModel);
        
        WireframeBuilder wireframeBuilder;
        wireframeBuilder.createWireframeRepresentation(wireframeNode, mesh);
        modeSwitch->addChild(wireframeNode);

        // Create point view node (Points mode)
        SoSeparator* pointViewNode = new SoSeparator();
        pointViewNode->ref();
        pointViewNode->renderCaching.setValue(SoSeparator::OFF);
        pointViewNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        pointViewNode->pickCulling.setValue(SoSeparator::OFF);
        
        DisplaySettings pointSettings;
        pointSettings.pointViewColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);
        pointSettings.pointViewSize = 3.0;
        pointSettings.pointViewShape = 0;
        
        PointViewBuilder pointViewBuilder;
        pointViewBuilder.createPointViewRepresentation(pointViewNode, mesh, pointSettings);
        modeSwitch->addChild(pointViewNode);

        // Set default mode to Solid (index 0)
        modeSwitch->whichChild.setValue(0);
        rootNode->addChild(modeSwitch);

        // Store mode switch in geometry for later mode switching
        // Note: This requires adding a method to OCCGeometry to store the switch
        geometry->setCoinNode(rootNode);
        
        LOG_INF_S("Created OCCGeometry from mesh with all display modes: " + 
                 std::to_string(mesh.vertices.size()) + " vertices, " + 
                 std::to_string(mesh.triangles.size() / 3) + " triangles");
        
        return geometry;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create geometry from mesh: " + std::string(e.what()));
        return nullptr;
    }
}

bool GeometryReader::validateFile(const std::string& filePath, std::string& errorMessage)
{
    try {
        if (!std::filesystem::exists(filePath)) {
            errorMessage = "File does not exist: " + filePath;
            return false;
        }
        
        if (!std::filesystem::is_regular_file(filePath)) {
            errorMessage = "Path is not a regular file: " + filePath;
            return false;
        }
        
        // Check if file is readable
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            errorMessage = "Cannot open file for reading: " + filePath;
            return false;
        }
        
        // Check if file has content
        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            errorMessage = "File is empty: " + filePath;
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        errorMessage = "Error validating file: " + std::string(e.what());
        return false;
    }
}

std::vector<std::unique_ptr<GeometryReader>> GeometryReaderFactory::getAllReaders()
{
    std::vector<std::unique_ptr<GeometryReader>> readers;
    
    // Add all available readers
    readers.push_back(std::make_unique<STEPReader>());
    readers.push_back(std::make_unique<IGESReader>());
    readers.push_back(std::make_unique<OBJReader>());
    readers.push_back(std::make_unique<STLReader>());
    readers.push_back(std::make_unique<BREPReader>());
    readers.push_back(std::make_unique<XTReader>());
    
    return readers;
}

std::unique_ptr<GeometryReader> GeometryReaderFactory::getReaderForExtension(const std::string& extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Ensure extension starts with dot
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    // Check each reader for support
    auto readers = getAllReaders();
    for (auto& reader : readers) {
        auto supportedExts = reader->getSupportedExtensions();
        for (const auto& supportedExt : supportedExts) {
            if (supportedExt == ext) {
                return std::move(reader);
            }
        }
    }
    
    return nullptr;
}

std::unique_ptr<GeometryReader> GeometryReaderFactory::getReaderForFile(const std::string& filePath)
{
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    return getReaderForExtension(extension);
}

std::string GeometryReaderFactory::getAllSupportedFileFilter()
{
    std::string filter = "All supported formats|";
    std::string descriptions;
    
    auto readers = getAllReaders();
    for (size_t i = 0; i < readers.size(); ++i) {
        if (i > 0) {
            descriptions += "|";
        }
        descriptions += readers[i]->getFileFilter();
        
        // Add extensions to "All supported formats"
        auto extensions = readers[i]->getSupportedExtensions();
        for (const auto& ext : extensions) {
            if (!filter.empty() && filter.back() != '|') {
                filter += ";";
            }
            filter += "*" + ext;
        }
    }
    
    return filter + "|" + descriptions + "|All files (*.*)|*.*";
}

std::vector<std::string> GeometryReaderFactory::getAllSupportedExtensions()
{
    std::vector<std::string> allExtensions;
    auto readers = getAllReaders();
    
    for (auto& reader : readers) {
        auto extensions = reader->getSupportedExtensions();
        allExtensions.insert(allExtensions.end(), extensions.begin(), extensions.end());
    }
    
    // Remove duplicates
    std::sort(allExtensions.begin(), allExtensions.end());
    allExtensions.erase(std::unique(allExtensions.begin(), allExtensions.end()), allExtensions.end());
    
    return allExtensions;
}
