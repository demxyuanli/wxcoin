#include "config/ConfigManagerDialog.h"
#include "config/UnifiedConfigManager.h"
#include "config/ConfigManager.h"
#include "config/editor/ConfigEditorFactory.h"
#include "config/editor/ConfigCategoryEditor.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/spinctrl.h>
#include <wx/colordlg.h>
#include <sstream>

wxBEGIN_EVENT_TABLE(ConfigManagerDialog, wxDialog)
    EVT_TREE_SEL_CHANGED(wxID_ANY, ConfigManagerDialog::onCategorySelected)
wxEND_EVENT_TABLE()

ConfigManagerDialog::ConfigManagerDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Configuration Manager", wxDefaultPosition, wxSize(1000, 700),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
    , m_categoryTree(nullptr)
    , m_contentPanel(nullptr)
    , m_scrolledPanel(nullptr)
    , m_splitter(nullptr)
    , m_applyButton(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
    , m_configManager(&UnifiedConfigManager::getInstance())
    , m_currentEditor(nullptr)
    , m_currentCategory("")
{
    // Print diagnostics for debugging
    m_configManager->printDiagnostics();

    createUI();
    populateCategoryTree();

    if (m_categoryTree->GetCount() > 0) {
        m_categoryTree->SelectItem(m_categoryTree->GetFirstVisibleItem());
    }
}

ConfigManagerDialog::~ConfigManagerDialog() {
    if (m_currentEditor) {
        delete m_currentEditor;
    }
}

void ConfigManagerDialog::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxSP_3D | wxSP_LIVE_UPDATE);
    m_splitter->SetMinimumPaneSize(200);

    m_categoryTree = new wxTreeCtrl(m_splitter, wxID_ANY, wxDefaultPosition, wxSize(200, -1),
                                      wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_SINGLE);

    m_scrolledPanel = new wxScrolledWindow(m_splitter, wxID_ANY);
    m_scrolledPanel->SetScrollRate(10, 10);

    m_contentPanel = new wxPanel(m_scrolledPanel);
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    m_contentPanel->SetSizer(contentSizer);

    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    scrolledSizer->Add(m_contentPanel, 1, wxEXPAND | wxALL, 5);
    m_scrolledPanel->SetSizer(scrolledSizer);

    m_splitter->SplitVertically(m_categoryTree, m_scrolledPanel, 250);

    mainSizer->Add(m_splitter, 1, wxEXPAND | wxALL, 5);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_resetButton = new wxButton(this, wxID_ANY, "Reset");
    m_applyButton = new wxButton(this, wxID_APPLY, "Apply");
    m_okButton = new wxButton(this, wxID_OK, "OK");
    m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

    buttonSizer->Add(m_resetButton, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_applyButton, 0, wxALL, 5);
    buttonSizer->Add(m_okButton, 0, wxALL, 5);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);

    SetSizer(mainSizer);

    m_resetButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onReset(); });
    m_applyButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onApply(event); });
    m_okButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onOK(event); });
    m_cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onCancel(event); });
}

void ConfigManagerDialog::populateCategoryTree() {
    m_categoryTree->DeleteAllItems();
    wxTreeItemId root = m_categoryTree->AddRoot("Categories");

    auto categories = m_configManager->getCategories();

    std::map<std::string, wxTreeItemId> categoryItems;

    for (const auto& category : categories) {
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
    if (m_currentEditor) {
        m_currentEditor->Destroy();
        m_currentEditor = nullptr;
    }

    wxSizer* sizer = m_contentPanel->GetSizer();
    sizer->Clear(true);

    if (m_currentCategory.empty()) {
        return;
    }

    m_currentEditor = ConfigEditorFactory::createEditor(m_contentPanel, m_configManager, m_currentCategory);
    if (m_currentEditor) {
        m_currentEditor->setChangeCallback([this]() {
            if (m_currentEditor && m_currentEditor->hasChanges()) {
                m_applyButton->Enable(true);
            }
        });
        m_currentEditor->loadConfig();
        sizer->Add(m_currentEditor, 1, wxEXPAND | wxALL, 5);
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
    if (m_currentEditor) {
        m_currentEditor->saveConfig();
        m_applyButton->Enable(false);
        wxMessageBox("Configuration applied successfully", "Success", wxOK | wxICON_INFORMATION);
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
    int result = wxMessageBox("Reset all changes to original values?", "Reset", wxYES_NO | wxICON_QUESTION);
    if (result == wxYES && m_currentEditor) {
        m_currentEditor->resetConfig();
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

