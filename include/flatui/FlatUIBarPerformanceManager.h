#ifndef FLATUIBARPERFORMANCEMANAGER_H
#define FLATUIBARPERFORMANCEMANAGER_H

#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <memory>
#include <unordered_map>

class FlatUIBar;

// DPI-aware resource cache
struct DPIAwareResource {
    double scaleFactor;
    wxBitmap bitmap;
    wxFont font;
    int intValue;
    
    DPIAwareResource() : scaleFactor(1.0), intValue(0) {}
    DPIAwareResource(double sf, const wxBitmap& bmp) : scaleFactor(sf), bitmap(bmp), intValue(0) {}
    DPIAwareResource(double sf, const wxFont& f) : scaleFactor(sf), font(f), intValue(0) {}
    DPIAwareResource(double sf, int val) : scaleFactor(sf), intValue(val) {}
};

// Performance optimization flags
enum class PerformanceOptimization {
    NONE = 0,
    HARDWARE_ACCELERATION = 1 << 0,
    DIRTY_REGION_TRACKING = 1 << 1,
    RESOURCE_CACHING = 1 << 2,
    BATCH_PAINTING = 1 << 3,
    DPI_OPTIMIZATION = 1 << 4,
    ALL = HARDWARE_ACCELERATION | DIRTY_REGION_TRACKING | RESOURCE_CACHING | BATCH_PAINTING | DPI_OPTIMIZATION
};

inline PerformanceOptimization operator|(PerformanceOptimization a, PerformanceOptimization b) {
    return static_cast<PerformanceOptimization>(static_cast<int>(a) | static_cast<int>(b));
}

inline PerformanceOptimization operator&(PerformanceOptimization a, PerformanceOptimization b) {
    return static_cast<PerformanceOptimization>(static_cast<int>(a) & static_cast<int>(b));
}

class FlatUIBarPerformanceManager
{
public:
    FlatUIBarPerformanceManager(FlatUIBar* bar);
    ~FlatUIBarPerformanceManager();

    // DPI Management
    double GetCurrentDPIScale() const;
    void OnDPIChanged();
    wxSize FromDIP(const wxSize& size) const;
    wxPoint FromDIP(const wxPoint& point) const;
    int FromDIP(int value) const;
    wxFont GetDPIAwareFont(const wxString& fontKey) const;
    
    // Resource caching for DPI-aware resources
    wxBitmap GetDPIAwareBitmap(const wxString& key, const wxBitmap& originalBitmap);
    wxFont GetCachedFont() const;
    wxFont GetCachedFont(const wxString& key, const wxFont& originalFont) const;
    int GetDPIAwareValue(const wxString& key, int originalValue);
    void ClearResourceCache();
    
    // Hardware acceleration support
    void EnableHardwareAcceleration(bool enable = true);
    bool IsHardwareAccelerationEnabled() const;
    wxGraphicsContext* CreateOptimizedGraphicsContext(wxDC& dc);
    
    // Dirty region management
    void InvalidateRegion(const wxRect& region);
    void InvalidateAll();
    bool HasInvalidRegions() const;
    std::vector<wxRect> GetInvalidRegions() const;
    void ClearInvalidRegions();
    
    // Batch painting optimization
    void BeginBatchPaint();
    void EndBatchPaint();
    bool IsBatchPainting() const;
    void QueuePaintOperation(std::function<void(wxGraphicsContext*)> operation);
    
    // Performance monitoring
    void StartPerformanceTimer(const wxString& operation);
    void EndPerformanceTimer(const wxString& operation);
    void LogPerformanceStats() const;
    
    // Optimization control
    void SetOptimizationFlags(PerformanceOptimization flags);
    PerformanceOptimization GetOptimizationFlags() const;
    
    // Memory optimization
    void OptimizeMemoryUsage();
    void PreloadResources();
    
    // Platform-specific optimizations
#ifdef __WXMSW__
    void EnableWindowsComposition();
    void SetLayeredWindowAttributes();
#endif

private:
    FlatUIBar* m_bar;
    double m_currentDPIScale;
    bool m_hardwareAcceleration;
    bool m_batchPainting;
    PerformanceOptimization m_optimizationFlags;
    
    // Resource caches
    mutable std::unordered_map<wxString, DPIAwareResource> m_bitmapCache;
    mutable std::unordered_map<wxString, DPIAwareResource> m_fontCache;
    mutable std::unordered_map<wxString, DPIAwareResource> m_valueCache;
    
    // Dirty region tracking
    std::vector<wxRect> m_invalidRegions;
    bool m_hasInvalidRegions;
    
    // Batch operations
    std::vector<std::function<void(wxGraphicsContext*)>> m_queuedOperations;
    
    // Performance monitoring
    mutable std::unordered_map<wxString, wxLongLong> m_performanceTimers;
    mutable std::unordered_map<wxString, std::vector<double>> m_performanceStats;
    
    // Helper methods
    void UpdateDPIScale();
    wxString GenerateCacheKey(const wxString& baseKey, double scaleFactor) const;
    void CleanupExpiredCacheEntries();
    bool IsOptimizationEnabled(PerformanceOptimization opt) const;
};

#endif // FLATUIBARPERFORMANCEMANAGER_H 