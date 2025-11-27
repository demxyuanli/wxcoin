#ifndef CONFIG_MANAGER_DIALOG_H
#define CONFIG_MANAGER_DIALOG_H

#include <wx/wx.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/scrolwin.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/colordlg.h>
#include <string>
#include <map>
#include <vector>
#include "config/UnifiedConfigManager.h"
#include "widgets/FramelessModalPopup.h"

class ConfigItemEditor;
class ConfigCategoryEditor;
class FlatProgressBar;


class ConfigManagerDialog : public FramelessModalPopup {
public:
    ConfigManagerDialog(wxWindow* parent);
    virtual ~ConfigManagerDialog();
    
private:
    void createUI();
    void populateCategoryList();
    void createCategoryMenuItem(const ConfigCategory& category, wxSizer* sizer);
    void onCategoryMenuSelected(wxWindow* itemPanel);
    void loadAllConfigurations();  // Load all configurations with progress bar
    void onSearch();
    void onItemChanged(const std::string& key, const std::string& value);
    void onApply(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onReset();
    void refreshItemEditors();
    
    wxScrolledWindow* m_categoryScrollPanel;  // Menu-style navigation panel
    wxTextCtrl* m_searchCtrl;   // Search box
    wxPanel* m_editorContainer;  // Container for editors within scrolled panel
    wxScrolledWindow* m_scrolledPanel;
    wxButton* m_applyButton;
    wxButton* m_okButton;
    wxButton* m_cancelButton;
    wxButton* m_resetButton;
    
    UnifiedConfigManager* m_configManager;
    ConfigCategoryEditor* m_currentEditor;
    std::string m_currentCategory;
    std::map<std::string, ConfigCategoryEditor*> m_editorCache;  // Cache all editors by category ID
    std::map<wxWindow*, std::string> m_categoryButtonMap;  // Map menu button to category ID
    std::vector<wxWindow*> m_categoryButtons;  // All category menu buttons for selection management
    bool m_allConfigsLoaded;  // Flag to indicate if all configs are loaded
    
    wxDECLARE_EVENT_TABLE();
};

class ConfigItemEditor : public wxPanel {
public:
    ConfigItemEditor(wxWindow* parent, const ConfigItem& item, 
                     std::function<void(const std::string& value)> onChange);
    ~ConfigItemEditor();
    
    void setValue(const std::string& value);
    std::string getValue() const;
    bool isModified() const;
    void reset();
    
private:
    void createUI();
    void onValueChanged();
    void onSizeValueChanged();
    void onColorButton(wxCommandEvent& event);
    void updateVisualIndication();
    std::string colorToString(const wxColour& color) const;
    wxColour stringToColor(const std::string& str) const;
    std::string getCurrentThemeColor(const std::string& multiThemeValue) const;
    
    ConfigItem m_item;
    std::function<void(const std::string&)> m_onChange;
    
    wxStaticText* m_label;
    wxStaticText* m_description;
    wxTextCtrl* m_textCtrl;
    wxCheckBox* m_checkBox;
    wxChoice* m_choice;
    wxSpinCtrl* m_spinCtrl;
    wxSpinCtrlDouble* m_spinCtrlDouble;
    wxButton* m_colorButton;
    wxPanel* m_colorPreview;
    wxSpinCtrl* m_sizeSpinCtrl1;  // For size pairs like width,height
    wxSpinCtrl* m_sizeSpinCtrl2;
    wxStaticText* m_sizeSeparator;
    
    std::string m_originalValue;
    bool m_modified;
    wxColour m_originalBgColor;
};

#endif // CONFIG_MANAGER_DIALOG_H

