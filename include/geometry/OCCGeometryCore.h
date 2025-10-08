#pragma once

#include <string>
#include <memory>
#include <OpenCASCADE/TopoDS_Shape.hxx>

/**
 * @brief Core geometry data and basic operations
 * 
 * Contains the fundamental geometry data: shape, name, and file information
 */
class OCCGeometryCore {
public:
    OCCGeometryCore(const std::string& name);
    virtual ~OCCGeometryCore() = default;

    // Basic property accessors
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    const std::string& getFileName() const { return m_fileName; }
    void setFileName(const std::string& fileName) { m_fileName = fileName; }

    const TopoDS_Shape& getShape() const { return m_shape; }
    virtual void setShape(const TopoDS_Shape& shape);

    bool isValid() const { return !m_shape.IsNull(); }

protected:
    std::string m_name;
    std::string m_fileName;
    TopoDS_Shape m_shape;
};
