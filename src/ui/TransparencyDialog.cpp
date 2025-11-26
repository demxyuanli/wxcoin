#include "TransparencyDialog.h"
#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"
#include <wx/statbox.h>
#include <wx/msgdlg.h>
#include <wx/glcanvas.h>

TransparencyDialog::TransparencyDialog(wxWindow* parent, OCCViewer* occViewer,
	const std::vector<std::shared_ptr<OCCGeometry>>& selectedGeometries)
	: FramelessModalPopup(parent, "Set Transparency", wxSize(400, 300))
	, m_occViewer(occViewer)
	, m_selectedGeometries(selectedGeometries)
	, m_transparencySlider(nullptr)
	, m_transparencySpinCtrl(nullptr)
	, m_infoText(nullptr)
	, m_currentTransparency(0.0)
	, m_originalTransparency(0.0)
{
	if (!m_occViewer) {
		LOG_ERR_S("OCCViewer is null in TransparencyDialog");
		return;
	}

	// Set up title bar with icon
	SetTitleIcon("eye", wxSize(20, 20));
	ShowTitleIcon(true);

	if (m_selectedGeometries.empty()) {
		LOG_WRN_S("No selected geometries in TransparencyDialog, showing message");
		// Create a simple message dialog instead of empty dialog
		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText* messageText = new wxStaticText(m_contentPanel, wxID_ANY,
			"No objects available for transparency settings.\n\nPlease select objects in the view first.");
		messageText->SetFont(messageText->GetFont().MakeLarger());
		mainSizer->Add(messageText, 1, wxALIGN_CENTER | wxALL, 20);
		
		wxButton* okButton = new wxButton(m_contentPanel, wxID_OK, "OK");
		wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
		buttonSizer->AddStretchSpacer();
		buttonSizer->Add(okButton, 0, wxALIGN_CENTER);
		buttonSizer->AddStretchSpacer();
		mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
		
		m_contentPanel->SetSizer(mainSizer);
		okButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); });
		return;
	}

	// Get current transparency from the first selected geometry
	// In a real implementation, you might want to handle mixed transparency values
	m_currentTransparency = 0.0; // Default value
	if (!m_selectedGeometries.empty() && m_selectedGeometries[0]) {
		m_currentTransparency = m_selectedGeometries[0]->getTransparency();
	}
	m_originalTransparency = m_currentTransparency;

	createControls();
	layoutControls();
	bindEvents();
	updateControls();
}

TransparencyDialog::~TransparencyDialog()
{
}

void TransparencyDialog::createControls()
{
	// Info text
	m_infoText = new wxStaticText(m_contentPanel, wxID_ANY,
		wxString::Format("Setting transparency for %zu selected object(s)", m_selectedGeometries.size()));

	// Transparency slider (0-100 for percentage)
	m_transparencySlider = new wxSlider(m_contentPanel, ID_TRANSPARENCY_SLIDER,
		static_cast<int>(m_currentTransparency * 100), 0, 100,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);

	// Transparency spin control
	m_transparencySpinCtrl = new wxSpinCtrlDouble(m_contentPanel, ID_TRANSPARENCY_SPIN,
		wxString::Format("%.1f", m_currentTransparency * 100),
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
		0.0, 100.0, m_currentTransparency * 100, 0.1);
	m_transparencySpinCtrl->SetDigits(1);
}

void TransparencyDialog::layoutControls()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Info section
	mainSizer->Add(m_infoText, 0, wxALL | wxEXPAND, 10);

	// Transparency control section
	wxStaticBox* transparencyBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Transparency Settings");
	wxStaticBoxSizer* transparencySizer = new wxStaticBoxSizer(transparencyBox, wxVERTICAL);

	transparencySizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Transparency (0% = Opaque, 100% = Transparent):"),
		0, wxALL, 5);
	transparencySizer->Add(m_transparencySlider, 0, wxEXPAND | wxALL, 5);

	// Spin control in horizontal layout
	wxBoxSizer* spinSizer = new wxBoxSizer(wxHORIZONTAL);
	spinSizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Precise value (%):"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	spinSizer->Add(m_transparencySpinCtrl, 0, wxALIGN_CENTER_VERTICAL);

	transparencySizer->Add(spinSizer, 0, wxALL | wxALIGN_CENTER, 5);

	mainSizer->Add(transparencySizer, 1, wxEXPAND | wxALL, 10);

	// Button section
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(m_contentPanel, wxID_APPLY, "Apply"), 0, wxRIGHT, 5);
	buttonSizer->Add(new wxButton(m_contentPanel, wxID_OK, "OK"), 0, wxRIGHT, 5);
	buttonSizer->Add(new wxButton(m_contentPanel, wxID_CANCEL, "Cancel"), 0, 0);

	mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

	m_contentPanel->SetSizer(mainSizer);
}

void TransparencyDialog::bindEvents()
{
	// Bind slider events for real-time preview during dragging
	m_transparencySlider->Bind(wxEVT_SLIDER, &TransparencyDialog::onTransparencySlider, this);
	m_transparencySlider->Bind(wxEVT_SCROLL_THUMBTRACK, &TransparencyDialog::onTransparencySlider, this);
	m_transparencySlider->Bind(wxEVT_SCROLL_CHANGED, &TransparencyDialog::onTransparencySlider, this);

	// Bind spin control events
	m_transparencySpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &TransparencyDialog::onTransparencySpinCtrl, this);

	// Bind button events
	FindWindow(wxID_APPLY)->Bind(wxEVT_BUTTON, &TransparencyDialog::onApply, this);
	FindWindow(wxID_OK)->Bind(wxEVT_BUTTON, &TransparencyDialog::onOK, this);
	FindWindow(wxID_CANCEL)->Bind(wxEVT_BUTTON, &TransparencyDialog::onCancel, this);
}

void TransparencyDialog::updateControls()
{
	m_transparencySlider->SetValue(static_cast<int>(m_currentTransparency * 100));
	m_transparencySpinCtrl->SetValue(m_currentTransparency * 100);

	// Update info text to show current value
	m_infoText->SetLabel(wxString::Format("Setting transparency for %zu selected object(s) - Current: %.1f%%",
		m_selectedGeometries.size(), m_currentTransparency * 100));
}

void TransparencyDialog::applyTransparency()
{
	if (!m_occViewer || m_selectedGeometries.empty()) {
		LOG_WRN_S("TransparencyDialog::applyTransparency: OCCViewer or selected geometries not available");
		return;
	}

	try {
		LOG_INF_S("TransparencyDialog: Applying transparency " + std::to_string(m_currentTransparency) +
			" to " + std::to_string(m_selectedGeometries.size()) + " geometries");

		// Apply transparency to all selected geometries
		for (const auto& geometry : m_selectedGeometries) {
			if (geometry) {
				m_occViewer->setGeometryTransparency(geometry->getName(), m_currentTransparency);

				// Verify the transparency was set
				double actualTransparency = geometry->getTransparency();
				LOG_INF_S("Geometry " + geometry->getName() + " transparency set to: " + std::to_string(actualTransparency));
			}
		}

		// Force refresh the view to show changes immediately
		m_occViewer->requestViewRefresh();

		// Additional force refresh by calling parent frame's refresh if available
		wxWindow* parent = GetParent();
		while (parent) {
			if (parent->GetName() == "Canvas" || parent->IsKindOf(wxCLASSINFO(wxGLCanvas))) {
				parent->Refresh();
				parent->Update();
				break;
			}
			parent = parent->GetParent();
		}

		LOG_INF_S("Applied transparency " + std::to_string(m_currentTransparency) +
			" to " + std::to_string(m_selectedGeometries.size()) + " geometries");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Error applying transparency: " + std::string(e.what()));
		wxMessageBox("Error applying transparency: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR);
	}
}

void TransparencyDialog::onTransparencySlider(wxCommandEvent& event)
{
	int sliderValue = m_transparencySlider->GetValue();
	m_currentTransparency = sliderValue / 100.0;

	// Update spin control to sync values
	m_transparencySpinCtrl->SetValue(sliderValue);

	// Apply transparency in real-time for preview
	applyTransparency();

	// Update info text to show current value
	m_infoText->SetLabel(wxString::Format("Setting transparency for %zu selected object(s) - Current: %.1f%%",
		m_selectedGeometries.size(), m_currentTransparency * 100));
}

void TransparencyDialog::onTransparencySpinCtrl(wxSpinDoubleEvent& event)
{
	double spinValue = m_transparencySpinCtrl->GetValue();
	m_currentTransparency = spinValue / 100.0;

	// Update slider to sync values
	m_transparencySlider->SetValue(static_cast<int>(spinValue));

	// Apply transparency in real-time for preview
	applyTransparency();

	// Update info text to show current value
	m_infoText->SetLabel(wxString::Format("Setting transparency for %zu selected object(s) - Current: %.1f%%",
		m_selectedGeometries.size(), m_currentTransparency * 100));
}

void TransparencyDialog::onApply(wxCommandEvent& event)
{
	applyTransparency();

	// Show feedback that apply was successful
	m_infoText->SetLabel(wxString::Format("Applied transparency %.1f%% to %zu selected object(s)",
		m_currentTransparency * 100, m_selectedGeometries.size()));

	// Update the "original" transparency so Cancel won't revert this change
	m_originalTransparency = m_currentTransparency;
}

void TransparencyDialog::onOK(wxCommandEvent& event)
{
	applyTransparency();
	EndModal(wxID_OK);
}

void TransparencyDialog::onCancel(wxCommandEvent& event)
{
	// Restore original transparency
	if (m_occViewer && !m_selectedGeometries.empty()) {
		try {
			for (const auto& geometry : m_selectedGeometries) {
				if (geometry) {
					m_occViewer->setGeometryTransparency(geometry->getName(), m_originalTransparency);
				}
			}
			// Force refresh to show restored transparency
			m_occViewer->requestViewRefresh();
		}
		catch (const std::exception& e) {
			LOG_ERR_S("Error restoring transparency: " + std::string(e.what()));
		}
	}

	EndModal(wxID_CANCEL);
}