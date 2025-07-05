#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "Logger.h"

// OpenCASCADE includes
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <gp_Trsf.hxx>
#include <TopLoc_Location.hxx>
#include <BRepBuilderAPI_Transform.hxx>

// Coin3D includes
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>

// OCCGeometry base class implementation
OCCGeometry::OCCGeometry(const std::string& name)
    : m_name(name)
    , m_position(0, 0, 0)
    , m_rotationAxis(0, 0, 1)
    , m_rotationAngle(0.0)
    , m_scale(1.0)
    , m_visible(true)
    , m_selected(false)
    , m_color(0.7, 0.7, 0.7, Quantity_TOC_RGB)
    , m_transparency(0.0)
    , m_coinNode(nullptr)
    , m_coinTransform(nullptr)
    , m_coinNeedsUpdate(true)
{
    LOG_INF("Creating OCC geometry: " + name);
}

OCCGeometry::~OCCGeometry()
{
    if (m_coinNode) {
        m_coinNode->unref();
    }
    LOG_INF("Destroyed OCC geometry: " + m_name);
}

void OCCGeometry::setShape(const TopoDS_Shape& shape)
{
    m_shape = shape;
    m_coinNeedsUpdate = true;
    updateCoinRepresentation();
}

void OCCGeometry::setPosition(const gp_Pnt& position)
{
    m_position = position;
    m_coinNeedsUpdate = true;
    updateCoinRepresentation();
}

void OCCGeometry::setRotation(const gp_Vec& axis, double angle)
{
    m_rotationAxis = axis;
    m_rotationAngle = angle;
    m_coinNeedsUpdate = true;
    updateCoinRepresentation();
}

void OCCGeometry::setScale(double scale)
{
    m_scale = scale;
    m_coinNeedsUpdate = true;
    updateCoinRepresentation();
}

void OCCGeometry::setVisible(bool visible)
{
    m_visible = visible;
    if (m_coinNode) {
        // Update Coin3D visibility
        // This would typically be handled by the viewer
    }
}

void OCCGeometry::setSelected(bool selected)
{
    m_selected = selected;
    if (m_coinNode) {
        // Update Coin3D selection appearance
        // This would typically be handled by the viewer
    }
}

void OCCGeometry::setColor(const Quantity_Color& color)
{
    m_color = color;
    if (m_coinNode) {
        // Update Coin3D material
        updateCoinRepresentation();
    }
}

void OCCGeometry::setTransparency(double transparency)
{
    m_transparency = transparency;
    if (m_coinNode) {
        // Update Coin3D material
        updateCoinRepresentation();
    }
}

SoSeparator* OCCGeometry::getCoinNode()
{
    if (!m_coinNode) {
        buildCoinRepresentation();
    }
    return m_coinNode;
}

void OCCGeometry::updateCoinRepresentation()
{
    if (m_coinNeedsUpdate && m_coinNode) {
        buildCoinRepresentation();
        m_coinNeedsUpdate = false;
    }
}

void OCCGeometry::buildCoinRepresentation()
{
    if (m_coinNode) {
        m_coinNode->unref();
    }
    
    m_coinNode = new SoSeparator;
    m_coinNode->ref();
    
    // Create transform node
    m_coinTransform = new SoTransform;
    m_coinTransform->translation.setValue(
        static_cast<float>(m_position.X()),
        static_cast<float>(m_position.Y()),
        static_cast<float>(m_position.Z())
    );
    
    // Set rotation
    if (m_rotationAngle != 0.0) {
        SbVec3f axis(
            static_cast<float>(m_rotationAxis.X()),
            static_cast<float>(m_rotationAxis.Y()),
            static_cast<float>(m_rotationAxis.Z())
        );
        m_coinTransform->rotation.setValue(axis, static_cast<float>(m_rotationAngle));
    }
    
    // Set scale
    m_coinTransform->scaleFactor.setValue(
        static_cast<float>(m_scale),
        static_cast<float>(m_scale),
        static_cast<float>(m_scale)
    );
    
    // Create material
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(
        static_cast<float>(m_color.Red()),
        static_cast<float>(m_color.Green()),
        static_cast<float>(m_color.Blue())
    );
    material->transparency.setValue(static_cast<float>(m_transparency));
    
    m_coinNode->addChild(m_coinTransform);
    m_coinNode->addChild(material);
    
    // Convert OCC shape to Coin3D mesh
    if (!m_shape.IsNull()) {
        SoSeparator* meshNode = OCCMeshConverter::createCoinNode(m_shape);
        if (meshNode) {
            m_coinNode->addChild(meshNode);
        }
    }
}

void OCCGeometry::updateMesh()
{
    if (!m_shape.IsNull() && m_coinNode) {
        buildCoinRepresentation();
    }
}

// OCCBox implementation
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
    try {
        BRepPrimAPI_MakeBox boxMaker(m_width, m_height, m_depth);
        if (boxMaker.IsDone()) {
            setShape(boxMaker.Shape());
        }
    } catch (const std::exception& e) {
        LOG_ERR("Failed to create box: " + std::string(e.what()));
    }
}

// OCCCylinder implementation
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
    try {
        BRepPrimAPI_MakeCylinder cylinderMaker(m_radius, m_height);
        if (cylinderMaker.IsDone()) {
            setShape(cylinderMaker.Shape());
        }
    } catch (const std::exception& e) {
        LOG_ERR("Failed to create cylinder: " + std::string(e.what()));
    }
}

// OCCSphere implementation
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
    try {
        BRepPrimAPI_MakeSphere sphereMaker(m_radius);
        if (sphereMaker.IsDone()) {
            setShape(sphereMaker.Shape());
        }
    } catch (const std::exception& e) {
        LOG_ERR("Failed to create sphere: " + std::string(e.what()));
    }
}

// OCCCone implementation
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
    try {
        BRepPrimAPI_MakeCone coneMaker(m_bottomRadius, m_topRadius, m_height);
        if (coneMaker.IsDone()) {
            setShape(coneMaker.Shape());
        }
    } catch (const std::exception& e) {
        LOG_ERR("Failed to create cone: " + std::string(e.what()));
    }
} 