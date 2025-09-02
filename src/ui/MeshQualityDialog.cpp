#include "MeshQualityDialog.h"
#include "OCCViewer.h"
#include "logger/Logger.h"
#include <wx/statbox.h>
#include <wx/notebook.h>

MeshQualityDialog::MeshQualityDialog(wxWindow* parent, OCCViewer* occViewer)
	: wxDialog(parent, wxID_ANY, "Advanced Mesh Quality Control", wxDefaultPosition, wxSize(600, 700))
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
	, m_currentDeflection(0.1)
	, m_currentLODEnabled(true)
	, m_currentLODRoughDeflection(0.2)
	, m_currentLODFineDeflection(0.05)
	, m_currentLODTransitionTime(500)
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
{
	if (!m_occViewer) {
		LOG_ERR_S("OCCViewer is null in MeshQualityDialog");
		return;
	}

	// Load current values from OCCViewer
	m_currentDeflection = m_occViewer->getMeshDeflection();
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
	m_lodRoughDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODRoughDeflectionSpinCtrl, this);
	m_lodFineDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODFineDeflectionSpinCtrl, this);
	m_lodTransitionTimeSpinCtrl->Bind(wxEVT_SPINCTRL, &MeshQualityDialog::onLODTransitionTimeSpinCtrl, this);

	m_deflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onDeflectionSlider, this);
	m_lodRoughDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODRoughDeflectionSlider, this);
	m_lodFineDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODFineDeflectionSlider, this);
	m_lodTransitionTimeSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODTransitionTimeSlider, this);
	m_lodEnableCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onLODEnable, this);
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
}

void MeshQualityDialog::onDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
	double value = m_deflectionSpinCtrl->GetValue();
	m_deflectionSlider->SetValue(static_cast<int>(value * 1000));
	m_currentDeflection = value;
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

	// Apply basic quality settings
	m_occViewer->setMeshDeflection(m_currentDeflection, true);
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

	// Force remesh all geometries with new parameters
	LOG_INF_S("Forcing mesh regeneration with new parameters...");
	m_occViewer->remeshAllGeometries();

	// Validate parameters were applied
	m_occViewer->validateMeshParameters();

	LOG_INF_S("=== MESH QUALITY SETTINGS APPLIED SUCCESSFULLY ===");

	// Show success message
	wxMessageBox("Mesh quality settings have been applied successfully!\n\n"
		"Check the log for detailed information about the applied parameters.",
		"Settings Applied", wxOK | wxICON_INFORMATION);
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
	int totalChecks = 6;
	int passedChecks = (deflectionOK ? 1 : 0) + (subdivisionLevelOK ? 1 : 0) +
		(smoothingIterationsOK ? 1 : 0) + (subdivisionEnabledOK ? 1 : 0) +
		(smoothingEnabledOK ? 1 : 0) + (adaptiveMeshingOK ? 1 : 0);

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
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	
	// Preset buttons section
	wxStaticBox* presetBox = new wxStaticBox(basicPage, wxID_ANY, "Quick Presets");
	wxStaticBoxSizer* presetSizer = new wxStaticBoxSizer(presetBox, wxHORIZONTAL);
	
	wxButton* performanceBtn = new wxButton(basicPage, wxID_ANY, "[P] Performance");
	wxButton* balancedBtn = new wxButton(basicPage, wxID_ANY, "[B] Balanced");
	wxButton* qualityBtn = new wxButton(basicPage, wxID_ANY, "[Q] Quality");
	
	performanceBtn->SetToolTip("Maximum performance: Deflection=2.0, LOD enabled");
	balancedBtn->SetToolTip("Balanced settings: Deflection=1.0, LOD enabled");
	qualityBtn->SetToolTip("High quality: Deflection=0.2, LOD enabled");
	
	performanceBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onPerformancePreset, this);
	balancedBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onBalancedPreset, this);
	qualityBtn->Bind(wxEVT_BUTTON, &MeshQualityDialog::onQualityPreset, this);
	
	presetSizer->Add(performanceBtn, 0, wxALL, 5);
	presetSizer->Add(balancedBtn, 0, wxALL, 5);
	presetSizer->Add(qualityBtn, 0, wxALL, 5);
	
	sizer->Add(presetSizer, 0, wxEXPAND | wxALL, 5);

	// Mesh Deflection section
	wxStaticBox* deflectionBox = new wxStaticBox(basicPage, wxID_ANY, "Mesh Deflection");
	wxStaticBoxSizer* deflectionSizer = new wxStaticBoxSizer(deflectionBox, wxVERTICAL);

	deflectionSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Deflection controls mesh precision (lower = higher quality):"), 0, wxALL, 5);

	m_deflectionSlider = new wxSlider(basicPage, wxID_ANY,
		static_cast<int>(m_currentDeflection * 1000), 1, 1000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);

	m_deflectionSpinCtrl = new wxSpinCtrlDouble(basicPage, wxID_ANY,
		wxString::Format("%.3f", m_currentDeflection),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.001, 1.0, m_currentDeflection, 0.001);

	deflectionSizer->Add(m_deflectionSlider, 0, wxEXPAND | wxALL, 5);
	deflectionSizer->Add(m_deflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(deflectionSizer, 0, wxEXPAND | wxALL, 10);

	// LOD section
	wxStaticBox* lodBox = new wxStaticBox(basicPage, wxID_ANY, "Level of Detail (LOD)");
	wxStaticBoxSizer* lodSizer = new wxStaticBoxSizer(lodBox, wxVERTICAL);

	lodSizer->Add(new wxStaticText(basicPage, wxID_ANY, "LOD automatically adjusts mesh quality during interaction:"), 0, wxALL, 5);

	m_lodEnableCheckBox = new wxCheckBox(basicPage, wxID_ANY, "Enable Level of Detail (LOD)");
	m_lodEnableCheckBox->SetValue(m_currentLODEnabled);
	lodSizer->Add(m_lodEnableCheckBox, 0, wxALL, 5);

	lodSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Rough deflection (during interaction):"), 0, wxALL, 5);
	m_lodRoughDeflectionSlider = new wxSlider(basicPage, wxID_ANY,
		static_cast<int>(m_currentLODRoughDeflection * 1000), 1, 1000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_lodRoughDeflectionSpinCtrl = new wxSpinCtrlDouble(basicPage, wxID_ANY,
		wxString::Format("%.3f", m_currentLODRoughDeflection),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.001, 1.0, m_currentLODRoughDeflection, 0.001);
	lodSizer->Add(m_lodRoughDeflectionSlider, 0, wxEXPAND | wxALL, 5);
	lodSizer->Add(m_lodRoughDeflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	lodSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Fine deflection (after interaction):"), 0, wxALL, 5);
	m_lodFineDeflectionSlider = new wxSlider(basicPage, wxID_ANY,
		static_cast<int>(m_currentLODFineDeflection * 1000), 1, 1000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_lodFineDeflectionSpinCtrl = new wxSpinCtrlDouble(basicPage, wxID_ANY,
		wxString::Format("%.3f", m_currentLODFineDeflection),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.001, 1.0, m_currentLODFineDeflection, 0.001);
	lodSizer->Add(m_lodFineDeflectionSlider, 0, wxEXPAND | wxALL, 5);
	lodSizer->Add(m_lodFineDeflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	lodSizer->Add(new wxStaticText(basicPage, wxID_ANY, "Transition time (milliseconds):"), 0, wxALL, 5);
	m_lodTransitionTimeSlider = new wxSlider(basicPage, wxID_ANY,
		m_currentLODTransitionTime, 100, 2000,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_lodTransitionTimeSpinCtrl = new wxSpinCtrl(basicPage, wxID_ANY,
		wxString::Format("%d", m_currentLODTransitionTime),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		100, 2000, m_currentLODTransitionTime);
	lodSizer->Add(m_lodTransitionTimeSlider, 0, wxEXPAND | wxALL, 5);
	lodSizer->Add(m_lodTransitionTimeSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	sizer->Add(lodSizer, 0, wxEXPAND | wxALL, 10);
	basicPage->SetSizer(sizer);
	m_notebook->AddPage(basicPage, "Basic Quality");
}

void MeshQualityDialog::createSubdivisionPage()
{
	wxPanel* subdivisionPage = new wxPanel(m_notebook);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBox* subdivisionBox = new wxStaticBox(subdivisionPage, wxID_ANY, "Subdivision Surface");
	wxStaticBoxSizer* subdivisionSizer = new wxStaticBoxSizer(subdivisionBox, wxVERTICAL);

	subdivisionSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Subdivision surfaces create smoother, higher quality meshes:"), 0, wxALL, 5);

	m_subdivisionEnableCheckBox = new wxCheckBox(subdivisionPage, wxID_ANY, "Enable Subdivision Surfaces");
	m_subdivisionEnableCheckBox->SetValue(m_currentSubdivisionEnabled);
	subdivisionSizer->Add(m_subdivisionEnableCheckBox, 0, wxALL, 5);

	subdivisionSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Subdivision Method:"), 0, wxALL, 5);
	m_subdivisionMethodChoice = new wxChoice(subdivisionPage, wxID_ANY);
	m_subdivisionMethodChoice->Append("Catmull-Clark");
	m_subdivisionMethodChoice->Append("Loop");
	m_subdivisionMethodChoice->Append("Butterfly");
	m_subdivisionMethodChoice->Append("Doo-Sabin");
	m_subdivisionMethodChoice->SetSelection(m_currentSubdivisionMethod);
	subdivisionSizer->Add(m_subdivisionMethodChoice, 0, wxEXPAND | wxALL, 5);

	subdivisionSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Subdivision Levels:"), 0, wxALL, 5);
	m_subdivisionLevelSlider = new wxSlider(subdivisionPage, wxID_ANY,
		m_currentSubdivisionLevel, 1, 5,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_subdivisionLevelSpinCtrl = new wxSpinCtrl(subdivisionPage, wxID_ANY,
		wxString::Format("%d", m_currentSubdivisionLevel),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		1, 5, m_currentSubdivisionLevel);
	subdivisionSizer->Add(m_subdivisionLevelSlider, 0, wxEXPAND | wxALL, 5);
	subdivisionSizer->Add(m_subdivisionLevelSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	subdivisionSizer->Add(new wxStaticText(subdivisionPage, wxID_ANY, "Crease Angle (degrees):"), 0, wxALL, 5);
	m_subdivisionCreaseAngleSlider = new wxSlider(subdivisionPage, wxID_ANY,
		static_cast<int>(m_currentSubdivisionCreaseAngle), 0, 180,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_subdivisionCreaseAngleSpinCtrl = new wxSpinCtrlDouble(subdivisionPage, wxID_ANY,
		wxString::Format("%.1f", m_currentSubdivisionCreaseAngle),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.0, 180.0, m_currentSubdivisionCreaseAngle, 0.1);
	subdivisionSizer->Add(m_subdivisionCreaseAngleSlider, 0, wxEXPAND | wxALL, 5);
	subdivisionSizer->Add(m_subdivisionCreaseAngleSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	sizer->Add(subdivisionSizer, 0, wxEXPAND | wxALL, 10);
	subdivisionPage->SetSizer(sizer);
	m_notebook->AddPage(subdivisionPage, "Subdivision");
}

void MeshQualityDialog::createSmoothingPage()
{
	wxPanel* smoothingPage = new wxPanel(m_notebook);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBox* smoothingBox = new wxStaticBox(smoothingPage, wxID_ANY, "Mesh Smoothing");
	wxStaticBoxSizer* smoothingSizer = new wxStaticBoxSizer(smoothingBox, wxVERTICAL);

	smoothingSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Smoothing algorithms improve mesh surface quality:"), 0, wxALL, 5);

	m_smoothingEnableCheckBox = new wxCheckBox(smoothingPage, wxID_ANY, "Enable Mesh Smoothing");
	m_smoothingEnableCheckBox->SetValue(m_currentSmoothingEnabled);
	smoothingSizer->Add(m_smoothingEnableCheckBox, 0, wxALL, 5);

	smoothingSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Smoothing Method:"), 0, wxALL, 5);
	m_smoothingMethodChoice = new wxChoice(smoothingPage, wxID_ANY);
	m_smoothingMethodChoice->Append("Laplacian");
	m_smoothingMethodChoice->Append("Taubin");
	m_smoothingMethodChoice->Append("HC Laplacian");
	m_smoothingMethodChoice->Append("Bilateral");
	m_smoothingMethodChoice->SetSelection(m_currentSmoothingMethod);
	smoothingSizer->Add(m_smoothingMethodChoice, 0, wxEXPAND | wxALL, 5);

	smoothingSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Smoothing Iterations:"), 0, wxALL, 5);
	m_smoothingIterationsSlider = new wxSlider(smoothingPage, wxID_ANY,
		m_currentSmoothingIterations, 1, 10,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_smoothingIterationsSpinCtrl = new wxSpinCtrl(smoothingPage, wxID_ANY,
		wxString::Format("%d", m_currentSmoothingIterations),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		1, 10, m_currentSmoothingIterations);
	smoothingSizer->Add(m_smoothingIterationsSlider, 0, wxEXPAND | wxALL, 5);
	smoothingSizer->Add(m_smoothingIterationsSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	smoothingSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Smoothing Strength:"), 0, wxALL, 5);
	m_smoothingStrengthSlider = new wxSlider(smoothingPage, wxID_ANY,
		static_cast<int>(m_currentSmoothingStrength * 100), 1, 100,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_smoothingStrengthSpinCtrl = new wxSpinCtrlDouble(smoothingPage, wxID_ANY,
		wxString::Format("%.2f", m_currentSmoothingStrength),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.01, 1.0, m_currentSmoothingStrength, 0.01);
	smoothingSizer->Add(m_smoothingStrengthSlider, 0, wxEXPAND | wxALL, 5);
	smoothingSizer->Add(m_smoothingStrengthSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	smoothingSizer->Add(new wxStaticText(smoothingPage, wxID_ANY, "Crease Angle (degrees):"), 0, wxALL, 5);
	m_smoothingCreaseAngleSlider = new wxSlider(smoothingPage, wxID_ANY,
		static_cast<int>(m_currentSmoothingCreaseAngle), 0, 180,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_smoothingCreaseAngleSpinCtrl = new wxSpinCtrlDouble(smoothingPage, wxID_ANY,
		wxString::Format("%.1f", m_currentSmoothingCreaseAngle),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.0, 180.0, m_currentSmoothingCreaseAngle, 0.1);
	smoothingSizer->Add(m_smoothingCreaseAngleSlider, 0, wxEXPAND | wxALL, 5);
	smoothingSizer->Add(m_smoothingCreaseAngleSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	sizer->Add(smoothingSizer, 0, wxEXPAND | wxALL, 10);
	smoothingPage->SetSizer(sizer);
	m_notebook->AddPage(smoothingPage, "Smoothing");
}

void MeshQualityDialog::createAdvancedPage()
{
	wxPanel* advancedPage = new wxPanel(m_notebook);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBox* tessellationBox = new wxStaticBox(advancedPage, wxID_ANY, "Advanced Tessellation");
	wxStaticBoxSizer* tessellationSizer = new wxStaticBoxSizer(tessellationBox, wxVERTICAL);

	tessellationSizer->Add(new wxStaticText(advancedPage, wxID_ANY, "Advanced tessellation controls for high-quality meshing:"), 0, wxALL, 5);

	tessellationSizer->Add(new wxStaticText(advancedPage, wxID_ANY, "Tessellation Method:"), 0, wxALL, 5);
	m_tessellationMethodChoice = new wxChoice(advancedPage, wxID_ANY);
	m_tessellationMethodChoice->Append("Standard");
	m_tessellationMethodChoice->Append("Adaptive");
	m_tessellationMethodChoice->Append("Curvature-Based");
	m_tessellationMethodChoice->Append("Feature-Based");
	m_tessellationMethodChoice->SetSelection(m_currentTessellationMethod);
	tessellationSizer->Add(m_tessellationMethodChoice, 0, wxEXPAND | wxALL, 5);

	tessellationSizer->Add(new wxStaticText(advancedPage, wxID_ANY, "Tessellation Quality:"), 0, wxALL, 5);
	m_tessellationQualitySlider = new wxSlider(advancedPage, wxID_ANY,
		m_currentTessellationQuality, 1, 5,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_tessellationQualitySpinCtrl = new wxSpinCtrl(advancedPage, wxID_ANY,
		wxString::Format("%d", m_currentTessellationQuality),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		1, 5, m_currentTessellationQuality);
	tessellationSizer->Add(m_tessellationQualitySlider, 0, wxEXPAND | wxALL, 5);
	tessellationSizer->Add(m_tessellationQualitySpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	tessellationSizer->Add(new wxStaticText(advancedPage, wxID_ANY, "Feature Preservation:"), 0, wxALL, 5);
	m_featurePreservationSlider = new wxSlider(advancedPage, wxID_ANY,
		static_cast<int>(m_currentFeaturePreservation * 100), 0, 100,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	m_featurePreservationSpinCtrl = new wxSpinCtrlDouble(advancedPage, wxID_ANY,
		wxString::Format("%.2f", m_currentFeaturePreservation),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.0, 1.0, m_currentFeaturePreservation, 0.01);
	tessellationSizer->Add(m_featurePreservationSlider, 0, wxEXPAND | wxALL, 5);
	tessellationSizer->Add(m_featurePreservationSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);

	m_parallelProcessingCheckBox = new wxCheckBox(advancedPage, wxID_ANY, "Enable Parallel Processing");
	m_parallelProcessingCheckBox->SetValue(m_currentParallelProcessing);
	tessellationSizer->Add(m_parallelProcessingCheckBox, 0, wxALL, 5);

	m_adaptiveMeshingCheckBox = new wxCheckBox(advancedPage, wxID_ANY, "Enable Adaptive Meshing");
	m_adaptiveMeshingCheckBox->SetValue(m_currentAdaptiveMeshing);
	tessellationSizer->Add(m_adaptiveMeshingCheckBox, 0, wxALL, 5);

	sizer->Add(tessellationSizer, 0, wxEXPAND | wxALL, 10);
	advancedPage->SetSizer(sizer);
	m_notebook->AddPage(advancedPage, "Advanced");
}

// Preset handlers
void MeshQualityDialog::onPerformancePreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Performance Preset");
	applyPreset(2.0, true, 3.0, 1.0, true);
}

void MeshQualityDialog::onBalancedPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Balanced Preset");
	applyPreset(1.0, true, 1.5, 0.5, true);
}

void MeshQualityDialog::onQualityPreset(wxCommandEvent& event)
{
	LOG_INF_S("Applying Quality Preset");
	applyPreset(0.2, true, 0.5, 0.1, true);
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
	
	// Update UI controls
	updateControls();
	
	// Apply immediately
	if (m_occViewer) {
		m_occViewer->setMeshDeflection(m_currentDeflection, true);
		m_occViewer->setLODEnabled(m_currentLODEnabled);
		m_occViewer->setLODRoughDeflection(m_currentLODRoughDeflection);
		m_occViewer->setLODFineDeflection(m_currentLODFineDeflection);
		m_occViewer->setParallelProcessing(m_currentParallelProcessing);
		
		// Trigger remesh
		m_occViewer->remeshAllGeometries();
	}
	
	// Show feedback
	wxString msg = wxString::Format("Preset applied: Deflection=%.1f, LOD=%s", 
		deflection, lodEnabled ? "On" : "Off");
	wxMessageBox(msg, "Preset Applied", wxOK | wxICON_INFORMATION);
}