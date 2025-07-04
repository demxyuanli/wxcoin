#include "GeometryFactory.h"
#include "GeometryObject.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "Command.h"
#include "CreateCommand.h"
#include "Logger.h"
#include <memory>

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
    std::unique_ptr<GeometryObject> object = nullptr;
    if (type == "Box") {
        object = std::unique_ptr<GeometryObject>(createBox(position));
    }
    else if (type == "Sphere") {
        object = std::unique_ptr<GeometryObject>(createSphere(position));
    }
    else if (type == "Cylinder") {
        object = std::unique_ptr<GeometryObject>(createCylinder(position));
    }
    else if (type == "Cone") {
        object = std::unique_ptr<GeometryObject>(createCone(position));
    }
    else {
        LOG_ERR("Unknown geometry type: " + type);
        return;
    }

    if (object) {
        auto command = std::make_shared<CreateCommand>(std::move(object), m_root, m_treePanel, m_propPanel);
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