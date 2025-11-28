#include "docking/RenderObjectPool.h"

namespace ads {

RenderObjectPool& RenderObjectPool::getInstance() {
    static RenderObjectPool instance;
    return instance;
}

RenderObjectPool::RenderObjectPool()
    : m_tabRenderInfoPool(5, 20)  // Initial 5, max 20
    , m_buttonRenderInfoPool(5, 20)  // Initial 5, max 20
{
    // Set reset functions
    m_tabRenderInfoPool.setResetFunction(resetTabRenderInfoVector);
    m_buttonRenderInfoPool.setResetFunction(resetButtonRenderInfoVector);
}

RenderObjectPool::~RenderObjectPool() {
}

void RenderObjectPool::resetTabRenderInfoVector(std::vector<TabRenderInfo>& vec) {
    vec.clear();
    vec.shrink_to_fit();  // Release memory
}

void RenderObjectPool::resetButtonRenderInfoVector(std::vector<ButtonRenderInfo>& vec) {
    vec.clear();
    vec.shrink_to_fit();  // Release memory
}

std::unique_ptr<std::vector<TabRenderInfo>> RenderObjectPool::acquireTabRenderInfoVector() {
    return m_tabRenderInfoPool.acquire();
}

void RenderObjectPool::releaseTabRenderInfoVector(std::unique_ptr<std::vector<TabRenderInfo>> vec) {
    m_tabRenderInfoPool.release(std::move(vec));
}

std::unique_ptr<std::vector<ButtonRenderInfo>> RenderObjectPool::acquireButtonRenderInfoVector() {
    return m_buttonRenderInfoPool.acquire();
}

void RenderObjectPool::releaseButtonRenderInfoVector(std::unique_ptr<std::vector<ButtonRenderInfo>> vec) {
    m_buttonRenderInfoPool.release(std::move(vec));
}

void RenderObjectPool::clear() {
    // Pools will be cleared when destroyed
}

} // namespace ads

