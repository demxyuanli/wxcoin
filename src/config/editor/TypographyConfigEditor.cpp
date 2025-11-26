#include "config/editor/TypographyConfigEditor.h"
#include "config/UnifiedConfigManager.h"
#include "config/ConfigManagerDialog.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <algorithm>

TypographyConfigEditor::TypographyConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : ConfigCategoryEditor(parent, configManager, categoryId)
{
    createUI();
}

TypographyConfigEditor::~TypographyConfigEditor() {
}

void TypographyConfigEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
}

void TypographyConfigEditor::loadConfig() {
    wxSizer* sizer = GetSizer();
    sizer->Clear(true);
    m_editors.clear();
    m_originalValues.clear();
    m_fontTypeGroups.clear();
    m_propertyGroups.clear();

    auto items = m_configManager->getItemsForCategory(m_categoryId);
    if (items.empty()) {
        wxStaticText* noItemsText = new wxStaticText(this, wxID_ANY, "No typography configuration items found");
        sizer->Add(noItemsText, 0, wxALL | wxALIGN_CENTER, 20);
        return;
    }

    organizeFontProperties();
    groupItemsByFontType();

    // Display fonts grouped by font type (Default, Title, Label, etc.)
    std::vector<std::string> fontTypes;
    for (const auto& pair : m_fontTypeGroups) {
        fontTypes.push_back(pair.first);
    }
    std::sort(fontTypes.begin(), fontTypes.end());

    for (const auto& fontType : fontTypes) {
        addSectionHeader(sizer, fontType + " Font");
        
        // Display properties in logical order: Size, Family, Style, Weight, FaceName
        std::vector<std::string> propertyOrder = {"Size", "Family", "Style", "Weight", "FaceName"};
        
        for (const auto& property : propertyOrder) {
            for (const auto& item : m_fontTypeGroups[fontType]) {
                std::string lowerKey = item.displayName;
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                
                if ((property == "Size" && lowerKey.find("fontsize") != std::string::npos) ||
                    (property == "Family" && lowerKey.find("fontfamily") != std::string::npos) ||
                    (property == "Style" && lowerKey.find("fontstyle") != std::string::npos) ||
                    (property == "Weight" && lowerKey.find("fontweight") != std::string::npos) ||
                    (property == "FaceName" && (lowerKey.find("fontfacename") != std::string::npos || lowerKey.find("fontname") != std::string::npos))) {
                    createItemEditor(item);
                    addItemEditor(sizer, m_editors[item.key]);
                }
            }
        }
    }

    sizer->Layout();
    FitInside();
}

void TypographyConfigEditor::organizeFontProperties() {
    // Extract property types for organization
    auto items = m_configManager->getItemsForCategory(m_categoryId);
    for (const auto& item : items) {
        std::string lowerKey = item.displayName;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        
        if (lowerKey.find("fontsize") != std::string::npos) {
            m_propertyGroups["Size"].push_back(item);
        } else if (lowerKey.find("fontfamily") != std::string::npos) {
            m_propertyGroups["Family"].push_back(item);
        } else if (lowerKey.find("fontstyle") != std::string::npos) {
            m_propertyGroups["Style"].push_back(item);
        } else if (lowerKey.find("fontweight") != std::string::npos) {
            m_propertyGroups["Weight"].push_back(item);
        } else if (lowerKey.find("fontfacename") != std::string::npos || lowerKey.find("fontname") != std::string::npos) {
            m_propertyGroups["FaceName"].push_back(item);
        }
    }
}

void TypographyConfigEditor::groupItemsByFontType() {
    auto items = m_configManager->getItemsForCategory(m_categoryId);

    for (const auto& item : items) {
        std::string displayName = item.displayName;
        
        // Extract font type from key name (e.g., "DefaultFontSize" -> "Default")
        size_t fontPos = displayName.find("Font");
        if (fontPos != std::string::npos) {
            std::string fontType = displayName.substr(0, fontPos);
            if (!fontType.empty()) {
                m_fontTypeGroups[fontType].push_back(item);
            } else {
                // If no prefix, use "Default"
                m_fontTypeGroups["Default"].push_back(item);
            }
        } else {
            // If no "Font" in name, use "Default"
            m_fontTypeGroups["Default"].push_back(item);
        }
    }
}
