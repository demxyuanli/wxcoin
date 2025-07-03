#include "GeometryFactory.h"
#include "GeometryObject.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "Command.h"
#include "Logger.h"
#include <memory>

class CreateGeometryCommand : public Command {
public:
    CreateGeometryCommand(GeometryObject* obj, SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel)
        : m_object(obj), m_root(root), m_treePanel(treePanel), m_propPanel(propPanel) {
    }

    void execute() override {
        m_root->addChild(m_object->getRoot());
        m_treePanel->addObject(m_object);
        m_propPanel->updateProperties(m_object);
        LOG_INF("Executed CreateGeometryCommand: " + m_object->getName());
    }

    void undo() override {
        m_root->removeChild(m_object->getRoot());
        m_treePanel->removeObject(m_object);
        m_propPanel->clearProperties();
        LOG_INF("Undid CreateGeometryCommand: " + m_object->getName());
    }

    void redo() override {
        execute();
        LOG_INF("Redid CreateGeometryCommand: " + m_object->getName());
    }

    std::string getDescription() const override {
        return "Create " + m_object->getName();
    }

private:
    GeometryObject* m_object;
    SoSeparator* m_root;
    ObjectTreePanel* m_treePanel;
    PropertyPanel* m_propPanel;
};

GeometryFactory::GeometryFactory(SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel, CommandManager* cmdManager)
    : m_root(root)
    , m_treePanel(treePanel)
    , m_propPanel(propPanel)
    , m_cmdManager(cmdManager)
{
    LOG_INF("GeometryFactory initializing");
}

GeometryFactory::~GeometryFactory() {
    LOG_INF("GeometryFactory destroying");
}

void GeometryFactory::createGeometry(const std::string& type, const SbVec3f& position) {
    GeometryObject* object = nullptr;
    if (type == "Box") {
        object = createBox(position);
    }
    else if (type == "Sphere") {
        object = createSphere(position);
    }
    else if (type == "Cylinder") {
        object = createCylinder(position);
    }
    else if (type == "Cone") {
        object = createCone(position);
    }
    else {
        LOG_ERR("Unknown geometry type: " + type);
        return;
    }

    if (object) {
        auto command = std::make_shared<CreateGeometryCommand>(object, m_root, m_treePanel, m_propPanel);
        m_cmdManager->executeCommand(command);
    }
}

GeometryObject* GeometryFactory::createBox(const SbVec3f& position) {
    Box* box = new Box(1.0f, 1.0f, 1.0f);
    box->setPosition(position);
    return box;
}

GeometryObject* GeometryFactory::createSphere(const SbVec3f& position) {
    Sphere* sphere = new Sphere(0.5f);
    sphere->setPosition(position);
    return sphere;
}

GeometryObject* GeometryFactory::createCylinder(const SbVec3f& position) {
    Cylinder* cylinder = new Cylinder(0.5f, 1.0f);
    cylinder->setPosition(position);
    return cylinder;
}

GeometryObject* GeometryFactory::createCone(const SbVec3f& position) {
    Cone* cone = new Cone(0.5f, 1.0f);
    cone->setPosition(position);
    return cone;
}