#pragma once

#include <wx/frame.h>
#include <wx/aui/aui.h>
#include <memory>

class Canvas;
class PropertyPanel;
class ObjectTreePanel;
class MouseHandler;
class GeometryFactory;
class CommandManager;
class NavigationController;
class OCCViewer;
class CommandDispatcher;
class GeometryCommandListener;
class ViewCommandListener;
class FileCommandListener;

struct CommandResult;

enum
{
    // File menu IDs
    ID_NEW = wxID_HIGHEST + 1,
    ID_OPEN,
    ID_SAVE,
    ID_SAVE_AS,
    ID_IMPORT_STEP,
    
    // Create menu IDs
    ID_CREATE_BOX,
    ID_CREATE_SPHERE,
    ID_CREATE_CYLINDER,
    ID_CREATE_CONE,
    ID_CREATE_WRENCH,
    
    // View menu IDs
    ID_VIEW_ALL,
    ID_VIEW_TOP,
    ID_VIEW_FRONT,
    ID_VIEW_RIGHT,
    ID_VIEW_ISOMETRIC,
    ID_SHOW_NORMALS,
    ID_FIX_NORMALS,
    ID_VIEW_SHOWEDGES,
    
    // Edit menu IDs
    ID_UNDO,
    ID_REDO,
    
    // Navigation IDs
    ID_NAVIGATION_CUBE_CONFIG,
    ID_ZOOM_SPEED
};

/**
 * @brief Main application frame with command-pattern based menu and toolbar
 */
class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);
    virtual ~MainFrame();

private:
    /**
     * @brief Create status bar
     */
    void createStatusBar();
    
    /**
     * @brief Create UI panels
     */
    void createPanels();
    
    /**
     * @brief Create menu bar
     */
    void createMenu();
    
    /**
     * @brief Create toolbar
     */
    void createToolbar();
    
    /**
     * @brief Setup command system with dispatcher and listeners
     */
    void setupCommandSystem();
    
    /**
     * @brief Update UI state
     */
    void updateUI();

    // Unified command handler for all menu and toolbar events
    void onCommand(wxCommandEvent& event);
    
    // UI feedback handler for command results
    void onCommandFeedback(const CommandResult& result);
    
    // Window event handlers
    void onClose(wxCloseEvent& event);
    void onActivate(wxActivateEvent& event);
    
    /**
     * @brief Map wxWidgets event ID to command type string
     * @param eventId wxWidgets event ID
     * @return Command type string
     */
    std::string mapEventIdToCommandType(int eventId) const;

private:
    // UI components
    Canvas* m_canvas;
    MouseHandler* m_mouseHandler;
    GeometryFactory* m_geometryFactory;
    CommandManager* m_commandManager;
    OCCViewer* m_occViewer;
    wxAuiManager m_auiManager;
    bool m_isFirstActivate;
    
    // Command system components
    std::unique_ptr<CommandDispatcher> m_commandDispatcher;
    std::shared_ptr<GeometryCommandListener> m_geometryListener;
    std::shared_ptr<ViewCommandListener> m_viewListener;
    std::shared_ptr<FileCommandListener> m_fileListener;

    DECLARE_EVENT_TABLE()
};