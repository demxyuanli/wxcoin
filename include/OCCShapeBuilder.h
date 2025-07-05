#pragma once

#include <memory>
#include <string>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>

/**
 * @brief OpenCASCADE shape builder
 * 
 * Provides static methods for creating various CAD geometric shapes
 */
class OCCShapeBuilder {
public:
    // Basic geometric primitives
    static TopoDS_Shape createBox(double width, double height, double depth, 
                                  const gp_Pnt& position = gp_Pnt(0,0,0));
    
    static TopoDS_Shape createSphere(double radius, 
                                     const gp_Pnt& center = gp_Pnt(0,0,0));
    
    static TopoDS_Shape createCylinder(double radius, double height, 
                                       const gp_Pnt& position = gp_Pnt(0,0,0),
                                       const gp_Dir& direction = gp_Dir(0,0,1));
    
    static TopoDS_Shape createCone(double bottomRadius, double topRadius, double height,
                                   const gp_Pnt& position = gp_Pnt(0,0,0),
                                   const gp_Dir& direction = gp_Dir(0,0,1));
    
    static TopoDS_Shape createTorus(double majorRadius, double minorRadius,
                                    const gp_Pnt& center = gp_Pnt(0,0,0),
                                    const gp_Dir& direction = gp_Dir(0,0,1));

    // Complex geometric operations
    static TopoDS_Shape createExtrusion(const TopoDS_Shape& profile, const gp_Vec& direction);
    static TopoDS_Shape createRevolution(const TopoDS_Shape& profile, 
                                         const gp_Pnt& axisPosition,
                                         const gp_Dir& axisDirection,
                                         double angle = 2.0 * M_PI);
    
    static TopoDS_Shape createLoft(const std::vector<TopoDS_Shape>& profiles, bool solid = true);
    static TopoDS_Shape createPipe(const TopoDS_Shape& profile, const TopoDS_Shape& spine);

    // Boolean operations
    static TopoDS_Shape booleanUnion(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    static TopoDS_Shape booleanIntersection(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    static TopoDS_Shape booleanDifference(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);

    // Filleting and chamfering
    static TopoDS_Shape createFillet(const TopoDS_Shape& shape, double radius);
    static TopoDS_Shape createChamfer(const TopoDS_Shape& shape, double distance);

    // Transform operations
    static TopoDS_Shape translate(const TopoDS_Shape& shape, const gp_Vec& translation);
    static TopoDS_Shape rotate(const TopoDS_Shape& shape, const gp_Pnt& center, 
                               const gp_Dir& axis, double angle);
    static TopoDS_Shape scale(const TopoDS_Shape& shape, const gp_Pnt& center, double factor);
    static TopoDS_Shape mirror(const TopoDS_Shape& shape, const gp_Pnt& point, const gp_Dir& normal);

    // Utility methods
    static bool isValid(const TopoDS_Shape& shape);
    static double getVolume(const TopoDS_Shape& shape);
    static double getSurfaceArea(const TopoDS_Shape& shape);
    static void getBoundingBox(const TopoDS_Shape& shape, 
                               gp_Pnt& minPoint, gp_Pnt& maxPoint);

private:
    OCCShapeBuilder() = delete; // Pure static class
}; 