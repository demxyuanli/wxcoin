#include "config/editor/LayoutConfigEditor.h"
#include "config/UnifiedConfigManager.h"
#include "config/ConfigManagerDialog.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <algorithm>

LayoutConfigEditor::LayoutConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : ConfigCategoryEditor(parent, configManager, categoryId)
{
    createUI();
}

LayoutConfigEditor::~LayoutConfigEditor() {
}

void LayoutConfigEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
}

void LayoutConfigEditor::loadConfig() {
    wxSizer* sizer = GetSizer();
    sizer->Clear(true);
    m_editors.clear();
    m_originalValues.clear();
    m_sectionGroups.clear();
    m_componentGroups.clear();

    auto items = m_configManager->getItemsForCategory(m_categoryId);
    if (items.empty()) {
        wxStaticText* noItemsText = new wxStaticText(this, wxID_ANY, "No layout configuration items found");
        sizer->Add(noItemsText, 0, wxALL | wxALIGN_CENTER, 20);
        return;
    }

    organizeByComponent();
    groupItemsBySection();

    // Display by section, which already represents logical groupings
    std::vector<std::string> sections;
    for (const auto& pair : m_sectionGroups) {
        sections.push_back(pair.first);
    }
    std::sort(sections.begin(), sections.end());

    for (const auto& section : sections) {
        addSectionHeader(sizer, section);
        
        // Sort items within section for better organization
        std::vector<ConfigItem> sortedItems = m_sectionGroups[section];
        std::sort(sortedItems.begin(), sortedItems.end(), 
            [](const ConfigItem& a, const ConfigItem& b) {
                return a.displayName < b.displayName;
            });
        
        for (const auto& item : sortedItems) {
            createItemEditor(item);
            addItemEditor(sizer, m_editors[item.key]);
        }
    }

    sizer->Layout();
    FitInside();
}

void LayoutConfigEditor::organizeByComponent() {
    // Organize layout items by UI component for better visualization
    auto items = m_configManager->getItemsForCategory(m_categoryId);
    for (const auto& item : items) {
        std::string lowerSection = item.section;
        std::transform(lowerSection.begin(), lowerSection.end(), lowerSection.begin(), ::tolower);
        
        // Extract component name from section (e.g., "BarSizes" -> "Bar")
        if (lowerSection.find("bar") != std::string::npos) {
            m_componentGroups["Bar"].push_back(item);
        } else if (lowerSection.find("panel") != std::string::npos) {
            m_componentGroups["Panel"].push_back(item);
        } else if (lowerSection.find("dock") != std::string::npos) {
            m_componentGroups["Docking"].push_back(item);
        } else if (lowerSection.find("frame") != std::string::npos) {
            m_componentGroups["Frame"].push_back(item);
        } else if (lowerSection.find("gallery") != std::string::npos) {
            m_componentGroups["Gallery"].push_back(item);
        } else if (lowerSection.find("button") != std::string::npos) {
            m_componentGroups["Button"].push_back(item);
        } else if (lowerSection.find("scroll") != std::string::npos) {
            m_componentGroups["ScrollBar"].push_back(item);
        } else if (lowerSection.find("home") != std::string::npos) {
            m_componentGroups["Home"].push_back(item);
        } else if (lowerSection.find("separator") != std::string::npos || lowerSection.find("icon") != std::string::npos) {
            m_componentGroups["UI Elements"].push_back(item);
        } else {
            m_componentGroups["Other"].push_back(item);
        }
    }
}

void LayoutConfigEditor::groupItemsBySection() {
    auto items = m_configManager->getItemsForCategory(m_categoryId);

    for (const auto& item : items) {
        m_sectionGroups[item.section].push_back(item);
    }
}
