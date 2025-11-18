#include "STEPMetadataExtractor.h"
#include "OCCGeometry.h"
#include "STEPColorManager.h"
#include <STEPCAFControl_Reader.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <TDataStd_Name.hxx>
#include <TCollection_AsciiString.hxx>
#include <StepData_StepModel.hxx>
#include <StepRepr_RepresentationItem.hxx>

std::vector<STEPReader::STEPEntityInfo> STEPMetadataExtractor::extractStandardMetadata(
    const STEPControl_Reader& reader)
{
    std::vector<STEPReader::STEPEntityInfo> metadata;

    try {
        // Get the STEP model
        Handle(StepData_StepModel) stepModel = reader.StepModel();
        if (stepModel.IsNull()) {
            return metadata;
        }

        // Extract entity information
        Standard_Integer nbEntities = stepModel->NbEntities();
        metadata.reserve(nbEntities);

        for (Standard_Integer i = 1; i <= nbEntities; i++) {
            Handle(Standard_Transient) entity = stepModel->Entity(i);
            if (!entity.IsNull()) {
                STEPReader::STEPEntityInfo info;
                info.entityId = i;

                // Get entity type name
                info.type = entity->DynamicType()->Name();

                // Try to get name and other metadata
                info.name = extractEntityName(entity);
                info.material = extractEntityMaterial(entity);
                info.description = extractEntityDescription(entity);

                // Try to extract color
                extractColorFromEntity(entity, info);

                metadata.push_back(info);
            }
        }
    }
    catch (const std::exception& e) {
        // Log error but don't crash
        // LOG_ERR_S("Exception extracting standard metadata: " + std::string(e.what()));
    }

    return metadata;
}

std::vector<STEPReader::STEPEntityInfo> STEPMetadataExtractor::extractCAFMetadata(
    const STEPCAFControl_Reader& cafReader)
{
    std::vector<STEPReader::STEPEntityInfo> metadata;

    try {
        // CAF reader provides more advanced metadata extraction
        // This is a placeholder for future CAF-specific extraction
        // For now, return empty vector to indicate CAF should be used directly
    }
    catch (const std::exception& e) {
        // Log error but don't crash
    }

    return metadata;
}

STEPReader::STEPAssemblyInfo STEPMetadataExtractor::buildAssemblyStructure(
    const STEPControl_Reader& reader)
{
    STEPReader::STEPAssemblyInfo assemblyInfo;

    try {
        // Get the STEP model
        Handle(StepData_StepModel) stepModel = reader.StepModel();
        if (stepModel.IsNull()) {
            return assemblyInfo;
        }

        // Set basic assembly info
        assemblyInfo.name = "Root Assembly";
        assemblyInfo.type = "ASSEMBLY";

        // Extract components from transferred shapes
        Standard_Integer nbShapes = reader.NbShapes();
        for (Standard_Integer i = 1; i <= nbShapes; i++) {
            TopoDS_Shape shape = reader.Shape(i);
            if (!shape.IsNull()) {
                STEPReader::STEPEntityInfo component;
                component.name = "Component_" + std::to_string(i);
                component.type = "SHAPE";
                component.entityId = i;

                assemblyInfo.components.push_back(component);
            }
        }
    }
    catch (const std::exception& e) {
        // Log error but don't crash
    }

    return assemblyInfo;
}

STEPReader::STEPEntityInfo STEPMetadataExtractor::extractEntityInfo(
    const STEPControl_Reader& reader,
    int entityId)
{
    STEPReader::STEPEntityInfo info;
    info.entityId = entityId;

    try {
        Handle(StepData_StepModel) stepModel = reader.StepModel();
        if (!stepModel.IsNull() && entityId > 0 && entityId <= stepModel->NbEntities()) {
            Handle(Standard_Transient) entity = stepModel->Entity(entityId);
            if (!entity.IsNull()) {
                info.type = entity->DynamicType()->Name();
                info.name = extractEntityName(entity);
                info.material = extractEntityMaterial(entity);
                info.description = extractEntityDescription(entity);
                extractColorFromEntity(entity, info);
            }
        }
    }
    catch (const std::exception& e) {
        // Log error but don't crash
    }

    return info;
}

void STEPMetadataExtractor::extractColorFromEntity(
    const Handle(Standard_Transient)& entity,
    STEPReader::STEPEntityInfo& info)
{
    // This is a simplified version - in practice, STEP files rarely contain
    // color information in the standard entities. CAF reader is needed for colors.
    info.hasColor = false;
    info.color = STEPColorManager::getDefaultColor();
}

std::string STEPMetadataExtractor::safeConvertExtendedString(
    const TCollection_ExtendedString& extStr)
{
    try {
        // First try direct conversion
        TCollection_AsciiString asciiStr(extStr);
        const char* cStr = asciiStr.ToCString();
        if (cStr != nullptr) {
            std::string result(cStr);
            // Check if the result contains only printable ASCII characters
            bool isValid = true;
            for (char c : result) {
                if (c < 32 || c > 126) { // Not printable ASCII
                    isValid = false;
                    break;
                }
            }
            if (isValid && !result.empty()) {
                return result;
            }
        }
    }
    catch (const std::exception& e) {
        // Log error but continue
    }

    // Fallback: convert character by character, keeping only ASCII
    std::string result;
    const Standard_ExtString extCStr = extStr.ToExtString();
    if (extCStr != nullptr) {
        for (int i = 0; extCStr[i] != 0; i++) {
            wchar_t wc = extCStr[i];
            if (wc >= 32 && wc <= 126) { // Printable ASCII range
                result += static_cast<char>(wc);
            }
        }
    }

    return result.empty() ? "UnnamedComponent" : result;
}

bool STEPMetadataExtractor::hasValidColorInfo(
    const std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    if (geometries.empty()) return false;

    // Default gray color that OCCT uses when no color is specified
    const Quantity_Color defaultGray = STEPColorManager::getDefaultColor();
    const double colorTolerance = 0.01; // Small tolerance for floating point comparison

    // Check if any geometry has a non-default color
    int nonDefaultColorCount = 0;
    for (const auto& geom : geometries) {
        if (!geom) continue;
        Quantity_Color color = geom->getColor();

        if (STEPColorManager::isColorDifferentFromDefault(color)) {
            nonDefaultColorCount++;
        }
    }

    // Consider CAF valid if at least one component has a non-default color
    return nonDefaultColorCount > 0;
}

std::string STEPMetadataExtractor::extractEntityName(const Handle(Standard_Transient)& entity)
{
    try {
        // Try to get name if available
        Handle(StepRepr_RepresentationItem) reprItem =
            Handle(StepRepr_RepresentationItem)::DownCast(entity);
        if (!reprItem.IsNull()) {
            Handle(TCollection_HAsciiString) name = reprItem->Name();
            if (!name.IsNull()) {
                return safeConvertExtendedString(name->String());
            }
        }
    }
    catch (const std::exception& e) {
        // Return empty string on error
    }
    return "";
}

std::string STEPMetadataExtractor::extractEntityMaterial(const Handle(Standard_Transient)& entity)
{
    // STEP standard doesn't typically include material information
    // This would require CAF reader with material support
    return "";
}

std::string STEPMetadataExtractor::extractEntityDescription(const Handle(Standard_Transient)& entity)
{
    // STEP entities don't typically have descriptions
    return "";
}

void STEPMetadataExtractor::processComponent(const TopoDS_Shape& shape, const std::string& componentName,
	int componentIndex, std::vector<std::shared_ptr<OCCGeometry>>& geometries,
	std::vector<STEPReader::STEPEntityInfo>& entityMetadata) {

	// Generate distinct colors for components (cool tones and muted)
	static std::vector<Quantity_Color> distinctColors = {
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

	// Use distinct color for each component
	Quantity_Color color = distinctColors[componentIndex % distinctColors.size()];

	// Create geometry object
	auto geometry = std::make_shared<OCCGeometry>(componentName);
	geometry->setShape(shape);
	geometry->setColor(color);
	geometry->setTransparency(0.0);

	// Create entity info
	STEPReader::STEPEntityInfo entityInfo;
	entityInfo.name = componentName;
	entityInfo.type = "COMPONENT";
	entityInfo.color = color;
	entityInfo.hasColor = true;
	entityInfo.entityId = componentIndex;
	entityInfo.shapeIndex = componentIndex;

	geometries.push_back(geometry);
	entityMetadata.push_back(entityInfo);

	// Log component creation (using standard logging)
	std::cout << "Created colored component: " << componentName <<
		" (R=" << color.Red() <<
		" G=" << color.Green() <<
		" B=" << color.Blue() << ")" << std::endl;
}
