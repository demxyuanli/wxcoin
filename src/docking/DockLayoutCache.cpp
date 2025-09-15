#include "docking/DockLayoutCache.h"
#include <wx/splitter.h>
#include "logger/Logger.h"

namespace ads {

DockLayoutCache& DockLayoutCache::getInstance() {
    static DockLayoutCache instance;
    return instance;
}

void DockLayoutCache::cacheCurrentLayout(const wxString& key, wxWindow* container) {
    if (!container) return;
    
    LayoutSnapshot snapshot;
    snapshot.container_size = container->GetSize();
    
    // Collect all splitters and their positions
    collectSplitters(container, snapshot);
    
    // Store in cache
    m_cache[key] = snapshot;
    
    LOG_DBG_S("Cached layout '" + key.ToStdString() + "' with " + 
              std::to_string(snapshot.splitters.size()) + " splitters");
}

bool DockLayoutCache::applyCachedLayout(const wxString& key, wxWindow* container, const wxSize& newSize) {
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return false;
    }
    
    const LayoutSnapshot& snapshot = it->second;
    if (!snapshot.isValid()) {
        return false;
    }
    
    // Apply cached ratios to new size
    applySplitterRatios(container, snapshot, newSize);
    
    LOG_DBG_S("Applied cached layout '" + key.ToStdString() + "'");
    return true;
}

void DockLayoutCache::collectSplitters(wxWindow* window, LayoutSnapshot& snapshot) {
    if (!window) return;
    
    // Check if this is a splitter
    wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(window);
    if (splitter && splitter->IsSplit()) {
        LayoutSnapshot::SplitterInfo info;
        info.splitter = splitter;
        info.is_vertical = (splitter->GetSplitMode() == wxSPLIT_VERTICAL);
        
        // Calculate position ratio
        int pos = splitter->GetSashPosition();
        int total = info.is_vertical ? splitter->GetSize().x : splitter->GetSize().y;
        if (total > 0) {
            info.position_ratio = double(pos) / double(total);
        } else {
            info.position_ratio = 0.5;
        }
        
        snapshot.splitters.push_back(info);
    }
    
    // Recursively collect from children
    wxWindowList& children = window->GetChildren();
    for (auto* child : children) {
        collectSplitters(child, snapshot);
    }
}

void DockLayoutCache::applySplitterRatios(wxWindow* window, const LayoutSnapshot& snapshot, const wxSize& newSize) {
    if (!window) return;
    
    // Calculate scale factors
    double scaleX = double(newSize.x) / double(snapshot.container_size.x);
    double scaleY = double(newSize.y) / double(snapshot.container_size.y);
    
    // Apply to all splitters
    for (const auto& info : snapshot.splitters) {
        wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(info.splitter);
        if (splitter && splitter->IsSplit()) {
            int total = info.is_vertical ? splitter->GetSize().x : splitter->GetSize().y;
            int newPos = int(total * info.position_ratio);
            
            // Only update if position changed significantly
            int currentPos = splitter->GetSashPosition();
            if (std::abs(currentPos - newPos) > 2) {
                splitter->SetSashPosition(newPos);
            }
        }
    }
    
    // Process children
    wxWindowList& children = window->GetChildren();
    for (auto* child : children) {
        applySplitterRatios(child, snapshot, newSize);
    }
}

} // namespace ads