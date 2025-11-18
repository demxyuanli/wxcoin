#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TDF_Label.hxx>
#include <OpenCASCADE/TopLoc_Location.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "STEPReader.h"
#include "GeometryReader.h"

// Forward declarations
class STEPCAFControl_Reader;
class TDocStd_Document;
class XCAFDoc_ShapeTool;
class XCAFDoc_ColorTool;

/**
 * @brief CAF (XCAF) processor for STEP files with advanced features
 *
 * Handles STEP files with color, assembly, and material information using
 * OpenCASCADE's XCAF (Extended CAD Framework) functionality.
 */
class STEPCAFProcessor {
public:
    using ProgressCallback = std::function<void(int /*percent*/, const std::string& /*stage*/)>;

    /**
     * @brief Process STEP file with CAF reader
     * @param filePath Path to the STEP file
     * @param options Optimization options
     * @param progress Progress callback
     * @return ReadResult containing processed data
     */
    static STEPReader::ReadResult processSTEPFileWithCAF(
        const std::string& filePath,
        const GeometryReader::OptimizationOptions& options,
        ProgressCallback progress = nullptr);

private:
    /**
     * @brief Initialize CAF reader and document
     * @param filePath Path to the STEP file
     * @param doc Output document
     * @param cafReader Output CAF reader
     * @param errorMessage Output error message if initialization fails
     * @return true if initialization successful
     */
    static bool initializeCAFReader(
        const std::string& filePath,
        Handle(TDocStd_Document)& doc,
        STEPCAFControl_Reader& cafReader,
        std::string& errorMessage);

    /**
     * @brief Read and transfer STEP file with CAF reader
     * @param filePath Path to the STEP file
     * @param cafReader CAF reader instance
     * @param doc Document to transfer to
     * @param progress Progress callback
     * @param errorMessage Output error message if reading fails
     * @return true if reading and transfer successful
     */
    static bool readAndTransferCAF(
        const std::string& filePath,
        STEPCAFControl_Reader& cafReader,
        Handle(TDocStd_Document)& doc,
        ProgressCallback progress,
        std::string& errorMessage);

    /**
     * @brief Process assembly tree and extract components
     * @param shapeTool Shape tool from CAF document
     * @param colorTool Color tool from CAF document
     * @param baseName Base name for components
     * @param options Optimization options
     * @param geometries Output geometries
     * @param entityMetadata Output entity metadata
     * @param componentIndex Starting component index
     * @return Updated component index
     */
    static int processAssemblyTree(
        const Handle(XCAFDoc_ShapeTool)& shapeTool,
        const Handle(XCAFDoc_ColorTool)& colorTool,
        const std::string& baseName,
        const GeometryReader::OptimizationOptions& options,
        std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        std::vector<STEPReader::STEPEntityInfo>& entityMetadata,
        int componentIndex);

    /**
     * @brief Process individual label in assembly tree
     * @param label Current label to process
     * @param parentLoc Parent location
     * @param level Current assembly level
     * @param shapeTool Shape tool
     * @param colorTool Color tool
     * @param baseName Base name
     * @param options Optimization options
     * @param makeColorForName Color assignment function
     * @param geometries Output geometries
     * @param entityMetadata Output metadata
     * @param componentIndex Current component index
     * @return Updated component index
     */
    static int processLabel(
        const TDF_Label& label,
        const TopLoc_Location& parentLoc,
        int level,
        const Handle(XCAFDoc_ShapeTool)& shapeTool,
        const Handle(XCAFDoc_ColorTool)& colorTool,
        const std::string& baseName,
        const GeometryReader::OptimizationOptions& options,
        const std::function<Quantity_Color(const std::string&, const Quantity_Color*)>& makeColorForName,
        std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        std::vector<STEPReader::STEPEntityInfo>& entityMetadata,
        int& componentIndex);

    /**
     * @brief Extract and decompose shapes from label
     * @param located Located shape
     * @param compName Component name
     * @param options Decomposition options
     * @return Vector of decomposed shapes
     */
    static std::vector<TopoDS_Shape> extractAndDecomposeShapes(
        const TopoDS_Shape& located,
        const std::string& compName,
        const GeometryReader::OptimizationOptions& options);

    /**
     * @brief Create geometries from shape parts
     * @param parts Shape parts
     * @param compName Component name
     * @param hasCafColor Whether CAF color is available
     * @param cafColor CAF color
     * @param level Assembly level
     * @param baseName Base name
     * @param options Options
     * @param makeColorForName Color assignment function
     * @param geometries Output geometries
     * @param entityMetadata Output metadata
     * @param componentIndex Starting component index
     * @return Updated component index
     */
    static int createGeometriesFromParts(
        const std::vector<TopoDS_Shape>& parts,
        const std::string& compName,
        bool hasCafColor,
        const Quantity_Color& cafColor,
        int level,
        const std::string& baseName,
        const GeometryReader::OptimizationOptions& options,
        const std::function<Quantity_Color(const std::string&, const Quantity_Color*)>& makeColorForName,
        std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        std::vector<STEPReader::STEPEntityInfo>& entityMetadata,
        int componentIndex);

    /**
     * @brief Detect if a shape is a shell model
     * @param shape Shape to check
     * @return true if shape is a shell model
     */
    static bool detectShellModel(const TopoDS_Shape& shape);
};
