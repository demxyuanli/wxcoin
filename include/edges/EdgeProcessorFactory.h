#pragma once

#include <memory>
#include <map>
#include "extractors/BaseEdgeExtractor.h"
#include "renderers/BaseEdgeRenderer.h"
#include "EdgeTypes.h"

/**
 * @brief Edge processor factory
 * 
 * Manages creation and retrieval of edge extractors and renderers
 * for different edge types
 */
class EdgeProcessorFactory {
public:
    static EdgeProcessorFactory& getInstance();
    
    /**
     * @brief Get edge extractor for specific type
     * @param type Edge type
     * @return Shared pointer to extractor
     */
    std::shared_ptr<BaseEdgeExtractor> getExtractor(EdgeType type);
    
    /**
     * @brief Get edge renderer for specific type
     * @param type Edge type
     * @return Shared pointer to renderer
     */
    std::shared_ptr<BaseEdgeRenderer> getRenderer(EdgeType type);
    
    /**
     * @brief Register custom extractor
     * @param type Edge type
     * @param extractor Custom extractor
     */
    void registerExtractor(EdgeType type, std::shared_ptr<BaseEdgeExtractor> extractor);
    
    /**
     * @brief Register custom renderer
     * @param type Edge type
     * @param renderer Custom renderer
     */
    void registerRenderer(EdgeType type, std::shared_ptr<BaseEdgeRenderer> renderer);
    
    /**
     * @brief Get available edge types
     * @return Vector of supported edge types
     */
    std::vector<EdgeType> getAvailableTypes() const;
    
    /**
     * @brief Check if edge type is supported
     * @param type Edge type
     * @return True if supported
     */
    bool isTypeSupported(EdgeType type) const;

private:
    EdgeProcessorFactory();
    ~EdgeProcessorFactory() = default;

    // Disable copy
    EdgeProcessorFactory(const EdgeProcessorFactory&) = delete;
    EdgeProcessorFactory& operator=(const EdgeProcessorFactory&) = delete;

    void initializeDefaultProcessors();

    static EdgeProcessorFactory* m_instance;
    
    std::map<EdgeType, std::shared_ptr<BaseEdgeExtractor>> m_extractors;
    std::map<EdgeType, std::shared_ptr<BaseEdgeRenderer>> m_renderers;
};
