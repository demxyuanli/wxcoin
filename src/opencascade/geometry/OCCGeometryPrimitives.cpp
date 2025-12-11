#include "OCCGeometry.h"
#include "logger/Logger.h"
#include <algorithm>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRep_Builder.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopAbs.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <cmath>

// ===== OCCBox Implementation =====

OCCBox::OCCBox(const std::string& name, double width, double height, double depth)
    : OCCGeometry(name)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
{
    buildShape();
}

void OCCBox::setDimensions(double width, double height, double depth)
{
    m_width = width;
    m_height = height;
    m_depth = depth;
    buildShape();
}

void OCCBox::getSize(double& width, double& height, double& depth) const
{
    width = m_width;
    height = m_height;
    depth = m_depth;
}

void OCCBox::buildShape()
{
    gp_Pnt pos = getPosition();

    try {
        // Create box directly at specified position
        BRepPrimAPI_MakeBox boxMaker(pos, m_width, m_height, m_depth);
        boxMaker.Build();

        if (boxMaker.IsDone()) {
            TopoDS_Shape shape = boxMaker.Shape();
            setShape(shape);
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeBox failed for: " + getName());
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create box: " + std::string(e.what()));
    }
}

// ===== OCCCylinder Implementation =====

OCCCylinder::OCCCylinder(const std::string& name, double radius, double height)
    : OCCGeometry(name)
    , m_radius(radius)
    , m_height(height)
{
    buildShape();
}

void OCCCylinder::setDimensions(double radius, double height)
{
    m_radius = radius;
    m_height = height;
    buildShape();
}

void OCCCylinder::getSize(double& radius, double& height) const
{
    radius = m_radius;
    height = m_height;
}

void OCCCylinder::buildShape()
{
    gp_Pnt pos = getPosition();

    try {
        // Create cylinder at specified position using gp_Ax2
        gp_Ax2 axis(pos, gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCylinder cylinderMaker(axis, m_radius, m_height);
        cylinderMaker.Build();

        if (cylinderMaker.IsDone()) {
            TopoDS_Shape shape = cylinderMaker.Shape();
            setShape(shape);
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeCylinder failed for: " + getName());
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create cylinder: " + std::string(e.what()));
    }
}

// ===== OCCSphere Implementation =====

OCCSphere::OCCSphere(const std::string& name, double radius)
    : OCCGeometry(name)
    , m_radius(radius)
{
    buildShape();
}

void OCCSphere::setRadius(double radius)
{
    m_radius = radius;
    buildShape();
}

void OCCSphere::buildShape()
{
    gp_Pnt pos = getPosition();

    try {
        if (m_radius <= 0.0) {
            LOG_ERR_S("Invalid radius for OCCSphere: " + std::to_string(m_radius));
            return;
        }

        // Create sphere at specified position using gp_Ax2
        gp_Ax2 axis(pos, gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeSphere sphereMaker(axis, m_radius);
        sphereMaker.Build();

        if (sphereMaker.IsDone()) {
            TopoDS_Shape shape = sphereMaker.Shape();
            if (!shape.IsNull()) {
                setShape(shape);
                return;
            } else {
                LOG_ERR_S("Sphere shape is null after creation");
            }
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeSphere failed");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create sphere: " + std::string(e.what()));
    }
}

// ===== OCCCone Implementation =====

OCCCone::OCCCone(const std::string& name, double bottomRadius, double topRadius, double height)
    : OCCGeometry(name)
    , m_bottomRadius(bottomRadius)
    , m_topRadius(topRadius)
    , m_height(height)
{
    buildShape();
}

void OCCCone::setDimensions(double bottomRadius, double topRadius, double height)
{
    m_bottomRadius = bottomRadius;
    m_topRadius = topRadius;
    m_height = height;
    buildShape();
}

void OCCCone::getSize(double& bottomRadius, double& topRadius, double& height) const
{
    bottomRadius = m_bottomRadius;
    topRadius = m_topRadius;
    height = m_height;
}

void OCCCone::buildShape()
{
    gp_Pnt pos = getPosition();

    try {
        // Create cone at specified position
        gp_Ax2 axis(pos, gp_Dir(0, 0, 1));
        double actualTopRadius = m_topRadius;

        if (actualTopRadius <= 0.001) {
        }

        BRepPrimAPI_MakeCone coneMaker(axis, m_bottomRadius, actualTopRadius, m_height);
        coneMaker.Build();

        if (coneMaker.IsDone()) {
            TopoDS_Shape shape = coneMaker.Shape();
            if (!shape.IsNull()) {
                setShape(shape);
            } else {
                LOG_ERR_S("Cone shape is null after creation");
            }
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeCone failed");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to create cone: " + std::string(e.what()));
    }
}

// ===== OCCTorus Implementation =====

OCCTorus::OCCTorus(const std::string& name, double majorRadius, double minorRadius)
    : OCCGeometry(name)
    , m_majorRadius(majorRadius)
    , m_minorRadius(minorRadius)
{
    buildShape();
}

void OCCTorus::setDimensions(double majorRadius, double minorRadius)
{
    m_majorRadius = majorRadius;
    m_minorRadius = minorRadius;
    buildShape();
}

void OCCTorus::getSize(double& majorRadius, double& minorRadius) const
{
    majorRadius = m_majorRadius;
    minorRadius = m_minorRadius;
}

void OCCTorus::buildShape()
{
    gp_Pnt pos = getPosition();
              
    try {
        if (m_majorRadius <= 0.0 || m_minorRadius <= 0.0) {
            LOG_ERR_S("Invalid radii for OCCTorus - major: " + std::to_string(m_majorRadius) + 
                     " minor: " + std::to_string(m_minorRadius));
            return;
        }

        if (m_minorRadius >= m_majorRadius) {
            LOG_ERR_S("Invalid torus dimensions: minor radius must be less than major radius");
            return;
        }

        // Create torus at specified position
        gp_Ax2 axis(pos, gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeTorus torusMaker(axis, m_majorRadius, m_minorRadius);
        torusMaker.Build();

        if (torusMaker.IsDone()) {
            TopoDS_Shape shape = torusMaker.Shape();
            if (!shape.IsNull()) {
                setShape(shape);
            } else {
                LOG_ERR_S("Torus shape is null after creation");
            }
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeTorus failed");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception in OCCTorus::buildShape: " + std::string(e.what()));
    }
}

// ===== OCCTruncatedCylinder Implementation =====

OCCTruncatedCylinder::OCCTruncatedCylinder(const std::string& name, double bottomRadius, double topRadius, double height)
    : OCCGeometry(name)
    , m_bottomRadius(bottomRadius)
    , m_topRadius(topRadius)
    , m_height(height)
{
    buildShape();
}

void OCCTruncatedCylinder::setDimensions(double bottomRadius, double topRadius, double height)
{
    m_bottomRadius = bottomRadius;
    m_topRadius = topRadius;
    m_height = height;
    buildShape();
}

void OCCTruncatedCylinder::getSize(double& bottomRadius, double& topRadius, double& height) const
{
    bottomRadius = m_bottomRadius;
    topRadius = m_topRadius;
    height = m_height;
}

void OCCTruncatedCylinder::buildShape()
{
    gp_Pnt pos = getPosition();
              
    try {
        if (m_bottomRadius <= 0.0 || m_topRadius <= 0.0 || m_height <= 0.0) {
            LOG_ERR_S("Invalid dimensions for OCCTruncatedCylinder - bottom: " + 
                     std::to_string(m_bottomRadius) + " top: " + std::to_string(m_topRadius) + 
                     " height: " + std::to_string(m_height));
            return;
        }

        // Create truncated cylinder at specified position using cone with different radii
        gp_Ax2 axis(pos, gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCone truncatedCylinderMaker(axis, m_bottomRadius, m_topRadius, m_height);
        truncatedCylinderMaker.Build();

        if (truncatedCylinderMaker.IsDone()) {
            TopoDS_Shape shape = truncatedCylinderMaker.Shape();
            if (!shape.IsNull()) {
                setShape(shape);
            }
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeCone failed for OCCTruncatedCylinder");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception in OCCTruncatedCylinder::buildShape: " + std::string(e.what()));
    }
}

// ============================================================================
// OCCNavCube Implementation
// ============================================================================

OCCNavCube::OCCNavCube(const std::string& name, double size)
    : OCCGeometry(name), m_size(size) {
    buildShape();
}

void OCCNavCube::setSize(double size) {
    m_size = size;
    buildShape();
}

void OCCNavCube::getSize(double& size) const {
    size = m_size;
}

// Enumeration for face types in rhombicuboctahedron
enum class NavCubeFaceType {
    Main,   // 8-sided octagon faces (6 total)
    Corner, // Triangular corner faces (8 total)
    Edge    // Quadrilateral edge faces (12 total)
};

// Structure to hold face vertex data
struct NavCubeFace {
    NavCubeFaceType type;
    std::vector<gp_Pnt> vertices;
};

// Forward declarations for helper functions
std::vector<TopoDS_Face> createRhombicuboctahedronFaces(double size, double chamferSize);
TopoDS_Face createMainFace(const gp_Vec& xAxis, const gp_Vec& zAxis, double scale, double chamfer);
TopoDS_Face createCornerFace(const gp_Vec& xAxis, const gp_Vec& zAxis, double scale, double chamfer, double rotZ);
TopoDS_Face createEdgeFace(const gp_Vec& xAxis, const gp_Vec& zAxis, double scale, double chamfer, double rotZ, int edgeIndex);
TopoDS_Face createFaceFromVertices(const std::vector<gp_Pnt>& vertices);

void OCCNavCube::buildShape() {
    try {
        double size = std::max(m_size, 0.1); // Ensure minimum size of 0.1
        double chamferSize = 0.12 * size; // Chamfer size proportional to cube size

        // Create all 26 faces of the rhombicuboctahedron
        std::vector<TopoDS_Face> faces = createRhombicuboctahedronFaces(size, chamferSize);

        if (faces.empty()) {
            LOG_ERR_S("Failed to create any faces for rhombicuboctahedron");
            return;
        }

        // Count actual face types created
        int triangles = 0, quadrilaterals = 0, hexagons = 0, octagons = 0;
        for (const auto& face : faces) {
            if (!face.IsNull()) {
                TopExp_Explorer exp(face, TopAbs_EDGE);
                int edgeCount = 0;
                for (; exp.More(); exp.Next()) {
                    edgeCount++;
                }
                if (edgeCount == 3) triangles++;
                else if (edgeCount == 4) quadrilaterals++;
                else if (edgeCount == 6) hexagons++;
                else if (edgeCount == 8) octagons++;
            }
        }

        // Create solid from faces using BRepBuilderAPI_Sewing for better handling of complex shapes
        BRepBuilderAPI_Sewing sewer;
        sewer.SetTolerance(1e-6); // Set tolerance for sewing

        for (const auto& face : faces) {
            if (!face.IsNull()) {
                sewer.Add(face);
            }
        }

        // Perform sewing
        sewer.Perform();

        // Get the sewn shape
        TopoDS_Shape sewnShape = sewer.SewedShape();
        if (!sewnShape.IsNull()) {
            // Try to create solid from sewn shape
            BRepBuilderAPI_MakeSolid solidMaker;
            if (sewnShape.ShapeType() == TopAbs_SHELL) {
                // If it's already a shell, add it directly
                solidMaker.Add(TopoDS::Shell(sewnShape));
            } else if (sewnShape.ShapeType() == TopAbs_COMPOUND) {
                // If it's a compound, iterate through shells
                TopExp_Explorer exp(sewnShape, TopAbs_SHELL);
                for (; exp.More(); exp.Next()) {
                    solidMaker.Add(TopoDS::Shell(exp.Current()));
                }
            }

            if (solidMaker.IsDone()) {
                TopoDS_Shape solidShape = solidMaker.Shape();
                if (!solidShape.IsNull()) {
                    sewnShape = solidShape;
                }
            }
        }

        if (!sewnShape.IsNull()) {
            // Apply position transformation if needed
            gp_Pnt pos = getPosition();
            if (pos.X() != 0.0 || pos.Y() != 0.0 || pos.Z() != 0.0) {
                // Create transformation for positioning
                gp_Trsf transformation;
                transformation.SetTranslation(gp_Vec(pos.X(), pos.Y(), pos.Z()));
                sewnShape = BRepBuilderAPI_Transform(sewnShape, transformation).Shape();
            }

            setShape(sewnShape);
            } else {
            LOG_ERR_S("Sewing failed for OCCNavCube - no valid shape created");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception in OCCNavCube::buildShape: " + std::string(e.what()));
    }
}

// Create all 26 faces of the rhombicuboctahedron
std::vector<TopoDS_Face> createRhombicuboctahedronFaces(double size, double chamferSize) {
    std::vector<TopoDS_Face> faces;

    // Face counters
    int mainFaces = 0;
    int cornerFaces = 0;
    int edgeFaces = 0;


    // Normalize size to 1.0 for calculations, then scale
    double scale = size / 1.0;
    double chamfer = chamferSize / size; // Normalized chamfer

    // Base vectors
    gp_Vec x(1, 0, 0);
    gp_Vec y(0, 1, 0);
    gp_Vec z(0, 0, 1);


    try {
        // Create 6 main faces (octagons)
        faces.push_back(::createMainFace(x, z, scale, chamfer));      // Top
        faces.push_back(::createMainFace(x, -y, scale, chamfer));     // Front
        faces.push_back(::createMainFace(-y, -x, scale, chamfer));    // Left
        faces.push_back(::createMainFace(-x, y, scale, chamfer));     // Rear
        faces.push_back(::createMainFace(y, x, scale, chamfer));      // Right
        faces.push_back(::createMainFace(x, -z, scale, chamfer));     // Bottom
        mainFaces = 6;

        // Create 8 corner faces (hexagons)
        faces.push_back(::createCornerFace(-x-y, x-y+z, scale, chamfer, M_PI));     // Front-Top-Right
        faces.push_back(::createCornerFace(-x+y, -x-y+z, scale, chamfer, M_PI));    // Front-Top-Left
        faces.push_back(::createCornerFace(x+y, x-y-z, scale, chamfer, 0));         // Front-Bottom-Right
        faces.push_back(::createCornerFace(x-y, -x-y-z, scale, chamfer, 0));        // Front-Bottom-Left
        faces.push_back(::createCornerFace(x-y, x+y+z, scale, chamfer, M_PI));      // Rear-Top-Right
        faces.push_back(::createCornerFace(x+y, -x+y+z, scale, chamfer, M_PI));     // Rear-Top-Left
        faces.push_back(::createCornerFace(-x+y, x+y-z, scale, chamfer, 0));        // Rear-Bottom-Right
        faces.push_back(::createCornerFace(-x-y, -x+y-z, scale, chamfer, 0));       // Rear-Bottom-Left
        cornerFaces = 8;

        // Create 12 edge faces (quadrilaterals)
        // Edge 0-3: X-axis edges
        faces.push_back(::createEdgeFace(x, z-y, scale, chamfer, 0, 0));             // Front-Top
        faces.push_back(::createEdgeFace(x, -z-y, scale, chamfer, 0, 1));            // Front-Bottom
        faces.push_back(::createEdgeFace(x, -y+z, scale, chamfer, M_PI, 2));          // Rear-Bottom
        faces.push_back(::createEdgeFace(x, -y-z, scale, chamfer, M_PI, 3));          // Rear-Top
        
        // Edge 4-7: Z-axis edges  
        faces.push_back(::createEdgeFace(z, x+y, scale, chamfer, M_PI/2, 4));         // Rear-Right
        faces.push_back(::createEdgeFace(z, x-y, scale, chamfer, M_PI/2, 5));         // Front-Right
        faces.push_back(::createEdgeFace(z, -x-y, scale, chamfer, M_PI/2, 6));        // Front-Left
        faces.push_back(::createEdgeFace(z, y-x, scale, chamfer, M_PI/2, 7));         // Rear-Left
        
        // Edge 8-11: Y-axis edges
        faces.push_back(::createEdgeFace(y, -z+x, scale, chamfer, M_PI, 8));          // Top-Left
        faces.push_back(::createEdgeFace(y, x+z, scale, chamfer, 0, 9));              // Top-Right
        faces.push_back(::createEdgeFace(y, x-z, scale, chamfer, 0, 10));              // Bottom-Right
        faces.push_back(::createEdgeFace(y, z+x, scale, chamfer, M_PI, 11));           // Bottom-Left
        edgeFaces = 12;


    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in createRhombicuboctahedronFaces: " + std::string(e.what()));
    }

    return faces;
}

// Create a main face (octagon)
TopoDS_Face createMainFace(const gp_Vec& xAxis, const gp_Vec& zAxis, double scale, double chamfer) {
    gp_Vec yAxis = xAxis.Crossed(zAxis); // Calculate y axis


    // Calculate octagon vertices based on CuteNavCube's implementation
    double x2_scale = 1.0 - chamfer * 2.0;
    double y2_scale = 1.0 - chamfer * 2.0;
    double x4_scale = 1.0 - chamfer * 4.0;
    double y4_scale = 1.0 - chamfer * 4.0;

    // Based on CuteNavCube's main face implementation
    std::vector<gp_Pnt> vertices = {
        gp_Pnt((zAxis * scale - xAxis * x2_scale * scale - yAxis * y4_scale * scale).XYZ()), // Bottom-left
        gp_Pnt((zAxis * scale - xAxis * x4_scale * scale - yAxis * y2_scale * scale).XYZ()), // Left
        gp_Pnt((zAxis * scale + xAxis * x4_scale * scale - yAxis * y2_scale * scale).XYZ()), // Top-left
        gp_Pnt((zAxis * scale + xAxis * x2_scale * scale - yAxis * y4_scale * scale).XYZ()), // Top
        gp_Pnt((zAxis * scale + xAxis * x2_scale * scale + yAxis * y4_scale * scale).XYZ()), // Top-right
        gp_Pnt((zAxis * scale + xAxis * x4_scale * scale + yAxis * y2_scale * scale).XYZ()), // Right
        gp_Pnt((zAxis * scale - xAxis * x4_scale * scale + yAxis * y2_scale * scale).XYZ()), // Bottom-right
        gp_Pnt((zAxis * scale - xAxis * x2_scale * scale + yAxis * y4_scale * scale).XYZ())  // Bottom
    };

    return ::createFaceFromVertices(vertices);
}

// Create a corner face (hexagon)
TopoDS_Face createCornerFace(const gp_Vec& xAxis, const gp_Vec& zAxis, double scale, double chamfer, double rotZ) {
    gp_Vec xAxisRot = xAxis;
    gp_Vec yAxis = xAxis.Crossed(zAxis);
    gp_Vec zAxisRot = zAxis;


    // Apply rotation if needed
    if (rotZ != 0) {
        gp_Trsf rotation;
        rotation.SetRotation(gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,0,1)), rotZ);
        xAxisRot.Transform(rotation);
        yAxis.Transform(rotation);
        zAxisRot.Transform(rotation);
    }

    // Calculate corner face vertices using 6 vertices like CuteNavCube
    // Based on CuteNavCube's corner face implementation - creates a hexagon
    double zDepth = 1.0 - 2.0 * chamfer;

    // Create 6 vertices for hexagon in correct counter-clockwise order for outward normal
    std::vector<gp_Pnt> vertices = {
        gp_Pnt((zAxisRot * zDepth * scale - xAxisRot * 2 * chamfer * scale).XYZ()),                    // Apex 1
        gp_Pnt((zAxisRot * zDepth * scale - xAxisRot * chamfer * scale - yAxis * chamfer * scale).XYZ()), // Base 1
        gp_Pnt((zAxisRot * zDepth * scale + xAxisRot * chamfer * scale - yAxis * chamfer * scale).XYZ()), // Base 2
        gp_Pnt((zAxisRot * zDepth * scale + xAxisRot * 2 * chamfer * scale).XYZ()),                    // Apex 2
        gp_Pnt((zAxisRot * zDepth * scale + xAxisRot * chamfer * scale + yAxis * chamfer * scale).XYZ()), // Base 3
        gp_Pnt((zAxisRot * zDepth * scale - xAxisRot * chamfer * scale + yAxis * chamfer * scale).XYZ())  // Base 4
    };

    return ::createFaceFromVertices(vertices);
}

// Create an edge face (quadrilateral)
TopoDS_Face createEdgeFace(const gp_Vec& xAxis, const gp_Vec& zAxis, double scale, double chamfer, double rotZ, int edgeIndex) {
    // Calculate yAxis before rotation, using -zAxis to match CuteNavCube convention
    gp_Vec yAxis = xAxis.Crossed(-zAxis);
    gp_Vec xAxisRot = xAxis;
    gp_Vec zAxisRot = zAxis;

    // Apply rotation to all axes
    if (rotZ != 0) {
        gp_Trsf rotation;
        rotation.SetRotation(gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,0,1)), rotZ);
        xAxisRot.Transform(rotation);
        zAxisRot.Transform(rotation);
        yAxis.Transform(rotation);
    }

    // Calculate quadrilateral vertices based on CuteNavCube's implementation
    double x4_scale = 1.0 - chamfer * 4.0;
    double zE_scale = 1.0 - chamfer;

    // Based on CuteNavCube's edge face implementation
    std::vector<gp_Pnt> vertices = {
        gp_Pnt((zAxisRot * zE_scale * scale - xAxisRot * x4_scale * scale - yAxis * chamfer * scale).XYZ()), // Bottom-left
        gp_Pnt((zAxisRot * zE_scale * scale + xAxisRot * x4_scale * scale - yAxis * chamfer * scale).XYZ()), // Top-left
        gp_Pnt((zAxisRot * zE_scale * scale + xAxisRot * x4_scale * scale + yAxis * chamfer * scale).XYZ()), // Top-right
        gp_Pnt((zAxisRot * zE_scale * scale - xAxisRot * x4_scale * scale + yAxis * chamfer * scale).XYZ())  // Bottom-right
    };


    return ::createFaceFromVertices(vertices);
}

// Helper function to create a face from vertices
TopoDS_Face createFaceFromVertices(const std::vector<gp_Pnt>& vertices) {
    if (vertices.size() < 3) {
        LOG_WRN_S("Not enough vertices to create face");
        return TopoDS_Face();
    }

    try {
        // Create wire from vertices in REVERSE order to ensure outward normals
        // OpenCascade uses right-hand rule: counter-clockwise vertex order = outward normal
        BRepBuilderAPI_MakeWire wireMaker;
        for (int i = static_cast<int>(vertices.size()) - 1; i >= 0; --i) {
            int next = (i - 1 + static_cast<int>(vertices.size())) % static_cast<int>(vertices.size());
            BRepBuilderAPI_MakeEdge edgeMaker(vertices[i], vertices[next]);
            if (edgeMaker.IsDone()) {
                wireMaker.Add(edgeMaker.Edge());
            }
        }

        if (!wireMaker.IsDone()) {
            LOG_WRN_S("Failed to create wire for face");
            return TopoDS_Face();
        }

        // Create face from wire
        BRepBuilderAPI_MakeFace faceMaker(wireMaker.Wire());
        if (faceMaker.IsDone()) {
            return faceMaker.Face();
        } else {
            LOG_WRN_S("Failed to create face from wire");
            return TopoDS_Face();
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in createFaceFromVertices: " + std::string(e.what()));
        return TopoDS_Face();
    }
}