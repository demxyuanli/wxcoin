#include "ProgressiveGeometryLoader.h"
#include <wx/wx.h>
#include <wx/gauge.h>
#include <wx/statbmp.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

ProgressiveGeometryLoader::ProgressiveGeometryLoader()
    : m_state(LoadingState::Idle)
    , m_shouldStop(false)
    , m_isPaused(false)
{
}

ProgressiveGeometryLoader::~ProgressiveGeometryLoader()
{
    cancelLoading();

    if (m_loadingThread.joinable()) {
        m_loadingThread.join();
    }
    if (m_renderingThread.joinable()) {
        m_renderingThread.join();
    }
}

bool ProgressiveGeometryLoader::startLoading(const LoadingConfiguration& config,
                                           const Callbacks& callbacks)
{
    LOG_INF_S("ProgressiveGeometryLoader::startLoading called for: " + config.filePath);
    
    if (m_state != LoadingState::Idle) {
        LOG_WRN_S("Cannot start loading: loader is not idle, state=" + std::to_string(static_cast<int>(m_state)));
        return false;
    }

    if (!isFileSupported(config.filePath)) {
        LOG_ERR_S("File format not supported for progressive loading: " + config.filePath);
        return false;
    }

    m_config = config;
    m_callbacks = callbacks;
    m_shouldStop = false;
    m_isPaused = false;
    m_startTime = std::chrono::steady_clock::now();
    m_chunkLoadTimes.clear();
    m_renderChunks.clear();

    LOG_INF_S("Creating streaming reader");
    
    // Create appropriate streaming reader
    m_streamReader = createStreamingReader(config.filePath);
    if (!m_streamReader) {
        LOG_ERR_S("Failed to create streaming reader for: " + config.filePath);
        return false;
    }

    // Set loader reference for chunk notifications
    m_streamReader->setLoader(this);

    LOG_INF_S("Starting streaming reader");
    
    // Start streaming reader
    if (!m_streamReader->loadFile(config.filePath, config.streamConfig)) {
        LOG_ERR_S("Failed to start streaming reader for: " + config.filePath);
        return false;
    }

    LOG_INF_S("Streaming reader started successfully");
    
    changeState(LoadingState::Preparing, "Preparing for progressive loading...");

    LOG_INF_S("Starting loading and rendering threads");
    
    // Start loading and rendering threads
    m_loadingThread = std::thread(&ProgressiveGeometryLoader::loadingThreadFunc, this);
    m_renderingThread = std::thread(&ProgressiveGeometryLoader::renderingThreadFunc, this);

    LOG_INF_S("Threads started");
    
    return true;
}

void ProgressiveGeometryLoader::pauseLoading()
{
    if (m_state == LoadingState::Loading) {
        m_isPaused = true;
        changeState(LoadingState::Paused, "Loading paused");
    }
}

void ProgressiveGeometryLoader::resumeLoading()
{
    if (m_state == LoadingState::Paused) {
        m_isPaused = false;
        changeState(LoadingState::Loading, "Loading resumed");
        m_condition.notify_all();
    }
}

void ProgressiveGeometryLoader::cancelLoading()
{
    if (m_state == LoadingState::Idle || m_state == LoadingState::Completed ||
        m_state == LoadingState::Error) {
        return;
    }

    m_shouldStop = true;
    m_isPaused = false;
    m_condition.notify_all();

    if (m_streamReader) {
        m_streamReader->cancelLoading();
    }

    changeState(LoadingState::Cancelled, "Loading cancelled by user");
}

ProgressiveGeometryLoader::LoadingStats ProgressiveGeometryLoader::getStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    LoadingStats statsCopy = m_stats;
    return statsCopy;
}

double ProgressiveGeometryLoader::getProgress() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stats.totalChunks == 0) return 0.0;
    double progress = static_cast<double>(m_stats.loadedChunks) / m_stats.totalChunks;
    return progress;
}

void ProgressiveGeometryLoader::loadingThreadFunc()
{
    LOG_INF_S("Loading thread started");
    changeState(LoadingState::Loading, "Loading geometry chunks...");

    size_t chunkIndex = 0;
    while (!m_shouldStop) {
        if (m_isPaused) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this]() { return !m_isPaused || m_shouldStop; });
            if (m_shouldStop) break;
        }

        if (shouldThrottleLoading()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Load next chunk
        LOG_DBG_S("Loading thread: calling getNextChunk");
        std::vector<TopoDS_Shape> shapes;
        
        if (!m_streamReader) {
            LOG_ERR_S("Stream reader is null!");
            break;
        }
        
        if (!m_streamReader->getNextChunk(shapes)) {
            // No more chunks available
            LOG_INF_S("No more chunks available, loading complete");
            break;
        }

        LOG_INF_S("Loading thread: got " + std::to_string(shapes.size()) + " shapes");

        if (!shapes.empty()) {
            auto startTime = std::chrono::steady_clock::now();
            processLoadedChunk(shapes, chunkIndex++);
            auto endTime = std::chrono::steady_clock::now();

            double loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - startTime).count() / 1000.0;
            m_chunkLoadTimes.push_back(loadTime);
        }

        // Small delay to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_INF_S("Loading thread finishing");
    if (!m_shouldStop) {
        changeState(LoadingState::Completed, "Loading completed successfully");
        // Stop rendering thread since all chunks are loaded
        m_shouldStop = true;
        LOG_INF_S("Signaling rendering thread to stop");
    }
    LOG_INF_S("Loading thread completed");
}

void ProgressiveGeometryLoader::renderingThreadFunc()
{
    LOG_INF_S("Rendering thread started");

    while (!m_shouldStop) {
        bool hasWork = false;
        RenderChunk chunkToRender;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            LOG_DBG_S("Rendering thread: checking for work, chunks count: " + std::to_string(m_renderChunks.size()));

            // Find unrendered chunks
            for (auto& chunk : m_renderChunks) {
                LOG_DBG_S("Rendering thread: chunk rendered=" + std::to_string(chunk.isRendered) +
                         ", shapes=" + std::to_string(chunk.shapes.size()));
                if (!chunk.isRendered && !chunk.shapes.empty()) {
                    chunkToRender = chunk;
                    chunk.isRendered = true;
                    m_stats.renderedChunks++;
                    m_stats.renderedShapes += chunk.shapes.size();
                    hasWork = true;
                    LOG_DBG_S("Rendering thread: found work, shapes=" + std::to_string(chunk.shapes.size()));
                    break;
                }
            }
        }

        // Call callback outside the lock
        if (hasWork && m_callbacks.onChunkRendered) {
            LOG_INF_S("Rendering thread: calling onChunkRendered callback with " +
                     std::to_string(chunkToRender.shapes.size()) + " shapes");
            m_callbacks.onChunkRendered(chunkToRender);
        }

        if (!hasWork) {
            // No work to do, sleep a bit
            LOG_DBG_S("Rendering thread: no work, sleeping");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        updateStats();
    }
}

void ProgressiveGeometryLoader::processLoadedChunk(const std::vector<TopoDS_Shape>& shapes,
                                                 size_t chunkIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_INF_S("ProgressiveGeometryLoader: processing loaded chunk " + std::to_string(chunkIndex) +
             " with " + std::to_string(shapes.size()) + " shapes");

    RenderChunk chunk;
    chunk.shapes = shapes;
    chunk.chunkIndex = chunkIndex;
    chunk.loadTime = m_chunkLoadTimes.empty() ? 0.0 : m_chunkLoadTimes.back();

    m_renderChunks.push_back(chunk);

    LOG_INF_S("ProgressiveGeometryLoader: added chunk to render queue, total chunks: " +
             std::to_string(m_renderChunks.size()));

    // Update stats
    m_stats.loadedChunks++;
    m_stats.totalShapes += shapes.size();
    m_stats.memoryUsage += calculateMemoryUsage(chunk);
    m_stats.peakMemoryUsage = std::max(m_stats.peakMemoryUsage, m_stats.memoryUsage);

    // Estimate total chunks (rough estimate)
    if (m_stats.totalChunks == 0 && m_stats.loadedChunks == 1) {
        // Estimate based on first chunk
        size_t avgShapesPerChunk = shapes.size();
        size_t estimatedTotalShapes = 1000; // Rough estimate, would be better from file analysis
        m_stats.totalChunks = std::max(size_t(1), estimatedTotalShapes / avgShapesPerChunk);
    }
}

void ProgressiveGeometryLoader::updateStats()
{
    LoadingStats statsCopy;
    double progressValue = 0.0;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_stats.averageLoadTime = calculateAverageLoadTime();
        m_stats.totalLoadTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - m_startTime).count();

        statsCopy = m_stats;
        
        // Calculate progress without calling getProgress() to avoid recursive lock
        if (m_stats.totalChunks > 0) {
            progressValue = static_cast<double>(m_stats.loadedChunks) / m_stats.totalChunks;
        }
    }
    
    // Call callbacks outside the lock to avoid blocking
    if (m_callbacks.onStatsUpdated) {
        m_callbacks.onStatsUpdated(statsCopy);
    }

    if (m_callbacks.onProgress) {
        m_callbacks.onProgress(progressValue);
    }
}

void ProgressiveGeometryLoader::changeState(LoadingState newState, const std::string& message)
{
    m_state = newState;

    // Call callback outside any locks to prevent deadlock
    if (m_callbacks.onStateChanged) {
        m_callbacks.onStateChanged(newState, message);
    }
}

void ProgressiveGeometryLoader::handleError(const std::string& error)
{
    changeState(LoadingState::Error, error);

    if (m_callbacks.onError) {
        m_callbacks.onError(error);
    }

    LOG_ERR_S("Progressive loading error: " + error);
}

void ProgressiveGeometryLoader::monitorMemoryUsage()
{
    // Basic memory monitoring - in a real implementation,
    // this would integrate with system memory APIs
    if (m_stats.memoryUsage > m_config.streamConfig.maxMemoryUsage * 0.9) {
        LOG_WRN_S("Memory usage approaching limit: " +
                 std::to_string(m_stats.memoryUsage / (1024*1024)) + " MB");
    }
}

bool ProgressiveGeometryLoader::shouldThrottleLoading() const
{
    // Throttle if too many unrendered chunks
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t unrenderedCount = 0;
    for (const auto& chunk : m_renderChunks) {
        if (!chunk.isRendered) unrenderedCount++;
    }

    return unrenderedCount >= m_config.maxConcurrentChunks;
}

void ProgressiveGeometryLoader::cleanupOldChunks()
{
    // Basic cleanup - remove rendered chunks that are no longer needed
    // In a real implementation, this would be more sophisticated
    std::lock_guard<std::mutex> lock(m_mutex);

    // Keep only recent chunks in memory
    const size_t MAX_CHUNKS_IN_MEMORY = 10;
    if (m_renderChunks.size() > MAX_CHUNKS_IN_MEMORY) {
        // Remove old rendered chunks (simplified)
        auto it = std::remove_if(m_renderChunks.begin(), m_renderChunks.end(),
            [](const RenderChunk& chunk) {
                return chunk.isRendered && chunk.chunkIndex < 5; // Keep recent ones
            });
        m_renderChunks.erase(it, m_renderChunks.end());
    }
}

size_t ProgressiveGeometryLoader::calculateMemoryUsage(const RenderChunk& chunk) const
{
    // Rough memory estimate: each shape ~1KB + overhead
    return chunk.shapes.size() * 1024 + sizeof(RenderChunk);
}

double ProgressiveGeometryLoader::calculateAverageLoadTime() const
{
    if (m_chunkLoadTimes.empty()) return 0.0;

    double sum = 0.0;
    for (double time : m_chunkLoadTimes) {
        sum += time;
    }
    return sum / m_chunkLoadTimes.size();
}

bool ProgressiveGeometryLoader::isFileSupported(const std::string& filePath) const
{
    return StreamingFileReader::supportsStreaming(filePath);
}

ProgressiveGeometryLoader::LoadingConfiguration
ProgressiveGeometryLoader::getRecommendedConfig(const std::string& filePath)
{
    LoadingConfiguration config;
    config.filePath = filePath;

    // Set streaming config based on file size
    size_t fileSize;
    if (StreamingFileReader::shouldUseStreaming(filePath, fileSize)) {
        config.streamConfig.mode = StreamingFileReader::ReadMode::Progressive;
        config.streamConfig.chunkSize = StreamingFileReader::getOptimalChunkSize(fileSize);
        config.streamConfig.maxMemoryUsage = 1024 * 1024 * 1024; // 1GB
    } else {
        config.streamConfig.mode = StreamingFileReader::ReadMode::FullLoad;
    }

    // Adjust other settings based on file size
    if (fileSize > 100 * 1024 * 1024) { // > 100MB
        config.maxConcurrentChunks = 1;  // Be more conservative
        config.renderBatchSize = 25;
    } else {
        config.maxConcurrentChunks = 2;
        config.renderBatchSize = 50;
    }

    return config;
}

// =====================================================================================
// UI Integration Implementation
// =====================================================================================

ProgressiveLoadingDialog::ProgressiveLoadingDialog(wxWindow* parent,
                                                 ProgressiveGeometryLoader* loader)
    : m_parent(parent)
    , m_loader(loader)
{
    m_dialog = new wxDialog(parent, wxID_ANY, "Progressive Loading",
                          wxDefaultPosition, wxSize(400, 200),
                          wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Progress bar
    m_progressBar = new wxGauge(m_dialog, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 20));
    mainSizer->Add(m_progressBar, 0, wxEXPAND | wxALL, 10);

    // Status text
    m_statusText = new wxStaticText(m_dialog, wxID_ANY, "Initializing...");
    mainSizer->Add(m_statusText, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);

    // Stats text
    m_statsText = new wxStaticText(m_dialog, wxID_ANY, "");
    mainSizer->Add(m_statsText, 0, wxALIGN_CENTER | wxALL, 10);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pauseButton = new wxButton(m_dialog, wxID_ANY, "Pause");
    m_cancelButton = new wxButton(m_dialog, wxID_ANY, "Cancel");

    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_pauseButton, 0, wxALL, 5);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);

    m_dialog->SetSizer(mainSizer);
    m_dialog->Layout();

    // Bind events
    m_pauseButton->Bind(wxEVT_BUTTON, &ProgressiveLoadingDialog::onPause, this);
    m_cancelButton->Bind(wxEVT_BUTTON, &ProgressiveLoadingDialog::onCancel, this);
    m_dialog->Bind(wxEVT_CLOSE_WINDOW, &ProgressiveLoadingDialog::onClose, this);
}

ProgressiveLoadingDialog::~ProgressiveLoadingDialog()
{
    if (m_dialog) {
        m_dialog->Destroy();
    }
}

void ProgressiveLoadingDialog::show()
{
    if (m_dialog) {
        m_dialog->Show();
        updateDisplay();
    }
}

void ProgressiveLoadingDialog::hide()
{
    if (m_dialog) {
        m_dialog->Hide();
    }
}

void ProgressiveLoadingDialog::updateProgress()
{
    if (!m_dialog || !m_dialog->IsShown()) return;

    updateDisplay();
}

void ProgressiveLoadingDialog::onPause(wxCommandEvent& event)
{
    if (m_loader) {
        auto state = m_loader->getState();
        if (state == ProgressiveGeometryLoader::LoadingState::Loading) {
            m_loader->pauseLoading();
            m_pauseButton->SetLabel("Resume");
        } else if (state == ProgressiveGeometryLoader::LoadingState::Paused) {
            m_loader->resumeLoading();
            m_pauseButton->SetLabel("Pause");
        }
    }
}

void ProgressiveLoadingDialog::onCancel(wxCommandEvent& event)
{
    if (m_loader) {
        m_loader->cancelLoading();
    }
    hide();
}

void ProgressiveLoadingDialog::onClose(wxCloseEvent& event)
{
    if (m_loader) {
        m_loader->cancelLoading();
    }
    hide();
}

void ProgressiveLoadingDialog::updateDisplay()
{
    if (!m_loader) return;

    double progress = m_loader->getProgress() * 100.0;
    m_progressBar->SetValue(static_cast<int>(progress));

    auto stats = m_loader->getStats();

    // Update status text
    std::stringstream status;
    status << "Loaded " << stats.loadedChunks << "/" << stats.totalChunks
           << " chunks (" << stats.renderedShapes << " shapes)";
    m_statusText->SetLabel(status.str());

    // Update stats text
    std::stringstream statsStr;
    statsStr << "Memory: " << (stats.memoryUsage / (1024*1024)) << " MB | "
             << "Avg load time: " << std::fixed << std::setprecision(2)
             << stats.averageLoadTime << "s";
    m_statsText->SetLabel(statsStr.str());

    m_dialog->Layout();
}

// =====================================================================================
// ProgressiveGeometryReader Implementation
// =====================================================================================

bool ProgressiveGeometryReader::loadGeometry(const std::string& filePath,
                                           std::vector<TopoDS_Shape>& shapes,
                                           ProgressiveGeometryLoader* loader)
{
    if (loader && ProgressiveGeometryLoader::shouldLoadProgressively(filePath)) {
        // Use progressive loading
        auto config = ProgressiveGeometryLoader::getRecommendedConfig(filePath);

        // Set up callbacks
        ProgressiveGeometryLoader::Callbacks callbacks;
        callbacks.onChunkRendered = [&](const ProgressiveGeometryLoader::RenderChunk& chunk) {
            // Add shapes to output vector
            shapes.insert(shapes.end(), chunk.shapes.begin(), chunk.shapes.end());
        };

        return loader->startLoading(config, callbacks);
    } else {
        // Use traditional loading
        // This would integrate with existing geometry readers
        return false; // Placeholder - would implement actual loading
    }
}

bool ProgressiveGeometryReader::isProgressiveLoadingAvailable(const std::string& filePath)
{
    return ProgressiveGeometryLoader::shouldLoadProgressively(filePath) &&
           StreamingFileReader::supportsStreaming(filePath);
}
