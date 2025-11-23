#include "creators/GeometryCreatorFactory.h"
#include "creators/GeometryCreator.h"
#include "logger/Logger.h"
#include <algorithm>

// Static registry of geometry types and their display names
const std::unordered_map<std::string, std::string> GeometryCreatorFactory::s_typeDisplayNames = {
    {"Box", "Box"},
    {"Sphere", "Sphere"},
    {"Cylinder", "Cylinder"},
    {"Cone", "Cone"},
    {"Torus", "Torus"},
    {"TruncatedCylinder", "Truncated Cylinder"},
    {"NavCube", "Navigation Cube"}
};

std::shared_ptr<IGeometryCreator> GeometryCreatorFactory::getGeometryCreator(GeometryFactory* factory, const std::string& type) {
    if (!factory) {
        LOG_ERR_S("GeometryCreatorFactory: Factory is null");
        return nullptr;
    }

    if (type == "Box") {
        return std::make_shared<BoxCreator>(factory);
    }
    else if (type == "Sphere") {
        return std::make_shared<SphereCreator>(factory);
    }
    else if (type == "Cylinder") {
        return std::make_shared<CylinderCreator>(factory);
    }
    else if (type == "Cone") {
        return std::make_shared<ConeCreator>(factory);
    }
    else if (type == "Torus") {
        return std::make_shared<TorusCreator>(factory);
    }
    else if (type == "TruncatedCylinder") {
        return std::make_shared<TruncatedCylinderCreator>(factory);
    }
    else if (type == "NavCube") {
        return std::make_shared<NavCubeCreator>(factory);
    }
    else {
        LOG_ERR_S("Unknown geometry creator type: " + type);
        return nullptr;
    }
}

std::vector<std::string> GeometryCreatorFactory::getAvailableTypes() {
    std::vector<std::string> types;
    types.reserve(s_typeDisplayNames.size());

    for (const auto& pair : s_typeDisplayNames) {
        types.push_back(pair.first);
    }

    return types;
}

std::string GeometryCreatorFactory::getDisplayName(const std::string& type) {
    auto it = s_typeDisplayNames.find(type);
    if (it != s_typeDisplayNames.end()) {
        return it->second;
    }
    return type; // Return the type itself if not found
}

bool GeometryCreatorFactory::isTypeSupported(const std::string& type) {
    return s_typeDisplayNames.find(type) != s_typeDisplayNames.end();
}
