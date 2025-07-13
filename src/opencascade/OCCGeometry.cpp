#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "logger/Logger.h"
#include <limits>
#include <cmath>
#include <wx/gdicmn.h>

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
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoNode.h>

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
    LOG_INF_S("Creating OCC geometry: " + name);
}

OCCGeometry::~OCCGeometry()
{
    if (m_coinNode) {
        m_coinNode->unref();
    }
    LOG_INF_S("Destroyed OCC geometry: " + m_name);
}

void OCCGeometry::setShape(const TopoDS_Shape& shape)
{
    m_shape = shape;
    m_coinNeedsUpdate = true;
}

void OCCGeometry::setPosition(const gp_Pnt& position)
{
    m_position = position;
    if (m_coinTransform) {
        m_coinTransform->translation.setValue(
            static_cast<float>(m_position.X()),
            static_cast<float>(m_position.Y()),
            static_cast<float>(m_position.Z())
        );
    } else {
        m_coinNeedsUpdate = true;
    }
}

void OCCGeometry::setRotation(const gp_Vec& axis, double angle)
{
    m_rotationAxis = axis;
    m_rotationAngle = angle;
    if (m_coinTransform) {
        SbVec3f rot_axis(
            static_cast<float>(m_rotationAxis.X()),
            static_cast<float>(m_rotationAxis.Y()),
            static_cast<float>(m_rotationAxis.Z())
        );
        m_coinTransform->rotation.setValue(rot_axis, static_cast<float>(m_rotationAngle));
    } else {
        m_coinNeedsUpdate = true;
    }
}

void OCCGeometry::setScale(double scale)
{
    m_scale = scale;
    if (m_coinTransform) {
        m_coinTransform->scaleFactor.setValue(
            static_cast<float>(m_scale),
            static_cast<float>(m_scale),
            static_cast<float>(m_scale)
        );
    } else {
        m_coinNeedsUpdate = true;
    }
}

void OCCGeometry::setVisible(bool visible)
{
    m_visible = visible;
}

void OCCGeometry::setSelected(bool selected)
{
    if (m_selected != selected) {
        LOG_INF_S("Setting selection for geometry '" + m_name + "': " + (selected ? "true" : "false"));
        m_selected = selected;
        
        // Force rebuild of Coin3D representation to update edge colors
        m_coinNeedsUpdate = true;
        
        if (m_coinNode) {
            LOG_INF_S("Rebuilding Coin3D representation for selection change: " + m_name + ", selected: " + (selected ? "true" : "false"));
            
            // Rebuild the entire Coin3D representation to update edge colors
            buildCoinRepresentation();
            
            // Force a refresh of the scene to show the selection change
            m_coinNode->touch();
        } else {
            LOG_INF_S("Coin3D node not yet created for geometry: " + m_name);
        }
    }
}

void OCCGeometry::setColor(const Quantity_Color& color)
{
    m_color = color;
    if (m_coinNode) {
        // Find the material node and update it. It might not be at a fixed index.
        for (int i = 0; i < m_coinNode->getNumChildren(); ++i) {
            SoNode* child = m_coinNode->getChild(i);
            if (child && child->isOfType(SoMaterial::getClassTypeId())) {
                SoMaterial* material = static_cast<SoMaterial*>(child);
                material->diffuseColor.setValue(
                    static_cast<float>(m_color.Red()),
                    static_cast<float>(m_color.Green()),
                    static_cast<float>(m_color.Blue())
                );
                break; 
            }
        }
    }
}

void OCCGeometry::setTransparency(double transparency)
{
    m_transparency = transparency;
    if (m_coinNode) {
        // Find the material node and update it.
        for (int i = 0; i < m_coinNode->getNumChildren(); ++i) {
            SoNode* child = m_coinNode->getChild(i);
            if (child && child->isOfType(SoMaterial::getClassTypeId())) {
                SoMaterial* material = static_cast<SoMaterial*>(child);
                material->transparency.setValue(static_cast<float>(m_transparency));
                break;
            }
        }
    }
}

SoSeparator* OCCGeometry::getCoinNode()
{
    if (!m_coinNode || m_coinNeedsUpdate) {
        buildCoinRepresentation();
    }
    return m_coinNode;
}

void OCCGeometry::regenerateMesh(const OCCMeshConverter::MeshParameters& params)
{
    buildCoinRepresentation(params);
}

void OCCGeometry::buildCoinRepresentation(const OCCMeshConverter::MeshParameters& params)
{
    if (m_coinNode) {
        m_coinNode->removeAllChildren();
    } else {
        m_coinNode = new SoSeparator;
        m_coinNode->ref();
    }
    
    m_coinTransform = new SoTransform;
    m_coinTransform->translation.setValue(
        static_cast<float>(m_position.X()),
        static_cast<float>(m_position.Y()),
        static_cast<float>(m_position.Z())
    );
    
    if (m_rotationAngle != 0.0) {
        SbVec3f axis(
            static_cast<float>(m_rotationAxis.X()),
            static_cast<float>(m_rotationAxis.Y()),
            static_cast<float>(m_rotationAxis.Z())
        );
        m_coinTransform->rotation.setValue(axis, static_cast<float>(m_rotationAngle));
    }
    
    m_coinTransform->scaleFactor.setValue(
        static_cast<float>(m_scale),
        static_cast<float>(m_scale),
        static_cast<float>(m_scale)
    );
    m_coinNode->addChild(m_coinTransform);
    
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    hints->faceType = SoShapeHints::CONVEX;
    m_coinNode->addChild(hints);
    
    SoMaterial* material = new SoMaterial;
    
    // Surface material - always use normal appearance, selection is handled by edge color
    material->emissiveColor.setValue(0.2f, 0.2f, 0.2f);
    material->diffuseColor.setValue(
        static_cast<float>(m_color.Red()),
        static_cast<float>(m_color.Green()),
        static_cast<float>(m_color.Blue())
    );
    LOG_INF_S("Built Coin3D representation with normal surface appearance for: " + m_name);
    
    material->transparency.setValue(static_cast<float>(m_transparency));
    material->ambientColor.setValue(0.6f, 0.6f, 0.6f);
    material->specularColor.setValue(0.3f, 0.3f, 0.3f);
    material->shininess.setValue(0.4f);
    m_coinNode->addChild(material);

    static const unsigned char texData[16] = {
        173, 216, 230, 255,   173, 216, 230, 255,
        173, 216, 230, 255,   173, 216, 230, 255
    }; 
    SoTexture2* texture = new SoTexture2;
    texture->wrapS = SoTexture2::REPEAT;
    texture->wrapT = SoTexture2::REPEAT;
    texture->model = SoTexture2::REPLACE;
    texture->image.setValue(SbVec2s(2, 2), 4, texData);
    m_coinNode->addChild(texture);
    
    if (!m_shape.IsNull()) {
        SoSeparator* meshNode = OCCMeshConverter::createCoinNode(m_shape, params, m_selected);
        if (meshNode) {
            m_coinNode->addChild(meshNode);
        }
    }

    m_coinNeedsUpdate = false;
}

// All primitive classes (OCCBox, OCCCylinder, etc.) call setShape(),
// which sets the m_coinNeedsUpdate flag. The representation will be
// built on the next call to getCoinNode().

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
        LOG_ERR_S("Failed to create box: " + std::string(e.what()));
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
        LOG_ERR_S("Failed to create cylinder: " + std::string(e.what()));
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
        LOG_ERR_S("Failed to create sphere: " + std::string(e.what()));
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
        LOG_ERR_S("Failed to create cone: " + std::string(e.what()));
    }
}
