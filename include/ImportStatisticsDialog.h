#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/statbox.h>
#include <wx/notebook.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

struct ImportFileStatistics {
    std::string fileName;
    std::string filePath;
    std::string format;
    bool success;
    std::string errorMessage;
    size_t geometriesCreated;
    std::chrono::milliseconds importTime;
    size_t fileSize;
    std::string status;

    // Detailed STEP processing information
    int transferableRoots;
    int transferredShapes;
    int facesProcessed;
    int facesReversed;
    int solids;
    int shells;
    int faces;
    int wires;
    int edges;
    int vertices;
    bool shapeValid;
    bool shapeClosed;
    int meshVertices;
    int meshTriangles;
    double meshBuildTime;
    double normalCalculationTime;
    double normalSmoothingTime;

    // Material information
    std::string materialDiffuse;
    std::string materialAmbient;
    double materialTransparency;
    bool textureEnabled;
    std::string blendMode;

    ImportFileStatistics()
        : success(false), geometriesCreated(0), fileSize(0),
          transferableRoots(0), transferredShapes(0), facesProcessed(0), facesReversed(0),
          solids(0), shells(0), faces(0), wires(0), edges(0), vertices(0),
          shapeValid(false), shapeClosed(false), meshVertices(0), meshTriangles(0),
          meshBuildTime(0.0), normalCalculationTime(0.0), normalSmoothingTime(0.0),
          materialTransparency(0.0), textureEnabled(false) {}
};

struct ImportFormatStatistics {
    std::string formatName;
    int totalFiles;
    int successfulFiles;
    int failedFiles;
    std::chrono::milliseconds totalImportTime;
    size_t totalGeometries;
    size_t totalFileSize;

    ImportFormatStatistics()
        : totalFiles(0), successfulFiles(0), failedFiles(0), totalGeometries(0), totalFileSize(0) {}
};

struct ImportOverallStatistics {
    int totalFilesSelected;
    int totalFilesProcessed;
    int totalSuccessfulFiles;
    int totalFailedFiles;
    std::chrono::milliseconds totalImportTime;
    std::chrono::milliseconds totalDialogTime;
    size_t totalGeometriesCreated;
    size_t totalFileSize;
    std::vector<ImportFileStatistics> fileStats;
    std::unordered_map<std::string, ImportFormatStatistics> formatStats;

    // Performance and system information
    double averageGeometriesPerSecond;
    double totalGeometryAddTime;
    double totalMeshBuildTime;
    double totalNormalCalculationTime;
    int totalTransferableRoots;
    int totalTransferredShapes;
    int totalFacesProcessed;
    int totalSolids;
    int totalShells;
    int totalFaces;
    int totalWires;
    int totalEdges;
    int totalVertices;
    int totalMeshVertices;
    int totalMeshTriangles;
    bool lodEnabled;
    bool adaptiveMeshingEnabled;
    double meshDeflection;

    ImportOverallStatistics()
        : totalFilesSelected(0), totalFilesProcessed(0), totalSuccessfulFiles(0),
          totalFailedFiles(0), totalGeometriesCreated(0), totalFileSize(0),
          averageGeometriesPerSecond(0.0), totalGeometryAddTime(0.0), totalMeshBuildTime(0.0),
          totalNormalCalculationTime(0.0), totalTransferableRoots(0), totalTransferredShapes(0),
          totalFacesProcessed(0), totalSolids(0), totalShells(0), totalFaces(0),
          totalWires(0), totalEdges(0), totalVertices(0), totalMeshVertices(0),
          totalMeshTriangles(0), lodEnabled(false), adaptiveMeshingEnabled(false),
          meshDeflection(0.0) {}
};

class ImportStatisticsDialog : public FramelessModalPopup
{
public:
    ImportStatisticsDialog(wxWindow* parent, const ImportOverallStatistics& stats);
    virtual ~ImportStatisticsDialog();

private:
    void createControls();
    void layoutControls();
    void populateData();
    void populateDetailsData();
    wxString formatDuration(std::chrono::milliseconds duration) const;
    wxString formatFileSize(size_t bytes) const;
    wxString formatPercentage(int numerator, int denominator) const;
    wxColour getStatusColor(bool success) const;

    // Event handlers
    void onSaveReport(wxCommandEvent& event);
    void onClose(wxCommandEvent& event);
    void onFileItemSelected(wxListEvent& event);

    // Controls
    wxNotebook* m_notebook;

    // Summary page - Table format
    wxPanel* m_summaryPanel;
    wxListCtrl* m_summaryList;

    // Files page
    wxPanel* m_filesPanel;
    wxListCtrl* m_filesList;
    wxTextCtrl* m_fileDetailsText;

    // Formats page
    wxPanel* m_formatsPanel;
    wxListCtrl* m_formatsList;

    // Details page - New page for detailed processing information
    wxPanel* m_detailsPanel;
    wxStaticText* m_detailsTitleText;
    wxTextCtrl* m_detailsTextCtrl;

    // Statistics data
    ImportOverallStatistics m_statistics;

    wxDECLARE_EVENT_TABLE();
};

