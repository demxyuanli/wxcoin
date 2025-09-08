#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/gp_Dir.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "OptimizedGeometryCache.h"
#include "config/RenderingConfig.h"

// Forward declarations
class OptimizedGeometryManager;
enum class DisplayMode;

/**
 * @brief Optimized geometry manager with caching and performance monitoring
 */
class OptimizedGeometryManager {
public:
    OptimizedGeometryManager();
    ~OptimizedGeometryManager() = default;

    // Geometry management
    void addGeometry(std::shared_ptr<OCCGeometry> geometry);
    void removeGeometry(const std::string& name);
    void removeGeometry(std::shared_ptr<OCCGeometry> geometry);
    void clearAll();

    // Batch operations
    void addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);
    void removeGeometries(const std::vector<std::string>& names);

    // Geometry lookup
    std::shared_ptr<OCCGeometry> findGeometry(const std::string& name) const;
    std::shared_ptr<OCCGeometry> findGeometryByIndex(size_t index) const;
    std::vector<std::string> getAllGeometryNames() const;
    size_t getGeometryCount() const;

    // Selection management
    void selectGeometry(const std::string& name);
    void deselectGeometry(const std::string& name);
    void selectAll();
    void deselectAll();
    void toggleSelection(const std::string& name);
    bool isSelected(const std::string& name) const;
    std::vector<std::string> getSelectedGeometryNames() const;
    std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometries() const;
    size_t getSelectedCount() const;

    // Visibility management
    void setGeometryVisible(const std::string& name, bool visible);
    void setAllVisible(bool visible);
    void showAll();
    void hideAll();
    bool isVisible(const std::string& name) const;

    // Appearance management
    void setGeometryColor(const std::string& name, const Quantity_Color& color);
    void setGeometryTransparency(const std::string& name, double transparency);
    void setAllColor(const Quantity_Color& color);

    // Display mode management
    void setWireframeMode(bool wireframe);
    void setShadingMode(bool shaded);
    void setShowEdges(bool showEdges);
    void setShowNormals(bool showNormals);
    bool getWireframeMode() const { return m_wireframeMode; }
    bool getShadingMode() const { return m_shadingMode; }
    bool getShowEdges() const { return m_showEdges; }
    bool getShowNormals() const { return m_showNormals; }

    // Mesh settings
    void setMeshDeflection(double deflection, bool remesh = true);
    double getMeshDeflection() const;
    void setAngularDeflection(double angularDeflection, bool remesh = true);
    double getAngularDeflection() const;
    void setRelativeDeflection(bool relative, bool remesh = true);
    bool getRelativeDeflection() const { return m_meshParams.relative; }
    void remeshAllGeometries();

    // LOD management
    void setLODEnabled(bool enabled);
    void setLODRoughMode(bool roughMode);
    void setLODRoughDeflection(double deflection);
    void setLODFineDeflection(double deflection);
    void setLODTransitionTime(int milliseconds);
    bool isLODEnabled() const;
    bool isLODRoughMode() const;
    bool getLODEnabled() const { return m_lodEnabled; }
    bool getLODRoughMode() const { return m_lodRoughMode; }
    double getLODRoughDeflection() const;
    double getLODFineDeflection() const;
    int getLODTransitionTime() const;
    void setLODMode(bool roughMode);
    void startLODInteraction();
    void endLODInteraction();

    // Anti-aliasing
    void setAntiAliasing(bool enabled);
    bool getAntiAliasing() const { return m_antiAliasing; }

    // View operations
    void fitAll();
    void fitGeometry(const std::string& name);

    // Cache management
    void clearCache();
    void cleanupCache();
    std::string getCacheStats() const;

    // Performance monitoring
    std::string getPerformanceStats() const;

    // Additional methods
    std::vector<std::shared_ptr<OCCGeometry>> findGeometries(const std::vector<std::string>& names);
    void pickGeometry(const std::string& name);
    void pickGeometries(const std::vector<std::string>& names);

    // Thread safety
    void lockForWrite();
    void unlockForWrite();
    void lockForRead() const;
    void unlockForRead() const;

private:
    // Thread-safe data structures
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<OCCGeometry>> m_geometryMap;
    std::vector<std::shared_ptr<OCCGeometry>> m_geometryList;
    std::unordered_set<std::string> m_selectedGeometries;

    // Display settings
    bool m_wireframeMode;
    bool m_shadingMode;
    bool m_showEdges;
    bool m_showNormals;
    bool m_antiAliasing;
    bool m_lodEnabled;
    bool m_lodRoughMode;
    double m_lodRoughDeflection;
    double m_lodFineDeflection;
    int m_lodTransitionTime;

    // Mesh parameters
    OCCMeshConverter::MeshParameters m_meshParams;

    // Geometry cache
    std::unique_ptr<OptimizedGeometryCache> m_geometryCache;

    // Performance tracking
    mutable std::atomic<uint64_t> m_lookupCount{0};
    mutable std::atomic<uint64_t> m_cacheHits{0};
    mutable std::atomic<uint64_t> m_cacheMisses{0};

    // Internal helper methods
    void updateGeometryList();
    void updateSelectionMap();
    void updateStats(bool cacheHit) const;
    bool isValidGeometry(const std::shared_ptr<OCCGeometry>& geometry) const;
    void applyDisplayModeToGeometry(std::shared_ptr<OCCGeometry> geometry);
    void applyMeshSettingsToGeometry(std::shared_ptr<OCCGeometry> geometry);
    void applyLODSettingsToGeometry(std::shared_ptr<OCCGeometry> geometry);
};

/**
 * @brief Thread-safe geometry iterator
 */
class GeometryIterator {
public:
    GeometryIterator(const OptimizedGeometryManager& manager);
    ~GeometryIterator() = default;

    bool hasNext() const;
    std::shared_ptr<OCCGeometry> next();
    void reset();
    size_t getCurrentIndex() const;
    std::vector<std::shared_ptr<OCCGeometry>> getNextBatch(size_t batchSize);

private:
    const OptimizedGeometryManager& m_manager;
    std::vector<std::string> m_geometries;
    size_t m_currentIndex;
    mutable std::shared_mutex m_iteratorMutex;
};

/**
 * @brief Geometry search engine with indexing and pattern matching
 */
class GeometrySearchEngine {
public:
    GeometrySearchEngine(const OptimizedGeometryManager& manager);
    ~GeometrySearchEngine() = default;

    // Basic search
    std::shared_ptr<OCCGeometry> findExact(const std::string& name) const;
    std::vector<std::shared_ptr<OCCGeometry>> findPattern(const std::string& pattern) const;
    std::vector<std::string> searchByName(const std::string& pattern);
    std::vector<std::string> searchByType(const std::string& type);

    // Advanced search
    std::vector<std::shared_ptr<OCCGeometry>> findByType(const std::string& type) const;
    std::vector<std::shared_ptr<OCCGeometry>> findByPropertyRange(
        const std::string& property, double minValue, double maxValue) const;
    std::vector<std::shared_ptr<OCCGeometry>> findByBoundingBox(
        const gp_Pnt& minPoint, const gp_Pnt& maxPoint) const;
    std::vector<std::shared_ptr<OCCGeometry>> findByDistance(
        const gp_Pnt& center, double maxDistance) const;
    std::vector<std::string> searchByBoundingBox(const gp_Pnt& minPoint, const gp_Pnt& maxPoint);
    std::vector<std::string> searchByDistance(const gp_Pnt& center, double maxDistance);
    std::vector<std::shared_ptr<OCCGeometry>> advancedSearch(
        const std::unordered_map<std::string, std::string>& criteria);

    // Index management
    void buildIndices();

private:
    const OptimizedGeometryManager& m_manager;

    // Internal helper methods
    bool matchesPattern(const std::string& text, const std::string& pattern) const;
    double calculateDistance(const gp_Pnt& p1, const gp_Pnt& p2) const;
    bool isInBoundingBox(const std::shared_ptr<OCCGeometry>& geometry,
                        const gp_Pnt& minPoint, const gp_Pnt& maxPoint) const;
};

/**
 * @brief Batch processor for geometry operations
 */
class GeometryBatchProcessor {
public:
    GeometryBatchProcessor(OptimizedGeometryManager& manager);
    ~GeometryBatchProcessor() = default;

    // Batch appearance operations
    void batchSetColor(const std::vector<std::string>& names, const Quantity_Color& color);
    void batchSetTransparency(const std::vector<std::string>& names, double transparency);
    void batchSetVisible(const std::vector<std::string>& names, bool visible);
    void batchSetSelected(const std::vector<std::string>& names, bool selected);

    // Batch transformation operations
    void batchTranslate(const std::vector<std::string>& names, const gp_Vec& translation);
    void batchRotate(const std::vector<std::string>& names, const gp_Pnt& center,
                    const gp_Dir& axis, double angle);
    void batchScale(const std::vector<std::string>& names, const gp_Pnt& center, double factor);

    // Batch mesh operations
    void batchRemesh(const std::vector<std::string>& names,
                    const OCCMeshConverter::MeshParameters& params);
    void batchUpdateLOD(const std::vector<std::string>& names, bool roughMode);

    // Batch export operations
    void batchExportToSTL(const std::vector<std::string>& names, const std::string& directory);
    void batchExportToOBJ(const std::vector<std::string>& names, const std::string& directory);

    // Progress and error handling
    void setProgressCallback(std::function<void(size_t current, size_t total)> callback);
    std::string getLastError() const;

private:
    OptimizedGeometryManager& m_manager;
    std::function<void(size_t current, size_t total)> m_progressCallback;
    std::string m_lastError;

    // Internal helper methods
    void updateProgress(size_t current, size_t total);
    void setError(const std::string& error);
    bool validateGeometryNames(const std::vector<std::string>& names) const;
};
