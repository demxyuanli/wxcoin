#pragma once

#include <memory>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <Inventor/SbVec3f.h>

class OCCGeometry;
class SelectionAccelerator;

/**
 * @brief Selection acceleration service using BVH (Bounding Volume Hierarchy)
 * 
 * Provides fast geometry picking through spatial acceleration structures.
 * Manages SelectionAccelerator lifecycle and provides high-level picking APIs.
 */
class SelectionAcceleratorService {
public:
    SelectionAcceleratorService();
    ~SelectionAcceleratorService();

    /**
     * @brief Rebuild accelerator from current geometries
     * @param geometries Vector of all geometries in the scene
     */
    void rebuildFromGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);

    /**
     * @brief Pick geometry using ray casting with acceleration
     * @param origin Ray origin point
     * @param direction Normalized ray direction
     * @param geometries Vector of all geometries (for index mapping)
     * @return Selected geometry or nullptr if nothing picked
     */
    std::shared_ptr<OCCGeometry> pickByRay(
        const gp_Pnt& origin,
        const gp_Vec& direction,
        const std::vector<std::shared_ptr<OCCGeometry>>& geometries);

    /**
     * @brief Pick geometry using distance-based fallback method
     * @param worldPos World position to pick from
     * @param geometries Vector of all geometries
     * @param pickingRadius Maximum distance for picking
     * @return Closest geometry within radius or nullptr
     */
    std::shared_ptr<OCCGeometry> pickByDistance(
        const SbVec3f& worldPos,
        const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        double pickingRadius = 15.0);

    /**
     * @brief Clear accelerator data
     */
    void clear();

    /**
     * @brief Check if accelerator is ready for use
     * @return True if accelerator has been built
     */
    bool isReady() const;

    /**
     * @brief Get number of shapes in accelerator
     * @return Shape count
     */
    size_t getShapeCount() const;

private:
    std::unique_ptr<SelectionAccelerator> m_accelerator;
    size_t m_shapeCount = 0;
};

