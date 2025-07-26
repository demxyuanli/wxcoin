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
class PositionBasicDialog;

class MouseHandler {
public:
    enum class OperationMode { VIEW, CREATE };

    MouseHandler(Canvas* canvas, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel, CommandManager* commandManager);
    ~MouseHandler();

    void handleMouseButton(wxMouseEvent& event);
    void handleMouseMotion(wxMouseEvent& event);
    
    // Position picking completion handler
    void OnPositionPicked(const SbVec3f& position);

    void setOperationMode(OperationMode mode);
    void setCreationGeometryType(const std::string& type);
    void setNavigationController(NavigationController* controller);

    std::string getCreationGeometryType() const { return m_creationGeometryType; }
    OperationMode getOperationMode() const { return m_operationMode; }

private:

    
    Canvas* m_canvas;
    ObjectTreePanel* m_objectTree;
    PropertyPanel* m_propertyPanel;
    CommandManager* m_commandManager;
    NavigationController* m_navigationController;
    OperationMode m_operationMode;
    std::string m_creationGeometryType;
    bool m_isDragging;
    wxPoint m_lastMousePos;
    PositionBasicDialog* m_currentPositionBasicDialog;
};