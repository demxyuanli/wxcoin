#include "flatui/FlatUIHomeMenu.h"
#include "flatui/FlatUIFrame.h" // For m_parentFrame and wxEVT_THEME_CHANGED event type
#include "flatui/FlatUIHomeSpace.h" // Needed for dynamic_cast and OnHomeMenuClosed call
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/dcbuffer.h> // For wxAutoBufferedPaintDC
#include <wx/menu.h> // For wxMenu
#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h" // For LOG_DBG and LOG_INF macros

wxBEGIN_EVENT_TABLE(FlatUIHomeMenu, wxPopupTransientWindow)
EVT_MOTION(FlatUIHomeMenu::OnMouseMotion)
EVT_KILL_FOCUS(FlatUIHomeMenu::OnKillFocus)
wxEND_EVENT_TABLE()

FlatUIHomeMenu::FlatUIHomeMenu(wxWindow* parent, FlatUIFrame* eventSinkFrame)
	: wxPopupTransientWindow(parent, wxBORDER_NONE),
	m_eventSinkFrame(eventSinkFrame)
{
	// Enable paint background style for auto-buffered drawing
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));

	m_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	m_panel->SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_panel->SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));

	m_itemSizer = new wxBoxSizer(wxVERTICAL);
	m_panel->SetSizer(m_itemSizer);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(m_panel, 1, wxEXPAND);
	SetSizer(mainSizer);

	m_panel->Bind(wxEVT_PAINT, &FlatUIHomeMenu::OnPaint, this);

	// Register theme change listener
	auto& themeManager = ThemeManager::getInstance();
	themeManager.addThemeChangeListener(this, [this]() {
		RefreshTheme();
		});
}

FlatUIHomeMenu::~FlatUIHomeMenu()
{
}

void FlatUIHomeMenu::AddMenuItem(const wxString& text, int id, const wxBitmap& icon)
{
	m_menuItems.push_back(FlatHomeMenuItemInfo(text, id, icon));
	// We could build layout incrementally, or all at once before showing.
	// For now, let's assume BuildMenuLayout() is called before Show().
}

void FlatUIHomeMenu::AddSeparator()
{
	m_menuItems.push_back(FlatHomeMenuItemInfo(wxEmptyString, wxID_SEPARATOR, wxNullBitmap, true));
}

void FlatUIHomeMenu::BuildMenuLayout()
{
	m_itemSizer->Clear(true); // Clear previous items if any

	bool hasDynamicItems = !m_menuItems.empty();

	for (const auto& itemInfo : m_menuItems) {
		if (itemInfo.isSeparator) {
			wxPanel* separator = new wxPanel(m_panel, wxID_ANY, wxDefaultPosition, wxSize(CFG_INT("HomeMenuWidth") - 10, 1));
			separator->SetName("HomeMenuSeparator");
			separator->SetBackgroundColour(CFG_COLOUR("BarTabBorderColour"));
			m_itemSizer->Add(separator, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, (CFG_INT("HomeMenuSeparatorHeight") - 1) / 2);
		}
		else {
			wxPanel* itemPanel = new wxPanel(m_panel, wxID_ANY);
			itemPanel->SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));
			wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);

			if (itemInfo.icon.IsOk()) {
				wxStaticBitmap* sb = new wxStaticBitmap(itemPanel, wxID_ANY, itemInfo.icon);
				hsizer->Add(sb, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
			}
			wxStaticText* st = new wxStaticText(itemPanel, itemInfo.id, itemInfo.text);
			st->SetFont(CFG_DEFAULTFONT());
			st->SetForegroundColour(CFG_COLOUR("MenuTextColour"));
			hsizer->Add(st, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
			itemPanel->SetSizer(hsizer);

			m_itemSizer->Add(itemPanel, 0, wxEXPAND | wxALL, 2);
			itemPanel->SetMinSize(wxSize(CFG_INT("HomeMenuWidth"), CFG_INT("HomeMenuItemHeight")));

			itemPanel->Bind(wxEVT_LEFT_DOWN, [this, item_id = itemInfo.id](wxMouseEvent& event) {
				if (item_id == wxID_EXIT) {
					SendItemCommand(item_id);
					event.Skip(); // Still skip to allow any other low-level handlers
				}
				SendItemCommand(item_id);
				Close();
				event.Skip();
				});
			st->Bind(wxEVT_LEFT_DOWN, [this, item_id = itemInfo.id](wxMouseEvent& event) {
				if (item_id == wxID_EXIT) {
					event.SetId(item_id); // SetId might still be useful if other handlers check it
					event.Skip();
					return; // Do nothing further for EXIT
				}
				SendItemCommand(item_id);
				Close();
				event.SetId(item_id);
				event.Skip();
				});
		}
	}

	if (hasDynamicItems) {
		// Add a separator if there were dynamic items and we're about to add fixed ones
		wxPanel* separator = new wxPanel(m_panel, wxID_ANY, wxDefaultPosition, wxSize(CFG_INT("HomeMenuWidth") - 10, 1));
		separator->SetName("HomeMenuSeparator");
		separator->SetBackgroundColour(CFG_COLOUR("BarTabBorderColour"));
		// Add a bit more vertical margin for this separator
		m_itemSizer->Add(separator, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, CFG_INT("HomeMenuSeparatorHeight"));
	}

	m_itemSizer->AddStretchSpacer(1); // This will push subsequent items to the bottom

	// Helper lambda to create and add menu item panels (similar to the loop above)
	auto createAndAddFixedMenuItemPanel =
		[this](const wxString& text, int id, const wxBitmap& icon) {
		wxPanel* itemPanel = new wxPanel(m_panel, wxID_ANY);
		itemPanel->SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));
		wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);

		if (icon.IsOk()) {
			// For fixed items, ensure icon is scaled if necessary, though 16x16 is standard
			wxStaticBitmap* sb = new wxStaticBitmap(itemPanel, wxID_ANY, icon);
			hsizer->Add(sb, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
		}
		wxStaticText* st = new wxStaticText(itemPanel, id, text);
		st->SetFont(CFG_DEFAULTFONT());
		st->SetForegroundColour(CFG_COLOUR("MenuTextColour"));
		hsizer->Add(st, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
		itemPanel->SetSizer(hsizer);

		m_itemSizer->Add(itemPanel, 0, wxEXPAND | wxALL, 2);
		itemPanel->SetMinSize(wxSize(CFG_INT("HomeMenuWidth"), CFG_INT("HomeMenuItemHeight")));

		itemPanel->Bind(wxEVT_LEFT_DOWN, [this, item_id = id, itemPanel](wxMouseEvent& event) {
			if (item_id == ID_THEME_MENU) {
				ShowThemeSubmenu(itemPanel);
			}
			else {
				SendItemCommand(item_id);
				Close();
			}
			event.SetId(item_id);
			event.Skip();
			});
		st->Bind(wxEVT_LEFT_DOWN, [this, item_id = id, itemPanel](wxMouseEvent& event) {
			if (item_id == ID_THEME_MENU) {
				ShowThemeSubmenu(itemPanel);
			}
			else {
				SendItemCommand(item_id);
				Close();
			}
			event.SetId(item_id);
			event.Skip();
			});
		};

	auto addFixedSeparatorToSizer = [&]() {
		wxPanel* separator = new wxPanel(m_panel, wxID_ANY, wxDefaultPosition, wxSize(CFG_INT("HomeMenuWidth") - 10, 1));
		separator->SetName("HomeMenuSeparator");
		separator->SetBackgroundColour(CFG_COLOUR("BarTabBorderColour"));
		m_itemSizer->Add(separator, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, (CFG_INT("HomeMenuSeparatorHeight") - 1) / 2);
		};

	// Add fixed bottom items
	// Theme selection submenu
	createAndAddFixedMenuItemPanel("Theme", ID_THEME_MENU, SVG_ICON("palette", wxSize(16, 16)));
	addFixedSeparatorToSizer();

	createAndAddFixedMenuItemPanel("Settings", wxID_PREFERENCES, SVG_ICON("settings", wxSize(16, 16)));
	addFixedSeparatorToSizer();
	createAndAddFixedMenuItemPanel("Person Profiles", wxID_PREFERENCES, SVG_ICON("person", wxSize(16, 16)));
	addFixedSeparatorToSizer();
	createAndAddFixedMenuItemPanel("About", wxID_ABOUT, SVG_ICON("about", wxSize(16, 16)));
	addFixedSeparatorToSizer();
	createAndAddFixedMenuItemPanel("E&xit", wxID_EXIT, SVG_ICON("exit", wxSize(16, 16)));

	m_panel->Layout(); // Tell the panel's sizer to arrange children
	// No Fit() or SetSize() needed here, as frame's size is fixed and panel expands.
}

void FlatUIHomeMenu::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(m_panel);
	int w, h;
	m_panel->GetSize(&w, &h);

	// Fill background with theme color
	dc.SetBrush(wxBrush(CFG_COLOUR("PrimaryContentBgColour")));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, w, h);

	// Draw border
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(CFG_COLOUR("HomeMenuBorderColour"), 1));
	dc.DrawRectangle(0, 0, w, h);
}

void FlatUIHomeMenu::OnMouseMotion(wxMouseEvent& event)
{
	event.Skip();
}

void FlatUIHomeMenu::SendItemCommand(int id)
{
	if (m_eventSinkFrame && id != wxID_ANY && id != wxID_SEPARATOR) {
		wxCommandEvent cmdEvent(wxEVT_MENU, id);
		if (id == wxID_EXIT) {
			cmdEvent.SetEventObject(m_eventSinkFrame);
		}
		else {
			cmdEvent.SetEventObject(this);
		}
		wxPostEvent(m_eventSinkFrame, cmdEvent);
	}
}

void FlatUIHomeMenu::ShowAt(const wxPoint& pos, int contentHeight, bool& isShow)
{
	SetPosition(pos);
	SetSize(wxSize(CFG_INT("HomeMenuWidth"), contentHeight));
	Popup(); // Using Show as Popup() caused issues

	// Try to set focus to the popup window itself.
	// It's important that the window is visible and able to receive focus.
	if (IsShownOnScreen()) { // Check if it's actually on screen
		SetFocus(); // Equivalent to this->SetFocus()
	}
	else {
		// If not shown on screen immediately, SetFocus might fail.
		// This could indicate a deeper issue or a need to defer SetFocus.
		wxLogDebug(wxT("FlatUIHomeMenu::ShowAt - Window not shown on screen when trying to SetFocus."));
	}
	isShow = true;
}

bool FlatUIHomeMenu::Close(bool force)
{
	if (!IsShown()) {
		return false;
	}

	Hide();
	wxWindow* parent = GetParent();
	if (parent) {
		FlatUIHomeSpace* ownerHomeSpace = dynamic_cast<FlatUIHomeSpace*>(parent);
		if (ownerHomeSpace) {
			ownerHomeSpace->OnHomeMenuClosed(this);
		}
	}
	return true;
}

void FlatUIHomeMenu::OnDismiss()
{
	if (IsShown()) {
		Close();
	}
}

bool FlatUIHomeMenu::ProcessEvent(wxEvent& event)
{
	// Intercept hide events and call our Close method
	if (event.GetEventType() == wxEVT_SHOW && !static_cast<wxShowEvent&>(event).IsShown()) {
		Close();
		return true;
	}
	return wxPopupTransientWindow::ProcessEvent(event);
}

void FlatUIHomeMenu::OnKillFocus(wxFocusEvent& event)
{
	// Get the window that is receiving focus
	wxWindow* newFocus = event.GetWindow();

	// Check if the new focus window is this popup itself or one of its children.
	// If focus moves to a child of the popup, we don't want to close it.
	bool focusStillInside = (newFocus == this);
	if (!focusStillInside && newFocus) {
		wxWindow* parent = newFocus->GetParent();
		while (parent) {
			if (parent == this) {
				focusStillInside = true;
				break;
			}
			parent = parent->GetParent();
		}
	}

	if (!focusStillInside) {
		wxLogDebug(wxT("FlatUIHomeMenu::OnKillFocus - Focus moved outside. Closing menu."));
		Close(); // Use our existing Close method which also notifies HomeSpace
	}
	else {
		wxLogDebug(wxT("FlatUIHomeMenu::OnKillFocus - Focus moved to self or child."));
	}

	event.Skip(); // Important to allow default processing too
}

void FlatUIHomeMenu::ShowThemeSubmenu(wxWindow* parentItem)
{
	// Create a context menu for theme selection
	wxMenu* themeMenu = new wxMenu();

	// Get available themes from ThemeManager
	auto& themeManager = ThemeManager::getInstance();
	std::vector<std::string> themes = themeManager.getAvailableThemes();
	std::string currentTheme = themeManager.getCurrentTheme();

	// Add theme options to menu
	for (const auto& themeName : themes) {
		wxString displayName;
		if (themeName == "default") {
			displayName = "Light Theme";
		}
		else if (themeName == "dark") {
			displayName = "Dark Theme";
		}
		else if (themeName == "blue") {
			displayName = "Blue Theme";
		}
		else {
			displayName = wxString::FromUTF8(themeName);
		}

		int menuId = ID_THEME_DEFAULT;
		if (themeName == "dark") menuId = ID_THEME_DARK;
		else if (themeName == "blue") menuId = ID_THEME_BLUE;

		wxMenuItem* item = themeMenu->AppendRadioItem(menuId, displayName);
		if (themeName == currentTheme) {
			item->Check(true);
		}
	}

	// Bind menu events
	themeMenu->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) {
		std::string selectedTheme;
		switch (evt.GetId()) {
		case ID_THEME_DEFAULT:
			selectedTheme = "default";
			break;
		case ID_THEME_DARK:
			selectedTheme = "dark";
			break;
		case ID_THEME_BLUE:
			selectedTheme = "blue";
			break;
		default:
			return;
		}

		// Apply theme change
		auto& themeManager = ThemeManager::getInstance();
		if (themeManager.setCurrentTheme(selectedTheme)) {
			// Send theme change event to main frame
			if (m_eventSinkFrame) {
				wxCommandEvent themeChangeEvent(wxEVT_THEME_CHANGED, wxID_ANY);
				themeChangeEvent.SetString(wxString::FromUTF8(selectedTheme));
				themeChangeEvent.SetEventObject(this);
				wxPostEvent(m_eventSinkFrame, themeChangeEvent);
			}

			// Request UI refresh
			if (GetParent()) {
				GetParent()->Refresh(true);
				GetParent()->Update();
			}
			if (m_eventSinkFrame) {
				m_eventSinkFrame->Refresh(true);
				m_eventSinkFrame->Update();
			}
		}

		// Close main menu
		Close();
		});

	// Show submenu next to the theme item
	wxPoint menuPos = parentItem->ClientToScreen(wxPoint(parentItem->GetSize().GetWidth(), 0));
	PopupMenu(themeMenu, ScreenToClient(menuPos));

	// Clean up menu after use
	delete themeMenu;
}

void FlatUIHomeMenu::RefreshTheme()
{
	// Update background colors
	SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));

	if (m_panel) {
		m_panel->SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));

		// Recursive function to update all child windows
		std::function<void(wxWindow*)> updateChildrenTheme = [&](wxWindow* window) {
			if (!window) return;
			
			wxString className = window->GetClassInfo()->GetClassName();
			
			if (className == wxString("wxPanel")) {
				// Check if this is a separator by name or height
				wxString windowName = window->GetName();
				if (windowName == "HomeMenuSeparator" || window->GetSize().GetHeight() == 1) {
					wxColour separatorColor = CFG_COLOUR("BarTabBorderColour");
					window->SetBackgroundColour(separatorColor);
					window->Show(true); // Ensure separator is visible
					window->Refresh(true);
					window->Update();
					// Force immediate paint
					window->SetBackgroundStyle(wxBG_STYLE_COLOUR);
					LOG_DBG("Updated HomeMenu separator ('" + windowName.ToStdString() + "') background to: " + 
						std::to_string(separatorColor.Red()) + "," + 
						std::to_string(separatorColor.Green()) + "," + 
						std::to_string(separatorColor.Blue()), "FlatUIHomeMenu");
				}
				else {
					window->SetBackgroundColour(CFG_COLOUR("PrimaryContentBgColour"));
					window->Refresh(true);
				}
			}
			else if (className == wxString("wxStaticText")) {
				wxStaticText* text = static_cast<wxStaticText*>(window);
				text->SetForegroundColour(CFG_COLOUR("MenuTextColour"));
				text->SetFont(CFG_DEFAULTFONT());
				text->Refresh(true);
			}
			else if (className == wxString("wxStaticBitmap")) {
				// Refresh bitmap to ensure it's properly displayed
				window->Refresh(true);
			}
			
			// Recursively update all children
			wxWindowList& children = window->GetChildren();
			for (wxWindow* child : children) {
				updateChildrenTheme(child);
			}
		};

		// Update all children recursively
		updateChildrenTheme(m_panel);

		m_panel->Refresh(true);
		m_panel->Update();
	}

	Refresh(true);
	Update();
	LOG_INF("Home menu theme refresh completed", "FlatUIHomeMenu");
}