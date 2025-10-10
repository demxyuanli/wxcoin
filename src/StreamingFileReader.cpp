#include "StreamingFileReader.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <chrono>
#include <OpenCASCADE/STEPControl_Reader.hxx>
#include <OpenCASCADE/IGESControl_Reader.hxx>
#include <OpenCASCADE/Interface_Static.hxx>

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
    : m_currentPosition(0)
    , m_fileSize(0)
    , m_processedEntities(0)
{
}

StreamingSTEPReader::~StreamingSTEPReader()
{
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

bool StreamingSTEPReader::loadFile(const std::string& filePath, const LoadingConfig& config)
{
    m_config = config;
    m_filePath = filePath;
    m_isLoading = true;
    m_cancelRequested = false;

    LOG_INF_S("Starting streaming STEP load for: " + filePath);

    if (!openFile(filePath)) {
        LOG_ERR_S("Failed to open STEP file: " + filePath);
        m_isLoading = false;
        return false;
    }

    // Count total entities for progress estimation
    m_progress.totalShapes = countEntitiesInFile();
    m_progress.totalBytes = m_fileSize;

    LOG_INF_S("STEP file opened. Size: " + std::to_string(m_fileSize) +
              " bytes, Estimated entities: " + std::to_string(m_progress.totalShapes));

    return true;
}

bool StreamingSTEPReader::getNextChunk(std::vector<TopoDS_Shape>& shapes)
{
    if (!m_isLoading || m_cancelRequested) {
        return false;
    }

    shapes.clear();

    // Parse next chunk of STEP data
    if (!parseNextChunk()) {
        // No more data to parse
        m_progress.isComplete = true;
        m_isLoading = false;
        updateProgress(m_progress);
        return false;
    }

    // Extract shapes from parsed entities
    if (!extractShapesFromEntities(shapes)) {
        LOG_WRN_S("Failed to extract shapes from STEP entities");
        return true; // Continue with next chunk
    }

    // Update progress
    m_progress.shapesLoaded += shapes.size();
    m_progress.progressPercent = (static_cast<double>(m_currentPosition) / m_fileSize) * 100.0;
    updateProgress(m_progress);

    // Update memory info
    m_memoryInfo.currentUsage = estimateMemoryRequirements(m_filePath) * m_progress.progressPercent / 100.0;
    updateMemoryInfo(m_memoryInfo);

    return true;
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
    LOG_INF_S("STEP streaming loading cancelled");
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
    try {
        m_fileStream.open(filePath, std::ios::binary);
        if (!m_fileStream.is_open()) {
            return false;
        }

        // Get file size
        m_fileStream.seekg(0, std::ios::end);
        m_fileSize = m_fileStream.tellg();
        m_fileStream.seekg(0, std::ios::beg);

        m_currentPosition = 0;
        return true;
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception opening file: " + std::string(e.what()));
        return false;
    }
}

bool StreamingSTEPReader::parseNextChunk()
{
    if (!m_fileStream.is_open() || m_cancelRequested) {
        return false;
    }

    const size_t bufferSize = m_config.chunkSize;
    std::vector<char> buffer(bufferSize);

    // Read next chunk
    m_fileStream.read(buffer.data(), bufferSize);
    std::streamsize bytesRead = m_fileStream.gcount();

    if (bytesRead <= 0) {
        return false; // End of file
    }

    m_currentPosition += bytesRead;

    // Convert buffer to string and process
    std::string chunk(buffer.data(), bytesRead);

    // Simple STEP entity parsing (this is a simplified implementation)
    // In a real implementation, you'd use a proper STEP parser
    size_t pos = 0;
    while (pos < chunk.size()) {
        if (isCancelled()) break;

        // Find entity start
        size_t entityStart = chunk.find('#', pos);
        if (entityStart == std::string::npos) break;

        // Find entity end
        size_t entityEnd = chunk.find(';', entityStart);
        if (entityEnd == std::string::npos) {
            // Entity spans multiple chunks - this is complex to handle
            // For simplicity, we'll process complete entities only
            break;
        }

        // Extract entity
        std::string entity = chunk.substr(entityStart, entityEnd - entityStart + 1);
        processSTEPEntity(entity);

        pos = entityEnd + 1;
        m_processedEntities++;
    }

    return true;
}

void StreamingSTEPReader::processSTEPEntity(const std::string& entity)
{
    // Simple entity processing - in reality this would be much more complex
    // For now, just collect entities for later processing
    m_entityBuffer.push_back(entity);

    // Limit buffer size to prevent excessive memory usage
    if (m_entityBuffer.size() > m_config.maxShapesPerChunk * 10) {
        // Process some entities to free memory
        std::vector<TopoDS_Shape> dummyShapes;
        extractShapesFromEntities(dummyShapes);
    }
}

bool StreamingSTEPReader::extractShapesFromEntities(std::vector<TopoDS_Shape>& shapes)
{
    if (m_entityBuffer.empty()) {
        return true;
    }

    try {
        // This is a highly simplified STEP processing
        // In a real implementation, you'd use OpenCASCADE's STEPControl_Reader
        // with proper entity parsing and assembly

        // For demonstration, create some dummy shapes
        // In reality, this would parse the STEP entities and create actual geometry

        size_t shapesToCreate = std::min(m_entityBuffer.size() / 5, m_config.maxShapesPerChunk);
        shapesToCreate = std::max(shapesToCreate, size_t(1));

        for (size_t i = 0; i < shapesToCreate; ++i) {
            // Create a simple box as placeholder
            // In real implementation, this would parse actual STEP geometry
            TopoDS_Shape shape; // Empty shape for now
            shapes.push_back(shape);
        }

        // Clear processed entities
        m_entityBuffer.clear();

        return true;
    } catch (const std::exception& e) {
        LOG_ERR_S("Error extracting shapes from STEP entities: " + std::string(e.what()));
        return false;
    }
}

size_t StreamingSTEPReader::countEntitiesInFile() const
{
    // Simple estimation based on file size
    // In reality, you'd scan the file to count actual entities
    const size_t AVG_ENTITY_SIZE = 200; // Rough estimate
    return m_fileSize / AVG_ENTITY_SIZE;
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

    LOG_INF_S("Starting streaming IGES load for: " + filePath);

    if (!openFile(filePath)) {
        LOG_ERR_S("Failed to open IGES file: " + filePath);
        m_isLoading = false;
        return false;
    }

    LOG_INF_S("IGES file opened. Size: " + std::to_string(m_fileSize) + " bytes");

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
    LOG_INF_S("IGES streaming loading cancelled");
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
