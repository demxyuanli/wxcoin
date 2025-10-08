#pragma once

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include "viewer/PickingService.h"

/**
 * @brief Dialog for displaying face query information
 */
class FaceQueryDialog : public wxDialog
{
public:
	FaceQueryDialog(wxWindow* parent, const PickingResult& result);

private:
	void createControls(const PickingResult& result);
	void layoutControls();

	wxPropertyGrid* m_propGrid;
};

