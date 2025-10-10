#include "StreamingFileReader.h"
#include "ProgressiveGeometryLoader.h"
#include "logger/Logger.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <chrono>
#include <OpenCASCADE/STEPControl_Reader.hxx>
#include <OpenCASCADE/IGESControl_Reader.hxx>
#include <OpenCASCADE/Interface_Static.hxx>
#include <OpenCASCADE/IFSelect_ReturnStatus.hxx>
#include <OpenCASCADE/Standard_Failure.hxx>

StreamingFileReader::StreamingFileReader()
    : m_isLoading(false)
    , m_cancelRequested(false)
{
}

StreamingFileReader::~StreamingFileReader()
{
    cancelLoading();
}

void StreamingFileReader::updateProgress(const LoadingProgress& progress)
{
    m_progress = progress;
    if (m_config.progressCallback) {
        m_config.progressCallback(progress);
    }
}

void StreamingFileReader::updateMemoryInfo(const MemoryInfo& memory)
{
    m_memoryInfo = memory;
    if (m_config.memoryCallback) {
        m_config.memoryCallback(memory);
    }
}

bool StreamingFileReader::shouldUseStreaming(const std::string& filePath, size_t& estimatedSize)
{
    try {
        std::filesystem::path path(filePath);
        if (!std::filesystem::exists(path)) {
            return false;
        }

        estimatedSize = std::filesystem::file_size(path);

        // Use streaming for files larger than 100MB
        const size_t STREAMING_THRESHOLD = 100 * 1024 * 1024; // 100MB
        return estimatedSize > STREAMING_THRESHOLD;
    } catch (const std::exception&) {
        return false;
    }
}

std::string StreamingFileReader::getFileExtension(const std::string& filePath)
{
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot != std::string::npos) {
        return filePath.substr(lastDot);
    }
    return "";
}

size_t StreamingFileReader::getOptimalChunkSize(size_t fileSize)
{
    // Base chunk size on file size
    if (fileSize < 10 * 1024 * 1024) { // < 10MB
        return 64 * 1024;  // 64KB
    } else if (fileSize < 100 * 1024 * 1024) { // < 100MB
        return 256 * 1024; // 256KB
    } else if (fileSize < 1024 * 1024 * 1024) { // < 1GB
        return 1024 * 1024; // 1MB
    } else { // >= 1GB
        return 4 * 1024 * 1024; // 4MB
    }
}

bool StreamingFileReader::supportsStreaming(const std::string& filePath)
{
    std::string ext = StreamingFileReader::getFileExtension(filePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Currently support STEP and IGES streaming
    return ext == ".step" || ext == ".stp" || ext == ".iges" || ext == ".igs";
}

size_t StreamingFileReader::estimateMemoryRequirements(const std::string& filePath) const
{
    try {
        std::filesystem::path path(filePath);
        size_t fileSize = std::filesystem::file_size(path);

        // Rough estimate: 10x file size for processed geometry
        // This is a conservative estimate
        return fileSize * 10;
    } catch (const std::exception&) {
        return 1024 * 1024 * 1024; // 1GB default
    }
}

// =====================================================================================
// StreamingSTEPReader Implementation
// =====================================================================================

StreamingSTEPReader::StreamingSTEPReader()
    : StreamingFileReader()
    , m_currentPosition(0)
    , m_fileSize(0)
    , m_processedEntities(0)
    , m_stepReader(nullptr)
    , m_totalRoots(0)
    , m_currentRoot(0)
    , m_currentChunkIndex(0)
{
}

StreamingSTEPReader::~StreamingSTEPReader()
{
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    
    if (m_stepReader) {
        delete m_stepReader;
        m_stepReader = nullptr;
    }
}

bool StreamingSTEPReader::loadFile(const std::string& filePath, const LoadingConfig& config)
{
    m_config = config;
    m_filePath = filePath;
    m_isLoading = true;
    m_cancelRequested = false;
    m_currentRoot = 0;
    m_currentChunkIndex = 0;

    // Create and initialize STEP reader
    if (m_stepReader) {
        delete m_stepReader;
    }
    m_stepReader = new STEPControl_Reader();
    
    // Read the entire STEP file
    IFSelect_ReturnStatus status = m_stepReader->ReadFile(filePath.c_str());
    if (status != IFSelect_RetDone) {
        LOG_ERR_S("Failed to read STEP file: " + filePath);
        m_isLoading = false;
        delete m_stepReader;
        m_stepReader = nullptr;
        return false;
    }
    
    // Get total number of roots
    m_totalRoots = m_stepReader->NbRootsForTransfer();
    if (m_totalRoots == 0) {
        LOG_ERR_S("No transferable roots found in STEP file");
        m_isLoading = false;
        delete m_stepReader;
        m_stepReader = nullptr;
        return false;
    }
    
    LOG_INF_S("STEP file loaded: " + std::to_string(m_totalRoots) + " roots to process");

    // Initialize progress
    m_progress.totalShapes = m_totalRoots;
    m_progress.shapesLoaded = 0;
    
    try {
        m_fileSize = std::filesystem::file_size(filePath);
        m_progress.totalBytes = m_fileSize;
    } catch(...) {
        m_progress.totalBytes = 0;
    }

    return true;
}

bool StreamingSTEPReader::getNextChunk(std::vector<TopoDS_Shape>& shapes)
{
    LOG_DBG_S("StreamingSTEPReader::getNextChunk called");
    
    if (!m_isLoading || m_cancelRequested) {
        LOG_DBG_S("Not loading or cancelled");
        return false;
    }

    shapes.clear();
    
    // Check if we've already processed all roots
    if (m_currentRoot >= m_totalRoots) {
        LOG_INF_S("All roots already processed");
        m_progress.isComplete = true;
        m_isLoading = false;
        updateProgress(m_progress);
        return false;
    }

    // Parse next chunk of STEP data (transfer roots)
    LOG_DBG_S("Calling parseNextChunk, currentRoot=" + std::to_string(m_currentRoot) + 
             ", totalRoots=" + std::to_string(m_totalRoots));
    
    bool hasMoreChunks = parseNextChunk();
    
    LOG_DBG_S("parseNextChunk returned " + std::string(hasMoreChunks ? "true" : "false") + 
             ", now extracting shapes");
    
    // Extract shapes from the transferred roots
    if (!extractShapesFromEntities(shapes)) {
        LOG_WRN_S("Failed to extract shapes from STEP entities");
        
        // Check if we should continue
        if (hasMoreChunks) {
            return true; // Continue with next chunk
        } else {
            m_progress.isComplete = true;
            m_isLoading = false;
            updateProgress(m_progress);
            return false;
        }
    }

    LOG_INF_S("Extracted " + std::to_string(shapes.size()) + " shapes from chunk");

    // Update progress
    m_progress.shapesLoaded = m_currentRoot;
    m_progress.progressPercent = (static_cast<double>(m_currentRoot) / m_totalRoots) * 100.0;
    updateProgress(m_progress);

    // Update memory info
    try {
        m_memoryInfo.currentUsage = estimateMemoryRequirements(m_filePath) * m_progress.progressPercent / 100.0;
        updateMemoryInfo(m_memoryInfo);
    } catch (...) {
        // Ignore memory estimation errors
    }

    // Notify loader about the chunk if we have shapes
    if (!shapes.empty() && m_loader) {
        size_t chunkIndex = m_currentChunkIndex++;
        LOG_INF_S("Notifying loader about chunk " + std::to_string(chunkIndex) +
                 " with " + std::to_string(shapes.size()) + " shapes");
        m_loader->processLoadedChunk(shapes, chunkIndex);
    }

    // Return true if we have shapes OR if there are more chunks to process
    if (!shapes.empty() || hasMoreChunks) {
        return true;
    }
    
    // All done
    m_progress.isComplete = true;
    m_isLoading = false;
    updateProgress(m_progress);
    return false;
}

StreamingFileReader::LoadingProgress StreamingSTEPReader::getProgress() const
{
    return m_progress;
}

StreamingFileReader::MemoryInfo StreamingSTEPReader::getMemoryInfo() const
{
    return m_memoryInfo;
}

void StreamingSTEPReader::cancelLoading()
{
    m_cancelRequested = true;
    m_isLoading = false;
    
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    
    if (m_stepReader) {
        delete m_stepReader;
        m_stepReader = nullptr;
    }
}

bool StreamingSTEPReader::isLoading() const
{
    return m_isLoading;
}

std::vector<std::string> StreamingSTEPReader::getSupportedExtensions() const
{
    return {".step", ".stp"};
}

bool StreamingSTEPReader::openFile(const std::string& filePath)
{
    // Not used in new implementation - file is read in loadFile()
    return true;
}

bool StreamingSTEPReader::parseNextChunk()
{
    LOG_DBG_S("parseNextChunk: currentRoot=" + std::to_string(m_currentRoot) + 
             ", totalRoots=" + std::to_string(m_totalRoots));
    
    // Check if we have more roots to process
    if (m_currentRoot >= m_totalRoots || m_cancelRequested || !m_stepReader) {
        LOG_DBG_S("parseNextChunk: returning false (no more roots or cancelled)");
        return false;
    }

    // We'll process roots in batches
    Standard_Integer batchSize = static_cast<Standard_Integer>(m_config.maxShapesPerChunk);
    batchSize = std::min(batchSize, m_totalRoots - m_currentRoot);
    
    LOG_INF_S("parseNextChunk: processing batch of " + std::to_string(batchSize) + " roots");
    
    if (batchSize <= 0) {
        LOG_DBG_S("parseNextChunk: batchSize <= 0, returning false");
        return false;
    }

    // Transfer next batch of roots
    LOG_INF_S("Transferring roots " + std::to_string(m_currentRoot + 1) + 
             " to " + std::to_string(m_currentRoot + batchSize));
    
    for (Standard_Integer i = 0; i < batchSize && m_currentRoot < m_totalRoots; i++) {
        if (m_cancelRequested) break;
        
        m_currentRoot++;
        
        LOG_DBG_S("Transferring root " + std::to_string(m_currentRoot));
        
        // Transfer this root (1-based indexing in OpenCASCADE)
        if (!m_stepReader->TransferRoot(m_currentRoot)) {
            LOG_WRN_S("Failed to transfer root " + std::to_string(m_currentRoot));
        }
    }

    LOG_INF_S("parseNextChunk completed, processed " + std::to_string(m_currentRoot) + 
             " of " + std::to_string(m_totalRoots) + " roots");

    return m_currentRoot < m_totalRoots; // More data available
}

void StreamingSTEPReader::processSTEPEntity(const std::string& entity)
{
    // Not used in the new implementation
    // Keeping for compatibility
}

bool StreamingSTEPReader::extractShapesFromEntities(std::vector<TopoDS_Shape>& shapes)
{
    LOG_DBG_S("extractShapesFromEntities called");
    
    if (!m_stepReader) {
        LOG_ERR_S("Step reader is null in extractShapesFromEntities");
        return false;
    }

    try {
        // Get all shapes transferred so far
        Standard_Integer nbShapes = m_stepReader->NbShapes();
        
        LOG_INF_S("extractShapesFromEntities: reader has " + std::to_string(nbShapes) + " shapes");
        
        if (nbShapes == 0) {
            LOG_WRN_S("No shapes available from STEP reader");
            return false; // Changed to false - this is actually an error if we transferred roots
        }

        // Extract shapes from the reader
        for (Standard_Integer i = 1; i <= nbShapes; i++) {
            if (m_cancelRequested) break;
            
            LOG_DBG_S("Getting shape " + std::to_string(i) + " of " + std::to_string(nbShapes));
            
            TopoDS_Shape shape = m_stepReader->Shape(i);
            if (!shape.IsNull()) {
                shapes.push_back(shape);
                LOG_DBG_S("Shape " + std::to_string(i) + " added, type: " + std::to_string(shape.ShapeType()));
            } else {
                LOG_WRN_S("Shape " + std::to_string(i) + " is null");
            }
        }
        
        LOG_INF_S("Extracted " + std::to_string(shapes.size()) + " valid shapes from " + 
                 std::to_string(nbShapes) + " total shapes");
        
        // Update progress
        m_progress.shapesLoaded = m_currentRoot;
        m_progress.progressPercent = (static_cast<double>(m_currentRoot) / m_totalRoots) * 100.0;

        return shapes.size() > 0; // Return true only if we got shapes
    } catch (const Standard_Failure& e) {
        LOG_ERR_S("OpenCASCADE error extracting shapes: " + std::string(e.GetMessageString()));
        return false;
    } catch (const std::exception& e) {
        LOG_ERR_S("Error extracting shapes from STEP entities: " + std::string(e.what()));
        return false;
    }
}

size_t StreamingSTEPReader::countEntitiesInFile() const
{
    // Return the actual number of roots
    return m_totalRoots;
}

// =====================================================================================
// StreamingIGESReader Implementation
// =====================================================================================

StreamingIGESReader::StreamingIGESReader()
    : m_currentPosition(0)
    , m_fileSize(0)
    , m_processedEntries(0)
{
}

StreamingIGESReader::~StreamingIGESReader()
{
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

bool StreamingIGESReader::loadFile(const std::string& filePath, const LoadingConfig& config)
{
    m_config = config;
    m_filePath = filePath;
    m_isLoading = true;
    m_cancelRequested = false;

    if (!openFile(filePath)) {
        LOG_ERR_S("Failed to open IGES file: " + filePath);
        m_isLoading = false;
        return false;
    }

    return true;
}

bool StreamingIGESReader::getNextChunk(std::vector<TopoDS_Shape>& shapes)
{
    if (!m_isLoading || m_cancelRequested) {
        return false;
    }

    shapes.clear();

    // Simplified implementation - just return some dummy shapes for now
    size_t shapesToCreate = std::min(size_t(5), m_config.maxShapesPerChunk);
    for (size_t i = 0; i < shapesToCreate; ++i) {
        TopoDS_Shape shape; // Empty shape placeholder
        shapes.push_back(shape);
    }

    m_progress.shapesLoaded += shapes.size();
    m_progress.progressPercent = 50.0; // Dummy progress

    if (m_progress.shapesLoaded > 100) { // Arbitrary limit
        m_progress.isComplete = true;
        m_isLoading = false;
    }

    updateProgress(m_progress);
    return !m_progress.isComplete;
}

StreamingFileReader::LoadingProgress StreamingIGESReader::getProgress() const
{
    return m_progress;
}

StreamingFileReader::MemoryInfo StreamingIGESReader::getMemoryInfo() const
{
    return m_memoryInfo;
}

void StreamingIGESReader::cancelLoading()
{
    m_cancelRequested = true;
    m_isLoading = false;
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

bool StreamingIGESReader::isLoading() const
{
    return m_isLoading;
}

std::vector<std::string> StreamingIGESReader::getSupportedExtensions() const
{
    return {".iges", ".igs"};
}

bool StreamingIGESReader::openFile(const std::string& filePath)
{
    try {
        m_fileStream.open(filePath, std::ios::binary);
        if (!m_fileStream.is_open()) {
            return false;
        }

        m_fileStream.seekg(0, std::ios::end);
        m_fileSize = m_fileStream.tellg();
        m_fileStream.seekg(0, std::ios::beg);

        m_currentPosition = 0;
        return true;
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception opening IGES file: " + std::string(e.what()));
        return false;
    }
}

// =====================================================================================
// Factory Function
// =====================================================================================

std::unique_ptr<StreamingFileReader> createStreamingReader(const std::string& filePath)
{
    std::string ext = StreamingFileReader::getFileExtension(filePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".step" || ext == ".stp") {
        return std::make_unique<StreamingSTEPReader>();
    } else if (ext == ".iges" || ext == ".igs") {
        return std::make_unique<StreamingIGESReader>();
    }

    return nullptr;
}
