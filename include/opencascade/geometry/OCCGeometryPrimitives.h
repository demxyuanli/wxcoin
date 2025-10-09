#pragma once

#include "OCCGeometryCore.h"
#include <memory>

// Forward declarations
struct MeshParameters;

/**
 * @brief OCCBox - 长方体几何体
 */
class OCCBox : public OCCGeometryCore
{
public:
    OCCBox(const std::string& name, double width, double height, double depth);

    void setDimensions(double width, double height, double depth);
    void getSize(double& width, double& height, double& depth) const;

protected:
    void buildShape() override;

private:
    double m_width, m_height, m_depth;
};

/**
 * @brief OCCCylinder - 圆柱体几何体
 */
class OCCCylinder : public OCCGeometryCore
{
public:
    OCCCylinder(const std::string& name, double radius, double height);

    void setDimensions(double radius, double height);
    void getSize(double& radius, double& height) const;

protected:
    void buildShape() override;

private:
    double m_radius, m_height;
};

/**
 * @brief OCCSphere - 球体几何体
 */
class OCCSphere : public OCCGeometryCore
{
public:
    OCCSphere(const std::string& name, double radius);

    void setRadius(double radius);

protected:
    void buildShape() override;

private:
    double m_radius;
};

/**
 * @brief OCCCone - 圆锥体几何体
 */
class OCCCone : public OCCGeometryCore
{
public:
    OCCCone(const std::string& name, double bottomRadius, double topRadius, double height);

    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

protected:
    void buildShape() override;

private:
    double m_bottomRadius, m_topRadius, m_height;
};

/**
 * @brief OCCTorus - 圆环体几何体
 */
class OCCTorus : public OCCGeometryCore
{
public:
    OCCTorus(const std::string& name, double majorRadius, double minorRadius);

    void setDimensions(double majorRadius, double minorRadius);
    void getSize(double& majorRadius, double& minorRadius) const;

protected:
    void buildShape() override;

private:
    double m_majorRadius, m_minorRadius;
};

/**
 * @brief OCCTruncatedCylinder - 截断圆柱体几何体
 */
class OCCTruncatedCylinder : public OCCGeometryCore
{
public:
    OCCTruncatedCylinder(const std::string& name, double bottomRadius, double topRadius, double height);

    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

protected:
    void buildShape() override;

private:
    double m_bottomRadius, m_topRadius, m_height;
};
