#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "GeometryReader.h"

/**
 * @brief Color management utility for STEP files
 *
 * Provides color palette generation, assignment, and management for STEP geometry components.
 */
class STEPColorManager {
public:
    /**
     * @brief Generate distinct colors for assembly components
     * @param componentCount Number of components to color
     * @return Vector of distinct colors
     */
    static std::vector<Quantity_Color> generateDistinctColors(int componentCount);

    /**
     * @brief Get color palette for decomposition scheme
     * @param scheme Color scheme to use
     * @return Vector of colors in the palette
     */
    static std::vector<Quantity_Color> getPaletteForScheme(GeometryReader::ColorScheme scheme);

    /**
     * @brief Assign colors to geometries based on consistent hashing
     * @param geometries Vector of geometries to color
     * @param baseName Base name for consistent coloring
     * @param scheme Color scheme to use
     * @param useConsistentColoring Whether to use consistent coloring
     */
    static void assignColorsToGeometries(
        std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        const std::string& baseName,
        GeometryReader::ColorScheme scheme,
        bool useConsistentColoring = true);

    /**
     * @brief Create color mapping for assembly components
     * @param componentNames Vector of component names
     * @param scheme Color scheme to use
     * @return Map from component name to color
     */
    static std::unordered_map<std::string, Quantity_Color> createColorMapping(
        const std::vector<std::string>& componentNames,
        GeometryReader::ColorScheme scheme);

    /**
     * @brief Get default color for components without explicit colors
     * @return Default gray color
     */
    static Quantity_Color getDefaultColor();

    /**
     * @brief Check if a color is significantly different from default
     * @param color Color to check
     * @return true if color is different from default
     */
    static bool isColorDifferentFromDefault(const Quantity_Color& color);

private:
    // Color palette definitions
    static const std::vector<Quantity_Color> WARM_COLORS;
    static const std::vector<Quantity_Color> RAINBOW_COLORS;
    static const std::vector<Quantity_Color> MONOCHROME_BLUE;
    static const std::vector<Quantity_Color> MONOCHROME_GREEN;
    static const std::vector<Quantity_Color> MONOCHROME_GRAY;
    static const std::vector<Quantity_Color> DISTINCT_COLORS;
    static const std::vector<Quantity_Color> DISTINCT_COMPONENT_COLORS;
};

