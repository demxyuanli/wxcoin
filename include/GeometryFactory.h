#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbVec3f.h>
#include <string>

class ObjectTreePanel;
class PropertyPanel;
class CommandManager;
class GeometryObject;

class GeometryFactory {
public:
    GeometryFactory(SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel, CommandManager* cmdManager);
    ~GeometryFactory();

    void createGeometry(const std::string& type, const SbVec3f& position);

private:
    GeometryObject* createBox(const SbVec3f& position);
    GeometryObject* createSphere(const SbVec3f& position);
    GeometryObject* createCylinder(const SbVec3f& position);
    GeometryObject* createCone(const SbVec3f& position);

    SoSeparator* m_root;
    ObjectTreePanel* m_treePanel;
    PropertyPanel* m_propPanel;
    CommandManager* m_cmdManager;
};