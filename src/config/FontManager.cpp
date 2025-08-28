#include "config/FontManager.h"
#include "logger/Logger.h"
#include <wx/window.h>
#include <wx/control.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>

FontManager* FontManager::instance = nullptr;

FontManager::FontManager() {
	configManager = &ConfigManager::getInstance();
}

FontManager::~FontManager() {
}

FontManager& FontManager::getInstance() {
	if (!instance) {
		instance = new FontManager();
	}
	return *instance;
}

bool FontManager::initialize(const std::string& configFilePath) {
	if (configFilePath.empty()) {
		return configManager->initialize("");
	}
	else {
		return configManager->initialize(configFilePath);
	}
}

wxFontFamily FontManager::stringToFontFamily(const wxString& familyStr) {
	if (familyStr == "wxFONTFAMILY_DEFAULT") return wxFONTFAMILY_DEFAULT;
	if (familyStr == "wxFONTFAMILY_DECORATIVE") return wxFONTFAMILY_DECORATIVE;
	if (familyStr == "wxFONTFAMILY_ROMAN") return wxFONTFAMILY_ROMAN;
	if (familyStr == "wxFONTFAMILY_SCRIPT") return wxFONTFAMILY_SCRIPT;
	if (familyStr == "wxFONTFAMILY_SWISS") return wxFONTFAMILY_SWISS;
	if (familyStr == "wxFONTFAMILY_MODERN") return wxFONTFAMILY_MODERN;
	if (familyStr == "wxFONTFAMILY_TELETYPE") return wxFONTFAMILY_TELETYPE;
	return wxFONTFAMILY_DEFAULT;
}

wxFontStyle FontManager::stringToFontStyle(const wxString& styleStr) {
	if (styleStr == "wxFONTSTYLE_NORMAL") return wxFONTSTYLE_NORMAL;
	if (styleStr == "wxFONTSTYLE_ITALIC") return wxFONTSTYLE_ITALIC;
	if (styleStr == "wxFONTSTYLE_SLANT") return wxFONTSTYLE_SLANT;
	return wxFONTSTYLE_NORMAL;
}

wxFontWeight FontManager::stringToFontWeight(const wxString& weightStr) {
	if (weightStr == "wxFONTWEIGHT_NORMAL") return wxFONTWEIGHT_NORMAL;
	if (weightStr == "wxFONTWEIGHT_LIGHT") return wxFONTWEIGHT_LIGHT;
	if (weightStr == "wxFONTWEIGHT_BOLD") return wxFONTWEIGHT_BOLD;
	if (weightStr == "wxFONTWEIGHT_EXTRALIGHT") return wxFONTWEIGHT_EXTRALIGHT;
	if (weightStr == "wxFONTWEIGHT_EXTRABOLD") return wxFONTWEIGHT_EXTRABOLD;
	if (weightStr == "wxFONTWEIGHT_MEDIUM") return wxFONTWEIGHT_MEDIUM;
	if (weightStr == "wxFONTWEIGHT_SEMIBOLD") return wxFONTWEIGHT_SEMIBOLD;
	if (weightStr == "wxFONTWEIGHT_THIN") return wxFONTWEIGHT_THIN;
	if (weightStr == "wxFONTWEIGHT_MAX") return wxFONTWEIGHT_MAX;
	return wxFONTWEIGHT_NORMAL;
}

wxFont FontManager::createFontFromConfig(const wxString& prefix) {
	wxString section = "Font";

	int size = configManager->getInt(section.ToStdString(), (prefix + "FontSize").ToStdString(), 8);
	wxString familyStr = configManager->getString(section.ToStdString(), (prefix + "FontFamily").ToStdString(), "wxFONTFAMILY_DEFAULT");
	wxString styleStr = configManager->getString(section.ToStdString(), (prefix + "FontStyle").ToStdString(), "wxFONTSTYLE_NORMAL");
	wxString weightStr = configManager->getString(section.ToStdString(), (prefix + "FontWeight").ToStdString(), "wxFONTWEIGHT_NORMAL");
	wxString faceName = configManager->getString(section.ToStdString(), (prefix + "FontFaceName").ToStdString(), "");

	wxFontFamily family = stringToFontFamily(familyStr);
	wxFontStyle style = stringToFontStyle(styleStr);
	wxFontWeight weight = stringToFontWeight(weightStr);

	wxFont font(size, family, style, weight);
	if (!faceName.empty()) {
		font.SetFaceName(faceName);
	}

	return font;
}

wxFont FontManager::getDefaultFont() {
	return createFontFromConfig("Default");
}

wxFont FontManager::getTitleFont() {
	return createFontFromConfig("Title");
}

wxFont FontManager::getLabelFont() {
	return createFontFromConfig("Label");
}

wxFont FontManager::getButtonFont() {
	return createFontFromConfig("Button");
}

wxFont FontManager::getTextCtrlFont() {
	return createFontFromConfig("TextCtrl");
}

wxFont FontManager::getChoiceFont() {
	return createFontFromConfig("Choice");
}

wxFont FontManager::getStatusFont() {
	return createFontFromConfig("Status");
}

wxFont FontManager::getSmallFont() {
	return createFontFromConfig("Small");
}

wxFont FontManager::getLargeFont() {
	return createFontFromConfig("Large");
}

wxFont FontManager::getFont(const wxString& fontType, int customSize) {
	wxFont font;

	if (fontType == "Default") font = getDefaultFont();
	else if (fontType == "Title") font = getTitleFont();
	else if (fontType == "Label") font = getLabelFont();
	else if (fontType == "Button") font = getButtonFont();
	else if (fontType == "TextCtrl") font = getTextCtrlFont();
	else if (fontType == "Choice") font = getChoiceFont();
	else if (fontType == "Status") font = getStatusFont();
	else if (fontType == "Small") font = getSmallFont();
	else if (fontType == "Large") font = getLargeFont();
	else font = getDefaultFont();

	if (customSize > 0) {
		font.SetPointSize(customSize);
	}

	return font;
}

void FontManager::applyFontToWindow(wxWindow* window, const wxString& fontType) {
	if (!window) return;

	wxFont font = getFont(fontType);
	window->SetFont(font);
}

void FontManager::applyFontToWindowAndChildren(wxWindow* window, const wxString& fontType) {
	if (!window) return;

	// Apply font to the window itself
	applyFontToWindow(window, fontType);

	// Apply font to all children
	wxWindowList& children = window->GetChildren();
	for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
		wxWindow* child = *it;
		if (child) {
			// Determine appropriate font type based on control type
			wxString childFontType = fontType;

			if (dynamic_cast<wxButton*>(child)) {
				childFontType = "Button";
			}
			else if (dynamic_cast<wxTextCtrl*>(child)) {
				childFontType = "TextCtrl";
			}
			else if (dynamic_cast<wxChoice*>(child)) {
				childFontType = "Choice";
			}
			else if (dynamic_cast<wxStaticText*>(child)) {
				childFontType = "Label";
			}
			else if (dynamic_cast<wxCheckBox*>(child)) {
				childFontType = "Label";
			}
			else if (dynamic_cast<wxSlider*>(child)) {
				childFontType = "Label";
			}
			else if (dynamic_cast<wxSpinCtrl*>(child) || dynamic_cast<wxSpinCtrlDouble*>(child)) {
				childFontType = "TextCtrl";
			}

			applyFontToWindow(child, childFontType);

			// Recursively apply to children of this child
			applyFontToWindowAndChildren(child, childFontType);
		}
	}
}

bool FontManager::reloadConfig() {
	return configManager->reload();
}

wxString FontManager::getFontInfo(const wxString& fontType) {
	wxFont font = getFont(fontType);
	return wxString::Format("Font: %s, Size: %d, Family: %d, Style: %d, Weight: %d",
		font.GetFaceName(), font.GetPointSize(),
		font.GetFamily(), font.GetStyle(), font.GetWeight());
}