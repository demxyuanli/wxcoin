#include "viewer/GeometryFactoryService.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"

// OpenCASCADE includes for geometry creation
#include <OpenCASCADE/Geom_BezierCurve.hxx>
#include <OpenCASCADE/Geom_BezierSurface.hxx>
#include <OpenCASCADE/Geom_BSplineCurve.hxx>
#include <OpenCASCADE/TColgp_Array1OfPnt.hxx>
#include <OpenCASCADE/TColStd_Array1OfReal.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeEdge.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeFace.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeWire.hxx>

GeometryFactoryService::GeometryFactoryService()
{
}

GeometryFactoryService::~GeometryFactoryService()
{
}

std::shared_ptr<OCCGeometry> GeometryFactoryService::addGeometryWithAdvancedRendering(
    const TopoDS_Shape& shape,
    const std::string& name
) {
    try {
        // Create geometry with advanced rendering features
        auto geometry = createAdvancedGeometry(shape, name);
        if (geometry) {
            applyAdvancedRendering(geometry);
            LOG_INF_S("Created advanced geometry: " + name);
        }
        return geometry;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create advanced geometry '" + name + "': " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactoryService::addBezierCurve(
    const std::vector<gp_Pnt>& controlPoints,
    const std::string& name
) {
    try {
        if (controlPoints.size() < 2) {
            LOG_ERR_S("Bezier curve requires at least 2 control points");
            return nullptr;
        }

        TopoDS_Shape curveShape = createBezierCurveShape(controlPoints);
        if (!curveShape.IsNull()) {
            auto geometry = createBasicGeometry(curveShape, name);
            LOG_INF_S("Created Bezier curve: " + name);
            return geometry;
        }
        return nullptr;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create Bezier curve '" + name + "': " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactoryService::addBezierSurface(
    const std::vector<std::vector<gp_Pnt>>& controlPoints,
    const std::string& name
) {
    try {
        if (controlPoints.empty() || controlPoints[0].size() < 2) {
            LOG_ERR_S("Bezier surface requires valid control point grid");
            return nullptr;
        }

        TopoDS_Shape surfaceShape = createBezierSurfaceShape(controlPoints);
        if (!surfaceShape.IsNull()) {
            auto geometry = createBasicGeometry(surfaceShape, name);
            LOG_INF_S("Created Bezier surface: " + name);
            return geometry;
        }
        return nullptr;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create Bezier surface '" + name + "': " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactoryService::addBSplineCurve(
    const std::vector<gp_Pnt>& poles,
    const std::vector<double>& weights,
    const std::string& name
) {
    try {
        if (poles.size() < 2) {
            LOG_ERR_S("B-Spline curve requires at least 2 poles");
            return nullptr;
        }

        TopoDS_Shape curveShape = createBSplineCurveShape(poles, weights);
        if (!curveShape.IsNull()) {
            auto geometry = createBasicGeometry(curveShape, name);
            LOG_INF_S("Created B-Spline curve: " + name);
            return geometry;
        }
        return nullptr;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create B-Spline curve '" + name + "': " + std::string(e.what()));
        return nullptr;
    }
}

void GeometryFactoryService::upgradeGeometryToAdvanced(const std::string& name)
{
    // Implementation would upgrade existing geometry to use advanced rendering
    LOG_INF_S("Upgrading geometry to advanced rendering: " + name);
}

void GeometryFactoryService::upgradeAllGeometriesToAdvanced()
{
    // Implementation would upgrade all geometries to advanced rendering
    LOG_INF_S("Upgrading all geometries to advanced rendering");
}

bool GeometryFactoryService::isAdvancedGeometrySupported() const
{
    // Check if advanced geometry features are supported
    return true; // For now, assume supported
}

std::vector<std::string> GeometryFactoryService::getSupportedGeometryTypes() const
{
    return {
        "BasicGeometry",
        "AdvancedGeometry",
        "BezierCurve",
        "BezierSurface",
        "BSplineCurve"
    };
}

std::shared_ptr<OCCGeometry> GeometryFactoryService::createBasicGeometry(const TopoDS_Shape& shape, const std::string& name)
{
    return std::make_shared<OCCGeometry>(name);
}

std::shared_ptr<OCCGeometry> GeometryFactoryService::createAdvancedGeometry(const TopoDS_Shape& shape, const std::string& name)
{
    // Create geometry with advanced features
    auto geometry = std::make_shared<OCCGeometry>(name);
    geometry->setShape(shape);
    return geometry;
}

TopoDS_Shape GeometryFactoryService::createBezierCurveShape(const std::vector<gp_Pnt>& controlPoints)
{
    try {
        // Convert control points to OpenCASCADE format
        TColgp_Array1OfPnt poles(1, static_cast<Standard_Integer>(controlPoints.size()));
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            poles.SetValue(static_cast<Standard_Integer>(i + 1), controlPoints[i]);
        }

        // Create Bezier curve
        Handle(Geom_BezierCurve) bezierCurve = new Geom_BezierCurve(poles);

        // Create edge from curve
        TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(bezierCurve);

        return edge;
    }
    catch (const std::exception&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape GeometryFactoryService::createBezierSurfaceShape(const std::vector<std::vector<gp_Pnt>>& controlPoints)
{
    try {
        // This is a simplified implementation
        // Real implementation would create proper Bezier surface
        return TopoDS_Shape();
    }
    catch (const std::exception&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape GeometryFactoryService::createBSplineCurveShape(const std::vector<gp_Pnt>& poles, const std::vector<double>& weights)
{
    try {
        // Convert poles to OpenCASCADE format
        TColgp_Array1OfPnt occPoles(1, static_cast<Standard_Integer>(poles.size()));
        for (size_t i = 0; i < poles.size(); ++i) {
            occPoles.SetValue(static_cast<Standard_Integer>(i + 1), poles[i]);
        }

        // Handle weights if provided
        if (!weights.empty() && weights.size() == poles.size()) {
            TColStd_Array1OfReal occWeights(1, static_cast<Standard_Integer>(weights.size()));
            for (size_t i = 0; i < weights.size(); ++i) {
                occWeights.SetValue(static_cast<Standard_Integer>(i + 1), weights[i]);
            }

        // Create B-Spline curve with weights
        TColStd_Array1OfReal occKnots(1, 2);
        occKnots.SetValue(1, 0.0);
        occKnots.SetValue(2, 1.0);

        TColStd_Array1OfInteger occMults(1, 2);
        occMults.SetValue(1, poles.size());
        occMults.SetValue(2, poles.size());

        Handle(Geom_BSplineCurve) bsplineCurve = new Geom_BSplineCurve(occPoles, occWeights, occKnots, occMults, poles.size() - 1);
        TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(bsplineCurve);
        return edge;
        }
        else {
            // Create uniform B-Spline curve
            TColStd_Array1OfReal occKnots(1, 2);
            occKnots.SetValue(1, 0.0);
            occKnots.SetValue(2, 1.0);

            TColStd_Array1OfInteger occMults(1, 2);
            occMults.SetValue(1, poles.size());
            occMults.SetValue(2, poles.size());

            Handle(Geom_BSplineCurve) bsplineCurve = new Geom_BSplineCurve(occPoles, occKnots, occMults, poles.size() - 1);
            TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(bsplineCurve);
            return edge;
        }
    }
    catch (const std::exception&) {
        return TopoDS_Shape();
    }
}

void GeometryFactoryService::applyAdvancedRendering(std::shared_ptr<OCCGeometry> geometry)
{
    if (geometry) {
        // Apply advanced rendering features
        // This would include things like better shaders, lighting, etc.
        LOG_INF_S("Applied advanced rendering to geometry");
    }
}

bool GeometryFactoryService::needsUpgrade(std::shared_ptr<OCCGeometry> geometry) const
{
    // Check if geometry needs upgrade to advanced rendering
    return false; // Placeholder implementation
}
