#include <iostream>
#include <wx/wx.h>
#include <wx/button.h>
#include "docking/DockManager.h"
#include "docking/DockLayoutConfig.h"
#include "docking/DockContainerWidget.h"

// Simple test app to verify dock layout config dialog
class TestApp : public wxApp {
public:
    bool OnInit() override {
        wxFrame* frame = new wxFrame(nullptr, wxID_ANY, "Test Dock Config", 
                                    wxDefaultPosition, wxSize(800, 600));
        
        // Create a simple dock manager
        ads::DockManager* dockManager = new ads::DockManager(frame);
        
        // Create a button to test the dialog
        wxPanel* panel = new wxPanel(frame);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        wxButton* configButton = new wxButton(panel, wxID_ANY, "Configure Dock Layout");
        configButton->Bind(wxEVT_BUTTON, [dockManager, frame](wxCommandEvent&) {
            ads::DockLayoutConfig config = dockManager->getLayoutConfig();
            ads::DockLayoutConfigDialog dialog(frame, config, dockManager);
            
            if (dialog.ShowModal() == wxID_OK) {
                config = dialog.GetConfig();
                dockManager->setLayoutConfig(config);
                
                // Apply the configuration
                if (wxWindow* containerWidget = dockManager->containerWidget()) {
                    if (ads::DockContainerWidget* container = 
                        dynamic_cast<ads::DockContainerWidget*>(containerWidget)) {
                        container->applyLayoutConfig();
                    }
                }
                
                wxMessageBox("Configuration applied!", "Success");
            }
        });
        
        sizer->Add(configButton, 0, wxALL | wxCENTER, 20);
        panel->SetSizer(sizer);
        
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);