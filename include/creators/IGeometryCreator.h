#pragma once

#include <Inventor/SbVec3f.h>
#include <memory>
#include <string>

class OCCGeometry;

/**
 * @brief Interface for geometry creation
 *
 * This interface defines the contract for all geometry creators.
 * Each concrete geometry creator must implement this interface.
 */
class IGeometryCreator {
public:
    virtual ~IGeometryCreator() = default;

    /**
     * @brief Create geometry at specified position
     * @param position The 3D position where geometry should be created
     * @return Shared pointer to the created geometry, or nullptr on failure
     */
    virtual std::shared_ptr<OCCGeometry> createGeometry(const SbVec3f& position) = 0;

    /**
     * @brief Get the geometry type name
     * @return String representing the geometry type (e.g., "Box", "Sphere", etc.)
     */
    virtual std::string getGeometryType() const = 0;

    /**
     * @brief Get a display name for the geometry type
     * @return Human-readable name for the geometry type
     */
    virtual std::string getDisplayName() const = 0;

    /**
     * @brief Check if this creator can handle the specified geometry type
     * @param type The geometry type string to check
     * @return True if this creator can handle the type, false otherwise
     */
    virtual bool canHandleType(const std::string& type) const = 0;
};
