#include "MeshQualityDialog.h"
#include "OCCViewer.h"
#include "logger/Logger.h"
#include <wx/statbox.h>
#include <wx/notebook.h>

MeshQualityDialog::MeshQualityDialog(wxWindow* parent, OCCViewer* occViewer)
	: wxDialog(parent, wxID_ANY, "Mesh Quality Control", wxDefaultPosition, wxSize(600, 600))
	, m_occViewer(occViewer)
	, m_notebook(nullptr)
	, m_deflectionSlider(nullptr)
	, m_deflectionSpinCtrl(nullptr)
	, m_lodEnableCheckBox(nullptr)
	, m_lodRoughDeflectionSlider(nullptr)
	, m_lodRoughDeflectionSpinCtrl(nullptr)
	, m_lodFineDeflectionSlider(nullptr)
	, m_lodFineDeflectionSpinCtrl(nullptr)
	, m_lodTransitionTimeSlider(nullptr)
	, m_lodTransitionTimeSpinCtrl(nullptr)
	, m_subdivisionEnableCheckBox(nullptr)
	, m_subdivisionLevelSlider(nullptr)
	, m_subdivisionLevelSpinCtrl(nullptr)
	, m_subdivisionMethodChoice(nullptr)
	, m_subdivisionCreaseAngleSlider(nullptr)
	, m_subdivisionCreaseAngleSpinCtrl(nullptr)
	, m_smoothingEnableCheckBox(nullptr)
	, m_smoothingMethodChoice(nullptr)
	, m_smoothingIterationsSlider(nullptr)
	, m_smoothingIterationsSpinCtrl(nullptr)
	, m_smoothingStrengthSlider(nullptr)
	, m_smoothingStrengthSpinCtrl(nullptr)
	, m_smoothingCreaseAngleSlider(nullptr)
	, m_smoothingCreaseAngleSpinCtrl(nullptr)
	, m_tessellationMethodChoice(nullptr)
	, m_tessellationQualitySlider(nullptr)
	, m_tessellationQualitySpinCtrl(nullptr)
	, m_featurePreservationSlider(nullptr)
	, m_featurePreservationSpinCtrl(nullptr)
	, m_parallelProcessingCheckBox(nullptr)
	, m_adaptiveMeshingCheckBox(nullptr)
	, m_realTimePreviewCheckBox(nullptr)
	, m_currentDeflection(0.1)
	, m_currentLODEnabled(true)
	, m_currentLODRoughDeflection(0.2)
	, m_currentLODFineDeflection(0.05)
	, m_currentLODTransitionTime(500)
	, m_enableRealTimePreview(true)
	, m_currentSubdivisionEnabled(false)
	, m_currentSubdivisionLevel(2)
	, m_currentSubdivisionMethod(0)
	, m_currentSubdivisionCreaseAngle(30.0)
	, m_currentSmoothingEnabled(false)
	, m_currentSmoothingMethod(0)
	, m_currentSmoothingIterations(2)
	, m_currentSmoothingStrength(0.5)
	, m_currentSmoothingCreaseAngle(30.0)
	, m_currentTessellationMethod(0)
	, m_currentTessellationQuality(2)
	, m_currentFeaturePreservation(0.5)
	, m_currentParallelProcessing(true)
	, m_currentAdaptiveMeshing(false)
	, m_angularDeflectionSlider(nullptr)
	, m_angularDeflectionSpinCtrl(nullptr)
	, m_currentAngularDeflection(1.0)
{
	if (!m_occViewer) {
		LOG_ERR_S("OCCViewer is null in MeshQualityDialog");
		return;
	}

	// Load current values from OCCViewer
	m_currentDeflection = m_occViewer->getMeshDeflection();
	m_currentAngularDeflection = m_occViewer->getAngularDeflection();
	m_currentLODEnabled = m_occViewer->isLODEnabled();
	m_currentLODRoughDeflection = m_occViewer->getLODRoughDeflection();
	m_currentLODFineDeflection = m_occViewer->getLODFineDeflection();
	m_currentLODTransitionTime = m_occViewer->getLODTransitionTime();

	// Subdivision values
	m_currentSubdivisionEnabled = m_occViewer->isSubdivisionEnabled();
	m_currentSubdivisionLevel = m_occViewer->getSubdivisionLevel();
	m_currentSubdivisionMethod = m_occViewer->getSubdivisionMethod();
	m_currentSubdivisionCreaseAngle = m_occViewer->getSubdivisionCreaseAngle();

	// Smoothing values
	m_currentSmoothingEnabled = m_occViewer->isSmoothingEnabled();
	m_currentSmoothingMethod = m_occViewer->getSmoothingMethod();
	m_currentSmoothingIterations = m_occViewer->getSmoothingIterations();
	m_currentSmoothingStrength = m_occViewer->getSmoothingStrength();
	m_currentSmoothingCreaseAngle = m_occViewer->getSmoothingCreaseAngle();

	// Advanced values
	m_currentTessellationMethod = m_occViewer->getTessellationMethod();
	m_currentTessellationQuality = m_occViewer->getTessellationQuality();
	m_currentFeaturePreservation = m_occViewer->getFeaturePreservation();
	m_currentParallelProcessing = m_occViewer->isParallelProcessing();
	m_currentAdaptiveMeshing = m_occViewer->isAdaptiveMeshing();

	createControls();
	layoutControls();
	bindEvents();
	updateControls();

	m_deflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onDeflectionSpinCtrl, this);
	m_angularDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onAngularDeflectionSpinCtrl, this);
	m_lodRoughDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODRoughDeflectionSpinCtrl, this);
	m_lodFineDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODFineDeflectionSpinCtrl, this);
	m_lodTransitionTimeSpinCtrl->Bind(wxEVT_SPINCTRL, &MeshQualityDialog::onLODTransitionTimeSpinCtrl, this);

	m_deflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onDeflectionSlider, this);
	m_angularDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onAngularDeflectionSlider, this);
	m_lodRoughDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODRoughDeflectionSlider, this);
	m_lodFineDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODFineDeflectionSlider, this);
	m_lodTransitionTimeSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODTransitionTimeSlider, this);
	m_lodEnableCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onLODEnable, this);
	m_realTimePreviewCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onRealTimePreviewToggle, this);
	FindWindow(wxID_APPLY)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onApply, this);
	FindWindow(wxID_CANCEL)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onCancel, this);
	FindWindow(wxID_OK)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onOK, this);

	Fit();
	SetMinSize(GetBestSize());
}

MeshQualityDialog::~MeshQualityDialog()
{
}

void MeshQualityDialog::createControls()
{
	// Create notebook for different settings pages
	m_notebook = new wxNotebook(this, wxID_ANY);

	// Create pages
	createBasicQualityPage();
	createSubdivisionPage();
	createSmoothingPage();
	createAdvancedPage();
	createSurfaceSmoothingPresetsPage();
}

void MeshQualityDialog::layoutControls()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, wxID_APPLY, "Apply"), 0, wxALL, 5);
	buttonSizer->Add(new wxButton(this, wxID_RESET, "Reset"), 0, wxALL, 5);
	buttonSizer->Add(new wxButton(this, wxID_ANY, "Validate"), 0, wxALL, 5);
	buttonSizer->Add(new wxButton(this, wxID_ANY, "Export Report"), 0, wxALL, 5);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
	buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);

	mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(mainSizer);
	Layout();
}

void MeshQualityDialog::bindEvents()
{
	// Basic quality events
	m_deflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onDeflectionSlider, this);
	m_deflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onDeflectionSpinCtrl, this);
	m_angularDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onAngularDeflectionSlider, this);
	m_angularDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onAngularDeflectionSpinCtrl, this);
	m_lodEnableCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onLODEnable, this);
	m_lodRoughDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODRoughDeflectionSlider, this);
	m_lodRoughDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODRoughDeflectionSpinCtrl, this);
	m_lodFineDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODFineDeflectionSlider, this);
	m_lodFineDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODFineDeflectionSpinCtrl, this);
	m_lodTransitionTimeSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODTransitionTimeSlider, this);
	m_lodTransitionTimeSpinCtrl->Bind(wxEVT_SPINCTRL, &MeshQualityDialog::onLODTransitionTimeSpinCtrl, this);

	// Subdivision events
	m_subdivisionEnableCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onSubdivisionEnable, this);
	m_subdivisionLevelSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onSubdivisionLevelSlider, this);
	m_subdivisionLevelSpinCtrl->Bind(wxEVT_SPINCTRL, &MeshQualityDialog::onSubdivisionLevelSpinCtrl, this);
	m_subdivisionMethodChoice->Bind(wxEVT_CHOICE, &MeshQualityDialog::onSubdivisionMethodChoice, this);
	m_subdivisionCreaseAngleSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onSubdivisionCreaseAngleSlider, this);
	m_subdivisionCreaseAngleSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onSubdivisionCreaseAngleSpinCtrl, this);

	// Smoothing events
	m_smoothingEnableCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onSmoothingEnable, this);
	m_smoothingMethodChoice->Bind(wxEVT_CHOICE, &MeshQualityDialog::onSmoothingMethodChoice, this);
	m_smoothingIterationsSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onSmoothingIterationsSlider, this);
	m_smoothingIterationsSpinCtrl->Bind(wxEVT_SPINCTRL, &MeshQualityDialog::onSmoothingIterationsSpinCtrl, this);
	m_smoothingStrengthSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onSmoothingStrengthSlider, this);
	m_smoothingStrengthSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onSmoothingStrengthSpinCtrl, this);
	m_smoothingCreaseAngleSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onSmoothingCreaseAngleSlider, this);
	m_smoothingCreaseAngleSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onSmoothingCreaseAngleSpinCtrl, this);

	// Advanced events
	m_tessellationMethodChoice->Bind(wxEVT_CHOICE, &MeshQualityDialog::onTessellationMethodChoice, this);
	m_tessellationQualitySlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onTessellationQualitySlider, this);
	m_tessellationQualitySpinCtrl->Bind(wxEVT_SPINCTRL, &MeshQualityDialog::onTessellationQualitySpinCtrl, this);
	m_featurePreservationSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onFeaturePreservationSlider, this);
	m_featurePreservationSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onFeaturePreservationSpinCtrl, this);
	m_parallelProcessingCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onParallelProcessingCheckBox, this);
	m_adaptiveMeshingCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onAdaptiveMeshingCheckBox, this);

	// Dialog events
	FindWindow(wxID_APPLY)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onApply, this);
	FindWindow(wxID_RESET)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onReset, this);
	FindWindow(wxID_CANCEL)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onCancel, this);
	FindWindow(wxID_OK)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onOK, this);

	// Bind custom buttons
	wxWindow* validateButton = FindWindow(wxID_ANY);
	if (validateButton && validateButton->GetLabel() == "Validate") {
		validateButton->Bind(wxEVT_BUTTON, &MeshQualityDialog::onValidate, this);
	}

	wxWindow* exportButton = FindWindow(wxID_ANY);
	if (exportButton && exportButton->GetLabel() == "Export Report") {
		exportButton->Bind(wxEVT_BUTTON, &MeshQualityDialog::onExportReport, this);
	}
}

void MeshQualityDialog::updateControls()
{
	// Update subdivision controls
	bool subdivisionEnabled = m_subdivisionEnableCheckBox->GetValue();
	m_subdivisionLevelSlider->Enable(subdivisionEnabled);
	m_subdivisionLevelSpinCtrl->Enable(subdivisionEnabled);
	m_subdivisionMethodChoice->Enable(subdivisionEnabled);
	m_subdivisionCreaseAngleSlider->Enable(subdivisionEnabled);
	m_subdivisionCreaseAngleSpinCtrl->Enable(subdivisionEnabled);

	// Update smoothing controls
	bool smoothingEnabled = m_smoothingEnableCheckBox->GetValue();
	m_smoothingMethodChoice->Enable(smoothingEnabled);
	m_smoothingIterationsSlider->Enable(smoothingEnabled);
	m_smoothingIterationsSpinCtrl->Enable(smoothingEnabled);
	m_smoothingStrengthSlider->Enable(smoothingEnabled);
	m_smoothingStrengthSpinCtrl->Enable(smoothingEnabled);
	m_smoothingCreaseAngleSlider->Enable(smoothingEnabled);
	m_smoothingCreaseAngleSpinCtrl->Enable(smoothingEnabled);

	// Update LOD controls
	bool lodEnabled = m_lodEnableCheckBox->GetValue();
	m_lodRoughDeflectionSlider->Enable(lodEnabled);
	m_lodRoughDeflectionSpinCtrl->Enable(lodEnabled);
	m_lodFineDeflectionSlider->Enable(lodEnabled);
	m_lodFineDeflectionSpinCtrl->Enable(lodEnabled);
	m_lodTransitionTimeSlider->Enable(lodEnabled);
	m_lodTransitionTimeSpinCtrl->Enable(lodEnabled);
}

void MeshQualityDialog::onDeflectionSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_deflectionSlider->GetValue()) / 1000.0;
	m_deflectionSpinCtrl->SetValue(value);
	m_currentDeflection = value;
	
	// Update UI only, no real-time application
	updateParameterDependencies("deflection", value);
}

void MeshQualityDialog::onDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_deflectionSpinCtrl->GetValue();
	m_deflectionSlider->SetValue(static_cast<int>(value * 1000));
	m_currentDeflection = value;
	
	// Update UI only, no real-time application
	updateParameterDependencies("deflection", value);
}

void MeshQualityDialog::onLODEnable(wxCommandEvent& event)
{
	m_currentLODEnabled = m_lodEnableCheckBox->GetValue();
	updateControls();
}

void MeshQualityDialog::onLODRoughDeflectionSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_lodRoughDeflectionSlider->GetValue()) / 1000.0;
	m_lodRoughDeflectionSpinCtrl->SetValue(value);
	m_currentLODRoughDeflection = value;
}

void MeshQualityDialog::onLODRoughDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_lodRoughDeflectionSpinCtrl->GetValue();
	m_lodRoughDeflectionSlider->SetValue(static_cast<int>(value * 1000));
	m_currentLODRoughDeflection = value;
}

void MeshQualityDialog::onLODFineDeflectionSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_lodFineDeflectionSlider->GetValue()) / 1000.0;
	m_lodFineDeflectionSpinCtrl->SetValue(value);
	m_currentLODFineDeflection = value;
}

void MeshQualityDialog::onLODFineDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_lodFineDeflectionSpinCtrl->GetValue();
	m_lodFineDeflectionSlider->SetValue(static_cast<int>(value * 1000));
	m_currentLODFineDeflection = value;
}

void MeshQualityDialog::onLODTransitionTimeSlider(wxCommandEvent& event)
{
	int value = m_lodTransitionTimeSlider->GetValue();
	m_lodTransitionTimeSpinCtrl->SetValue(value);
	m_currentLODTransitionTime = value;
}

void MeshQualityDialog::onLODTransitionTimeSpinCtrl(wxSpinEvent& event)
{
	int value = m_lodTransitionTimeSpinCtrl->GetValue();
	m_lodTransitionTimeSlider->SetValue(value);
	m_currentLODTransitionTime = value;
}

void MeshQualityDialog::onRealTimePreviewToggle(wxCommandEvent& event)
{
	m_enableRealTimePreview = m_realTimePreviewCheckBox->GetValue();
	LOG_INF_S("Real-time preview " + std::string(m_enableRealTimePreview ? "enabled" : "disabled"));
}

void MeshQualityDialog::onApply(wxCommandEvent& event)
{
	if (!m_occViewer) {
		LOG_ERR("OCCViewer is null, cannot apply settings", "MeshQualityDialog");
		return;
	}

	LOG_INF_S("=== APPLYING MESH QUALITY SETTINGS ===");
	
	// Provide user-friendly feedback based on settings
	if (m_currentDeflection >= 2.0) {
		LOG_INF_S("[PERF] Performance Mode: Using very coarse mesh for maximum speed");
		LOG_INF_S("Tip: If quality is too low, try reducing Deflection to 1.0-1.5");
	} else if (m_currentDeflection >= 1.0) {
		LOG_INF_S("[BALANCED] Balanced Mode: Good balance between quality and performance");
	} else if (m_currentDeflection >= 0.5) {
		LOG_INF_S("[QUALITY] Quality Mode: Using fine mesh for better visual quality");
	} else {
		LOG_INF_S("[ULTRA] Ultra Quality Mode: Maximum quality, may impact performance");
		LOG_INF_S("Tip: Enable LOD for better interaction responsiveness");
	}
	
	if (m_currentLODEnabled) {
		LOG_INF_S("[OK] LOD Enabled: Automatic quality adjustment during interaction");
		LOG_INF_S("  - Rough mode: " + std::to_string(m_currentLODRoughDeflection) + 
			" (used during mouse interaction)");
		LOG_INF_S("  - Fine mode: " + std::to_string(m_currentLODFineDeflection) + 
			" (used when idle)");
	}

	// Apply all settings without triggering individual remesh operations
	// This avoids throttling issues and ensures all parameters are applied together
	m_occViewer->setMeshDeflection(m_currentDeflection, false);  // Don't remesh yet
	m_occViewer->setAngularDeflection(m_currentAngularDeflection, false);  // Don't remesh yet
	m_occViewer->setLODEnabled(m_currentLODEnabled);
	m_occViewer->setLODRoughDeflection(m_currentLODRoughDeflection);
	m_occViewer->setLODFineDeflection(m_currentLODFineDeflection);
	m_occViewer->setLODTransitionTime(m_currentLODTransitionTime);

	// Apply subdivision settings
	m_occViewer->setSubdivisionEnabled(m_currentSubdivisionEnabled);
	m_occViewer->setSubdivisionLevel(m_currentSubdivisionLevel);
	m_occViewer->setSubdivisionMethod(m_currentSubdivisionMethod);
	m_occViewer->setSubdivisionCreaseAngle(m_currentSubdivisionCreaseAngle);

	// Apply smoothing settings
	m_occViewer->setSmoothingEnabled(m_currentSmoothingEnabled);
	m_occViewer->setSmoothingMethod(m_currentSmoothingMethod);
	m_occViewer->setSmoothingIterations(m_currentSmoothingIterations);
	m_occViewer->setSmoothingStrength(m_currentSmoothingStrength);
	m_occViewer->setSmoothingCreaseAngle(m_currentSmoothingCreaseAngle);

	// Apply advanced settings
	m_occViewer->setTessellationMethod(m_currentTessellationMethod);
	m_occViewer->setTessellationQuality(m_currentTessellationQuality);
	m_occViewer->setFeaturePreservation(m_currentFeaturePreservation);
	m_occViewer->setParallelProcessing(m_currentParallelProcessing);
	m_occViewer->setAdaptiveMeshing(m_currentAdaptiveMeshing);

	// Force complete remesh with all new parameters
	LOG_INF_S("Forcing complete mesh regeneration with all new parameters...");
	LOG_INF_S("Parameters: Deflection=" + std::to_string(m_currentDeflection) + 
		", Angular=" + std::to_string(m_currentAngularDeflection) +
		", Subdivision=" + std::string(m_currentSubdivisionEnabled ? "On" : "Off") +
		", Smoothing=" + std::string(m_currentSmoothingEnabled ? "On" : "Off"));
	
	// Use a small delay to ensure all parameter changes are processed
	wxMilliSleep(50);
	
	// Force regeneration of all geometries to ensure they use the latest parameters
	// This is critical to ensure Coin3D representation matches EdgeComponent mesh data
	m_occViewer->remeshAllGeometries();
	
	// Additional step: Force all geometries to regenerate their Coin3D representation
	// This ensures that the geometry faces use the same parameters as the mesh edges
	auto geometries = m_occViewer->getAllGeometry();
	for (auto& geometry : geometries) {
		if (geometry) {
			// Force regeneration by marking as needed and updating
			geometry->setMeshRegenerationNeeded(true);
			geometry->updateCoinRepresentationIfNeeded(m_occViewer->getMeshParameters());
			LOG_INF_S("Forced regeneration for geometry: " + geometry->getName());
		}
	}
	
	LOG_INF_S("Completed forced regeneration of " + std::to_string(geometries.size()) + " geometries");
	
	// Force view refresh to ensure changes are visible
	m_occViewer->requestViewRefresh();
	
	// Additional refresh mechanisms to ensure parameter changes are visible
	wxYield(); // Allow UI to process
	wxMilliSleep(100); // Give time for remesh to complete
	
	// Force another refresh to ensure low-resolution changes are visible
	m_occViewer->requestViewRefresh();

	// Validate parameters were applied
	m_occViewer->validateMeshParameters();

	LOG_INF_S("=== MESH QUALITY SETTINGS APPLIED SUCCESSFULLY ===");

	// Force immediate visual update for resolution changes
	forceImmediateVisualUpdate();

	// Show success message with current parameters
	wxString successMsg = wxString::Format(
		"Mesh quality settings have been applied successfully!\n\n"
		"Current parameters:\n"
		"- Mesh Deflection: %.3f\n"
		"- Angular Deflection: %.3f\n"
		"- LOD: %s\n"
		"- Subdivision: %s\n"
		"- Smoothing: %s\n\n"
		"Check the log for detailed information.",
		m_currentDeflection,
		m_currentAngularDeflection,
		m_currentLODEnabled ? "Enabled" : "Disabled",
		m_currentSubdivisionEnabled ? "Enabled" : "Disabled",
		m_currentSmoothingEnabled ? "Enabled" : "Disabled"
	);
	
	wxMessageBox(successMsg, "Settings Applied", wxOK | wxICON_INFORMATION);
}

void MeshQualityDialog::onReset(wxCommandEvent& event)
{
	// Reset to default values
	m_currentDeflection = 0.1;
	m_currentLODEnabled = true;
	m_currentLODRoughDeflection = 0.2;
	m_currentLODFineDeflection = 0.05;
	m_currentLODTransitionTime = 500;

	m_currentSubdivisionEnabled = false;
	m_currentSubdivisionLevel = 2;
	m_currentSubdivisionMethod = 0;
	m_currentSubdivisionCreaseAngle = 30.0;

	m_currentSmoothingEnabled = false;
	m_currentSmoothingMethod = 0;
	m_currentSmoothingIterations = 2;
	m_currentSmoothingStrength = 0.5;
	m_currentSmoothingCreaseAngle = 30.0;

	m_currentTessellationMethod = 0;
	m_currentTessellationQuality = 2;
	m_currentFeaturePreservation = 0.5;
	m_currentParallelProcessing = true;
	m_currentAdaptiveMeshing = false;

	updateControls();
}

void MeshQualityDialog::onCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void MeshQualityDialog::onOK(wxCommandEvent& event)
{
	onApply(event);
	EndModal(wxID_OK);
}

// Subdivision event handlers
void MeshQualityDialog::onSubdivisionEnable(wxCommandEvent& event)
{
	m_currentSubdivisionEnabled = m_subdivisionEnableCheckBox->GetValue();
	updateControls();
}

void MeshQualityDialog::onSubdivisionLevelSlider(wxCommandEvent& event)
{
	int value = m_subdivisionLevelSlider->GetValue();
	m_subdivisionLevelSpinCtrl->SetValue(value);
	m_currentSubdivisionLevel = value;
}

void MeshQualityDialog::onSubdivisionLevelSpinCtrl(wxSpinEvent& event)
{
	int value = m_subdivisionLevelSpinCtrl->GetValue();
	m_subdivisionLevelSlider->SetValue(value);
	m_currentSubdivisionLevel = value;
}

void MeshQualityDialog::onSubdivisionMethodChoice(wxCommandEvent& event)
{
	m_currentSubdivisionMethod = m_subdivisionMethodChoice->GetSelection();
}

void MeshQualityDialog::onSubdivisionCreaseAngleSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_subdivisionCreaseAngleSlider->GetValue());
	m_subdivisionCreaseAngleSpinCtrl->SetValue(value);
	m_currentSubdivisionCreaseAngle = value;
}

void MeshQualityDialog::onSubdivisionCreaseAngleSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_subdivisionCreaseAngleSpinCtrl->GetValue();
	m_subdivisionCreaseAngleSlider->SetValue(static_cast<int>(value));
	m_currentSubdivisionCreaseAngle = value;
}

// Smoothing event handlers
void MeshQualityDialog::onSmoothingEnable(wxCommandEvent& event)
{
	m_currentSmoothingEnabled = m_smoothingEnableCheckBox->GetValue();
	updateControls();
}

void MeshQualityDialog::onSmoothingMethodChoice(wxCommandEvent& event)
{
	m_currentSmoothingMethod = m_smoothingMethodChoice->GetSelection();
}

void MeshQualityDialog::onSmoothingIterationsSlider(wxCommandEvent& event)
{
	int value = m_smoothingIterationsSlider->GetValue();
	m_smoothingIterationsSpinCtrl->SetValue(value);
	m_currentSmoothingIterations = value;
}

void MeshQualityDialog::onSmoothingIterationsSpinCtrl(wxSpinEvent& event)
{
	int value = m_smoothingIterationsSpinCtrl->GetValue();
	m_smoothingIterationsSlider->SetValue(value);
	m_currentSmoothingIterations = value;
}

void MeshQualityDialog::onSmoothingStrengthSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_smoothingStrengthSlider->GetValue()) / 100.0;
	m_smoothingStrengthSpinCtrl->SetValue(value);
	m_currentSmoothingStrength = value;
}

void MeshQualityDialog::onSmoothingStrengthSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_smoothingStrengthSpinCtrl->GetValue();
	m_smoothingStrengthSlider->SetValue(static_cast<int>(value * 100));
	m_currentSmoothingStrength = value;
}

void MeshQualityDialog::onSmoothingCreaseAngleSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_smoothingCreaseAngleSlider->GetValue());
	m_smoothingCreaseAngleSpinCtrl->SetValue(value);
	m_currentSmoothingCreaseAngle = value;
}

void MeshQualityDialog::onSmoothingCreaseAngleSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_smoothingCreaseAngleSpinCtrl->GetValue();
	m_smoothingCreaseAngleSlider->SetValue(static_cast<int>(value));
	m_currentSmoothingCreaseAngle = value;
}

// Advanced event handlers
void MeshQualityDialog::onTessellationMethodChoice(wxCommandEvent& event)
{
	m_currentTessellationMethod = m_tessellationMethodChoice->GetSelection();
}

void MeshQualityDialog::onTessellationQualitySlider(wxCommandEvent& event)
{
	int value = m_tessellationQualitySlider->GetValue();
	m_tessellationQualitySpinCtrl->SetValue(value);
	m_currentTessellationQuality = value;
}

void MeshQualityDialog::onTessellationQualitySpinCtrl(wxSpinEvent& event)
{
	int value = m_tessellationQualitySpinCtrl->GetValue();
	m_tessellationQualitySlider->SetValue(value);
	m_currentTessellationQuality = value;
}

void MeshQualityDialog::onFeaturePreservationSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_featurePreservationSlider->GetValue()) / 100.0;
	m_featurePreservationSpinCtrl->SetValue(value);
	m_currentFeaturePreservation = value;
}

void MeshQualityDialog::onFeaturePreservationSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_featurePreservationSpinCtrl->GetValue();
	m_featurePreservationSlider->SetValue(static_cast<int>(value * 100));
	m_currentFeaturePreservation = value;
}

void MeshQualityDialog::onParallelProcessingCheckBox(wxCommandEvent& event)
{
	m_currentParallelProcessing = m_parallelProcessingCheckBox->GetValue();
}

void MeshQualityDialog::onAdaptiveMeshingCheckBox(wxCommandEvent& event)
{
	m_currentAdaptiveMeshing = m_adaptiveMeshingCheckBox->GetValue();
}

void MeshQualityDialog::onValidate(wxCommandEvent& event)
{
	if (!m_occViewer) {
		wxMessageBox("OCCViewer is not available", "Validation Error", wxOK | wxICON_ERROR);
		return;
	}

	LOG_INF_S("=== VALIDATING MESH PARAMETERS ===");

	// Validate current parameters
	m_occViewer->validateMeshParameters();
	m_occViewer->logCurrentMeshSettings();

	// Verify specific parameters
	bool deflectionOK = m_occViewer->verifyParameterApplication("deflection", m_currentDeflection);
	bool angularDeflectionOK = m_occViewer->verifyParameterApplication("angular_deflection", m_currentAngularDeflection);
	bool subdivisionLevelOK = m_occViewer->verifyParameterApplication("subdivision_level", m_currentSubdivisionLevel);
	bool smoothingIterationsOK = m_occViewer->verifyParameterApplication("smoothing_iterations", m_currentSmoothingIterations);

	// Additional checks
	bool subdivisionEnabledOK = (m_occViewer->isSubdivisionEnabled() == m_currentSubdivisionEnabled);
	bool smoothingEnabledOK = (m_occViewer->isSmoothingEnabled() == m_currentSmoothingEnabled);
	bool adaptiveMeshingOK = (m_occViewer->isAdaptiveMeshing() == m_currentAdaptiveMeshing);

	// Show detailed validation results
	std::string result = "=== MESH PARAMETER VALIDATION RESULTS ===\n\n";
	result += "Basic Parameters:\n";
	result += "  Deflection: " + std::string(deflectionOK ? " PASS" : " FAIL") +
		" (Expected: " + std::to_string(m_currentDeflection) + ")\n";
	result += "  Angular Deflection: " + std::string(angularDeflectionOK ? " PASS" : " FAIL") +
		" (Expected: " + std::to_string(m_currentAngularDeflection) + ")\n";

	result += "\nSubdivision Parameters:\n";
	result += "  Enabled: " + std::string(subdivisionEnabledOK ? " PASS" : " FAIL") +
		" (Expected: " + std::string(m_currentSubdivisionEnabled ? "true" : "false") + ")\n";
	result += "  Level: " + std::string(subdivisionLevelOK ? " PASS" : " FAIL") +
		" (Expected: " + std::to_string(m_currentSubdivisionLevel) + ")\n";

	result += "\nSmoothing Parameters:\n";
	result += "  Enabled: " + std::string(smoothingEnabledOK ? " PASS" : " FAIL") +
		" (Expected: " + std::string(m_currentSmoothingEnabled ? "true" : "false") + ")\n";
	result += "  Iterations: " + std::string(smoothingIterationsOK ? " PASS" : " FAIL") +
		" (Expected: " + std::to_string(m_currentSmoothingIterations) + ")\n";

	result += "\nAdvanced Parameters:\n";
	result += "  Adaptive Meshing: " + std::string(adaptiveMeshingOK ? " PASS" : " FAIL") +
		" (Expected: " + std::string(m_currentAdaptiveMeshing ? "true" : "false") + ")\n";

	// Count total results
	int totalChecks = 7;
	int passedChecks = (deflectionOK ? 1 : 0) + (angularDeflectionOK ? 1 : 0) +
		(subdivisionLevelOK ? 1 : 0) + (smoothingIterationsOK ? 1 : 0) +
		(subdivisionEnabledOK ? 1 : 0) + (smoothingEnabledOK ? 1 : 0) +
		(adaptiveMeshingOK ? 1 : 0);

	result += "\n=== SUMMARY ===\n";
	result += "Passed: " + std::to_string(passedChecks) + "/" + std::to_string(totalChecks) + " checks\n";

	if (passedChecks == totalChecks) {
		result += "\n All parameters applied successfully!";
		wxMessageBox(result, "Validation Success", wxOK | wxICON_INFORMATION);
	}
	else {
		result += "\n  Some parameters failed to apply correctly.\n";
		result += "Check the log for detailed information.";
		wxMessageBox(result, "Validation Warning", wxOK | wxICON_WARNING);
	}

	LOG_INF_S("Validation completed: " + std::to_string(passedChecks) + "/" + std::to_string(totalChecks) + " checks passed");
}

void MeshQualityDialog::onExportReport(wxCommandEvent& event)
{
	if (!m_occViewer) {
		wxMessageBox("OCCViewer is not available", "Export Error", wxOK | wxICON_ERROR);
		return;
	}

	// Get mesh quality report
	std::string report = m_occViewer->getMeshQualityReport();

	// Add current dialog settings to report
	report += "\nDialog Settings:\n";
	report += "- Current Deflection: " + std::to_string(m_currentDeflection) + "\n";
	report += "- LOD Enabled: " + std::string(m_currentLODEnabled ? "Yes" : "No") + "\n";
	report += "- Subdivision Enabled: " + std::string(m_currentSubdivisionEnabled ? "Yes" : "No") + "\n";
	report += "- Smoothing Enabled: " + std::string(m_currentSmoothingEnabled ? "Yes" : "No") + "\n";
	report += "- Adaptive Meshing: " + std::string(m_currentAdaptiveMeshing ? "Yes" : "No") + "\n";

	// Show report in dialog
	wxDialog* reportDialog = new wxDialog(this, wxID_ANY, "Mesh Quality Report",
		wxDefaultPosition, wxSize(500, 400));

	wxTextCtrl* textCtrl = new wxTextCtrl(reportDialog, wxID_ANY, report,
		wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(textCtrl, 1, wxEXPAND | wxALL, 10);

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(reportDialog, wxID_OK, "Close"), 0, wxALL, 5);
	sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

	reportDialog->SetSizer(sizer);
	reportDialog->ShowModal();
	reportDialog->Destroy();
}

void MeshQualityDialog::createBasicQualityPage()
{
	wxPanel* basicPage = new wxPanel(m_notebook);
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	
	// Top row: Presets and Preview options
	wxBoxSizer* topRowSizer = new wxBoxSizer(wxHORIZONTAL);
	
	// Preset buttons section (left side)
	wxStaticBox* presetBox = new wxStaticBox(basicPage, wxID_ANY, "Quick Presets");
	wxStaticBoxSizer* presetSizer = new wxStaticBoxSizer(presetBox, wxVERTICAL);
	
	// Create preset buttons with expanded options
	wxButton* performanceBtn = new wxButton(basicPage, wxID_ANY, "[P] Performance");
	wxButton* balancedBtn = new wxButton(basicPage, wxID_ANY, "[B] Balanced");
	wxButton* qualityBtn = new wxButton(basicPage, wxID_ANY, "[Q] Quality");
	wxButton* ultraBtn = new wxButton(basicPage, wxID_ANY, "[U] Ultra");
	wxButton* gamingBtn = new wxButton(basicPage, wxID_ANY, "[G] Gaming");
	wxButton* cadBtn = new wxButton(basicPage, wxID_ANY, "[C] CAD");
	wxButton* renderBtn = new wxButton(basicPage, wxID_ANY, "[R] Render");
	
	performanceBtn->SetToolTip("Maximum performance: Deflection=2.0, LOD enabled, no subdivision");
	balancedBtn->SetToolTip("Balanced settings: Deflection=1.0, LOD enabled, light smoothing");
	qualityBtn->SetToolTip("High quality: Deflection=0.5, LOD enabled, subdivision enabled");
	ultraBtn->SetToolTip("Ultra quality: Deflection=0.2, advanced features, maximum smoothness");
	gamingBtn->SetToolTip("Gaming optimized: Deflection=1.2, fast LOD, optimized for real-time");
	cadBtn->SetToolTip("CAD precision: Deflection=0.3, high accuracy, feature preservation");
	renderBtn->SetToolTip("Rendering quality: Deflection=0.1, maximum subdivision, photorealistic");
	
	performanceBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onPerformancePreset, this);
	balancedBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onBalancedPreset, this);
	qualityBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onQualityPreset, this);
	ultraBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onUltraQualityPreset, this);
	gamingBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onGamingPreset, this);
	cadBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onCADPreset, this);
	renderBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onRenderingPreset, this);
	
	// Create grid layout for preset buttons
	wxGridSizer* presetGridSizer = new wxGridSizer(2, 4, 3, 3);
	presetGridSizer->Add(performanceBtn, 0, wxEXPAND | wxALL, 2);
	presetGridSizer->Add(balancedBtn, 0, wxEXPAND | wxALL, 2);
	presetGridSizer->Add(qualityBtn, 0, wxEXPAND | wxALL, 2);
	presetGridSizer->Add(ultraBtn, 0, wxEXPAND | wxALL, 2);
	presetGridSizer->Add(gamingBtn, 0, wxEXPAND | wxALL, 2);
	presetGridSizer->Add(cadBtn, 0, wxEXPAND | wxALL, 2);
	presetGridSizer->Add(renderBtn, 0, wxEXPAND | wxALL, 2);
	presetSizer->Add(presetGridSizer, 0, wxEXPAND);
	
	// Preview options (right side)
	wxStaticBox* previewBox = new wxStaticBox(basicPage, wxID_ANY, "Preview");
	wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(previewBox, wxVERTICAL);
	
	m_realTimePreviewCheckBox = new wxCheckBox(basicPage, wxID_ANY, "Real-time Preview");
	m_realTimePreviewCheckBox->SetValue(m_enableRealTimePreview);
	m_realTimePreviewCheckBox->SetToolTip("Preview changes immediately as you adjust sliders");
	previewSizer->Add(m_realTimePreviewCheckBox, 0, wxALL, 5);
	
	topRowSizer->Add(presetSizer, 1, wxEXPAND | wxALL, 3);
	topRowSizer->Add(previewSizer, 1, wxEXPAND | wxALL, 3);
	mainSizer->Add(topRowSizer, 0, wxEXPAND | wxALL, 3);

	// Middle section: Main deflection controls in two columns
	wxBoxSizer* middleRowSizer = new wxBoxSizer(wxHORIZONTAL);
	
	// Left column: Mesh Deflection
	wxStaticBox* deflectionBox = new wxStaticBox(basicPage, wxID_ANY, "Mesh Deflection");
	wxStaticBoxSizer* deflectionSizer = new wxStaticBoxSizer(deflectionBox, wxVERTICAL);

	deflectionSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Precision (lower = higher quality):"), 0, wxALL, 2);

	m_deflectionSlider = new wxSlider(basicPage, wxID_ANY,
		static_cast<int>(m_currentDeflection * 1000), 1, 1000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);

	m_deflectionSpinCtrl = new wxSpinCtrlDouble(basicPage, wxID_ANY,
		wxString::Format("%.3f", m_currentDeflection),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.001, 1.0, m_currentDeflection, 0.001);

	deflectionSizer->Add(m_deflectionSlider, 0, wxEXPAND | wxALL, 2);
	deflectionSizer->Add(m_deflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 2);

	// Right column: Angular Deflection
	wxStaticBox* angularBox = new wxStaticBox(basicPage, wxID_ANY, "Angular Deflection");
	wxStaticBoxSizer* angularSizer = new wxStaticBoxSizer(angularBox, wxVERTICAL);

	angularSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Curve approximation:"), 0, wxALL, 2);

	m_angularDeflectionSlider = new wxSlider(basicPage, wxID_ANY,
		static_cast<int>(m_currentAngularDeflection * 1000), 100, 5000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);

	m_angularDeflectionSpinCtrl = new wxSpinCtrlDouble(basicPage, wxID_ANY,
		wxString::Format("%.3f", m_currentAngularDeflection),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.1, 5.0, m_currentAngularDeflection, 0.01);

	angularSizer->Add(m_angularDeflectionSlider, 0, wxEXPAND | wxALL, 2);
	angularSizer->Add(m_angularDeflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 2);

	middleRowSizer->Add(deflectionSizer, 1, wxEXPAND | wxALL, 3);
	middleRowSizer->Add(angularSizer, 1, wxEXPAND | wxALL, 3);
	mainSizer->Add(middleRowSizer, 0, wxEXPAND | wxALL, 3);

	// Bottom section: LOD controls (compact)
	wxStaticBox* lodBox = new wxStaticBox(basicPage, wxID_ANY, "Level of Detail (LOD)");
	wxStaticBoxSizer* lodSizer = new wxStaticBoxSizer(lodBox, wxVERTICAL);

	// LOD enable checkbox
	m_lodEnableCheckBox = new wxCheckBox(basicPage, wxID_ANY, "Enable LOD (auto quality adjustment)");
	m_lodEnableCheckBox->SetValue(m_currentLODEnabled);
	lodSizer->Add(m_lodEnableCheckBox, 0, wxALL, 2);

	// LOD parameters in two columns
	wxBoxSizer* lodParamsSizer = new wxBoxSizer(wxHORIZONTAL);
	
	// Left: Rough deflection
	wxBoxSizer* roughSizer = new wxBoxSizer(wxVERTICAL);
	roughSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Rough (interaction):"), 0, wxALL, 1);
	m_lodRoughDeflectionSlider = new wxSlider(basicPage, wxID_ANY,
		static_cast<int>(m_currentLODRoughDeflection * 1000), 1, 1000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_lodRoughDeflectionSpinCtrl = new wxSpinCtrlDouble(basicPage, wxID_ANY,
		wxString::Format("%.3f", m_currentLODRoughDeflection),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.001, 1.0, m_currentLODRoughDeflection, 0.001);
	roughSizer->Add(m_lodRoughDeflectionSlider, 0, wxEXPAND | wxALL, 1);
	roughSizer->Add(m_lodRoughDeflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 1);

	// Right: Fine deflection
	wxBoxSizer* fineSizer = new wxBoxSizer(wxVERTICAL);
	fineSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Fine (idle):"), 0, wxALL, 1);
	m_lodFineDeflectionSlider = new wxSlider(basicPage, wxID_ANY,
		static_cast<int>(m_currentLODFineDeflection * 1000), 1, 1000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_lodFineDeflectionSpinCtrl = new wxSpinCtrlDouble(basicPage, wxID_ANY,
		wxString::Format("%.3f", m_currentLODFineDeflection),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.001, 1.0, m_currentLODFineDeflection, 0.001);
	fineSizer->Add(m_lodFineDeflectionSlider, 0, wxEXPAND | wxALL, 1);
	fineSizer->Add(m_lodFineDeflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 1);

	lodParamsSizer->Add(roughSizer, 1, wxEXPAND | wxALL, 2);
	lodParamsSizer->Add(fineSizer, 1, wxEXPAND | wxALL, 2);
	lodSizer->Add(lodParamsSizer, 0, wxEXPAND | wxALL, 2);

	// Transition time (compact)
	wxBoxSizer* transitionSizer = new wxBoxSizer(wxHORIZONTAL);
	transitionSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Transition time:"), 0, wxALL, 2);
	m_lodTransitionTimeSlider = new wxSlider(basicPage, wxID_ANY,
		m_currentLODTransitionTime, 100, 2000,
		wxDefaultPosition, wxSize(120, -1), wxSL_HORIZONTAL | wxSL_LABELS);
	m_lodTransitionTimeSpinCtrl = new wxSpinCtrl(basicPage, wxID_ANY,
		wxString::Format("%d", m_currentLODTransitionTime),
		wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
		100, 2000, m_currentLODTransitionTime);
	transitionSizer->Add(m_lodTransitionTimeSlider, 0, wxALL, 2);
	transitionSizer->Add(m_lodTransitionTimeSpinCtrl, 0, wxALL, 2);
	lodSizer->Add(transitionSizer, 0, wxEXPAND | wxALL, 2);

	mainSizer->Add(lodSizer, 0, wxEXPAND | wxALL, 3);
	basicPage->SetSizer(mainSizer);
	m_notebook->AddPage(basicPage, "Basic Quality");
}

void MeshQualityDialog::createSubdivisionPage()
{
	wxPanel* subdivisionPage = new wxPanel(m_notebook);
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBox* subdivisionBox = new wxStaticBox(subdivisionPage, wxID_ANY, "Subdivision Surface");
	wxStaticBoxSizer* subdivisionSizer = new wxStaticBoxSizer(subdivisionBox, wxVERTICAL);

	subdivisionSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Create smoother, higher quality meshes:"), 0, wxALL, 2);

	m_subdivisionEnableCheckBox = new wxCheckBox(subdivisionPage, wxID_ANY, "Enable Subdivision Surfaces");
	m_subdivisionEnableCheckBox->SetValue(m_currentSubdivisionEnabled);
	subdivisionSizer->Add(m_subdivisionEnableCheckBox, 0, wxALL, 2);

	// Method and Level in one row
	wxBoxSizer* methodLevelSizer = new wxBoxSizer(wxHORIZONTAL);
	
	wxBoxSizer* methodSizer = new wxBoxSizer(wxVERTICAL);
	methodSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Method:"), 0, wxALL, 1);
	m_subdivisionMethodChoice = new wxChoice(subdivisionPage, wxID_ANY);
	m_subdivisionMethodChoice->Append("Catmull-Clark");
	m_subdivisionMethodChoice->Append("Loop");
	m_subdivisionMethodChoice->Append("Butterfly");
	m_subdivisionMethodChoice->Append("Doo-Sabin");
	m_subdivisionMethodChoice->SetSelection(m_currentSubdivisionMethod);
	methodSizer->Add(m_subdivisionMethodChoice, 0, wxEXPAND | wxALL, 1);

	wxBoxSizer* levelSizer = new wxBoxSizer(wxVERTICAL);
	levelSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Level:"), 0, wxALL, 1);
	m_subdivisionLevelSlider = new wxSlider(subdivisionPage, wxID_ANY,
		m_currentSubdivisionLevel, 1, 5,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_subdivisionLevelSpinCtrl = new wxSpinCtrl(subdivisionPage, wxID_ANY,
		wxString::Format("%d", m_currentSubdivisionLevel),
		wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
		1, 5, m_currentSubdivisionLevel);
	levelSizer->Add(m_subdivisionLevelSlider, 0, wxEXPAND | wxALL, 1);
	levelSizer->Add(m_subdivisionLevelSpinCtrl, 0, wxALIGN_CENTER | wxALL, 1);

	methodLevelSizer->Add(methodSizer, 1, wxEXPAND | wxALL, 2);
	methodLevelSizer->Add(levelSizer, 1, wxEXPAND | wxALL, 2);
	subdivisionSizer->Add(methodLevelSizer, 0, wxEXPAND | wxALL, 2);

	// Crease Angle
	subdivisionSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Crease Angle (degrees):"), 0, wxALL, 2);
	m_subdivisionCreaseAngleSlider = new wxSlider(subdivisionPage, wxID_ANY,
		static_cast<int>(m_currentSubdivisionCreaseAngle), 0, 180,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_subdivisionCreaseAngleSpinCtrl = new wxSpinCtrlDouble(subdivisionPage, wxID_ANY,
		wxString::Format("%.1f", m_currentSubdivisionCreaseAngle),
		wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
		0.0, 180.0, m_currentSubdivisionCreaseAngle, 0.1);
	subdivisionSizer->Add(m_subdivisionCreaseAngleSlider, 0, wxEXPAND | wxALL, 2);
	subdivisionSizer->Add(m_subdivisionCreaseAngleSpinCtrl, 0, wxALIGN_CENTER | wxALL, 2);

	mainSizer->Add(subdivisionSizer, 0, wxEXPAND | wxALL, 5);
	subdivisionPage->SetSizer(mainSizer);
	m_notebook->AddPage(subdivisionPage, "Subdivision");
}

void MeshQualityDialog::createSmoothingPage()
{
	wxPanel* smoothingPage = new wxPanel(m_notebook);
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBox* smoothingBox = new wxStaticBox(smoothingPage, wxID_ANY, "Mesh Smoothing");
	wxStaticBoxSizer* smoothingSizer = new wxStaticBoxSizer(smoothingBox, wxVERTICAL);

	smoothingSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Improve mesh surface quality:"), 0, wxALL, 2);

	m_smoothingEnableCheckBox = new wxCheckBox(smoothingPage, wxID_ANY, "Enable Mesh Smoothing");
	m_smoothingEnableCheckBox->SetValue(m_currentSmoothingEnabled);
	smoothingSizer->Add(m_smoothingEnableCheckBox, 0, wxALL, 2);

	// Method and Iterations in one row
	wxBoxSizer* methodIterSizer = new wxBoxSizer(wxHORIZONTAL);
	
	wxBoxSizer* methodSizer = new wxBoxSizer(wxVERTICAL);
	methodSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Method:"), 0, wxALL, 1);
	m_smoothingMethodChoice = new wxChoice(smoothingPage, wxID_ANY);
	m_smoothingMethodChoice->Append("Laplacian");
	m_smoothingMethodChoice->Append("Taubin");
	m_smoothingMethodChoice->Append("HC Laplacian");
	m_smoothingMethodChoice->Append("Bilateral");
	m_smoothingMethodChoice->SetSelection(m_currentSmoothingMethod);
	methodSizer->Add(m_smoothingMethodChoice, 0, wxEXPAND | wxALL, 1);

	wxBoxSizer* iterSizer = new wxBoxSizer(wxVERTICAL);
	iterSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Iterations:"), 0, wxALL, 1);
	m_smoothingIterationsSlider = new wxSlider(smoothingPage, wxID_ANY,
		m_currentSmoothingIterations, 1, 10,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_smoothingIterationsSpinCtrl = new wxSpinCtrl(smoothingPage, wxID_ANY,
		wxString::Format("%d", m_currentSmoothingIterations),
		wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
		1, 10, m_currentSmoothingIterations);
	iterSizer->Add(m_smoothingIterationsSlider, 0, wxEXPAND | wxALL, 1);
	iterSizer->Add(m_smoothingIterationsSpinCtrl, 0, wxALIGN_CENTER | wxALL, 1);

	methodIterSizer->Add(methodSizer, 1, wxEXPAND | wxALL, 2);
	methodIterSizer->Add(iterSizer, 1, wxEXPAND | wxALL, 2);
	smoothingSizer->Add(methodIterSizer, 0, wxEXPAND | wxALL, 2);

	// Strength and Crease Angle in one row
	wxBoxSizer* strengthCreaseSizer = new wxBoxSizer(wxHORIZONTAL);
	
	wxBoxSizer* strengthSizer = new wxBoxSizer(wxVERTICAL);
	strengthSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Strength:"), 0, wxALL, 1);
	m_smoothingStrengthSlider = new wxSlider(smoothingPage, wxID_ANY,
		static_cast<int>(m_currentSmoothingStrength * 100), 1, 100,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_smoothingStrengthSpinCtrl = new wxSpinCtrlDouble(smoothingPage, wxID_ANY,
		wxString::Format("%.2f", m_currentSmoothingStrength),
		wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
		0.01, 1.0, m_currentSmoothingStrength, 0.01);
	strengthSizer->Add(m_smoothingStrengthSlider, 0, wxEXPAND | wxALL, 1);
	strengthSizer->Add(m_smoothingStrengthSpinCtrl, 0, wxALIGN_CENTER | wxALL, 1);

	wxBoxSizer* creaseSizer = new wxBoxSizer(wxVERTICAL);
	creaseSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Crease Angle:"), 0, wxALL, 1);
	m_smoothingCreaseAngleSlider = new wxSlider(smoothingPage, wxID_ANY,
		static_cast<int>(m_currentSmoothingCreaseAngle), 0, 180,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_smoothingCreaseAngleSpinCtrl = new wxSpinCtrlDouble(smoothingPage, wxID_ANY,
		wxString::Format("%.1f", m_currentSmoothingCreaseAngle),
		wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
		0.0, 180.0, m_currentSmoothingCreaseAngle, 0.1);
	creaseSizer->Add(m_smoothingCreaseAngleSlider, 0, wxEXPAND | wxALL, 1);
	creaseSizer->Add(m_smoothingCreaseAngleSpinCtrl, 0, wxALIGN_CENTER | wxALL, 1);

	strengthCreaseSizer->Add(strengthSizer, 1, wxEXPAND | wxALL, 2);
	strengthCreaseSizer->Add(creaseSizer, 1, wxEXPAND | wxALL, 2);
	smoothingSizer->Add(strengthCreaseSizer, 0, wxEXPAND | wxALL, 2);

	mainSizer->Add(smoothingSizer, 0, wxEXPAND | wxALL, 5);
	smoothingPage->SetSizer(mainSizer);
	m_notebook->AddPage(smoothingPage, "Smoothing");
}

void MeshQualityDialog::createAdvancedPage()
{
	wxPanel* advancedPage = new wxPanel(m_notebook);
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBox* tessellationBox = new wxStaticBox(advancedPage, wxID_ANY, "Advanced Tessellation");
	wxStaticBoxSizer* tessellationSizer = new wxStaticBoxSizer(tessellationBox, wxVERTICAL);

	tessellationSizer->Add(new wxStaticText(advancedPage, wxID_ANY, "High-quality meshing controls:"), 0, wxALL, 2);

	// Method and Quality in one row
	wxBoxSizer* methodQualitySizer = new wxBoxSizer(wxHORIZONTAL);
	
	wxBoxSizer* methodSizer = new wxBoxSizer(wxVERTICAL);
	methodSizer->Add(new wxStaticText(advancedPage, wxID_ANY, "Method:"), 0, wxALL, 1);
	m_tessellationMethodChoice = new wxChoice(advancedPage, wxID_ANY);
	m_tessellationMethodChoice->Append("Standard");
	m_tessellationMethodChoice->Append("Adaptive");
	m_tessellationMethodChoice->Append("Curvature-Based");
	m_tessellationMethodChoice->Append("Feature-Based");
	m_tessellationMethodChoice->SetSelection(m_currentTessellationMethod);
	methodSizer->Add(m_tessellationMethodChoice, 0, wxEXPAND | wxALL, 1);

	wxBoxSizer* qualitySizer = new wxBoxSizer(wxVERTICAL);
	qualitySizer->Add(new wxStaticText(advancedPage, wxID_ANY, "Quality:"), 0, wxALL, 1);
	m_tessellationQualitySlider = new wxSlider(advancedPage, wxID_ANY,
		m_currentTessellationQuality, 1, 5,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_tessellationQualitySpinCtrl = new wxSpinCtrl(advancedPage, wxID_ANY,
		wxString::Format("%d", m_currentTessellationQuality),
		wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
		1, 5, m_currentTessellationQuality);
	qualitySizer->Add(m_tessellationQualitySlider, 0, wxEXPAND | wxALL, 1);
	qualitySizer->Add(m_tessellationQualitySpinCtrl, 0, wxALIGN_CENTER | wxALL, 1);

	methodQualitySizer->Add(methodSizer, 1, wxEXPAND | wxALL, 2);
	methodQualitySizer->Add(qualitySizer, 1, wxEXPAND | wxALL, 2);
	tessellationSizer->Add(methodQualitySizer, 0, wxEXPAND | wxALL, 2);

	// Feature Preservation
	tessellationSizer->Add(new wxStaticText(advancedPage, wxID_ANY, "Feature Preservation:"), 0, wxALL, 2);
	m_featurePreservationSlider = new wxSlider(advancedPage, wxID_ANY,
		static_cast<int>(m_currentFeaturePreservation * 100), 0, 100,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_featurePreservationSpinCtrl = new wxSpinCtrlDouble(advancedPage, wxID_ANY,
		wxString::Format("%.2f", m_currentFeaturePreservation),
		wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
		0.0, 1.0, m_currentFeaturePreservation, 0.01);
	tessellationSizer->Add(m_featurePreservationSlider, 0, wxEXPAND | wxALL, 2);
	tessellationSizer->Add(m_featurePreservationSpinCtrl, 0, wxALIGN_CENTER | wxALL, 2);

	// Performance options in one row
	wxBoxSizer* perfSizer = new wxBoxSizer(wxHORIZONTAL);
	m_parallelProcessingCheckBox = new wxCheckBox(advancedPage, wxID_ANY, "Parallel Processing");
	m_parallelProcessingCheckBox->SetValue(m_currentParallelProcessing);
	m_adaptiveMeshingCheckBox = new wxCheckBox(advancedPage, wxID_ANY, "Adaptive Meshing");
	m_adaptiveMeshingCheckBox->SetValue(m_currentAdaptiveMeshing);
	perfSizer->Add(m_parallelProcessingCheckBox, 0, wxALL, 2);
	perfSizer->Add(m_adaptiveMeshingCheckBox, 0, wxALL, 2);
	tessellationSizer->Add(perfSizer, 0, wxEXPAND | wxALL, 2);

	mainSizer->Add(tessellationSizer, 0, wxEXPAND | wxALL, 5);
	advancedPage->SetSizer(mainSizer);
	m_notebook->AddPage(advancedPage, "Advanced");
}

void MeshQualityDialog::createSurfaceSmoothingPresetsPage()
{
	wxPanel* surfacePage = new wxPanel(m_notebook);
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Title and description
	wxStaticText* titleText = new wxStaticText(surfacePage, wxID_ANY, "Surface Smoothing Rendering Presets");
	wxFont titleFont = titleText->GetFont();
	titleFont.MakeBold();
	titleFont.Scale(1.2f);
	titleText->SetFont(titleFont);
	mainSizer->Add(titleText, 0, wxALIGN_CENTER | wxALL, 10);

	wxStaticText* descText = new wxStaticText(surfacePage, wxID_ANY, 
		"Pre-configured parameter combinations for optimal surface rendering quality");
	mainSizer->Add(descText, 0, wxALIGN_CENTER | wxALL, 5);

	// Preset buttons grid
	wxStaticBox* presetBox = new wxStaticBox(surfacePage, wxID_ANY, "Surface Rendering Presets");
	wxStaticBoxSizer* presetSizer = new wxStaticBoxSizer(presetBox, wxVERTICAL);

	// Create preset buttons in a grid layout
	wxGridSizer* gridSizer = new wxGridSizer(2, 3, 5, 5);

	// Coarse Surface Preset
	wxButton* coarseBtn = new wxButton(surfacePage, wxID_ANY, "Coarse Surface\n[Fast Rendering]");
	coarseBtn->SetToolTip("Fast rendering with basic surface quality\n- Deflection: 1.5\n- Angular: 0.5\n- No subdivision/smoothing\n- LOD enabled");
	coarseBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onCoarseSurfacePreset, this);
	gridSizer->Add(coarseBtn, 0, wxEXPAND | wxALL, 5);

	// Standard Surface Preset
	wxButton* standardBtn = new wxButton(surfacePage, wxID_ANY, "Standard Surface\n[Balanced Quality]");
	standardBtn->SetToolTip("Balanced quality and performance\n- Deflection: 0.8\n- Angular: 0.3\n- Basic smoothing enabled\n- LOD enabled");
	standardBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onStandardSurfacePreset, this);
	gridSizer->Add(standardBtn, 0, wxEXPAND | wxALL, 5);

	// Smooth Surface Preset
	wxButton* smoothBtn = new wxButton(surfacePage, wxID_ANY, "Smooth Surface\n[High Quality]");
	smoothBtn->SetToolTip("High quality smooth surfaces\n- Deflection: 0.4\n- Angular: 0.2\n- Subdivision + Smoothing\n- LOD enabled");
	smoothBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onSmoothSurfacePreset, this);
	gridSizer->Add(smoothBtn, 0, wxEXPAND | wxALL, 5);

	// Fine Surface Preset
	wxButton* fineBtn = new wxButton(surfacePage, wxID_ANY, "Fine Surface\n[Ultra Quality]");
	fineBtn->SetToolTip("Ultra high quality surfaces\n- Deflection: 0.2\n- Angular: 0.15\n- Advanced subdivision\n- High-quality smoothing");
	fineBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onFineSurfacePreset, this);
	gridSizer->Add(fineBtn, 0, wxEXPAND | wxALL, 5);

	// Ultra Fine Surface Preset
	wxButton* ultraBtn = new wxButton(surfacePage, wxID_ANY, "Ultra Fine Surface\n[Maximum Quality]");
	ultraBtn->SetToolTip("Maximum quality for critical surfaces\n- Deflection: 0.1\n- Angular: 0.1\n- Maximum subdivision\n- Advanced smoothing");
	ultraBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onUltraFineSurfacePreset, this);
	gridSizer->Add(ultraBtn, 0, wxEXPAND | wxALL, 5);

	// Custom Surface Preset
	wxButton* customBtn = new wxButton(surfacePage, wxID_ANY, "Custom Surface\n[User Defined]");
	customBtn->SetToolTip("Apply current settings as custom preset\n- Uses current parameter values\n- Can be saved for reuse");
	customBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onCustomSurfacePreset, this);
	gridSizer->Add(customBtn, 0, wxEXPAND | wxALL, 5);

	presetSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);

	// Performance vs Quality indicator
	wxStaticBox* indicatorBox = new wxStaticBox(surfacePage, wxID_ANY, "Quality vs Performance");
	wxStaticBoxSizer* indicatorSizer = new wxStaticBoxSizer(indicatorBox, wxHORIZONTAL);

	wxStaticText* perfText = new wxStaticText(surfacePage, wxID_ANY, "Performance");
	wxStaticText* qualityText = new wxStaticText(surfacePage, wxID_ANY, "Quality");
	
	wxBoxSizer* scaleSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText* scaleText = new wxStaticText(surfacePage, wxID_ANY, "Coarse -> Standard -> Smooth -> Fine -> Ultra Fine");
	scaleText->SetForegroundColour(wxColour(100, 100, 100));
	scaleSizer->Add(scaleText, 0, wxALIGN_CENTER | wxALL, 5);

	indicatorSizer->Add(perfText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	indicatorSizer->Add(scaleSizer, 1, wxEXPAND | wxALL, 5);
	indicatorSizer->Add(qualityText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	presetSizer->Add(indicatorSizer, 0, wxEXPAND | wxALL, 5);

	mainSizer->Add(presetSizer, 0, wxEXPAND | wxALL, 10);
	surfacePage->SetSizer(mainSizer);
	m_notebook->AddPage(surfacePage, "Surface Presets");
}

// Preset handlers
void MeshQualityDialog::onPerformancePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Performance Preset");
	m_currentAngularDeflection = 2.0; // Larger angular deflection for performance
	applyPreset(2.0, true, 3.0, 1.0, true);
}

void MeshQualityDialog::onBalancedPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Balanced Preset");
	m_currentAngularDeflection = 1.0; // Medium angular deflection for balance
	applyPreset(1.0, true, 1.5, 0.5, true);
}

void MeshQualityDialog::onQualityPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Quality Preset");
	m_currentAngularDeflection = 0.5; // Small angular deflection for quality
	applyPreset(0.5, true, 0.6, 0.3, true);
}

void MeshQualityDialog::onUltraQualityPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Ultra Quality Preset");
	m_currentAngularDeflection = 0.3; // Very small angular deflection for ultra quality
	applyPreset(0.2, true, 0.4, 0.1, true);
	
	// Enable advanced features for ultra quality
	m_currentSubdivisionEnabled = true;
	m_currentSubdivisionLevel = 3;
	m_currentSmoothingEnabled = true;
	m_currentSmoothingIterations = 4;
	m_currentSmoothingStrength = 0.8;
	m_currentTessellationQuality = 4;
	m_currentFeaturePreservation = 0.9;
	m_currentAdaptiveMeshing = true;
	
	syncAllUI();
}

void MeshQualityDialog::onGamingPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Gaming Preset");
	m_currentAngularDeflection = 0.8; // Balanced for gaming performance
	applyPreset(1.2, true, 1.8, 0.6, true);
	
	// Gaming-optimized settings
	m_currentSubdivisionEnabled = false; // Disable for performance
	m_currentSmoothingEnabled = true;    // Light smoothing for visual appeal
	m_currentSmoothingIterations = 1;
	m_currentSmoothingStrength = 0.4;
	m_currentTessellationQuality = 2;
	m_currentFeaturePreservation = 0.6;
	m_currentLODTransitionTime = 200;    // Fast LOD transitions
	
	syncAllUI();
}

void MeshQualityDialog::onCADPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying CAD Preset");
	m_currentAngularDeflection = 0.4; // High precision for CAD
	applyPreset(0.3, true, 0.6, 0.15, true);
	
	// CAD-optimized settings for precision
	m_currentSubdivisionEnabled = false; // Keep original geometry intact
	m_currentSmoothingEnabled = false;   // No smoothing for accuracy
	m_currentTessellationQuality = 3;
	m_currentFeaturePreservation = 1.0;  // Maximum feature preservation
	m_currentAdaptiveMeshing = false;    // Consistent meshing
	m_currentParallelProcessing = true;  // Use parallel processing
	
	syncAllUI();
}

void MeshQualityDialog::onRenderingPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Rendering Preset");
	m_currentAngularDeflection = 0.2; // Very high precision for rendering
	applyPreset(0.1, false, 0.2, 0.05, true); // Disable LOD for consistent quality
	
	// Rendering-optimized settings for photorealistic output
	m_currentSubdivisionEnabled = true;
	m_currentSubdivisionLevel = 4;
	m_currentSubdivisionMethod = 0; // Catmull-Clark for smooth surfaces
	m_currentSmoothingEnabled = true;
	m_currentSmoothingIterations = 5;
	m_currentSmoothingStrength = 0.9;
	m_currentSmoothingCreaseAngle = 20.0; // Preserve sharp edges
	m_currentTessellationQuality = 5;
	m_currentTessellationMethod = 2; // Curvature-based tessellation
	m_currentFeaturePreservation = 1.0;
	m_currentAdaptiveMeshing = true;
	
	syncAllUI();
}

void MeshQualityDialog::applyPreset(double deflection, bool lodEnabled,
                                    double roughDeflection, double fineDeflection,
                                    bool parallelProcessing)
{
	// Update current values
	m_currentDeflection = deflection;
	m_currentLODEnabled = lodEnabled;
	m_currentLODRoughDeflection = roughDeflection;
	m_currentLODFineDeflection = fineDeflection;
	m_currentParallelProcessing = parallelProcessing;

	// Update UI controls for angular deflection
	if (m_angularDeflectionSlider) {
		m_angularDeflectionSlider->SetValue(static_cast<int>(m_currentAngularDeflection * 1000));
	}
	if (m_angularDeflectionSpinCtrl) {
		m_angularDeflectionSpinCtrl->SetValue(m_currentAngularDeflection);
	}

	// Update UI controls only, no immediate application
	syncAllUI();

	// Show feedback
	wxString msg = wxString::Format("Preset configured: Deflection=%.1f, Angular=%.1f, LOD=%s\n\nClick Apply to apply these settings.",
		deflection, m_currentAngularDeflection, lodEnabled ? "On" : "Off");
	wxMessageBox(msg, "Preset Configured", wxOK | wxICON_INFORMATION);
}

// Angular Deflection event handlers
void MeshQualityDialog::onAngularDeflectionSlider(wxCommandEvent& event)
{
	double value = static_cast<double>(m_angularDeflectionSlider->GetValue()) / 1000.0;
	m_angularDeflectionSpinCtrl->SetValue(value);
	m_currentAngularDeflection = value;
}

void MeshQualityDialog::onAngularDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_angularDeflectionSpinCtrl->GetValue();
	m_angularDeflectionSlider->SetValue(static_cast<int>(value * 1000));
	m_currentAngularDeflection = value;
}

// Surface Smoothing Preset Handlers
void MeshQualityDialog::onCoarseSurfacePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Coarse Surface Preset");
	applySurfacePreset(1.5, 0.5, false, 1, false, 1, 0.3, true, 0.3, 0.8, 1, 0.5, 0.3);
}

void MeshQualityDialog::onStandardSurfacePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Standard Surface Preset");
	applySurfacePreset(0.8, 0.3, false, 1, true, 2, 0.5, true, 0.2, 0.5, 2, 0.7, 0.4);
}

void MeshQualityDialog::onSmoothSurfacePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Smooth Surface Preset");
	applySurfacePreset(0.4, 0.2, true, 2, true, 3, 0.7, true, 0.15, 0.3, 3, 0.8, 0.6);
}

void MeshQualityDialog::onFineSurfacePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Fine Surface Preset");
	applySurfacePreset(0.2, 0.15, true, 3, true, 4, 0.8, true, 0.1, 0.2, 4, 0.9, 0.8);
}

void MeshQualityDialog::onUltraFineSurfacePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Ultra Fine Surface Preset");
	applySurfacePreset(0.1, 0.1, true, 4, true, 5, 0.9, true, 0.05, 0.1, 5, 1.0, 1.0);
}

void MeshQualityDialog::onCustomSurfacePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Custom Surface Preset");
	// Use current values as custom preset
	applySurfacePreset(m_currentDeflection, m_currentAngularDeflection,
		m_currentSubdivisionEnabled, m_currentSubdivisionLevel,
		m_currentSmoothingEnabled, m_currentSmoothingIterations,
		m_currentSmoothingStrength, m_currentLODEnabled,
		m_currentLODFineDeflection, m_currentLODRoughDeflection,
		m_currentTessellationQuality, m_currentFeaturePreservation,
		m_currentSmoothingCreaseAngle);
}

void MeshQualityDialog::applySurfacePreset(double deflection, double angularDeflection,
	bool subdivisionEnabled, int subdivisionLevel,
	bool smoothingEnabled, int smoothingIterations, double smoothingStrength,
	bool lodEnabled, double lodFineDeflection, double lodRoughDeflection,
	int tessellationQuality, double featurePreservation, double smoothingCreaseAngle)
{
	LOG_INF_S("=== APPLYING SURFACE PRESET ===");
	
	// Update current values
	m_currentDeflection = deflection;
	m_currentAngularDeflection = angularDeflection;
	m_currentSubdivisionEnabled = subdivisionEnabled;
	m_currentSubdivisionLevel = subdivisionLevel;
	m_currentSmoothingEnabled = smoothingEnabled;
	m_currentSmoothingIterations = smoothingIterations;
	m_currentSmoothingStrength = smoothingStrength;
	m_currentLODEnabled = lodEnabled;
	m_currentLODFineDeflection = lodFineDeflection;
	m_currentLODRoughDeflection = lodRoughDeflection;
	m_currentTessellationQuality = tessellationQuality;
	m_currentFeaturePreservation = featurePreservation;
	m_currentSmoothingCreaseAngle = smoothingCreaseAngle;
	
	// Set LOD transition time for smooth experience
	m_currentLODTransitionTime = 300;
	
	// Enable advanced features for high-quality surfaces
	m_currentParallelProcessing = true;
	m_currentAdaptiveMeshing = true;
	m_currentTessellationMethod = 1; // Adaptive tessellation
	m_currentSubdivisionMethod = 0; // Catmull-Clark
	m_currentSmoothingMethod = 0; // Laplacian
	
	// Update UI controls
	updateControls();
	
	// Apply to OCCViewer (preset application - no immediate remesh)
	if (m_occViewer) {
		// Basic quality settings (no remesh during preset configuration)
		m_occViewer->setMeshDeflection(m_currentDeflection, false);
		m_occViewer->setAngularDeflection(m_currentAngularDeflection, false);
		m_occViewer->setLODEnabled(m_currentLODEnabled);
		m_occViewer->setLODRoughDeflection(m_currentLODRoughDeflection);
		m_occViewer->setLODFineDeflection(m_currentLODFineDeflection);
		m_occViewer->setLODTransitionTime(m_currentLODTransitionTime);
		
		// Subdivision settings
		m_occViewer->setSubdivisionEnabled(m_currentSubdivisionEnabled);
		m_occViewer->setSubdivisionLevel(m_currentSubdivisionLevel);
		m_occViewer->setSubdivisionMethod(m_currentSubdivisionMethod);
		m_occViewer->setSubdivisionCreaseAngle(m_currentSubdivisionCreaseAngle);
		
		// Smoothing settings
		m_occViewer->setSmoothingEnabled(m_currentSmoothingEnabled);
		m_occViewer->setSmoothingMethod(m_currentSmoothingMethod);
		m_occViewer->setSmoothingIterations(m_currentSmoothingIterations);
		m_occViewer->setSmoothingStrength(m_currentSmoothingStrength);
		m_occViewer->setSmoothingCreaseAngle(m_currentSmoothingCreaseAngle);
		
		// Advanced settings
		m_occViewer->setTessellationMethod(m_currentTessellationMethod);
		m_occViewer->setTessellationQuality(m_currentTessellationQuality);
		m_occViewer->setFeaturePreservation(m_currentFeaturePreservation);
		m_occViewer->setParallelProcessing(m_currentParallelProcessing);
		m_occViewer->setAdaptiveMeshing(m_currentAdaptiveMeshing);
		
		// Note: No immediate remesh - user must click Apply to apply preset
	}
	
	// Show detailed feedback
	wxString msg = wxString::Format(
		"Surface Preset Applied:\n"
		"- Deflection: %.1f (mesh precision)\n"
		"- Angular: %.1f (curve smoothness)\n"
		"- Subdivision: %s Level %d\n"
		"- Smoothing: %s %d iterations, %.1f strength\n"
		"- LOD: %s (Fine: %.2f, Rough: %.2f)\n"
		"- Tessellation: Quality %d, Feature %.1f",
		deflection, angularDeflection,
		subdivisionEnabled ? "Enabled" : "Disabled", subdivisionLevel,
		smoothingEnabled ? "Enabled" : "Disabled", smoothingIterations, smoothingStrength,
		lodEnabled ? "Enabled" : "Disabled", lodFineDeflection, lodRoughDeflection,
		tessellationQuality, featurePreservation);
	wxMessageBox(msg, "Surface Preset Applied", wxOK | wxICON_INFORMATION);
	
	LOG_INF_S("Surface preset applied successfully");
}

// Parameter dependency and linking implementation
void MeshQualityDialog::updateParameterDependencies(const std::string& parameter, double value)
{
	if (parameter == "deflection") {
		// Auto-adjust LOD parameters based on deflection
		if (value >= 1.5) {
			// High deflection -> use more aggressive LOD
			m_currentLODRoughDeflection = std::min(value * 1.5, 2.0);
			m_currentLODFineDeflection = std::min(value * 0.8, 1.0);
		} else if (value <= 0.3) {
			// Low deflection -> use subtle LOD
			m_currentLODRoughDeflection = std::max(value * 2.0, 0.6);
			m_currentLODFineDeflection = std::max(value * 0.5, 0.1);
		} else {
			// Medium deflection -> balanced LOD
			m_currentLODRoughDeflection = value * 1.2;
			m_currentLODFineDeflection = value * 0.6;
		}
		
		// Auto-adjust angular deflection based on mesh deflection
		if (value >= 1.0) {
			m_currentAngularDeflection = 1.0 + (value - 1.0) * 0.5;
		} else {
			m_currentAngularDeflection = 1.0 - (1.0 - value) * 0.3;
		}
		
		// Auto-adjust subdivision and smoothing based on deflection
		if (value >= 1.5) {
			m_currentSubdivisionEnabled = false;
			m_currentSmoothingEnabled = false;
		} else if (value <= 0.5) {
			m_currentSubdivisionEnabled = true;
			m_currentSmoothingEnabled = true;
			m_currentSubdivisionLevel = value <= 0.2 ? 3 : 2;
			m_currentSmoothingIterations = value <= 0.2 ? 3 : 2;
		}
		
		// Update UI controls
		syncAllUI();
	}
}

void MeshQualityDialog::syncAllUI()
{
	// Update all UI controls with current values
	if (m_deflectionSlider) {
		m_deflectionSlider->SetValue(static_cast<int>(m_currentDeflection * 1000));
	}
	if (m_deflectionSpinCtrl) {
		m_deflectionSpinCtrl->SetValue(m_currentDeflection);
	}
	if (m_angularDeflectionSlider) {
		m_angularDeflectionSlider->SetValue(static_cast<int>(m_currentAngularDeflection * 1000));
	}
	if (m_angularDeflectionSpinCtrl) {
		m_angularDeflectionSpinCtrl->SetValue(m_currentAngularDeflection);
	}
	if (m_lodEnableCheckBox) {
		m_lodEnableCheckBox->SetValue(m_currentLODEnabled);
	}
	if (m_lodRoughDeflectionSlider) {
		m_lodRoughDeflectionSlider->SetValue(static_cast<int>(m_currentLODRoughDeflection * 1000));
	}
	if (m_lodRoughDeflectionSpinCtrl) {
		m_lodRoughDeflectionSpinCtrl->SetValue(m_currentLODRoughDeflection);
	}
	if (m_lodFineDeflectionSlider) {
		m_lodFineDeflectionSlider->SetValue(static_cast<int>(m_currentLODFineDeflection * 1000));
	}
	if (m_lodFineDeflectionSpinCtrl) {
		m_lodFineDeflectionSpinCtrl->SetValue(m_currentLODFineDeflection);
	}
	if (m_subdivisionEnableCheckBox) {
		m_subdivisionEnableCheckBox->SetValue(m_currentSubdivisionEnabled);
	}
	if (m_subdivisionLevelSlider) {
		m_subdivisionLevelSlider->SetValue(m_currentSubdivisionLevel);
	}
	if (m_subdivisionLevelSpinCtrl) {
		m_subdivisionLevelSpinCtrl->SetValue(m_currentSubdivisionLevel);
	}
	if (m_smoothingEnableCheckBox) {
		m_smoothingEnableCheckBox->SetValue(m_currentSmoothingEnabled);
	}
	if (m_smoothingIterationsSlider) {
		m_smoothingIterationsSlider->SetValue(m_currentSmoothingIterations);
	}
	if (m_smoothingIterationsSpinCtrl) {
		m_smoothingIterationsSpinCtrl->SetValue(m_currentSmoothingIterations);
	}
	
	// Update control states
	updateControls();
}

void MeshQualityDialog::forceImmediateVisualUpdate()
{
	if (!m_occViewer) return;
	
	LOG_INF_S("Forcing immediate visual update for resolution changes...");
	
	// Multiple refresh strategies to ensure visibility
	try {
		// 1. Force immediate remesh without throttling
		m_occViewer->remeshAllGeometries();
		
		// 2. Multiple view refresh requests
		for (int i = 0; i < 3; i++) {
			m_occViewer->requestViewRefresh();
			wxYield();
			wxMilliSleep(10);
		}
		
		// 3. Force display mode refresh
		bool currentWireframe = m_occViewer->isWireframeMode();
		m_occViewer->setWireframeMode(!currentWireframe);
		wxYield();
		m_occViewer->setWireframeMode(currentWireframe);
		
		// 4. Final refresh
		m_occViewer->requestViewRefresh();
		
		LOG_INF_S("Immediate visual update completed");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Error during immediate visual update: " + std::string(e.what()));
	}
}