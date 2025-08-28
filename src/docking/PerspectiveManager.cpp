#include "docking/PerspectiveManager.h"
#include "docking/DockManager.h"
#include <wx/xml/xml.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/listctrl.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/dcmemory.h>
#include <wx/dcscreen.h>
#include <wx/artprov.h>
#include <wx/splitter.h>
#include <functional>

namespace ads {
	// Perspective implementation
	Perspective::Perspective(const wxString& name)
		: m_name(name)
		, m_createdDate(wxDateTime::Now())
		, m_modifiedDate(wxDateTime::Now())
	{
	}

	Perspective::~Perspective() {
	}

	void Perspective::setLayoutData(const wxString& data) {
		m_layoutData = data;
		m_modifiedDate = wxDateTime::Now();
	}

	void Perspective::saveToXml(wxXmlNode* node) const {
		if (!node) return;

		node->AddAttribute("name", m_name);
		node->AddAttribute("description", m_description);
		node->AddAttribute("created", m_createdDate.Format());
		node->AddAttribute("modified", m_modifiedDate.Format());

		// Add layout data as child node
		wxXmlNode* layoutNode = new wxXmlNode(wxXML_ELEMENT_NODE, "Layout");
		layoutNode->AddChild(new wxXmlNode(wxXML_CDATA_SECTION_NODE, "", m_layoutData));
		node->AddChild(layoutNode);

		// TODO: Save preview bitmap if needed
	}

	bool Perspective::loadFromXml(wxXmlNode* node) {
		if (!node) return false;

		m_name = node->GetAttribute("name");
		m_description = node->GetAttribute("description");

		wxString createdStr = node->GetAttribute("created");
		wxString modifiedStr = node->GetAttribute("modified");

		m_createdDate.ParseFormat(createdStr);
		m_modifiedDate.ParseFormat(modifiedStr);

		// Load layout data
		wxXmlNode* layoutNode = node->GetChildren();
		while (layoutNode) {
			if (layoutNode->GetName() == "Layout") {
				wxXmlNode* cdataNode = layoutNode->GetChildren();
				if (cdataNode && cdataNode->GetType() == wxXML_CDATA_SECTION_NODE) {
					m_layoutData = cdataNode->GetContent();
				}
			}
			layoutNode = layoutNode->GetNext();
		}

		return !m_layoutData.IsEmpty();
	}

	// PerspectiveManager implementation
	// PerspectiveManager doesn't inherit from wxEvtHandler, so no event table needed
	// Timer events will be handled differently

	PerspectiveManager::PerspectiveManager(DockManager* dockManager)
		: m_dockManager(dockManager)
		, m_autoSaveEnabled(false)
		, m_autoSaveTimer(nullptr)
	{
		m_autoSaveTimer = new wxTimer();
	}

	PerspectiveManager::~PerspectiveManager() {
		if (m_autoSaveTimer) {
			m_autoSaveTimer->Stop();
			delete m_autoSaveTimer;
		}
	}

	bool PerspectiveManager::savePerspective(const wxString& name, const wxString& description) {
		if (!m_dockManager || name.IsEmpty()) {
			return false;
		}

		// Save current state
		wxString layoutData;
		m_dockManager->saveState(layoutData);
		if (layoutData.IsEmpty()) {
			return false;
		}

		// Create or update perspective
		auto& perspective = m_perspectives[name];
		if (!perspective) {
			perspective = std::make_unique<Perspective>(name);
		}

		perspective->setLayoutData(layoutData);
		perspective->setDescription(description);

		// Capture preview
		perspective->setPreview(capturePreview());

		// Update current
		m_currentPerspective = name;

		// Notify callbacks
		for (const auto& callback : m_savedCallbacks) {
			callback(name);
		}

		return true;
	}

	bool PerspectiveManager::loadPerspective(const wxString& name) {
		if (!m_dockManager || name.IsEmpty()) {
			return false;
		}

		auto it = m_perspectives.find(name);
		if (it == m_perspectives.end() || !it->second) {
			return false;
		}

		// Restore layout
		bool success = m_dockManager->restoreState(it->second->layoutData());

		if (success) {
			m_currentPerspective = name;

			// Notify callbacks
			for (const auto& callback : m_loadedCallbacks) {
				callback(name);
			}
		}

		return success;
	}

	bool PerspectiveManager::deletePerspective(const wxString& name) {
		if (name.IsEmpty()) {
			return false;
		}

		auto it = m_perspectives.find(name);
		if (it == m_perspectives.end()) {
			return false;
		}

		m_perspectives.erase(it);

		if (m_currentPerspective == name) {
			m_currentPerspective.Clear();
		}

		// Notify callbacks
		for (const auto& callback : m_deletedCallbacks) {
			callback(name);
		}

		return true;
	}

	bool PerspectiveManager::renamePerspective(const wxString& oldName, const wxString& newName) {
		if (oldName.IsEmpty() || newName.IsEmpty() || oldName == newName) {
			return false;
		}

		auto it = m_perspectives.find(oldName);
		if (it == m_perspectives.end()) {
			return false;
		}

		// Check if new name already exists
		if (m_perspectives.find(newName) != m_perspectives.end()) {
			return false;
		}

		// Move perspective
		auto perspective = std::move(it->second);
		perspective->setName(newName);
		m_perspectives.erase(it);
		m_perspectives[newName] = std::move(perspective);

		if (m_currentPerspective == oldName) {
			m_currentPerspective = newName;
		}

		return true;
	}

	std::vector<wxString> PerspectiveManager::perspectiveNames() const {
		std::vector<wxString> names;
		for (const auto& pair : m_perspectives) {
			names.push_back(pair.first);
		}
		return names;
	}

	Perspective* PerspectiveManager::perspective(const wxString& name) const {
		auto it = m_perspectives.find(name);
		if (it != m_perspectives.end()) {
			return it->second.get();
		}
		return nullptr;
	}

	bool PerspectiveManager::hasPerspective(const wxString& name) const {
		return m_perspectives.find(name) != m_perspectives.end();
	}

	void PerspectiveManager::setCurrentPerspective(const wxString& name) {
		m_currentPerspective = name;
	}

	bool PerspectiveManager::exportPerspective(const wxString& name, const wxString& filename) {
		auto it = m_perspectives.find(name);
		if (it == m_perspectives.end() || !it->second) {
			return false;
		}

		wxXmlDocument doc;
		wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "Perspective");
		doc.SetRoot(root);

		it->second->saveToXml(root);

		return doc.Save(filename);
	}

	bool PerspectiveManager::importPerspective(const wxString& filename, const wxString& newName) {
		wxXmlDocument doc;
		if (!doc.Load(filename)) {
			return false;
		}

		wxXmlNode* root = doc.GetRoot();
		if (!root || root->GetName() != "Perspective") {
			return false;
		}

		auto perspective = std::make_unique<Perspective>();
		if (!perspective->loadFromXml(root)) {
			return false;
		}

		// Use new name if provided
		wxString name = newName.IsEmpty() ? perspective->name() : newName;

		// Ensure unique name
		name = generateUniqueName(name);
		perspective->setName(name);

		m_perspectives[name] = std::move(perspective);

		return true;
	}

	void PerspectiveManager::saveToFile(const wxString& filename) {
		wxXmlDocument doc;
		wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "Perspectives");
		doc.SetRoot(root);

		// Save current perspective name
		root->AddAttribute("current", m_currentPerspective);

		// Save all perspectives
		for (const auto& pair : m_perspectives) {
			wxXmlNode* perspNode = new wxXmlNode(wxXML_ELEMENT_NODE, "Perspective");
			pair.second->saveToXml(perspNode);
			root->AddChild(perspNode);
		}

		doc.Save(filename);
	}

	bool PerspectiveManager::loadFromFile(const wxString& filename) {
		wxXmlDocument doc;
		if (!doc.Load(filename)) {
			return false;
		}

		wxXmlNode* root = doc.GetRoot();
		if (!root || root->GetName() != "Perspectives") {
			return false;
		}

		// Clear existing perspectives
		m_perspectives.clear();

		// Load current perspective name
		m_currentPerspective = root->GetAttribute("current");

		// Load all perspectives
		wxXmlNode* perspNode = root->GetChildren();
		while (perspNode) {
			if (perspNode->GetName() == "Perspective") {
				auto perspective = std::make_unique<Perspective>();
				if (perspective->loadFromXml(perspNode)) {
					m_perspectives[perspective->name()] = std::move(perspective);
				}
			}
			perspNode = perspNode->GetNext();
		}

		return true;
	}

	void PerspectiveManager::enableAutoSave(bool enable, int intervalSeconds) {
		m_autoSaveEnabled = enable;

		if (enable && m_autoSaveTimer) {
			m_autoSaveTimer->Start(intervalSeconds * 1000);
		}
		else if (m_autoSaveTimer) {
			m_autoSaveTimer->Stop();
		}
	}

	void PerspectiveManager::onPerspectiveSaved(const PerspectiveCallback& callback) {
		m_savedCallbacks.push_back(callback);
	}

	void PerspectiveManager::onPerspectiveLoaded(const PerspectiveCallback& callback) {
		m_loadedCallbacks.push_back(callback);
	}

	void PerspectiveManager::onPerspectiveDeleted(const PerspectiveCallback& callback) {
		m_deletedCallbacks.push_back(callback);
	}

	void PerspectiveManager::createDefaultPerspectives() {
		// Create a default perspective
		savePerspective("Default", "Default window layout");

		// Create a debugging perspective
		savePerspective("Debug", "Layout optimized for debugging");

		// Create a design perspective
		savePerspective("Design", "Layout optimized for UI design");
	}

	void PerspectiveManager::resetToDefault() {
		if (hasPerspective("Default")) {
			loadPerspective("Default");
		}
	}

	void PerspectiveManager::onAutoSaveTimer(wxTimerEvent& event) {
		if (m_autoSaveEnabled && !m_currentPerspective.IsEmpty()) {
			savePerspective(m_currentPerspective);
		}
	}

	wxBitmap PerspectiveManager::capturePreview() {
		if (!m_dockManager || !m_dockManager->containerWidget()) {
			return wxNullBitmap;
		}

		// Get the container widget
		wxWindow* container = m_dockManager->containerWidget();
		wxSize containerSize = container->GetSize();

		// Create a smaller preview size
		wxSize previewSize(200, 150);

		// Calculate scale
		double scaleX = static_cast<double>(previewSize.GetWidth()) / containerSize.GetWidth();
		double scaleY = static_cast<double>(previewSize.GetHeight()) / containerSize.GetHeight();
		double scale = std::min(scaleX, scaleY);

		// Create bitmap
		wxBitmap preview(previewSize);
		wxMemoryDC memDC(preview);

		// Clear background
		memDC.SetBackground(*wxWHITE_BRUSH);
		memDC.Clear();

		// Draw scaled container
		// Note: This is a simplified preview - in a real implementation,
		// we would capture the actual window content
		memDC.SetPen(wxPen(*wxBLACK, 1));
		memDC.SetBrush(*wxLIGHT_GREY_BRUSH);

		// Draw a simple representation
		int x = 5, y = 5;
		int w = previewSize.GetWidth() - 10;
		int h = previewSize.GetHeight() - 10;

		memDC.DrawRectangle(x, y, w, h);
		memDC.DrawText("Preview", x + 10, y + 10);

		memDC.SelectObject(wxNullBitmap);

		return preview;
	}

	wxString PerspectiveManager::generateUniqueName(const wxString& baseName) const {
		wxString name = baseName;
		int counter = 1;

		while (hasPerspective(name)) {
			name = wxString::Format("%s_%d", baseName, counter);
			counter++;
		}

		return name;
	}

	// PerspectiveDialog implementation
	enum {
		ID_PERSPECTIVE_LIST = wxID_HIGHEST + 1,
		ID_LOAD_PERSPECTIVE,
		ID_DELETE_PERSPECTIVE,
		ID_RENAME_PERSPECTIVE,
		ID_EXPORT_PERSPECTIVE,
		ID_IMPORT_PERSPECTIVE
	};

	wxBEGIN_EVENT_TABLE(PerspectiveDialog, wxDialog)
		EVT_LIST_ITEM_SELECTED(ID_PERSPECTIVE_LIST, PerspectiveDialog::OnPerspectiveSelected)
		EVT_BUTTON(ID_LOAD_PERSPECTIVE, PerspectiveDialog::OnLoadPerspective)
		EVT_BUTTON(wxID_OK, PerspectiveDialog::OnSavePerspective)
		EVT_BUTTON(ID_DELETE_PERSPECTIVE, PerspectiveDialog::OnDeletePerspective)
		EVT_BUTTON(ID_RENAME_PERSPECTIVE, PerspectiveDialog::OnRenamePerspective)
		EVT_BUTTON(ID_EXPORT_PERSPECTIVE, PerspectiveDialog::OnExportPerspective)
		EVT_BUTTON(ID_IMPORT_PERSPECTIVE, PerspectiveDialog::OnImportPerspective)
		wxEND_EVENT_TABLE()

		PerspectiveDialog::PerspectiveDialog(wxWindow* parent, PerspectiveManager* manager)
		: wxDialog(parent, wxID_ANY, "Manage Perspectives", wxDefaultPosition, wxSize(600, 400))
		, m_manager(manager)
	{
		CreateControls();
		UpdatePerspectiveList();
		Centre();
	}

	PerspectiveDialog::~PerspectiveDialog() {
	}

	void PerspectiveDialog::CreateControls() {
		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

		// Create splitter
		wxSplitterWindow* splitter = new wxSplitterWindow(this);

		// Left panel - perspective list
		wxPanel* leftPanel = new wxPanel(splitter);
		wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);

		wxStaticText* listLabel = new wxStaticText(leftPanel, wxID_ANY, "Perspectives:");
		leftSizer->Add(listLabel, 0, wxALL, 5);

		m_perspectiveList = new wxListCtrl(leftPanel, ID_PERSPECTIVE_LIST,
			wxDefaultPosition, wxDefaultSize,
			wxLC_REPORT | wxLC_SINGLE_SEL);
		m_perspectiveList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 150);
		m_perspectiveList->AppendColumn("Modified", wxLIST_FORMAT_LEFT, 150);
		leftSizer->Add(m_perspectiveList, 1, wxEXPAND | wxALL, 5);

		leftPanel->SetSizer(leftSizer);

		// Right panel - preview and details
		wxPanel* rightPanel = new wxPanel(splitter);
		wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);

		wxStaticText* previewLabel = new wxStaticText(rightPanel, wxID_ANY, "Preview:");
		rightSizer->Add(previewLabel, 0, wxALL, 5);

		m_previewImage = new wxStaticBitmap(rightPanel, wxID_ANY, wxNullBitmap);
		m_previewImage->SetMinSize(wxSize(200, 150));
		rightSizer->Add(m_previewImage, 0, wxALL | wxALIGN_CENTER, 5);

		wxStaticText* descLabel = new wxStaticText(rightPanel, wxID_ANY, "Description:");
		rightSizer->Add(descLabel, 0, wxALL, 5);

		m_descriptionText = new wxTextCtrl(rightPanel, wxID_ANY, "",
			wxDefaultPosition, wxDefaultSize,
			wxTE_MULTILINE | wxTE_READONLY);
		rightSizer->Add(m_descriptionText, 1, wxEXPAND | wxALL, 5);

		rightPanel->SetSizer(rightSizer);

		// Set up splitter
		splitter->SplitVertically(leftPanel, rightPanel, 300);
		mainSizer->Add(splitter, 1, wxEXPAND | wxALL, 5);

		// Button panel
		wxPanel* buttonPanel = new wxPanel(this);
		wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

		m_loadButton = new wxButton(buttonPanel, ID_LOAD_PERSPECTIVE, "Load");
		m_deleteButton = new wxButton(buttonPanel, ID_DELETE_PERSPECTIVE, "Delete");
		m_renameButton = new wxButton(buttonPanel, ID_RENAME_PERSPECTIVE, "Rename");
		m_exportButton = new wxButton(buttonPanel, ID_EXPORT_PERSPECTIVE, "Export...");
		wxButton* importButton = new wxButton(buttonPanel, ID_IMPORT_PERSPECTIVE, "Import...");
		wxButton* saveButton = new wxButton(buttonPanel, wxID_OK, "Save Current");
		wxButton* closeButton = new wxButton(buttonPanel, wxID_CANCEL, "Close");

		buttonSizer->Add(m_loadButton, 0, wxALL, 5);
		buttonSizer->Add(m_deleteButton, 0, wxALL, 5);
		buttonSizer->Add(m_renameButton, 0, wxALL, 5);
		buttonSizer->Add(m_exportButton, 0, wxALL, 5);
		buttonSizer->Add(importButton, 0, wxALL, 5);
		buttonSizer->AddStretchSpacer();
		buttonSizer->Add(saveButton, 0, wxALL, 5);
		buttonSizer->Add(closeButton, 0, wxALL, 5);

		buttonPanel->SetSizer(buttonSizer);
		mainSizer->Add(buttonPanel, 0, wxEXPAND);

		SetSizer(mainSizer);

		// Initially disable buttons
		m_loadButton->Enable(false);
		m_deleteButton->Enable(false);
		m_renameButton->Enable(false);
		m_exportButton->Enable(false);
	}

	void PerspectiveDialog::UpdatePerspectiveList() {
		m_perspectiveList->DeleteAllItems();

		std::vector<wxString> names = m_manager->perspectiveNames();
		for (size_t i = 0; i < names.size(); ++i) {
			Perspective* persp = m_manager->perspective(names[i]);
			if (persp) {
				long index = m_perspectiveList->InsertItem(i, persp->name());
				m_perspectiveList->SetItem(index, 1, persp->modifiedDate().Format("%Y-%m-%d %H:%M"));
			}
		}
	}

	void PerspectiveDialog::OnPerspectiveSelected(wxListEvent& event) {
		long selected = event.GetIndex();
		if (selected >= 0) {
			wxString name = m_perspectiveList->GetItemText(selected, 0);
			Perspective* persp = m_manager->perspective(name);

			if (persp) {
				// Update preview
				if (persp->preview().IsOk()) {
					m_previewImage->SetBitmap(persp->preview());
				}
				else {
					m_previewImage->SetBitmap(wxNullBitmap);
				}

				// Update description
				m_descriptionText->SetValue(persp->description());

				// Enable buttons
				m_loadButton->Enable(true);
				m_deleteButton->Enable(true);
				m_renameButton->Enable(true);
				m_exportButton->Enable(true);
			}
		}
	}

	void PerspectiveDialog::OnLoadPerspective(wxCommandEvent& event) {
		long selected = m_perspectiveList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (selected >= 0) {
			wxString name = m_perspectiveList->GetItemText(selected, 0);
			if (m_manager->loadPerspective(name)) {
				EndModal(wxID_OK);
			}
		}
	}

	void PerspectiveDialog::OnSavePerspective(wxCommandEvent& event) {
		wxTextEntryDialog dlg(this, "Enter name for perspective:", "Save Perspective");
		if (dlg.ShowModal() == wxID_OK) {
			wxString name = dlg.GetValue();
			if (!name.IsEmpty()) {
				wxTextEntryDialog descDlg(this, "Enter description (optional):", "Perspective Description");
				wxString description;
				if (descDlg.ShowModal() == wxID_OK) {
					description = descDlg.GetValue();
				}

				if (m_manager->savePerspective(name, description)) {
					UpdatePerspectiveList();
				}
			}
		}
	}

	void PerspectiveDialog::OnDeletePerspective(wxCommandEvent& event) {
		long selected = m_perspectiveList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (selected >= 0) {
			wxString name = m_perspectiveList->GetItemText(selected, 0);

			wxMessageDialog dlg(this,
				wxString::Format("Delete perspective '%s'?", name),
				"Confirm Delete",
				wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);

			if (dlg.ShowModal() == wxID_YES) {
				if (m_manager->deletePerspective(name)) {
					UpdatePerspectiveList();

					// Clear preview
					m_previewImage->SetBitmap(wxNullBitmap);
					m_descriptionText->Clear();

					// Disable buttons
					m_loadButton->Enable(false);
					m_deleteButton->Enable(false);
					m_renameButton->Enable(false);
					m_exportButton->Enable(false);
				}
			}
		}
	}

	void PerspectiveDialog::OnRenamePerspective(wxCommandEvent& event) {
		long selected = m_perspectiveList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (selected >= 0) {
			wxString oldName = m_perspectiveList->GetItemText(selected, 0);

			wxTextEntryDialog dlg(this, "Enter new name:", "Rename Perspective", oldName);
			if (dlg.ShowModal() == wxID_OK) {
				wxString newName = dlg.GetValue();
				if (!newName.IsEmpty() && newName != oldName) {
					if (m_manager->renamePerspective(oldName, newName)) {
						UpdatePerspectiveList();
					}
				}
			}
		}
	}

	void PerspectiveDialog::OnExportPerspective(wxCommandEvent& event) {
		long selected = m_perspectiveList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (selected >= 0) {
			wxString name = m_perspectiveList->GetItemText(selected, 0);

			wxFileDialog dlg(this, "Export Perspective", "", name + ".xml",
				"XML files (*.xml)|*.xml",
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

			if (dlg.ShowModal() == wxID_OK) {
				m_manager->exportPerspective(name, dlg.GetPath());
			}
		}
	}

	void PerspectiveDialog::OnImportPerspective(wxCommandEvent& event) {
		wxFileDialog dlg(this, "Import Perspective", "", "",
			"XML files (*.xml)|*.xml",
			wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (dlg.ShowModal() == wxID_OK) {
			if (m_manager->importPerspective(dlg.GetPath())) {
				UpdatePerspectiveList();
			}
		}
	}

	// PerspectiveToolBar implementation
	enum {
		ID_PERSPECTIVE_CHOICE = wxID_HIGHEST + 100,
		ID_MANAGE_PERSPECTIVES,
		ID_SAVE_CURRENT_PERSPECTIVE
	};

	wxBEGIN_EVENT_TABLE(PerspectiveToolBar, wxToolBar)
		EVT_CHOICE(ID_PERSPECTIVE_CHOICE, PerspectiveToolBar::OnPerspectiveSelected)
		EVT_TOOL(ID_MANAGE_PERSPECTIVES, PerspectiveToolBar::OnManagePerspectives)
		EVT_TOOL(ID_SAVE_CURRENT_PERSPECTIVE, PerspectiveToolBar::OnSaveCurrentPerspective)
		wxEND_EVENT_TABLE()

		PerspectiveToolBar::PerspectiveToolBar(wxWindow* parent, PerspectiveManager* manager)
		: wxToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_FLAT)
		, m_manager(manager)
	{
		// Add perspective selector
		AddControl(new wxStaticText(this, wxID_ANY, "Perspective: "));

		m_perspectiveChoice = new wxChoice(this, ID_PERSPECTIVE_CHOICE);
		m_perspectiveChoice->SetMinSize(wxSize(150, -1));
		AddControl(m_perspectiveChoice);

		AddSeparator();

		// Add tools
		AddTool(ID_SAVE_CURRENT_PERSPECTIVE, "Save",
			wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR),
			"Save current perspective");

		AddTool(ID_MANAGE_PERSPECTIVES, "Manage",
			wxArtProvider::GetBitmap(wxART_LIST_VIEW, wxART_TOOLBAR),
			"Manage perspectives");

		Realize();

		UpdatePerspectives();
	}

	PerspectiveToolBar::~PerspectiveToolBar() {
	}

	void PerspectiveToolBar::UpdatePerspectives() {
		m_perspectiveChoice->Clear();

		std::vector<wxString> names = m_manager->perspectiveNames();
		for (const auto& name : names) {
			m_perspectiveChoice->Append(name);
		}

		// Select current perspective
		wxString current = m_manager->currentPerspective();
		if (!current.IsEmpty()) {
			m_perspectiveChoice->SetStringSelection(current);
		}
	}

	void PerspectiveToolBar::OnPerspectiveSelected(wxCommandEvent& event) {
		wxString selected = m_perspectiveChoice->GetStringSelection();
		if (!selected.IsEmpty()) {
			m_manager->loadPerspective(selected);
		}
	}

	void PerspectiveToolBar::OnManagePerspectives(wxCommandEvent& event) {
		PerspectiveDialog dlg(GetParent(), m_manager);
		if (dlg.ShowModal() == wxID_OK) {
			UpdatePerspectives();
		}
	}

	void PerspectiveToolBar::OnSaveCurrentPerspective(wxCommandEvent& event) {
		wxString current = m_manager->currentPerspective();
		if (!current.IsEmpty()) {
			m_manager->savePerspective(current);
		}
		else {
			// Ask for new name
			wxTextEntryDialog dlg(GetParent(), "Enter name for perspective:", "Save Perspective");
			if (dlg.ShowModal() == wxID_OK) {
				wxString name = dlg.GetValue();
				if (!name.IsEmpty()) {
					m_manager->savePerspective(name);
					UpdatePerspectives();
				}
			}
		}
	}
} // namespace ads