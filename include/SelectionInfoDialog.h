#pragma once

#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include "viewer/PickingService.h"

class Canvas;

/**
 * @brief Small floating info window to display selection (face/edge/vertex) information.
 * Positioned at the top-left of the canvas, similar to SliceParamDialog.
 */
class SelectionInfoDialog : public wxFrame {
public:
	explicit SelectionInfoDialog(Canvas* canvas);

	// Show and position relative to canvas
	void ShowAtCanvasTopLeft();
	void UpdatePosition();

	// Update content from picking result
	void SetPickingResult(const PickingResult& result);

	// Get current mode state
	bool isSelectionMode() const;

protected:
	void OnParentResize(wxSizeEvent& event);
	void OnParentMove(wxMoveEvent& event);
	void OnToggleSizeButton(wxCommandEvent& event);
	void OnMouseModeToggle(wxCommandEvent& event);

private:
	void UpdateContent();
	void UpdateModeButton();

private:
	Canvas* m_canvas{ nullptr };
	wxPanel* m_contentPanel{ nullptr };
	wxPanel* m_mainContent{ nullptr };
	wxStaticText* m_titleText{ nullptr };
	wxButton* m_toggleSizeBtn{ nullptr };
	wxButton* m_mouseModeBtn{ nullptr };
	
	// Card panels for grouped display
	wxStaticBox* m_geometryCard{ nullptr };
	wxStaticBox* m_elementCard{ nullptr };
	wxStaticBox* m_positionCard{ nullptr };
	wxStaticBox* m_statisticsCard{ nullptr };
	
	// Geometry info fields
	wxStaticText* m_geomNameLabel{ nullptr };
	wxStaticText* m_geomNameValue{ nullptr };
	wxStaticText* m_fileNameLabel{ nullptr };
	wxStaticText* m_fileNameValue{ nullptr };
	
	// Element info fields (dynamic based on type)
	wxStaticText* m_elementTypeLabel{ nullptr };
	wxStaticText* m_elementNameLabel{ nullptr };
	wxStaticText* m_elementNameValue{ nullptr };
	wxStaticText* m_elementIdLabel{ nullptr };
	wxStaticText* m_elementIdValue{ nullptr };
	wxStaticText* m_elementIndexLabel{ nullptr };
	wxStaticText* m_elementIndexValue{ nullptr };
	
	// Position info fields
	wxStaticText* m_posXLabel{ nullptr };
	wxStaticText* m_posXValue{ nullptr };
	wxStaticText* m_posYLabel{ nullptr };
	wxStaticText* m_posYValue{ nullptr };
	wxStaticText* m_posZLabel{ nullptr };
	wxStaticText* m_posZValue{ nullptr };
	
	// Statistics fields (for faces)
	wxStaticText* m_statLabel1{ nullptr };
	wxStaticText* m_statValue1{ nullptr };
	wxStaticText* m_statLabel2{ nullptr };
	wxStaticText* m_statValue2{ nullptr };
	wxStaticText* m_statLabel3{ nullptr };
	wxStaticText* m_statValue3{ nullptr };

	PickingResult m_result;
	bool m_isMinimized{ false };
	bool m_isSelectionMode{ true };
	std::string m_lastSelectionCommandType;  // Save last active selection command type

	wxDECLARE_EVENT_TABLE();
};


