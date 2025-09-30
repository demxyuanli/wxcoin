#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <string>
#include <map>
#include <vector>
#include <Inventor/SbVec3f.h>
#include "GeometryDialogTypes.h"
#include "widgets/FramelessModalPopup.h"

// Forward declarations
class PickingAidManager;
class VisualSettingsDialog;

// Callback function type for position picking completion
typedef std::function<void(const SbVec3f&)> PositionPickingCallback;

class PositionBasicDialog : public FramelessModalPopup {
public:
	PositionBasicDialog(wxWindow* parent, const wxString& title, PickingAidManager* pickingAidManager, const std::string& geometryType = "");
	~PositionBasicDialog() {}

	void SetPosition(const SbVec3f& position);
	SbVec3f GetPosition() const;
	void SetGeometryType(const std::string& geometryType);
	BasicGeometryParameters GetBasicParameters() const;
	AdvancedGeometryParameters GetAdvancedParameters() const;

private:
	wxTextCtrl* m_xTextCtrl;
	wxTextCtrl* m_yTextCtrl;
	wxTextCtrl* m_zTextCtrl;
	wxTextCtrl* m_referenceZTextCtrl;
	wxCheckBox* m_showGridCheckBox;
	wxButton* m_pickButton;
	wxButton* m_visualSettingsButton; // New button for VisualSettingsDialog
	std::map<std::string, wxTextCtrl*> m_parameterControls;
	wxStaticText* m_geometryTypeLabel;

	wxPanel* m_positionPanel;
	wxPanel* m_parametersPanel;
	wxBoxSizer* m_parametersSizer;
	PickingAidManager* m_pickingAidManager;
	PositionPickingCallback m_pickingCallback;

	BasicGeometryParameters m_basicParams;
	AdvancedGeometryParameters m_advancedParams; // Store advanced parameters

	void CreatePositionTab();
	void CreateParametersTab();
	void UpdateParametersTab();
	void LoadParametersFromControls();
	void SaveParametersToControls();
	void OnPickButton(wxCommandEvent& event);
	void OnVisualSettingsButton(wxCommandEvent& event); // New event handler
	void OnOkButton(wxCommandEvent& event);
	void OnCancelButton(wxCommandEvent& event);
	void OnShowGridChanged(wxCommandEvent& event);
	void OnReferenceZChanged(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()

public:
	// Public method to set picking callback
	void SetPickingCallback(PositionPickingCallback callback) { m_pickingCallback = callback; }

	// Public method to handle picking completion
	void OnPickingComplete(const SbVec3f& position);
};