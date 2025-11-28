#pragma once

#include "docking/ObjectPool.h"
#include "docking/TitleBarRenderer.h"
#include <vector>
#include <memory>

namespace ads {

// Object pool manager for rendering-related objects
class RenderObjectPool {
public:
    static RenderObjectPool& getInstance();

    // Get pooled vector for TabRenderInfo
    std::unique_ptr<std::vector<TabRenderInfo>> acquireTabRenderInfoVector();
    void releaseTabRenderInfoVector(std::unique_ptr<std::vector<TabRenderInfo>> vec);

    // Get pooled vector for ButtonRenderInfo
    std::unique_ptr<std::vector<ButtonRenderInfo>> acquireButtonRenderInfoVector();
    void releaseButtonRenderInfoVector(std::unique_ptr<std::vector<ButtonRenderInfo>> vec);

    // Clear all pools (useful for cleanup)
    void clear();

private:
    RenderObjectPool();
    ~RenderObjectPool();
    RenderObjectPool(const RenderObjectPool&) = delete;
    RenderObjectPool& operator=(const RenderObjectPool&) = delete;

    // Reset function for TabRenderInfo vector
    static void resetTabRenderInfoVector(std::vector<TabRenderInfo>& vec);

    // Reset function for ButtonRenderInfo vector
    static void resetButtonRenderInfoVector(std::vector<ButtonRenderInfo>& vec);

    ObjectPool<std::vector<TabRenderInfo>> m_tabRenderInfoPool;
    ObjectPool<std::vector<ButtonRenderInfo>> m_buttonRenderInfoPool;
};

} // namespace ads

