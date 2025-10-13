#pragma once

#include <memory>
#include <vector>
#include <string>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>

class OCCGeometry;

/**
 * @brief Service for creating advanced geometries and geometric primitives
 *
 * This service encapsulates complex geometry creation operations including
 * advanced rendering geometries, curves, surfaces, and geometry upgrades.
 */
class GeometryFactoryService {
public:
    GeometryFactoryService();
    ~GeometryFactoryService();

    // Advanced geometry creation
    std::shared_ptr<OCCGeometry> addGeometryWithAdvancedRendering(
        const TopoDS_Shape& shape,
        const std::string& name
    );

    // Curve creation
    std::shared_ptr<OCCGeometry> addBezierCurve(
        const std::vector<gp_Pnt>& controlPoints,
        const std::string& name
    );

    // Surface creation
    std::shared_ptr<OCCGeometry> addBezierSurface(
        const std::vector<std::vector<gp_Pnt>>& controlPoints,
        const std::string& name
    );

    std::shared_ptr<OCCGeometry> addBSplineCurve(
        const std::vector<gp_Pnt>& poles,
        const std::vector<double>& weights,
        const std::string& name
    );

    // Geometry upgrade operations
    void upgradeGeometryToAdvanced(const std::string& name);
    void upgradeAllGeometriesToAdvanced();

    // Utility methods
    bool isAdvancedGeometrySupported() const;
    std::vector<std::string> getSupportedGeometryTypes() const;

private:
    // Helper methods for geometry creation
    std::shared_ptr<OCCGeometry> createBasicGeometry(const TopoDS_Shape& shape, const std::string& name);
    std::shared_ptr<OCCGeometry> createAdvancedGeometry(const TopoDS_Shape& shape, const std::string& name);

    // Curve and surface creation helpers
    TopoDS_Shape createBezierCurveShape(const std::vector<gp_Pnt>& controlPoints);
    TopoDS_Shape createBezierSurfaceShape(const std::vector<std::vector<gp_Pnt>>& controlPoints);
    TopoDS_Shape createBSplineCurveShape(const std::vector<gp_Pnt>& poles, const std::vector<double>& weights);

    // Upgrade helpers
    void applyAdvancedRendering(std::shared_ptr<OCCGeometry> geometry);
    bool needsUpgrade(std::shared_ptr<OCCGeometry> geometry) const;
};
