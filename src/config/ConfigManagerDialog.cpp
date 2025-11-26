#include "config/ConfigManagerDialog.h"
#include "config/UnifiedConfigManager.h"
#include "config/ConfigManager.h"
#include "config/editor/ConfigEditorFactory.h"
#include "config/editor/ConfigCategoryEditor.h"
#include "widgets/FlatProgressBar.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/spinctrl.h>
#include <wx/colordlg.h>
#include <wx/dialog.h>
#include <sstream>

wxBEGIN_EVENT_TABLE(ConfigManagerDialog, FramelessModalPopup)
    EVT_TREE_SEL_CHANGED(wxID_ANY, ConfigManagerDialog::onCategorySelected)
wxEND_EVENT_TABLE()

ConfigManagerDialog::ConfigManagerDialog(wxWindow* parent)
    : FramelessModalPopup(parent, "Configuration Manager", wxSize(1200, 700))
    , m_categoryTree(nullptr)
    , m_scrolledPanel(nullptr)
    , m_splitter(nullptr)
    , m_applyButton(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
    , m_configManager(&UnifiedConfigManager::getInstance())
    , m_currentEditor(nullptr)
    , m_currentCategory("")
    , m_allConfigsLoaded(false)
{
    // Print diagnostics for debugging
    m_configManager->printDiagnostics();

    // Create UI on the content panel provided by FramelessModalPopup
    createUI();
    populateCategoryTree();
    
    // Load all configurations with flat progress bar first (before showing main window)
    loadAllConfigurations();
    
    // Show the main window after loading is complete (centered by FramelessModalPopup)
    Show();
    wxSafeYield();  // Allow window to render

    if (m_categoryTree->GetCount() > 0) {
        m_categoryTree->SelectItem(m_categoryTree->GetFirstVisibleItem());
    }
}

ConfigManagerDialog::~ConfigManagerDialog() {
    // Clean up all cached editors
    for (auto& pair : m_editorCache) {
        if (pair.second) {
            delete pair.second;
        }
    }
    m_editorCache.clear();
    m_currentEditor = nullptr;
}

void ConfigManagerDialog::createUI() {
    // Use the content panel provided by FramelessModalPopup
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_splitter = new wxSplitterWindow(m_contentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxSP_3D | wxSP_LIVE_UPDATE);
    m_splitter->SetMinimumPaneSize(200);

    m_categoryTree = new wxTreeCtrl(m_splitter, wxID_ANY, wxDefaultPosition, wxSize(200, -1),
                                      wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_SINGLE);

    m_scrolledPanel = new wxScrolledWindow(m_splitter, wxID_ANY);
    m_scrolledPanel->SetScrollRate(10, 10);

    wxPanel* editorContainer = new wxPanel(m_scrolledPanel);
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    editorContainer->SetSizer(contentSizer);

    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    scrolledSizer->Add(editorContainer, 1, wxEXPAND | wxALL, 5);
    m_scrolledPanel->SetSizer(scrolledSizer);

    m_splitter->SplitVertically(m_categoryTree, m_scrolledPanel, 250);

    mainSizer->Add(m_splitter, 1, wxEXPAND | wxALL, 5);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_resetButton = new wxButton(m_contentPanel, wxID_ANY, "Reset");
    m_applyButton = new wxButton(m_contentPanel, wxID_APPLY, "Apply");
    m_okButton = new wxButton(m_contentPanel, wxID_OK, "OK");
    m_cancelButton = new wxButton(m_contentPanel, wxID_CANCEL, "Cancel");

    buttonSizer->Add(m_resetButton, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_applyButton, 0, wxALL, 5);
    buttonSizer->Add(m_okButton, 0, wxALL, 5);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);

    m_contentPanel->SetSizer(mainSizer);

    m_resetButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onReset(); });
    m_applyButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onApply(event); });
    m_okButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onOK(event); });
    m_cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onCancel(event); });
    
    // Store editor container for later use
    m_editorContainer = editorContainer;
}

void ConfigManagerDialog::populateCategoryTree() {
    m_categoryTree->DeleteAllItems();
    wxTreeItemId root = m_categoryTree->AddRoot("Categories");

    auto categories = m_configManager->getCategories();
    LOG_INF("Populating category tree with " + std::to_string(categories.size()) + " categories", "ConfigManagerDialog");

    std::map<std::string, wxTreeItemId> categoryItems;

    for (const auto& category : categories) {
        LOG_INF("Adding category '" + category.id + "' with " + std::to_string(category.items.size()) + " items", "ConfigManagerDialog");
        wxTreeItemId catItem = m_categoryTree->AppendItem(root, category.displayName);
        // Use custom tree item data to store category ID
        CategoryTreeItemData* data = new CategoryTreeItemData(category.id);
        m_categoryTree->SetItemData(catItem, data);
        categoryItems[category.id] = catItem;
    }

    // Expand all child items instead of the hidden root
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_categoryTree->GetFirstChild(root, cookie);
    while (child.IsOk()) {
        m_categoryTree->Expand(child);
        child = m_categoryTree->GetNextChild(root, cookie);
    }
}

void ConfigManagerDialog::loadAllConfigurations() {
    auto categories = m_configManager->getCategories();
    if (categories.empty()) {
        m_allConfigsLoaded = true;
        return;
    }

    // Create flat style progress dialog with theme-adapted colors
    // Use parent window (main frame) instead of this (config dialog) so it can show before config dialog
    wxWindow* parentWindow = GetParent();
    if (!parentWindow) {
        parentWindow = wxTheApp->GetTopWindow();
    }
    wxDialog* progressDialog = new wxDialog(parentWindow, wxID_ANY, "Loading Configuration", 
                                            wxDefaultPosition, wxSize(400, 150),
                                            wxNO_BORDER | wxFRAME_SHAPED);
    
    // Use PanelDialogBgColour for dialog background, with fallback chain
    wxColour bgColor = CFG_COLOUR("PanelDialogBgColour");
    // Check if color is valid and not the error color (red)
    if (!bgColor.IsOk() || (bgColor.Red() == 255 && bgColor.Green() == 0 && bgColor.Blue() == 0)) {
        // Try PanelPopupBgColour as fallback
        bgColor = CFG_COLOUR("PanelPopupBgColour");
        if (!bgColor.IsOk() || (bgColor.Red() == 255 && bgColor.Green() == 0 && bgColor.Blue() == 0)) {
            // Try SecondaryBackgroundColour
            bgColor = CFG_COLOUR("SecondaryBackgroundColour");
            if (!bgColor.IsOk() || (bgColor.Red() == 255 && bgColor.Green() == 0 && bgColor.Blue() == 0)) {
                // Final fallback: use a light gray that works in all themes
                bgColor = wxColour(250, 250, 250);
            }
        }
    }
    
    // Create a content panel to ensure background color is properly applied
    wxPanel* contentPanel = new wxPanel(progressDialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    contentPanel->SetBackgroundColour(bgColor);
    contentPanel->SetDoubleBuffered(true);
    
    // Also set dialog background (though content panel will cover it)
    progressDialog->SetBackgroundColour(bgColor);
    progressDialog->SetDoubleBuffered(true);
    
    wxBoxSizer* progressSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title label with theme text color
    wxStaticText* progressLabel = new wxStaticText(contentPanel, wxID_ANY, 
                                                   "Loading all configuration categories...");
    wxColour textColor = CFG_COLOUR("PrimaryTextColour");
    if (!textColor.IsOk() || (textColor.Red() == 255 && textColor.Green() == 0 && textColor.Blue() == 0)) {
        textColor = wxColour(100, 100, 100);  // Fallback to dark gray
    }
    progressLabel->SetForegroundColour(textColor);
    progressSizer->Add(progressLabel, 0, wxALL | wxALIGN_CENTER, 15);
    
    // Create flat progress bar (colors are automatically set from theme in InitializeDefaultColors)
    FlatProgressBar* progressBar = new FlatProgressBar(contentPanel, wxID_ANY, 0, 0, 
                                                        static_cast<int>(categories.size()),
                                                        wxDefaultPosition, wxSize(350, 25),
                                                        FlatProgressBar::ProgressBarStyle::MODERN_LINEAR);
    progressBar->SetShowPercentage(true);
    progressBar->SetTextFollowProgress(true);
    progressBar->SetCornerRadius(12);
    // Progress bar colors are already theme-adapted via InitializeDefaultColors:
    // - Background: SecondaryBackgroundColour
    // - Progress: AccentColour
    // - Text: PrimaryTextColour
    progressSizer->Add(progressBar, 0, wxALL | wxALIGN_CENTER, 15);
    
    // Status label with theme text color
    wxStaticText* statusLabel = new wxStaticText(contentPanel, wxID_ANY, "");
    statusLabel->SetForegroundColour(textColor);
    progressSizer->Add(statusLabel, 0, wxALL | wxALIGN_CENTER, 10);
    
    // Set sizer for content panel
    contentPanel->SetSizer(progressSizer);
    
    // Create dialog sizer to hold content panel
    wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(contentPanel, 1, wxEXPAND);
    progressDialog->SetSizer(dialogSizer);
    progressDialog->Layout();
    progressDialog->CentreOnParent();
    // Show progress dialog first (before config dialog)
    progressDialog->Show();
    wxSafeYield();  // Allow progress dialog to render

    int currentProgress = 0;
    
    // Load all category editors
    for (const auto& category : categories) {
        // Update progress bar
        progressBar->SetValue(currentProgress);
        statusLabel->SetLabel("Loading: " + category.displayName);
        progressDialog->Refresh();
        wxSafeYield();  // Allow UI to update

        // Create editor for this category
        ConfigCategoryEditor* editor = ConfigEditorFactory::createEditor(m_editorContainer, m_configManager, category.id);
        if (editor) {
            editor->setChangeCallback([this]() {
                if (m_currentEditor && m_currentEditor->hasChanges()) {
                    m_applyButton->Enable(true);
                }
            });
            
            // Load configuration (this creates all UI elements)
            editor->loadConfig();
            
            // Hide editor initially (will be shown when category is selected)
            editor->Hide();
            
            // Cache the editor
            m_editorCache[category.id] = editor;
        }

        currentProgress++;
    }

    // Complete progress
    progressBar->SetValue(categories.size());
    statusLabel->SetLabel("Configuration loaded successfully");
    progressDialog->Refresh();
    wxSafeYield();
    wxMilliSleep(300);  // Brief pause to show completion
    
    progressDialog->Destroy();
    m_allConfigsLoaded = true;
}

void ConfigManagerDialog::onCategorySelected(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk()) return;

    // Get tree item data
    wxTreeItemData* itemData = m_categoryTree->GetItemData(item);
    if (!itemData) return;

    CategoryTreeItemData* data = dynamic_cast<CategoryTreeItemData*>(itemData);
    if (!data) return;

    std::string categoryId = data->GetCategoryId();
    m_currentCategory = categoryId;

    refreshItemEditors();
}

void ConfigManagerDialog::refreshItemEditors() {
    wxSizer* sizer = m_editorContainer->GetSizer();
    
    // Hide current editor if exists
    if (m_currentEditor) {
        m_currentEditor->Hide();
        sizer->Detach(m_currentEditor);
    }

    if (m_currentCategory.empty()) {
        sizer->Layout();
        m_scrolledPanel->FitInside();
        return;
    }

    // All editors should be pre-loaded, just get from cache
    auto cacheIt = m_editorCache.find(m_currentCategory);
    if (cacheIt != m_editorCache.end()) {
        m_currentEditor = cacheIt->second;
        if (m_currentEditor) {
            // Refresh values from config manager (in case config was changed externally)
            m_currentEditor->refreshValues();
            m_currentEditor->Show();
            sizer->Add(m_currentEditor, 1, wxEXPAND | wxALL, 5);
        }
    } else {
        // Fallback: if editor not in cache (shouldn't happen if loadAllConfigurations worked)
        LOG_WRN("Editor for category '" + m_currentCategory + "' not found in cache", "ConfigManagerDialog");
        m_currentEditor = ConfigEditorFactory::createEditor(m_editorContainer, m_configManager, m_currentCategory);
        if (m_currentEditor) {
            m_currentEditor->setChangeCallback([this]() {
                if (m_currentEditor && m_currentEditor->hasChanges()) {
                    m_applyButton->Enable(true);
                }
            });
            m_currentEditor->loadConfig();
            m_editorCache[m_currentCategory] = m_currentEditor;
            sizer->Add(m_currentEditor, 1, wxEXPAND | wxALL, 5);
        }
    }

    sizer->Layout();
    m_scrolledPanel->FitInside();
}

void ConfigManagerDialog::onItemChanged(const std::string& key, const std::string& value) {
    if (m_currentEditor && m_currentEditor->hasChanges()) {
        m_applyButton->Enable(true);
    }
}

void ConfigManagerDialog::onApply(wxCommandEvent& event) {
    // Save all cached editors (all categories that have been modified)
    // This ensures all changes in memory are written to file at once
    bool hasChanges = false;
    int savedCount = 0;
    
    for (auto& pair : m_editorCache) {
        if (pair.second && pair.second->hasChanges()) {
            pair.second->saveConfig();
            savedCount++;
            hasChanges = true;
        }
    }
    
    if (hasChanges) {
        // Final save to ensure all changes are persisted
        m_configManager->save();
        m_applyButton->Enable(false);
        
        wxString message = wxString::Format("Configuration saved successfully.\n%d categor%s modified.", 
                                           savedCount, savedCount == 1 ? "y" : "ies");
        wxMessageBox(message, "Success", wxOK | wxICON_INFORMATION);
    } else {
        wxMessageBox("No changes to save", "Information", wxOK | wxICON_INFORMATION);
    }
}

void ConfigManagerDialog::onOK(wxCommandEvent& event) {
    wxCommandEvent applyEvent;
    onApply(applyEvent);
    EndModal(wxID_OK);
}

void ConfigManagerDialog::onCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ConfigManagerDialog::onReset() {
    int result = wxMessageBox("Reset all changes in all categories to original values?", "Reset", wxYES_NO | wxICON_QUESTION);
    if (result == wxYES) {
        // Reset all cached editors
        for (auto& pair : m_editorCache) {
            if (pair.second) {
                pair.second->resetConfig();
            }
        }
        m_applyButton->Enable(false);
    }
}

ConfigItemEditor::ConfigItemEditor(wxWindow* parent, const ConfigItem& item,
                                   std::function<void(const std::string& value)> onChange)
    : wxPanel(parent, wxID_ANY)
    , m_item(item)
    , m_onChange(onChange)
    , m_label(nullptr)
    , m_description(nullptr)
    , m_textCtrl(nullptr)
    , m_checkBox(nullptr)
    , m_choice(nullptr)
    , m_spinCtrl(nullptr)
    , m_spinCtrlDouble(nullptr)
    , m_colorButton(nullptr)
    , m_colorPreview(nullptr)
    , m_sizeSpinCtrl1(nullptr)
    , m_sizeSpinCtrl2(nullptr)
    , m_sizeSeparator(nullptr)
    , m_originalValue(item.currentValue)
    , m_modified(false)
{
    createUI();
    setValue(item.currentValue);
}

ConfigItemEditor::~ConfigItemEditor() {
}

void ConfigItemEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_label = new wxStaticText(this, wxID_ANY, m_item.displayName);
    wxFont boldFont = m_label->GetFont();
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_label->SetFont(boldFont);
    mainSizer->Add(m_label, 0, wxALL, 5);

    if (!m_item.description.empty()) {
        m_description = new wxStaticText(this, wxID_ANY, m_item.description);
        m_description->SetForegroundColour(wxColour(100, 100, 100));
        mainSizer->Add(m_description, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
    }

    wxBoxSizer* valueSizer = new wxBoxSizer(wxHORIZONTAL);

    switch (m_item.type) {
        case ConfigValueType::Bool: {
            m_checkBox = new wxCheckBox(this, wxID_ANY, "");
            m_checkBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { onValueChanged(); });
            valueSizer->Add(m_checkBox, 0, wxALL, 5);
            break;
        }
        case ConfigValueType::Int: {
            m_spinCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                       wxSP_ARROW_KEYS, (int)m_item.minValue, (int)m_item.maxValue);
            m_spinCtrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onValueChanged(); });
            valueSizer->Add(m_spinCtrl, 1, wxALL | wxEXPAND, 5);
            break;
        }
        case ConfigValueType::Double: {
            m_spinCtrlDouble = new wxSpinCtrlDouble(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                                   wxSP_ARROW_KEYS, m_item.minValue, m_item.maxValue, 0.0, 0.1);
            m_spinCtrlDouble->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { onValueChanged(); });
            valueSizer->Add(m_spinCtrlDouble, 1, wxALL | wxEXPAND, 5);
            break;
        }
        case ConfigValueType::Enum: {
            m_choice = new wxChoice(this, wxID_ANY);
            for (const auto& val : m_item.enumValues) {
                m_choice->Append(val);
            }
            m_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onValueChanged(); });
            valueSizer->Add(m_choice, 1, wxALL | wxEXPAND, 5);
            break;
        }
        case ConfigValueType::Color: {
            m_colorPreview = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(30, 20));
            m_colorButton = new wxButton(this, wxID_ANY, "Choose Color");
            m_colorButton->Bind(wxEVT_BUTTON, &ConfigItemEditor::onColorButton, this);
            valueSizer->Add(m_colorPreview, 0, wxALL, 5);
            valueSizer->Add(m_colorButton, 0, wxALL, 5);
            break;
        }
        case ConfigValueType::Size: {
            // Special handling for size pairs like "width,height"
            std::string value = m_item.currentValue;
            size_t commaPos = value.find(',');
            if (commaPos != std::string::npos) {
                std::string widthStr = value.substr(0, commaPos);
                std::string heightStr = value.substr(commaPos + 1);

                int width = 0, height = 0;
                try { width = std::stoi(widthStr); } catch (...) {}
                try { height = std::stoi(heightStr); } catch (...) {}

                m_sizeSpinCtrl1 = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(60, -1),
                                                wxSP_ARROW_KEYS, 0, 10000, width);
                m_sizeSpinCtrl2 = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(60, -1),
                                                wxSP_ARROW_KEYS, 0, 10000, height);
                m_sizeSeparator = new wxStaticText(this, wxID_ANY, "x");

                m_sizeSpinCtrl1->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onSizeValueChanged(); });
                m_sizeSpinCtrl2->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onSizeValueChanged(); });

                valueSizer->Add(m_sizeSpinCtrl1, 0, wxALL, 2);
                valueSizer->Add(m_sizeSeparator, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 2);
                valueSizer->Add(m_sizeSpinCtrl2, 0, wxALL, 2);
            } else {
                // Fallback to single spin control
                m_spinCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                           wxSP_ARROW_KEYS, 0, 10000);
                m_spinCtrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onValueChanged(); });
                valueSizer->Add(m_spinCtrl, 1, wxALL | wxEXPAND, 5);
            }
            break;
        }
        default: {
            m_textCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
            m_textCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { onValueChanged(); });
            valueSizer->Add(m_textCtrl, 1, wxALL | wxEXPAND, 5);
            break;
        }
    }

    mainSizer->Add(valueSizer, 0, wxEXPAND | wxALL, 5);

    SetSizer(mainSizer);
    SetBackgroundColour(wxColour(250, 250, 250));
}

void ConfigItemEditor::setValue(const std::string& value) {
    m_originalValue = value;
    m_modified = false;

    switch (m_item.type) {
        case ConfigValueType::Bool:
            if (m_checkBox) {
                m_checkBox->SetValue(value == "true");
            }
            break;
        case ConfigValueType::Int:
            if (m_spinCtrl) {
                m_spinCtrl->SetValue(std::stoi(value));
            }
            break;
        case ConfigValueType::Double:
            if (m_spinCtrlDouble) {
                m_spinCtrlDouble->SetValue(std::stod(value));
            }
            break;
        case ConfigValueType::Enum:
            if (m_choice) {
                int index = m_choice->FindString(value);
                if (index != wxNOT_FOUND) {
                    m_choice->SetSelection(index);
                }
            }
            break;
        case ConfigValueType::Color: {
            wxColour color = stringToColor(value);
            if (m_colorPreview) {
                m_colorPreview->SetBackgroundColour(color);
                m_colorPreview->Refresh();
            }
            break;
        }
        case ConfigValueType::Size: {
            // Parse size pair like "1200,700"
            size_t commaPos = value.find(',');
            if (commaPos != std::string::npos && m_sizeSpinCtrl1 && m_sizeSpinCtrl2) {
                std::string widthStr = value.substr(0, commaPos);
                std::string heightStr = value.substr(commaPos + 1);

                int width = 0, height = 0;
                try { width = std::stoi(widthStr); } catch (...) {}
                try { height = std::stoi(heightStr); } catch (...) {}

                m_sizeSpinCtrl1->SetValue(width);
                m_sizeSpinCtrl2->SetValue(height);
            }
            break;
        }
        default:
            if (m_textCtrl) {
                m_textCtrl->SetValue(value);
            }
            break;
    }
}

std::string ConfigItemEditor::getValue() const {
    switch (m_item.type) {
        case ConfigValueType::Bool:
            return m_checkBox ? (m_checkBox->GetValue() ? "true" : "false") : "";
        case ConfigValueType::Int:
            return m_spinCtrl ? std::to_string(m_spinCtrl->GetValue()) : "";
        case ConfigValueType::Double:
            return m_spinCtrlDouble ? std::to_string(m_spinCtrlDouble->GetValue()) : "";
        case ConfigValueType::Enum:
            if (m_choice && m_choice->GetSelection() != wxNOT_FOUND) {
                return m_choice->GetStringSelection().ToStdString();
            }
            return "";
        case ConfigValueType::Color:
            return m_originalValue;
        case ConfigValueType::Size:
            if (m_sizeSpinCtrl1 && m_sizeSpinCtrl2) {
                return std::to_string(m_sizeSpinCtrl1->GetValue()) + "," +
                       std::to_string(m_sizeSpinCtrl2->GetValue());
            }
            return "";
        default:
            return m_textCtrl ? m_textCtrl->GetValue().ToStdString() : "";
    }
}

bool ConfigItemEditor::isModified() const {
    return getValue() != m_originalValue;
}

void ConfigItemEditor::reset() {
    setValue(m_originalValue);
}

void ConfigItemEditor::onValueChanged() {
    std::string newValue = getValue();
    m_modified = (newValue != m_originalValue);
    if (m_onChange) {
        m_onChange(newValue);
    }
}

void ConfigItemEditor::onSizeValueChanged() {
    std::string newValue = getValue();
    m_modified = (newValue != m_originalValue);
    if (m_onChange) {
        m_onChange(newValue);
    }
}

void ConfigItemEditor::onColorButton(wxCommandEvent& event) {
    wxColour currentColor = stringToColor(m_originalValue);
    wxColourDialog dlg(this);
    dlg.GetColourData().SetColour(currentColor);

    if (dlg.ShowModal() == wxID_OK) {
        wxColour color = dlg.GetColourData().GetColour();
        std::string colorStr = colorToString(color);
        setValue(colorStr);
        onValueChanged();
    }
}

std::string ConfigItemEditor::colorToString(const wxColour& color) const {
    std::ostringstream oss;
    oss << (color.Red() / 255.0) << ","
        << (color.Green() / 255.0) << ","
        << (color.Blue() / 255.0);
    return oss.str();
}

wxColour ConfigItemEditor::stringToColor(const std::string& str) const {
    std::istringstream iss(str);
    std::string token;
    std::vector<double> values;

    while (std::getline(iss, token, ',')) {
        try {
            values.push_back(std::stod(token));
        } catch (...) {
            return wxColour(0, 0, 0);
        }
    }

    if (values.size() >= 3) {
        int r = (int)(values[0] * 255.0);
        int g = (int)(values[1] * 255.0);
        int b = (int)(values[2] * 255.0);
        r = std::max(0, std::min(255, r));
        g = std::max(0, std::min(255, g));
        b = std::max(0, std::min(255, b));
        return wxColour(r, g, b);
    }

    return wxColour(0, 0, 0);
}
