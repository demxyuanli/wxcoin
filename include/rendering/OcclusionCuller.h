#pragma once

#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <Inventor/nodes/SoSeparator.h>
#include <vector>
#include <memory>
#include <map>

// Forward declarations
class SoCamera;
class FrustumCuller;

/**
 * @brief Occlusion culling system for rendering optimization
 * 
 * Implements occlusion culling to skip rendering of hidden objects
 */
class OcclusionCuller {
public:
    OcclusionCuller();
    virtual ~OcclusionCuller();

    /**
     * @brief Occluder object with bounding box and depth information
     */
    struct Occluder {
        TopoDS_Shape shape;
        Bnd_Box bbox;
        gp_Pnt center;
        double radius;
        float minDepth;
        float maxDepth;
        bool isVisible;
        
        Occluder() : radius(0), minDepth(0), maxDepth(0), isVisible(true) {}
        
        // Update occluder from shape
        void updateFromShape(const TopoDS_Shape& shape);
        
        // Check if this occluder can potentially occlude another object
        bool canOcclude(const Bnd_Box& targetBbox) const;
        
        // Check if this occluder is closer than target
        bool isCloserThan(const gp_Pnt& targetCenter, float targetDepth) const;
    };

    /**
     * @brief Occlusion query result
     */
    struct OcclusionQuery {
        unsigned int queryId;
        Bnd_Box bbox;
        bool isOccluded;
        float depth;
        
        OcclusionQuery() : queryId(0), isOccluded(false), depth(0.0f) {}
    };

    /**
     * @brief Update occlusion culling from camera
     * @param camera Current camera
     * @param frustumCuller Frustum culler for pre-filtering
     */
    void updateOcclusion(const SoCamera* camera, const FrustumCuller* frustumCuller);

    /**
     * @brief Add occluder to the scene
     * @param shape Shape to add as occluder
     * @param sceneNode Coin3D scene node
     */
    void addOccluder(const TopoDS_Shape& shape, SoSeparator* sceneNode);

    /**
     * @brief Remove occluder from the scene
     * @param shape Shape to remove
     */
    void removeOccluder(const TopoDS_Shape& shape);

    /**
     * @brief Check if shape is occluded by other objects
     * @param shape Shape to test
     * @return true if visible, false if occluded
     */
    bool isShapeVisible(const TopoDS_Shape& shape);

    /**
     * @brief Check if bounding box is occluded
     * @param bbox Bounding box to test
     * @param center Center point of the object
     * @return true if visible, false if occluded
     */
    bool isBoundingBoxVisible(const Bnd_Box& bbox, const gp_Pnt& center);

    /**
     * @brief Perform occlusion query for a bounding box
     * @param bbox Bounding box to query
     * @return Query result
     */
    OcclusionQuery performOcclusionQuery(const Bnd_Box& bbox);

    /**
     * @brief Enable/disable occlusion culling
     * @param enabled Culling state
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief Check if occlusion culling is enabled
     * @return true if enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Set maximum number of occluders to consider
     * @param maxOccluders Maximum number
     */
    void setMaxOccluders(int maxOccluders) { m_maxOccluders = maxOccluders; }

    /**
     * @brief Get maximum number of occluders
     * @return Maximum number
     */
    int getMaxOccluders() const { return m_maxOccluders; }

    /**
     * @brief Get occlusion statistics
     * @return Number of objects occluded in last frame
     */
    int getOccludedCount() const { return m_occludedCount; }

    /**
     * @brief Reset occlusion statistics
     */
    void resetStats() { m_occludedCount = 0; }

    /**
     * @brief Clear all occluders
     */
    void clearOccluders();

    /**
     * @brief Get number of active occluders
     * @return Number of occluders
     */
    size_t getOccluderCount() const { return m_occluders.size(); }

private:
    std::vector<Occluder> m_occluders;
    std::map<const TopoDS_Shape*, size_t> m_occluderMap; // Shape to index mapping
    bool m_enabled;
    int m_maxOccluders;
    int m_occludedCount;
    unsigned int m_nextQueryId;
    
    // Helper methods
    void updateOccluderDepths(const SoCamera* camera);
    void sortOccludersByDepth();
    bool isBboxOccludedByOccluder(const Bnd_Box& bbox, const Occluder& occluder) const;
    float calculateDepth(const gp_Pnt& point, const SoCamera* camera) const;
    void cullDistantOccluders();
}; 