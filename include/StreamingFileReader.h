#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TColStd_SequenceOfAsciiString.hxx>
#include "logger/Logger.h"

/**
 * @brief Streaming file reader for large CAD models
 *
 * Provides progressive loading capabilities for large CAD files that exceed
 * available memory. Supports chunked reading, memory management, and
 * progressive rendering updates.
 */
class StreamingFileReader {
public:
    /**
     * @brief Reading modes
     */
    enum class ReadMode {
        FullLoad,       // Load entire file at once (traditional)
        Progressive,    // Load in chunks with progress updates
        MemoryMapped    // Use memory mapping for very large files
    };

    /**
     * @brief Loading progress information
     */
    struct LoadingProgress {
        size_t bytesRead = 0;           // Bytes read so far
        size_t totalBytes = 0;          // Total file size
        size_t shapesLoaded = 0;        // Shapes loaded so far
        size_t totalShapes = 0;         // Estimated total shapes
        double progressPercent = 0.0;   // Loading progress (0-100)
        std::string currentOperation;    // Current loading operation
        bool isComplete = false;        // Loading completed
    };

    /**
     * @brief Memory usage information
     */
    struct MemoryInfo {
        size_t currentUsage = 0;        // Current memory usage (bytes)
        size_t peakUsage = 0;          // Peak memory usage (bytes)
        size_t availableMemory = 0;    // Available system memory (bytes)
        bool memoryWarning = false;    // Memory usage warning flag
    };

    /**
     * @brief Loading configuration
     */
    struct LoadingConfig {
        ReadMode mode = ReadMode::Progressive;
        size_t maxMemoryUsage = 1024 * 1024 * 1024; // 1GB default
        size_t chunkSize = 64 * 1024;               // 64KB chunks
        size_t maxShapesPerChunk = 100;             // Shapes per chunk
        bool enableMemoryMapping = false;           // Use memory mapping
        std::function<void(const LoadingProgress&)> progressCallback;
        std::function<void(const MemoryInfo&)> memoryCallback;
    };

    StreamingFileReader();
    virtual ~StreamingFileReader();

    /**
     * @brief Load file with streaming capabilities
     * @param filePath Path to the CAD file
     * @param config Loading configuration
     * @return True if loading started successfully
     */
    virtual bool loadFile(const std::string& filePath, const LoadingConfig& config) = 0;

    /**
     * @brief Get next chunk of geometry
     * @param shapes Output vector for loaded shapes
     * @return True if more data available, false if loading complete
     */
    virtual bool getNextChunk(std::vector<TopoDS_Shape>& shapes) = 0;

    /**
     * @brief Get current loading progress
     * @return Current loading progress
     */
    virtual LoadingProgress getProgress() const = 0;

    /**
     * @brief Get current memory usage
     * @return Current memory information
     */
    virtual MemoryInfo getMemoryInfo() const = 0;

    /**
     * @brief Cancel loading operation
     */
    virtual void cancelLoading() = 0;

    /**
     * @brief Check if loading is in progress
     * @return True if currently loading
     */
    virtual bool isLoading() const = 0;

    /**
     * @brief Get supported file extensions
     * @return Vector of supported file extensions
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;

    /**
     * @brief Get estimated file size for streaming decision
     * @param filePath Path to the file
     * @return Estimated processing requirements
     */
    static bool shouldUseStreaming(const std::string& filePath, size_t& estimatedSize);

    /**
     * @brief Get optimal chunk size for file size
     * @param fileSize File size in bytes
     * @return Recommended chunk size
     */
    static size_t getOptimalChunkSize(size_t fileSize);

    /**
     * @brief Check if file format supports streaming
     * @param filePath Path to the file
     * @return True if streaming is supported
     */
    static bool supportsStreaming(const std::string& filePath);

    /**
     * @brief Get file extension from file path
     * @param filePath Path to the file
     * @return File extension (including dot)
     */
    static std::string getFileExtension(const std::string& filePath);

protected:
    LoadingConfig m_config;
    LoadingProgress m_progress;
    MemoryInfo m_memoryInfo;
    bool m_isLoading;
    bool m_cancelRequested;

    /**
     * @brief Update progress information
     * @param progress New progress information
     */
    void updateProgress(const LoadingProgress& progress);

    /**
     * @brief Update memory information
     * @param memory New memory information
     */
    void updateMemoryInfo(const MemoryInfo& memory);

    /**
     * @brief Check if operation was cancelled
     * @return True if cancelled
     */
    bool isCancelled() const { return m_cancelRequested; }

    /**
     * @brief Estimate memory requirements for file
     * @param filePath File path
     * @return Estimated memory usage
     */
    size_t estimateMemoryRequirements(const std::string& filePath) const;
};

// Streaming STEP reader implementation
class StreamingSTEPReader : public StreamingFileReader {
public:
    StreamingSTEPReader();
    ~StreamingSTEPReader() override;

    bool loadFile(const std::string& filePath, const LoadingConfig& config) override;
    bool getNextChunk(std::vector<TopoDS_Shape>& shapes) override;
    LoadingProgress getProgress() const override;
    MemoryInfo getMemoryInfo() const override;
    void cancelLoading() override;
    bool isLoading() const override;
    std::vector<std::string> getSupportedExtensions() const override;

private:
    std::string m_filePath;
    std::ifstream m_fileStream;
    TColStd_SequenceOfAsciiString m_stepData;
    size_t m_currentPosition;
    size_t m_fileSize;

    // STEP parsing state
    std::vector<std::string> m_entityBuffer;
    size_t m_processedEntities;

    // Helper methods
    bool openFile(const std::string& filePath);
    bool parseNextChunk();
    bool extractShapesFromEntities(std::vector<TopoDS_Shape>& shapes);
    void processSTEPEntity(const std::string& entity);
    bool isEntityComplete(const std::string& entity) const;
    size_t countEntitiesInFile() const;
};

// Streaming IGES reader implementation
class StreamingIGESReader : public StreamingFileReader {
public:
    StreamingIGESReader();
    ~StreamingIGESReader() override;

    bool loadFile(const std::string& filePath, const LoadingConfig& config) override;
    bool getNextChunk(std::vector<TopoDS_Shape>& shapes) override;
    LoadingProgress getProgress() const override;
    MemoryInfo getMemoryInfo() const override;
    void cancelLoading() override;
    bool isLoading() const override;
    std::vector<std::string> getSupportedExtensions() const override;

private:
    std::string m_filePath;
    std::ifstream m_fileStream;
    size_t m_currentPosition;
    size_t m_fileSize;

    // IGES parsing state
    std::vector<std::string> m_directoryEntries;
    std::vector<std::string> m_parameterData;
    size_t m_processedEntries;

    // Helper methods
    bool openFile(const std::string& filePath);
    bool parseIGESHeader();
    bool parseNextChunk();
    bool extractShapesFromIGES(std::vector<TopoDS_Shape>& shapes);
};

// Factory function for creating appropriate reader
std::unique_ptr<StreamingFileReader> createStreamingReader(const std::string& filePath);
