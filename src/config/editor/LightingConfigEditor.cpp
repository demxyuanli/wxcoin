#include "config/editor/LightingConfigEditor.h"
#include "config/UnifiedConfigManager.h"
#include "config/ConfigManagerDialog.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>

LightingConfigEditor::LightingConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : ConfigCategoryEditor(parent, configManager, categoryId)
{
    createUI();
}

LightingConfigEditor::~LightingConfigEditor() {
}

void LightingConfigEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
}

void LightingConfigEditor::loadConfig() {
    wxSizer* sizer = GetSizer();
    sizer->Clear(true);
    m_editors.clear();
    m_originalValues.clear();
    m_sectionGroups.clear();
    m_lightSections.clear();

    auto items = m_configManager->getItemsForCategory(m_categoryId);
    if (items.empty()) {
        wxStaticText* noItemsText = new wxStaticText(this, wxID_ANY, "No lighting configuration items found");
        sizer->Add(noItemsText, 0, wxALL | wxALIGN_CENTER, 20);
        return;
    }

    organizeLightSettings();
    groupItemsBySection();

    // First show Environment settings
    if (m_sectionGroups.find("Environment") != m_sectionGroups.end()) {
        addSectionHeader(sizer, "Environment Settings");
        for (const auto& item : m_sectionGroups["Environment"]) {
            createItemEditor(item);
            addItemEditor(sizer, m_editors[item.key]);
        }
    }

    // Then show individual light settings in order
    for (const auto& lightSection : m_lightSections) {
        if (m_sectionGroups.find(lightSection) != m_sectionGroups.end()) {
            addSectionHeader(sizer, lightSection);
            for (const auto& item : m_sectionGroups[lightSection]) {
                createItemEditor(item);
                addItemEditor(sizer, m_editors[item.key]);
            }
        }
    }

    // Show any other sections
    for (const auto& sectionPair : m_sectionGroups) {
        if (sectionPair.first != "Environment" && 
            std::find(m_lightSections.begin(), m_lightSections.end(), sectionPair.first) == m_lightSections.end()) {
            addSectionHeader(sizer, sectionPair.first);
            for (const auto& item : sectionPair.second) {
                createItemEditor(item);
                addItemEditor(sizer, m_editors[item.key]);
            }
        }
    }

    sizer->Layout();
    FitInside();
}

void LightingConfigEditor::organizeLightSettings() {
    // Organize light sections in order (Light0, Light1, Light2, etc.)
    std::vector<std::string> tempLightSections;
    for (const auto& item : m_configManager->getItemsForCategory(m_categoryId)) {
        if (item.section.find("Light") == 0 && item.section != "Lighting") {
            if (std::find(tempLightSections.begin(), tempLightSections.end(), item.section) == tempLightSections.end()) {
                tempLightSections.push_back(item.section);
            }
        }
    }
    
    // Sort light sections (Light0, Light1, Light2, ...)
    std::sort(tempLightSections.begin(), tempLightSections.end());
    m_lightSections = tempLightSections;
}

void LightingConfigEditor::groupItemsBySection() {
    auto items = m_configManager->getItemsForCategory(m_categoryId);

    for (const auto& item : items) {
        m_sectionGroups[item.section].push_back(item);
    }
}
