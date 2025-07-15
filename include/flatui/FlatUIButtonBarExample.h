#pragma once

#include "flatui/FlatUIButtonBar.h"
#include <wx/wx.h>

// Example usage of the extended FlatUIButtonBar with new control types
class FlatUIButtonBarExample : public wxPanel
{
public:
    FlatUIButtonBarExample(wxWindow* parent);
    virtual ~FlatUIButtonBarExample() = default;

private:
    FlatUIButtonBar* m_buttonBar;
    
    // Example event handlers
    void OnToggleButton(wxCommandEvent& event);
    void OnCheckBox(wxCommandEvent& event);
    void OnRadioButton(wxCommandEvent& event);
    void OnChoice(wxCommandEvent& event);
    void OnNormalButton(wxCommandEvent& event);
    
    // Control IDs
    enum {
        ID_TOGGLE_WIREFRAME = 1000,
        ID_TOGGLE_LIGHTING,
        ID_CHECKBOX_SHOW_EDGES,
        ID_CHECKBOX_SHOW_VERTICES,
        ID_CHECKBOX_ENABLE_SHADOWS,
        ID_RADIO_VIEW_FRONT,
        ID_RADIO_VIEW_TOP,
        ID_RADIO_VIEW_RIGHT,
        ID_RADIO_VIEW_ISO,
        ID_CHOICE_RENDER_QUALITY,
        ID_CHOICE_MATERIAL,
        ID_BUTTON_RESET,
        ID_BUTTON_APPLY
    };

    DECLARE_EVENT_TABLE()
};

// Usage example:
/*
    // Create the button bar
    auto* buttonBar = new FlatUIButtonBar(this);
    
    // Add different types of controls
    buttonBar->AddToggleButton(ID_TOGGLE_WIREFRAME, "Wireframe", false, wxNullBitmap, "Toggle wireframe display");
    buttonBar->AddToggleButton(ID_TOGGLE_LIGHTING, "Lighting", true, wxNullBitmap, "Toggle lighting");
    
    buttonBar->AddSeparator();
    
    buttonBar->AddCheckBox(ID_CHECKBOX_SHOW_EDGES, "Show Edges", true, "Display object edges");
    buttonBar->AddCheckBox(ID_CHECKBOX_SHOW_VERTICES, "Show Vertices", false, "Display vertices");
    buttonBar->AddCheckBox(ID_CHECKBOX_ENABLE_SHADOWS, "Shadows", true, "Enable shadow rendering");
    
    buttonBar->AddSeparator();
    
    // Radio button group for view modes (group ID = 1)
    buttonBar->AddRadioButton(ID_RADIO_VIEW_FRONT, "Front", 1, false, "Front view");
    buttonBar->AddRadioButton(ID_RADIO_VIEW_TOP, "Top", 1, false, "Top view");
    buttonBar->AddRadioButton(ID_RADIO_VIEW_RIGHT, "Right", 1, false, "Right view");
    buttonBar->AddRadioButton(ID_RADIO_VIEW_ISO, "Isometric", 1, true, "Isometric view");
    
    buttonBar->AddSeparator();
    
    // Choice controls
    wxArrayString qualityChoices;
    qualityChoices.Add("Draft");
    qualityChoices.Add("Normal");
    qualityChoices.Add("High");
    qualityChoices.Add("Ultra");
    buttonBar->AddChoiceControl(ID_CHOICE_RENDER_QUALITY, "Quality", qualityChoices, 1, "Rendering quality");
    
    wxArrayString materialChoices;
    materialChoices.Add("Metal");
    materialChoices.Add("Plastic");
    materialChoices.Add("Glass");
    materialChoices.Add("Wood");
    buttonBar->AddChoiceControl(ID_CHOICE_MATERIAL, "Material", materialChoices, 0, "Material type");
    
    buttonBar->AddSeparator();
    
    // Regular buttons
    buttonBar->AddButton(ID_BUTTON_RESET, "Reset", wxNullBitmap, nullptr, "Reset all settings");
    buttonBar->AddButton(ID_BUTTON_APPLY, "Apply", wxNullBitmap, nullptr, "Apply changes");
    
    // Set custom colors for some controls
    buttonBar->SetButtonCustomColors(ID_BUTTON_APPLY, wxColour(0, 120, 0), *wxWHITE);
    buttonBar->SetButtonCustomColors(ID_BUTTON_RESET, wxColour(180, 0, 0), *wxWHITE);
    
    // Example of manipulating controls programmatically
    buttonBar->SetButtonChecked(ID_TOGGLE_LIGHTING, true);
    buttonBar->SetRadioGroupSelection(1, ID_RADIO_VIEW_ISO);
    buttonBar->SetChoiceSelection(ID_CHOICE_RENDER_QUALITY, 2);
*/ 