#pragma once

#include <wx/wx.h>
#include <vector>
#include <map>
#include <memory>

namespace ads {

// Forward declarations
class DockManager;

/**
 * @brief Represents a saved docking layout perspective
 */
class Perspective {
public:
    Perspective(const wxString& name = wxEmptyString);
    ~Perspective();
    
    // Properties
    wxString name() const { return m_name; }
    void setName(const wxString& name) { m_name = name; }
    
    wxDateTime createdDate() const { return m_createdDate; }
    wxDateTime modifiedDate() const { return m_modifiedDate; }
    
    wxString description() const { return m_description; }
    void setDescription(const wxString& desc) { m_description = desc; }
    
    // Layout data
    wxString layoutData() const { return m_layoutData; }
    void setLayoutData(const wxString& data);
    
    // Icon/preview
    wxBitmap preview() const { return m_preview; }
    void setPreview(const wxBitmap& preview) { m_preview = preview; }
    
    // Serialization
    void saveToXml(wxXmlNode* node) const;
    bool loadFromXml(wxXmlNode* node);
    
private:
    wxString m_name;
    wxString m_description;
    wxString m_layoutData;
    wxDateTime m_createdDate;
    wxDateTime m_modifiedDate;
    wxBitmap m_preview;
};

/**
 * @brief Manages multiple layout perspectives
 */
class PerspectiveManager {
public:
    PerspectiveManager(DockManager* dockManager);
    ~PerspectiveManager();
    
    // Perspective management
    bool savePerspective(const wxString& name, const wxString& description = wxEmptyString);
    bool loadPerspective(const wxString& name);
    bool deletePerspective(const wxString& name);
    bool renamePerspective(const wxString& oldName, const wxString& newName);
    
    // Query perspectives
    std::vector<wxString> perspectiveNames() const;
    Perspective* perspective(const wxString& name) const;
    bool hasPerspective(const wxString& name) const;
    
    // Current perspective
    wxString currentPerspective() const { return m_currentPerspective; }
    void setCurrentPerspective(const wxString& name);
    
    // Import/Export
    bool exportPerspective(const wxString& name, const wxString& filename);
    bool importPerspective(const wxString& filename, const wxString& newName = wxEmptyString);
    
    // Persistence
    void saveToFile(const wxString& filename);
    bool loadFromFile(const wxString& filename);
    
    // Auto-save
    void enableAutoSave(bool enable, int intervalSeconds = 300);
    bool isAutoSaveEnabled() const { return m_autoSaveEnabled; }
    
    // Events
    typedef std::function<void(const wxString&)> PerspectiveCallback;
    void onPerspectiveSaved(const PerspectiveCallback& callback);
    void onPerspectiveLoaded(const PerspectiveCallback& callback);
    void onPerspectiveDeleted(const PerspectiveCallback& callback);
    
    // Default perspectives
    void createDefaultPerspectives();
    void resetToDefault();
    
private:
    DockManager* m_dockManager;
    std::map<wxString, std::unique_ptr<Perspective>> m_perspectives;
    wxString m_currentPerspective;
    
    // Auto-save
    bool m_autoSaveEnabled;
    wxTimer* m_autoSaveTimer;
    void onAutoSaveTimer(wxTimerEvent& event);
    
    // Callbacks
    std::vector<PerspectiveCallback> m_savedCallbacks;
    std::vector<PerspectiveCallback> m_loadedCallbacks;
    std::vector<PerspectiveCallback> m_deletedCallbacks;
    
    // Helpers
    wxBitmap capturePreview();
    wxString generateUniqueName(const wxString& baseName) const;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Dialog for managing perspectives
 */
class PerspectiveDialog : public wxDialog {
public:
    PerspectiveDialog(wxWindow* parent, PerspectiveManager* manager);
    ~PerspectiveDialog();
    
private:
    void CreateControls();
    void UpdatePerspectiveList();
    void OnPerspectiveSelected(wxListEvent& event);
    void OnLoadPerspective(wxCommandEvent& event);
    void OnSavePerspective(wxCommandEvent& event);
    void OnDeletePerspective(wxCommandEvent& event);
    void OnRenamePerspective(wxCommandEvent& event);
    void OnExportPerspective(wxCommandEvent& event);
    void OnImportPerspective(wxCommandEvent& event);
    
    PerspectiveManager* m_manager;
    wxListCtrl* m_perspectiveList;
    wxStaticBitmap* m_previewImage;
    wxTextCtrl* m_descriptionText;
    wxButton* m_loadButton;
    wxButton* m_deleteButton;
    wxButton* m_renameButton;
    wxButton* m_exportButton;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Toolbar for quick perspective switching
 */
class PerspectiveToolBar : public wxToolBar {
public:
    PerspectiveToolBar(wxWindow* parent, PerspectiveManager* manager);
    ~PerspectiveToolBar();
    
    void UpdatePerspectives();
    
private:
    void OnPerspectiveSelected(wxCommandEvent& event);
    void OnManagePerspectives(wxCommandEvent& event);
    void OnSaveCurrentPerspective(wxCommandEvent& event);
    
    PerspectiveManager* m_manager;
    wxChoice* m_perspectiveChoice;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads