#include "LayoutStrategyDialog.h"
#include "ModernDockManager.h"
#include "UnifiedDockTypes.h"
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(LayoutStrategyDialog, wxDialog)
EVT_CHOICE(wxID_ANY, LayoutStrategyDialog::OnStrategyChanged)
EVT_BUTTON(wxID_APPLY, LayoutStrategyDialog::OnApplyClicked)
EVT_BUTTON(wxID_CANCEL, LayoutStrategyDialog::OnCancelClicked)
EVT_BUTTON(wxID_OK, LayoutStrategyDialog::OnOKClicked)
wxEND_EVENT_TABLE()

LayoutStrategyDialog::LayoutStrategyDialog(wxWindow* parent, ModernDockManager* dockManager)
	: wxDialog(parent, wxID_ANY, "Layout Strategy Configuration",
		wxDefaultPosition, wxSize(500, 400),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	m_dockManager(dockManager)
{
	InitializeUI();

	// Get available strategies (hardcoded for now)
	m_availableStrategies.clear();
	m_availableStrategies.push_back("IDE");
	m_availableStrategies.push_back("Flexible");
	m_availableStrategies.push_back("Hybrid");

	// Populate strategy choice
	for (const auto& strategy : m_availableStrategies) {
		m_strategyChoice->Append(wxString(strategy));
	}

	// Select current strategy (get from dock manager if available)
	if (m_dockManager) {
		auto currentStrategy = m_dockManager->GetLayoutStrategy();
		std::string currentStrategyName = "IDE"; // Default
		switch (currentStrategy) {
		case LayoutStrategy::IDE: currentStrategyName = "IDE"; break;
		case LayoutStrategy::Flexible: currentStrategyName = "Flexible"; break;
		case LayoutStrategy::Hybrid: currentStrategyName = "Hybrid"; break;
		default: currentStrategyName = "IDE"; break;
		}

		int index = m_strategyChoice->FindString(wxString(currentStrategyName));
		if (index != wxNOT_FOUND) {
			m_strategyChoice->SetSelection(index);
		}

		// Set current settings
		m_cachingCheckBox->SetValue(m_dockManager->GetAutoSaveLayout());
		m_optimizationCheckBox->SetValue(true); // Default to true
		m_updateModeSpin->SetValue(m_dockManager->GetLayoutUpdateInterval());
	}
	else {
		m_strategyChoice->SetSelection(0); // Default to first option
		m_cachingCheckBox->SetValue(true);
		m_optimizationCheckBox->SetValue(true);
		m_updateModeSpin->SetValue(100);
	}

	UpdateStrategyDescription();
}

void LayoutStrategyDialog::InitializeUI()
{
	// Main sizer
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Strategy selection section
	wxStaticBoxSizer* strategySizer = new wxStaticBoxSizer(wxVERTICAL, this, "Layout Strategy");

	wxBoxSizer* choiceSizer = new wxBoxSizer(wxHORIZONTAL);
	choiceSizer->Add(new wxStaticText(this, wxID_ANY, "Strategy:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

	m_strategyChoice = new wxChoice(this, wxID_ANY);
	choiceSizer->Add(m_strategyChoice, 1, wxEXPAND);

	strategySizer->Add(choiceSizer, 0, wxEXPAND | wxALL, 10);

	// Strategy description
	m_descriptionText = new wxStaticText(this, wxID_ANY,
		"Select a layout strategy to configure how panels are arranged in the docking system.");
	m_descriptionText->Wrap(450);
	strategySizer->Add(m_descriptionText, 0, wxEXPAND | wxALL, 10);

	mainSizer->Add(strategySizer, 0, wxEXPAND | wxALL, 10);

	// Performance options section
	wxStaticBoxSizer* performanceSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Performance Options");

	m_cachingCheckBox = new wxCheckBox(this, wxID_ANY, "Enable Layout Caching");
	m_cachingCheckBox->SetValue(true);
	performanceSizer->Add(m_cachingCheckBox, 0, wxALL, 10);

	m_optimizationCheckBox = new wxCheckBox(this, wxID_ANY, "Enable Layout Optimization");
	m_optimizationCheckBox->SetValue(true);
	performanceSizer->Add(m_optimizationCheckBox, 0, wxALL, 10);

	wxBoxSizer* updateModeSizer = new wxBoxSizer(wxHORIZONTAL);
	m_updateModeLabel = new wxStaticText(this, wxID_ANY, "Layout Update Mode:");
	updateModeSizer->Add(m_updateModeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

	m_updateModeSpin = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
		wxSP_ARROW_KEYS, 0, 3, 0);
	m_updateModeSpin->SetToolTip("0: Immediate, 1: Deferred, 2: Batched, 3: Manual");
	updateModeSizer->Add(m_updateModeSpin, 0, wxEXPAND);

	performanceSizer->Add(updateModeSizer, 0, wxEXPAND | wxALL, 10);

	mainSizer->Add(performanceSizer, 0, wxEXPAND | wxALL, 10);

	// Button sizer
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

	m_applyButton = new wxButton(this, wxID_APPLY, "Apply");
	m_okButton = new wxButton(this, wxID_OK, "OK");
	m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

	buttonSizer->Add(m_applyButton, 0, wxRIGHT, 10);
	buttonSizer->Add(m_okButton, 0, wxRIGHT, 10);
	buttonSizer->Add(m_cancelButton, 0);

	mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

	SetSizer(mainSizer);

	// Set default button
	m_okButton->SetDefault();

	// Update initial state
	UpdateParameterVisibility();
}

void LayoutStrategyDialog::OnStrategyChanged(wxCommandEvent& event)
{
	wxUnusedVar(event);
	UpdateStrategyDescription();
	UpdateParameterVisibility();
}

void LayoutStrategyDialog::OnApplyClicked(wxCommandEvent& event)
{
	wxUnusedVar(event);
	if (!m_dockManager) {
		wxMessageBox("No dock manager available", "Error", wxOK | wxICON_ERROR);
		return;
	}

	try {
		// Get selected strategy
		int selection = m_strategyChoice->GetSelection();
		if (selection == wxNOT_FOUND) {
			wxMessageBox("Please select a layout strategy", "Error", wxOK | wxICON_ERROR);
			return;
		}

		std::string strategyName = m_availableStrategies[selection];

		// Apply strategy
		LayoutStrategy strategy = LayoutStrategy::IDE; // Default
		if (strategyName == "IDE") {
			strategy = LayoutStrategy::IDE;
		}
		else if (strategyName == "Flexible") {
			strategy = LayoutStrategy::Flexible;
		}
		else if (strategyName == "Hybrid") {
			strategy = LayoutStrategy::Hybrid;
		}
		else if (strategyName == "Fixed") {
			strategy = LayoutStrategy::Fixed;
		}

		m_dockManager->SetLayoutStrategy(strategy);

		// Apply performance options
		m_dockManager->EnableLayoutCaching(m_cachingCheckBox->GetValue());
		LayoutUpdateMode updateMode = LayoutUpdateMode::Immediate; // Default
		int updateValue = m_updateModeSpin->GetValue();
		if (updateValue == 0) {
			updateMode = LayoutUpdateMode::Immediate;
		}
		else if (updateValue == 1) {
			updateMode = LayoutUpdateMode::Deferred;
		}
		else if (updateValue == 2) {
			updateMode = LayoutUpdateMode::Lazy;
		}
		else if (updateValue == 3) {
			updateMode = LayoutUpdateMode::Manual;
		}
		m_dockManager->SetLayoutUpdateMode(updateMode);

		if (m_optimizationCheckBox->GetValue()) {
			m_dockManager->OptimizeLayout();
		}

		wxMessageBox("Layout strategy applied successfully", "Success", wxOK | wxICON_INFORMATION);
	}
	catch (const std::exception& e) {
		wxMessageBox(wxString::Format("Error applying strategy: %s", e.what()),
			"Error", wxOK | wxICON_ERROR);
	}
}

void LayoutStrategyDialog::OnCancelClicked(wxCommandEvent& event)
{
	wxUnusedVar(event);
	EndModal(wxID_CANCEL);
}

void LayoutStrategyDialog::OnOKClicked(wxCommandEvent& event)
{
	wxUnusedVar(event);
	// Apply changes first
	wxCommandEvent applyEvent;
	OnApplyClicked(applyEvent);

	// Then close dialog
	EndModal(wxID_OK);
}

void LayoutStrategyDialog::UpdateStrategyDescription()
{
	int selection = m_strategyChoice->GetSelection();
	if (selection == wxNOT_FOUND) {
		m_descriptionText->SetLabel("Please select a layout strategy.");
		return;
	}

	std::string strategyName = m_availableStrategies[selection];

	if (strategyName == "IDE") {
		m_descriptionText->SetLabel(
			"IDE Layout: Traditional IDE-style layout with fixed areas for left sidebar, "
			"center canvas, and bottom panel. Provides a familiar and predictable arrangement "
			"suitable for development environments."
		);
	}
	else if (strategyName == "Flexible") {
		m_descriptionText->SetLabel(
			"Flexible Layout: Dynamic tree-based layout that allows panels to be placed "
			"anywhere and automatically creates necessary splitters and containers. "
			"Provides maximum flexibility for custom arrangements."
		);
	}
	else if (strategyName == "Hybrid") {
		m_descriptionText->SetLabel(
			"Hybrid Layout: Combines the structured approach of IDE layouts with the "
			"flexibility of dynamic layouts. Uses IDE strategy for main areas and "
			"flexible strategy for secondary areas."
		);
	}
	else {
		m_descriptionText->SetLabel(
			"Custom layout strategy with specialized behavior for specific use cases."
		);
	}

	m_descriptionText->Wrap(450);
	Layout();
}

void LayoutStrategyDialog::UpdateParameterVisibility()
{
	int selection = m_strategyChoice->GetSelection();
	if (selection == wxNOT_FOUND) {
		return;
	}

	std::string strategyName = m_availableStrategies[selection];

	// Show/hide performance options based on strategy
	bool showPerformance = true;

	if (strategyName == "IDE") {
		// IDE layout is optimized by default
		m_cachingCheckBox->SetValue(true);
		m_optimizationCheckBox->SetValue(true);
		m_updateModeSpin->SetValue(0); // Immediate updates
	}
	else if (strategyName == "Flexible") {
		// Flexible layout benefits from caching and optimization
		m_cachingCheckBox->SetValue(true);
		m_optimizationCheckBox->SetValue(true);
		m_updateModeSpin->SetValue(1); // Deferred updates
	}
	else if (strategyName == "Hybrid") {
		// Hybrid layout uses balanced settings
		m_cachingCheckBox->SetValue(true);
		m_optimizationCheckBox->SetValue(true);
		m_updateModeSpin->SetValue(1); // Deferred updates
	}

	// Update UI state
	m_cachingCheckBox->Enable(showPerformance);
	m_optimizationCheckBox->Enable(showPerformance);
	m_updateModeSpin->Enable(showPerformance);
	m_updateModeLabel->Enable(showPerformance);
}

std::string LayoutStrategyDialog::GetSelectedStrategy() const
{
	int selection = m_strategyChoice->GetSelection();
	if (selection != wxNOT_FOUND && selection < m_availableStrategies.size()) {
		return m_availableStrategies[selection];
	}
	return "";
}

bool LayoutStrategyDialog::IsLayoutCachingEnabled() const
{
	return m_cachingCheckBox->GetValue();
}

bool LayoutStrategyDialog::IsOptimizationEnabled() const
{
	return m_optimizationCheckBox->GetValue();
}

int LayoutStrategyDialog::GetLayoutUpdateMode() const
{
	return m_updateModeSpin->GetValue();
}