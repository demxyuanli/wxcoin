#pragma once

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/colordlg.h>
#include <wx/filedlg.h>
#include <wx/textctrl.h>
#include <wx/colour.h>
#include <wx/event.h>
#include <string>
#include <Inventor/SbVec3f.h>

// Forward declarations
class ObjectManager;
struct ObjectSettings;
class wxStaticText;
class wxButton;
class wxChoice;
class wxStaticBitmap;

class ObjectSettingsPanel : public wxPanel
{
public:
    ObjectSettingsPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~ObjectSettingsPanel();

    // Object selection
    void setObjectManager(ObjectManager* objectManager);
    void setPreviewCanvas(class PreviewCanvas* canvas);
    void setSelectedObject(int objectId);
    int getSelectedObject() const;
    void refreshObjectList();
    
    // Object material settings
    float getAmbient() const;
    float getDiffuse() const;
    float getSpecular() const;
    float getShininess() const;
    float getTransparency() const;
    wxColour getMaterialColor() const;
    
    // Object texture settings
    bool isTextureEnabled() const;
    int getTextureMode() const;
    float getTextureScale() const;
    std::string getTexturePath() const;
    
    // Object transform settings
    SbVec3f getPosition() const;
    SbVec3f getRotation() const;
    SbVec3f getScale() const;
    
    // Configuration methods
    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    void applySettingsToObject();
    
    // Button event handlers
    void OnObjectApply(wxCommandEvent& event);
    void OnObjectSave(wxCommandEvent& event);
    void OnObjectReset(wxCommandEvent& event);
    void OnObjectUndo(wxCommandEvent& event);
    void OnObjectRedo(wxCommandEvent& event);
    void OnObjectAutoApply(wxCommandEvent& event);
    
    // Helper methods
    void loadObjectSettings();
    void createDefaultObjectControls();
    void updateObjectSelectionVisual();
    void loadTexturePreview(const wxString& filePath);
    

    
    // Material presets
    void applyMaterialPreset(const std::string& presetName);
    void applyMaterialPresetToObject(int objectId, const std::string& presetName);
    void generateTextureFromColor();
    void updateMaterialPresetChoice(wxChoice* choice, const ObjectSettings& settings);
    void saveObjectSettingsToConfig(int objectId, const ObjectSettings& settings);

private:
    void createUI();
    wxSizer* createMaterialTab(wxWindow* parent);
    wxSizer* createTextureTab(wxWindow* parent);
    wxSizer* createTransformTab(wxWindow* parent);
    void bindEvents();
    
    // Event handlers
    void onMaterialChanged(wxCommandEvent& event);
    void onTextureChanged(wxCommandEvent& event);
    void onTransformChanged(wxCommandEvent& event);
    void onTextureFileSelected(wxCommandEvent& event);
    void onGenerateTexture(wxCommandEvent& event);
    void onColorPickerClicked(wxCommandEvent& event);
    
    // UI Components for Object Controls
    wxBoxSizer* m_objectControlsSizer;
    
    // Individual object controls (for the 3 objects)
    struct ObjectControl {
        wxButton* nameLabel; // Now it's a button for selection
        wxButton* colorButton;
        wxChoice* materialPresetChoice;
        int objectId;
    };
    std::vector<ObjectControl> m_objectControls;
    
    // UI Components for Material (detailed controls in tab)
    wxSlider* m_ambientSlider;
    wxSlider* m_diffuseSlider;
    wxSlider* m_specularSlider;
    wxSpinCtrl* m_shininessSpin;
    wxSlider* m_transparencySlider;
    
    // UI Components for Texture
    wxCheckBox* m_textureCheckBox;
    wxButton* m_generateTextureButton;
    wxChoice* m_textureModeChoice;
    wxSlider* m_textureScaleSlider;
    wxTextCtrl* m_texturePathText;
    wxButton* m_textureFileButton;
    wxStaticBitmap* m_texturePreview;  // Texture preview image
    wxButton* m_colorPickerButton;     // Color picker for texture generation
    
    // UI Components for Transform
    wxSpinCtrl* m_posXSpin;
    wxSpinCtrl* m_posYSpin;
    wxSpinCtrl* m_posZSpin;
    wxSpinCtrl* m_rotXSpin;
    wxSpinCtrl* m_rotYSpin;
    wxSpinCtrl* m_rotZSpin;
    wxSpinCtrl* m_scaleXSpin;
    wxSpinCtrl* m_scaleYSpin;
    wxSpinCtrl* m_scaleZSpin;
    
    // Tab notebook
    wxNotebook* m_notebook;
    
    // Data
    ObjectManager* m_objectManager;
    class PreviewCanvas* m_previewCanvas;
    int m_selectedObjectId;
    wxColour m_currentColor;
    
    // Object Settings buttons
    wxButton* m_objectApplyButton;
    wxButton* m_objectSaveButton;
    wxButton* m_objectResetButton;
    wxButton* m_objectUndoButton;
    wxButton* m_objectRedoButton;
    wxCheckBox* m_objectAutoApplyCheckBox;
    bool m_autoApplyEnabled;
    

    
    DECLARE_EVENT_TABLE()
}; 