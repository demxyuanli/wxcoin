#include "config/ConfigManagerDialog.h"
#include "config/UnifiedConfigManager.h"
#include "config/ConfigManager.h"
#include "config/editor/ConfigEditorFactory.h"
#include "config/editor/ConfigCategoryEditor.h"
#include "widgets/FlatProgressBar.h"
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/spinctrl.h>
#include <wx/colordlg.h>
#include <wx/dialog.h>
#include <wx/statline.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

wxBEGIN_EVENT_TABLE(ConfigManagerDialog, FramelessModalPopup)
wxEND_EVENT_TABLE()

ConfigManagerDialog::ConfigManagerDialog(wxWindow* parent)
    : FramelessModalPopup(parent, "Configuration Manager", wxSize(1400, 800))
    , m_categoryScrollPanel(nullptr)
    , m_searchCtrl(nullptr)
    , m_scrolledPanel(nullptr)
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
    populateCategoryList();
    
    // Load all configurations with flat progress bar first (before showing main window)
    loadAllConfigurations();
    
    // Show the main window after loading is complete (centered by FramelessModalPopup)
    Show();
    wxSafeYield();  // Allow window to render

    // Select first category if available
    if (!m_categoryButtons.empty()) {
        onCategoryMenuSelected(m_categoryButtons[0]);
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
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Left navigation panel with modern style
    wxPanel* navPanel = new wxPanel(m_contentPanel, wxID_ANY);
    navPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
    wxBoxSizer* navSizer = new wxBoxSizer(wxVERTICAL);

    // Search box
    wxStaticText* searchLabel = new wxStaticText(navPanel, wxID_ANY, "Search settings");
    wxFont searchLabelFont = CFG_DEFAULTFONT();
    searchLabelFont.SetPointSize(searchLabelFont.GetPointSize() - 1);
    searchLabel->SetFont(searchLabelFont);
    searchLabel->SetForegroundColour(wxColour(128, 128, 128)); // Gray for normal text
    navSizer->Add(searchLabel, 0, wxLEFT | wxTOP | wxRIGHT, 12);

    m_searchCtrl = new wxTextCtrl(navPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 28),
                                   wxTE_PROCESS_ENTER);
    m_searchCtrl->SetHint("Ctrl+F");
    m_searchCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { onSearch(); });
    m_searchCtrl->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_ESCAPE) {
            m_searchCtrl->Clear();
            onSearch();
        }
        event.Skip();
    });
    navSizer->Add(m_searchCtrl, 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT | wxBOTTOM, 12);

    // Category menu list (custom menu-style with icons and groups)
    m_categoryScrollPanel = new wxScrolledWindow(navPanel, wxID_ANY);
    m_categoryScrollPanel->SetScrollRate(10, 10);
    m_categoryScrollPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
    m_categoryScrollPanel->SetMinSize(wxSize(240, -1));
    
    wxBoxSizer* categorySizer = new wxBoxSizer(wxVERTICAL);
    m_categoryScrollPanel->SetSizer(categorySizer);
    
    navSizer->Add(m_categoryScrollPanel, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    navSizer->AddSpacer(12);

    navPanel->SetSizer(navSizer);

    // Right content panel
    m_scrolledPanel = new wxScrolledWindow(m_contentPanel, wxID_ANY);
    m_scrolledPanel->SetScrollRate(10, 10);
    m_scrolledPanel->SetBackgroundColour(CFG_COLOUR("PrimaryBackgroundColour"));

    wxPanel* editorContainer = new wxPanel(m_scrolledPanel);
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    editorContainer->SetSizer(contentSizer);

    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    scrolledSizer->Add(editorContainer, 1, wxEXPAND | wxALL, 20);
    m_scrolledPanel->SetSizer(scrolledSizer);

    // Split layout
    mainSizer->Add(navPanel, 0, wxEXPAND | wxALL, 0);
    mainSizer->Add(m_scrolledPanel, 1, wxEXPAND | wxALL, 0);

    // Bottom button bar (minimal, only essential buttons)
    wxPanel* buttonPanel = new wxPanel(m_contentPanel, wxID_ANY);
    buttonPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_resetButton = new wxButton(buttonPanel, wxID_ANY, "Reset");
    m_applyButton = new wxButton(buttonPanel, wxID_APPLY, "Apply");
    m_okButton = new wxButton(buttonPanel, wxID_OK, "OK");
    m_cancelButton = new wxButton(buttonPanel, wxID_CANCEL, "Cancel");

    buttonSizer->Add(m_resetButton, 0, wxALL, 8);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_applyButton, 0, wxALL, 8);
    buttonSizer->Add(m_okButton, 0, wxALL, 8);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 8);

    buttonPanel->SetSizer(buttonSizer);

    // Main layout
    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(mainSizer, 1, wxEXPAND);
    outerSizer->Add(buttonPanel, 0, wxEXPAND);
    m_contentPanel->SetSizer(outerSizer);

    m_resetButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onReset(); });
    m_applyButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onApply(event); });
    m_okButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onOK(event); });
    m_cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { onCancel(event); });
    
    // Store editor container for later use
    m_editorContainer = editorContainer;
}

void ConfigManagerDialog::populateCategoryList() {
    // Clear existing buttons
    wxSizer* sizer = m_categoryScrollPanel->GetSizer();
    sizer->Clear(true);
    m_categoryButtonMap.clear();
    m_categoryButtons.clear();

    auto categories = m_configManager->getCategories();

    // Group categories by similarity (like catalog grouping)
    struct CategoryGroup {
        std::string groupName;
        std::vector<ConfigCategory> items;
    };

    std::vector<CategoryGroup> groups = {
        {"System", {}},
        {"Appearance", {}},
        {"Rendering", {}},
        {"User Interface", {}},
        {"Other", {}}
    };

    // Categorize items by similarity
    for (const auto& category : categories) {
        std::string id = category.id;
        if (id == "System" || id == "General") {
            groups[0].items.push_back(category);
        } else if (id == "Appearance") {
            groups[1].items.push_back(category);
        } else if (id == "Rendering" || id == "Lighting" || id == "Edge Settings" || id == "Render Preview") {
            groups[2].items.push_back(category);
        } else if (id == "UI Components" || id == "Layout" || id == "Dock Layout" || id == "Typography" || id == "Navigation") {
            groups[3].items.push_back(category);
        } else {
            groups[4].items.push_back(category);
        }
    }

    // Sort items within each group alphabetically
    for (auto& group : groups) {
        std::sort(group.items.begin(), group.items.end(), 
            [](const ConfigCategory& a, const ConfigCategory& b) {
                return a.displayName < b.displayName;
            });
    }

    // Create menu items for each group
    for (const auto& group : groups) {
        if (group.items.empty()) continue;

        // Add group header (always show for clarity)
        wxStaticText* groupHeader = new wxStaticText(m_categoryScrollPanel, wxID_ANY, group.groupName);
        wxFont headerFont = CFG_DEFAULTFONT();
        headerFont.SetPointSize(headerFont.GetPointSize() - 1);
        headerFont.SetWeight(wxFONTWEIGHT_BOLD);
        groupHeader->SetFont(headerFont);
        groupHeader->SetForegroundColour(wxColour(128, 128, 128)); // Gray for normal text
        sizer->Add(groupHeader, 0, wxLEFT | wxTOP | wxRIGHT, 12);
        sizer->AddSpacer(4);

        // Add menu items for each category in group
        for (const auto& category : group.items) {
            createCategoryMenuItem(category, sizer);
        }

        // Add separator after group (except last)
        if (&group != &groups.back() && !group.items.empty()) {
            sizer->AddSpacer(8);
            // Add subtle separator line
            wxStaticLine* separator = new wxStaticLine(m_categoryScrollPanel, wxID_ANY, 
                                                       wxDefaultPosition, wxSize(-1, 1));
            separator->SetBackgroundColour(CFG_COLOUR("BorderColour"));
            sizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
            sizer->AddSpacer(8);
        }
    }

    sizer->Layout();
    m_categoryScrollPanel->FitInside();
}

void ConfigManagerDialog::createCategoryMenuItem(const ConfigCategory& category, wxSizer* sizer) {
    // Create menu item panel
    wxPanel* itemPanel = new wxPanel(m_categoryScrollPanel, wxID_ANY);
    itemPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
    itemPanel->SetMinSize(wxSize(-1, 24)); // Menu item height: 24px
    
    wxBoxSizer* itemSizer = new wxBoxSizer(wxHORIZONTAL);
    itemSizer->AddSpacer(12); // Left padding: 12px

    // Load icon
    wxBitmap iconBitmap;
    bool iconLoaded = false;
    try {
        SvgIconManager& iconMgr = SvgIconManager::GetInstance();
        iconBitmap = iconMgr.GetIconBitmap(category.icon, wxSize(12, 12)); // Icon size: 12x12
        if (iconBitmap.IsOk()) {
            iconLoaded = true;
        } else {
            LOG_WRN(wxString::Format("ConfigManagerDialog: Failed to load SVG icon '%s' for category '%s'",
                category.icon.c_str(), category.displayName.c_str()), "ConfigManagerDialog");
        }
    } catch (const std::exception& e) {
        LOG_ERR(wxString::Format("ConfigManagerDialog: Exception loading SVG icon '%s' for category '%s': %s",
            category.icon.c_str(), category.displayName.c_str(), e.what()), "ConfigManagerDialog");
    } catch (...) {
        LOG_ERR(wxString::Format("ConfigManagerDialog: Unknown exception loading SVG icon '%s' for category '%s'",
            category.icon.c_str(), category.displayName.c_str()), "ConfigManagerDialog");
    }

    // Icon
    if (iconLoaded && iconBitmap.IsOk()) {
        wxStaticBitmap* iconStatic = new wxStaticBitmap(itemPanel, wxID_ANY, iconBitmap);
        itemSizer->Add(iconStatic, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    } else {
        // Placeholder for missing icon
        itemSizer->AddSpacer(20); // 12px icon + 8px spacing
    }

    // Label with theme font
    wxStaticText* label = new wxStaticText(itemPanel, wxID_ANY, category.displayName);
    label->SetFont(CFG_DEFAULTFONT()); // Use theme font
    label->SetForegroundColour(wxColour(128, 128, 128)); // Gray for normal text
    itemSizer->Add(label, 1, wxALIGN_CENTER_VERTICAL);
    itemSizer->AddSpacer(12); // Right padding: 12px

    itemPanel->SetSizer(itemSizer);

    // Store mapping
    m_categoryButtonMap[itemPanel] = category.id;
    m_categoryButtons.push_back(itemPanel);

    // Bind events for selection
    itemPanel->Bind(wxEVT_LEFT_DOWN, [this, itemPanel](wxMouseEvent&) {
        onCategoryMenuSelected(itemPanel);
    });
    label->Bind(wxEVT_LEFT_DOWN, [this, itemPanel](wxMouseEvent&) {
        onCategoryMenuSelected(itemPanel);
    });

    // Hover effect - only apply if not selected
    itemPanel->Bind(wxEVT_ENTER_WINDOW, [itemPanel](wxMouseEvent&) {
        wxColour currentBg = itemPanel->GetBackgroundColour();
        wxColour accentColor = CFG_COLOUR("AccentColour");
        // Only apply hover if not already selected (accent color)
        if (currentBg != accentColor) {
            itemPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour").ChangeLightness(105));
            itemPanel->Refresh();
        }
    });
    itemPanel->Bind(wxEVT_LEAVE_WINDOW, [itemPanel](wxMouseEvent&) {
        wxColour currentBg = itemPanel->GetBackgroundColour();
        wxColour accentColor = CFG_COLOUR("AccentColour");
        // Only restore if not selected
        if (currentBg != accentColor) {
            itemPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
            itemPanel->Refresh();
        }
    });

    sizer->Add(itemPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(2); // Spacing between items: 2px
}

void ConfigManagerDialog::onCategoryMenuSelected(wxWindow* itemPanel) {
    auto it = m_categoryButtonMap.find(itemPanel);
    if (it == m_categoryButtonMap.end()) return;

    // Update visual selection - highlight selected item
    wxColour accentColor = CFG_COLOUR("AccentColour");
    wxColour normalBg = CFG_COLOUR("SecondaryBackgroundColour");
    wxColour normalText = wxColour(128, 128, 128); // Gray for normal text
    wxColour selectedText = wxColour(0, 0, 0); // Black for emphasized text

    for (wxWindow* btn : m_categoryButtons) {
        if (btn == itemPanel) {
            // Selected item - accent background, black text
            btn->SetBackgroundColour(accentColor);
            wxWindowList& children = btn->GetChildren();
            for (wxWindow* child : children) {
                if (wxStaticText* text = dynamic_cast<wxStaticText*>(child)) {
                    text->SetForegroundColour(selectedText);
                }
            }
        } else {
            // Unselected items - normal background, gray text
            btn->SetBackgroundColour(normalBg);
            wxWindowList& children = btn->GetChildren();
            for (wxWindow* child : children) {
                if (wxStaticText* text = dynamic_cast<wxStaticText*>(child)) {
                    text->SetForegroundColour(normalText);
                }
            }
        }
        btn->Refresh();
    }

    std::string categoryId = it->second;
    m_currentCategory = categoryId;
    refreshItemEditors();
}

void ConfigManagerDialog::onSearch() {
    wxString searchText = m_searchCtrl->GetValue().Lower();
    
    // Filter category menu based on search
    if (searchText.IsEmpty()) {
        // Show all categories
        for (wxWindow* btn : m_categoryButtons) {
            btn->Show();
        }
    } else {
        // Filter categories
        for (wxWindow* btn : m_categoryButtons) {
            auto it = m_categoryButtonMap.find(btn);
            if (it == m_categoryButtonMap.end()) {
                btn->Show(false);
                continue;
            }

            std::string categoryId = it->second;
            auto categories = m_configManager->getCategories();
            ConfigCategory* category = nullptr;
            for (auto& cat : categories) {
                if (cat.id == categoryId) {
                    category = &cat;
                    break;
                }
            }

            bool matches = false;
            if (category) {
                // Check category name
                if (wxString(category->displayName).Lower().Contains(searchText)) {
                    matches = true;
                }
                
                // Check items in category
                if (!matches) {
                    auto items = m_configManager->getItemsForCategory(categoryId);
                    for (const auto& item : items) {
                        if (wxString(item.displayName).Lower().Contains(searchText) ||
                            wxString(item.description).Lower().Contains(searchText)) {
                            matches = true;
                            break;
                        }
                    }
                }
            }

            btn->Show(matches);
        }
    }

    m_categoryScrollPanel->GetSizer()->Layout();
    m_categoryScrollPanel->FitInside();
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
                // Check all cached editors for changes, not just the current one
                bool hasAnyChanges = false;
                for (auto& pair : m_editorCache) {
                    if (pair.second && pair.second->hasChanges()) {
                        hasAnyChanges = true;
                        break;
                    }
                }
                if (hasAnyChanges) {
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


void ConfigManagerDialog::refreshItemEditors() {
    wxSizer* sizer = m_editorContainer->GetSizer();

    // Hide and detach current editor if exists (but don't delete it - it's cached)
    if (m_currentEditor) {
        m_currentEditor->Hide();
        sizer->Detach(m_currentEditor);
    }

    // Remove title and spacer if exists
    // Find and remove them directly after detaching m_currentEditor
    wxSizerItemList& items = sizer->GetChildren();
    wxStaticText* titleToRemove = nullptr;
    wxSizerItem* spacerToRemove = nullptr;

    // Find title window and spacer in current sizer state
    for (size_t i = 0; i < items.size(); ++i) {
        wxSizerItem* item = items[i];
        if (!item) continue;

        // Check if this is a spacer after title
        if (!spacerToRemove && item->IsSpacer() && item->GetMinSize().y >= 20) {
            spacerToRemove = item;
        }

        // Check if this is the title window
        if (!titleToRemove) {
            wxWindow* win = item->GetWindow();
            if (win) {
                if (wxStaticText* text = dynamic_cast<wxStaticText*>(win)) {
                    wxFont font = text->GetFont();
                    if (font.GetWeight() == wxFONTWEIGHT_BOLD &&
                        font.GetPointSize() >= GetFont().GetPointSize() + 2) {
                        titleToRemove = text;
                    }
                }
            }
        }
    }

    // Remove spacer first (before removing title window)
    if (spacerToRemove) {
        // Find the index of the spacer in current sizer
        int spacerIndex = -1;
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i] == spacerToRemove) {
                spacerIndex = static_cast<int>(i);
                break;
            }
        }

        if (spacerIndex >= 0) {
            // Detach by index - this removes the item from sizer but doesn't delete it
            if (sizer->Detach(spacerIndex)) {
                // Now safely delete the detached item
                spacerToRemove ->DetachSizer();
            }
        }
    }
    
    // Remove title window
    if (titleToRemove) {
        sizer->Detach(titleToRemove);
        titleToRemove->Destroy();
    }

    if (m_currentCategory.empty()) {
        sizer->Layout();
        m_scrolledPanel->FitInside();
        return;
    }

    // Add category title
    auto categories = m_configManager->getCategories();
    std::string categoryDisplayName = m_currentCategory;
    for (const auto& cat : categories) {
        if (cat.id == m_currentCategory) {
            categoryDisplayName = cat.displayName;
            break;
        }
    }
    
    wxStaticText* titleText = new wxStaticText(m_editorContainer, wxID_ANY, categoryDisplayName);
    wxFont titleFont = CFG_DEFAULTFONT();
    titleFont.SetPointSize(titleFont.GetPointSize() + 4);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleText->SetFont(titleFont);
    titleText->SetForegroundColour(CFG_COLOUR("PrimaryTextColour"));
    sizer->Add(titleText, 0, wxLEFT | wxRIGHT | wxTOP, 0);
    sizer->AddSpacer(24); // Spacing after title

    // All editors should be pre-loaded, just get from cache
    auto cacheIt = m_editorCache.find(m_currentCategory);
    if (cacheIt != m_editorCache.end()) {
        m_currentEditor = cacheIt->second;
        if (m_currentEditor) {
            // Refresh values from config manager (in case config was changed externally)
            m_currentEditor->refreshValues();
            m_currentEditor->Show();
            sizer->Add(m_currentEditor, 1, wxEXPAND | wxLEFT | wxRIGHT, 0);
        }
    } else {
        // Fallback: if editor not in cache (shouldn't happen if loadAllConfigurations worked)
        LOG_WRN("Editor for category '" + m_currentCategory + "' not found in cache", "ConfigManagerDialog");
        m_currentEditor = ConfigEditorFactory::createEditor(m_editorContainer, m_configManager, m_currentCategory);
        if (m_currentEditor) {
            m_currentEditor->setChangeCallback([this]() {
                // Check all cached editors for changes, not just the current one
                bool hasAnyChanges = false;
                for (auto& pair : m_editorCache) {
                    if (pair.second && pair.second->hasChanges()) {
                        hasAnyChanges = true;
                        break;
                    }
                }
                if (hasAnyChanges) {
                    m_applyButton->Enable(true);
                }
            });
            m_currentEditor->loadConfig();
            m_editorCache[m_currentCategory] = m_currentEditor;
            sizer->Add(m_currentEditor, 1, wxEXPAND | wxLEFT | wxRIGHT, 0);
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
    , m_originalBgColor()
{
    createUI();
    setValue(item.currentValue);

    // Add hover effect for better user feedback
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) {
        wxColour hoverBg = CFG_COLOUR("SecondaryBackgroundColour");
        hoverBg = hoverBg.ChangeLightness(105); // Slightly lighter
        SetBackgroundColour(hoverBg);
        Refresh();
    });

    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
        SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
        Refresh();
    });
}

ConfigItemEditor::~ConfigItemEditor() {
}

void ConfigItemEditor::createUI() {
    // Modern card-style layout with theme colors
    wxColour bgColor = CFG_COLOUR("SecondaryBackgroundColour");
    wxColour textColor = CFG_COLOUR("PrimaryTextColour");
    wxColour descColor = CFG_COLOUR("SecondaryTextColour");
    wxColour borderColor = CFG_COLOUR("BorderColour");

    SetBackgroundColour(bgColor);
    SetMinSize(wxSize(-1, 60)); // Minimum height for card

    // Create main horizontal sizer - card layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->AddSpacer(16); // Left padding

    // Left section: Title and description
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    leftSizer->AddSpacer(8); // Top padding

    // Main label - bold, larger, black color
    m_label = new wxStaticText(this, wxID_ANY, m_item.displayName);
    wxFont labelFont = CFG_DEFAULTFONT();
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    labelFont.SetPointSize(labelFont.GetPointSize() + 1);
    m_label->SetFont(labelFont);
    m_label->SetForegroundColour(wxColour(0, 0, 0)); // Black color
    leftSizer->Add(m_label, 0, wxLEFT | wxRIGHT, 0);

    // Description - smaller font, gray color, with spacing
    if (!m_item.description.empty()) {
        leftSizer->AddSpacer(4);
        m_description = new wxStaticText(this, wxID_ANY, m_item.description);
        wxFont descFont = CFG_DEFAULTFONT();
        descFont.SetPointSize(descFont.GetPointSize() - 1);
        m_description->SetFont(descFont);
        m_description->SetForegroundColour(wxColour(128, 128, 128)); // Gray color
        m_description->Wrap(400); // Allow wrapping for long descriptions
        leftSizer->Add(m_description, 0, wxLEFT | wxRIGHT | wxBOTTOM, 0);
    }

    leftSizer->AddSpacer(8); // Bottom padding
    mainSizer->Add(leftSizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 0);

    // Spacer to push controls to the right
    mainSizer->AddStretchSpacer();

    // Right section: Control widget
    wxBoxSizer* valueSizer = new wxBoxSizer(wxHORIZONTAL);

    switch (m_item.type) {
        case ConfigValueType::Bool: {
            // For boolean values, use checkbox styled as toggle switch
            m_checkBox = new wxCheckBox(this, wxID_ANY, "");
            m_checkBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { onValueChanged(); });
            // Style checkbox to look more like a toggle switch
            wxFont checkFont = m_checkBox->GetFont();
            checkFont.SetPointSize(checkFont.GetPointSize() + 2);
            m_checkBox->SetFont(checkFont);
            valueSizer->Add(m_checkBox, 0, wxALIGN_CENTER_VERTICAL);
            break;
        }
        case ConfigValueType::Int: {
            m_spinCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(120, -1),
                                       wxSP_ARROW_KEYS, (int)m_item.minValue, (int)m_item.maxValue);
            m_spinCtrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onValueChanged(); });
            valueSizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_VERTICAL);
            break;
        }
        case ConfigValueType::Double: {
            m_spinCtrlDouble = new wxSpinCtrlDouble(this, wxID_ANY, "", wxDefaultPosition, wxSize(120, -1),
                                                   wxSP_ARROW_KEYS, m_item.minValue, m_item.maxValue, 0.0, 0.1);
            m_spinCtrlDouble->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { onValueChanged(); });
            valueSizer->Add(m_spinCtrlDouble, 0, wxALIGN_CENTER_VERTICAL);
            break;
        }
        case ConfigValueType::Enum: {
            m_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(150, -1));
            for (const auto& val : m_item.enumValues) {
                m_choice->Append(val);
            }
            m_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onValueChanged(); });
            valueSizer->Add(m_choice, 0, wxALIGN_CENTER_VERTICAL);
            break;
        }
        case ConfigValueType::Color: {
            // Compact color picker: preview and button in a tight horizontal layout
            m_colorPreview = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(24, 20));
            // Set initial color to gray to indicate it's not set yet
            m_colorPreview->SetBackgroundColour(wxColour(160, 160, 160));

            // Add custom paint handler for color preview with border and checkerboard pattern
            m_colorPreview->Bind(wxEVT_PAINT, [this](wxPaintEvent& event) {
                wxPaintDC dc(m_colorPreview);
                wxRect rect = m_colorPreview->GetClientRect();

                // Fill with current background color first
                dc.SetPen(*wxTRANSPARENT_PEN);
                dc.SetBrush(wxBrush(m_colorPreview->GetBackgroundColour()));
                dc.DrawRectangle(rect);

                // Draw border
                dc.SetPen(wxPen(wxColour(100, 100, 100), 1));
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                dc.DrawRectangle(rect);
            });

            valueSizer->Add(m_colorPreview, 0, wxRIGHT, 3);

            m_colorButton = new wxButton(this, wxID_ANY, "...", wxDefaultPosition, wxSize(35, 22));
            wxFont btnFont = m_colorButton->GetFont();
            btnFont.SetPointSize(btnFont.GetPointSize() - 1);
            m_colorButton->SetFont(btnFont);
            m_colorButton->Bind(wxEVT_BUTTON, &ConfigItemEditor::onColorButton, this);

            // Check if this is a multi-theme color (contains semicolons)
            bool isMultiTheme = m_item.currentValue.find(';') != std::string::npos;
            if (isMultiTheme) {
                // For multi-theme colors, show current theme indicator
                std::string currentTheme = "default";
                try {
                    currentTheme = ThemeManager::getInstance().getCurrentTheme();
                } catch (...) {}
                wxString themeLabel = wxString::Format(" (%s)", currentTheme);
                m_colorButton->SetLabel("...");
                m_colorButton->SetToolTip("Click to choose color" + themeLabel +
                                        "\nThis color supports multiple themes");
            } else {
                m_colorButton->SetToolTip("Click to choose color");
            }

            valueSizer->Add(m_colorButton, 0);
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

                m_sizeSpinCtrl1 = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(50, -1),
                                                wxSP_ARROW_KEYS, 0, 10000, width);
                m_sizeSpinCtrl2 = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(50, -1),
                                                wxSP_ARROW_KEYS, 0, 10000, height);
                m_sizeSeparator = new wxStaticText(this, wxID_ANY, "x");

                m_sizeSpinCtrl1->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onSizeValueChanged(); });
                m_sizeSpinCtrl2->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onSizeValueChanged(); });

                wxFont sepFont = CFG_DEFAULTFONT();
                sepFont.SetPointSize(sepFont.GetPointSize() - 1);
                m_sizeSeparator->SetFont(sepFont);
                m_sizeSeparator->SetForegroundColour(descColor);

                valueSizer->Add(m_sizeSpinCtrl1, 0, wxRIGHT, 1);
                valueSizer->Add(m_sizeSeparator, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 2);
                valueSizer->Add(m_sizeSpinCtrl2, 0, wxLEFT, 1);
            } else {
                // Fallback to single spin control
                m_spinCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(120, -1),
                                           wxSP_ARROW_KEYS, 0, 10000);
                m_spinCtrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onValueChanged(); });
                valueSizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_VERTICAL);
            }
            break;
        }
        default: {
            m_textCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1));
            m_textCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { onValueChanged(); });
            valueSizer->Add(m_textCtrl, 0, wxALIGN_CENTER_VERTICAL);
            break;
        }
    }

    valueSizer->AddSpacer(8);
    mainSizer->Add(valueSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
    mainSizer->AddSpacer(16); // Right padding

    SetSizer(mainSizer);

    // Add subtle border for card effect
    SetWindowStyle(GetWindowStyle() | wxBORDER_NONE);
    
    // Custom paint for card border
    Bind(wxEVT_PAINT, [this, borderColor](wxPaintEvent& event) {
        wxPaintDC dc(this);
        wxRect rect = GetClientRect();
        dc.SetPen(wxPen(borderColor, 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
    });

    // Store original background color for change indication
    m_originalBgColor = bgColor;
}

void ConfigItemEditor::setValue(const std::string& value) {
    // Don't modify m_originalValue here - it should only be set in constructor and reset()
    // This method only updates the UI controls
    m_modified = false;

    switch (m_item.type) {
        case ConfigValueType::Bool:
            if (m_checkBox) {
                m_checkBox->SetValue(value == "true");
            }
            break;
        case ConfigValueType::Int:
            if (m_spinCtrl) {
                try {
                    m_spinCtrl->SetValue(std::stoi(value));
                } catch (...) {
                    // Invalid integer, set to 0
                    m_spinCtrl->SetValue(0);
                }
            }
            break;
        case ConfigValueType::Double:
            if (m_spinCtrlDouble) {
                try {
                    m_spinCtrlDouble->SetValue(std::stod(value));
                } catch (...) {
                    // Invalid double, set to 0.0
                    m_spinCtrlDouble->SetValue(0.0);
                }
            }
            break;
        case ConfigValueType::Enum:
            if (m_choice) {
                int index = m_choice->FindString(value);
                if (index != wxNOT_FOUND) {
                    m_choice->SetSelection(index);
                } else {
                    // If value not found in choices, select first item
                    if (m_choice->GetCount() > 0) {
                        m_choice->SetSelection(0);
                    }
                }
            }
            break;
        case ConfigValueType::Color: {
            wxColour color = stringToColor(value);
            if (m_colorPreview) {
                m_colorPreview->SetBackgroundColour(color);
                m_colorPreview->Refresh();
                // Force repaint to ensure color shows immediately
                m_colorPreview->Update();
            } else {
                // Color preview not created yet, this shouldn't happen
                // since createUI is called before setValue
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
            // Get current color from preview panel, not from m_originalValue
            if (m_colorPreview) {
                wxColour currentColor = m_colorPreview->GetBackgroundColour();
                return colorToString(currentColor);
            }
            return m_originalValue; // Fallback to original if preview not available
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
    // For color types, compare actual color values instead of strings to avoid precision issues
    if (m_item.type == ConfigValueType::Color) {
        wxColour currentColor = stringToColor(getValue());
        wxColour originalColor = stringToColor(m_originalValue);
        // Compare RGB values directly to avoid string precision issues
        return (currentColor.Red() != originalColor.Red() ||
                currentColor.Green() != originalColor.Green() ||
                currentColor.Blue() != originalColor.Blue());
    }
    // For other types, use string comparison
    return getValue() != m_originalValue;
}

void ConfigItemEditor::reset() {
    setValue(m_originalValue);
}

void ConfigItemEditor::onValueChanged() {
    std::string newValue = getValue();
    bool wasModified = m_modified;
    m_modified = (newValue != m_originalValue);

    // Update visual indication if modification state changed
    if (m_modified != wasModified) {
        updateVisualIndication();
    }

    if (m_onChange) {
        m_onChange(newValue);
    }
}

void ConfigItemEditor::updateVisualIndication() {
    if (m_modified) {
        // Show modified state with a slightly different background
        wxColour modifiedBg = m_originalBgColor;
        modifiedBg = modifiedBg.ChangeLightness(110); // Slightly lighter for modified items
        SetBackgroundColour(modifiedBg);
    } else {
        // Restore original background
        SetBackgroundColour(m_originalBgColor);
    }
    Refresh();
}

void ConfigItemEditor::onSizeValueChanged() {
    std::string newValue = getValue();
    m_modified = (newValue != m_originalValue);
    if (m_onChange) {
        m_onChange(newValue);
    }
}

void ConfigItemEditor::onColorButton(wxCommandEvent& event) {
    // Get current color from preview panel (not original value) to show user's current selection
    std::string currentValue = getValue();
    wxColour currentColor = stringToColor(currentValue);
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
    // Use fixed precision with 6 decimal places to avoid rounding errors
    // This ensures that color values can be converted back and forth without precision loss
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << (color.Red() / 255.0) << ","
        << (color.Green() / 255.0) << ","
        << (color.Blue() / 255.0);
    return oss.str();
}

wxColour ConfigItemEditor::stringToColor(const std::string& str) const {
    if (str.empty()) {
        return wxColour(128, 128, 128); // Default gray for empty values
    }

    // Handle multi-theme format (value1;value2;value3) - select based on current theme
    std::string colorStr = getCurrentThemeColor(str);

    std::istringstream iss(colorStr);
    std::string token;
    std::vector<double> values;

    while (std::getline(iss, token, ',')) {
        // Trim whitespace
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), token.end());

        try {
            values.push_back(std::stod(token));
        } catch (...) {
            // If parsing fails, try to parse as 0-255 integer values
            try {
                int intVal = std::stoi(token);
                values.push_back(intVal / 255.0); // Convert to 0.0-1.0 range
            } catch (...) {
                return wxColour(255, 0, 0); // Red for invalid values
            }
        }
    }

    if (values.size() >= 3) {
        // Check if values are in 0-1 range or 0-255 range
        bool isNormalized = true;
        for (double val : values) {
            if (val > 1.0) {
                isNormalized = false;
                break;
            }
        }

        int r, g, b;
        if (isNormalized) {
            // Values are in 0.0-1.0 range
            r = (int)(values[0] * 255.0);
            g = (int)(values[1] * 255.0);
            b = (int)(values[2] * 255.0);
        } else {
            // Values are in 0-255 range
            r = (int)values[0];
            g = (int)values[1];
            b = (int)values[2];
        }

        r = std::max(0, std::min(255, r));
        g = std::max(0, std::min(255, g));
        b = std::max(0, std::min(255, b));
        return wxColour(r, g, b);
    }

    // Fallback for invalid format
    return wxColour(255, 192, 203); // Pink for unrecognized format
}

std::string ConfigItemEditor::getCurrentThemeColor(const std::string& multiThemeValue) const {
    // Check if this is a multi-theme value (contains semicolons)
    size_t firstSemicolon = multiThemeValue.find(';');
    if (firstSemicolon == std::string::npos) {
        // Not a multi-theme value, return as-is
        return multiThemeValue;
    }

    // This is a multi-theme value: theme1;theme2;theme3
    // Get current theme from ThemeManager
    std::string currentTheme = "default"; // fallback
    try {
        currentTheme = ThemeManager::getInstance().getCurrentTheme();
    } catch (...) {
        // If ThemeManager not available, use default
        currentTheme = "default";
    }

    // Parse the multi-theme value
    std::vector<std::string> themeColors;
    std::string remaining = multiThemeValue;
    size_t pos = 0;
    while ((pos = remaining.find(';')) != std::string::npos) {
        themeColors.push_back(remaining.substr(0, pos));
        remaining = remaining.substr(pos + 1);
    }
    themeColors.push_back(remaining); // Last part

    // Map theme names to indices
    std::map<std::string, int> themeIndex = {
        {"default", 0},
        {"dark", 1},
        {"blue", 2}
    };

    // Get the color for current theme, fallback to first if not found
    int index = themeIndex.count(currentTheme) ? themeIndex[currentTheme] : 0;
    if (index < (int)themeColors.size()) {
        return themeColors[index];
    }

    // Fallback to first theme color
    return themeColors.empty() ? multiThemeValue : themeColors[0];
}
