#include "FaceQueryDialog.h"
#include "OCCGeometry.h"
#include <wx/sizer.h>

FaceQueryDialog::FaceQueryDialog(wxWindow* parent, const PickingResult& result)
	: wxDialog(parent, wxID_ANY, "Face Query Information", wxDefaultPosition, wxSize(500, 400),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	createControls(result);
	layoutControls();
	Centre();
}

void FaceQueryDialog::createControls(const PickingResult& result)
{
	m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER);

	if (!result.geometry) {
		m_propGrid->Append(new wxStringProperty("Result", "Result", "No geometry selected"));
		return;
	}

	// Display geometry information
	m_propGrid->Append(new wxStringProperty("Geometry", "Geometry", result.geometry->getName()));
	m_propGrid->Append(new wxStringProperty("File", "File", result.geometry->getFileName()));

	// Display face information
	if (result.triangleIndex >= 0) {
		m_propGrid->Append(new wxStringProperty("Triangle Index", "TriangleIndex",
			std::to_string(result.triangleIndex)));
	} else {
		m_propGrid->Append(new wxStringProperty("Triangle Index", "TriangleIndex", "N/A"));
	}

	if (result.geometryFaceId >= 0) {
		m_propGrid->Append(new wxStringProperty("Geometry Face ID", "GeometryFaceId",
			std::to_string(result.geometryFaceId)));

		// Show additional face information if available
		if (result.geometry->hasFaceIndexMapping()) {
			auto triangles = result.geometry->getTrianglesForGeometryFace(result.geometryFaceId);
			m_propGrid->Append(new wxStringProperty("Triangles in Face", "TrianglesInFace",
				std::to_string(triangles.size())));
		}
	} else {
		m_propGrid->Append(new wxStringProperty("Geometry Face ID", "GeometryFaceId", "N/A"));
	}

	// Display mapping status
	bool hasMapping = result.geometry->hasFaceIndexMapping();
	m_propGrid->Append(new wxStringProperty("Face Mapping", "FaceMapping",
		hasMapping ? "Available" : "Not Available"));

	if (!hasMapping) {
		m_propGrid->Append(new wxStringProperty("Note", "Note",
			"Face index mapping not built. Use FACE_LEVEL decomposition for detailed face information."));
	}

	// Add OK button
	wxSizer* buttonSizer = CreateButtonSizer(wxOK);
	if (buttonSizer) {
		// Remove the OK button from button sizer and create our own
		buttonSizer->Clear(true);
		wxButton* okButton = new wxButton(this, wxID_OK, "OK");
		okButton->SetDefault();
		buttonSizer->Add(okButton, 0, wxALL, 5);
	}
}

void FaceQueryDialog::layoutControls()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	mainSizer->Add(m_propGrid, 1, wxEXPAND | wxALL, 10);

	// Add button sizer
	wxSizer* buttonSizer = CreateButtonSizer(wxOK);
	if (buttonSizer) {
		mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
	}

	SetSizer(mainSizer);
	Layout();
}
