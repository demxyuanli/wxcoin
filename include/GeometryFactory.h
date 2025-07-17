#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbVec3f.h>
#include <string>
#include <memory>
#include <OpenCASCADE/TopoDS_Shape.hxx>

class SoSeparator;
class ObjectTreePanel;
class PropertyPanel;
class OCCViewer;
class OCCGeometry;
struct GeometryParameters; // Forward declaration

enum class GeometryType {
    COIN3D,     // Traditional Coin3D geometry
    OPENCASCADE // OpenCASCADE geometry
};

class GeometryFactory {
public:
    GeometryFactory(SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel, 
                   OCCViewer* occViewer);
    ~GeometryFactory();

    // Create geometry with default parameters
    void createOCCGeometry(const std::string& type, const SbVec3f& position);
    
    // Create geometry with custom parameters
    void createOCCGeometryWithParameters(const std::string& type, const SbVec3f& position, const GeometryParameters& params);

    // Individual geometry creation methods
    std::shared_ptr<OCCGeometry> createOCCBox(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCBox(const SbVec3f& position, double width, double height, double depth);
    
    std::shared_ptr<OCCGeometry> createOCCSphere(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCSphere(const SbVec3f& position, double radius);
    
    std::shared_ptr<OCCGeometry> createOCCCylinder(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCCylinder(const SbVec3f& position, double radius, double height);
    
    std::shared_ptr<OCCGeometry> createOCCCone(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCCone(const SbVec3f& position, double bottomRadius, double topRadius, double height);
    
    std::shared_ptr<OCCGeometry> createOCCTorus(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCTorus(const SbVec3f& position, double majorRadius, double minorRadius);
    
    std::shared_ptr<OCCGeometry> createOCCTruncatedCylinder(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCTruncatedCylinder(const SbVec3f& position, double bottomRadius, double topRadius, double height);
    
    std::shared_ptr<OCCGeometry> createOCCWrench(const SbVec3f& position);

    // Set default geometry type
    void setDefaultGeometryType(GeometryType type) { m_defaultGeometryType = type; }
    GeometryType getDefaultGeometryType() const { return m_defaultGeometryType; }

private:
    // Optimized shape creation methods for caching
    TopoDS_Shape createOCCBoxShape(const SbVec3f& position);
    TopoDS_Shape createOCCSphereShape(const SbVec3f& position);
    TopoDS_Shape createOCCCylinderShape(const SbVec3f& position);
    TopoDS_Shape createOCCConeShape(const SbVec3f& position);
    TopoDS_Shape createOCCTorusShape(const SbVec3f& position);
    TopoDS_Shape createOCCTruncatedCylinderShape(const SbVec3f& position);
    TopoDS_Shape createOCCWrenchShape(const SbVec3f& position);
    
    // Helper method to create geometry from cached shape
    std::shared_ptr<OCCGeometry> createGeometryFromShape(const std::string& type, const TopoDS_Shape& shape, const SbVec3f& position);
    
    SoSeparator* m_root;
    ObjectTreePanel* m_treePanel;
    PropertyPanel* m_propPanel;
    OCCViewer* m_occViewer;
    GeometryType m_defaultGeometryType;
};