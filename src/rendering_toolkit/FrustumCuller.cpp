#include "rendering/FrustumCuller.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec3f.h>
#include <cmath>
#include <algorithm>

FrustumCuller::FrustumCuller() 
    : m_enabled(true)
    , m_culledCount(0) {
    m_frustumPlanes.resize(6); // 6 planes: near, far, left, right, top, bottom
    LOG_INF_S("FrustumCuller created");
}

FrustumCuller::~FrustumCuller() {
    LOG_INF_S("FrustumCuller destroyed");
}

void FrustumCuller::FrustumPlane::normalize() {
    float length = std::sqrt(a * a + b * b + c * c);
    if (length > 1e-6f) {
        a /= length;
        b /= length;
        c /= length;
        d /= length;
    }
}

float FrustumCuller::FrustumPlane::distance(const gp_Pnt& point) const {
    return a * static_cast<float>(point.X()) + 
           b * static_cast<float>(point.Y()) + 
           c * static_cast<float>(point.Z()) + d;
}

void FrustumCuller::CullableBoundingBox::updateFromShape(const TopoDS_Shape& shape) {
    if (shape.IsNull()) {
        return;
    }
    
    // Compute bounding box
    BRepBndLib::Add(shape, bbox);
    
    if (!bbox.IsVoid()) {
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        // Calculate center
        center = gp_Pnt((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
        
        // Calculate radius (distance from center to farthest corner)
        double dx = xmax - xmin;
        double dy = ymax - ymin;
        double dz = zmax - zmin;
        radius = std::sqrt(dx * dx + dy * dy + dz * dz) / 2.0;
    }
}

bool FrustumCuller::CullableBoundingBox::isInFrustum(const FrustumCuller& owner) const {
    if (bbox.IsVoid()) {
        return false;
    }
    if (!owner.sphereInFrustum(center, radius)) {
        return false;
    }

    return true;
}

bool FrustumCuller::CullableBoundingBox::isOutsideFrustum(const FrustumCuller& owner) const {
    if (bbox.IsVoid()) {
        return true;
    }
    if (!owner.sphereInFrustum(center, radius)) {
        return true;
    }

    return false;
}

void FrustumCuller::updateFrustum(const SoCamera* camera) {
    if (!camera || !m_enabled) {
        return;
    }
    
    extractFrustumPlanes(camera);
}

void FrustumCuller::extractFrustumPlanes(const SoCamera* camera) {
    SbMatrix viewMatrix = camera->getViewVolume().getMatrix();
}

bool FrustumCuller::isShapeVisible(const TopoDS_Shape& shape) const {
    if (!m_enabled || shape.IsNull()) {
        return true;
    }
    
    CullableBoundingBox bbox;
    bbox.updateFromShape(shape);
    
    bool visible = bbox.isInFrustum(*this);
    if (!visible) {
        m_culledCount++;
    }
    
    return visible;
}

bool FrustumCuller::isBoundingBoxVisible(const CullableBoundingBox& bbox) {
    if (!m_enabled || bbox.bbox.IsVoid()) {
        return true;
    }
    
    return bbox.isInFrustum(*this);
}

bool FrustumCuller::pointInFrustum(const gp_Pnt& point) const {
    for (const auto& plane : m_frustumPlanes) {
        if (plane.distance(point) < 0) {
            return false;
        }
    }
    return true;
}

bool FrustumCuller::sphereInFrustum(const gp_Pnt& center, double radius) const {
    for (const auto& plane : m_frustumPlanes) {
        if (plane.distance(center) < -static_cast<float>(radius)) {
            return false;
        }
    }
    return true;
}

bool FrustumCuller::boxInFrustum(const Bnd_Box& bbox) const {
    if (bbox.IsVoid()) {
        return false;
    }
    
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    
    // Test all 8 corners
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
    
    for (int i = 0; i < 8; ++i) {
        if (pointInFrustum(corners[i])) {
            return true;
        }
    }
    
    return false;
} 