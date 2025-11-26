#include "config/editor/RenderingConfigEditor.h"
#include "config/UnifiedConfigManager.h"
#include "config/ConfigManagerDialog.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>

RenderingConfigEditor::RenderingConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : ConfigCategoryEditor(parent, configManager, categoryId)
{
    createUI();
}

RenderingConfigEditor::~RenderingConfigEditor() {
}

void RenderingConfigEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
}

void RenderingConfigEditor::loadConfig() {
    wxSizer* sizer = GetSizer();
    sizer->Clear(true);
    m_editors.clear();
    m_originalValues.clear();
    m_sectionGroups.clear();

    auto items = m_configManager->getItemsForCategory(m_categoryId);
    if (items.empty()) {
        wxStaticText* noItemsText = new wxStaticText(this, wxID_ANY, "No configuration items found in this category");
        sizer->Add(noItemsText, 0, wxALL | wxALIGN_CENTER, 20);
        return;
    }

    groupItemsBySection();

    for (const auto& sectionPair : m_sectionGroups) {
        const std::string& sectionName = sectionPair.first;
        const std::vector<ConfigItem>& sectionItems = sectionPair.second;

        addSectionHeader(sizer, sectionName);

        for (const auto& item : sectionItems) {
            createItemEditor(item);
            addItemEditor(sizer, m_editors[item.key]);
        }
    }

    sizer->Layout();
    FitInside();
}

void RenderingConfigEditor::groupItemsBySection() {
    auto items = m_configManager->getItemsForCategory(m_categoryId);

    for (const auto& item : items) {
        m_sectionGroups[item.section].push_back(item);
    }
}