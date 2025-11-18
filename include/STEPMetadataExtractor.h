#pragma once

#include <vector>
#include <string>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "STEPReader.h"

// Forward declarations
class STEPControl_Reader;
class STEPCAFControl_Reader;

/**
 * @brief Metadata extraction utility for STEP files
 *
 * Provides functionality to extract colors, materials, names, and other metadata
 * from STEP files using both standard and CAF (XCAF) readers.
 */
class STEPMetadataExtractor {
public:
    /**
     * @brief Extract metadata using standard STEP reader
     * @param reader The STEP reader instance
     * @return Vector of entity metadata
     */
    static std::vector<STEPReader::STEPEntityInfo> extractStandardMetadata(
        const STEPControl_Reader& reader);

    /**
     * @brief Extract metadata using CAF reader (advanced features)
     * @param cafReader The CAF reader instance
     * @return Vector of entity metadata with colors
     */
    static std::vector<STEPReader::STEPEntityInfo> extractCAFMetadata(
        const STEPCAFControl_Reader& cafReader);

    /**
     * @brief Build assembly structure from STEP file
     * @param reader The STEP reader instance
     * @return Assembly structure information
     */
    static STEPReader::STEPAssemblyInfo buildAssemblyStructure(
        const STEPControl_Reader& reader);

    /**
     * @brief Extract entity information from STEP model
     * @param reader The STEP reader instance
     * @param entityId Entity ID in STEP file
     * @return Entity information
     */
    static STEPReader::STEPEntityInfo extractEntityInfo(
        const STEPControl_Reader& reader,
        int entityId);

    /**
     * @brief Extract color information from STEP entity
     * @param entity The STEP entity
     * @param info Entity info to update with color
     */
    static void extractColorFromEntity(
        const Handle(Standard_Transient)& entity,
        STEPReader::STEPEntityInfo& info);

    /**
     * @brief Safely convert ExtendedString to std::string
     * @param extStr ExtendedString to convert
     * @return Converted string
     */
    static std::string safeConvertExtendedString(
        const TCollection_ExtendedString& extStr);

    /**
     * @brief Check if CAF results contain valid color information
     * @param geometries Vector of geometries to check
     * @return true if valid colors are found
     */
    static bool hasValidColorInfo(
        const std::vector<std::shared_ptr<OCCGeometry>>& geometries);

private:
    /**
     * @brief Extract name from STEP entity
     * @param entity The STEP entity
     * @return Entity name or empty string
     */
    static std::string extractEntityName(const Handle(Standard_Transient)& entity);

    /**
     * @brief Extract material information from STEP entity
     * @param entity The STEP entity
     * @return Material name or empty string
     */
    static std::string extractEntityMaterial(const Handle(Standard_Transient)& entity);

    /**
     * @brief Extract description from STEP entity
     * @param entity The STEP entity
     * @return Description or empty string
     */
    static std::string extractEntityDescription(const Handle(Standard_Transient)& entity);
    static void processComponent(const TopoDS_Shape& shape, const std::string& componentName,
        int componentIndex, std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        std::vector<STEPReader::STEPEntityInfo>& entityMetadata);
};
