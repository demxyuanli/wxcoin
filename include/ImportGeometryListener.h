#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include "GeometryReader.h"
#include "GeometryImportOptimizer.h"
#include "ImportStatisticsDialog.h"
#include "ProgressiveGeometryLoader.h"
#include <memory>
#include <wx/frame.h>
#include <chrono>

class Canvas;
class OCCViewer;
class FlatUIStatusBar;

/**
 * @brief Unified geometry import listener supporting multiple formats
 *
 * Supports importing STEP, IGES, OBJ, STL and other geometry formats
 * with a unified interface and progress reporting
 */
class ImportGeometryListener : public CommandListener {
public:
    ImportGeometryListener(wxFrame* frame, Canvas* canvas, OCCViewer* occViewer);
    ~ImportGeometryListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ImportGeometryListener"; }

private:
    wxFrame* m_frame;
    Canvas* m_canvas;
    OCCViewer* m_occViewer;
    FlatUIStatusBar* m_statusBar;
    GeometryReader::DecompositionOptions m_decompositionOptions;

    /**
     * @brief Import files using the specified reader
     * @param reader Geometry reader to use
     * @param filePaths Vector of file paths to import
     * @param options Import options
     * @return Command result
     */
    CommandResult importFiles(std::unique_ptr<GeometryReader> reader,
        const std::vector<std::string>& filePaths,
        const GeometryReader::OptimizationOptions& options);

    /**
     * @brief Import files with detailed statistics collection
     * @param reader Geometry reader to use
     * @param filePaths Vector of file paths to import
     * @param options Enhanced import options
     * @param overallStats Statistics to update
     * @param formatName Format name for statistics
     * @param allGeometries Output vector to accumulate geometries
     * @return Command result
     */
    CommandResult importFilesWithStats(std::unique_ptr<GeometryReader> reader,
        const std::vector<std::string>& filePaths,
        const GeometryImportOptimizer::EnhancedOptions& options,
        ImportOverallStatistics& overallStats,
        const std::string& formatName,
        std::vector<std::shared_ptr<OCCGeometry>>& allGeometries);

    /**
     * @brief Setup balanced default import options
     * @param options Output options with balanced settings
     */
    void setupBalancedImportOptions(GeometryReader::OptimizationOptions& options);
    
    /**
     * @brief Setup balanced default import options for enhanced options
     * @param options Output enhanced options with balanced settings
     */
    void setupBalancedImportOptions(GeometryImportOptimizer::EnhancedOptions& options);

    /**
     * @brief Update progress in status bar and message panel
     * @param percent Progress percentage (0-100)
     * @param message Progress message
     * @param flatFrame Frame for message output
     */
    void updateProgress(int percent, const std::string& message, void* flatFrame);

    /**
     * @brief Clean up progress display
     */
    void cleanupProgress();

    /**
     * @brief Check if file should use progressive loading
     * @param filePath Path to file
     * @param fileSize File size in bytes
     * @return True if progressive loading should be used
     */
    bool shouldUseProgressiveLoading(const std::string& filePath, size_t fileSize);

    /**
     * @brief Import large file using progressive loading
     * @param filePath Path to large file
     * @param options Import options
     * @param allGeometries Output vector to accumulate geometries
     * @return True if import successful
     */
    bool importWithProgressiveLoading(const std::string& filePath,
        const GeometryReader::OptimizationOptions& options,
        std::vector<std::shared_ptr<OCCGeometry>>& allGeometries);
};
