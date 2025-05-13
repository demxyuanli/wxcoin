#ifndef MAIN_FRAME_H
#define MAIN_FRAME_H

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include "Canvas.h"
#include "MouseHandler.h"
#include "Command.h"

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);
    virtual ~MainFrame();

private:
    wxAuiManager m_auiManager;
    Canvas* m_canvas;
    MouseHandler* m_mouseHandler;
    CommandManager* m_commandManager;
    
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createPanels();
    
    void onNew(wxCommandEvent& event);
    void onOpen(wxCommandEvent& event);
    void onSave(wxCommandEvent& event);
    void onSaveAs(wxCommandEvent& event);
    void onExit(wxCommandEvent& event);
    
    void onUndo(wxCommandEvent& event);
    void onRedo(wxCommandEvent& event);
    
    void onCreateBox(wxCommandEvent& event);
    void onCreateSphere(wxCommandEvent& event);
    void onCreateCylinder(wxCommandEvent& event);
    void onCreateCone(wxCommandEvent& event);
    
    void onViewMode(wxCommandEvent& event);
    void onSelectMode(wxCommandEvent& event);
    
    void onViewAll(wxCommandEvent& event);
    void onViewTop(wxCommandEvent& event);
    void onViewFront(wxCommandEvent& event);
    void onViewRight(wxCommandEvent& event);
    void onViewIsometric(wxCommandEvent& event);
    
    void onAbout(wxCommandEvent& event);
    
    void updateUI();
    
    wxDECLARE_EVENT_TABLE();
};

#endif // MAIN_FRAME_H