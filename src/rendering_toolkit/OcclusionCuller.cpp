#include "rendering/OcclusionCuller.h"
#include "rendering/FrustumCuller.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec3f.h>
#include <OpenCASCADE/BRepBndLib.hxx>
#include <algorithm>
#include <cmath>

OcclusionCuller::OcclusionCuller() 
    : m_enabled(true)
    , m_maxOccluders(50)
    , m_occludedCount(0)
    , m_nextQueryId(1) {
    LOG_INF_S("OcclusionCuller created");
}

OcclusionCuller::~OcclusionCuller() {
    LOG_INF_S("OcclusionCuller destroyed");
}

void OcclusionCuller::Occluder::updateFromShape(const TopoDS_Shape& shape) {
    if (shape.IsNull()) {
        return;
    }
    
    this->shape = shape;
    
    // Compute bounding box
    BRepBndLib::Add(shape, bbox);
    
    if (!bbox.IsVoid()) {
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        // Calculate center
        center = gp_Pnt((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
        
        // Calculate radius
        double dx = xmax - xmin;
        double dy = ymax - ymin;
        double dz = zmax - zmin;
        radius = std::sqrt(dx * dx + dy * dy + dz * dz) / 2.0;
    }
}

bool OcclusionCuller::Occluder::canOcclude(const Bnd_Box& targetBbox) const {
    if (bbox.IsVoid() || targetBbox.IsVoid()) {
        return false;
    }
    
    // Check if bounding boxes overlap in screen space
    double xmin1, ymin1, zmin1, xmax1, ymax1, zmax1;
    double xmin2, ymin2, zmin2, xmax2, ymax2, zmax2;
    
    bbox.Get(xmin1, ymin1, zmin1, xmax1, ymax1, zmax1);
    targetBbox.Get(xmin2, ymin2, zmin2, xmax2, ymax2, zmax2);
    
    // Check overlap in all dimensions
    if (xmax1 < xmin2 || xmin1 > xmax2) return false;
    if (ymax1 < ymin2 || ymin1 > ymax2) return false;
    if (zmax1 < zmin2 || zmin1 > zmax2) return false;
    
    return true;
}

bool OcclusionCuller::Occluder::isCloserThan(const gp_Pnt& targetCenter, float targetDepth) const {
    return minDepth < targetDepth;
}

void OcclusionCuller::updateOcclusion(const SoCamera* camera, const FrustumCuller* frustumCuller) {
    if (!m_enabled || !camera) {
        return;
    }
    
    // Update depths for all occluders
    updateOccluderDepths(camera);
    
    // Sort occluders by depth (closest first)
    sortOccludersByDepth();
    
    // Cull distant occluders to maintain performance
    cullDistantOccluders();
    
    // Pre-filter occluders using frustum culling if available
    if (frustumCuller && frustumCuller->isEnabled()) {
        for (auto& occluder : m_occluders) {
            occluder.isVisible = frustumCuller->isShapeVisible(occluder.shape); 
        }
    }
}

void OcclusionCuller::addOccluder(const TopoDS_Shape& shape, SoSeparator* sceneNode) {
    if (shape.IsNull()) {
        return;
    }
    
    // Check if occluder already exists
    auto it = m_occluderMap.find(&shape);
    if (it != m_occluderMap.end()) {
        return; // Already exists
    }
    
    // Create new occluder
    Occluder occluder;
    occluder.updateFromShape(shape);
    
    // Add to list
    m_occluders.push_back(occluder);
    m_occluderMap[&shape] = m_occluders.size() - 1;
    
    LOG_INF_S("Added occluder, total: " + std::to_string(m_occluders.size()));
}

void OcclusionCuller::removeOccluder(const TopoDS_Shape& shape) {
    auto it = m_occluderMap.find(&shape);
    if (it == m_occluderMap.end()) {
        return;
    }
    
    size_t index = it->second;
    
    // Remove from map
    m_occluderMap.erase(it);
    
    // Remove from vector
    if (index < m_occluders.size()) {
        m_occluders.erase(m_occluders.begin() + index);
        
        // Update indices in map
        for (auto& pair : m_occluderMap) {
            if (pair.second > index) {
                pair.second--;
            }
        }
    }
    
    LOG_INF_S("Removed occluder, total: " + std::to_string(m_occluders.size()));
}

bool OcclusionCuller::isShapeVisible(const TopoDS_Shape& shape) {
    if (!m_enabled || shape.IsNull() || m_occluders.empty()) {
        return true;
    }
    
    // Calculate bounding box for the shape
    Bnd_Box bbox;
    BRepBndLib::Add(shape, bbox);
    
    if (bbox.IsVoid()) {
        return true;
    }
    
    // Calculate center
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
    
    bool visible = isBoundingBoxVisible(bbox, center);
    if (!visible) {
        m_occludedCount++;
    }
    
    return visible;
}

bool OcclusionCuller::isBoundingBoxVisible(const Bnd_Box& bbox, const gp_Pnt& center) {
    if (!m_enabled || bbox.IsVoid() || m_occluders.empty()) {
        return true;
    }
    
    // Check against all visible occluders
    for (const auto& occluder : m_occluders) {
        if (!occluder.isVisible) {
            continue;
        }
        
        if (isBboxOccludedByOccluder(bbox, occluder)) {
            return false; // Occluded
        }
    }
    
    return true; // Visible
}

OcclusionCuller::OcclusionQuery OcclusionCuller::performOcclusionQuery(const Bnd_Box& bbox) {
    OcclusionQuery query;
    query.queryId = m_nextQueryId++;
    query.bbox = bbox;
    
    if (!m_enabled || bbox.IsVoid()) {
        query.isOccluded = false;
        return query;
    }
    
    // Calculate center for depth
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
    
    query.isOccluded = !isBoundingBoxVisible(bbox, center);
    
    return query;
}

void OcclusionCuller::clearOccluders() {
    m_occluders.clear();
    m_occluderMap.clear();
    LOG_INF_S("Cleared all occluders");
}

void OcclusionCuller::updateOccluderDepths(const SoCamera* camera) {
    for (auto& occluder : m_occluders) {
        if (occluder.bbox.IsVoid()) {
            continue;
        }
        
        // Calculate depth for center point
        float centerDepth = calculateDepth(occluder.center, camera);
        
        // Calculate depths for bounding box corners
        double xmin, ymin, zmin, xmax, ymax, zmax;
        occluder.bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        gp_Pnt corners[8] = {
            gp_Pnt(xmin, ymin, zmin),
            gp_Pnt(xmax, ymin, zmin),
            gp_Pnt(xmin, ymax, zmin),
            gp_Pnt(xmax, ymax, zmin),
            gp_Pnt(xmin, ymin, zmax),
            gp_Pnt(xmax, ymin, zmax),
            gp_Pnt(xmin, ymax, zmax),
            gp_Pnt(xmax, ymax, zmax)
        };
        
        float minDepth = centerDepth;
        float maxDepth = centerDepth;
        
        for (int i = 0; i < 8; ++i) {
            float depth = calculateDepth(corners[i], camera);
            minDepth = std::min(minDepth, depth);
            maxDepth = std::max(maxDepth, depth);
        }
        
        occluder.minDepth = minDepth;
        occluder.maxDepth = maxDepth;
    }
}

void OcclusionCuller::sortOccludersByDepth() {
    std::sort(m_occluders.begin(), m_occluders.end(), 
        [](const Occluder& a, const Occluder& b) {
            return a.minDepth < b.minDepth;
        });
}

bool OcclusionCuller::isBboxOccludedByOccluder(const Bnd_Box& bbox, const Occluder& occluder) const {
    // Check if occluder can potentially occlude this bbox
    if (!occluder.canOcclude(bbox)) {
        return false;
    }
    
    // Check if occluder is closer (simplified depth test)
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    gp_Pnt bboxCenter((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
    
    // Simple depth comparison
    if (occluder.maxDepth < occluder.minDepth) {
        return false; // Invalid depth range
    }
    
    // If occluder's maximum depth is less than bbox's minimum depth, it can occlude
    // This is a simplified test - in a real implementation, you'd use GPU occlusion queries
    return occluder.maxDepth < occluder.minDepth + 0.1f; // Small tolerance
}

float OcclusionCuller::calculateDepth(const gp_Pnt& point, const SoCamera* camera) const {
    if (!camera) {
        return 0.0f;
    }
    
    // Get camera's view matrix
    SbMatrix viewMatrix = camera->getViewVolume().getMatrix();
    
    // Transform point to view space
    SbVec3f viewPoint(
        static_cast<float>(point.X()),
        static_cast<float>(point.Y()),
        static_cast<float>(point.Z())
    );
    
    // Apply view transformation (simplified)
    // In a real implementation, you'd properly transform the point
    float depth = viewPoint[2]; // Z component in view space
    
    return depth;
}

void OcclusionCuller::cullDistantOccluders() {
    if (m_occluders.size() <= static_cast<size_t>(m_maxOccluders)) {
        return;
    }
    
    // Keep only the closest occluders
    std::sort(m_occluders.begin(), m_occluders.end(), 
        [](const Occluder& a, const Occluder& b) {
            return a.minDepth < b.minDepth;
        });
    
    // Remove distant occluders
    m_occluders.resize(m_maxOccluders);
    
    // Update map
    m_occluderMap.clear();
    for (size_t i = 0; i < m_occluders.size(); ++i) {
        m_occluderMap[&m_occluders[i].shape] = i;
    }
    
    LOG_INF_S("Culled distant occluders, remaining: " + std::to_string(m_occluders.size()));
} 