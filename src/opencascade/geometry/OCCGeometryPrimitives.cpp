#include "OCCGeometry.h"
#include "logger/Logger.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <gp_Ax2.hxx>

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

            // Log the center of the created shape
            Bnd_Box box;
            BRepBndLib::Add(shape, box);
            if (!box.IsVoid()) {
                Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
            }
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
