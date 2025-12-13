#pragma once

#include <vector>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "rendering/GeometryProcessor.h"

// Forward declarations
class SoSeparator;

/**
 * @brief VBO-based edge renderer (bypasses Coin3D scene graph)
 * 
 * Direct OpenGL VBO rendering for maximum performance.
 * Similar to FreeCAD's approach: precompute edge buffer and render with GL_LINES.
 * 
 * Performance: ~0.6-1.2ms for 100k triangles (vs 15-200ms with SoIndexedLineSet)
 */
class VBOEdgeRenderer {
public:
    VBOEdgeRenderer();
    ~VBOEdgeRenderer();
    
    /**
     * @brief Initialize VBO resources
     * @return True if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown and cleanup VBO resources
     */
    void shutdown();
    
    /**
     * @brief Create edge buffer from mesh (deduplicated edges)
     * @param mesh Triangle mesh
     * @return True if buffer created successfully
     */
    bool createEdgeBuffer(const TriangleMesh& mesh);
    
    /**
     * @brief Create edge buffer from point pairs
     * @param edgePoints Vector of point pairs (each pair represents an edge)
     * @return True if buffer created successfully
     */
    bool createEdgeBuffer(const std::vector<gp_Pnt>& edgePoints);
    
    /**
     * @brief Render edges using VBO (must be called in OpenGL context)
     * @param color Edge color
     * @param lineWidth Line width (screen space pixels)
     */
    void render(const Quantity_Color& color, float lineWidth = 1.0f);
    
    /**
     * @brief Check if VBO is valid and ready to render
     */
    bool isValid() const { return m_vboValid && m_edgeCount > 0; }
    
    /**
     * @brief Get number of edges in buffer
     */
    size_t getEdgeCount() const { return m_edgeCount; }
    
    /**
     * @brief Clear VBO buffer
     */
    void clear();
    
private:
    unsigned int m_vboId;          // OpenGL VBO ID
    size_t m_edgeCount;             // Number of edges
    bool m_vboValid;                 // VBO is valid and ready
    
    /**
     * @brief Extract unique edges from mesh
     */
    void extractUniqueEdges(const TriangleMesh& mesh, std::vector<float>& vertices);
    
    /**
     * @brief Convert point pairs to vertex array
     */
    void convertPointsToVertices(const std::vector<gp_Pnt>& edgePoints, std::vector<float>& vertices);
};



