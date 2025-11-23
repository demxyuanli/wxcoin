#include "creators/GeometryCreator.h"
#include "GeometryFactory.h"
#include "logger/Logger.h"

// Box Creator Implementation
BoxCreator::BoxCreator(GeometryFactory* factory) : GeometryCreator(factory) {}

std::shared_ptr<OCCGeometry> BoxCreator::createGeometry(const SbVec3f& position) {
    if (!isFactoryValid()) {
        LOG_ERR_S("BoxCreator: Factory is null");
        return nullptr;
    }

    LOG_INF_S("Creating Box geometry at position " + createPositionString(position));
    return m_factory->createOCCBox(position, 2.0, 2.0, 2.0); // Default dimensions
}

// Sphere Creator Implementation
SphereCreator::SphereCreator(GeometryFactory* factory) : GeometryCreator(factory) {}

std::shared_ptr<OCCGeometry> SphereCreator::createGeometry(const SbVec3f& position) {
    if (!isFactoryValid()) {
        LOG_ERR_S("SphereCreator: Factory is null");
        return nullptr;
    }

    LOG_INF_S("Creating Sphere geometry at position " + createPositionString(position));
    return m_factory->createOCCSphere(position, 1.0); // Default radius
}

// Cylinder Creator Implementation
CylinderCreator::CylinderCreator(GeometryFactory* factory) : GeometryCreator(factory) {}

std::shared_ptr<OCCGeometry> CylinderCreator::createGeometry(const SbVec3f& position) {
    if (!isFactoryValid()) {
        LOG_ERR_S("CylinderCreator: Factory is null");
        return nullptr;
    }

    LOG_INF_S("Creating Cylinder geometry at position " + createPositionString(position));
    return m_factory->createOCCCylinder(position, 1.0, 2.0); // Default radius and height
}

// Cone Creator Implementation
ConeCreator::ConeCreator(GeometryFactory* factory) : GeometryCreator(factory) {}

std::shared_ptr<OCCGeometry> ConeCreator::createGeometry(const SbVec3f& position) {
    if (!isFactoryValid()) {
        LOG_ERR_S("ConeCreator: Factory is null");
        return nullptr;
    }

    LOG_INF_S("Creating Cone geometry at position " + createPositionString(position));
    return m_factory->createOCCCone(position, 1.0, 0.5, 2.0); // Default radii and height
}

// Torus Creator Implementation
TorusCreator::TorusCreator(GeometryFactory* factory) : GeometryCreator(factory) {}

std::shared_ptr<OCCGeometry> TorusCreator::createGeometry(const SbVec3f& position) {
    if (!isFactoryValid()) {
        LOG_ERR_S("TorusCreator: Factory is null");
        return nullptr;
    }

    LOG_INF_S("Creating Torus geometry at position " + createPositionString(position));
    return m_factory->createOCCTorus(position, 2.0, 0.5); // Default radii
}

// Truncated Cylinder Creator Implementation
TruncatedCylinderCreator::TruncatedCylinderCreator(GeometryFactory* factory) : GeometryCreator(factory) {}

std::shared_ptr<OCCGeometry> TruncatedCylinderCreator::createGeometry(const SbVec3f& position) {
    if (!isFactoryValid()) {
        LOG_ERR_S("TruncatedCylinderCreator: Factory is null");
        return nullptr;
    }

    LOG_INF_S("Creating TruncatedCylinder geometry at position " + createPositionString(position));
    return m_factory->createOCCTruncatedCylinder(position, 1.0, 0.5, 2.0); // Default radii and height
}

// NavCube Creator Implementation
NavCubeCreator::NavCubeCreator(GeometryFactory* factory) : GeometryCreator(factory) {}

std::shared_ptr<OCCGeometry> NavCubeCreator::createGeometry(const SbVec3f& position) {
    if (!isFactoryValid()) {
        LOG_ERR_S("NavCubeCreator: Factory is null");
        return nullptr;
    }

    LOG_INF_S("Creating NavCube geometry at position " + createPositionString(position));
    return m_factory->createOCCNavCube(position, 2.0); // Default size
}
