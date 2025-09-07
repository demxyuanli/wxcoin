#pragma once

#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "OCCViewer.h"
#include "config/EdgeSettingsConfig.h"

class EdgeSettingsDialog : public wxDialog
{
public:
	EdgeSettingsDialog(wxWindow* parent, OCCViewer* viewer);
	virtual ~EdgeSettingsDialog();

	// Getters for edge settings
	bool isShowEdgesEnabled() const { return m_currentSettings.showEdges; }
	double getEdgeWidth() const { return m_currentSettings.edgeWidth; }
	Quantity_Color getEdgeColor() const { return m_currentSettings.edgeColor; }
	bool isEdgeColorEnabled() const { return m_currentSettings.edgeColorEnabled; }
	int getEdgeStyle() const { return m_currentSettings.edgeStyle; }
	double getEdgeOpacity() const { return m_currentSettings.edgeOpacity; }

	// Get all settings
	const EdgeSettings& getGlobalSettings() const { return m_globalSettings; }
	const EdgeSettings& getSelectedSettings() const { return m_selectedSettings; }
	const EdgeSettings& getHoverSettings() const { return m_hoverSettings; }

private:
	void createControls();
	void createGlobalPage();
	void createSelectedPage();
	void createHoverPage();
	void createFeatureEdgePage();
	void bindEvents();
	void updateControls();
	void applySettings();
	void loadSettings();
	void saveSettings();
	void switchToPage(const std::string& pageName);

	// Event handlers
	// Global page events
	void onGlobalShowEdgesCheckbox(wxCommandEvent& event);
	void onGlobalEdgeWidthSlider(wxCommandEvent& event);
	void onGlobalEdgeColorButton(wxCommandEvent& event);
	void onGlobalEdgeColorEnabledCheckbox(wxCommandEvent& event);
	void onGlobalEdgeStyleChoice(wxCommandEvent& event);
	void onGlobalEdgeOpacitySlider(wxCommandEvent& event);

	// Selected page events
	void onSelectedShowEdgesCheckbox(wxCommandEvent& event);
	void onSelectedEdgeWidthSlider(wxCommandEvent& event);
	void onSelectedEdgeColorButton(wxCommandEvent& event);
	void onSelectedEdgeColorEnabledCheckbox(wxCommandEvent& event);
	void onSelectedEdgeStyleChoice(wxCommandEvent& event);
	void onSelectedEdgeOpacitySlider(wxCommandEvent& event);

	// Hover page events
	void onHoverShowEdgesCheckbox(wxCommandEvent& event);
	void onHoverEdgeWidthSlider(wxCommandEvent& event);
	void onHoverEdgeColorButton(wxCommandEvent& event);
	void onHoverEdgeColorEnabledCheckbox(wxCommandEvent& event);
	void onHoverEdgeStyleChoice(wxCommandEvent& event);
	void onHoverEdgeOpacitySlider(wxCommandEvent& event);

	// Button events
	void onApply(wxCommandEvent& event);
	void onSave(wxCommandEvent& event);
	void onReset(wxCommandEvent& event);
	void onCancel(wxCommandEvent& event);
	void onOK(wxCommandEvent& event);

	// Helper methods
	void updateColorButtons();
	wxColour quantityColorToWxColour(const Quantity_Color& color);
	Quantity_Color wxColourToQuantityColor(const wxColour& color);

	OCCViewer* m_viewer;

	// UI components
	wxNotebook* m_notebook;
	wxPanel* m_globalPage;
	wxPanel* m_selectedPage;
	wxPanel* m_hoverPage;
	wxPanel* m_featureEdgePage = nullptr;

	// Global page controls
	wxCheckBox* m_globalShowEdgesCheckbox;
	wxSlider* m_globalEdgeWidthSlider;
	wxStaticText* m_globalEdgeWidthLabel;
	wxButton* m_globalEdgeColorButton;
	wxCheckBox* m_globalEdgeColorEnabledCheckbox;
	wxChoice* m_globalEdgeStyleChoice;
	wxSlider* m_globalEdgeOpacitySlider;
	wxStaticText* m_globalEdgeOpacityLabel;

	// Selected page controls
	wxCheckBox* m_selectedShowEdgesCheckbox;
	wxSlider* m_selectedEdgeWidthSlider;
	wxStaticText* m_selectedEdgeWidthLabel;
	wxButton* m_selectedEdgeColorButton;
	wxCheckBox* m_selectedEdgeColorEnabledCheckbox;
	wxChoice* m_selectedEdgeStyleChoice;
	wxSlider* m_selectedEdgeOpacitySlider;
	wxStaticText* m_selectedEdgeOpacityLabel;

	// Hover page controls
	wxCheckBox* m_hoverShowEdgesCheckbox;
	wxSlider* m_hoverEdgeWidthSlider;
	wxStaticText* m_hoverEdgeWidthLabel;
	wxButton* m_hoverEdgeColorButton;
	wxCheckBox* m_hoverEdgeColorEnabledCheckbox;
	wxChoice* m_hoverEdgeStyleChoice;
	wxSlider* m_hoverEdgeOpacitySlider;
	wxStaticText* m_hoverEdgeOpacityLabel;

	// Settings
	EdgeSettings m_globalSettings;
	EdgeSettings m_selectedSettings;
	EdgeSettings m_hoverSettings;
	EdgeSettings m_currentSettings;
	std::string m_currentPage;

	// Buttons
	wxButton* m_applyButton;
	wxButton* m_cancelButton;
	wxButton* m_okButton;
	wxButton* m_resetButton;
	wxButton* m_saveButton;

	int m_featureEdgeAngle = 30;
	double m_featureEdgeMinLength = 0.1;
	void onFeatureEdgeAngleSlider(wxCommandEvent& event);
	void onFeatureEdgeMinLengthSlider(wxCommandEvent& event);
	void onFeatureEdgeConvexCheckbox(wxCommandEvent& event);
	void onFeatureEdgeConcaveCheckbox(wxCommandEvent& event);

	wxSlider* m_featureEdgeAngleSlider = nullptr;
	wxStaticText* m_featureEdgeAngleLabel = nullptr;
	wxSlider* m_featureEdgeMinLengthSlider = nullptr;
	wxStaticText* m_featureEdgeMinLengthLabel = nullptr;

	wxCheckBox* m_onlyConvexCheckbox = nullptr;
	wxCheckBox* m_onlyConcaveCheckbox = nullptr;
	bool m_onlyConvex = false;
	bool m_onlyConcave = false;
};