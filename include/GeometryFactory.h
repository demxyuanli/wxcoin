#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbVec3f.h>
#include <string>
#include <memory>

class ObjectTreePanel;
class PropertyPanel;
class CommandManager;
class GeometryObject;
class OCCGeometry;
class OCCViewer;

enum class GeometryType {
    COIN3D,     // Traditional Coin3D geometry
    OPENCASCADE // OpenCASCADE geometry
};

class GeometryFactory {
public:
    GeometryFactory(SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel, 
                   CommandManager* cmdManager, OCCViewer* occViewer = nullptr);
    ~GeometryFactory();

    // Create geometry with specified type system
    void createGeometry(const std::string& type, const SbVec3f& position, GeometryType geomType = GeometryType::COIN3D);
    
    // Create OpenCASCADE geometry
    void createOCCGeometry(const std::string& type, const SbVec3f& position);
    
    // Set default geometry type
    void setDefaultGeometryType(GeometryType type) { m_defaultGeometryType = type; }
    GeometryType getDefaultGeometryType() const { return m_defaultGeometryType; }

private:
    
    // OpenCASCADE geometry creation
    std::shared_ptr<OCCGeometry> createOCCBox(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCSphere(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCCylinder(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCCone(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCTorus(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCTruncatedCylinder(const SbVec3f& position);
    std::shared_ptr<OCCGeometry> createOCCWrench(const SbVec3f& position);

    SoSeparator* m_root;
    ObjectTreePanel* m_treePanel;
    PropertyPanel* m_propPanel;
    CommandManager* m_cmdManager;
    OCCViewer* m_occViewer;
    GeometryType m_defaultGeometryType;
};