#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <queue>

namespace ads {

template<typename T>
class ObjectPool {
public:
    ObjectPool(size_t initialSize = 10, size_t maxSize = 100)
        : m_maxSize(maxSize) {
        for (size_t i = 0; i < initialSize; ++i) {
            m_pool.push(std::make_unique<T>());
        }
    }
    
    std::unique_ptr<T> acquire() {
        if (m_pool.empty()) {
            return std::make_unique<T>();
        }
        
        auto obj = std::move(m_pool.front());
        m_pool.pop();
        return obj;
    }
    
    void release(std::unique_ptr<T> obj) {
        if (m_pool.size() < m_maxSize && obj) {
            resetObject(*obj);
            m_pool.push(std::move(obj));
        }
    }
    
    void setResetFunction(std::function<void(T&)> resetFunc) {
        m_resetFunc = resetFunc;
    }
    
    size_t size() const { return m_pool.size(); }
    size_t maxSize() const { return m_maxSize; }
    
private:
    std::queue<std::unique_ptr<T>> m_pool;
    size_t m_maxSize;
    std::function<void(T&)> m_resetFunc;
    
    void resetObject(T& obj) {
        if (m_resetFunc) {
            m_resetFunc(obj);
        }
    }
};

template<typename T>
class SharedObjectPool {
public:
    SharedObjectPool(size_t initialSize = 10, size_t maxSize = 100)
        : m_maxSize(maxSize) {
        for (size_t i = 0; i < initialSize; ++i) {
            m_pool.push(std::make_shared<T>());
        }
    }
    
    std::shared_ptr<T> acquire() {
        if (m_pool.empty()) {
            return std::make_shared<T>();
        }
        
        auto obj = m_pool.front();
        m_pool.pop();
        return obj;
    }
    
    void release(std::shared_ptr<T> obj) {
        if (m_pool.size() < m_maxSize && obj) {
            resetObject(*obj);
            m_pool.push(obj);
        }
    }
    
    void setResetFunction(std::function<void(T&)> resetFunc) {
        m_resetFunc = resetFunc;
    }
    
    size_t size() const { return m_pool.size(); }
    size_t maxSize() const { return m_maxSize; }
    
private:
    std::queue<std::shared_ptr<T>> m_pool;
    size_t m_maxSize;
    std::function<void(T&)> m_resetFunc;
    
    void resetObject(T& obj) {
        if (m_resetFunc) {
            m_resetFunc(obj);
        }
    }
};

} // namespace ads

