#pragma once

#include "OCCMeshConverter.h"
#include <string>
#include <memory>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>

// Forward declarations
class SoSeparator;
class SoTransform;
class SoMaterial;
class OCCMeshConverter;

/**
 * @brief Base class for OpenCASCADE geometry objects
 */
class OCCGeometry {
public:
    OCCGeometry(const std::string& name);
    virtual ~OCCGeometry();

    // Property accessors
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    const TopoDS_Shape& getShape() const { return m_shape; }
    virtual void setShape(const TopoDS_Shape& shape);

    gp_Pnt getPosition() const { return m_position; }
    virtual void setPosition(const gp_Pnt& position);

    void getRotation(gp_Vec& axis, double& angle) const { axis = m_rotationAxis; angle = m_rotationAngle; }
    virtual void setRotation(const gp_Vec& axis, double angle);

    double getScale() const { return m_scale; }
    virtual void setScale(double scale);

    bool isVisible() const { return m_visible; }
    virtual void setVisible(bool visible);

    bool isSelected() const { return m_selected; }
    virtual void setSelected(bool selected);

    Quantity_Color getColor() const { return m_color; }
    virtual void setColor(const Quantity_Color& color);

    double getTransparency() const { return m_transparency; }
    virtual void setTransparency(double transparency);

    // Coin3D integration
    SoSeparator* getCoinNode();
    void regenerateMesh(const OCCMeshConverter::MeshParameters& params);

private:
    void buildCoinRepresentation(const OCCMeshConverter::MeshParameters& params = OCCMeshConverter::MeshParameters());

protected:
    std::string m_name;
    TopoDS_Shape m_shape;
    
    // Transform parameters
    gp_Pnt m_position;
    gp_Vec m_rotationAxis;
    double m_rotationAngle;
    double m_scale;
    
    // Display properties
    bool m_visible;
    bool m_selected;
    Quantity_Color m_color;
    double m_transparency;
    
    // Coin3D representation
    SoSeparator* m_coinNode;
    SoTransform* m_coinTransform;
    bool m_coinNeedsUpdate;
};

/**
 * @brief OpenCASCADE box geometry
 */
class OCCBox : public OCCGeometry {
public:
    OCCBox(const std::string& name, double width, double height, double depth);
    
    void setDimensions(double width, double height, double depth);
    void getSize(double& width, double& height, double& depth) const;

private:
    void buildShape();
    
    double m_width, m_height, m_depth;
};

/**
 * @brief OpenCASCADE cylinder geometry
 */
class OCCCylinder : public OCCGeometry {
public:
    OCCCylinder(const std::string& name, double radius, double height);
    
    void setDimensions(double radius, double height);
    void getSize(double& radius, double& height) const;

private:
    void buildShape();
    
    double m_radius, m_height;
};

/**
 * @brief OpenCASCADE sphere geometry
 */
class OCCSphere : public OCCGeometry {
public:
    OCCSphere(const std::string& name, double radius);
    
    void setRadius(double radius);
    double getRadius() const { return m_radius; }

private:
    void buildShape();
    
    double m_radius;
};

/**
 * @brief OpenCASCADE cone geometry
 */
class OCCCone : public OCCGeometry {
public:
    OCCCone(const std::string& name, double bottomRadius, double topRadius, double height);
    
    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

private:
    void buildShape();
    
    double m_bottomRadius, m_topRadius, m_height;
}; 