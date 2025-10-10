#pragma once

#include <vector>
#include <memory>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "geometry/BVHAccelerator.h"
#include "logger/Logger.h"

/**
 * @brief Selection accelerator using BVH for fast CAD model picking
 *
 * Provides accelerated ray casting and point-in-shape testing for interactive
 * selection in CAD models. Uses BVH (Bounding Volume Hierarchy) for O(log n)
 * intersection queries.
 */
class SelectionAccelerator {
public:
    /**
     * @brief Selection modes
     */
    enum class SelectionMode {
        Shapes,      // Select individual shapes
        Faces,       // Select individual faces
        Edges,       // Select individual edges
        Vertices     // Select individual vertices
    };

    /**
     * @brief Selection result
     */
    struct SelectionResult {
        bool found = false;
        TopoDS_Shape selectedShape;
        size_t shapeIndex = SIZE_MAX;
        gp_Pnt intersectionPoint;
        double distance = std::numeric_limits<double>::max();
    };

    SelectionAccelerator();
    ~SelectionAccelerator();

    /**
     * @brief Build acceleration structures for selection
     * @param shapes Vector of shapes to accelerate
     * @param mode Selection granularity mode
     * @return True if build successful
     */
    bool build(const std::vector<TopoDS_Shape>& shapes, SelectionMode mode = SelectionMode::Shapes);

    /**
     * @brief Perform ray casting selection
     * @param rayOrigin Ray origin in world space
     * @param rayDirection Ray direction (should be normalized)
     * @param result Selection result (output)
     * @return True if something was selected
     */
    bool selectByRay(const gp_Pnt& rayOrigin, const gp_Vec& rayDirection, SelectionResult& result);

    /**
     * @brief Perform point-based selection (find shape containing point)
     * @param point Test point in world space
     * @param result Selection result (output)
     * @return True if something was selected
     */
    bool selectByPoint(const gp_Pnt& point, SelectionResult& result);

    /**
     * @brief Perform rectangle selection (find all shapes in rectangle)
     * @param rectMin Rectangle minimum corner in screen space
     * @param rectMax Rectangle maximum corner in screen space
     * @param viewMatrix Current view matrix for screen-to-world conversion
     * @param projectionMatrix Current projection matrix
     * @param viewport Viewport dimensions [x, y, width, height]
     * @param results Vector to store selection results
     * @return Number of selected items
     */
    size_t selectByRectangle(const gp_Pnt& rectMin, const gp_Pnt& rectMax,
                           const std::vector<double>& viewMatrix,
                           const std::vector<double>& projectionMatrix,
                           const std::vector<int>& viewport,
                           std::vector<SelectionResult>& results);

    /**
     * @brief Update selection mode
     * @param mode New selection mode
     * @return True if mode changed and rebuild needed
     */
    bool setSelectionMode(SelectionMode mode);

    /**
     * @brief Get current selection mode
     * @return Current selection mode
     */
    SelectionMode getSelectionMode() const { return m_selectionMode; }

    /**
     * @brief Check if accelerator is ready
     * @return True if built and ready for selection
     */
    bool isReady() const { return m_bvh && m_bvh->isBuilt(); }

    /**
     * @brief Get performance statistics
     * @return String containing performance stats
     */
    std::string getPerformanceStats() const;

    /**
     * @brief Clear acceleration structures
     */
    void clear();

private:
    SelectionMode m_selectionMode;
    std::unique_ptr<BVHAccelerator> m_bvh;
    std::vector<TopoDS_Shape> m_shapes;

    // Performance tracking
    size_t m_rayTestsPerformed = 0;
    size_t m_pointTestsPerformed = 0;
    size_t m_selectionsFound = 0;

    // Helper methods
    void buildForShapes(const std::vector<TopoDS_Shape>& shapes);
    void buildForFaces(const std::vector<TopoDS_Shape>& shapes);
    void buildForEdges(const std::vector<TopoDS_Shape>& shapes);
    void buildForVertices(const std::vector<TopoDS_Shape>& shapes);

    std::vector<TopoDS_Shape> extractFaces(const TopoDS_Shape& shape);
    std::vector<TopoDS_Shape> extractEdges(const TopoDS_Shape& shape);
    std::vector<TopoDS_Shape> extractVertices(const TopoDS_Shape& shape);

    // Screen-to-world conversion helpers
    gp_Pnt screenToWorld(double screenX, double screenY, double depth,
                        const std::vector<double>& viewMatrix,
                        const std::vector<double>& projectionMatrix,
                        const std::vector<int>& viewport);

    bool pointInRectangle(const gp_Pnt& point, const gp_Pnt& rectMin, const gp_Pnt& rectMax);
};

// Utility functions for selection
gp_Vec normalizeVector(const gp_Vec& vec);
double vectorMagnitude(const gp_Vec& vec);
gp_Vec crossProduct(const gp_Vec& a, const gp_Vec& b);

