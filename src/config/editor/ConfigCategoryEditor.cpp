#include "config/editor/ConfigCategoryEditor.h"
#include "config/ConfigManagerDialog.h"
#include "config/UnifiedConfigManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/panel.h>

ConfigCategoryEditor::ConfigCategoryEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL)
    , m_configManager(configManager)
    , m_categoryId(categoryId)
    , m_changeCallback(nullptr)
{
    SetBackgroundColour(CFG_COLOUR("PrimaryBackgroundColour"));
    SetScrollRate(10, 10);

    // Create main sizer for this scrolled window
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
}

ConfigCategoryEditor::~ConfigCategoryEditor() {
    for (auto& pair : m_editors) {
        delete pair.second;
    }
}

void ConfigCategoryEditor::createItemEditor(const ConfigItem& item) {
    ConfigItemEditor* editor = new ConfigItemEditor(this, item,
        [this, key = item.key](const std::string& value) {
            onItemChanged(key, value);
        });

    m_editors[item.key] = editor;

    if (m_originalValues.find(item.key) == m_originalValues.end()) {
        m_originalValues[item.key] = item.currentValue;
    }
}

void ConfigCategoryEditor::onItemChanged(const std::string& key, const std::string& value) {
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void ConfigCategoryEditor::addSectionHeader(wxSizer* sizer, const std::string& sectionName) {
    // Modern section header with spacing
    wxStaticText* headerLabel = new wxStaticText(this, wxID_ANY, sectionName);
    wxFont headerFont = headerLabel->GetFont();
    headerFont.SetWeight(wxFONTWEIGHT_BOLD);
    headerFont.SetPointSize(headerFont.GetPointSize() + 2);
    headerLabel->SetFont(headerFont);
    headerLabel->SetForegroundColour(CFG_COLOUR("PrimaryTextColour"));
    
    sizer->AddSpacer(24); // Top spacing before section
    sizer->Add(headerLabel, 0, wxLEFT | wxRIGHT, 0);
    sizer->AddSpacer(12); // Spacing after header
}

void ConfigCategoryEditor::addItemEditor(wxSizer* sizer, ConfigItemEditor* editor) {
    // Card-style spacing between items
    sizer->Add(editor, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
    sizer->AddSpacer(8); // Spacing between cards
}

void ConfigCategoryEditor::saveConfig() {
    for (const auto& pair : m_editors) {
        std::string value = pair.second->getValue();
        m_configManager->setValue(pair.first, value);
    }
    m_configManager->save();
}

void ConfigCategoryEditor::resetConfig() {
    for (auto& pair : m_editors) {
        auto it = m_originalValues.find(pair.first);
        if (it != m_originalValues.end()) {
            pair.second->setValue(it->second);
        }
    }
}

bool ConfigCategoryEditor::hasChanges() const {
    for (const auto& pair : m_editors) {
        if (pair.second->isModified()) {
            return true;
        }
    }
    return false;
}

void ConfigCategoryEditor::setChangeCallback(std::function<void()> callback) {
    m_changeCallback = callback;
}

void ConfigCategoryEditor::refreshValues() {
    // Refresh values from config manager without recreating UI
    auto items = m_configManager->getItemsForCategory(m_categoryId);
    
    for (const auto& item : items) {
        auto editorIt = m_editors.find(item.key);
        if (editorIt != m_editors.end()) {
            // Update editor value from config
            editorIt->second->setValue(item.currentValue);
            
            // Update original value if needed
            if (m_originalValues.find(item.key) == m_originalValues.end()) {
                m_originalValues[item.key] = item.currentValue;
            }
        }
    }
}