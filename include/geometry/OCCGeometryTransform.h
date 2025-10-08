#pragma once

#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>

// Forward declarations
class SoTransform;

/**
 * @brief Geometry transformation properties
 * 
 * Manages position, rotation, and scale transformations
 */
class OCCGeometryTransform {
public:
    OCCGeometryTransform();
    virtual ~OCCGeometryTransform() = default;

    // Transform accessors
    gp_Pnt getPosition() const { return m_position; }
    virtual void setPosition(const gp_Pnt& position);

    void getRotation(gp_Vec& axis, double& angle) const { axis = m_rotationAxis; angle = m_rotationAngle; }
    virtual void setRotation(const gp_Vec& axis, double angle);

    double getScale() const { return m_scale; }
    virtual void setScale(double scale);

    // Coin3D transform node management
    SoTransform* getCoinTransform() const { return m_coinTransform; }
    void updateCoinTransform();

protected:
    gp_Pnt m_position;
    gp_Vec m_rotationAxis;
    double m_rotationAngle;
    double m_scale;

    SoTransform* m_coinTransform;
};
