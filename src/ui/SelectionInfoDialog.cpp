#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "SelectionInfoDialog.h"
#include "OCCGeometry.h"
#include "Canvas.h"
#include "InputManager.h"
#include "FlatFrame.h"
#include "CommandType.h"
#include <wx/sizer.h>
#include <wx/event.h>

wxBEGIN_EVENT_TABLE(SelectionInfoDialog, wxFrame)
	EVT_SIZE(SelectionInfoDialog::OnParentResize)
	EVT_BUTTON(wxID_DOWN, SelectionInfoDialog::OnToggleSizeButton)
	EVT_BUTTON(wxID_FORWARD, SelectionInfoDialog::OnMouseModeToggle)
wxEND_EVENT_TABLE()

SelectionInfoDialog::SelectionInfoDialog(Canvas* canvas)
	: wxFrame(canvas, wxID_ANY, "", wxDefaultPosition, wxSize(260, 200),
		wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR | wxNO_BORDER)
	, m_canvas(canvas)
	, m_isSelectionMode(true) {

	// Semi-transparent small info window
	SetTransparent(static_cast<wxByte>(255 * 0.8));

	m_contentPanel = new wxPanel(this);
	m_contentPanel->SetBackgroundColour(wxColour(45, 45, 48));

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Title bar with minimize button
	wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
	m_titleText = new wxStaticText(m_contentPanel, wxID_ANY, "Selection Info");
	wxFont titleFont = m_titleText->GetFont();
	titleFont.SetPointSize(9);
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	m_titleText->SetFont(titleFont);
	m_titleText->SetForegroundColour(wxColour(220, 220, 220));
	titleSizer->Add(m_titleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

	// Toggle size button (minimize/maximize)
	m_toggleSizeBtn = new wxButton(m_contentPanel, wxID_DOWN, "-", wxDefaultPosition, wxSize(24, 24));
	m_toggleSizeBtn->SetBackgroundColour(wxColour(60, 60, 65));
	m_toggleSizeBtn->SetForegroundColour(wxColour(220, 220, 220));
	m_toggleSizeBtn->SetToolTip("Minimize/Maximize");
	titleSizer->Add(m_toggleSizeBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	mainSizer->Add(titleSizer, 0, wxEXPAND | wxALL, 4);

	// Separator
	wxPanel* separator = new wxPanel(m_contentPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
	separator->SetBackgroundColour(wxColour(80, 80, 85));
	mainSizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);

	// Main content panel (can be hidden when minimized)
	m_mainContent = new wxPanel(m_contentPanel);
	m_mainContent->SetBackgroundColour(wxColour(45, 45, 48));
	wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

	// Helper function to create label-value pairs (compact version)
	auto createInfoRow = [](wxWindow* parent, const wxString& label, wxStaticText*& labelCtrl, wxStaticText*& valueCtrl) {
		wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
		
		labelCtrl = new wxStaticText(parent, wxID_ANY, label + ":");
		wxFont labelFont = labelCtrl->GetFont();
		labelFont.SetPointSize(7);  // Reduced from 8 to 7
		labelFont.SetWeight(wxFONTWEIGHT_NORMAL);
		labelCtrl->SetFont(labelFont);
		labelCtrl->SetForegroundColour(wxColour(180, 180, 180));
		labelCtrl->SetMinSize(wxSize(70, -1));  // Reduced from 90 to 70
		rowSizer->Add(labelCtrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);  // Reduced from 4 to 2
		
		valueCtrl = new wxStaticText(parent, wxID_ANY, "");
		wxFont valueFont = valueCtrl->GetFont();
		valueFont.SetPointSize(7);  // Reduced from 8 to 7
		valueFont.SetWeight(wxFONTWEIGHT_BOLD);
		valueCtrl->SetFont(valueFont);
		valueCtrl->SetForegroundColour(wxColour(255, 255, 255));
		rowSizer->Add(valueCtrl, 1, wxEXPAND);
		
		return rowSizer;
	};

	// Geometry Info Card
	m_geometryCard = new wxStaticBox(m_mainContent, wxID_ANY, "Geometry");
	m_geometryCard->SetForegroundColour(wxColour(220, 220, 220));
	wxStaticBoxSizer* geometrySizer = new wxStaticBoxSizer(m_geometryCard, wxVERTICAL);
	
	wxBoxSizer* geomNameRow = createInfoRow(m_mainContent, "Name", m_geomNameLabel, m_geomNameValue);
	geometrySizer->Add(geomNameRow, 0, wxEXPAND | wxALL, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* fileNameRow = createInfoRow(m_mainContent, "File", m_fileNameLabel, m_fileNameValue);
	geometrySizer->Add(fileNameRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	contentSizer->Add(geometrySizer, 0, wxEXPAND | wxALL, 3);  // Reduced from 6 to 3

	// Element Info Card
	m_elementCard = new wxStaticBox(m_mainContent, wxID_ANY, "Element");
	m_elementCard->SetForegroundColour(wxColour(220, 220, 220));
	wxStaticBoxSizer* elementSizer = new wxStaticBoxSizer(m_elementCard, wxVERTICAL);
	
	m_elementTypeLabel = new wxStaticText(m_mainContent, wxID_ANY, "Type:");
	wxFont typeFont = m_elementTypeLabel->GetFont();
	typeFont.SetPointSize(7);  // Reduced from default to 7
	m_elementTypeLabel->SetFont(typeFont);
	m_elementTypeLabel->SetForegroundColour(wxColour(180, 180, 180));
	m_elementTypeLabel->SetMinSize(wxSize(70, -1));  // Reduced from 90 to 70
	
	wxBoxSizer* typeRow = new wxBoxSizer(wxHORIZONTAL);
	typeRow->Add(m_elementTypeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);  // Reduced from 4 to 2
	elementSizer->Add(typeRow, 0, wxEXPAND | wxALL, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* idRow = createInfoRow(m_mainContent, "ID", m_elementIdLabel, m_elementIdValue);
	elementSizer->Add(idRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* indexRow = createInfoRow(m_mainContent, "Index", m_elementIndexLabel, m_elementIndexValue);
	elementSizer->Add(indexRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* nameRow = createInfoRow(m_mainContent, "Name", m_elementNameLabel, m_elementNameValue);
	elementSizer->Add(nameRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	contentSizer->Add(elementSizer, 0, wxEXPAND | wxALL, 3);  // Reduced from 6 to 3

	// Position Card
	m_positionCard = new wxStaticBox(m_mainContent, wxID_ANY, "Position (3D)");
	m_positionCard->SetForegroundColour(wxColour(220, 220, 220));
	wxStaticBoxSizer* positionSizer = new wxStaticBoxSizer(m_positionCard, wxVERTICAL);
	
	wxBoxSizer* posXRow = createInfoRow(m_mainContent, "X", m_posXLabel, m_posXValue);
	positionSizer->Add(posXRow, 0, wxEXPAND | wxALL, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* posYRow = createInfoRow(m_mainContent, "Y", m_posYLabel, m_posYValue);
	positionSizer->Add(posYRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* posZRow = createInfoRow(m_mainContent, "Z", m_posZLabel, m_posZValue);
	positionSizer->Add(posZRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	contentSizer->Add(positionSizer, 0, wxEXPAND | wxALL, 3);  // Reduced from 6 to 3

	// Statistics Card (for faces with mapping)
	m_statisticsCard = new wxStaticBox(m_mainContent, wxID_ANY, "Statistics");
	m_statisticsCard->SetForegroundColour(wxColour(220, 220, 220));
	wxStaticBoxSizer* statsSizer = new wxStaticBoxSizer(m_statisticsCard, wxVERTICAL);
	
	wxBoxSizer* stat1Row = createInfoRow(m_mainContent, "", m_statLabel1, m_statValue1);
	statsSizer->Add(stat1Row, 0, wxEXPAND | wxALL, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* stat2Row = createInfoRow(m_mainContent, "", m_statLabel2, m_statValue2);
	statsSizer->Add(stat2Row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	wxBoxSizer* stat3Row = createInfoRow(m_mainContent, "", m_statLabel3, m_statValue3);
	statsSizer->Add(stat3Row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);  // Reduced from 5 to 3
	
	contentSizer->Add(statsSizer, 0, wxEXPAND | wxALL, 3);  // Reduced from 6 to 3
	m_statisticsCard->Hide(); // Initially hidden

	// Mouse Mode Button - Not in a card, directly in layout
	m_mouseModeBtn = new wxButton(m_mainContent, wxID_FORWARD, "Mode: Selection");
	m_mouseModeBtn->SetBackgroundColour(wxColour(60, 120, 60));  // Green tone for selection mode
	m_mouseModeBtn->SetForegroundColour(wxColour(255, 255, 255));
	wxFont btnFont = m_mouseModeBtn->GetFont();
	btnFont.SetPointSize(8);
	btnFont.SetWeight(wxFONTWEIGHT_BOLD);
	m_mouseModeBtn->SetFont(btnFont);
	m_mouseModeBtn->SetToolTip("Click to toggle between selection and camera rotation modes");
	
	// Add button directly to content sizer (not in a card)
	contentSizer->Add(m_mouseModeBtn, 0, wxEXPAND | wxALL, 3);

	m_mainContent->SetSizer(contentSizer);
	mainSizer->Add(m_mainContent, 1, wxEXPAND);

	m_contentPanel->SetSizer(mainSizer);

	wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
	frameSizer->Add(m_contentPanel, 1, wxEXPAND);
	SetSizer(frameSizer);

	Layout();

	SetClientSize(wxSize(280, 420));  // Reduced height from 480 to 420
	SetMinSize(wxSize(280, 32));  // Minimum size is title bar only
	SetMaxSize(wxSize(280, 600));

	// Bind to parent resize and move events
	if (GetParent()) {
		GetParent()->Bind(wxEVT_SIZE, &SelectionInfoDialog::OnParentResize, this);
		GetParent()->Bind(wxEVT_MOVE, &SelectionInfoDialog::OnParentMove, this);
	}
}

void SelectionInfoDialog::ShowAtCanvasTopLeft() {
	UpdatePosition();
	Show(true);
	Raise();
}

void SelectionInfoDialog::UpdatePosition() {
	if (GetParent()) {
		wxPoint canvasScreenPos = GetParent()->GetScreenPosition();
		wxPoint dialogPos(canvasScreenPos.x + 4, canvasScreenPos.y + 4);
		SetPosition(dialogPos);
	}
}

void SelectionInfoDialog::SetPickingResult(const PickingResult& result) {
	m_result = result;
	
	// Save the selection command type based on element type
	if (result.elementType == "Face") {
		m_lastSelectionCommandType = "FACE_SELECTION_TOOL";
	} else if (result.elementType == "Edge") {
		m_lastSelectionCommandType = "EDGE_SELECTION_TOOL";
	} else if (result.elementType == "Vertex") {
		m_lastSelectionCommandType = "VERTEX_SELECTION_TOOL";
	}
	
	UpdateContent();
	ShowAtCanvasTopLeft();
}

void SelectionInfoDialog::UpdateContent() {
	if (!m_result.geometry) {
		m_titleText->SetLabel("Selection Info");
		
		// Clear all fields
		if (m_geomNameValue) m_geomNameValue->SetLabel("N/A");
		if (m_fileNameValue) m_fileNameValue->SetLabel("N/A");
		if (m_elementTypeLabel) m_elementTypeLabel->SetLabel("Type: N/A");
		if (m_elementNameValue) m_elementNameValue->SetLabel("N/A");
		if (m_elementIdValue) m_elementIdValue->SetLabel("N/A");
		if (m_elementIndexValue) m_elementIndexValue->SetLabel("N/A");
		if (m_posXValue) m_posXValue->SetLabel("N/A");
		if (m_posYValue) m_posYValue->SetLabel("N/A");
		if (m_posZValue) m_posZValue->SetLabel("N/A");
		
		m_statisticsCard->Hide();
		Layout();
		return;
	}

	// Geometry Info
	wxString geomName = wxString::FromUTF8(m_result.geometry->getName().c_str());
	wxString fileName = wxString::FromUTF8(m_result.geometry->getFileName().c_str());

	if (fileName.length() > 35) {
		fileName = fileName.substr(0, 32) + "...";
	}

	if (m_geomNameValue) m_geomNameValue->SetLabel(geomName);
	if (m_fileNameValue) m_fileNameValue->SetLabel(fileName);

	// Position Info (3D coordinates)
	if (m_posXValue) m_posXValue->SetLabel(wxString::Format("%.3f", m_result.x));
	if (m_posYValue) m_posYValue->SetLabel(wxString::Format("%.3f", m_result.y));
	if (m_posZValue) m_posZValue->SetLabel(wxString::Format("%.3f", m_result.z));

	// Element Info
	// First, clear statistics fields in case they were set by previous selection type
	if (m_statLabel1) m_statLabel1->SetLabel("");
	if (m_statValue1) m_statValue1->SetLabel("");
	if (m_statLabel2) m_statLabel2->SetLabel("");
	if (m_statValue2) m_statValue2->SetLabel("");
	if (m_statLabel3) m_statLabel3->SetLabel("");
	if (m_statValue3) m_statValue3->SetLabel("");
	
	if (m_result.elementType == "Face") {
		m_titleText->SetLabel("Face Selection");
		
		if (m_elementTypeLabel) m_elementTypeLabel->SetLabel("Type: Face");
		
		// SubElement name (e.g., "Face5")
		wxString subElementName = !m_result.subElementName.empty()
			? wxString::FromUTF8(m_result.subElementName.c_str())
			: "N/A";
		if (m_elementNameValue) m_elementNameValue->SetLabel(subElementName);
		
		wxString faceId = m_result.geometryFaceId >= 0
			? wxString::Format("%d", m_result.geometryFaceId)
			: "N/A";
		if (m_elementIdValue) m_elementIdValue->SetLabel(faceId);
		
		wxString triIdx = m_result.triangleIndex >= 0
			? wxString::Format("%d", m_result.triangleIndex)
			: "N/A";
		if (m_elementIndexValue) m_elementIndexValue->SetLabel(triIdx);

		// Statistics for faces
		bool hasMapping = m_result.geometry->hasFaceDomainMapping();
		if (hasMapping && m_result.geometryFaceId >= 0) {
			auto triangles = m_result.geometry->getTrianglesForGeometryFace(m_result.geometryFaceId);
			if (m_statLabel1) m_statLabel1->SetLabel("Triangles:");
			if (m_statValue1) m_statValue1->SetLabel(wxString::Format("%zu", triangles.size()));
			
			// Get vertex count if available
			const FaceDomain* domain = m_result.geometry->getFaceDomain(m_result.geometryFaceId);
			if (domain) {
				if (m_statLabel2) m_statLabel2->SetLabel("Vertices:");
				if (m_statValue2) m_statValue2->SetLabel(wxString::Format("%zu", domain->getVertexCount()));
				
				// Show mapping status
				if (m_statLabel3) m_statLabel3->SetLabel("Mapping:");
				if (m_statValue3) m_statValue3->SetLabel("Available");
			} else {
				if (m_statLabel2) m_statLabel2->SetLabel("");
				if (m_statValue2) m_statValue2->SetLabel("");
				if (m_statLabel3) m_statLabel3->SetLabel("Mapping:");
				if (m_statValue3) m_statValue3->SetLabel("Partial");
			}
			
			m_statisticsCard->Show();
		} else {
			if (m_statLabel1) m_statLabel1->SetLabel("Mapping:");
			if (m_statValue1) m_statValue1->SetLabel("Not Available");
			if (m_statLabel2) m_statLabel2->SetLabel("");
			if (m_statValue2) m_statValue2->SetLabel("");
			if (m_statLabel3) m_statLabel3->SetLabel("");
			if (m_statValue3) m_statValue3->SetLabel("");
			m_statisticsCard->Show();
		}
	} else if (m_result.elementType == "Edge") {
		m_titleText->SetLabel("Edge Selection");
		
		if (m_elementTypeLabel) m_elementTypeLabel->SetLabel("Type: Edge");
		
		// SubElement name (e.g., "Edge12")
		wxString subElementName = !m_result.subElementName.empty()
			? wxString::FromUTF8(m_result.subElementName.c_str())
			: "N/A";
		if (m_elementNameValue) m_elementNameValue->SetLabel(subElementName);
		
		wxString edgeId = m_result.geometryEdgeId >= 0
			? wxString::Format("%d", m_result.geometryEdgeId)
			: "N/A";
		if (m_elementIdValue) m_elementIdValue->SetLabel(edgeId);
		
		wxString idx = m_result.lineIndex >= 0
			? wxString::Format("%d", m_result.lineIndex)
			: "N/A";
		if (m_elementIndexValue) m_elementIndexValue->SetLabel(idx);
		
		// Clear statistics fields for Edge (not applicable)
		if (m_statLabel1) m_statLabel1->SetLabel("");
		if (m_statValue1) m_statValue1->SetLabel("");
		if (m_statLabel2) m_statLabel2->SetLabel("");
		if (m_statValue2) m_statValue2->SetLabel("");
		if (m_statLabel3) m_statLabel3->SetLabel("");
		if (m_statValue3) m_statValue3->SetLabel("");
		m_statisticsCard->Hide();
	} else if (m_result.elementType == "Vertex") {
		m_titleText->SetLabel("Vertex Selection");
		
		if (m_elementTypeLabel) m_elementTypeLabel->SetLabel("Type: Vertex");
		
		// SubElement name (e.g., "Vertex3")
		wxString subElementName = !m_result.subElementName.empty()
			? wxString::FromUTF8(m_result.subElementName.c_str())
			: "N/A";
		if (m_elementNameValue) m_elementNameValue->SetLabel(subElementName);
		
		wxString vertexId = m_result.geometryVertexId >= 0
			? wxString::Format("%d", m_result.geometryVertexId)
			: "N/A";
		if (m_elementIdValue) m_elementIdValue->SetLabel(vertexId);
		
		wxString idx = m_result.vertexIndex >= 0
			? wxString::Format("%d", m_result.vertexIndex)
			: "N/A";
		if (m_elementIndexValue) m_elementIndexValue->SetLabel(idx);
		
		// Clear statistics fields for Vertex (not applicable)
		if (m_statLabel1) m_statLabel1->SetLabel("");
		if (m_statValue1) m_statValue1->SetLabel("");
		if (m_statLabel2) m_statLabel2->SetLabel("");
		if (m_statValue2) m_statValue2->SetLabel("");
		if (m_statLabel3) m_statLabel3->SetLabel("");
		if (m_statValue3) m_statValue3->SetLabel("");
		m_statisticsCard->Hide();
	} else {
		m_titleText->SetLabel("Selection Info");
		
		if (m_elementTypeLabel) {
			m_elementTypeLabel->SetLabel("Type: " + wxString::FromUTF8(m_result.elementType.c_str()));
		}
		if (m_elementNameValue) m_elementNameValue->SetLabel("N/A");
		if (m_elementIdValue) m_elementIdValue->SetLabel("N/A");
		if (m_elementIndexValue) m_elementIndexValue->SetLabel("N/A");
		
		// Clear statistics fields
		if (m_statLabel1) m_statLabel1->SetLabel("");
		if (m_statValue1) m_statValue1->SetLabel("");
		if (m_statLabel2) m_statLabel2->SetLabel("");
		if (m_statValue2) m_statValue2->SetLabel("");
		if (m_statLabel3) m_statLabel3->SetLabel("");
		if (m_statValue3) m_statValue3->SetLabel("");
		m_statisticsCard->Hide();
	}

	Layout();
}

void SelectionInfoDialog::OnParentResize(wxSizeEvent& event) {
	UpdatePosition();
	event.Skip();
}

void SelectionInfoDialog::OnParentMove(wxMoveEvent& event) {
	UpdatePosition();
	event.Skip();
}

void SelectionInfoDialog::OnToggleSizeButton(wxCommandEvent& event) {
	if (m_isMinimized) {
		// Restore to full size
		SetClientSize(wxSize(280, 420));  // Updated to match new default size
		m_mainContent->Show(true);
		m_toggleSizeBtn->SetLabel("-");
		m_isMinimized = false;
	} else {
		// Minimize to title bar only
		m_mainContent->Show(false);
		
		// Get title bar height
		wxSize titleBarSize = GetSize();
		titleBarSize.SetHeight(32); // Only title bar height
		SetClientSize(titleBarSize);
		
		m_toggleSizeBtn->SetLabel("+");
		m_isMinimized = true;
	}
	
	Layout();
	Refresh();
}

void SelectionInfoDialog::OnMouseModeToggle(wxCommandEvent& event) {
	m_isSelectionMode = !m_isSelectionMode;
	
	// Apply mode change to InputManager
	if (m_canvas && m_canvas->getInputManager()) {
		auto* inputManager = m_canvas->getInputManager();
		if (m_isSelectionMode) {
			// Switching back to selection mode - check if selection tool is active
			if (!inputManager->isCustomInputStateActive()) {
				// Selection tool is not active, need to reactivate it
				// Try to get FlatFrame to access CommandDispatcher
				wxWindow* parent = m_canvas->GetParent();
				while (parent && !dynamic_cast<FlatFrame*>(parent)) {
					parent = parent->GetParent();
				}
				
				FlatFrame* flatFrame = dynamic_cast<FlatFrame*>(parent);
				if (flatFrame && !m_lastSelectionCommandType.empty()) {
					// Re-execute the last selection command to reactivate selection tool
					// Convert string command type to event ID
					int eventId = 0;
					cmd::CommandType commandType;
					if (m_lastSelectionCommandType == "FACE_SELECTION_TOOL") {
						eventId = ID_FACE_SELECTION_TOOL;
						commandType = cmd::CommandType::FaceSelectionTool;
					} else if (m_lastSelectionCommandType == "EDGE_SELECTION_TOOL") {
						eventId = ID_EDGE_SELECTION_TOOL;
						commandType = cmd::CommandType::EdgeSelectionTool;
					} else if (m_lastSelectionCommandType == "VERTEX_SELECTION_TOOL") {
						eventId = ID_VERTEX_SELECTION_TOOL;
						commandType = cmd::CommandType::VertexSelectionTool;
					} else {
						UpdateModeButton();
						return; // Unknown command type
					}
					
					// Create a command event and process it synchronously
					// Use wxEVT_COMMAND_BUTTON_CLICKED to match the event binding in FlatFrame
					wxCommandEvent cmdEvent(wxEVT_COMMAND_BUTTON_CLICKED, eventId);
					cmdEvent.SetEventObject(flatFrame);
					// Process event synchronously to ensure it's handled immediately
					flatFrame->GetEventHandler()->ProcessEvent(cmdEvent);
				}
			}
		} else {
			// Switch to default state (camera rotation)
			inputManager->enterDefaultState();
		}
	}
	
	UpdateModeButton();
}

bool SelectionInfoDialog::isSelectionMode() const {
	return m_isSelectionMode;
}

void SelectionInfoDialog::UpdateModeButton() {
	if (m_mouseModeBtn) {
		if (m_isSelectionMode) {
			m_mouseModeBtn->SetLabel("Mode: Selection");
			m_mouseModeBtn->SetBackgroundColour(wxColour(60, 120, 60)); // Green tone
		} else {
			m_mouseModeBtn->SetLabel("Mode: Rotate Camera");
			m_mouseModeBtn->SetBackgroundColour(wxColour(120, 60, 60)); // Red tone
		}
		m_mouseModeBtn->Refresh();
	}
}


