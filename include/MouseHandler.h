#pragma once

#include <string>
#include <wx/event.h>
#include "Canvas.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "NavigationStyle.h"
#include "Command.h"
#include <Inventor/SbVec3f.h>
#include <ctime>

class MouseHandler
{
public:
    enum OperationMode
    {
        NAVIGATE,
        SELECT,
        CREATE
    };

    MouseHandler(Canvas* canvas, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel, CommandManager* commandManager);
    ~MouseHandler();

    void setOperationMode(OperationMode mode);
    void setCreationGeometryType(const std::string& type);
    void handleMouseButton(const wxMouseEvent& event);
    void handleMouseMotion(const wxMouseEvent& event);
    void createObject(const wxPoint& position);
    bool screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos);

    NavigationStyle* getNavigationStyle() { return m_navStyle; }
    void setNavigationStyle(NavigationStyle* navStyle) { m_navStyle = navStyle; }

    const std::string& getCreationGeometryType() const { return m_creationGeometryType; }

    void createGeometryAtPosition(const SbVec3f& position);

private:
    void selectObject(const wxPoint& position);

    Canvas* m_canvas;
    ObjectTreePanel* m_objectTree;
    PropertyPanel* m_propertyPanel;
    CommandManager* m_commandManager;
    NavigationStyle* m_navStyle;
    OperationMode m_opMode;
    std::string m_creationGeometryType;
    GeometryObject* m_previewObject = nullptr;
};