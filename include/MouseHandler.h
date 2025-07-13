#pragma once

#include <wx/event.h>
#include <Inventor/SbLinear.h>
#include <string>
#include <memory>

class Canvas;
class ObjectTreePanel;
class PropertyPanel;
class CommandManager;
class NavigationController;
class OCCViewer;
class OCCGeometry;

class MouseHandler {
public:
    enum class OperationMode { VIEW, SELECT, CREATE };

    MouseHandler(Canvas* canvas, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel, CommandManager* commandManager);
    ~MouseHandler();

    void handleMouseButton(wxMouseEvent& event);
    void handleMouseMotion(wxMouseEvent& event);

    void setOperationMode(OperationMode mode);
    void setCreationGeometryType(const std::string& type);
    void setNavigationController(NavigationController* controller);

    std::string getCreationGeometryType() const { return m_creationGeometryType; }
    OperationMode getOperationMode() const { return m_operationMode; }

private:
    void handleGeometrySelection(wxMouseEvent& event);
    
    Canvas* m_canvas;
    ObjectTreePanel* m_objectTree;
    PropertyPanel* m_propertyPanel;
    CommandManager* m_commandManager;
    NavigationController* m_navigationController;
    OperationMode m_operationMode;
    std::string m_creationGeometryType;
    bool m_isDragging;
    wxPoint m_lastMousePos;
};