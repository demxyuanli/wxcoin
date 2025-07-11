#ifndef SVG_ICON_MANAGER_H
#define SVG_ICON_MANAGER_H

#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/bmpbndl.h>  // Use wxBitmapBundle instead of wxSVG
#include <map>
#include <memory>

/**
 * @class SvgIconManager
 * @brief Enhanced utility class to manage and load SVG icons using wxBitmapBundle.
 * Provides functionality to retrieve wxBitmap for wxButton usage based on icon name and size.
 * Supports caching and singleton pattern for better performance.
 */
class SvgIconManager {
private:
    std::map<wxString, wxString> iconMap; // Maps icon names to file paths
    std::map<wxString, wxBitmap> iconCache; // Cache for rendered bitmaps
    std::map<wxString, wxBitmapBundle> bundleCache; // Cache for bitmap bundles
    std::map<wxString, wxString> themedSvgCache; // Cache for theme-processed SVG content
    wxString iconDir; // Directory containing SVG files
    static std::unique_ptr<SvgIconManager> instance;
    static wxString defaultIconDir;

    /**
     * @brief Loads all SVG files from the specified directory into the icon map.
     */
    void LoadIcons();

    /**
     * @brief Generates cache key for bitmap caching.
     */
    wxString GetCacheKey(const wxString& name, const wxSize& size) const;

    /**
     * @brief Gets or creates a bitmap bundle for the specified icon.
     */
    wxBitmapBundle GetBitmapBundle(const wxString& name);

    /**
     * @brief Applies theme colors to SVG content.
     * @param svgContent The original SVG content string.
     * @return Theme-processed SVG content string.
     */
    wxString ApplyThemeToSvg(const wxString& svgContent);

    /**
     * @brief Reads SVG file content as string.
     * @param filePath Path to the SVG file.
     * @return SVG content as string, or empty string if failed.
     */
    wxString ReadSvgFile(const wxString& filePath);

    /**
     * @brief Gets theme-processed SVG content.
     * @param name Icon name.
     * @return Theme-processed SVG content, or empty string if failed.
     */
    wxString GetThemedSvgContent(const wxString& name);

    /**
     * @brief Applies direct theme colors to SVG content without analyzing original colors.
     * @param svgContent The original SVG content.
     * @param primaryIconColor Primary icon color for fills and strokes.
     * @param backgroundIconColor Background color for light elements.
     * @return Theme-processed SVG content.
     */
    wxString ApplyDirectThemeColors(const wxString& svgContent, const wxString& primaryIconColor, const wxString& backgroundIconColor);

    /**
     * @brief Adds default fill color to SVG elements without color attributes.
     * @param svgContent The SVG content to process.
     * @param defaultColor The default color to apply.
     * @return SVG content with default colors applied.
     */
    wxString AddDefaultFillToElements(const wxString& svgContent, const wxString& defaultColor);

    /**
     * @brief Normalizes the SVG structure.
     * @param svgContent The original SVG content string.
     * @return Normalized SVG content string.
     */
    wxString NormalizeSvgStructure(const wxString& svgContent);

    /**
     * @brief Replaces non-light colors in specified attributes with theme color.
     * @param content SVG content to process.
     * @param attribute Attribute name (fill, stroke, etc.).
     * @param targetColor Theme color to replace with.
     * @return SVG content with non-light colors replaced.
     */
    wxString ReplaceNonLightColors(const wxString& content, const wxString& attribute, const wxString& targetColor);

    /**
     * @brief Replaces non-light colors in CSS style attributes.
     * @param content SVG content to process.
     * @param targetColor Theme color to replace with.
     * @return SVG content with non-light colors in styles replaced.
     */
    wxString ReplaceNonLightColorsInStyles(const wxString& content, const wxString& targetColor);

    /**
     * @brief Determines if a color should be replaced based on brightness.
     * @param colorValue Color value to analyze.
     * @return True if color should be replaced (dark/medium), false if light.
     */
    bool ShouldReplaceColor(const wxString& colorValue);

    /**
     * @brief Calculates the perceived brightness of a color.
     * @param colorValue Color value in any supported format.
     * @return Brightness value (0-255) or -1 if invalid.
     */
    int CalculateColorBrightness(const wxString& colorValue);

public:
    /**
     * @brief Constructor.
     * @param dir The directory containing SVG files.
     */
    SvgIconManager(const wxString& dir);

    /**
     * @brief Gets the singleton instance.
     */
    static SvgIconManager& GetInstance();

    /**
     * @brief Sets the default icon directory for the singleton instance.
     */
    static void SetDefaultIconDirectory(const wxString& dir);

    /**
     * @brief Gets a wxBitmap for the specified icon name and size.
     * @param name The name of the icon (without .svg extension).
     * @param size The desired size of the bitmap.
     * @param useCache Whether to use cached bitmaps for better performance.
     * @return wxBitmap containing the rendered SVG, or an empty bitmap if the icon is not found.
     */
    wxBitmap GetIconBitmap(const wxString& name, const wxSize& size, bool useCache = true);

    /**
     * @brief Gets a wxBitmap with fallback to default icon if not found.
     * @param name The name of the icon (without .svg extension).
     * @param size The desired size of the bitmap.
     * @param fallbackName Fallback icon name if primary icon is not found.
     * @return wxBitmap containing the rendered SVG.
     */
    wxBitmap GetIconBitmapWithFallback(const wxString& name, const wxSize& size, const wxString& fallbackName = "default");

    /**
     * @brief Gets a wxBitmapBundle for the specified icon name.
     * @param name The name of the icon (without .svg extension).
     * @return wxBitmapBundle containing the SVG, or an empty bundle if not found.
     */
    wxBitmapBundle GetIconBundle(const wxString& name);

    /**
     * @brief Checks if an icon exists in the manager.
     * @param name The name of the icon to check.
     * @return True if the icon exists, false otherwise.
     */
    bool HasIcon(const wxString& name) const;

    /**
     * @brief Gets the list of available icon names.
     * @return A wxArrayString containing all icon names.
     */
    wxArrayString GetAvailableIcons() const;

    /**
     * @brief Clears all caches (bitmap, bundle, and themed SVG).
     */
    void ClearCache();

    /**
     * @brief Clears only the themed SVG cache (useful when theme changes).
     */
    void ClearThemeCache();

    /**
     * @brief Preloads commonly used icons into cache.
     */
    void PreloadCommonIcons(const wxSize& size);
};

// Convenience macros for easy icon access
#define SVG_ICON(name, size) SvgIconManager::GetInstance().GetIconBitmap(name, size)
#define SVG_ICON_FALLBACK(name, size, fallback) SvgIconManager::GetInstance().GetIconBitmapWithFallback(name, size, fallback)
#define SVG_BUNDLE(name) SvgIconManager::GetInstance().GetIconBundle(name)
#define SVG_THEMED_ICON(name, size) SvgIconManager::GetInstance().GetIconBitmap(name, size, true) // Always use cache for themed icons

#endif // SVG_ICON_MANAGER_H