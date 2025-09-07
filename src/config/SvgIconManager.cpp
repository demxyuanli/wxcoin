#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/file.h>
#include <regex>
#include <algorithm>

// Static member definitions
std::unique_ptr<SvgIconManager> SvgIconManager::instance = nullptr;
wxString SvgIconManager::defaultIconDir = "";

SvgIconManager::SvgIconManager(const wxString& dir)
	: iconDir(dir)
{
	LoadIcons();
}

SvgIconManager& SvgIconManager::GetInstance()
{
	if (!instance) {
		if (defaultIconDir.IsEmpty()) {
			// Default to executable directory + icons/svg
			wxString exePath = wxStandardPaths::Get().GetExecutablePath();
			wxFileName exeFile(exePath);
			defaultIconDir = exeFile.GetPath() + wxFILE_SEP_PATH + "config" + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "svg";
		}
		instance = std::make_unique<SvgIconManager>(defaultIconDir);
	}
	return *instance;
}

void SvgIconManager::SetDefaultIconDirectory(const wxString& dir)
{
	defaultIconDir = dir;
	// Reset instance to reload with new directory
	instance.reset();
}

void SvgIconManager::LoadIcons()
{
	if (!wxDir::Exists(iconDir)) {
		LOG_ERR(wxString::Format("SvgIconManager: Icon directory '%s' does not exist.", iconDir.ToStdString()), "SvgIconManager");
		return;
	}

	wxDir dir(iconDir);
	if (!dir.IsOpened()) {
		LOG_ERR(wxString::Format("SvgIconManager: Could not open icon directory '%s'", iconDir.ToStdString()), "SvgIconManager");
		return;
	}

	wxString filename;
	bool cont = dir.GetFirst(&filename, "*.svg", wxDIR_FILES);
	while (cont) {
		wxFileName fn(filename);
		wxString fullPath = iconDir + wxFILE_SEP_PATH + filename;
		iconMap[fn.GetName()] = fullPath;
		// LOG_DBG(wxString::Format("SvgIconManager: Loaded icon '%s' from '%s'", fn.GetName().ToStdString(), fullPath.ToStdString()), "SvgIconManager");
		cont = dir.GetNext(&filename);
	}

	LOG_INF(wxString::Format("SvgIconManager: Loaded %u SVG icons from '%s'", (unsigned int)iconMap.size(), iconDir.ToStdString()), "SvgIconManager");
}

wxString SvgIconManager::GetCacheKey(const wxString& name, const wxSize& size) const
{
	return wxString::Format("%s_%dx%d", name, size.GetWidth(), size.GetHeight());
}

wxBitmapBundle SvgIconManager::GetBitmapBundle(const wxString& name)
{
	// Check bundle cache first
	auto bundleIt = bundleCache.find(name);
	if (bundleIt != bundleCache.end()) {
		return bundleIt->second;
	}

	auto it = iconMap.find(name);
	if (it != iconMap.end()) {
		try {
			// Get themed SVG content
			wxString themedSvgContent = GetThemedSvgContent(name);

			if (!themedSvgContent.IsEmpty()) {
				// Create bitmap bundle from themed SVG content
				wxBitmapBundle bundle = wxBitmapBundle::FromSVG(themedSvgContent.ToUTF8().data(), wxSize(16, 16));

				if (bundle.IsOk()) {
					// Cache the bundle
					bundleCache[name] = bundle;
					return bundle;
				}
				else {
					LOG_WRN(wxString::Format("SvgIconManager: Failed to create bundle from themed SVG for '%s', trying original file.", name.ToStdString()), "SvgIconManager");
					// Fallback to original file
					bundle = wxBitmapBundle::FromSVGFile(it->second, wxSize(16, 16));
					if (bundle.IsOk()) {
						bundleCache[name] = bundle;
						return bundle;
					}
				}
			}
			else {
				// Fallback to original SVG file if theming failed
				wxBitmapBundle bundle = wxBitmapBundle::FromSVGFile(it->second, wxSize(16, 16));
				if (bundle.IsOk()) {
					bundleCache[name] = bundle;
					return bundle;
				}
			}

			LOG_ERR(wxString::Format("SvgIconManager: Failed to create bitmap bundle for icon '%s' from path '%s'.",
				name.ToStdString(), it->second.ToStdString()), "SvgIconManager");
		}
		catch (const std::exception& e) {
			LOG_ERR(wxString::Format("SvgIconManager: Exception while loading SVG icon '%s': %s",
				name.ToStdString(), e.what()), "SvgIconManager");
		}
		catch (...) {
			LOG_ERR(wxString::Format("SvgIconManager: Unknown exception while loading SVG icon '%s'",
				name.ToStdString()), "SvgIconManager");
		}
	}
	else {
		LOG_WRN(wxString::Format("SvgIconManager: Icon '%s' not found.", name.ToStdString()), "SvgIconManager");
	}

	return wxBitmapBundle(); // Return empty bundle
}

wxBitmap SvgIconManager::GetIconBitmap(const wxString& name, const wxSize& size, bool useCache)
{
	// Check cache first if enabled
	if (useCache) {
		wxString cacheKey = GetCacheKey(name, size);
		auto cacheIt = iconCache.find(cacheKey);
		if (cacheIt != iconCache.end()) {
			return cacheIt->second;
		}
	}

	// Get bitmap bundle and extract bitmap at desired size
	wxBitmapBundle bundle = GetBitmapBundle(name);
	if (bundle.IsOk()) {
		wxBitmap bitmap = bundle.GetBitmap(size);
		if (bitmap.IsOk()) {
			// Cache the rendered bitmap if enabled
			if (useCache) {
				wxString cacheKey = GetCacheKey(name, size);
				iconCache[cacheKey] = bitmap;
			}
			return bitmap;
		}
		else {
			LOG_ERR(wxString::Format("SvgIconManager: Failed to get bitmap from bundle for icon '%s' at size %dx%d.",
				name.ToStdString(), size.GetWidth(), size.GetHeight()), "SvgIconManager");
		}
	}

	return wxBitmap(); // Return empty bitmap
}

wxBitmap SvgIconManager::GetIconBitmapWithFallback(const wxString& name, const wxSize& size, const wxString& fallbackName)
{
	wxBitmap bitmap = GetIconBitmap(name, size);
	if (!bitmap.IsOk() && !fallbackName.IsEmpty() && fallbackName != name) {
		LOG_DBG(wxString::Format("SvgIconManager: Using fallback icon '%s' for missing icon '%s'",
			fallbackName.ToStdString(), name.ToStdString()), "SvgIconManager");
		bitmap = GetIconBitmap(fallbackName, size);
	}
	return bitmap;
}

wxBitmapBundle SvgIconManager::GetIconBundle(const wxString& name)
{
	return GetBitmapBundle(name);
}

bool SvgIconManager::HasIcon(const wxString& name) const
{
	return iconMap.count(name) > 0;
}

wxArrayString SvgIconManager::GetAvailableIcons() const
{
	wxArrayString names;
	for (const auto& pair : iconMap) {
		names.Add(pair.first);
	}
	return names;
}

void SvgIconManager::ClearCache()
{
	iconCache.clear();
	bundleCache.clear();
	themedSvgCache.clear();
	LOG_DBG("SvgIconManager: All caches cleared", "SvgIconManager");
}

void SvgIconManager::ClearThemeCache()
{
	themedSvgCache.clear();
	// Also clear the rendered caches since they depend on themed SVG
	iconCache.clear();
	bundleCache.clear();
	LOG_DBG("SvgIconManager: Theme cache cleared", "SvgIconManager");
}

wxString SvgIconManager::ReadSvgFile(const wxString& filePath)
{
	if (!wxFile::Exists(filePath)) {
		LOG_WRN(wxString::Format("SvgIconManager: SVG file '%s' does not exist.", filePath.ToStdString()), "SvgIconManager");
		return wxEmptyString;
	}

	wxFile file(filePath, wxFile::read);
	if (!file.IsOpened()) {
		LOG_ERR(wxString::Format("SvgIconManager: Could not open SVG file '%s'.", filePath.ToStdString()), "SvgIconManager");
		return wxEmptyString;
	}

	wxString content;
	if (!file.ReadAll(&content)) {
		LOG_ERR(wxString::Format("SvgIconManager: Failed to read SVG file '%s'.", filePath.ToStdString()), "SvgIconManager");
		return wxEmptyString;
	}

	return content;
}

wxString SvgIconManager::ApplyThemeToSvg(const wxString& svgContent)
{
	if (svgContent.IsEmpty()) {
		return wxEmptyString;
	}

	wxString themedContent = svgContent;

	try {
		// Check if SVG theming is enabled
		bool svgThemeEnabled = CFG_INT("SvgThemeEnabled") != 0;
		// LOG_DBG(wxString::Format("SvgIconManager: SVG theming enabled: %s", svgThemeEnabled ? "true" : "false"), "SvgIconManager");

		if (!svgThemeEnabled) {
			// LOG_DBG("SvgIconManager: SVG theming is disabled", "SvgIconManager");
			return svgContent; // Return original content if theming is disabled
		}

		// Get theme colors
		wxColour primaryIconColor = CFG_COLOUR("SvgPrimaryIconColour");
		wxColour secondaryIconColor = CFG_COLOUR("SvgSecondaryIconColour");
		wxColour disabledIconColor = CFG_COLOUR("SvgDisabledIconColour");
		wxColour highlightIconColor = CFG_COLOUR("SvgHighlightIconColour");
		wxColour primaryBgColor = CFG_COLOUR("PrimaryContentBgColour");
		wxColour secondaryBgColor = CFG_COLOUR("SecondaryBackgroundColour");

		// Convert wxColour to hex strings
		wxString primaryIconHex = wxString::Format("#%02x%02x%02x",
			primaryIconColor.Red(), primaryIconColor.Green(), primaryIconColor.Blue());
		wxString secondaryIconHex = wxString::Format("#%02x%02x%02x",
			secondaryIconColor.Red(), secondaryIconColor.Green(), secondaryIconColor.Blue());
		wxString disabledIconHex = wxString::Format("#%02x%02x%02x",
			disabledIconColor.Red(), disabledIconColor.Green(), disabledIconColor.Blue());
		wxString highlightIconHex = wxString::Format("#%02x%02x%02x",
			highlightIconColor.Red(), highlightIconColor.Green(), highlightIconColor.Blue());
		wxString primaryBgHex = wxString::Format("#%02x%02x%02x",
			primaryBgColor.Red(), primaryBgColor.Green(), primaryBgColor.Blue());
		wxString secondaryBgHex = wxString::Format("#%02x%02x%02x",
			secondaryBgColor.Red(), secondaryBgColor.Green(), secondaryBgColor.Blue());

		// LOG_DBG(wxString::Format("SvgIconManager: Theme colors - Primary: %s, Secondary: %s, Disabled: %s, Highlight: %s",
		//     primaryIconHex, secondaryIconHex, disabledIconHex, highlightIconHex), "SvgIconManager");

		// Direct theme color application - replace all colors with theme colors
		themedContent = ApplyDirectThemeColors(themedContent, primaryIconHex, secondaryBgHex);

		// LOG_DBG(wxString::Format("SvgIconManager: Applied direct theme colors to SVG content. Original length: %d, Themed length: %d",
		//     (int)svgContent.length(), (int)themedContent.length()), "SvgIconManager");
	}
	catch (const std::exception& e) {
		LOG_ERR(wxString::Format("SvgIconManager: Exception while applying theme to SVG: %s", e.what()), "SvgIconManager");
		return svgContent; // Return original content on error
	}
	catch (...) {
		LOG_ERR("SvgIconManager: Unknown exception while applying theme to SVG", "SvgIconManager");
		return svgContent; // Return original content on error
	}

	return themedContent;
}

wxString SvgIconManager::GetThemedSvgContent(const wxString& name)
{
	// Check themed SVG cache first
	auto cacheIt = themedSvgCache.find(name);
	if (cacheIt != themedSvgCache.end()) {
		return cacheIt->second;
	}

	auto it = iconMap.find(name);
	if (it != iconMap.end()) {
		// Read original SVG content
		wxString originalContent = ReadSvgFile(it->second);
		if (!originalContent.IsEmpty()) {
			// LOG_DBG(wxString::Format("SvgIconManager: Original SVG content for '%s' (first 100 chars): %s",
			//     name, originalContent.Left(100)), "SvgIconManager");

			// Apply theme colors
			wxString themedContent = ApplyThemeToSvg(originalContent);

			// Cache the themed content
			themedSvgCache[name] = themedContent;

			// LOG_DBG(wxString::Format("SvgIconManager: Generated themed SVG for icon '%s'", name.ToStdString()), "SvgIconManager");
			return themedContent;
		}
		else {
			LOG_WRN(wxString::Format("SvgIconManager: Failed to read SVG file for icon '%s'", name.ToStdString()), "SvgIconManager");
		}
	}
	else {
		LOG_WRN(wxString::Format("SvgIconManager: Icon '%s' not found in icon map.", name.ToStdString()), "SvgIconManager");
	}

	return wxEmptyString;
}

void SvgIconManager::PreloadCommonIcons(const wxSize& size)
{
	// Preload commonly used icons
	wxArrayString commonIcons;
	commonIcons.Add("home");
	commonIcons.Add("settings");
	commonIcons.Add("user");
	commonIcons.Add("file");
	commonIcons.Add("folder");
	commonIcons.Add("search");
	commonIcons.Add("open");
	commonIcons.Add("save");
	commonIcons.Add("filemenu");
	commonIcons.Add("copy");
	commonIcons.Add("paste");
	commonIcons.Add("find");
	commonIcons.Add("help");
	commonIcons.Add("person");
	commonIcons.Add("info");
	commonIcons.Add("about");
	commonIcons.Add("exit");
	commonIcons.Add("thumbtack");
	commonIcons.Add("thumbtack");

	for (const wxString& iconName : commonIcons) {
		GetIconBitmap(iconName, size, true); // This will cache the bitmap
	}

	LOG_INF(wxString::Format("SvgIconManager: Preloaded %u common icons", (unsigned int)commonIcons.size()), "SvgIconManager");
}

// Enhanced color detection and mapping methods

wxString SvgIconManager::ApplyDirectThemeColors(const wxString& svgContent, const wxString& primaryIconColor, const wxString& backgroundIconColor)
{
	wxString themedContent = svgContent;

	// LOG_DBG("SvgIconManager: Starting selective theme color application (non-light colors only)", "SvgIconManager");

	// Replace colors selectively - only non-light colors
	themedContent = ReplaceNonLightColors(themedContent, "fill", primaryIconColor);
	themedContent = ReplaceNonLightColors(themedContent, "stroke", primaryIconColor);

	// Handle colors in style attributes
	themedContent = ReplaceNonLightColorsInStyles(themedContent, primaryIconColor);

	// Add default fill color to elements without any color attributes
	themedContent = AddDefaultFillToElements(themedContent, primaryIconColor);

	// LOG_DBG(wxString::Format("SvgIconManager: Selective theme color application completed. Replaced non-light colors with %s", primaryIconColor), "SvgIconManager");

	return themedContent;
}

wxString SvgIconManager::NormalizeSvgStructure(const wxString& svgContent)
{
	wxString normalizedContent = svgContent;

	// Check if SVG has direct path elements under svg root (without g wrapper)
	std::regex directPathRegex("<svg[^>]*>\\s*<path", std::regex_constants::icase);
	std::string content = svgContent.ToStdString();

	if (std::regex_search(content, directPathRegex)) {
		// This SVG has direct path elements, need to wrap them in a group

		// Find the opening svg tag
		std::regex svgOpenRegex("<svg([^>]*)>", std::regex_constants::icase);
		std::smatch svgMatch;

		if (std::regex_search(content, svgMatch, svgOpenRegex)) {
			size_t svgEndPos = svgMatch.position() + svgMatch.length();

			// Find the closing svg tag
			std::regex svgCloseRegex("</svg>", std::regex_constants::icase);
			std::smatch closeMatch;
			std::string remaining = content.substr(svgEndPos);

			if (std::regex_search(remaining, closeMatch, svgCloseRegex)) {
				size_t closeStartPos = svgEndPos + closeMatch.position();

				// Extract the content between svg tags
				std::string svgInnerContent = content.substr(svgEndPos, closeMatch.position());

				// Check if there are path elements not already wrapped in g
				std::regex unwrappedPathRegex("(?!.*<g[^>]*>.*)<path[^>]*>", std::regex_constants::icase);

				if (std::regex_search(svgInnerContent, unwrappedPathRegex)) {
					// Wrap all content in a group element
					std::string newContent = content.substr(0, svgEndPos) +
						"<g>" + svgInnerContent + "</g>" +
						content.substr(closeStartPos);

					normalizedContent = wxString(newContent);
					LOG_DBG("SvgIconManager: Added group wrapper to SVG with direct path elements", "SvgIconManager");
				}
			}
		}
	}

	return normalizedContent;
}

wxString SvgIconManager::AddDefaultFillToElements(const wxString& svgContent, const wxString& defaultColor)
{
	// First normalize the SVG structure by adding g wrappers where needed
	wxString themedContent = NormalizeSvgStructure(svgContent);

	// Now we can use a unified approach - just process group elements
	// since all path elements should now be wrapped in groups
	std::regex groupRegex("<g(\\s+[^>]*?)?>", std::regex_constants::icase);
	std::string content = themedContent.ToStdString();
	std::string result;

	std::sregex_iterator iter(content.begin(), content.end(), groupRegex);
	std::sregex_iterator end;

	size_t lastPos = 0;
	while (iter != end) {
		std::smatch match = *iter;
		std::string groupTag = match.str();

		// Check if this group already has fill or stroke attribute
		if (groupTag.find("fill=") == std::string::npos && groupTag.find("stroke=") == std::string::npos) {
			// Add fill attribute with default color
			size_t insertPos = groupTag.find_last_of('>');
			if (insertPos != std::string::npos) {
				groupTag.insert(insertPos, " fill=\"" + defaultColor.ToStdString() + "\"");
			}
		}

		result += content.substr(lastPos, match.position() - lastPos);
		result += groupTag;
		lastPos = match.position() + match.length();
		++iter;
	}
	result += content.substr(lastPos);

	LOG_DBG("SvgIconManager: Added default fill color to group elements", "SvgIconManager");
	return wxString(result);
}

wxString SvgIconManager::ReplaceNonLightColors(const wxString& content, const wxString& attribute, const wxString& targetColor)
{
	wxString result = content;

	// Create regex pattern to match the attribute with any color value
	std::string pattern = attribute.ToStdString() + "=\"([^\"]+)\"";
	std::regex attrRegex(pattern, std::regex_constants::icase);

	std::string contentStr = content.ToStdString();
	std::string resultStr;

	std::sregex_iterator iter(contentStr.begin(), contentStr.end(), attrRegex);
	std::sregex_iterator end;

	size_t lastPos = 0;
	while (iter != end) {
		std::smatch match = *iter;
		wxString colorValue = wxString(match[1].str());

		// Check if this color should be replaced (non-light colors)
		if (ShouldReplaceColor(colorValue)) {
			// Replace this color
			std::string replacement = attribute.ToStdString() + "=\"" + targetColor.ToStdString() + "\"";

			resultStr += contentStr.substr(lastPos, match.position() - lastPos);
			resultStr += replacement;

			// LOG_DBG(wxString::Format("SvgIconManager: Replaced %s color '%s' with '%s'",
			//        attribute, colorValue, targetColor), "SvgIconManager");
		}
		else {
			// Keep original light color
			resultStr += contentStr.substr(lastPos, match.position() - lastPos);
			resultStr += match.str();

			// LOG_DBG(wxString::Format("SvgIconManager: Preserved light %s color '%s'",
			//        attribute, colorValue), "SvgIconManager");
		}

		lastPos = match.position() + match.length();
		++iter;
	}
	resultStr += contentStr.substr(lastPos);

	return wxString(resultStr);
}

wxString SvgIconManager::ReplaceNonLightColorsInStyles(const wxString& content, const wxString& targetColor)
{
	wxString result = content;

	// Process style attributes like style="fill:#000000;stroke:red"
	std::regex styleRegex("style=\"([^\"]*)\"", std::regex_constants::icase);
	std::string contentStr = content.ToStdString();
	std::string resultStr;

	std::sregex_iterator iter(contentStr.begin(), contentStr.end(), styleRegex);
	std::sregex_iterator end;

	size_t lastPos = 0;
	while (iter != end) {
		std::smatch match = *iter;
		std::string styleValue = match[1].str();

		// Process fill properties within style
		std::regex fillRegex("fill\\s*:\\s*([^;]+)", std::regex_constants::icase);
		std::smatch fillMatch;

		if (std::regex_search(styleValue, fillMatch, fillRegex)) {
			wxString fillColor = wxString(fillMatch[1].str());
			if (ShouldReplaceColor(fillColor)) {
				styleValue = std::regex_replace(styleValue, fillRegex,
					"fill:" + targetColor.ToStdString());
				// LOG_DBG(wxString::Format("SvgIconManager: Replaced style fill color '%s' with '%s'",
				//        fillColor, targetColor), "SvgIconManager");
			}
		}

		// Process stroke properties within style
		std::regex strokeRegex("stroke\\s*:\\s*([^;]+)", std::regex_constants::icase);
		std::smatch strokeMatch;

		if (std::regex_search(styleValue, strokeMatch, strokeRegex)) {
			wxString strokeColor = wxString(strokeMatch[1].str());
			if (ShouldReplaceColor(strokeColor)) {
				styleValue = std::regex_replace(styleValue, strokeRegex,
					"stroke:" + targetColor.ToStdString());
				// LOG_DBG(wxString::Format("SvgIconManager: Replaced style stroke color '%s' with '%s'",
				//        strokeColor, targetColor), "SvgIconManager");
			}
		}

		std::string replacement = "style=\"" + styleValue + "\"";

		resultStr += contentStr.substr(lastPos, match.position() - lastPos);
		resultStr += replacement;
		lastPos = match.position() + match.length();
		++iter;
	}
	resultStr += contentStr.substr(lastPos);

	return wxString(resultStr);
}

bool SvgIconManager::ShouldReplaceColor(const wxString& colorValue)
{
	// Determine if a color should be replaced based on brightness
	// Returns true for dark/medium colors, false for light colors

	int brightness = CalculateColorBrightness(colorValue);

	if (brightness < 0) {
		// Invalid color, check common keywords
		wxString lowerColor = colorValue.Lower();

		// Don't replace light colors and transparent
		if (lowerColor == "white" || lowerColor == "#fff" || lowerColor == "#ffffff" ||
			lowerColor == "transparent" || lowerColor == "none" ||
			lowerColor.Contains("lightgray") || lowerColor.Contains("lightgrey") ||
			lowerColor.Contains("silver")) {
			return false;
		}

		// Replace dark/medium colors
		if (lowerColor == "black" || lowerColor == "#000" || lowerColor == "#000000" ||
			lowerColor == "gray" || lowerColor == "grey" || lowerColor == "currentcolor" ||
			lowerColor.Contains("dark")) {
			return true;
		}

		// For unknown colors, replace them (conservative approach)
		return true;
	}

	// Light colors (brightness > 180) - don't replace
	if (brightness > 180) {
		return false;
	}

	// Dark and medium colors - replace
	return true;
}

int SvgIconManager::CalculateColorBrightness(const wxString& colorValue)
{
	std::string color = colorValue.Lower().ToStdString();

	// Remove whitespace
	color.erase(std::remove_if(color.begin(), color.end(), ::isspace), color.end());

	// Handle hex colors
	if (!color.empty() && color[0] == '#') {
		std::string hex = color.substr(1);
		if (hex.length() == 3) {
			// Short hex format (#RGB -> #RRGGBB)
			hex = std::string(1, hex[0]) + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
		}

		if (hex.length() == 6) {
			try {
				int r = std::stoi(hex.substr(0, 2), nullptr, 16);
				int g = std::stoi(hex.substr(2, 2), nullptr, 16);
				int b = std::stoi(hex.substr(4, 2), nullptr, 16);

				// Calculate perceived brightness using standard formula
				return static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
			}
			catch (...) {
				return -1;
			}
		}
	}

	// Handle RGB/RGBA format
	std::regex rgbRegex("rgba?\\((\\d+),(\\d+),(\\d+)(?:,[\\d.]+)?\\)");
	std::smatch match;
	if (std::regex_match(color, match, rgbRegex)) {
		try {
			int r = std::stoi(match[1].str());
			int g = std::stoi(match[2].str());
			int b = std::stoi(match[3].str());

			return static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
		}
		catch (...) {
			return -1;
		}
	}

	// For named colors, return predefined brightness values
	if (color == "white") return 255;
	if (color == "black") return 0;
	if (color == "gray" || color == "grey") return 128;
	if (color == "lightgray" || color == "lightgrey" || color == "silver") return 192;
	if (color == "darkgray" || color == "darkgrey") return 64;

	return -1; // Unknown format
}