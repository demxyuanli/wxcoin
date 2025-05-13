#ifndef CREATE_COMMAND_H
#define CREATE_COMMAND_H

#include "Command.h"
#include "GeometryObject.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"

class CreateCommand : public Command
{
public:
    CreateCommand(GeometryObject* object, SoSeparator* objectRoot, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel);
    ~CreateCommand();

    void execute() override;
    void undo() override;
    void redo() override;
    std::string getDescription() const override { return "Create " + m_object->getName(); }

private:
    GeometryObject* m_object;
    SoSeparator* m_objectRoot;
    ObjectTreePanel* m_objectTree;
    PropertyPanel* m_propertyPanel;
};

#endif