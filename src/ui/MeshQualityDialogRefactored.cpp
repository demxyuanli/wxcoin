#include "MeshQualityDialog.h"
#include "MeshParameterManager.h"
#include "logger/Logger.h"
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>

/**
 * Refactored MeshQualityDialog that uses MeshParameterManager
 * for unified parameter management and application to geometries
 */
class MeshQualityDialogRefactored : public wxDialog {
public:
    MeshQualityDialogRefactored(wxWindow* parent, const wxString& title)
        : wxDialog(parent, wxID_ANY, title), m_occViewer(nullptr), m_parameterCallbackId(0) {
        
        initializeParameterManager();
        createControls();
        bindEvents();
        loadCurrentParameters();
        syncUIControls();
    }

    ~MeshQualityDialogRefactored() {
        // Unregister callback
        MeshParameterManager::getInstance().unregisterParameterChangeCallback(m_parameterCallbackId);
    }

    void setOCCViewer(std::shared_ptr<OCCViewer> viewer) {
        m_occViewer = viewer;
        
        // Register callback for real-time parameter changes
        updateRealTimeCallback();
    }

private:
    std::shared_ptr<OCCViewer> m_occViewer;
    int m_parameterCallbackId;
    bool m_enableRealTimePreview;

    // UI Controls
    wxSpinCtrlDouble* m_deflectionSpinCtrl;
    wxSpinCtrlDouble* m_angularDeflectionSpinCtrl;
    wxCheckBox* m_subdivisionCheckBox;
    wxSpinCtrl* m_subdivisionLevelSpinCtrl;
    wxCheckBox* m_smoothingCheckBox;
    wxSpinCtrl* m_smoothingIterationsSpinCtrl;
    wxSlider* m_smoothingStrengthSlider;
    wxCheckBox* m_lodCheckBox;
    wxSpinCtrlDouble* m_lodRoughSpinCtrl;
    wxSpinCtrlDouble* m_lodFineSpinCtrl;
    wxCheckBox* m_parallelProcessingCheckBox;
    wxCheckBox* m_adaptiveMeshingCheckBox;
    wxCheckBox* m_realTimePreviewCheckBox;
    
    // Parameter Manager Reference
    MeshParameterManager& m_paramManager;

    void initializeParameterManager() {
        m_paramManager = MeshParameterManager::getInstance();
        
        // Register callback for parameter changes (for UI synchronization)
        m_parameterCallbackId = m_paramManager.registerParameterChangeCallback(
            [this](const MeshParameterManager::ParameterChange& change) {
                onParameterManagerChange(change);
            }
        );
        
        LOG_INF_S("MeshQualityDialogRefactored initialized with parameter manager");
    }

    void createControls() {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        auto* notebook = new wxNotebook(this, wxID_ANY);
        
        // === Basic Quality Tab ===
        auto* basicPanel = createBasicQualityPanel(notebook);
        notebook->AddPage(basicPanel, "Basic Quality");
        
        // === Advanced Settings Tab ===
        auto* advancedPanel = createAdvancedSettingsPanel(notebook);
        notebook->AddPage(advancedPanel, "Advanced");
        
        // === Performance Tab ===
        auto* performancePanel = createPerformancePanel(notebook);
        notebook->AddPage(performancePanel, "Performance");
        
        mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
        
        // Control buttons
        auto* buttonSizer = createControlButtons();
        mainSizer->Add(buttonSizer, 0, wxEXPAND | wxTOP, 5);
        
        SetSizer(mainSizer);
        mainSizer->SetSizeHints(this);
    }

    wxPanel* createBasicQualityPanel(wxNotebook* parent) {
        auto* panel = new wxPanel(parent);
        auto* sizer = new wxFlexGridSizer(2, wxSize(10, 5));
        sizer->AddGrowableCol(1);
        
        // Mesh deflection
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Mesh Deflection:"), 0, wxALIGN_CENTER_VERTICAL);
        m_deflectionSpinCtrl = new wxSpinCtrlDouble(panel, ID_DEFLECTION_SPIN);
        m_deflectionSpinCtrl->SetRange(0.001, 5.0);
        m_deflectionSpinCtrl->SetIncrement(0.01);
        m_deflectionSpinCtrl->SetDigits(3);
        sizer->Add(m_deflectionSpinCtrl, 1, wxEXPAND);
        
        // Angular deflection
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Angular Deflection:"), 0, wxALIGN_CENTER_VERTICAL);
        m_angularDeflectionSpinCtrl = new wxSpinCtrlDouble(panel, ID_ANGULAR_DEFLECTION_SPIN);
        m_angularDeflectionSpinCtrl->SetRange(0.1, 90.0);
        m_angularDeflectionSpinCtrl->SetIncrement(0.1);
        m_angularDeflectionSpinCtrl->SetDigits(1);
        sizer->Add(m_angularDeflectionSpinCtrl, 1, wxEXPAND);
        
        // LOD settings
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Level of Detail (LOD):"), 0, wxALIGN_CENTER_VERTICAL);
        m_lodCheckBox = new wxCheckBox(panel, ID_LOD_CHECKBOX, "Enable LOD");
        sizer->Add(m_lodCheckBox, 1, wxEXPAND);
        
        sizer->Add(new wxStaticText(panel, wxID_ANY, "LOD Rough Deflection:"), 0, wxALIGN_CENTER_VERTICAL);
        m_lodRoughSpinCtrl = new wxSpinCtrlDouble(panel, ID_LOD_ROUGH_SPIN);
        m_lodRoughSpinCtrl->SetRange(0.01, 5.0);
        m_lodRoughSpinCtrl->SetIncrement(0.01);
        m_lodRoughSpinCtrl->SetDigits(3);
        sizer->Add(m_lodRoughSpinCtrl, 1, wxEXPAND);
        
        sizer->Add(new wxStaticText(panel, wxID_ANY, "LOD Fine Deflection:"), 0, wxALIGN_CENTER_VERTICAL);
        m_lodFineSpinCtrl = new wxSpinCtrlDouble(panel, ID_LOD_FINE_SPIN);
        m_lodFineSpinCtrl->SetRange(0.001, 2.0);
        m_lodFineSpinCtrl->SetIncrement(0.001);
        m_lodFineSpinCtrl->SetDigits(3);
        sizer->Add(m_lodFineSpinCtrl, 1, wxEXPAND);
        
        panel->SetSizer(sizer);
        return panel;
    }

    wxPanel* createAdvancedSettingsPanel(wxNotebook* parent) {
        auto* panel = new wxPanel(parent);
        auto* sizer = new wxFlexGridSizer(2, wxSize(10, 5));

        // Subdivision settings
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Subdivision Surfaces:"), 0, wxALIGN_CENTER_VERTICAL);
        m_subdivisionCheckBox = new wxCheckBox(panel, ID_SUBDIVISION_CHECKBOX, "Enable Subdivision");
        sizer->Add(m_subdivisionCheckBox, 1, wxEXPAND);
        
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Subdivision Level:"), 0, wxALIGN_CENTER_VERTICAL);
        m_subdivisionLevelSpinCtrl = new wxSpinCtrl(panel, ID_SUBDIVISION_LEVEL_SPIN);
        m_subdivisionLevelSpinCtrl->SetRange(1, 5);
        sizer->Add(m_subdivisionLevelSpinCtrl, 1, wxEXPAND);
        
        // Smoothing settings
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Mesh Smoothing:"), 0, wxALIGN_CENTER_VERTICAL);
        m_smoothingCheckBox = new wxCheckBox(panel, ID_SMOOTHING_CHECKBOX, "Enable Smoothing");
        sizer->Add(m_smoothingCheckBox, 1, wxEXPAND);
        
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Smoothing Iterations:"), 0, wxALIGN_CENTER_VERTICAL);
        m_smoothingIterationsSpinCtrl = new wxSpinCtrl(panel, ID_SMOOTHING_ITERATIONS_SPIN);
        m_smoothingIterationsSpinCtrl->SetRange(1, 10);
        sizer->Add(m_smoothingIterationsSpinCtrl, 1, wxEXPAND);
        
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Smoothing Strength:"), 0, wxALIGN_CENTER_VERTICAL);
        m_smoothingStrengthSlider = new wxSlider(panel, ID_SMOOTHING_STRENGTH_SLIDER, 50, 1, 100);
        sizer->Add(m_smoothingStrengthSlider, 1, wxEXPAND);
        
        panel->SetSizer(sizer);
        return panel;
    }

    wxPanel* createPerformancePanel(wxNotebook* parent) {
        auto* panel = new wxPanel(parent);
        auto* sizer = new wxFlexGridSizer(2, wxSize(10, 5));

        // Parallel processing
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Parallel Processing:"), 0, wxALIGN_CENTER_VERTICAL);
        m_parallelProcessingCheckBox = new wxCheckBox(panel, ID_PARALLEL_CHECKBOX, "Enable Parallel Processing");
        sizer->Add(m_parallelProcessingCheckBox, 1, wxEXPAND);
        
        // Adaptive meshing
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Adaptive Meshing:"), 0, wxALIGN_CENTER_VERTICAL);
        m_adaptiveMeshingCheckBox = new wxCheckBox(panel, ID_ADAPTIVE_CHECKBOX, "Enable Adaptive Meshing");
        sizer->Add(m_adaptiveMeshingCheckBox, 1, wxEXPAND);
        
        // Real-time preview
        sizer->Add(new wxStaticText(panel, wxID_ANY, "Real-time Preview:"), 0, wxALIGN_CENTER_VERTICAL);
        m_realTimePreviewCheckBox = new wxCheckBox(panel, ID_REALTIME_CHECKBOX, "Enable Real-time Preview");
        sizer->Add(m_realTimePreviewCheckBox, 1, wxEXPAND);
        
        panel->SetSizer(sizer);
        return panel;
    }

    wxSizer* createControlButtons() {
        auto* sizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Preset buttons
        auto* fastBtn = new wxButton(this, ID_PRESET_FAST, "Fast");
        auto* balancedBtn = new wxButton(this, ID_PRESET_BALANCED, "Balanced");
        auto* qualityBtn = new wxButton(this, ID_PRESET_QUALITY, "Quality");
        auto* surfaceBtn = new wxButton(this, ID_PRESET_SURFACE, "Surface");
        
        sizer->Add(fastBtn, 0, wxRIGHT, 5);
        sizer->Add(balancedBtn, 0, wxRIGHT, 5);
        sizer->Add(qualityBtn, 0, wxRIGHT, 5);
        sizer->Add(surfaceBtn, 0, wxRIGHT, 5);
        
        sizer->AddStretchSpacer();
        
        // Apply and Cancel buttons
        auto* applyBtn = new wxButton(this, wxID_APPLY, "Apply");
        auto* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
        
        sizer->Add(applyBtn, 0, wxRIGHT, 5);
        sizer->Add(cancelBtn, 0);
        
        return sizer;
    }

    void bindEvents() {
        // Bind all UI control events
        Bind(wxEVT_SPINDIRECT, &MeshQualityDialogRefactored::onDeflectionSpin, this, ID_DEFLECTION_SPIN);
        Bind(wxEVT_SPINDIRECT, &MeshQualityDialogRefactored::onAngularDeflectionSpin, this, ID_ANGULAR_DEFLECTION_SPIN);
        Bind(wxEVT_CHECKBOX, &MeshQualityDialogRefactored::onSubdivisionCheckbox, this, ID_SUBDIVISION_CHECKBOX);
        Bind(wxEVT_SPINCTRL, &MeshQualityDialogRefactored::onSubdivisionLevelSpin, this, ID_SUBDIVISION_LEVEL_SPIN);
        Bind(wxEVT_CHECKBOX, &MeshQualityDialogRefactored::onSmoothingCheckbox, this, ID_SMOOTHING_CHECKBOX);
        Bind(wxEVT_SPINCTRL, &MeshQualityDialogRefactored::onSmoothingIterationsSpin, this, ID_SMOOTHING_ITERATIONS_SPIN);
        Bind(wxEVT_COMMAND_SLIDER_UPDATED, &MeshQualityDialogRefactored::onSmoothingStrengthSlider, this, ID_SMOOTHING_STRENGTH_SLIDER);
        Bind(wxEVT_CHECKBOX, &MeshQualityDialogRefactored::onLODCheckbox, this, ID_LOD_CHECKBOX);
        Bind(wxEVT_SPINCTRLDONE, &MeshQualityDialogRefactored::onLODRoughSpin, this, ID_LOD_ROUGH_SPIN);
        Bind(wxEVT_SPINCTRLDONE, &MeshQualityDialogRefactored::onLODFineSpin, this, ID_LOD_FINE_SPIN);
        Bind(wxEVT_CHECKBOX, &MeshQualityDialogRefactored::onParallelCheckbox, this, ID_PARALLEL_CHECKBOX);
        Bind(wxEVT_CHECKBOX, &MeshQualityDialogRefactored::onAdaptiveCheckbox, this, ID_ADAPTIVE_CHECKBOX);
        Bind(wxEVT_CHECKBOX, &MeshQualityDialogRefactored::onRealTimeCheckbox, this, ID_REALTIME_CHECKBOX);
        
        // Preset buttons
        Bind(wxEVT_BUTTON, &MeshQualityDialogRefactored::onPresetFast, this, ID_PRESET_FAST);
        Bind(wxEVT_BUTTON, &MeshQualityDialogRefactored::onPresetBalanced, this, ID_PRESET_BALANCED);
        Bind(wxEVT_BUTTON, &MeshQualityDialogRefactored::onPresetQuality, this, ID_PRESET_QUALITY);
        Bind(wxEVT_BUTTON, &MeshQualityDialogRefactored::onPresetSurface, this, ID_PRESET_SURFACE);
        
        // Apply and cancel
        Bind(wxEVT_BUTTON, &MeshQualityDialogRefactored::onApply, this, wxID_APPLY);
        Bind(wxEVT_BUTTON, &MeshQualityDialogRefactored::onCancel, this, wxID_CANCEL);
    }

    void loadCurrentParameters() {
        LOG_INF_S("Loading current parameters from MeshParameterManager");
        
        // Load basic mesh parameters
        m_deflectionSpinCtrl->SetValue(m_paramManager.getParameter(
            MeshParameterManager::Category::BASIC_MESH, MeshParamNames::BasicMesh::DEFLECTION));
        m_angularDeflectionSpinCtrl->SetValue(m_paramManager.getParameter(
            MeshParameterManager::Category::BASIC_MESH, MeshParamNames::BasicMesh::ANGULAR_DEFLECTION));
        
        // Load subdivision parameters
        bool subdivEnabled = m_paramManager.getParameter(
            MeshParameterManager::Category::SUBDIVISION, MeshParamNames::Subdivision::ENABLED) != 0.0;
        m_subdivisionCheckBox->SetValue(subdivEnabled);
        m_subdivisionLevelSpinCtrl->SetValue(static_cast<int>(m_paramManager.getParameter(
            MeshParameterManager::Category::SUBDIVISION, MeshParamNames::Subdivision::LEVEL)));
        
        // Load smoothing parameters
        bool smoothingEnabled = m_paramManager.getParameter(
            MeshParameterManager::Category::SMOOTHING, MeshParamNames::Smoothing::ENABLED) != 0.0;
        m_smoothingCheckBox->SetValue(smoothingEnabled);
        m_smoothingIterationsSpinCtrl->SetValue(static_cast<int>(m_paramManager.getParameter(
            MeshParameterManager::Category::SMOOTHING, MeshParamNames::Smoothing::ITERATIONS)));
        m_smoothingStrengthSlider->SetValue(static_cast<int>(m_paramManager.getParameter(
            MeshParameterManager::Category::SMOOTHING, MeshParamNames::Smoothing::STRENGTH) * 100));
        
        // Load LOD parameters
        bool lodEnabled = m_paramManager.getParameter(
            MeshParameterManager::Category::LOD, MeshParamNames::LOD::ENABLED) != 0.0;
        m_lodCheckBox->SetValue(lodEnabled);
        m_lodRoughSpinCtrl->SetValue(m_paramManager.getParameter(
            MeshParameterManager::Category::LOD, MeshParamNames::LOD::ROUGH_DEFLECTION));
        m_lodFineSpinCtrl->SetValue(m_paramManager.getParameter(
            MeshParameterManager::Category::LOD, MeshParamNames::LOD::FINE_DEFLECTION));
        
        // Load performance parameters
        m_realTimePreviewCheckBox->SetValue(m_enableRealTimePreview);
        
        LOG_INF_S("Parameters loaded from MeshParameterManager");
    }

    void syncUIControls() {
        // Enable/disable dependent controls
        m_subdivisionLevelSpinCtrl->Enable(m_subdivisionCheckBox->GetValue());
        m_smoothingIterationsSpinCtrl->Enable(m_smoothingCheckBox->GetValue());
        m_smoothingStrengthSlider->Enable(m_smoothingCheckBox->GetValue());
        m_lodRoughSpinCtrl->Enable(m_lodCheckBox->GetValue());
        m_lodFineSpinCtrl->Enable(m_lodCheckBox->GetValue());
    }

    void onDeflectionSpin(wxSpinDoubleEvent& event) {
        double value = event.GetValue();
        LOG_DBG_S("Deflection changed to: " + std::to_string(value));
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::BASIC_MESH, 
                                      MeshParamNames::BasicMesh::DEFLECTION, value);
            
            // Apply to geometries in real-time
            applyParametersToGeometries();
        }
    }

    void onAngularDeflectionSpin(wxSpinDoubleEvent& event) {
        double value = event.GetValue();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::BASIC_MESH, 
                                      MeshParamNames::BasicMesh::ANGULAR_DEFLECTION, value);
            applyParametersToGeometries();
        }
    }

    void onSubdivisionCheckbox(wxCommandEvent& event) {
        bool enabled = event.IsChecked();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::SUBDIVISION, 
                                      MeshParamNames::Subdivision::ENABLED, enabled ? 1.0 : 0.0);
            applyParametersToGeometries();
        }
        
        m_subdivisionLevelSpinCtrl->Enable(enabled);
    }

    void onSubdivisionLevelSpin(wxSpinEvent& event) {
        int value = event.GetValue();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::SUBDIVISION, 
                                      MeshParamNames::Subdivision::LEVEL, static_cast<double>(value));
            applyParametersToGeometries();
        }
    }

    void onSmoothingCheckbox(wxCommandEvent& event) {
        bool enabled = event.IsChecked();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::SMOOTHING, 
                                      MeshParamNames::Smoothing::ENABLED, enabled ? 1.0 : 0.0);
            applyParametersToGeometries();
        }
        
        m_smoothingIterationsSpinCtrl->Enable(enabled);
        m_smoothingStrengthSlider->Enable(enabled);
    }

    void onSmoothingIterationsSpin(wxSpinEvent& event) {
        int value = event.GetValue();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::SMOOTHING, 
                                      MeshParamNames::Smoothing::ITERATIONS, static_cast<double>(value));
            applyParametersToGeometries();
        }
    }

    void onSmoothingStrengthSlider(wxScrollEvent& event) {
        int value = event.GetPosition();
        double strength = value / 100.0;
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::SMOOTHING, 
                                      MeshParamNames::Smoothing::STRENGTH, strength);
            applyParametersToGeometries();
        }
    }

    void onLODCheckbox(wxCommandEvent& event) {
        bool enabled = event.IsChecked();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                      MeshParamNames::LOD::ENABLED, enabled ? 1.0 : 0.0);
            applyParametersToGeometries();
        }
        
        m_lodRoughSpinCtrl->Enable(enabled);
        m_lodFineSpinCtrl->Enable(enabled);
    }

    void onLODRoughSpin(wxSpinDoubleEvent& event) {
        double value = event.GetValue();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                      MeshParamNames::LOD::ROUGH_DEFLECTION, value);
            applyParametersToGeometries();
        }
    }

    void onLODFineSpin(wxSpinDoubleEvent& event) {
        double value = event.GetValue();
        
        if (m_enableRealTimePreview) {
            m_paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                      MeshParamNames::LOD::FINE_DEFLECTION, value);
            applyParametersToGeometries();
        }
    }

    void onParallelCheckbox(wxCommandEvent& event) {
        // Performance parameters don't need real-time preview
        LOG_INF_S("Parallel processing " + std::string(event.IsChecked() ? "enabled" : "disabled"));
    }

    void onAdaptiveCheckbox(wxCommandEvent& event) {
        // Performance parameters don't need real-time preview
        LOG_INF_S("Adaptive meshing " + std::string(event.IsChecked() ? "enabled" : "disabled"));
    }

    void onRealTimeCheckbox(wxCommandEvent& event) {
        m_enableRealTimePreview = event.IsChecked();
        LOG_INF_S("Real-time preview " + std::string(m_enableRealTimePreview ? "enabled" : "disabled"));
        
        updateRealTimeCallback();
    }

    void onPresetFast(wxCommandEvent& event) {
        LOG_INF_S("Applying Fast preset");
        m_paramManager.applyPreset("Fast");
        loadCurrentParameters();
        syncUIControls();
        
        if (m_enableRealTimePreview) {
            applyParametersToGeometries();
        }
    }

    void onPresetBalanced(wxCommandEvent& event) {
        LOG_INF_S("Applying Balanced preset");
        m_paramManager.applyPreset("Balanced");
        loadCurrentParameters();
        syncUIControls();
        
        if (m_enableRealTimePreview) {
            applyParametersToGeometries();
        }
    }

    void onPresetQuality(wxCommandEvent& event) {
        LOG_INF_S("Applying Quality preset");
        m_paramManager.applyPreset("Quality");
        loadCurrentParameters();
        syncUIControls();
        
        if (m_enableRealTimePreview) {
            applyParametersToGeometries();
        }
    }

    void onPresetSurface(wxCommandEvent& event) {
        LOG_INF_S("Applying Surface preset");
        m_paramManager.applyPreset("Surface");
        loadCurrentParameters();
        syncUIControls();
        
        if (m_enableRealTimePreview) {
            applyParametersToGeometries();
        }
    }

    void onApply(wxCommandEvent& event) {
        LOG_INF_S("=== APPLYING PARAMETERS FROM MESHQUALITYDIALOG ===");
        
        // Apply all current UI values to parameter manager
        updateParametersFromUI();
        
        // Validate parameters
        if (!m_paramManager.validateCurrentParameters()) {
            wxMessageBox("Some parameter values are invalid. Please check the settings.", 
                        "Invalid Parameters", wxOK | wxICON_WARNING);
            return;
        }
        
        // Apply parameters to all geometries
        if (m_occViewer) {
            m_paramManager.regenerateAllGeometries(m_occViewer);
            
            // Validate application
            wxString msg = wxString::Format(
                "Mesh parameters applied successfully.\n\n"
                "Current settings:\n"
                "- Deflection: %.3f\n"
                "- Angular Deflection: %.3f\n"
                "- Subdivision: %s (Level %d)\n"
                "- Smoothing: %s (%d iterations)\n"
                "- LOD: %s\n"
                "- Parallel Processing: %s",
                m_deflectionSpinCtrl->GetValue(),
                m_angularDeflectionSpinCtrl->GetValue(),
                m_subdivisionCheckBox->GetValue() ? "Enabled" : "Disabled",
                m_subdivisionLevelSpinCtrl->GetValue(),
                m_smoothingCheckBox->GetValue() ? "Enabled" : "Disabled",
                m_smoothingIterationsSpinCtrl->GetValue(),
                m_lodCheckBox->GetValue() ? "Enabled" : "Disabled",
                m_parallelProcessingCheckBox->GetValue() ? "Enabled" : "Disabled"
            );
            
            wxMessageBox(msg, "Parameters Applied", wxOK | wxICON_INFORMATION);
        }
        
        LOG_INF_S("=== PARAMETERS APPLIED SUCCESSFULLY ===");
    }

    void onCancel(wxCommandEvent& event) {
        EndModal(wxID_CANCEL);
    }

    void updateParametersFromUI() {
        LOG_INF_S("Updating parameter manager from UI values");
        
        // Update basic mesh parameters
        m_paramManager.setParameter(MeshParameterManager::Category::BASIC_MESH, 
                                  MeshParamNames::BasicMesh::DEFLECTION, m_deflectionSpinCtrl->GetValue());
        m_paramManager.setParameter(MeshParameterManager::Category::BASIC_MESH, 
                                  MeshParamNames::BasicMesh::ANGULAR_DEFLECTION, m_angularDeflectionSpinCtrl->GetValue());
        
        // Update subdivision parameters
        m_paramManager.setParameter(MeshParameterManager::Category::SUBDIVISION, 
                                  MeshParamNames::Subdivision::ENABLED, m_subdivisionCheckBox->GetValue() ? 1.0 : 0.0);
        m_paramManager.setParameter(MeshParameterManager::Category::SUBDIVISION, 
                                  MeshParamNames::Subdivision::LEVEL, static_cast<double>(m_subdivisionLevelSpinCtrl->GetValue()));
        
        // Update smoothing parameters
        m_paramManager.setParameter(MeshParameterManager::Category::SMOOTHING, 
                                  MeshParamNames::Smoothing::ENABLED, m_smoothingCheckBox->GetValue() ? 1.0 : 0.0);
        m_paramManager.setParameter(MeshParameterManager::Category::SMOOTHING, 
                                  MeshParamNames::Smoothing::ITERATIONS, static_cast<double>(m_smoothingIterationsSpinCtrl->GetValue()));
        m_paramManager.setParameter(MeshParameterManager::Category::SMOOTHING, 
                                  MeshParamNames::Smoothing::STRENGTH, m_smoothingStrengthSlider->GetValue() / 100.0);
        
        // Update LOD parameters
        m_paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                  MeshParamNames::LOD::ENABLED, m_lodCheckBox->GetValue() ? 1.0 : 0.0);
        m_paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                  MeshParamNames::LOD::ROUGH_DEFLECTION, m_lodRoughSpinCtrl->GetValue());
        m_paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                  MeshParamNames::LOD::FINE_DEFLECTION, m_lodFineSpinCtrl->GetValue());
    }

    void applyParametersToGeometries() {
        if (!m_occViewer) return;
        
        LOG_DBG_S("Applying parameters to geometries (real-time preview)");
        
        auto geometries = m_occViewer->getAllGeometry();
        m_paramManager.applyToGeometries(geometries);
        
        m_occViewer->requestViewRefresh();
    }

    void onParameterManagerChange(const MeshParameterManager::ParameterChange& change) {
        // Called when parameter manager changes parameters (e.g., from presets or dependencies)
        LOG_DBG_S("Parameter manager changed: " + change.name + " [" + 
                 std::to_string(change.oldValue) + " -> " + std::to_string(change.newValue) + "]");
        
        // Update UI to reflect parameter manager changes
        updateUIFromParameterManager(change);
    }

    void updateUIFromParameterManager(const MeshParameterManager::ParameterChange& change) {
        // Update UI controls to match parameter manager state
        if (change.category == MeshParameterManager::Category::BASIC_MESH) {
            if (change.name == MeshParamNames::BasicMesh::DEFLECTION) {
                m_deflectionSpinCtrl->SetValue(change.newValue);
            } else if (change.name == MeshParamNames::BasicMesh::ANGULAR_DEFLECTION) {
                m_angularDeflectionSpinCtrl->SetValue(change.newValue);
            }
        } else if (change.category == MeshParameterManager::Category::LOD) {
            if (change.name == MeshParamNames::LOD::ROUGH_DEFLECTION) {
                m_lodRoughSpinCtrl->SetValue(change.newValue);
            } else if (change.name == MeshParamNames::LOD::FINE_DEFLECTION) {
                m_lodFineSpinCtrl->SetValue(change.newValue);
            }
        }
        
        syncUIControls();
    }

    void updateRealTimeCallback() {
        // Enable/disable real-time parameter application based on preview checkbox
        if (m_enableRealTimePreview && m_occViewer) {
            LOG_INF_S("Real-time parameter updates enabled");
        } else {
            LOG_INF_S("Real-time parameter updates disabled");
        }
    }

public:
    enum ControlID {
        ID_DEFLECTION_SPIN = wxID_HIGHEST + 1,
        ID_ANGULAR_DEFLECTION_SPIN,
        ID_SUBDIVISION_CHECKBOX,
        ID_SUBDIVISION_LEVEL_SPIN,
        ID_SMOOTHING_CHECKBOX,
        ID_SMOOTHING_ITERATIONS_SPIN,
        ID_SMOOTHING_STRENGTH_SLIDER,
        ID_LOD_CHECKBOX,
        ID_LOD_ROUGH_SPIN,
        ID_LOD_FINE_SPIN,
        ID_PARALLEL_CHECKBOX,
        ID_ADAPTIVE_CHECKBOX,
        ID_REALTIME_CHECKBOX,
        ID_PRESET_FAST,
        ID_PRESET_BALANCED,
        ID_PRESET_QUALITY,
        ID_PRESET_SURFACE
    };
};

/**
 * Simplified wrapper for using the refactored dialog
 */
class SimplifiedMeshQualityDialog {
public:
    static void applyPresetToViewer(std::shared_ptr<OCCViewer> viewer, const std::string& presetName) {
        LOG_INF_S("Applying preset '" + presetName + "' directly to viewer");
        
        auto& paramManager = MeshParameterManager::getInstance();
        paramManager.applyPreset(presetName);
        paramManager.regenerateAllGeometries(viewer);
        
        LOG_INF_S("Preset applied successfully");
    }
    
    static void setParametersAndApply(std::shared_ptr<OCCViewer> viewer, 
                                     double deflection, double angularDeflection,
                                     bool subdivisionEnabled, int subdivisionLevel,
                                     bool smoothingEnabled, int smoothingIterations,
                                     bool lodEnabled) {
        LOG_INF_S("Setting parameters directly");
        
        auto& paramManager = MeshParameterManager::getInstance();
        
        // Set parameters atomically
        std::map<std::pair<MeshParameterManager::Category, std::string>, double> params;
        params[{{MeshParameterManager::Category::BASIC_MESH}, MeshParamNames::BasicMesh::DEFLECTION}] = deflection;
        params[{{MeshParameterManager::Category::BASIC_MESH}, MeshParamNames::BasicMesh::ANGULAR_DEFLECTION}] = angularDeflection;
        params[{{MeshParameterManager::Category::SUBDIVISION}, MeshParamNames::Subdivision::LEVEL}] = static_cast<double>(subdivisionLevel);
        params[{{MeshParameterManager::Category::SMOOTHING}, MeshParamNames::Smoothing::ITERATIONS}] = static_cast<double>(smoothingIterations);
        
        paramManager.setParameters(params);
        
        // Apply to geometries
        paramManager.regenerateAllGeometries(viewer);
        
        LOG_INF_S("Parameters set and applied successfully");
    }
};
