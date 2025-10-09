#pragma once

#include <string>
#include <OpenCASCADE/TopoDS_Shape.hxx>

/**
 * @brief Base class for primitive geometry shapes
 */
class OCCPrimitiveBase {
public:
    OCCPrimitiveBase(const std::string& name) : m_name(name) {}
    virtual ~OCCPrimitiveBase() = default;

    const std::string& getName() const { return m_name; }
    const TopoDS_Shape& getShape() const { return m_shape; }

protected:
    virtual void buildShape() = 0;
    
    std::string m_name;
    TopoDS_Shape m_shape;
};

/**
 * @brief OpenCASCADE box geometry
 */
class OCCBox : public OCCPrimitiveBase {
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
 * @brief OpenCASCADE cylinder geometry
 */
class OCCCylinder : public OCCPrimitiveBase {
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
 * @brief OpenCASCADE sphere geometry
 */
class OCCSphere : public OCCPrimitiveBase {
public:
    OCCSphere(const std::string& name, double radius);

    void setRadius(double radius);
    double getRadius() const { return m_radius; }

protected:
    void buildShape() override;

private:
    double m_radius;
};

/**
 * @brief OpenCASCADE cone geometry
 */
class OCCCone : public OCCPrimitiveBase {
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
 * @brief OpenCASCADE torus geometry
 */
class OCCTorus : public OCCPrimitiveBase {
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
 * @brief OpenCASCADE truncated cylinder geometry (frustum)
 */
class OCCTruncatedCylinder : public OCCPrimitiveBase {
public:
    OCCTruncatedCylinder(const std::string& name, double bottomRadius, double topRadius, double height);

    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

protected:
    void buildShape() override;

private:
    double m_bottomRadius, m_topRadius, m_height;
};
