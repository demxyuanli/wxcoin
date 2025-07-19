#pragma once

#include <memory>
#include <string>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/gp_Dir.hxx>

// Forward declarations for OpenCASCADE geometry classes
class Geom_BezierCurve;
class Geom_BezierSurface;
class Geom_BSplineCurve;
class Geom_BSplineSurface;
class Geom_Surface;

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

    // Bezier curve and surface creation
    static TopoDS_Shape createBezierCurve(const std::vector<gp_Pnt>& controlPoints);
    static TopoDS_Shape createBezierSurface(const std::vector<std::vector<gp_Pnt>>& controlPoints);
    
    // B-spline curve and surface creation
    static TopoDS_Shape createBSplineCurve(const std::vector<gp_Pnt>& poles, 
                                           const std::vector<double>& weights = {},
                                           int degree = 3);
    static TopoDS_Shape createBSplineSurface(const std::vector<std::vector<gp_Pnt>>& poles,
                                             const std::vector<std::vector<double>>& weights = {},
                                             int uDegree = 3, int vDegree = 3);
    
    // NURBS curve and surface creation
    static TopoDS_Shape createNURBSCurve(const std::vector<gp_Pnt>& poles,
                                         const std::vector<double>& weights,
                                         const std::vector<double>& knots,
                                         const std::vector<int>& multiplicities,
                                         int degree = 3);
    static TopoDS_Shape createNURBSSurface(const std::vector<std::vector<gp_Pnt>>& poles,
                                           const std::vector<std::vector<double>>& weights,
                                           const std::vector<double>& uKnots,
                                           const std::vector<double>& vKnots,
                                           const std::vector<int>& uMultiplicities,
                                           const std::vector<int>& vMultiplicities,
                                           int uDegree = 3, int vDegree = 3);

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

    // Debug and analysis methods
    static void analyzeShapeTopology(const TopoDS_Shape& shape, const std::string& shapeName = "");
    static void outputFaceNormalsAndIndices(const TopoDS_Shape& shape, const std::string& shapeName = "");
    static bool checkShapeClosure(const TopoDS_Shape& shape, const std::string& shapeName = "");
    static void analyzeShapeProperties(const TopoDS_Shape& shape, const std::string& shapeName = "");


private:
    OCCShapeBuilder() = delete; // Pure static class
}; 