#pragma once

#include "IGeometryCreator.h"
#include "GeometryFactory.h"
#include <memory>
#include <string>

/**
 * @brief Base class for geometry creators
 *
 * This class provides common functionality for all geometry creators
 * and implements the IGeometryCreator interface.
 */
class GeometryCreator : public IGeometryCreator {
public:
    GeometryCreator(GeometryFactory* factory);
    virtual ~GeometryCreator() = default;

    // Interface implementation - pure virtual in derived classes
    virtual std::shared_ptr<OCCGeometry> createGeometry(const SbVec3f& position) = 0;
    virtual std::string getGeometryType() const = 0;

    // Default implementation for display name (can be overridden)
    virtual std::string getDisplayName() const;

    // Default implementation for type checking (can be overridden)
    virtual bool canHandleType(const std::string& type) const;

protected:
    GeometryFactory* m_factory;

    // Common helper methods
    bool isFactoryValid() const;
    std::string createPositionString(const SbVec3f& position) const;
};

/**
 * @brief Macro to define a standard geometry creator
 *
 * This macro eliminates repetitive header file code for standard geometry creators.
 */
#define DEFINE_STANDARD_CREATOR(ClassName, TypeName, DisplayText) \
class ClassName : public GeometryCreator { \
public: \
    ClassName(GeometryFactory* factory); \
    virtual ~ClassName() = default; \
    \
    std::shared_ptr<OCCGeometry> createGeometry(const SbVec3f& position) override; \
    std::string getGeometryType() const override { return TypeName; } \
    std::string getDisplayName() const override { return DisplayText; } \
};

// Define all standard geometry creators
DEFINE_STANDARD_CREATOR(BoxCreator, "Box", "Box")
DEFINE_STANDARD_CREATOR(SphereCreator, "Sphere", "Sphere")
DEFINE_STANDARD_CREATOR(CylinderCreator, "Cylinder", "Cylinder")
DEFINE_STANDARD_CREATOR(ConeCreator, "Cone", "Cone")
DEFINE_STANDARD_CREATOR(TorusCreator, "Torus", "Torus")
DEFINE_STANDARD_CREATOR(TruncatedCylinderCreator, "TruncatedCylinder", "Truncated Cylinder")
DEFINE_STANDARD_CREATOR(NavCubeCreator, "NavCube", "Navigation Cube")
