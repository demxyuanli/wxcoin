#include "CreateCommand.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "logger/Logger.h"

CreateCommand::CreateCommand(std::unique_ptr<GeometryObject> object, SoSeparator* objectRoot, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel)
	: m_object(std::move(object)), m_objectRoot(objectRoot), m_objectTree(objectTree), m_propertyPanel(propertyPanel)
{
}

CreateCommand::~CreateCommand()
{
	// unique_ptr handles memory management
}

void CreateCommand::execute()
{
	if (!m_object) return;
	LOG_INF_S("Executing CreateCommand for object: " + m_object->getName());
	m_objectRoot->addChild(m_object->getRoot());
	m_objectTree->addObject(m_object.get());
	m_propertyPanel->updateProperties(m_object.get());
	m_objectRoot->touch();
}

void CreateCommand::unexecute()
{
	if (!m_object) return;
	m_objectRoot->removeChild(m_object->getRoot());
	m_objectTree->removeObject(m_object.get());
	m_propertyPanel->clearProperties();
}

std::string CreateCommand::getDescription() const
{
	if (m_object) {
		return "Create " + m_object->getName();
	}
	return "Create Object";
}