#pragma once

#include <wx/dialog.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <memory>
#include <string>
#include "widgets/FramelessModalPopup.h"

// Forward declarations
class ModernDockManager;

// Dialog for selecting and configuring layout strategies
class LayoutStrategyDialog : public FramelessModalPopup {
public:
	LayoutStrategyDialog(wxWindow* parent, ModernDockManager* dockManager);
	~LayoutStrategyDialog() = default;

	// Get selected strategy
	std::string GetSelectedStrategy() const;

	// Get strategy parameters
	bool IsLayoutCachingEnabled() const;
	bool IsOptimizationEnabled() const;
	int GetLayoutUpdateMode() const;

private:
	void InitializeUI();
	void OnStrategyChanged(wxCommandEvent& event);
	void OnApplyClicked(wxCommandEvent& event);
	void OnCancelClicked(wxCommandEvent& event);
	void OnOKClicked(wxCommandEvent& event);

	void UpdateStrategyDescription();
	void UpdateParameterVisibility();

	// UI components
	wxChoice* m_strategyChoice;
	wxStaticText* m_descriptionText;
	wxCheckBox* m_cachingCheckBox;
	wxCheckBox* m_optimizationCheckBox;
	wxSpinCtrl* m_updateModeSpin;
	wxStaticText* m_updateModeLabel;

	// Buttons
	wxButton* m_applyButton;
	wxButton* m_okButton;
	wxButton* m_cancelButton;

	// Data
	ModernDockManager* m_dockManager;
	std::vector<std::string> m_availableStrategies;

	// Event table
	wxDECLARE_EVENT_TABLE();
};
