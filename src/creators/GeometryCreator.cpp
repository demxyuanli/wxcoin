#include "creators/GeometryCreator.h"
#include "logger/Logger.h"

GeometryCreator::GeometryCreator(GeometryFactory* factory) : m_factory(factory) {
    if (!m_factory) {
        LOG_ERR_S("GeometryCreator: Factory is null");
    }
}

std::string GeometryCreator::getDisplayName() const {
    return getGeometryType(); // Default implementation returns the type name
}

bool GeometryCreator::canHandleType(const std::string& type) const {
    return type == getGeometryType(); // Default implementation checks exact match
}

bool GeometryCreator::isFactoryValid() const {
    return m_factory != nullptr;
}

std::string GeometryCreator::createPositionString(const SbVec3f& position) const {
    return "(" + std::to_string(position[0]) + ", " +
           std::to_string(position[1]) + ", " +
           std::to_string(position[2]) + ")";
}
