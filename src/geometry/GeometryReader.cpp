#include "GeometryReader.h"
#include "logger/Logger.h"
#include "OCCShapeBuilder.h"
#include "STEPReader.h"
#include "IGESReader.h"
#include "OBJReader.h"
#include "STLReader.h"
#include "BREPReader.h"
#include "XTReader.h"
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
