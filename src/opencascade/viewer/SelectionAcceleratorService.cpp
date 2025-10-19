#include "viewer/SelectionAcceleratorService.h"
#include "SelectionAccelerator.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"
#include <chrono>
#include <algorithm>
#include <limits>
#include <cmath>

SelectionAcceleratorService::SelectionAcceleratorService()
    : m_accelerator(std::make_unique<SelectionAccelerator>())
{
}

SelectionAcceleratorService::~SelectionAcceleratorService()
{
}

void SelectionAcceleratorService::rebuildFromGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    if (!m_accelerator) {
        LOG_ERR_S("Selection accelerator not initialized");
        return;
    }


    // Collect all shapes from geometries
    std::vector<TopoDS_Shape> shapes;
    shapes.reserve(geometries.size());

    for (const auto& geometry : geometries) {
        if (geometry && !geometry->getShape().IsNull() && geometry->isVisible()) {
            shapes.push_back(geometry->getShape());
        }
    }


    // Rebuild accelerator for shape-level selection
    if (!shapes.empty()) {
        auto startTime = std::chrono::high_resolution_clock::now();

        bool success = m_accelerator->build(shapes, SelectionAccelerator::SelectionMode::Shapes);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        if (success) {
            m_shapeCount = shapes.size();

        } else {
            m_shapeCount = 0;
            LOG_ERR_S("Failed to rebuild selection accelerator");
        }
    } else {
        m_shapeCount = 0;
        LOG_WRN_S("No shapes available for selection accelerator rebuild");
    }
}

std::shared_ptr<OCCGeometry> SelectionAcceleratorService::pickByRay(
    const gp_Pnt& origin,
    const gp_Vec& direction,
    const std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    if (!m_accelerator || !isReady()) {
        return nullptr;
    }

    SelectionAccelerator::SelectionResult result;
    if (m_accelerator->selectByRay(origin, direction, result)) {
        // Map shape index back to geometry
        if (result.shapeIndex < geometries.size()) {
            return geometries[result.shapeIndex];
        }
    }

    return nullptr;
}

std::shared_ptr<OCCGeometry> SelectionAcceleratorService::pickByDistance(
    const SbVec3f& worldPos,
    const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    double pickingRadius)
{
    std::shared_ptr<OCCGeometry> closestGeometry = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for (const auto& geometry : geometries) {
        if (!geometry || !geometry->isVisible()) continue;

        gp_Pnt geometryPos = geometry->getPosition();

        double distance = std::sqrt(
            std::pow(worldPos[0] - geometryPos.X(), 2) +
            std::pow(worldPos[1] - geometryPos.Y(), 2) +
            std::pow(worldPos[2] - geometryPos.Z(), 2)
        );

        if (distance < minDistance && distance < pickingRadius) {
            minDistance = distance;
            closestGeometry = geometry;
        }
    }

    return closestGeometry;
}

void SelectionAcceleratorService::clear()
{
    if (m_accelerator) {
        m_accelerator->clear();
    }
    m_shapeCount = 0;
}

bool SelectionAcceleratorService::isReady() const
{
    return m_shapeCount > 0;
}

size_t SelectionAcceleratorService::getShapeCount() const
{
    return m_shapeCount;
}

