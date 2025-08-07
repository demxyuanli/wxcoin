# Font Configuration Guide

This guide explains how to use the font configuration system for custom FlatWidgets.

## Overview

The font configuration system allows you to define fonts for different UI elements through configuration files, making it easy to customize the appearance of your application without modifying code.

## Configuration File Structure

Font configurations are defined in the `config/config.ini` file under the `[Font]` section:

```ini
[Font]
# Default font configuration
DefaultFontSize=8
DefaultFontFamily=wxFONTFAMILY_TELETYPE
DefaultFontStyle=wxFONTSTYLE_NORMAL
DefaultFontWeight=wxFONTWEIGHT_NORMAL
DefaultFontFaceName=Consolas

# Button font configuration
ButtonFontSize=8
ButtonFontFamily=wxFONTFAMILY_DEFAULT
ButtonFontStyle=wxFONTSTYLE_NORMAL
ButtonFontWeight=wxFONTWEIGHT_NORMAL
ButtonFontFaceName=Microsoft YaHei

# Label font configuration
LabelFontSize=8
LabelFontFamily=wxFONTFAMILY_DEFAULT
LabelFontStyle=wxFONTSTYLE_NORMAL
LabelFontWeight=wxFONTWEIGHT_NORMAL
LabelFontFaceName=Microsoft YaHei

# Text control font configuration
TextCtrlFontSize=8
TextCtrlFontFamily=wxFONTFAMILY_TELETYPE
TextCtrlFontStyle=wxFONTSTYLE_NORMAL
TextCtrlFontWeight=wxFONTWEIGHT_NORMAL
TextCtrlFontFaceName=Consolas
```

## Supported Font Types

The following font types are supported:

- `Default` - Default application font
- `Title` - Title font for headers and titles
- `Label` - Font for labels and text elements
- `Button` - Font for buttons
- `TextCtrl` - Font for text input controls
- `Choice` - Font for dropdown and choice controls
- `Status` - Font for status bar text
- `Small` - Small font for secondary information
- `Large` - Large font for emphasis

## Font Properties

Each font type supports the following properties:

- `FontSize` - Font size in points
- `FontFamily` - Font family (wxFONTFAMILY_*)
- `FontStyle` - Font style (wxFONTSTYLE_*)
- `FontWeight` - Font weight (wxFONTWEIGHT_*)
- `FontFaceName` - Specific font face name

## Using Font Configuration in FlatWidgets

### FlatButton

```cpp
// Create a button that uses configuration font
FlatButton* button = new FlatButton(this, wxID_ANY, "Click Me");
button->UseConfigFont(true); // Enable config font (default)

// Or use custom font
wxFont customFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
button->SetCustomFont(customFont);
button->UseConfigFont(false); // Disable config font
```

### FlatCheckBox

```cpp
// Create a checkbox that uses configuration font
FlatCheckBox* checkbox = new FlatCheckBox(this, wxID_ANY, "Check me");
checkbox->UseConfigFont(true); // Enable config font (default)

// Or use custom font
wxFont customFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
checkbox->SetCustomFont(customFont);
checkbox->UseConfigFont(false); // Disable config font
```

### FlatLineEdit

```cpp
// Create a line edit that uses configuration font
FlatLineEdit* lineEdit = new FlatLineEdit(this, wxID_ANY, "Enter text...");
lineEdit->UseConfigFont(true); // Enable config font (default)

// Or use custom font
wxFont customFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
lineEdit->SetCustomFont(customFont);
lineEdit->UseConfigFont(false); // Disable config font
```

## Font Manager API

The `FontManager` class provides the following methods:

```cpp
// Get fonts for different UI elements
wxFont getDefaultFont();
wxFont getTitleFont();
wxFont getLabelFont();
wxFont getButtonFont();
wxFont getTextCtrlFont();
wxFont getChoiceFont();
wxFont getStatusFont();
wxFont getSmallFont();
wxFont getLargeFont();

// Get font with custom size
wxFont getFont(const wxString& fontType, int customSize = -1);

// Apply font to window and its children
void applyFontToWindow(wxWindow* window, const wxString& fontType);
void applyFontToWindowAndChildren(wxWindow* window, const wxString& fontType);
```

## Dynamic Font Reloading

You can reload font configurations at runtime:

```cpp
// Reload font configuration
FontManager& fontManager = FontManager::getInstance();
fontManager.reloadConfig();

// Reload font for specific widget
button->ReloadFontFromConfig();
checkbox->ReloadFontFromConfig();
lineEdit->ReloadFontFromConfig();
```

## Font Family Values

Supported font family values:

- `wxFONTFAMILY_DEFAULT` - Default font family
- `wxFONTFAMILY_DECORATIVE` - Decorative font family
- `wxFONTFAMILY_ROMAN` - Roman font family
- `wxFONTFAMILY_SCRIPT` - Script font family
- `wxFONTFAMILY_SWISS` - Swiss font family
- `wxFONTFAMILY_MODERN` - Modern font family
- `wxFONTFAMILY_TELETYPE` - Teletype font family

## Font Style Values

Supported font style values:

- `wxFONTSTYLE_NORMAL` - Normal style
- `wxFONTSTYLE_ITALIC` - Italic style
- `wxFONTSTYLE_SLANT` - Slant style

## Font Weight Values

Supported font weight values:

- `wxFONTWEIGHT_NORMAL` - Normal weight
- `wxFONTWEIGHT_LIGHT` - Light weight
- `wxFONTWEIGHT_BOLD` - Bold weight
- `wxFONTWEIGHT_EXTRALIGHT` - Extra light weight
- `wxFONTWEIGHT_EXTRABOLD` - Extra bold weight
- `wxFONTWEIGHT_MEDIUM` - Medium weight
- `wxFONTWEIGHT_SEMIBOLD` - Semi-bold weight
- `wxFONTWEIGHT_THIN` - Thin weight
- `wxFONTWEIGHT_MAX` - Maximum weight

## Example Configuration

Here's a complete example of font configuration:

```ini
[Font]
# Default font - used when no specific font is defined
DefaultFontSize=9
DefaultFontFamily=wxFONTFAMILY_DEFAULT
DefaultFontStyle=wxFONTSTYLE_NORMAL
DefaultFontWeight=wxFONTWEIGHT_NORMAL
DefaultFontFaceName=Segoe UI

# Title font - for headers and titles
TitleFontSize=12
TitleFontFamily=wxFONTFAMILY_DEFAULT
TitleFontStyle=wxFONTSTYLE_NORMAL
TitleFontWeight=wxFONTWEIGHT_BOLD
TitleFontFaceName=Segoe UI

# Label font - for labels and text elements
LabelFontSize=9
LabelFontFamily=wxFONTFAMILY_DEFAULT
LabelFontStyle=wxFONTSTYLE_NORMAL
LabelFontWeight=wxFONTWEIGHT_NORMAL
LabelFontFaceName=Segoe UI

# Button font - for buttons
ButtonFontSize=9
ButtonFontFamily=wxFONTFAMILY_DEFAULT
ButtonFontStyle=wxFONTSTYLE_NORMAL
ButtonFontWeight=wxFONTWEIGHT_NORMAL
ButtonFontFaceName=Segoe UI

# Text control font - for text inputs
TextCtrlFontSize=9
TextCtrlFontFamily=wxFONTFAMILY_TELETYPE
TextCtrlFontStyle=wxFONTSTYLE_NORMAL
TextCtrlFontWeight=wxFONTWEIGHT_NORMAL
TextCtrlFontFaceName=Consolas

# Choice font - for dropdowns
ChoiceFontSize=9
ChoiceFontFamily=wxFONTFAMILY_DEFAULT
ChoiceFontStyle=wxFONTSTYLE_NORMAL
ChoiceFontWeight=wxFONTWEIGHT_NORMAL
ChoiceFontFaceName=Segoe UI

# Status font - for status bar
StatusFontSize=8
StatusFontFamily=wxFONTFAMILY_DEFAULT
StatusFontStyle=wxFONTSTYLE_NORMAL
StatusFontWeight=wxFONTWEIGHT_NORMAL
StatusFontFaceName=Segoe UI

# Small font - for secondary information
SmallFontSize=7
SmallFontFamily=wxFONTFAMILY_DEFAULT
SmallFontStyle=wxFONTSTYLE_NORMAL
SmallFontWeight=wxFONTWEIGHT_NORMAL
SmallFontFaceName=Segoe UI

# Large font - for emphasis
LargeFontSize=11
LargeFontFamily=wxFONTFAMILY_DEFAULT
LargeFontStyle=wxFONTSTYLE_NORMAL
LargeFontWeight=wxFONTWEIGHT_BOLD
LargeFontFaceName=Segoe UI
```

## Best Practices

1. **Use consistent font families** - Stick to a few font families throughout your application for consistency
2. **Consider readability** - Choose font sizes that are comfortable to read
3. **Test on different platforms** - Font availability may vary across platforms
4. **Provide fallbacks** - Always provide fallback fonts in case the specified font is not available
5. **Use semantic naming** - Use descriptive names for font types that reflect their purpose

## Troubleshooting

### Font not loading
- Check if the font file exists on the system
- Verify the font name spelling in the configuration
- Use fallback fonts for better compatibility

### Font size issues
- Ensure font sizes are reasonable (typically 7-16 points)
- Consider DPI scaling on high-resolution displays
- Test on different screen resolutions

### Performance considerations
- Font loading is cached for better performance
- Avoid changing fonts frequently during runtime
- Use font families that are commonly available on target platforms
