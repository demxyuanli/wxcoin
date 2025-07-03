#include "CreateCommand.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "Logger.h"

CreateCommand::CreateCommand(GeometryObject* object, SoSeparator* objectRoot, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel)
    : m_object(object), m_objectRoot(objectRoot), m_objectTree(objectTree), m_propertyPanel(propertyPanel)
{
}

CreateCommand::~CreateCommand()
{
    delete m_object; // Ensure proper cleanup if needed
}

void CreateCommand::execute()
{
    LOG_INF("Executing CreateCommand for object: " + m_object->getName());
    m_objectRoot->addChild(m_object->getRoot());
    m_objectTree->addObject(m_object);
    m_propertyPanel->updateProperties(m_object);
    m_objectRoot->touch();
}

void CreateCommand::undo()
{
    m_objectRoot->removeChild(m_object->getRoot());
    m_objectTree->removeObject(m_object);
    m_propertyPanel->clearProperties();
}

void CreateCommand::redo()
{
    execute();
}