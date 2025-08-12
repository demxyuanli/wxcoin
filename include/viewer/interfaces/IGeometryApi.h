#pragma once

#include <memory>
#include <string>
#include <vector>

class OCCGeometry;

class IGeometryApi {
public:
    virtual ~IGeometryApi() = default;

    // Geometry management
    virtual void addGeometry(std::shared_ptr<OCCGeometry> geometry) = 0;
    virtual void removeGeometry(std::shared_ptr<OCCGeometry> geometry) = 0;
    virtual void removeGeometry(const std::string& name) = 0;
    virtual void clearAll() = 0;
    virtual std::shared_ptr<OCCGeometry> findGeometry(const std::string& name) = 0;
    virtual std::vector<std::shared_ptr<OCCGeometry>> getAllGeometry() const = 0;
    virtual std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometries() const = 0;

    // Batch addition and deferred updates
    virtual void addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) = 0;
    virtual void updateObjectTreeDeferred() = 0;
};



