#include "STEPColorManager.h"
#include "OCCGeometry.h"
#include <algorithm>

// Define static color palettes
const std::vector<Quantity_Color> STEPColorManager::WARM_COLORS = {
    Quantity_Color(0.90, 0.12, 0.14, Quantity_TOC_RGB), // strong red
    Quantity_Color(1.00, 0.45, 0.00, Quantity_TOC_RGB), // vivid orange
    Quantity_Color(0.99, 0.76, 0.07, Quantity_TOC_RGB), // bright yellow
    Quantity_Color(0.60, 0.00, 0.00, Quantity_TOC_RGB), // dark red
    Quantity_Color(0.95, 0.30, 0.55, Quantity_TOC_RGB), // pink
    Quantity_Color(0.70, 0.35, 0.00, Quantity_TOC_RGB)  // brownish orange
};

const std::vector<Quantity_Color> STEPColorManager::RAINBOW_COLORS = {
    Quantity_Color(0.90, 0.12, 0.14, Quantity_TOC_RGB), // red
    Quantity_Color(1.00, 0.50, 0.00, Quantity_TOC_RGB), // orange
    Quantity_Color(0.99, 0.76, 0.07, Quantity_TOC_RGB), // yellow
    Quantity_Color(0.20, 0.70, 0.00, Quantity_TOC_RGB), // green
    Quantity_Color(0.00, 0.65, 0.75, Quantity_TOC_RGB), // cyan
    Quantity_Color(0.12, 0.47, 0.71, Quantity_TOC_RGB), // blue
    Quantity_Color(0.42, 0.24, 0.60, Quantity_TOC_RGB)  // purple
};

const std::vector<Quantity_Color> STEPColorManager::MONOCHROME_BLUE = {
    Quantity_Color(0.10, 0.18, 0.30, Quantity_TOC_RGB),
    Quantity_Color(0.12, 0.47, 0.71, Quantity_TOC_RGB),
    Quantity_Color(0.17, 0.63, 0.88, Quantity_TOC_RGB),
    Quantity_Color(0.40, 0.76, 1.00, Quantity_TOC_RGB),
    Quantity_Color(0.70, 0.86, 1.00, Quantity_TOC_RGB)
};

const std::vector<Quantity_Color> STEPColorManager::MONOCHROME_GREEN = {
    Quantity_Color(0.05, 0.30, 0.10, Quantity_TOC_RGB),
    Quantity_Color(0.20, 0.60, 0.20, Quantity_TOC_RGB),
    Quantity_Color(0.33, 0.75, 0.29, Quantity_TOC_RGB),
    Quantity_Color(0.60, 0.85, 0.35, Quantity_TOC_RGB),
    Quantity_Color(0.80, 0.93, 0.60, Quantity_TOC_RGB)
};

const std::vector<Quantity_Color> STEPColorManager::MONOCHROME_GRAY = {
    Quantity_Color(0.15, 0.15, 0.15, Quantity_TOC_RGB),
    Quantity_Color(0.35, 0.35, 0.35, Quantity_TOC_RGB),
    Quantity_Color(0.55, 0.55, 0.55, Quantity_TOC_RGB),
    Quantity_Color(0.75, 0.75, 0.75, Quantity_TOC_RGB),
    Quantity_Color(0.90, 0.90, 0.90, Quantity_TOC_RGB)
};

const std::vector<Quantity_Color> STEPColorManager::DISTINCT_COLORS = {
    Quantity_Color(0.12, 0.47, 0.71, Quantity_TOC_RGB), // blue
    Quantity_Color(1.00, 0.50, 0.05, Quantity_TOC_RGB), // orange
    Quantity_Color(0.17, 0.63, 0.17, Quantity_TOC_RGB), // green
    Quantity_Color(0.84, 0.15, 0.16, Quantity_TOC_RGB), // red
    Quantity_Color(0.58, 0.40, 0.74, Quantity_TOC_RGB), // purple
    Quantity_Color(0.55, 0.34, 0.29, Quantity_TOC_RGB), // brown
    Quantity_Color(0.89, 0.47, 0.76, Quantity_TOC_RGB), // pink
    Quantity_Color(0.50, 0.50, 0.50, Quantity_TOC_RGB), // gray
    Quantity_Color(0.74, 0.74, 0.13, Quantity_TOC_RGB), // olive
    Quantity_Color(0.09, 0.75, 0.81, Quantity_TOC_RGB), // cyan
    Quantity_Color(0.35, 0.31, 0.64, Quantity_TOC_RGB), // indigo
    Quantity_Color(0.95, 0.90, 0.25, Quantity_TOC_RGB)  // bright yellow
};

const std::vector<Quantity_Color> STEPColorManager::DISTINCT_COMPONENT_COLORS = {
    Quantity_Color(0.4, 0.5, 0.6, Quantity_TOC_RGB), // Cool Blue-Gray
    Quantity_Color(0.3, 0.5, 0.7, Quantity_TOC_RGB), // Steel Blue
    Quantity_Color(0.2, 0.4, 0.6, Quantity_TOC_RGB), // Deep Blue
    Quantity_Color(0.4, 0.6, 0.7, Quantity_TOC_RGB), // Light Blue-Gray
    Quantity_Color(0.3, 0.6, 0.5, Quantity_TOC_RGB), // Teal
    Quantity_Color(0.2, 0.5, 0.4, Quantity_TOC_RGB), // Dark Teal
    Quantity_Color(0.5, 0.4, 0.6, Quantity_TOC_RGB), // Cool Purple
    Quantity_Color(0.4, 0.3, 0.5, Quantity_TOC_RGB), // Muted Purple
    Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB), // Neutral Gray
    Quantity_Color(0.4, 0.4, 0.4, Quantity_TOC_RGB), // Dark Gray
    Quantity_Color(0.6, 0.5, 0.4, Quantity_TOC_RGB), // Cool Beige
    Quantity_Color(0.5, 0.6, 0.5, Quantity_TOC_RGB), // Cool Green-Gray
    Quantity_Color(0.3, 0.4, 0.5, Quantity_TOC_RGB), // Slate Blue
    Quantity_Color(0.4, 0.5, 0.4, Quantity_TOC_RGB), // Cool Green
    Quantity_Color(0.6, 0.4, 0.5, Quantity_TOC_RGB), // Cool Rose
};

std::vector<Quantity_Color> STEPColorManager::generateDistinctColors(int componentCount) {
    if (componentCount <= 0) {
        return {};
    }

    // Use the distinct component colors palette
    std::vector<Quantity_Color> colors;
    colors.reserve(componentCount);

    for (int i = 0; i < componentCount; ++i) {
        colors.push_back(DISTINCT_COMPONENT_COLORS[i % DISTINCT_COMPONENT_COLORS.size()]);
    }

    return colors;
}

std::vector<Quantity_Color> STEPColorManager::getPaletteForScheme(GeometryReader::ColorScheme scheme) {
    using CS = GeometryReader::ColorScheme;

    switch (scheme) {
        case CS::WARM_COLORS:
            return WARM_COLORS;
        case CS::RAINBOW:
            return RAINBOW_COLORS;
        case CS::MONOCHROME_BLUE:
            return MONOCHROME_BLUE;
        case CS::MONOCHROME_GREEN:
            return MONOCHROME_GREEN;
        case CS::MONOCHROME_GRAY:
            return MONOCHROME_GRAY;
        case CS::DISTINCT_COLORS:
        default:
            return DISTINCT_COLORS;
    }
}

void STEPColorManager::assignColorsToGeometries(
    std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    const std::string& baseName,
    GeometryReader::ColorScheme scheme,
    bool useConsistentColoring)
{
    if (geometries.empty()) {
        return;
    }

    auto palette = getPaletteForScheme(scheme);
    std::hash<std::string> hasher;

    for (size_t i = 0; i < geometries.size(); ++i) {
        if (!geometries[i]) continue;

        Quantity_Color color;
        if (useConsistentColoring) {
            // Use hash-based consistent coloring
            std::string name = baseName + "_" + std::to_string(i);
            size_t colorIndex = hasher(name) % palette.size();
            color = palette[colorIndex];
        } else {
            // Use sequential coloring
            color = palette[i % palette.size()];
        }

        geometries[i]->setColor(color);
        geometries[i]->setTransparency(0.0);
    }
}

std::unordered_map<std::string, Quantity_Color> STEPColorManager::createColorMapping(
    const std::vector<std::string>& componentNames,
    GeometryReader::ColorScheme scheme)
{
    std::unordered_map<std::string, Quantity_Color> colorMap;
    auto palette = getPaletteForScheme(scheme);
    std::hash<std::string> hasher;

    for (size_t i = 0; i < componentNames.size(); ++i) {
        size_t colorIndex = hasher(componentNames[i]) % palette.size();
        colorMap[componentNames[i]] = palette[colorIndex];
    }

    return colorMap;
}

Quantity_Color STEPColorManager::getDefaultColor() {
    return Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB); // Default gray
}

bool STEPColorManager::isColorDifferentFromDefault(const Quantity_Color& color) {
    const Quantity_Color defaultGray(0.7, 0.7, 0.7, Quantity_TOC_RGB);
    const double colorTolerance = 0.01;

    return (std::abs(color.Red() - defaultGray.Red()) > colorTolerance ||
            std::abs(color.Green() - defaultGray.Green()) > colorTolerance ||
            std::abs(color.Blue() - defaultGray.Blue()) > colorTolerance);
}

