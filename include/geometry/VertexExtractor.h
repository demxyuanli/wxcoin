#pragma once

#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <Inventor/nodes/SoSeparator.h>
#include <vector>
#include <mutex>

/**
 * @brief Independent vertex extractor and cache for point view rendering
 * 
 * Extracts vertices from OpenCASCADE shapes at import time and caches them
 * for fast point rendering without async threading or GL context issues.
 */
class VertexExtractor {
public:
    VertexExtractor();
    ~VertexExtractor();

    /**
     * @brief Extract and cache all unique vertices from a shape
     * @param shape OpenCASCADE shape to extract vertices from
     * @return Number of vertices extracted
     */
    size_t extractAndCache(const TopoDS_Shape& shape);

    /**
     * @brief Create a Coin3D point node from cached vertices
     * @param color Point color
     * @param pointSize Point size in pixels
     * @return SoSeparator node containing the point set (caller must ref/unref)
     */
    SoSeparator* createPointNode(const Quantity_Color& color, double pointSize) const;

    /**
     * @brief Check if vertices are cached
     */
    bool hasCache() const;

    /**
     * @brief Get number of cached vertices
     */
    size_t getCachedCount() const;

    /**
     * @brief Clear cached vertex data
     */
    void clearCache();

    /**
     * @brief Get direct access to cached vertices (const)
     */
    const std::vector<gp_Pnt>& getCachedVertices() const { return m_cachedVertices; }

private:
    std::vector<gp_Pnt> m_cachedVertices;  // Cached vertex positions
    bool m_cacheValid;                      // Cache validity flag
    mutable std::mutex m_mutex;             // Thread safety

    /**
     * @brief Helper to check if a vertex is already in cache (with tolerance)
     */
    bool isDuplicate(const gp_Pnt& point, double tolerance = 1e-6) const;
};



