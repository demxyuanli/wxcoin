#pragma once

#include <wx/wx.h>
#include <vector>
#include <set>
#include <memory>

namespace ads {

class BatchRefreshManager {
public:
    static BatchRefreshManager& getInstance();
    
    void scheduleRefresh(wxWindow* window, const wxRect* rect = nullptr);
    void scheduleRefreshAll(wxWindow* window);
    
    void flush();
    void flushImmediate();
    
    void beginBatch();
    void endBatch();
    
    bool isBatching() const { return m_batchCount > 0; }
    
    void setMaxBatchSize(size_t maxSize) { m_maxBatchSize = maxSize; }
    size_t getMaxBatchSize() const { return m_maxBatchSize; }
    
    void clear();
    
private:
    BatchRefreshManager() = default;
    ~BatchRefreshManager() = default;
    BatchRefreshManager(const BatchRefreshManager&) = delete;
    BatchRefreshManager& operator=(const BatchRefreshManager&) = delete;
    
    struct RefreshRequest {
        wxWindow* window;
        wxRect rect;
        bool fullRefresh;
        
        RefreshRequest(wxWindow* w, const wxRect& r, bool full) 
            : window(w), rect(r), fullRefresh(full) {}
        
        RefreshRequest(const RefreshRequest& other)
            : window(other.window), rect(other.rect), fullRefresh(other.fullRefresh) {}
        
        RefreshRequest& operator=(const RefreshRequest& other) {
            if (this != &other) {
                window = other.window;
                rect = other.rect;
                fullRefresh = other.fullRefresh;
            }
            return *this;
        }
    };
    
    std::vector<RefreshRequest> m_pendingRefreshes;
    std::set<wxWindow*> m_pendingFullRefreshes;
    int m_batchCount = 0;
    size_t m_maxBatchSize = 100;
    wxTimer* m_flushTimer = nullptr;
    
    void initializeTimer();
    void onFlushTimer(wxTimerEvent& event);
    void mergeRefreshRequests();
};

} // namespace ads

