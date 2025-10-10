#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include "StreamingFileReader.h"
#include "logger/Logger.h"

/**
 * @brief Progressive geometry loader for large CAD models
 *
 * Manages the progressive loading and rendering of large CAD files.
 * Provides real-time progress updates, memory management, and smooth
 * integration with the CAD viewer.
 */
class ProgressiveGeometryLoader {
public:
    /**
     * @brief Loading states
     */
    enum class LoadingState {
        Idle,           // Not loading
        Preparing,      // Setting up for loading
        Loading,        // Actively loading chunks
        Rendering,      // Rendering loaded chunks
        Paused,         // Loading paused
        Completed,      // Loading completed successfully
        Cancelled,      // Loading was cancelled
        Error          // Loading failed with error
    };

    /**
     * @brief Render chunk information
     */
    struct RenderChunk {
        std::vector<TopoDS_Shape> shapes;
        size_t chunkIndex;
        bool isRendered = false;
        double loadTime = 0.0;  // Time to load this chunk
    };

    /**
     * @brief Loading statistics
     */
    struct LoadingStats {
        size_t totalChunks = 0;
        size_t loadedChunks = 0;
        size_t renderedChunks = 0;
        size_t totalShapes = 0;
        size_t renderedShapes = 0;
        double averageLoadTime = 0.0;
        double totalLoadTime = 0.0;
        size_t memoryUsage = 0;
        size_t peakMemoryUsage = 0;
    };

    /**
     * @brief Loading configuration
     */
    struct LoadingConfiguration {
        std::string filePath;
        StreamingFileReader::LoadingConfig streamConfig;
        size_t maxConcurrentChunks = 2;      // Maximum chunks to load concurrently
        size_t renderBatchSize = 50;         // Shapes per render batch
        bool autoStartRendering = true;      // Start rendering as chunks load
        bool enableMemoryManagement = true;  // Enable memory usage monitoring
        double targetFrameRate = 30.0;       // Target frame rate for smooth rendering
    };

    /**
     * @brief Event callbacks
     */
    struct Callbacks {
        std::function<void(const RenderChunk&)> onChunkRendered;
        std::function<void(const LoadingStats&)> onStatsUpdated;
        std::function<void(LoadingState, const std::string&)> onStateChanged;
        std::function<void(const std::string&)> onError;
        std::function<void(double)> onProgress;  // Progress 0.0 to 1.0
    };

    ProgressiveGeometryLoader();
    ~ProgressiveGeometryLoader();

    /**
     * @brief Start progressive loading
     * @param config Loading configuration
     * @param callbacks Event callbacks
     * @return True if loading started successfully
     */
    bool startLoading(const LoadingConfiguration& config, const Callbacks& callbacks);

    /**
     * @brief Pause loading
     */
    void pauseLoading();

    /**
     * @brief Resume loading
     */
    void resumeLoading();

    /**
     * @brief Cancel loading
     */
    void cancelLoading();

    /**
     * @brief Get current loading state
     * @return Current loading state
     */
    LoadingState getState() const { return m_state; }

    /**
     * @brief Get loading statistics
     * @return Current loading statistics
     */
    LoadingStats getStats() const;

    /**
     * @brief Get loading progress (0.0 to 1.0)
     * @return Loading progress
     */
    double getProgress() const;

    /**
     * @brief Check if file should be loaded progressively
     * @param filePath Path to the file
     * @return True if progressive loading is recommended
     */
    static bool shouldLoadProgressively(const std::string& filePath) {
        size_t fileSize;
        return StreamingFileReader::shouldUseStreaming(filePath, fileSize);
    }

    /**
     * @brief Get recommended configuration for file
     * @param filePath Path to the file
     * @return Recommended loading configuration
     */
    static LoadingConfiguration getRecommendedConfig(const std::string& filePath);

    /**
     * @brief Process a loaded chunk (called by streaming reader)
     * @param shapes Loaded shapes
     * @param chunkIndex Index of the chunk
     */
    void processLoadedChunk(const std::vector<TopoDS_Shape>& shapes, size_t chunkIndex);

private:
    LoadingState m_state;
    LoadingConfiguration m_config;
    Callbacks m_callbacks;

    std::unique_ptr<StreamingFileReader> m_streamReader;
    std::vector<RenderChunk> m_renderChunks;
    LoadingStats m_stats;

    // Threading
    std::thread m_loadingThread;
    std::thread m_renderingThread;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_shouldStop;
    std::atomic<bool> m_isPaused;

    // Timing
    std::chrono::steady_clock::time_point m_startTime;
    std::vector<double> m_chunkLoadTimes;

    // Helper methods
    void loadingThreadFunc();
    void renderingThreadFunc();
    void updateStats();
    void changeState(LoadingState newState, const std::string& message = "");
    void handleError(const std::string& error);

    // Memory management
    void monitorMemoryUsage();
    bool shouldThrottleLoading() const;
    void cleanupOldChunks();

    // Utility methods
    size_t calculateMemoryUsage(const RenderChunk& chunk) const;
    double calculateAverageLoadTime() const;
    bool isFileSupported(const std::string& filePath) const;
};

// =====================================================================================
// UI Integration Classes
// =====================================================================================

/**
 * @brief Progress dialog for progressive loading
 */
class ProgressiveLoadingDialog {
public:
    ProgressiveLoadingDialog(wxWindow* parent, ProgressiveGeometryLoader* loader);
    ~ProgressiveLoadingDialog();

    void show();
    void hide();
    void updateProgress();

private:
    wxWindow* m_parent;
    ProgressiveGeometryLoader* m_loader;
    wxDialog* m_dialog;
    wxGauge* m_progressBar;
    wxStaticText* m_statusText;
    wxStaticText* m_statsText;
    wxButton* m_pauseButton;
    wxButton* m_cancelButton;

    void onPause(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onClose(wxCloseEvent& event);
    void updateDisplay();
};

/**
 * @brief Memory monitor widget
 */
class MemoryMonitorWidget {
public:
    MemoryMonitorWidget(wxWindow* parent);
    ~MemoryMonitorWidget();

    void updateMemoryInfo(size_t currentUsage, size_t peakUsage, size_t available);
    void showWarning(bool show);

private:
    wxWindow* m_parent;
    wxPanel* m_panel;
    wxStaticText* m_memoryText;
    wxGauge* m_memoryGauge;
    wxStaticBitmap* m_warningIcon;
};

// =====================================================================================
// Integration with existing systems
// =====================================================================================

/**
 * @brief Extended geometry reader with progressive loading support
 */
class ProgressiveGeometryReader {
public:
    /**
     * @brief Load geometry with automatic mode selection
     * @param filePath Path to the geometry file
     * @param shapes Output vector for loaded shapes
     * @param loader Optional progressive loader for large files
     * @return True if loading successful
     */
    static bool loadGeometry(const std::string& filePath,
                           std::vector<TopoDS_Shape>& shapes,
                           ProgressiveGeometryLoader* loader = nullptr);

    /**
     * @brief Check if progressive loading is available for file
     * @param filePath Path to the file
     * @return True if progressive loading is supported and recommended
     */
    static bool isProgressiveLoadingAvailable(const std::string& filePath);
};
