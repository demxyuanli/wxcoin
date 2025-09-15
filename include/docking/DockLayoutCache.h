#pragma once

#include <wx/wx.h>
#include <unordered_map>
#include <memory>

namespace ads {

/**
 * Layout cache system to avoid recalculating layouts during resize
 */
class DockLayoutCache {
public:
    struct LayoutSnapshot {
        struct SplitterInfo {
            wxWindow* splitter;
            double position_ratio;  // 0.0 to 1.0
            bool is_vertical;
        };
        
        std::vector<SplitterInfo> splitters;
        wxSize container_size;
        
        bool isValid() const { return !splitters.empty(); }
    };
    
    static DockLayoutCache& getInstance();
    
    // Cache current layout
    void cacheCurrentLayout(const wxString& key, wxWindow* container);
    
    // Apply cached layout
    bool applyCachedLayout(const wxString& key, wxWindow* container, const wxSize& newSize);
    
    // Clear cache
    void clearCache() { m_cache.clear(); }
    
private:
    DockLayoutCache() = default;
    
    std::unordered_map<wxString, LayoutSnapshot> m_cache;
    
    void collectSplitters(wxWindow* window, LayoutSnapshot& snapshot);
    void applySplitterRatios(wxWindow* window, const LayoutSnapshot& snapshot, const wxSize& newSize);
};

} // namespace ads
