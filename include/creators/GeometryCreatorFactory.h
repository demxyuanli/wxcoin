#pragma once

#include "IGeometryCreator.h"
#include <memory>
#include <string>
#include <unordered_map>

class GeometryFactory;

class GeometryCreatorFactory {
public:
    // Get geometry creator for specific type
    static std::shared_ptr<IGeometryCreator> getGeometryCreator(GeometryFactory* factory, const std::string& type);

    // Get all available geometry types
    static std::vector<std::string> getAvailableTypes();

    // Get display name for a geometry type
    static std::string getDisplayName(const std::string& type);

    // Check if a type is supported
    static bool isTypeSupported(const std::string& type);

private:
    // Registry of supported geometry types
    static const std::unordered_map<std::string, std::string> s_typeDisplayNames;
};
