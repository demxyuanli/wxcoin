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
#include <BRepPrimAPI_MakeTorus.hxx>
#include <gp_Trsf.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
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
    LOG_INF_S("Getting Coin3D node for geometry: " + m_name + " - Node exists: " + (m_coinNode ? "yes" : "no") + " - Needs update: " + (m_coinNeedsUpdate ? "yes" : "no"));
    
    if (!m_coinNode || m_coinNeedsUpdate) {
        LOG_INF_S("Building Coin3D representation for geometry: " + m_name);
        buildCoinRepresentation();
    }
    
    LOG_INF_S("Returning Coin3D node for geometry: " + m_name + " - Node: " + (m_coinNode ? "valid" : "null"));
    return m_coinNode;
}

void OCCGeometry::regenerateMesh(const OCCMeshConverter::MeshParameters& params)
{
    buildCoinRepresentation(params);
}

void OCCGeometry::buildCoinRepresentation(const OCCMeshConverter::MeshParameters& params)
{
    LOG_INF_S("Building Coin3D representation for geometry: " + m_name);
    
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
    
    LOG_INF_S("Shape is null: " + std::string(m_shape.IsNull() ? "yes" : "no") + " for geometry: " + m_name);
    
    if (!m_shape.IsNull()) {
        LOG_INF_S("Creating mesh node for geometry: " + m_name);
        SoSeparator* meshNode = OCCMeshConverter::createCoinNode(m_shape, params, m_selected);
        if (meshNode) {
            m_coinNode->addChild(meshNode);
            LOG_INF_S("Successfully added mesh node to Coin3D representation for: " + m_name);
        } else {
            LOG_ERR_S("Failed to create mesh node for geometry: " + m_name);
        }
    } else {
        LOG_ERR_S("Shape is null, cannot create mesh node for geometry: " + m_name);
    }

    m_coinNeedsUpdate = false;
    LOG_INF_S("Finished building Coin3D representation for geometry: " + m_name);
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
    LOG_INF_S("Building OCCBox shape with dimensions: " + std::to_string(m_width) + " x " + std::to_string(m_height) + " x " + std::to_string(m_depth));
    
    try {
        // Use the simplest constructor that takes dimensions only
        BRepPrimAPI_MakeBox boxMaker(m_width, m_height, m_depth);
        boxMaker.Build();  
        LOG_INF_S("BRepPrimAPI_MakeBox created for OCCBox: " + m_name);
        
        if (boxMaker.IsDone()) {
            TopoDS_Shape shape = boxMaker.Shape();
            LOG_INF_S("Box shape created successfully for OCCBox: " + m_name + " - Shape is null: " + (shape.IsNull() ? "yes" : "no"));
            setShape(shape);
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeBox failed for OCCBox: " + m_name);
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Failed to create box: " + std::string(e.what()) + " for OCCBox: " + m_name);
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
    LOG_INF_S("Building OCCCylinder shape with radius: " + std::to_string(m_radius) + " height: " + std::to_string(m_height));
    
    try {
        // Use the simplest constructor that takes radius and height
        BRepPrimAPI_MakeCylinder cylinderMaker(m_radius, m_height);
        LOG_INF_S("BRepPrimAPI_MakeCylinder created for OCCCylinder: " + m_name);
        cylinderMaker.Build();
        if (cylinderMaker.IsDone()) {
            TopoDS_Shape shape = cylinderMaker.Shape();
            LOG_INF_S("Cylinder shape created successfully for OCCCylinder: " + m_name + " - Shape is null: " + (shape.IsNull() ? "yes" : "no"));
            setShape(shape);
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeCylinder failed for OCCCylinder: " + m_name);
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Failed to create cylinder: " + std::string(e.what()) + " for OCCCylinder: " + m_name);
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
    LOG_INF_S("Building OCCSphere shape with radius: " + std::to_string(m_radius));
    
    try {
        // Validate radius parameter
        if (m_radius <= 0.0) {
            LOG_ERR_S("Invalid radius for OCCSphere: " + m_name + " - radius: " + std::to_string(m_radius));
            return;
        }
        
        // Try simple constructor first, as in pythonocc examples
        BRepPrimAPI_MakeSphere sphereMaker(m_radius);
        sphereMaker.Build();
        LOG_INF_S("Simple BRepPrimAPI_MakeSphere created for OCCSphere: " + m_name);
        
        if (sphereMaker.IsDone()) {
            TopoDS_Shape shape = sphereMaker.Shape();
            if (!shape.IsNull()) {
                LOG_INF_S("Sphere shape created successfully with simple constructor for: " + m_name);
                setShape(shape);
                return;
            } else {
                LOG_ERR_S("Simple constructor returned null shape for: " + m_name);
            }
        } else {
            LOG_ERR_S("Simple BRepPrimAPI_MakeSphere failed (IsDone = false) for: " + m_name);
        }
        
        // Fallback to axis-based constructor
        LOG_INF_S("Falling back to axis-based constructor for OCCSphere: " + m_name);
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeSphere fallbackMaker(axis, m_radius);
        fallbackMaker.Build();
        if (fallbackMaker.IsDone()) {
            TopoDS_Shape fallbackShape = fallbackMaker.Shape();
            if (!fallbackShape.IsNull()) {
                LOG_INF_S("Fallback sphere creation successful for: " + m_name);
                setShape(fallbackShape);
            } else {
                LOG_ERR_S("Fallback also returned null shape for: " + m_name);
            }
        } else {
            LOG_ERR_S("Fallback BRepPrimAPI_MakeSphere failed for: " + m_name);
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Failed to create sphere: " + std::string(e.what()) + " for OCCSphere: " + m_name);
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
    LOG_INF_S("Building OCCCone shape with bottomRadius: " + std::to_string(m_bottomRadius) + " topRadius: " + std::to_string(m_topRadius) + " height: " + std::to_string(m_height));
    
    try {
        // Following OpenCASCADE examples: use gp_Ax2 for proper cone orientation
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        
        // Enhanced parameter validation and fallback mechanism
        double actualTopRadius = m_topRadius;
        
        // Log which constructor approach we're using
        if (actualTopRadius <= 0.001) {
            LOG_INF_S("Creating perfect cone (topRadius ~= 0) for: " + m_name);
        } else {
            LOG_INF_S("Creating truncated cone for: " + m_name);
        }
        
        // Always use axis-based constructor with proper parameter validation
        BRepPrimAPI_MakeCone coneMaker(axis, m_bottomRadius, actualTopRadius, m_height);
        LOG_INF_S("BRepPrimAPI_MakeCone created for OCCCone: " + m_name + 
                  " with params - bottomRadius: " + std::to_string(m_bottomRadius) + 
                  ", topRadius: " + std::to_string(actualTopRadius) + 
                  ", height: " + std::to_string(m_height));
        
        coneMaker.Build();
        
        if (coneMaker.IsDone()) {
            TopoDS_Shape shape = coneMaker.Shape();
            if (!shape.IsNull()) {
                LOG_INF_S("Cone shape created successfully for OCCCone: " + m_name);
                setShape(shape);
            } else {
                LOG_ERR_S("BRepPrimAPI_MakeCone returned null shape for OCCCone: " + m_name);
                
                // Fallback: try with small non-zero topRadius if it was exactly 0
                if (m_topRadius == 0.0 && actualTopRadius == 0.0) {
                    LOG_INF_S("Attempting fallback with small topRadius for perfect cone: " + m_name);
                    actualTopRadius = 0.001;
                    BRepPrimAPI_MakeCone fallbackMaker(axis, m_bottomRadius, actualTopRadius, m_height);
                    fallbackMaker.Build();
                    if (fallbackMaker.IsDone()) {
                        TopoDS_Shape fallbackShape = fallbackMaker.Shape();
                        if (!fallbackShape.IsNull()) {
                            LOG_INF_S("Fallback cone creation successful for: " + m_name);
                            setShape(fallbackShape);
                        } else {
                            LOG_ERR_S("Fallback cone creation also failed for: " + m_name);
                        }
                    } else {
                        LOG_ERR_S("Fallback BRepPrimAPI_MakeCone also failed for: " + m_name);
                    }
                }
            }
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeCone failed (IsDone = false) for OCCCone: " + m_name);
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Failed to create cone: " + std::string(e.what()) + " for OCCCone: " + m_name);
    }
}

// OCCTorus implementation
OCCTorus::OCCTorus(const std::string& name, double majorRadius, double minorRadius)
    : OCCGeometry(name), m_majorRadius(majorRadius), m_minorRadius(minorRadius)
{
    LOG_INF_S("Creating OCCTorus: " + name + " with major radius: " + std::to_string(majorRadius) + " minor radius: " + std::to_string(minorRadius));
    buildShape();
}

void OCCTorus::setDimensions(double majorRadius, double minorRadius)
{
    if (m_majorRadius != majorRadius || m_minorRadius != minorRadius) {
        m_majorRadius = majorRadius;
        m_minorRadius = minorRadius;
        LOG_INF_S("OCCTorus dimensions changed: " + m_name + " major: " + std::to_string(majorRadius) + " minor: " + std::to_string(minorRadius));
        buildShape();
        m_coinNeedsUpdate = true;
    }
}

void OCCTorus::getSize(double& majorRadius, double& minorRadius) const
{
    majorRadius = m_majorRadius;
    minorRadius = m_minorRadius;
}

void OCCTorus::buildShape()
{
    LOG_INF_S("Building OCCTorus shape with major radius: " + std::to_string(m_majorRadius) + " minor radius: " + std::to_string(m_minorRadius));
    
    try {
        // Validate parameters
        if (m_majorRadius <= 0.0 || m_minorRadius <= 0.0) {
            LOG_ERR_S("Invalid radii for OCCTorus: " + m_name + " - major: " + std::to_string(m_majorRadius) + " minor: " + std::to_string(m_minorRadius));
            return;
        }
        
        if (m_minorRadius >= m_majorRadius) {
            LOG_ERR_S("Invalid torus dimensions: minor radius must be less than major radius for " + m_name);
            return;
        }
        
        // Create torus using axis-based constructor for better control
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeTorus torusMaker(axis, m_majorRadius, m_minorRadius);
        torusMaker.Build();
        
        if (torusMaker.IsDone()) {
            TopoDS_Shape shape = torusMaker.Shape();
            if (!shape.IsNull()) {
                m_shape = shape;
                LOG_INF_S("OCCTorus shape created successfully: " + m_name);
            } else {
                LOG_ERR_S("OCCTorus shape is null after creation: " + m_name);
            }
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeTorus failed for: " + m_name);
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in OCCTorus::buildShape for " + m_name + ": " + e.what());
    }
}

// OCCTruncatedCylinder implementation
OCCTruncatedCylinder::OCCTruncatedCylinder(const std::string& name, double bottomRadius, double topRadius, double height)
    : OCCGeometry(name), m_bottomRadius(bottomRadius), m_topRadius(topRadius), m_height(height)
{
    LOG_INF_S("Creating OCCTruncatedCylinder: " + name + " with bottom radius: " + std::to_string(bottomRadius) + 
              " top radius: " + std::to_string(topRadius) + " height: " + std::to_string(height));
    buildShape();
}

void OCCTruncatedCylinder::setDimensions(double bottomRadius, double topRadius, double height)
{
    if (m_bottomRadius != bottomRadius || m_topRadius != topRadius || m_height != height) {
        m_bottomRadius = bottomRadius;
        m_topRadius = topRadius;
        m_height = height;
        LOG_INF_S("OCCTruncatedCylinder dimensions changed: " + m_name + 
                  " bottom: " + std::to_string(bottomRadius) + 
                  " top: " + std::to_string(topRadius) + 
                  " height: " + std::to_string(height));
        buildShape();
        m_coinNeedsUpdate = true;
    }
}

void OCCTruncatedCylinder::getSize(double& bottomRadius, double& topRadius, double& height) const
{
    bottomRadius = m_bottomRadius;
    topRadius = m_topRadius;
    height = m_height;
}

void OCCTruncatedCylinder::buildShape()
{
    LOG_INF_S("Building OCCTruncatedCylinder shape with bottom radius: " + std::to_string(m_bottomRadius) + 
              " top radius: " + std::to_string(m_topRadius) + " height: " + std::to_string(m_height));
    
    try {
        // Validate parameters
        if (m_bottomRadius <= 0.0 || m_topRadius <= 0.0 || m_height <= 0.0) {
            LOG_ERR_S("Invalid dimensions for OCCTruncatedCylinder: " + m_name + 
                      " - bottom: " + std::to_string(m_bottomRadius) + 
                      " top: " + std::to_string(m_topRadius) + 
                      " height: " + std::to_string(m_height));
            return;
        }
        
        // Create truncated cylinder using cone with different radii
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCone truncatedCylinderMaker(axis, m_bottomRadius, m_topRadius, m_height);
        truncatedCylinderMaker.Build();
        
        if (truncatedCylinderMaker.IsDone()) {
            TopoDS_Shape shape = truncatedCylinderMaker.Shape();
            if (!shape.IsNull()) {
                m_shape = shape;
                LOG_INF_S("OCCTruncatedCylinder shape created successfully: " + m_name);
            } else {
                LOG_ERR_S("OCCTruncatedCylinder shape is null after creation: " + m_name);
            }
        } else {
            LOG_ERR_S("BRepPrimAPI_MakeCone failed for OCCTruncatedCylinder: " + m_name);
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in OCCTruncatedCylinder::buildShape for " + m_name + ": " + e.what());
    }
}
