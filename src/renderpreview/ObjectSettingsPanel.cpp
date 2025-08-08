#include "renderpreview/ObjectSettingsPanel.h"
#include "renderpreview/ObjectManager.h"
#include "renderpreview/PreviewCanvas.h"
#include "config/ConfigManager.h"
#include "config/FontManager.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <Inventor/SbVec3f.h>

BEGIN_EVENT_TABLE(ObjectSettingsPanel, wxPanel)
END_EVENT_TABLE()

ObjectSettingsPanel::ObjectSettingsPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
    , m_objectManager(nullptr)
    , m_previewCanvas(nullptr)
    , m_selectedObjectId(-1)
    , m_currentColor(128, 128, 128)
    , m_autoApplyEnabled(false)

{
    LOG_INF_S("ObjectSettingsPanel::ObjectSettingsPanel: Initializing");
    
    // Initialize font manager
    FontManager& fontManager = FontManager::getInstance();
    fontManager.initialize();
    
    createUI();
    bindEvents();
    loadSettings();
    
    // Apply fonts to the entire panel and its children
    fontManager.applyFontToWindowAndChildren(this, "Default");
    
    // Apply specific fonts to buttons and static texts
    applySpecificFonts();
    
    // Create default object controls even without ObjectManager
    createDefaultObjectControls();
    
    LOG_INF_S("ObjectSettingsPanel::ObjectSettingsPanel: Initialized successfully");
}

ObjectSettingsPanel::~ObjectSettingsPanel()
{
    LOG_INF_S("ObjectSettingsPanel::~ObjectSettingsPanel: Destroying");
}

void ObjectSettingsPanel::createUI()
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Upper section: Direct object controls
    auto* upperSizer = new wxBoxSizer(wxVERTICAL);
    
    // Object controls section
    auto* objectControlsSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Object Controls");
    
    // Apply title font to StaticBoxSizer label
    if (objectControlsSizer->GetStaticBox()) {
        objectControlsSizer->GetStaticBox()->SetFont(FontManager::getInstance().getTitleFont());
    }
    
    // Create individual controls for each object
    m_objectControlsSizer = new wxBoxSizer(wxVERTICAL);
    objectControlsSizer->Add(m_objectControlsSizer, 1, wxEXPAND | wxALL, 4);
    
    upperSizer->Add(objectControlsSizer, 0, wxEXPAND | wxALL, 4);
    
    mainSizer->Add(upperSizer, 0, wxEXPAND | wxALL, 4);
    
    // Lower section: Notebook for object properties
    m_notebook = new wxNotebook(this, wxID_ANY);
    m_notebook->SetMinSize(wxSize(400, 400)); // Set minimum size for notebook
    
    // Create tab panels
    auto* materialPanel = new wxPanel(m_notebook, wxID_ANY);
    auto* texturePanel = new wxPanel(m_notebook, wxID_ANY);
    auto* transformPanel = new wxPanel(m_notebook, wxID_ANY);
    
    // Set up tab panels with their content
    materialPanel->SetSizer(createMaterialTab(materialPanel));
    texturePanel->SetSizer(createTextureTab(texturePanel));
    transformPanel->SetSizer(createTransformTab(transformPanel));
    
    // Add tabs to notebook
    m_notebook->AddPage(materialPanel, "Material");
    m_notebook->AddPage(texturePanel, "Texture");
    m_notebook->AddPage(transformPanel, "Transform");
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 4);
    
    // Add Object Settings buttons at the bottom
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Auto-apply checkbox
    m_objectAutoApplyCheckBox = new wxCheckBox(this, wxID_ANY, wxT("Auto"));
    m_objectAutoApplyCheckBox->Bind(wxEVT_CHECKBOX, &ObjectSettingsPanel::OnObjectAutoApply, this);
    buttonSizer->Add(m_objectAutoApplyCheckBox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
    
    // Apply button
    m_objectApplyButton = new wxButton(this, wxID_APPLY, wxT("Preview"));
    m_objectApplyButton->Bind(wxEVT_BUTTON, &ObjectSettingsPanel::OnObjectApply, this);
    buttonSizer->Add(m_objectApplyButton, 0, wxALL, 2);
    
    // Save button
    m_objectSaveButton = new wxButton(this, wxID_SAVE, wxT("Save"));
    m_objectSaveButton->Bind(wxEVT_BUTTON, &ObjectSettingsPanel::OnObjectSave, this);
    buttonSizer->Add(m_objectSaveButton, 0, wxALL, 2);
    
    // Reset button
    m_objectResetButton = new wxButton(this, wxID_RESET, wxT("Reset"));
    m_objectResetButton->Bind(wxEVT_BUTTON, &ObjectSettingsPanel::OnObjectReset, this);
    buttonSizer->Add(m_objectResetButton, 0, wxALL, 2);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    
    // Set minimum size to ensure all controls are visible
    SetMinSize(wxSize(500, 700));
    
    // Force layout update
    Layout();
    Refresh();
}



wxSizer* ObjectSettingsPanel::createMaterialTab(wxWindow* parent)
{
    auto* materialSizer = new wxBoxSizer(wxVERTICAL);
    
    // Note: Color and Material preset controls are now in the upper section
    // This tab contains only the detailed material properties
    
    // Ambient
    auto* ambientSizer = new wxBoxSizer(wxHORIZONTAL);
    ambientSizer->Add(new wxStaticText(parent, wxID_ANY, "Ambient:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_ambientSlider = new wxSlider(parent, wxID_ANY, 30, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    ambientSizer->Add(m_ambientSlider, 1, wxEXPAND);
    materialSizer->Add(ambientSizer, 0, wxEXPAND | wxALL, 4);
    
    // Diffuse
    auto* diffuseSizer = new wxBoxSizer(wxHORIZONTAL);
    diffuseSizer->Add(new wxStaticText(parent, wxID_ANY, "Diffuse:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_diffuseSlider = new wxSlider(parent, wxID_ANY, 70, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    diffuseSizer->Add(m_diffuseSlider, 1, wxEXPAND);
    materialSizer->Add(diffuseSizer, 0, wxEXPAND | wxALL, 4);
    
    // Specular
    auto* specularSizer = new wxBoxSizer(wxHORIZONTAL);
    specularSizer->Add(new wxStaticText(parent, wxID_ANY, "Specular:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_specularSlider = new wxSlider(parent, wxID_ANY, 50, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    specularSizer->Add(m_specularSlider, 1, wxEXPAND);
    materialSizer->Add(specularSizer, 0, wxEXPAND | wxALL, 4);
    
    // Shininess
    auto* shininessSizer = new wxBoxSizer(wxHORIZONTAL);
    shininessSizer->Add(new wxStaticText(parent, wxID_ANY, "Shininess:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_shininessSpin = new wxSpinCtrl(parent, wxID_ANY, "32", wxDefaultPosition, wxSize(100, -1), 
        wxSP_ARROW_KEYS, 1, 128, 32);
    shininessSizer->Add(m_shininessSpin, 0);
    materialSizer->Add(shininessSizer, 0, wxEXPAND | wxALL, 4);
    
    // Transparency
    auto* transparencySizer = new wxBoxSizer(wxHORIZONTAL);
    transparencySizer->Add(new wxStaticText(parent, wxID_ANY, "Transparency:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_transparencySlider = new wxSlider(parent, wxID_ANY, 0, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    transparencySizer->Add(m_transparencySlider, 1, wxEXPAND);
    materialSizer->Add(transparencySizer, 0, wxEXPAND | wxALL, 4);
    
    return materialSizer;
}

wxSizer* ObjectSettingsPanel::createTextureTab(wxWindow* parent)
{
    auto* textureSizer = new wxBoxSizer(wxVERTICAL);
    
    // Enable Texture
    m_textureCheckBox = new wxCheckBox(parent, wxID_ANY, "Enable Texture");
    textureSizer->Add(m_textureCheckBox, 0, wxALL, 4);
    
    // Texture File
    auto* fileSizer = new wxBoxSizer(wxHORIZONTAL);
    fileSizer->Add(new wxStaticText(parent, wxID_ANY, "Texture File:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_texturePathText = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1));
    m_textureFileButton = new wxButton(parent, wxID_ANY, "Browse", wxDefaultPosition, wxSize(60, -1));
    fileSizer->Add(m_texturePathText, 1, wxEXPAND | wxRIGHT, 4);
    fileSizer->Add(m_textureFileButton, 0);
    textureSizer->Add(fileSizer, 0, wxEXPAND | wxALL, 4);
    
    // Texture Preview
    auto* previewSizer = new wxBoxSizer(wxHORIZONTAL);
    previewSizer->Add(new wxStaticText(parent, wxID_ANY, "Preview:"), 0, wxALIGN_CENTER | wxRIGHT, 8);
    m_texturePreview = new wxStaticBitmap(parent, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize(128, 128));
    m_texturePreview->SetBackgroundColour(wxColour(240, 240, 240));
    previewSizer->Add(m_texturePreview, 0, wxALIGN_CENTER);
    textureSizer->Add(previewSizer, 0, wxEXPAND | wxALL, 4);
    
    // Generate texture from color
    auto* generateSizer = new wxBoxSizer(wxHORIZONTAL);
    generateSizer->Add(new wxStaticText(parent, wxID_ANY, "Generate from color:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_colorPickerButton = new wxButton(parent, wxID_ANY, "Pick Color", wxDefaultPosition, wxSize(80, -1));
    m_colorPickerButton->SetBackgroundColour(wxColour(255, 0, 0)); // Default red
    m_generateTextureButton = new wxButton(parent, wxID_ANY, "Generate 8x8", wxDefaultPosition, wxSize(80, -1));
    generateSizer->Add(m_colorPickerButton, 0, wxRIGHT, 4);
    generateSizer->Add(m_generateTextureButton, 0);
    textureSizer->Add(generateSizer, 0, wxEXPAND | wxALL, 4);
    
    // Texture Mode
    auto* modeSizer = new wxBoxSizer(wxHORIZONTAL);
    modeSizer->Add(new wxStaticText(parent, wxID_ANY, "Texture Mode:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_textureModeChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    m_textureModeChoice->Append("Replace");
    m_textureModeChoice->Append("Modulate");
    m_textureModeChoice->Append("Decal");
    m_textureModeChoice->Append("Blend");
    m_textureModeChoice->SetSelection(0);
    modeSizer->Add(m_textureModeChoice, 1, wxEXPAND);
    textureSizer->Add(modeSizer, 0, wxEXPAND | wxALL, 4);
    
    // Texture Scale
    auto* scaleSizer = new wxBoxSizer(wxHORIZONTAL);
    scaleSizer->Add(new wxStaticText(parent, wxID_ANY, "Texture Scale:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_textureScaleSlider = new wxSlider(parent, wxID_ANY, 100, 10, 500, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    scaleSizer->Add(m_textureScaleSlider, 1, wxEXPAND);
    textureSizer->Add(scaleSizer, 0, wxEXPAND | wxALL, 4);
    
    return textureSizer;
}

wxSizer* ObjectSettingsPanel::createTransformTab(wxWindow* parent)
{
    auto* transformSizer = new wxBoxSizer(wxVERTICAL);
    
    // Position
    auto* posGroup = new wxStaticBoxSizer(wxVERTICAL, parent, "Position");
    
    // Apply title font to StaticBoxSizer label
    if (posGroup->GetStaticBox()) {
        posGroup->GetStaticBox()->SetFont(FontManager::getInstance().getTitleFont());
    }
    
    auto* posXSizer = new wxBoxSizer(wxHORIZONTAL);
    posXSizer->Add(new wxStaticText(parent, wxID_ANY, "X:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_posXSpin = new wxSpinCtrl(parent, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, -1000, 1000, 0);
    posXSizer->Add(m_posXSpin, 0);
    posGroup->Add(posXSizer, 0, wxEXPAND | wxALL, 4);
    
    auto* posYSizer = new wxBoxSizer(wxHORIZONTAL);
    posYSizer->Add(new wxStaticText(parent, wxID_ANY, "Y:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_posYSpin = new wxSpinCtrl(parent, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, -1000, 1000, 0);
    posYSizer->Add(m_posYSpin, 0);
    posGroup->Add(posYSizer, 0, wxEXPAND | wxALL, 4);
    
    auto* posZSizer = new wxBoxSizer(wxHORIZONTAL);
    posZSizer->Add(new wxStaticText(parent, wxID_ANY, "Z:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_posZSpin = new wxSpinCtrl(parent, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, -1000, 1000, 0);
    posZSizer->Add(m_posZSpin, 0);
    posGroup->Add(posZSizer, 0, wxEXPAND | wxALL, 4);
    
    transformSizer->Add(posGroup, 0, wxEXPAND | wxALL, 4);
    
    // Rotation
    auto* rotGroup = new wxStaticBoxSizer(wxVERTICAL, parent, "Rotation");
    
    // Apply title font to StaticBoxSizer label
    if (rotGroup->GetStaticBox()) {
        rotGroup->GetStaticBox()->SetFont(FontManager::getInstance().getTitleFont());
    }
    
    auto* rotXSizer = new wxBoxSizer(wxHORIZONTAL);
    rotXSizer->Add(new wxStaticText(parent, wxID_ANY, "X:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_rotXSpin = new wxSpinCtrl(parent, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, -360, 360, 0);
    rotXSizer->Add(m_rotXSpin, 0);
    rotGroup->Add(rotXSizer, 0, wxEXPAND | wxALL, 4);
    
    auto* rotYSizer = new wxBoxSizer(wxHORIZONTAL);
    rotYSizer->Add(new wxStaticText(parent, wxID_ANY, "Y:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_rotYSpin = new wxSpinCtrl(parent, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, -360, 360, 0);
    rotYSizer->Add(m_rotYSpin, 0);
    rotGroup->Add(rotYSizer, 0, wxEXPAND | wxALL, 4);
    
    auto* rotZSizer = new wxBoxSizer(wxHORIZONTAL);
    rotZSizer->Add(new wxStaticText(parent, wxID_ANY, "Z:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_rotZSpin = new wxSpinCtrl(parent, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, -360, 360, 0);
    rotZSizer->Add(m_rotZSpin, 0);
    rotGroup->Add(rotZSizer, 0, wxEXPAND | wxALL, 4);
    
    transformSizer->Add(rotGroup, 0, wxEXPAND | wxALL, 4);
    
    // Scale
    auto* scaleGroup = new wxStaticBoxSizer(wxVERTICAL, parent, "Scale");
    
    // Apply title font to StaticBoxSizer label
    if (scaleGroup->GetStaticBox()) {
        scaleGroup->GetStaticBox()->SetFont(FontManager::getInstance().getTitleFont());
    }
    
    auto* scaleXSizer = new wxBoxSizer(wxHORIZONTAL);
    scaleXSizer->Add(new wxStaticText(parent, wxID_ANY, "X:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_scaleXSpin = new wxSpinCtrl(parent, wxID_ANY, "10", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, 1, 100, 10);
    scaleXSizer->Add(m_scaleXSpin, 0);
    scaleGroup->Add(scaleXSizer, 0, wxEXPAND | wxALL, 4);
    
    auto* scaleYSizer = new wxBoxSizer(wxHORIZONTAL);
    scaleYSizer->Add(new wxStaticText(parent, wxID_ANY, "Y:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_scaleYSpin = new wxSpinCtrl(parent, wxID_ANY, "10", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, 1, 100, 10);
    scaleYSizer->Add(m_scaleYSpin, 0);
    scaleGroup->Add(scaleYSizer, 0, wxEXPAND | wxALL, 4);
    
    auto* scaleZSizer = new wxBoxSizer(wxHORIZONTAL);
    scaleZSizer->Add(new wxStaticText(parent, wxID_ANY, "Z:"), 0, wxALIGN_CENTER | wxRIGHT, 4);
    m_scaleZSpin = new wxSpinCtrl(parent, wxID_ANY, "10", wxDefaultPosition, wxSize(80, -1), 
        wxSP_ARROW_KEYS, 1, 100, 10);
    scaleZSizer->Add(m_scaleZSpin, 0);
    scaleGroup->Add(scaleZSizer, 0, wxEXPAND | wxALL, 4);
    
    transformSizer->Add(scaleGroup, 0, wxEXPAND | wxALL, 4);
    
    return transformSizer;
}

void ObjectSettingsPanel::bindEvents()
{
    // Object selection events (now handled in refreshObjectList with individual controls)
    
    // Material events (for the detailed material tab)
    m_ambientSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    m_diffuseSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    m_specularSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    m_shininessSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onMaterialChanged, this);
    m_transparencySlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    
    // Texture events
    m_textureCheckBox->Bind(wxEVT_CHECKBOX, &ObjectSettingsPanel::onTextureChanged, this);
    m_generateTextureButton->Bind(wxEVT_BUTTON, &ObjectSettingsPanel::onGenerateTexture, this);
    m_colorPickerButton->Bind(wxEVT_BUTTON, &ObjectSettingsPanel::onColorPickerClicked, this);
    m_textureModeChoice->Bind(wxEVT_CHOICE, &ObjectSettingsPanel::onTextureChanged, this);
    m_textureScaleSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onTextureChanged, this);
    m_textureFileButton->Bind(wxEVT_BUTTON, &ObjectSettingsPanel::onTextureFileSelected, this);
    
    // Transform events
    m_posXSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_posYSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_posZSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_rotXSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_rotYSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_rotZSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_scaleXSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_scaleYSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
    m_scaleZSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onTransformChanged, this);
}

// Object selection methods
void ObjectSettingsPanel::setPreviewCanvas(PreviewCanvas* canvas)
{
    m_previewCanvas = canvas;
    LOG_INF_S("ObjectSettingsPanel::setPreviewCanvas: Canvas reference set");
}

void ObjectSettingsPanel::setObjectManager(ObjectManager* objectManager)
{
    LOG_INF_S("ObjectSettingsPanel::setObjectManager: Setting object manager");
    m_objectManager = objectManager;
    
    // Clear existing controls and recreate them with actual object data
    if (m_objectControlsSizer) {
        m_objectControlsSizer->Clear(true);
        m_objectControls.clear();
    }
    
    // Create controls based on actual objects from ObjectManager
    if (m_objectManager) {
        LOG_INF_S("ObjectSettingsPanel::setObjectManager: ObjectManager connected, refreshing object list");
        
        // Force refresh the object list
        refreshObjectList();
        
        // Select the first object by default if we have objects
        auto objectIds = m_objectManager->getAllObjectIds();
        LOG_INF_S("ObjectSettingsPanel::setObjectManager: Found " + std::to_string(objectIds.size()) + " objects");
        
        if (!objectIds.empty()) {
            m_selectedObjectId = objectIds[0];
            loadObjectSettings();
            updateObjectSelectionVisual();
            LOG_INF_S("ObjectSettingsPanel::setObjectManager: Selected first object with ID " + std::to_string(m_selectedObjectId));
        } else {
            LOG_WRN_S("ObjectSettingsPanel::setObjectManager: No objects found in ObjectManager");
            // Only create default controls if no controls exist
            if (m_objectControls.empty()) {
                createDefaultObjectControls();
            }
        }
    } else {
        LOG_WRN_S("ObjectSettingsPanel::setObjectManager: No ObjectManager provided");
        // Only create default controls if no controls exist
        if (m_objectControls.empty()) {
            createDefaultObjectControls();
        }
    }
    
    // Force layout update
    Layout();
    Refresh();
}

void ObjectSettingsPanel::setSelectedObject(int objectId)
{
    m_selectedObjectId = objectId;
    // Object selection is now handled by button bindings in the Light List style
    // No need to programmatically select items in the list
}

int ObjectSettingsPanel::getSelectedObject() const
{
    return m_selectedObjectId;
}

void ObjectSettingsPanel::createDefaultObjectControls()
{
    if (!m_objectControlsSizer) return;
    
    // Clear existing object controls
    m_objectControlsSizer->Clear(true);
    m_objectControls.clear();
    
    // Create controls for the three default objects
    std::vector<std::pair<std::string, wxColour>> defaultObjects = {
        {"Red Sphere", wxColour(255, 77, 77)},
        {"Green Cone", wxColour(77, 255, 77)},
        {"Blue Box", wxColour(77, 77, 255)}
    };
    
    for (size_t i = 0; i < defaultObjects.size(); ++i) {
        const auto& obj = defaultObjects[i];
        
        // Create horizontal sizer for this object
        auto* objectSizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Object name button (clickable)
        auto* nameButton = new wxButton(m_objectControlsSizer->GetContainingWindow(), wxID_ANY, obj.first, 
            wxDefaultPosition, wxSize(120, 30));
        nameButton->SetToolTip("Click to select this object");
        objectSizer->Add(nameButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        
        // Color button
        auto* colorButton = new wxButton(m_objectControlsSizer->GetContainingWindow(), wxID_ANY, "", 
            wxDefaultPosition, wxSize(40, 30));
        colorButton->SetBackgroundColour(obj.second);
        colorButton->SetToolTip("Click to change object color");
        objectSizer->Add(colorButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        
        // Material preset choice
        auto* materialPresetChoice = new wxChoice(m_objectControlsSizer->GetContainingWindow(), wxID_ANY, 
            wxDefaultPosition, wxSize(120, -1));
        materialPresetChoice->Append("Custom");
        materialPresetChoice->Append("Metal");
        materialPresetChoice->Append("Plastic");
        materialPresetChoice->Append("Glass");
        materialPresetChoice->Append("Wood");
        materialPresetChoice->Append("Ceramic");
        materialPresetChoice->SetSelection(0);
        objectSizer->Add(materialPresetChoice, 0, wxALIGN_CENTER_VERTICAL);
        
        // Bind name button click event (object selection)
        nameButton->Bind(wxEVT_BUTTON, [this, i, nameButton, colorButton, materialPresetChoice](wxCommandEvent& event) {
            // Select this object
            if (m_objectManager) {
                auto objectIds = m_objectManager->getAllObjectIds();
                if (i < objectIds.size()) {
                    int objectId = objectIds[i];
                    setSelectedObject(objectId);
                    loadObjectSettings();
                    
                    // Update visual selection (highlight selected button)
                    updateObjectSelectionVisual();
                    
                    // Update color button and material preset choice to reflect current object settings
                    auto settings = m_objectManager->getObjectSettings(objectId);
                    colorButton->SetBackgroundColour(settings.materialColor);
                    
                    // Update material preset choice based on current settings
                    updateMaterialPresetChoice(materialPresetChoice, settings);
                    
                    LOG_INF_S("ObjectSettingsPanel: Selected object " + std::to_string(objectId) + " and updated controls");
                }
            }
        });
        
        // Bind color button event
        colorButton->Bind(wxEVT_BUTTON, [this, i, colorButton, materialPresetChoice, nameButton](wxCommandEvent& event) {
            // Open color dialog for this object
            wxColourDialog dialog(this);
            if (dialog.ShowModal() == wxID_OK) {
                wxColour color = dialog.GetColourData().GetColour();
                
                // Update the object's color
                if (m_objectManager) {
                    auto objectIds = m_objectManager->getAllObjectIds();
                    if (i < objectIds.size()) {
                        int objectId = objectIds[i];
                        auto settings = m_objectManager->getObjectSettings(objectId);
                        settings.materialColor = color;
                        m_objectManager->updateObject(objectId, settings);
                        
                        // Update the color button
                        colorButton->SetBackgroundColour(color);
                        
                        // Reset material preset to "Custom" since we changed the color
                        materialPresetChoice->SetSelection(0);
                        
                        // Select this object (highlight the name button)
                        setSelectedObject(objectId);
                        updateObjectSelectionVisual();
                        
                        // Update the material tab
                        m_currentColor = color;
                        loadObjectSettings();
                        
                        // Force canvas refresh to show the color change
                        if (m_previewCanvas) {
                            m_previewCanvas->render(true);
                            m_previewCanvas->Refresh();
                            m_previewCanvas->Update();
                        }
                        
                        LOG_INF_S("ObjectSettingsPanel: Updated object " + std::to_string(objectId) + " color to RGB(" + 
                                 std::to_string(color.Red()) + "," + std::to_string(color.Green()) + "," + std::to_string(color.Blue()) + ")");
                    }
                }
            }
        });
        
        // Bind material preset choice event
        materialPresetChoice->Bind(wxEVT_CHOICE, [this, i, materialPresetChoice, colorButton, nameButton](wxCommandEvent& event) {
            int selection = materialPresetChoice->GetSelection();
            if (selection > 0) { // Skip "Custom"
                std::string presetName = materialPresetChoice->GetString(selection).ToStdString();
                if (m_objectManager) {
                    auto objectIds = m_objectManager->getAllObjectIds();
                    if (i < objectIds.size()) {
                        int objectId = objectIds[i];
                        applyMaterialPresetToObject(objectId, presetName);
                        
                        // Update color button to reflect the preset's color
                        auto settings = m_objectManager->getObjectSettings(objectId);
                        colorButton->SetBackgroundColour(settings.materialColor);
                        
                        // Select this object (highlight the name button)
                        setSelectedObject(objectId);
                        updateObjectSelectionVisual();
                        
                        // Update the material tab
                        loadObjectSettings();
                        
                        // Force canvas refresh to show the material change
                        if (m_previewCanvas) {
                            m_previewCanvas->render(true);
                            m_previewCanvas->Refresh();
                            m_previewCanvas->Update();
                        }
                        
                        LOG_INF_S("ObjectSettingsPanel: Applied " + presetName + " preset to object " + std::to_string(objectId));
                    }
                }
            }
        });
        
        // Store the controls
        ObjectControl control;
        control.nameLabel = nameButton; // Now it's a button, not a label
        control.colorButton = colorButton;
        control.materialPresetChoice = materialPresetChoice;
        control.objectId = static_cast<int>(i); // Use index as temporary ID
        m_objectControls.push_back(control);
        
        m_objectControlsSizer->Add(objectSizer, 0, wxEXPAND | wxALL, 4);
    }
    
    // Layout the sizer
    m_objectControlsSizer->Layout();
    m_objectControlsSizer->GetContainingWindow()->Layout();
    
    LOG_INF_S("ObjectSettingsPanel::createDefaultObjectControls: Created " + std::to_string(m_objectControls.size()) + " default object controls");
}

void ObjectSettingsPanel::refreshObjectList()
{
    if (!m_objectManager || !m_objectControlsSizer) {
        LOG_WRN_S("ObjectSettingsPanel::refreshObjectList: ObjectManager or sizer not available");
        return;
    }
    
    // Clear existing object controls
    m_objectControlsSizer->Clear(true);
    m_objectControls.clear();
    
    // Get all objects from ObjectManager
    auto objectIds = m_objectManager->getAllObjectIds();
    LOG_INF_S("ObjectSettingsPanel::refreshObjectList: Refreshing list with " + std::to_string(objectIds.size()) + " objects");
    
    if (objectIds.empty()) {
        LOG_WRN_S("ObjectSettingsPanel::refreshObjectList: No objects found, creating default controls");
        createDefaultObjectControls();
        return;
    }
    
    // Create controls for each actual object
    for (int objectId : objectIds) {
        ObjectSettings settings = m_objectManager->getObjectSettings(objectId);
        
        // Create horizontal sizer for this object
        auto* objectSizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Object name button (clickable)
        auto* nameButton = new wxButton(m_objectControlsSizer->GetContainingWindow(), wxID_ANY, settings.name, 
            wxDefaultPosition, wxSize(120, 30));
        nameButton->SetToolTip("Click to select this object");
        objectSizer->Add(nameButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        
        // Color button
        auto* colorButton = new wxButton(m_objectControlsSizer->GetContainingWindow(), wxID_ANY, "", 
            wxDefaultPosition, wxSize(40, 30));
        colorButton->SetBackgroundColour(settings.materialColor);
        colorButton->SetToolTip("Click to change object color");
        objectSizer->Add(colorButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        
        // Material preset choice
        auto* materialPresetChoice = new wxChoice(m_objectControlsSizer->GetContainingWindow(), wxID_ANY, 
            wxDefaultPosition, wxSize(120, -1));
        materialPresetChoice->Append("Custom");
        materialPresetChoice->Append("Metal");
        materialPresetChoice->Append("Plastic");
        materialPresetChoice->Append("Glass");
        materialPresetChoice->Append("Wood");
        materialPresetChoice->Append("Ceramic");
        materialPresetChoice->SetSelection(0);
        objectSizer->Add(materialPresetChoice, 0, wxALIGN_CENTER_VERTICAL);
        
        // Bind name button click event (object selection)
        nameButton->Bind(wxEVT_BUTTON, [this, objectId, nameButton, colorButton, materialPresetChoice](wxCommandEvent& event) {
            // Select this object
            setSelectedObject(objectId);
            loadObjectSettings();
            
            // Update visual selection (highlight selected button)
            updateObjectSelectionVisual();
            
            // Update color button and material preset choice to reflect current object settings
            auto settings = m_objectManager->getObjectSettings(objectId);
            colorButton->SetBackgroundColour(settings.materialColor);
            
            // Update material preset choice based on current settings
            updateMaterialPresetChoice(materialPresetChoice, settings);
            
            LOG_INF_S("ObjectSettingsPanel: Selected object " + std::to_string(objectId) + " and updated controls");
        });
        
        // Bind color button event
        colorButton->Bind(wxEVT_BUTTON, [this, objectId, colorButton, materialPresetChoice, nameButton](wxCommandEvent& event) {
            // Open color dialog for this object
            wxColourDialog dialog(this);
            if (dialog.ShowModal() == wxID_OK) {
                wxColour color = dialog.GetColourData().GetColour();
                
                // Update the object's color
                if (m_objectManager) {
                    auto settings = m_objectManager->getObjectSettings(objectId);
                    settings.materialColor = color;
                    m_objectManager->updateObject(objectId, settings);
                }
                
                // Update the color button
                colorButton->SetBackgroundColour(color);
                
                // Reset material preset to "Custom" since we changed the color
                materialPresetChoice->SetSelection(0);
                
                // Select this object (highlight the name button)
                setSelectedObject(objectId);
                updateObjectSelectionVisual();
                
                // Update the material tab
                m_currentColor = color;
                loadObjectSettings();
                
                // Force canvas refresh to show the color change
                if (m_previewCanvas) {
                    m_previewCanvas->render(true);
                    m_previewCanvas->Refresh();
                    m_previewCanvas->Update();
                }
                
                LOG_INF_S("ObjectSettingsPanel: Updated object " + std::to_string(objectId) + " color to RGB(" + 
                         std::to_string(color.Red()) + "," + std::to_string(color.Green()) + "," + std::to_string(color.Blue()) + ")");
            }
        });
        
        // Bind material preset choice event
        materialPresetChoice->Bind(wxEVT_CHOICE, [this, objectId, materialPresetChoice, colorButton, nameButton](wxCommandEvent& event) {
            int selection = materialPresetChoice->GetSelection();
            if (selection > 0) { // Skip "Custom"
                std::string presetName = materialPresetChoice->GetString(selection).ToStdString();
                applyMaterialPresetToObject(objectId, presetName);
                
                // Update color button to reflect the preset's color
                auto settings = m_objectManager->getObjectSettings(objectId);
                colorButton->SetBackgroundColour(settings.materialColor);
                
                // Select this object (highlight the name button)
                setSelectedObject(objectId);
                updateObjectSelectionVisual();
                
                // Update the material tab
                loadObjectSettings();
                
                // Force canvas refresh to show the material change
                if (m_previewCanvas) {
                    m_previewCanvas->render(true);
                    m_previewCanvas->Refresh();
                    m_previewCanvas->Update();
                }
                
                LOG_INF_S("ObjectSettingsPanel: Applied material preset '" + presetName + "' to object " + std::to_string(objectId));
            }
        });
        
        // Store control references
        ObjectControl control;
        control.nameLabel = nameButton;
        control.colorButton = colorButton;
        control.materialPresetChoice = materialPresetChoice;
        control.objectId = objectId;
        m_objectControls.push_back(control);
        
        // Add to sizer
        m_objectControlsSizer->Add(objectSizer, 0, wxEXPAND | wxALL, 4);
    }
    
    // Force layout update
    m_objectControlsSizer->Layout();
    Layout();
    Refresh();
    
    LOG_INF_S("ObjectSettingsPanel::refreshObjectList: Object list refreshed successfully");
}

// Material methods
float ObjectSettingsPanel::getAmbient() const
{
    return m_ambientSlider ? m_ambientSlider->GetValue() / 100.0f : 0.3f;
}

float ObjectSettingsPanel::getDiffuse() const
{
    return m_diffuseSlider ? m_diffuseSlider->GetValue() / 100.0f : 0.7f;
}

float ObjectSettingsPanel::getSpecular() const
{
    return m_specularSlider ? m_specularSlider->GetValue() / 100.0f : 0.5f;
}

float ObjectSettingsPanel::getShininess() const
{
    return m_shininessSpin ? static_cast<float>(m_shininessSpin->GetValue()) : 32.0f;
}

float ObjectSettingsPanel::getTransparency() const
{
    return m_transparencySlider ? m_transparencySlider->GetValue() / 100.0f : 0.0f;
}

wxColour ObjectSettingsPanel::getMaterialColor() const
{
    return m_currentColor;
}

// Texture methods
bool ObjectSettingsPanel::isTextureEnabled() const
{
    return m_textureCheckBox ? m_textureCheckBox->GetValue() : false;
}

int ObjectSettingsPanel::getTextureMode() const
{
    return m_textureModeChoice ? m_textureModeChoice->GetSelection() : 0;
}

float ObjectSettingsPanel::getTextureScale() const
{
    return m_textureScaleSlider ? m_textureScaleSlider->GetValue() / 100.0f : 1.0f;
}

std::string ObjectSettingsPanel::getTexturePath() const
{
    return m_texturePathText ? m_texturePathText->GetValue().ToStdString() : "";
}

// Transform methods
SbVec3f ObjectSettingsPanel::getPosition() const
{
    return SbVec3f(
        m_posXSpin ? static_cast<float>(m_posXSpin->GetValue()) / 10.0f : 0.0f,
        m_posYSpin ? static_cast<float>(m_posYSpin->GetValue()) / 10.0f : 0.0f,
        m_posZSpin ? static_cast<float>(m_posZSpin->GetValue()) / 10.0f : 0.0f
    );
}

SbVec3f ObjectSettingsPanel::getRotation() const
{
    return SbVec3f(
        m_rotXSpin ? static_cast<float>(m_rotXSpin->GetValue()) : 0.0f,
        m_rotYSpin ? static_cast<float>(m_rotYSpin->GetValue()) : 0.0f,
        m_rotZSpin ? static_cast<float>(m_rotZSpin->GetValue()) : 0.0f
    );
}

SbVec3f ObjectSettingsPanel::getScale() const
{
    return SbVec3f(
        m_scaleXSpin ? static_cast<float>(m_scaleXSpin->GetValue()) / 10.0f : 1.0f,
        m_scaleYSpin ? static_cast<float>(m_scaleYSpin->GetValue()) / 10.0f : 1.0f,
        m_scaleZSpin ? static_cast<float>(m_scaleZSpin->GetValue()) / 10.0f : 1.0f
    );
}



void ObjectSettingsPanel::onMaterialChanged(wxCommandEvent& event)
{
    if (m_autoApplyEnabled) {
        applySettingsToObject();
    }
}

void ObjectSettingsPanel::onTextureChanged(wxCommandEvent& event)
{
    if (m_autoApplyEnabled) {
        applySettingsToObject();
    }
}

void ObjectSettingsPanel::onTransformChanged(wxCommandEvent& event)
{
    if (m_autoApplyEnabled) {
        applySettingsToObject();
    }
}



void ObjectSettingsPanel::onTextureFileSelected(wxCommandEvent& event)
{
    if (event.GetEventObject() == m_textureFileButton) {
        wxFileDialog dialog(this, "Select Texture File", "", "", 
            "Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tga)|*.png;*.jpg;*.jpeg;*.bmp;*.tga|All files (*.*)|*.*",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        
        if (dialog.ShowModal() == wxID_OK) {
            wxString filePath = dialog.GetPath();
            m_texturePathText->SetValue(filePath);
            
            // Load and preview the texture
            loadTexturePreview(filePath);
            
            applySettingsToObject();
        }
    }
}

void ObjectSettingsPanel::onColorPickerClicked(wxCommandEvent& event)
{
    if (event.GetEventObject() == m_colorPickerButton) {
        wxColourDialog dialog(this);
        if (dialog.ShowModal() == wxID_OK) {
            wxColour color = dialog.GetColourData().GetColour();
            m_colorPickerButton->SetBackgroundColour(color);
            
            LOG_INF_S("ObjectSettingsPanel: Selected color for texture generation: RGB(" + 
                     std::to_string(color.Red()) + "," + std::to_string(color.Green()) + "," + std::to_string(color.Blue()) + ")");
        }
    }
}

void ObjectSettingsPanel::onGenerateTexture(wxCommandEvent& event)
{
    if (event.GetEventObject() == m_generateTextureButton) {
        generateTextureFromColor();
    }
}



void ObjectSettingsPanel::loadObjectSettings()
{
    if (!m_objectManager || m_selectedObjectId < 0) return;
    
    auto settings = m_objectManager->getObjectSettings(m_selectedObjectId);
    
    // Load material settings
    if (m_ambientSlider) m_ambientSlider->SetValue(static_cast<int>(settings.ambient * 100));
    if (m_diffuseSlider) m_diffuseSlider->SetValue(static_cast<int>(settings.diffuse * 100));
    if (m_specularSlider) m_specularSlider->SetValue(static_cast<int>(settings.specular * 100));
    if (m_shininessSpin) m_shininessSpin->SetValue(static_cast<int>(settings.shininess));
    if (m_transparencySlider) m_transparencySlider->SetValue(static_cast<int>(settings.transparency * 100));
    
    m_currentColor = settings.materialColor;
    

    

    
    // Load texture settings
    if (m_textureCheckBox) m_textureCheckBox->SetValue(settings.textureEnabled);
    if (m_textureModeChoice) m_textureModeChoice->SetSelection(static_cast<int>(settings.textureMode));
    if (m_textureScaleSlider) m_textureScaleSlider->SetValue(static_cast<int>(settings.textureScale * 100));
    if (m_texturePathText) m_texturePathText->SetValue(settings.texturePath);
    
    // Load texture preview if texture is enabled and path is not empty
    if (settings.textureEnabled && !settings.texturePath.empty()) {
        loadTexturePreview(settings.texturePath);
    } else {
        // Clear preview if no texture
        if (m_texturePreview) {
            m_texturePreview->SetBitmap(wxNullBitmap);
        }
    }
    
    // Load transform settings
    if (m_posXSpin) m_posXSpin->SetValue(static_cast<int>(settings.position[0] * 10));
    if (m_posYSpin) m_posYSpin->SetValue(static_cast<int>(settings.position[1] * 10));
    if (m_posZSpin) m_posZSpin->SetValue(static_cast<int>(settings.position[2] * 10));
    if (m_rotXSpin) m_rotXSpin->SetValue(static_cast<int>(settings.rotation[0]));
    if (m_rotYSpin) m_rotYSpin->SetValue(static_cast<int>(settings.rotation[1]));
    if (m_rotZSpin) m_rotZSpin->SetValue(static_cast<int>(settings.rotation[2]));
    if (m_scaleXSpin) m_scaleXSpin->SetValue(static_cast<int>(settings.scale[0] * 10));
    if (m_scaleYSpin) m_scaleYSpin->SetValue(static_cast<int>(settings.scale[1] * 10));
    if (m_scaleZSpin) m_scaleZSpin->SetValue(static_cast<int>(settings.scale[2] * 10));
}

void ObjectSettingsPanel::applySettingsToObject()
{
    if (!m_objectManager || m_selectedObjectId < 0) return;
    
    auto settings = m_objectManager->getObjectSettings(m_selectedObjectId);
    
    // Update material settings
    settings.ambient = getAmbient();
    settings.diffuse = getDiffuse();
    settings.specular = getSpecular();
    settings.shininess = getShininess();
    settings.transparency = getTransparency();
    settings.materialColor = getMaterialColor();
    
    // Update texture settings
    settings.textureEnabled = isTextureEnabled();
    settings.textureMode = static_cast<TextureMode>(getTextureMode());
    settings.textureScale = getTextureScale();
    settings.texturePath = getTexturePath();
    
    // Update transform settings
    settings.position = getPosition();
    settings.rotation = getRotation();
    settings.scale = getScale();
    
    m_objectManager->updateObject(m_selectedObjectId, settings);
    
    // Trigger canvas refresh - ensure this is called
    if (m_previewCanvas) {
        m_previewCanvas->render(true);
        m_previewCanvas->Refresh();
        m_previewCanvas->Update();
    }
    
    LOG_INF_S("ObjectSettingsPanel::applySettingsToObject: Applied settings to object " + std::to_string(m_selectedObjectId));
}

void ObjectSettingsPanel::loadSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString configPath = exeFile.GetPath() + "/render_preview_settings.ini";
        
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, configPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
        
        // Load auto apply setting
        m_autoApplyEnabled = renderConfig.ReadBool("Object.AutoApply", false);
        if (m_objectAutoApplyCheckBox) {
            m_objectAutoApplyCheckBox->SetValue(m_autoApplyEnabled);
        }
        
        // Load material settings
        if (m_ambientSlider) {
            m_ambientSlider->SetValue(renderConfig.ReadLong("Object.Material.Ambient", 30));
        }
        if (m_diffuseSlider) {
            m_diffuseSlider->SetValue(renderConfig.ReadLong("Object.Material.Diffuse", 70));
        }
        if (m_specularSlider) {
            m_specularSlider->SetValue(renderConfig.ReadLong("Object.Material.Specular", 50));
        }
        if (m_shininessSpin) {
            m_shininessSpin->SetValue(renderConfig.ReadLong("Object.Material.Shininess", 32));
        }
        if (m_transparencySlider) {
            m_transparencySlider->SetValue(renderConfig.ReadLong("Object.Material.Transparency", 0));
        }
        
        // Load texture settings
        if (m_textureCheckBox) {
            m_textureCheckBox->SetValue(renderConfig.ReadBool("Object.Texture.Enabled", false));
        }
        if (m_textureModeChoice) {
            m_textureModeChoice->SetSelection(renderConfig.ReadLong("Object.Texture.Mode", 0));
        }
        if (m_textureScaleSlider) {
            m_textureScaleSlider->SetValue(renderConfig.ReadLong("Object.Texture.Scale", 100));
        }
        
        LOG_INF_S("ObjectSettingsPanel::loadSettings: Settings loaded successfully from " + configPath.ToStdString());
    }
    catch (const std::exception& e) {
        LOG_ERR_S("ObjectSettingsPanel::loadSettings: Failed to load settings: " + std::string(e.what()));
    }
}

void ObjectSettingsPanel::saveSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString configPath = exeFile.GetPath() + "/render_preview_settings.ini";
        
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, configPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
        
        // Save material settings
        if (m_ambientSlider) {
            renderConfig.Write("Object.Material.Ambient", m_ambientSlider->GetValue());
        }
        if (m_diffuseSlider) {
            renderConfig.Write("Object.Material.Diffuse", m_diffuseSlider->GetValue());
        }
        if (m_specularSlider) {
            renderConfig.Write("Object.Material.Specular", m_specularSlider->GetValue());
        }
        if (m_shininessSpin) {
            renderConfig.Write("Object.Material.Shininess", m_shininessSpin->GetValue());
        }
        if (m_transparencySlider) {
            renderConfig.Write("Object.Material.Transparency", m_transparencySlider->GetValue());
        }
        
        // Save texture settings
        if (m_textureCheckBox) {
            renderConfig.Write("Object.Texture.Enabled", m_textureCheckBox->GetValue());
        }
        if (m_textureModeChoice) {
            renderConfig.Write("Object.Texture.Mode", m_textureModeChoice->GetSelection());
        }
        if (m_textureScaleSlider) {
            renderConfig.Write("Object.Texture.Scale", m_textureScaleSlider->GetValue());
        }
        
        renderConfig.Flush();
        LOG_INF_S("ObjectSettingsPanel::saveSettings: Settings saved successfully to " + configPath.ToStdString());
    }
    catch (const std::exception& e) {
        LOG_ERR_S("ObjectSettingsPanel::saveSettings: Failed to save settings: " + std::string(e.what()));
    }
}

void ObjectSettingsPanel::applyMaterialPreset(const std::string& presetName)
{
    if (!m_objectManager || m_selectedObjectId < 0) return;
    
    auto settings = m_objectManager->getObjectSettings(m_selectedObjectId);
    
    if (presetName == "Metal") {
        settings.ambient = 0.1f;
        settings.diffuse = 0.6f;
        settings.specular = 0.9f;
        settings.shininess = 96.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(192, 192, 192); // Silver
    }
    else if (presetName == "Plastic") {
        settings.ambient = 0.2f;
        settings.diffuse = 0.8f;
        settings.specular = 0.3f;
        settings.shininess = 32.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(128, 128, 128); // Gray
    }
    else if (presetName == "Glass") {
        settings.ambient = 0.1f;
        settings.diffuse = 0.3f;
        settings.specular = 0.9f;
        settings.shininess = 128.0f;
        settings.transparency = 0.7f;
        settings.materialColor = wxColour(200, 220, 255); // Light blue
    }
    else if (presetName == "Wood") {
        settings.ambient = 0.3f;
        settings.diffuse = 0.7f;
        settings.specular = 0.1f;
        settings.shininess = 16.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(139, 69, 19); // Brown
    }
    else if (presetName == "Ceramic") {
        settings.ambient = 0.2f;
        settings.diffuse = 0.6f;
        settings.specular = 0.8f;
        settings.shininess = 64.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(255, 255, 255); // White
    }
    
    // Update UI controls
    if (m_ambientSlider) m_ambientSlider->SetValue(static_cast<int>(settings.ambient * 100));
    if (m_diffuseSlider) m_diffuseSlider->SetValue(static_cast<int>(settings.diffuse * 100));
    if (m_specularSlider) m_specularSlider->SetValue(static_cast<int>(settings.specular * 100));
    if (m_shininessSpin) m_shininessSpin->SetValue(static_cast<int>(settings.shininess));
    if (m_transparencySlider) m_transparencySlider->SetValue(static_cast<int>(settings.transparency * 100));
    
    m_currentColor = settings.materialColor;
    
    // Apply to object
    m_objectManager->updateObject(m_selectedObjectId, settings);
}

void ObjectSettingsPanel::applyMaterialPresetToObject(int objectId, const std::string& presetName)
{
    if (!m_objectManager) return;
    
    auto settings = m_objectManager->getObjectSettings(objectId);
    
    if (presetName == "Metal") {
        settings.ambient = 0.1f;
        settings.diffuse = 0.6f;
        settings.specular = 0.9f;
        settings.shininess = 96.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(192, 192, 192); // Silver
    }
    else if (presetName == "Plastic") {
        settings.ambient = 0.2f;
        settings.diffuse = 0.8f;
        settings.specular = 0.3f;
        settings.shininess = 32.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(128, 128, 128); // Gray
    }
    else if (presetName == "Glass") {
        settings.ambient = 0.1f;
        settings.diffuse = 0.3f;
        settings.specular = 0.9f;
        settings.shininess = 128.0f;
        settings.transparency = 0.7f;
        settings.materialColor = wxColour(200, 220, 255); // Light blue
    }
    else if (presetName == "Wood") {
        settings.ambient = 0.3f;
        settings.diffuse = 0.7f;
        settings.specular = 0.1f;
        settings.shininess = 16.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(139, 69, 19); // Brown
    }
    else if (presetName == "Ceramic") {
        settings.ambient = 0.2f;
        settings.diffuse = 0.6f;
        settings.specular = 0.8f;
        settings.shininess = 64.0f;
        settings.transparency = 0.0f;
        settings.materialColor = wxColour(255, 255, 255); // White
    }
    
    // Update the color button for this object
    for (auto& control : m_objectControls) {
        if (control.objectId == objectId) {
            control.colorButton->SetBackgroundColour(settings.materialColor);
            break;
        }
    }
    
    // Apply to object
    m_objectManager->updateObject(objectId, settings);
    
    // Force canvas refresh to show the material change
    if (m_previewCanvas) {
        m_previewCanvas->render(true);
        m_previewCanvas->Refresh();
        m_previewCanvas->Update();
    }
}



void ObjectSettingsPanel::updateMaterialPresetChoice(wxChoice* choice, const ObjectSettings& settings)
{
    if (!choice) return;
    
    // Determine which preset matches the current settings
    // Metal preset
    if (settings.ambient == 0.1f && settings.diffuse == 0.6f && settings.specular == 0.9f && 
        settings.shininess == 96.0f && settings.transparency == 0.0f) {
        choice->SetSelection(1); // Metal
    }
    // Plastic preset
    else if (settings.ambient == 0.2f && settings.diffuse == 0.8f && settings.specular == 0.3f && 
             settings.shininess == 32.0f && settings.transparency == 0.0f) {
        choice->SetSelection(2); // Plastic
    }
    // Glass preset
    else if (settings.ambient == 0.1f && settings.diffuse == 0.3f && settings.specular == 0.9f && 
             settings.shininess == 128.0f && settings.transparency == 0.7f) {
        choice->SetSelection(3); // Glass
    }
    // Wood preset
    else if (settings.ambient == 0.3f && settings.diffuse == 0.7f && settings.specular == 0.1f && 
             settings.shininess == 16.0f && settings.transparency == 0.0f) {
        choice->SetSelection(4); // Wood
    }
    // Ceramic preset
    else if (settings.ambient == 0.2f && settings.diffuse == 0.6f && settings.specular == 0.8f && 
             settings.shininess == 64.0f && settings.transparency == 0.0f) {
        choice->SetSelection(5); // Ceramic
    }
    else {
        choice->SetSelection(0); // Custom
    }
}

void ObjectSettingsPanel::saveObjectSettingsToConfig(int objectId, const ObjectSettings& settings)
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString configPath = exeFile.GetPath() + "/render_preview_settings.ini";
        
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, configPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
        std::string prefix = "Object." + std::to_string(objectId) + ".";
        
        // Save material settings
        renderConfig.Write(prefix + "Material.Ambient", settings.ambient);
        renderConfig.Write(prefix + "Material.Diffuse", settings.diffuse);
        renderConfig.Write(prefix + "Material.Specular", settings.specular);
        renderConfig.Write(prefix + "Material.Shininess", settings.shininess);
        renderConfig.Write(prefix + "Material.Transparency", settings.transparency);
        renderConfig.Write(prefix + "Material.Color.Red", settings.materialColor.Red());
        renderConfig.Write(prefix + "Material.Color.Green", settings.materialColor.Green());
        renderConfig.Write(prefix + "Material.Color.Blue", settings.materialColor.Blue());
        
        // Save texture settings
        renderConfig.Write(prefix + "Texture.Enabled", settings.textureEnabled);
        renderConfig.Write(prefix + "Texture.Mode", static_cast<int>(settings.textureMode));
        renderConfig.Write(prefix + "Texture.Scale", settings.textureScale);
        renderConfig.Write(prefix + "Texture.Path", wxString(settings.texturePath));
        
        // Save transform settings
        renderConfig.Write(prefix + "Transform.Position.X", settings.position[0]);
        renderConfig.Write(prefix + "Transform.Position.Y", settings.position[1]);
        renderConfig.Write(prefix + "Transform.Position.Z", settings.position[2]);
        renderConfig.Write(prefix + "Transform.Rotation.X", settings.rotation[0]);
        renderConfig.Write(prefix + "Transform.Rotation.Y", settings.rotation[1]);
        renderConfig.Write(prefix + "Transform.Rotation.Z", settings.rotation[2]);
        renderConfig.Write(prefix + "Transform.Scale.X", settings.scale[0]);
        renderConfig.Write(prefix + "Transform.Scale.Y", settings.scale[1]);
        renderConfig.Write(prefix + "Transform.Scale.Z", settings.scale[2]);
        
        renderConfig.Flush();
        LOG_INF_S("ObjectSettingsPanel::saveObjectSettingsToConfig: Saved settings for object " + std::to_string(objectId) + " to " + configPath.ToStdString());
    }
    catch (const std::exception& e) {
        LOG_ERR_S("ObjectSettingsPanel::saveObjectSettingsToConfig: Failed to save settings for object " + std::to_string(objectId) + ": " + std::string(e.what()));
        throw;
    }
}

void ObjectSettingsPanel::generateTextureFromColor()
{
    if (!m_objectManager || m_selectedObjectId < 0) return;
    
    // Get the selected color from the color picker button
    wxColour color = m_colorPickerButton->GetBackgroundColour();
    
    // Generate an 8x8 texture image
    wxImage textureImage(8, 8);
    
    // Fill the image with the selected color
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            textureImage.SetRGB(x, y, color.Red(), color.Green(), color.Blue());
        }
    }
    
    // Create a simple pattern (checkerboard effect)
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            if ((x + y) % 2 == 0) {
                // Make every other pixel slightly darker
                wxColour darkerColor = color.ChangeLightness(80); // 80% of original lightness
                textureImage.SetRGB(x, y, darkerColor.Red(), darkerColor.Green(), darkerColor.Blue());
            }
        }
    }
    
    // Save the texture to a temporary file
    std::string texturePath = "generated_texture_" + std::to_string(m_selectedObjectId) + ".png";
    if (textureImage.SaveFile(texturePath, wxBITMAP_TYPE_PNG)) {
        // Update the texture path and enable texture
        if (m_texturePathText) {
            m_texturePathText->SetValue(texturePath);
        }
        if (m_textureCheckBox) {
            m_textureCheckBox->SetValue(true);
        }
        
        // Preview the generated texture
        loadTexturePreview(texturePath);
        
        // Apply settings
        applySettingsToObject();
        
        LOG_INF_S("ObjectSettingsPanel: Generated 8x8 texture from color RGB(" + 
                 std::to_string(color.Red()) + "," + std::to_string(color.Green()) + "," + std::to_string(color.Blue()) + ")");
    } else {
        LOG_ERR_S("ObjectSettingsPanel: Failed to save generated texture");
    }
}

void ObjectSettingsPanel::updateObjectSelectionVisual()
{
    // Update visual appearance of object name buttons to show selection
    for (auto& control : m_objectControls) {
        if (control.nameLabel) {
            if (control.objectId == m_selectedObjectId) {
                // Highlight selected object
                control.nameLabel->SetBackgroundColour(wxColour(100, 150, 255)); // Light blue
                control.nameLabel->SetForegroundColour(wxColour(255, 255, 255)); // White text
            } else {
                // Reset to default appearance
                control.nameLabel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
                control.nameLabel->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
            }
        }
    }
}

void ObjectSettingsPanel::loadTexturePreview(const wxString& filePath)
{
    if (!m_texturePreview) return;
    
    // Load the image
    wxImage image;
    if (image.LoadFile(filePath)) {
        // Scale the image to fit the preview area (128x128)
        wxImage scaledImage = image.Scale(128, 128, wxIMAGE_QUALITY_HIGH);
        
        // Create a bitmap from the scaled image
        wxBitmap bitmap(scaledImage);
        
        // Set the bitmap to the preview control
        m_texturePreview->SetBitmap(bitmap);
        
        LOG_INF_S("ObjectSettingsPanel: Loaded texture preview from " + filePath.ToStdString());
    } else {
        // Clear the preview if loading failed
        m_texturePreview->SetBitmap(wxNullBitmap);
        LOG_ERR_S("ObjectSettingsPanel: Failed to load texture preview from " + filePath.ToStdString());
    }
}

void ObjectSettingsPanel::resetToDefaults()
{
    // Reset material settings
    if (m_ambientSlider) {
        m_ambientSlider->SetValue(30);
    }
    if (m_diffuseSlider) {
        m_diffuseSlider->SetValue(70);
    }
    if (m_specularSlider) {
        m_specularSlider->SetValue(50);
    }
    if (m_shininessSpin) {
        m_shininessSpin->SetValue(32);
    }
    if (m_transparencySlider) {
        m_transparencySlider->SetValue(0);
    }
    
    // Reset texture settings
    if (m_textureCheckBox) {
        m_textureCheckBox->SetValue(false);
    }
    if (m_textureModeChoice) {
        m_textureModeChoice->SetSelection(0);
    }
    if (m_textureScaleSlider) {
        m_textureScaleSlider->SetValue(100);
    }
    

    
    LOG_INF_S("ObjectSettingsPanel::resetToDefaults: Settings reset to defaults");
}

// Object Settings button event handlers
void ObjectSettingsPanel::OnObjectApply(wxCommandEvent& event)
{
    // Apply only object settings (material, texture, transform)
    if (m_objectManager && m_selectedObjectId >= 0) {
        auto settings = m_objectManager->getObjectSettings(m_selectedObjectId);
        
        // Update material settings
        settings.ambient = getAmbient();
        settings.diffuse = getDiffuse();
        settings.specular = getSpecular();
        settings.shininess = getShininess();
        settings.transparency = getTransparency();
        settings.materialColor = getMaterialColor();
        
        // Update texture settings
        settings.textureEnabled = isTextureEnabled();
        settings.textureMode = static_cast<TextureMode>(getTextureMode());
        settings.textureScale = getTextureScale();
        settings.texturePath = getTexturePath();
        
        // Update transform settings
        settings.position = getPosition();
        settings.rotation = getRotation();
        settings.scale = getScale();
        
        // Apply to object manager
        m_objectManager->updateObject(m_selectedObjectId, settings);
        
        // Force canvas refresh
        if (m_previewCanvas) {
            m_previewCanvas->render(true);
            m_previewCanvas->Refresh();
            m_previewCanvas->Update();
        }
        
        LOG_INF_S("ObjectSettingsPanel::OnObjectApply: Object settings applied to object " + std::to_string(m_selectedObjectId));
    } else {
        wxMessageBox(wxT("No object selected. Please select an object first."), wxT("Apply Object"), wxOK | wxICON_WARNING);
        LOG_WRN_S("ObjectSettingsPanel::OnObjectApply: No object selected");
    }
}

void ObjectSettingsPanel::OnObjectSave(wxCommandEvent& event)
{
    if (!m_objectManager) {
        wxMessageBox(wxT("No object manager available. Cannot save settings."), wxT("Save Object"), wxOK | wxICON_WARNING);
        LOG_WRN_S("ObjectSettingsPanel::OnObjectSave: No object manager available");
        return;
    }
    
    try {
        // Save current object settings
        saveSettings();
        
        // Save all objects' settings to config
        auto objectIds = m_objectManager->getAllObjectIds();
        for (int objectId : objectIds) {
            auto settings = m_objectManager->getObjectSettings(objectId);
            saveObjectSettingsToConfig(objectId, settings);
        }
        
        wxMessageBox(wxT("All object settings saved successfully!"), wxT("Save Object"), wxOK | wxICON_INFORMATION);
        LOG_INF_S("ObjectSettingsPanel::OnObjectSave: All object settings saved successfully");
    }
    catch (const std::exception& e) {
        wxMessageBox(wxString::Format(wxT("Failed to save settings: %s"), e.what()), wxT("Save Object"), wxOK | wxICON_ERROR);
        LOG_ERR_S("ObjectSettingsPanel::OnObjectSave: Failed to save settings: " + std::string(e.what()));
    }
}

void ObjectSettingsPanel::OnObjectReset(wxCommandEvent& event)
{
    resetToDefaults();
    wxMessageBox(wxT("Object settings reset to defaults!"), wxT("Reset Object"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("ObjectSettingsPanel::OnObjectReset: Object settings reset");
}

void ObjectSettingsPanel::OnObjectAutoApply(wxCommandEvent& event)
{
    m_autoApplyEnabled = event.IsChecked();
    
    // Save auto apply setting to render_preview_settings.ini
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString configPath = exeFile.GetPath() + "/render_preview_settings.ini";
        
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, configPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
        renderConfig.Write("Object.AutoApply", m_autoApplyEnabled);
        renderConfig.Flush();
    }
    catch (const std::exception& e) {
        LOG_ERR_S("ObjectSettingsPanel::OnObjectAutoApply: Failed to save auto apply setting: " + std::string(e.what()));
    }
    
    LOG_INF_S("ObjectSettingsPanel::OnObjectAutoApply: Object auto apply " + std::string(m_autoApplyEnabled ? "enabled" : "disabled"));
}

void ObjectSettingsPanel::applySpecificFonts()
{
    FontManager& fontManager = FontManager::getInstance();
    
    // Apply button fonts
    if (m_objectApplyButton) {
        m_objectApplyButton->SetFont(fontManager.getButtonFont());
    }
    if (m_objectSaveButton) {
        m_objectSaveButton->SetFont(fontManager.getButtonFont());
    }
    if (m_objectResetButton) {
        m_objectResetButton->SetFont(fontManager.getButtonFont());
    }
    if (m_textureFileButton) {
        m_textureFileButton->SetFont(fontManager.getButtonFont());
    }
    if (m_colorPickerButton) {
        m_colorPickerButton->SetFont(fontManager.getButtonFont());
    }
    if (m_generateTextureButton) {
        m_generateTextureButton->SetFont(fontManager.getButtonFont());
    }
    
    // Apply fonts to all static texts and other controls in the panel
    wxWindowList& children = GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        if (child) {
            if (wxStaticText* staticText = dynamic_cast<wxStaticText*>(child)) {
                staticText->SetFont(fontManager.getLabelFont());
            }
            // Recursively apply to children of this child
            applyFontsToChildren(child, fontManager);
        }
    }
}

void ObjectSettingsPanel::applyFontsToChildren(wxWindow* parent, FontManager& fontManager)
{
    wxWindowList& children = parent->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        if (child) {
            if (wxStaticText* staticText = dynamic_cast<wxStaticText*>(child)) {
                staticText->SetFont(fontManager.getLabelFont());
            } else if (wxButton* button = dynamic_cast<wxButton*>(child)) {
                button->SetFont(fontManager.getButtonFont());
            } else if (wxChoice* choice = dynamic_cast<wxChoice*>(child)) {
                choice->SetFont(fontManager.getChoiceFont());
            } else if (wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>(child)) {
                textCtrl->SetFont(fontManager.getTextCtrlFont());
            } else if (wxCheckBox* checkBox = dynamic_cast<wxCheckBox*>(child)) {
                checkBox->SetFont(fontManager.getLabelFont());
            } else if (wxSlider* slider = dynamic_cast<wxSlider*>(child)) {
                slider->SetFont(fontManager.getLabelFont());
            } else if (wxSpinCtrl* spinCtrl = dynamic_cast<wxSpinCtrl*>(child)) {
                spinCtrl->SetFont(fontManager.getTextCtrlFont());
            } else if (wxSpinCtrlDouble* spinCtrlDouble = dynamic_cast<wxSpinCtrlDouble*>(child)) {
                spinCtrlDouble->SetFont(fontManager.getTextCtrlFont());
            }
            
            // Recursively apply to children of this child
            applyFontsToChildren(child, fontManager);
        }
    }
}

 