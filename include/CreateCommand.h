#ifndef CREATE_COMMAND_H
#define CREATE_COMMAND_H

#include "Command.h"
#include "GeometryObject.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"

class CreateCommand : public Command
{
public:
    CreateCommand(std::unique_ptr<GeometryObject> object, SoSeparator* objectRoot, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel);
    ~CreateCommand();

    void execute() override;
    void unexecute() override;
    std::string getDescription() const override;

private:
    std::unique_ptr<GeometryObject> m_object;
    SoSeparator* m_objectRoot;
    ObjectTreePanel* m_objectTree;
    PropertyPanel* m_propertyPanel;
};

#endif