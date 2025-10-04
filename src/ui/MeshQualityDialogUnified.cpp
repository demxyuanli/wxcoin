#include "MeshQualityDialog.h"
#include "MeshParameterManager.h"
#include "OCCViewer.h"
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
 * Unified MeshQualityDialog that simplifies parameter management
 * Uses MeshParameterManager for centralized parameter control
 */
class MeshQualityDialogUnified : public wxDialog {
public:
    MeshQualityDialogUnified(wxWindow* parent, OCCViewer* occViewer)
        : wxDialog(parent, wxID_ANY, "Mesh Quality Control", wxDefaultPosition, wxSize(600, 600))
        , m_occViewer(occViewer)
        , m_paramManager(nullptr)
        , m_notebook(nullptr)
        , m_enableRealTimePreview(false) {
        
        LOG_INF_S("=== UNIFIED MESH QUALITY DIALOG INITIALIZATION ===");
        
        if (!m_occViewer) {
            LOG_ERR_S("OCCViewer is null in MeshQualityDialogUnified");
            return;
        }
        
        // Initialize parameter manager
        initializeParameterManager();
        
        // Load current parameters
        loadCurrentMeshParameters();
        
        // Create UI
        createUI();
        
        // Bind events
        bindEvents();
        
        LOG_INF_S("Unified MeshQualityDialog initialized successfully");
    }
    
    ~MeshQualityDialogUnified() {
        if (m_paramManager) {
            // Unregister callback
            m_paramManager->unregisterParameterChangeCallback(m_parameterCallbackId);
        }
    }

private:
    struct ControlGroup {
        wxSlider* slider;
        wxSpinCtrlDouble* spinCtrl;
        wxStraticText* label;
        std::string paramName;
        MeshParameterManager::Category category;
        
        ControlGroup() : slider(nullptr), spinCtrl(nullptr), label(nullptr) {}
        
        void updateRange(double minVal, double maxVal, double step = 0.001) {
            if (slider) {
                slider->SetRange(static_cast<int>(minVal * 1000), static_cast<int>(maxVal * 1000));
            }
            if (spinCtrl) {
                spinCtrl->SetRange(minVal, maxVal);
                spinCtrl->SetIncrement(step);
            }
        }
        
        void setValue(double value) {
            if (slider) {
                slider->SetValue(static_cast<int>(value * 1000));
            }
            if (spinCtrl) {
                spinCtrl->SetValue(value);
            }
        }
        
        double getValue() const {
            return slider ? static_cast<double>(slider->GetValue()) / 1000.0 : 
                   spinCtrl ? spinCtrl->GetValue() : 0.0;
        }
        
        void setEnabled(bool enabled) {
            if (slider) slider->Enable(enabled);
            if (spinCtrl) spinCtrl->Enable(enabled);
        }
    };
    
    // Core components
    OCCViewer* m_occViewer;
    MeshParameterManager* m_paramManager;
    wxNotebook* m_notebook;
    
    // Basic mesh parameters
    ControlGroup m_deflection;
    ControlGroup m_angularDeflection;
    
    // LOD parameters
    wxCheckBox* m_lodEnableCheckbox;
    ControlGroup m_lodRough;
    ControlGroup m_lodFine;
    
    // Subdivision parameters
    wxCheckBox* m_subdivisionEnableCheckbox;
    ControlGroup m_subdivisionLevel;
    wxChoice* m_subdivisionMethodChoice;
    ControlGroup m_subdivisionCreaseAngle;
    
    // Smoothing parameters
    wxCheckBox* m_smoothingEnableCheckbox;
    ControlGroup m_smoothingIterations;
    ControlGroup m_smoothingStrength;
    ControlGroup m_smoothingCreaseAngle;
    
    // Advanced parameters
    ControlGroup m_tessellationQuality;
    ControlGroup m_featurePreservation;
    wxCheckBox* m_parallelProcessingCheckbox;
    wxCheckBox* m_adaptiveMeshingCheckbox;
    
    // UI controls
    wxCheckBox* m_realTimePreviewCheckbox;
    
    // Parameter sync
    bool m_enableRealTimePreview;
    uint32_t m_parameterCallbackId;
    
    void initializeParameterManager() {
        LOG_INF_S("Initializing parameter manager...");
        
        m_paramManager = &MeshParameterManager::getInstance();
        
        // Register callback for parameter changes
        m_parameterCallbackId = m_paramManager->registerParameterChangeCallback(
            [this](const MeshParameterManager::ParameterChange& change) {
                onParameterChanged(change);
            }
        );
        
        // Load default configuration
        m_paramManager->loadFromConfig();
        
        LOG_INF_S("Parameter manager initialized with callback ID: " + 
                 std::to_string(m_parameterCallbackId));
    }
    
    void loadCurrentMeshParameters() {
        LOG_INF_S("Loading current mesh parameters...");
        
        if (!m_occViewer) {
            LOG_WRN_S("OCCViewer not available, using defaults");
            return;
        }
        
        // Get current parameters from OCCViewer and sync to ParameterManager
        std::unordered_map<MeshParameterManager::Category, 
                          std::unordered_map<std::string, double>> paramSets;
        
        // Basic mesh parameters
        paramSets[MeshParameterManager::Category::BASIC_MESH][MeshParamNames::BasicMesh::DEFLECTION] = 
            m_occViewer->getMeshDeflection();
        paramSets[MeshParameterManager::Category::BASIC_MESH][MeshParamNames::BasicMesh::ANGULAR_DEFLECTION] = 
            m_occViewer->getAngularDeflection();
        
        // Subdivision parameters
        paramSets[MeshParameterManager::Category::SUBDIVISION][MeshParamNames::Subdivision::LEVEL] = 
            static_cast<double>(m_occViewer->getSubdivisionLevel());
        paramSets[MeshParameterManager::Category::SUBDIVISION][MeshParamNames::Subdivision::ENABLED] = 
            m_occViewer->isSubdivisionEnabled() ? 1.0 : 0.0;
        
        // Smoothing parameters
        paramSets[MeshParameterManager::Category::SMOOTHING][MeshParamNames::Smoothing::ITERATIONS] = 
            static_cast<double>(m_occViewer->getSmoothingIterations());
        paramSets[MeshParameterManager::Category::SMOOTHING][MeshParamNames::Smoothing::STRENGTH] = 
            m_occViewer->getSmoothingStrength();
        paramSets[MeshParameterManager::Category::SMOOTHING][MeshParamNames::Smoothing::ENABLED] = 
            m_occViewer->isSmoothingEnabled() ? 1.0 : 0.0;
        
        // Set parameters atomically
        m_paramManager->setParametersBulk(paramSets);
        
        LOG_INF_S("Current parameters synced to parameter manager");
    }
    
    void createUI() {
        LOG_INF_S("Creating UI controls...");
        
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        
        // Create notebook
        m_notebook = new wxNotebook(this, wxID_ANY);
        
        // Create pages
        createBasicPage();
        createAdvancedPage();
        createPresetsPage();
        
        mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);
        
        // Button panel
        createButtonPanel(mainSizer);
        
        SetSizerAndFit(mainSizer);
        SetMinSize(wxSize(600, 600));
        
        LOG_INF_S("UI created successfully");
    }
    
    void createBasicPage() {
        wxPanel* basicPage = new wxPanel(m_notebook);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        // Basic Deflection Group
        createControlGroup(basicPage, sizer, "Mesh Deflection", 
                          "deflection", MeshParameterManager::Category::BASIC_MESH,
                          m_deflection, 0.001, 2.0);
        
        // Angular Deflection Group
        createControlGroup(basicPage, sizer, "Angular Deflection",
                          "angular_deflection", MeshParameterManager::Category::BASIC_MESH,
                          m_angularDeflection, 0.1, 2.0);
        
        // Enable/Disable checkboxes
        wxStaticBox* optionsBox = new wxStaticBox(basicPage, wxID_ANY, "Real-time Options");
        wxStaticBoxSizer* optionsSizer = new wxStaticBoxSizer(optionsBox, wxVERTICAL);
        
        m_realTimePreviewCheckbox = new wxCheckBox(basicPage, wxID_ANY, 
                                                  "Enable Real-time Preview");
        m_realTimePreviewCheckbox->SetValue(m_enableRealTimePreview);
        optionsSizer->Add(m_realTimePreviewCheckbox, 0, wxALL, 5);
        
        sizer->Add(optionsSizer, 0, wxEXPAND | wxALL, 5);
        
        basicPage->SetSizer(sizer);
        m_notebook->AddPage(basicPage, "Basic Quality");
    }
    
    void createAdvancedPage() {
        wxPanel* advancedPage = new wxPanel(m_notebook);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        // Subdivision Section
        wxStaticBox* subBox = new wxStaticBox(advancedPage, wxID_ANY, "Subdivision Surfaces");
        wxStaticBoxSizer* subSizer = new wxStaticBoxSizer(subBox, wxVERTICAL);
        
        m_subdivisionEnableCheckbox = new wxCheckBox(advancedPage, wxID_ANY, 
                                                   "Enable Subdivision");
        subSizer->Add(m_subdivisionEnableCheckbox, 0, wxALL, 5);
        
        createControlGroup(advancedPage, subSizer, "Subdivision Level",
                          "subdivision_level", MeshParameterManager::Category::SUBDIVISION,
                          m_subdivisionLevel, 1, 5, 1);
        
        sizer->Add(subSizer, 0, wxEXPAND | wxALL, 5);
        
        // Smoothing Section
        wxStaticBox* smoothBox = new wxStaticBox(advancedPage, wxID_ANY, "Mesh Smoothing");
        wxStaticBoxSizer* smoothSizer = new wxStaticBoxSizer(smoothBox, wxVERTICAL);
        
        m_smoothingEnableCheckbox = new wxCheckBox(advancedPage, wxID_ANY, 
                                                 "Enable Smoothing");
        smoothSizer->Add(m_smoothingEnableCheckbox, 0, wxALL, 5);
        
        createControlGroup(advancedPage, smoothSizer, "Smoothing Iterations",
                          "smoothing_iterations", MeshParameterManager::Category::SMOOTHING,
                          m_smoothingIterations, 1, 10, 1);
        
        createControlGroup(advancedPage, smoothSizer, "Smoothing Strength",
                          "smoothing_strength", MeshParameterManager::Category::SMOOTHING,
                          m_smoothingStrength, 0.01, 1.0);
        
        sizer->Add(smoothSizer, 0, wxEXPAND | wxALL, 5);
        
        // Performance Section
        wxStaticBox* perfBox = new wxStaticBox(advancedPage, wxID_ANY, "Performance");
        wxStaticBoxSizer* perfSizer = new wxStaticBoxSizer(perfBox, wxVERTICAL);
        
        m_parallelProcessingCheckbox = new wxCheckBox(advancedPage, wxID_ANY, 
                                                    "Parallel Processing");
        m_adaptiveMeshingCheckbox = new wxCheckBox(advancedPage, wxID_ANY, 
                                                "Adaptive Meshing");
        
        perfSizer->Add(m_parallelProcessingCheckbox, 0, wxALL, 5);
        perfSizer->Add(m_adaptiveMeshingCheckbox, 0, wxALL, 5);
        
        sizer->Add(perfSizer, 0, wxEXPAND | wxALL, 5);
        
        advancedPage->SetSizer(sizer);
        m_notebook->AddPage(advancedPage, "Advanced");
    }
    
    void createPresetsPage() {
        wxPanel* presetsPage = new wxPanel(m_notebook);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        wxStaticBox* presetBox = new wxStaticBox(presetsPage, wxID_ANY, "Quick Presets");
        wxStaticBoxSizer* presetSizer = new wxStaticBoxSizer(presetBox, wxVERTICAL);
        
        // Create preset buttons
        wxButton* performanceBtn = new wxButton(presetsPage, wxID_ANY, "Performance");
        wxButton* balancedBtn = new wxButton(presetsPage, wxID_ANY, "Balanced");
        wxButton* qualityBtn = new wxButton(presetsPage, wxID_ANY, "Quality");
        wxButton* ultraBtn = new wxButton(presetsPage, wxID_ANY, "Ultra Quality");
        
        // Bind events
        performanceBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onPerformancePreset, this);
        balancedBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onBalancedPreset, this);
        qualityBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onQualityPreset, this);
        ultraBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onUltraPreset, this);
        
        wxGridSizer* buttonGrid = new wxGridSizer(2, 2, 5, 5);
        buttonGrid->Add(performanceBtn, 0, wxEXPAND);
        buttonGrid->Add(balancedBtn, 0, wxEXPAND);
        buttonGrid->Add(qualityBtn, 0, wxEXPAND);
        buttonGrid->Add(ultraBtn, 0, wxEXPAND);
        
        presetSizer->Add(buttonGrid, 0, wxEXPAND | wxALL, 10);
        sizer->Add(presetSizer, 0, wxEXPAND | wxALL, 10);
        
        presetsPage->SetSizer(sizer);
        m_notebook->AddPage(presetsPage, "Presets");
    }
    
    void createControlGroup(wxWindow* parent, wxSizer* sizer, const std::string& label,
                           const std::string& paramName, MeshParameterManager::Category category,
                           ControlGroup& group, double minVal, double maxVal, double step = 0.001) {
        
        group.paramName = paramName;
        group.category = category;
        
        wxStaticText* labelCtrl = new wxStaticText(parent, wxID_ANY, label);
        sizer->Add(labelCtrl, 0, wxALL, 2);
        
        wxBoxSizer* controlSizer = new wxBoxSizer(wxHORIZONTAL);
        
        group.slider = new wxSlider(parent, wxID_ANY, 0, 
                                   static_cast<int>(minVal * 1000), static_cast<int>(maxVal * 1000),
                                   wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
        
        group.spinCtrl = new wxSpinCtrlDouble(parent, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
                                             minVal, maxVal, minVal, step);
        
        controlSizer->Add(group.slider, 1, wxEXPAND | wxRIGHT, 5);
        controlSizer->Add(group.spinCtrl, 0, wxALIGN_CENTER_VERTICAL);
        
        sizer->Add(controlSizer, 0, wxEXPAND | wxALL, 2);
        
        // Initialize with current value from parameter manager
        if (m_paramManager) {
            double value = m_paramManager->getParameter(category, paramName);
            group.setValue(value);
        }
    }
    
    void createButtonPanel(wxSizer* mainSizer) {
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        
        wxButton* validateBtn = new wxButton(this, wxID_ANY, "Validate");
        wxButton* applyBtn = new wxButton(this, wxID_APPLY, "Apply");
        wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
        wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
        
        // Bind button events
        validateBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onValidate, this);
        applyBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onApply, this);
        okBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onOK, this);
        cancelBtn->Bind(wxEVT_BUTTON, &MeshQualityDialogUnified::onCancel, this);
        
        buttonSizer->Add(validateBtn, 0, wxALL, 5);
        buttonSizer->Add(applyBtn, 0, wxALL, 5);
        buttonSizer->AddStretchSpacer();
        buttonSizer->Add(okBtn, 0, wxALL, 5);
        buttonSizer->Add(cancelBtn, 0, wxALL, 5);
        
        mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    }
    
    void bindEvents() {
        LOG_INF_S("Binding UI events...");
        
        // Bind parameter control events
        if (m_deflection.slider) {
            m_deflection.slider->Bind(wxEVT_SLIDER, 
                [this](wxCommandEvent&) { onParameterControlChange(m_deflection); });
        }
        if (m_deflection.spinCtrl) {
            m_deflection.spinCtrl->Bind(wxEVT_SPINCTRLDOUBLE,
                [this](wxSpinDoubleEvent&) { onParameterControlChange(m_deflection); });
        }
        
        if (m_angularDeflection.slider) {
            m_angularDeflection.slider->Bind(wxEVT_SLIDER,
                [this](wxCommandEvent&) { onParameterControlChange(m_angularDeflection); });
        }
        if (m_angularDeflection.spinCtrl) {
            m_angularDeflection.spinCtrl->Bind(wxEVT_SPINCTRLDOUBLE,
                [this](wxSpinDoubleEvent&) { onParameterControlChange(m_angularDeflection); });
        }
        
        // Bind checkbox events
        if (m_subdivisionEnableCheckbox) {
            m_subdivisionEnableCheckbox->Bind(wxEVT_CHECKBOX,
                [this](wxCommandEvent&) { onCheckboxChange(MeshParameterManager::Category::SUBDIVISION); });
        }
        
        if (m_smoothingEnableCheckbox) {
            m_smoothingEnableCheckbox->Bind(wxEVT_CHECKBOX,
                [this](wxCommandEvent&) { onCheckboxChange(MeshParameterManager::Category::SMOOTHING); });
        }
        
        LOG_INF_S("Events bound successfully");
    }
    
    void onParameterControlChange(const ControlGroup& group) {
        LOG_DBG_S("Parameter control changed: " + group.paramName + 
                 " = " + std::to_string(group.getValue()));
        
        // Update parameter manager
        m_paramManager->setParameter(group.category, group.paramName, group.getValue());
        
        // Sync spin control with slider
        if (group.slider && group.spinCtrl) {
            double value = static_cast<double>(group.slider->GetValue()) / 1000.0;
            group.spinCtrl->SetValue(value);
        }
    }
    
    void onCheckboxChange(MeshParameterManager::Category category) {
        bool enabled = false;
        
        if (category == MeshParameterManager::Category::SUBDIVISION && m_subdivisionEnableCheckbox) {
            enabled = m_subdivisionEnableCheckbox->GetValue();
        } else if (category == MeshParameterManager::Category::SMOOTHING && m_smoothingEnableCheckbox) {
            enabled = m_smoothingEnableCheckbox->GetValue();
        }
        
        if (category == MeshParameterManager::Category::SUBDIVISION) {
            m_paramManager->setParameter(category, "subdivision_enabled", enabled ? 1.0 : 0.0);
            
            // Enable/disable subdivision controls
            m_subdivisionLevel.setEnabled(enabled);
        } else if (category == MeshParameterManager::Category::SMOOTHING) {
            m_paramManager->setParameter(category, "smoothing_enabled", enabled ? 1.0 : 0.0);
            
            // Enable/disable smoothing controls
            m_smoothingIterations.setEnabled(enabled);
            m_smoothingStrength.setEnabled(enabled);
        }
    }
    
    void onParameterChanged(const MeshParameterManager::ParameterChange& change) {
        LOG_DBG_S("Parameter changed: " + change.name + " = " + std::to_string(change.newValue));
        
        // Apply changes to UI controls if needed
        if (change.name == "deflection" && m_deflection.slider) {
            m_deflection.setValue(change.newValue);
        } else if (change.name == "angular_deflection" && m_angularDeflection.slider) {
            m_angularDeflection.setValue(change.newValue);
        }
        
        // Apply to geometries immediately if real-time preview is enabled
        if (m_enableRealTimePreview && m_occViewer) {
            applyParametersToGeometries();
        }
    }
    
    // Preset handlers
    void onPerformancePreset(wxCommandEvent&) {
        LOG_INF_S("Applying Performance Preset");
        std::unordered_map<MeshParameterManager::Category, std::unordered_map<std::string, double>> params;
        params[MeshParameterManager::Category::BASIC_MESH]["deflection"] = 2.0;
        params[MeshParameterManager::Category::BASIC_MESH]["angular_deflection"] = 1.0;
        params[MeshParameterManager::Category::SUBDIVISION]["subdivision_enabled"] = 0.0;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_enabled"] = 0.0;
        m_paramManager->setParametersBulk(params);
        syncUIFromParameters();
    }
    
    void onBalancedPreset(wxCommandEvent&) {
        LOG_INF_S("Applying Balanced Preset");
        std::unordered_map<MeshParameterManager::Category, std::unordered_map<std::string, double>> params;
        params[MeshParameterManager::Category::BASIC_MESH]["deflection"] = 1.0;
        params[MeshParameterManager::Category::BASIC_MESH]["angular_deflection"] = 0.5;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_enabled"] = 1.0;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_iterations"] = 2.0;
        m_paramManager->setParametersBulk(params);
        syncUIFromParameters();
    }
    
    void onQualityPreset(wxCommandEvent&) {
        LOG_INF_S("Applying Quality Preset");
        std::unordered_map<MeshParameterManager::Category, std::unordered_map<std::string, double>> params;
        params[MeshParameterManager::Category::BASIC_MESH]["deflection"] = 0.5;
       params[MeshParameterManager::Category::BASIC_MESH]["angular_deflection"] = 0.3;
        params[MeshParameterManager::Category::SUBDIVISION]["subdivision_enabled"] = 1.0;
        params[MeshParameterManager::Category::SUBDIVISION]["subdivision_level"] = 2.0;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_enabled"] = 1.0;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_iterations"] = 3.0;
        m_paramManager->setParametersBulk(params);
        syncUIFromParameters();
    }
    
    void onUltraPreset(wxCommandEvent&) {
        LOG_INF_S("Applying Ultra Quality Preset");
        std::unordered_map<MeshParameterManager::Category, std::unordered_map<std::string, double>> params;
        params[MeshParameterManager::Category::BASIC_MESH]["deflection"] = 0.2;
        params[MeshParameterManager::Category::BASIC_MESH]["angular_deflection"] = 0.15;
        params[MeshParameterManager::Category::SUBDIVISION]["subdivision_enabled"] = 1.0;
        params[MeshParameterManager::Category::SUBDIVISION]["subdivision_level"] = 3.0;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_enabled"] = 1.0;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_iterations"] = 4.0;
        params[MeshParameterManager::Category::SMOOTHING]["smoothing_strength"] = 0.8;
        m_paramManager->setParametersBulk(params);
        syncUIFromParameters();
    }
    
    void syncUIFromParameters() {
        LOG_INF_S("Syncing UI from parameter manager...");
        
        if (!m_paramManager) return;
        
        // Update all controls
        if (m_deflection.slider) {
            double value = m_paramManager->getParameter(MeshParameterManager::Category::BASIC_MESH, "deflection");
            m_deflection.setValue(value);
        }
        
        if (m_angularDeflection.slider) {
            double value = m_paramManager->getParameter(MeshParameterManager::Category::BASIC_MESH, "angular_deflection");
            m_angularDeflection.setValue(value);
        }
        
        if (m_subdivisionEnableCheckbox) {
            bool enabled = m_paramManager->getParameter(MeshParameterManager::Category::SUBDIVISION, "subdivision_enabled") > 0.5;
            m_subdivisionEnableCheckbox->SetValue(enabled);
            m_subdivisionLevel.setEnabled(enabled);
        }
        
        if (m_smoothingEnableCheckbox) {
            bool enabled = m_paramManager->getParameter(MeshParameterManager::Category::SMOOTHING, "smoothing_enabled") > 0.5;
            m_smoothingEnableCheckbox->SetValue(enabled);
            m_smoothingIterations.setEnabled(enabled);
            m_smoothingStrength.setEnabled(enabled);
        }
        
        LOG_INF_S("UI synced successfully");
    }
    
    void applyParametersToGeometries() {
        LOG_INF_S("Applying parameters to geometries...");
        
        if (!m_occViewer || !m_paramManager) {
            LOG_WRN_S("OCCViewer or ParameterManager not available");
            return;
        }
        
        // Get all current parameters from manager
        auto params = m_paramManager->getAllParameters();
        
        // Apply to OCCViewer
        if (params.count(MeshParameterManager::Category::BASIC_MESH)) {
            const auto& basicParams = params[MeshParameterManager::Category::BASIC_MESH];
            
            if (basicParams.count("deflection")) {
                m_occViewer->setMeshDeflection(basicParams.at("deflection"), false);
            }
            if (basicParams.count("angular_deflection")) {
                m_occViewer->setAngularDeflection(basicParams.at("angular_deflection"), false);
            }
        }
        
        // Apply subdivision parameters
        if (params.count(MeshParameterManager::Category::SUBDIVISION)) {
            const auto& subParams = params[MeshParameterManager::Category::SUBDIVISION];
            
            bool enabled = subParams.count("subdivision_enabled") && subParams.at("subdivision_enabled") > 0.5;
            m_occViewer->setSubdivisionEnabled(enabled);
            
            if (subParams.count("subdivision_level")) {
                m_occViewer->setSubdivisionLevel(static_cast<int>(subParams.at("subdivision_level")));
            }
        }
        
        // Apply smoothing parameters
        if (params.count(MeshParameterManager::Category::SMOOTHING)) {
            const auto& smoothParams = params[MeshParameterManager::Category::SMOOTHING];
            
            bool enabled = smoothParams.count("smoothing_enabled") && smoothParams.at("smoothing_enabled") > 0.5;
            m_occViewer->setSmoothingEnabled(enabled);
            
            if (smoothParams.count("smoothing_iterations")) {
                m_occViewer->setSmoothingIterations(static_cast<int>(smoothParams.at("smoothing_iterations")));
            }
            if (smoothParams.count("smoothing_strength")) {
                m_occViewer->setSmoothingStrength(smoothParams.at("smoothing_strength"));
            }
        }
        
        // Force remesh all geometries
        m_occViewer->remeshAllGeometries();
        
        LOG_INF_S("Parameters applied successfully");
    }
    
    // Event handlers
    void onValidate(wxCommandEvent&) {
        LOG_INF_S("=== VALIDATING MESH PARAMETERS ===");
        
        if (!m_occViewer) {
            wxMessageBox("OCCViewer not available", "Validation Error", wxOK | wxICON_ERROR);
            return;
        }
        
        // Validate parameters in manager
        m_paramManager->validateParameters();
        
        // Validate application to geometries
        applyParametersToGeometries();
        
        wxMessageBox("Parameters validated successfully", "Validation Complete", wxOK | wxICON_INFORMATION);
    }
    
    void onApply(wxCommandEvent&) {
        LOG_INF_S("=== APPLYING MESH QUALITY SETTINGS ===");
        
        // Save parameters to config
        m_paramManager->saveToConfig();
        
        // Apply to geometries
        applyParametersToGeometries();
        
        wxMessageBox("Mesh quality settings applied successfully!", "Settings Applied", wxOK | wxICON_INFORMATION);
    }
    
    void onOK(wxCommandEvent& event) {
        onApply(event);
        EndModal(wxID_OK);
    }
    
    void onCancel(wxCommandEvent&) {
        EndModal(wxID_CANCEL);
    }
};

// Factory function to create unified dialog
std::unique_ptr<wxDialog> createUnifiedMeshQualityDialog(wxWindow* parent, OCCViewer* occViewer) {
    return std::make_unique<MeshQualityDialogUnified>(parent, occViewer);
}

