#include "ImportStatisticsDialog.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/clipbrd.h>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

wxBEGIN_EVENT_TABLE(ImportStatisticsDialog, wxDialog)
    EVT_BUTTON(wxID_SAVE, ImportStatisticsDialog::onSaveReport)
    EVT_BUTTON(wxID_CLOSE, ImportStatisticsDialog::onClose)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, ImportStatisticsDialog::onFileItemSelected)
wxEND_EVENT_TABLE()

ImportStatisticsDialog::ImportStatisticsDialog(wxWindow* parent, const ImportOverallStatistics& stats)
    : wxDialog(parent, wxID_ANY, "Import Statistics Report",
               wxDefaultPosition, wxSize(800, 600),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_statistics(stats)
{
    createControls();
    layoutControls();
    populateData();
    Centre();
}

ImportStatisticsDialog::~ImportStatisticsDialog()
{
}

void ImportStatisticsDialog::createControls()
{
    // Create notebook
    m_notebook = new wxNotebook(this, wxID_ANY);

    // Summary page
    m_summaryPanel = new wxPanel(m_notebook, wxID_ANY);
    m_totalFilesText = new wxStaticText(m_summaryPanel, wxID_ANY, "");
    m_successfulFilesText = new wxStaticText(m_summaryPanel, wxID_ANY, "");
    m_failedFilesText = new wxStaticText(m_summaryPanel, wxID_ANY, "");
    m_totalGeometriesText = new wxStaticText(m_summaryPanel, wxID_ANY, "");
    m_totalSizeText = new wxStaticText(m_summaryPanel, wxID_ANY, "");
    m_totalTimeText = new wxStaticText(m_summaryPanel, wxID_ANY, "");
    m_averageTimeText = new wxStaticText(m_summaryPanel, wxID_ANY, "");
    m_successRateText = new wxStaticText(m_summaryPanel, wxID_ANY, "");

    // Files page
    m_filesPanel = new wxPanel(m_notebook, wxID_ANY);
    m_filesList = new wxListCtrl(m_filesPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    m_fileDetailsText = new wxTextCtrl(m_filesPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY | wxBORDER_SUNKEN);

    // Formats page
    m_formatsPanel = new wxPanel(m_notebook, wxID_ANY);
    m_formatsList = new wxListCtrl(m_formatsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxBORDER_SUNKEN);

    // Details page - New page for detailed processing information
    m_detailsPanel = new wxPanel(m_notebook, wxID_ANY);
    m_detailsTitleText = new wxStaticText(m_detailsPanel, wxID_ANY, "Detailed Processing Information");
    m_detailsTextCtrl = new wxTextCtrl(m_detailsPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxBORDER_SUNKEN);

    // Buttons will be created in layoutControls

    // Set up list controls
    // Files list
    m_filesList->InsertColumn(0, "File Name", wxLIST_FORMAT_LEFT, 200);
    m_filesList->InsertColumn(1, "Format", wxLIST_FORMAT_LEFT, 80);
    m_filesList->InsertColumn(2, "Status", wxLIST_FORMAT_LEFT, 80);
    m_filesList->InsertColumn(3, "Size", wxLIST_FORMAT_LEFT, 80);
    m_filesList->InsertColumn(4, "Time", wxLIST_FORMAT_LEFT, 80);
    m_filesList->InsertColumn(5, "Geometries", wxLIST_FORMAT_LEFT, 80);

    // Formats list
    m_formatsList->InsertColumn(0, "Format", wxLIST_FORMAT_LEFT, 100);
    m_formatsList->InsertColumn(1, "Total Files", wxLIST_FORMAT_LEFT, 100);
    m_formatsList->InsertColumn(2, "Successful", wxLIST_FORMAT_LEFT, 100);
    m_formatsList->InsertColumn(3, "Failed", wxLIST_FORMAT_LEFT, 100);
    m_formatsList->InsertColumn(4, "Success Rate", wxLIST_FORMAT_LEFT, 120);
    m_formatsList->InsertColumn(5, "Total Time", wxLIST_FORMAT_LEFT, 120);
}

void ImportStatisticsDialog::layoutControls()
{
    // Summary page layout
    wxBoxSizer* summarySizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer* summaryGrid = new wxGridSizer(2, 10, 10);

    wxStaticBoxSizer* summaryBox = new wxStaticBoxSizer(wxVERTICAL, m_summaryPanel, "Import Summary");

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Total Files Selected:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_totalFilesText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Successful Files:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_successfulFilesText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Failed Files:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_failedFilesText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Total Geometries:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_totalGeometriesText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Total Size:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_totalSizeText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Total Import Time:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_totalTimeText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Average Time per File:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_averageTimeText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryGrid->Add(new wxStaticText(m_summaryPanel, wxID_ANY, "Success Rate:"), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    summaryGrid->Add(m_successRateText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    summaryBox->Add(summaryGrid, 1, wxEXPAND | wxALL, 10);
    summarySizer->Add(summaryBox, 1, wxEXPAND | wxALL, 10);
    m_summaryPanel->SetSizer(summarySizer);

    // Files page layout
    wxBoxSizer* filesSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* filesContentSizer = new wxBoxSizer(wxVERTICAL);

    filesContentSizer->Add(m_filesList, 1, wxEXPAND | wxALL, 5);
    filesContentSizer->Add(new wxStaticText(m_filesPanel, wxID_ANY, "File Details:"), 0, wxLEFT | wxRIGHT | wxTOP, 5);
    filesContentSizer->Add(m_fileDetailsText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    filesSizer->Add(filesContentSizer, 1, wxEXPAND);
    m_filesPanel->SetSizer(filesSizer);

    // Formats page layout
    wxBoxSizer* formatsSizer = new wxBoxSizer(wxVERTICAL);
    formatsSizer->Add(m_formatsList, 1, wxEXPAND | wxALL, 5);
    m_formatsPanel->SetSizer(formatsSizer);

    // Details page layout
    wxBoxSizer* detailsSizer = new wxBoxSizer(wxVERTICAL);
    detailsSizer->Add(m_detailsTitleText, 0, wxALL, 5);
    detailsSizer->Add(m_detailsTextCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    m_detailsPanel->SetSizer(detailsSizer);

    // Add pages to notebook
    m_notebook->AddPage(m_summaryPanel, "Summary");
    m_notebook->AddPage(m_filesPanel, "Files");
    m_notebook->AddPage(m_formatsPanel, "Formats");
    m_notebook->AddPage(m_detailsPanel, "Details");

    // Create buttons
    wxButton* saveButton = new wxButton(this, wxID_SAVE, "Save Report");
    wxButton* closeButton = new wxButton(this, wxID_CLOSE, "Close");

    // Main layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(saveButton, 0, wxRIGHT, 5);
    buttonSizer->Add(closeButton, 0);

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    SetSizer(mainSizer);
}

void ImportStatisticsDialog::populateData()
{
    // Populate summary data
    m_totalFilesText->SetLabel(wxString::Format("%d", m_statistics.totalFilesSelected));
    m_successfulFilesText->SetLabel(wxString::Format("%d", m_statistics.totalSuccessfulFiles));
    m_failedFilesText->SetLabel(wxString::Format("%d", m_statistics.totalFailedFiles));
    m_totalGeometriesText->SetLabel(wxString::Format("%zu", m_statistics.totalGeometriesCreated));
    m_totalSizeText->SetLabel(formatFileSize(m_statistics.totalFileSize));
    m_totalTimeText->SetLabel(formatDuration(m_statistics.totalImportTime));

    if (m_statistics.totalFilesProcessed > 0) {
        auto avgTime = m_statistics.totalImportTime / m_statistics.totalFilesProcessed;
        m_averageTimeText->SetLabel(formatDuration(avgTime));
        m_successRateText->SetLabel(formatPercentage(m_statistics.totalSuccessfulFiles, m_statistics.totalFilesProcessed) + "%");
    } else {
        m_averageTimeText->SetLabel("N/A");
        m_successRateText->SetLabel("N/A");
    }

    // Populate files list
    for (size_t i = 0; i < m_statistics.fileStats.size(); ++i) {
        const auto& fileStat = m_statistics.fileStats[i];

        wxFileName fileName(fileStat.filePath);
        wxString displayName = fileName.GetFullName();

        long itemIndex = m_filesList->InsertItem(i, displayName);
        m_filesList->SetItem(itemIndex, 1, fileStat.format);
        m_filesList->SetItem(itemIndex, 2, fileStat.success ? "Success" : "Failed");
        m_filesList->SetItem(itemIndex, 3, formatFileSize(fileStat.fileSize));
        m_filesList->SetItem(itemIndex, 4, formatDuration(fileStat.importTime));
        m_filesList->SetItem(itemIndex, 5, wxString::Format("%zu", fileStat.geometriesCreated));

        // Set item color based on success
        wxColour statusColor = getStatusColor(fileStat.success);
        m_filesList->SetItemTextColour(itemIndex, statusColor);
    }

    // Populate formats list
    int formatIndex = 0;
    for (const auto& formatPair : m_statistics.formatStats) {
        const auto& formatStat = formatPair.second;

        long itemIndex = m_formatsList->InsertItem(formatIndex, formatPair.first);
        m_formatsList->SetItem(itemIndex, 1, wxString::Format("%d", formatStat.totalFiles));
        m_formatsList->SetItem(itemIndex, 2, wxString::Format("%d", formatStat.successfulFiles));
        m_formatsList->SetItem(itemIndex, 3, wxString::Format("%d", formatStat.failedFiles));
        m_formatsList->SetItem(itemIndex, 4, formatPercentage(formatStat.successfulFiles, formatStat.totalFiles) + "%");
        m_formatsList->SetItem(itemIndex, 5, formatDuration(formatStat.totalImportTime));

        formatIndex++;
    }

    // Populate details page
    populateDetailsData();
}

void ImportStatisticsDialog::populateDetailsData()
{
    wxString detailsText;

    // Performance and system information
    detailsText << "=== PERFORMANCE & SYSTEM INFORMATION ===\n\n";
    detailsText << wxString::Format("Performance: %.2f geometries/second", m_statistics.averageGeometriesPerSecond) << "\n";
    detailsText << wxString::Format("Total geometry add time: %.2f ms", m_statistics.totalGeometryAddTime) << "\n";
    detailsText << wxString::Format("Total mesh build time: %.2f ms", m_statistics.totalMeshBuildTime) << "\n";
    detailsText << wxString::Format("Total normal calculation time: %.2f ms", m_statistics.totalNormalCalculationTime) << "\n";
    detailsText << wxString::Format("LOD enabled: %s", m_statistics.lodEnabled ? "Yes" : "No") << "\n";
    detailsText << wxString::Format("Adaptive meshing enabled: %s", m_statistics.adaptiveMeshingEnabled ? "Yes" : "No") << "\n";
    detailsText << wxString::Format("Mesh deflection: %.6f", m_statistics.meshDeflection) << "\n";
    detailsText << "\n";

    // Topology statistics
    detailsText << "=== TOPOLOGY STATISTICS ===\n\n";
    detailsText << wxString::Format("Total transferable roots: %d", m_statistics.totalTransferableRoots) << "\n";
    detailsText << wxString::Format("Total transferred shapes: %d", m_statistics.totalTransferredShapes) << "\n";
    detailsText << wxString::Format("Total faces processed: %d", m_statistics.totalFacesProcessed) << "\n";
    detailsText << wxString::Format("Total solids: %d", m_statistics.totalSolids) << "\n";
    detailsText << wxString::Format("Total shells: %d", m_statistics.totalShells) << "\n";
    detailsText << wxString::Format("Total faces: %d", m_statistics.totalFaces) << "\n";
    detailsText << wxString::Format("Total wires: %d", m_statistics.totalWires) << "\n";
    detailsText << wxString::Format("Total edges: %d", m_statistics.totalEdges) << "\n";
    detailsText << wxString::Format("Total vertices: %d", m_statistics.totalVertices) << "\n";
    detailsText << "\n";

    // Mesh statistics
    detailsText << "=== MESH STATISTICS ===\n\n";
    detailsText << wxString::Format("Total mesh vertices: %d", m_statistics.totalMeshVertices) << "\n";
    detailsText << wxString::Format("Total mesh triangles: %d", m_statistics.totalMeshTriangles) << "\n";
    detailsText << "\n";

    // Detailed file information
    detailsText << "=== DETAILED FILE PROCESSING INFORMATION ===\n\n";

    for (size_t i = 0; i < m_statistics.fileStats.size(); ++i) {
        const auto& fileStat = m_statistics.fileStats[i];

        detailsText << wxString::Format("File %zu: %s", i + 1, fileStat.fileName) << "\n";
        detailsText << wxString::Format("  Path: %s", fileStat.filePath) << "\n";
        detailsText << wxString::Format("  Format: %s", fileStat.format) << "\n";
        detailsText << wxString::Format("  Status: %s", fileStat.success ? "SUCCESS" : "FAILED") << "\n";

        if (!fileStat.success && !fileStat.errorMessage.empty()) {
            detailsText << wxString::Format("  Error: %s", fileStat.errorMessage) << "\n";
        }

        detailsText << wxString::Format("  File size: %s", formatFileSize(fileStat.fileSize)) << "\n";
        detailsText << wxString::Format("  Import time: %s", formatDuration(fileStat.importTime)) << "\n";
        detailsText << wxString::Format("  Geometries created: %zu", fileStat.geometriesCreated) << "\n";

        // STEP processing details
        if (fileStat.transferableRoots > 0) {
            detailsText << "  STEP Processing:\n";
            detailsText << wxString::Format("    Transferable roots: %d", fileStat.transferableRoots) << "\n";
            detailsText << wxString::Format("    Transferred shapes: %d", fileStat.transferredShapes) << "\n";
            detailsText << wxString::Format("    Faces processed: %d", fileStat.facesProcessed) << "\n";
            detailsText << wxString::Format("    Faces reversed: %d", fileStat.facesReversed) << "\n";
        }

        // Topology details
        if (fileStat.solids > 0 || fileStat.faces > 0) {
            detailsText << "  Topology:\n";
            detailsText << wxString::Format("    Solids: %d", fileStat.solids) << "\n";
            detailsText << wxString::Format("    Shells: %d", fileStat.shells) << "\n";
            detailsText << wxString::Format("    Faces: %d", fileStat.faces) << "\n";
            detailsText << wxString::Format("    Wires: %d", fileStat.wires) << "\n";
            detailsText << wxString::Format("    Edges: %d", fileStat.edges) << "\n";
            detailsText << wxString::Format("    Vertices: %d", fileStat.vertices) << "\n";
            detailsText << wxString::Format("    Shape validity: %s", fileStat.shapeValid ? "VALID" : "INVALID") << "\n";
            detailsText << wxString::Format("    Shape closure: %s", fileStat.shapeClosed ? "CLOSED" : "OPEN") << "\n";
        }

        // Mesh details
        if (fileStat.meshVertices > 0) {
            detailsText << "  Mesh:\n";
            detailsText << wxString::Format("    Vertices: %d", fileStat.meshVertices) << "\n";
            detailsText << wxString::Format("    Triangles: %d", fileStat.meshTriangles) << "\n";
            detailsText << wxString::Format("    Build time: %.2f ms", fileStat.meshBuildTime) << "\n";
            detailsText << wxString::Format("    Normal calculation: %.2f ms", fileStat.normalCalculationTime) << "\n";
            detailsText << wxString::Format("    Normal smoothing: %.2f ms", fileStat.normalSmoothingTime) << "\n";
        }

        // Material details
        if (!fileStat.materialDiffuse.empty()) {
            detailsText << "  Material:\n";
            detailsText << wxString::Format("    Diffuse: %s", fileStat.materialDiffuse) << "\n";
            detailsText << wxString::Format("    Ambient: %s", fileStat.materialAmbient) << "\n";
            detailsText << wxString::Format("    Transparency: %.3f", fileStat.materialTransparency) << "\n";
            detailsText << wxString::Format("    Texture enabled: %s", fileStat.textureEnabled ? "Yes" : "No") << "\n";
            detailsText << wxString::Format("    Blend mode: %s", fileStat.blendMode) << "\n";
        }

        detailsText << "\n";
    }

    m_detailsTextCtrl->SetValue(detailsText);
}

wxString ImportStatisticsDialog::formatDuration(std::chrono::milliseconds duration) const
{
    auto ms = duration.count();
    if (ms < 1000) {
        return wxString::Format("%lld ms", ms);
    } else if (ms < 60000) {
        return wxString::Format("%.2f s", ms / 1000.0);
    } else {
        auto minutes = ms / 60000;
        auto seconds = (ms % 60000) / 1000.0;
        return wxString::Format("%lld m %.1f s", minutes, seconds);
    }
}

wxString ImportStatisticsDialog::formatFileSize(size_t bytes) const
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    return wxString::Format("%.2f %s", size, units[unitIndex]);
}

wxString ImportStatisticsDialog::formatPercentage(int numerator, int denominator) const
{
    if (denominator == 0) return "0.00";
    return wxString::Format("%.2f", (static_cast<double>(numerator) / denominator) * 100.0);
}

wxColour ImportStatisticsDialog::getStatusColor(bool success) const
{
    return success ? wxColour(0, 128, 0) : wxColour(128, 0, 0); // Green for success, red for failure
}

void ImportStatisticsDialog::onSaveReport(wxCommandEvent& event)
{
    wxFileDialog saveDialog(this, "Save Import Report", "", "import_report.txt",
                           "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveDialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    wxString filePath = saveDialog.GetPath();
    wxFile file(filePath, wxFile::write);

    if (!file.IsOpened()) {
        wxMessageBox("Failed to save report file!", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Write report header
    file.Write("=== GEOMETRY IMPORT STATISTICS REPORT ===\n\n");
    file.Write(wxString::Format("Report generated: %s", wxDateTime::Now().Format()) << "\n\n");

    // Write summary
    file.Write("=== SUMMARY ===\n");
    file.Write(wxString::Format("Total files selected: %d", m_statistics.totalFilesSelected) << "\n");
    file.Write(wxString::Format("Files processed: %d", m_statistics.totalFilesProcessed) << "\n");
    file.Write(wxString::Format("Successful files: %d", m_statistics.totalSuccessfulFiles) << "\n");
    file.Write(wxString::Format("Failed files: %d", m_statistics.totalFailedFiles) << "\n");
    file.Write(wxString::Format("Total geometries created: %zu", m_statistics.totalGeometriesCreated) << "\n");
    file.Write(wxString::Format("Total file size: %s", formatFileSize(m_statistics.totalFileSize)) << "\n");
    file.Write(wxString::Format("Total import time: %s", formatDuration(m_statistics.totalImportTime)) << "\n");
    file.Write(wxString::Format("Dialog selection time: %s", formatDuration(m_statistics.totalDialogTime)) << "\n");

    if (m_statistics.totalFilesProcessed > 0) {
        auto avgTime = m_statistics.totalImportTime / m_statistics.totalFilesProcessed;
        file.Write(wxString::Format("Average time per file: %s", formatDuration(avgTime)) << "\n");
        file.Write(wxString::Format("Success rate: %s%%", formatPercentage(m_statistics.totalSuccessfulFiles, m_statistics.totalFilesProcessed)) << "\n");
        file.Write(wxString::Format("Performance: %.2f geometries/second", m_statistics.averageGeometriesPerSecond) << "\n");
    }

    // Performance and system information
    file.Write("\n=== PERFORMANCE & SYSTEM INFORMATION ===\n");
    file.Write(wxString::Format("Total geometry add time: %.2f ms", m_statistics.totalGeometryAddTime) << "\n");
    file.Write(wxString::Format("Total mesh build time: %.2f ms", m_statistics.totalMeshBuildTime) << "\n");
    file.Write(wxString::Format("Total normal calculation time: %.2f ms", m_statistics.totalNormalCalculationTime) << "\n");
    file.Write(wxString::Format("LOD enabled: %s", m_statistics.lodEnabled ? "Yes" : "No") << "\n");
    file.Write(wxString::Format("Adaptive meshing enabled: %s", m_statistics.adaptiveMeshingEnabled ? "Yes" : "No") << "\n");
    file.Write(wxString::Format("Mesh deflection: %.6f", m_statistics.meshDeflection) << "\n");

    // Topology statistics
    file.Write("\n=== TOPOLOGY STATISTICS ===\n");
    file.Write(wxString::Format("Total transferable roots: %d", m_statistics.totalTransferableRoots) << "\n");
    file.Write(wxString::Format("Total transferred shapes: %d", m_statistics.totalTransferredShapes) << "\n");
    file.Write(wxString::Format("Total faces processed: %d", m_statistics.totalFacesProcessed) << "\n");
    file.Write(wxString::Format("Total solids: %d", m_statistics.totalSolids) << "\n");
    file.Write(wxString::Format("Total shells: %d", m_statistics.totalShells) << "\n");
    file.Write(wxString::Format("Total faces: %d", m_statistics.totalFaces) << "\n");
    file.Write(wxString::Format("Total wires: %d", m_statistics.totalWires) << "\n");
    file.Write(wxString::Format("Total edges: %d", m_statistics.totalEdges) << "\n");
    file.Write(wxString::Format("Total vertices: %d", m_statistics.totalVertices) << "\n");

    // Mesh statistics
    file.Write("\n=== MESH STATISTICS ===\n");
    file.Write(wxString::Format("Total mesh vertices: %d", m_statistics.totalMeshVertices) << "\n");
    file.Write(wxString::Format("Total mesh triangles: %d", m_statistics.totalMeshTriangles) << "\n");

    file.Write("\n");

    // Write format statistics
    file.Write("=== FORMAT STATISTICS ===\n");
    for (const auto& formatPair : m_statistics.formatStats) {
        const auto& formatStat = formatPair.second;
        file.Write(wxString::Format("Format: %s", formatPair.first) << "\n");
        file.Write(wxString::Format("  Total files: %d", formatStat.totalFiles) << "\n");
        file.Write(wxString::Format("  Successful: %d", formatStat.successfulFiles) << "\n");
        file.Write(wxString::Format("  Failed: %d", formatStat.failedFiles) << "\n");
        file.Write(wxString::Format("  Success rate: %s%%", formatPercentage(formatStat.successfulFiles, formatStat.totalFiles)) << "\n");
        file.Write(wxString::Format("  Total time: %s", formatDuration(formatStat.totalImportTime)) << "\n");
        file.Write(wxString::Format("  Total geometries: %zu", formatStat.totalGeometries) << "\n");
        file.Write(wxString::Format("  Total size: %s", formatFileSize(formatStat.totalFileSize)) << "\n");
        file.Write("\n");
    }

    // Write detailed file statistics
    file.Write("=== FILE DETAILS ===\n");
    for (const auto& fileStat : m_statistics.fileStats) {
        wxFileName fileName(fileStat.filePath);
        file.Write(wxString::Format("File: %s", fileName.GetFullName()) << "\n");
        file.Write(wxString::Format("  Path: %s", fileStat.filePath) << "\n");
        file.Write(wxString::Format("  Format: %s", fileStat.format) << "\n");
        file.Write(wxString::Format("  Status: %s", fileStat.success ? "SUCCESS" : "FAILED") << "\n");
        if (!fileStat.success && !fileStat.errorMessage.empty()) {
            file.Write(wxString::Format("  Error: %s", fileStat.errorMessage) << "\n");
        }
        file.Write(wxString::Format("  File size: %s", formatFileSize(fileStat.fileSize)) << "\n");
        file.Write(wxString::Format("  Import time: %s", formatDuration(fileStat.importTime)) << "\n");
        file.Write(wxString::Format("  Geometries created: %zu", fileStat.geometriesCreated) << "\n");
        file.Write("\n");
    }

    file.Close();
    wxMessageBox("Report saved successfully!", "Success", wxOK | wxICON_INFORMATION);
}

void ImportStatisticsDialog::onClose(wxCommandEvent& event)
{
    EndModal(wxID_CLOSE);
}

void ImportStatisticsDialog::onFileItemSelected(wxListEvent& event)
{
    long itemIndex = event.GetIndex();
    if (itemIndex >= 0 && itemIndex < static_cast<long>(m_statistics.fileStats.size())) {
        const auto& fileStat = m_statistics.fileStats[itemIndex];

        wxString details;
        details << "File: " << fileStat.fileName << "\n";
        details << "Path: " << fileStat.filePath << "\n";
        details << "Format: " << fileStat.format << "\n";
        details << "Status: " << (fileStat.success ? "SUCCESS" : "FAILED") << "\n";

        if (!fileStat.success && !fileStat.errorMessage.empty()) {
            details << "Error: " << fileStat.errorMessage << "\n";
        }

        details << "File Size: " << formatFileSize(fileStat.fileSize) << "\n";
        details << "Import Time: " << formatDuration(fileStat.importTime) << "\n";
        details << "Geometries Created: " << fileStat.geometriesCreated << "\n";

        // STEP processing details
        if (fileStat.transferableRoots > 0) {
            details << "\nSTEP Processing:\n";
            details << "  Transferable roots: " << fileStat.transferableRoots << "\n";
            details << "  Transferred shapes: " << fileStat.transferredShapes << "\n";
            details << "  Faces processed: " << fileStat.facesProcessed << "\n";
            details << "  Faces reversed: " << fileStat.facesReversed << "\n";
        }

        // Topology details
        if (fileStat.solids > 0 || fileStat.faces > 0) {
            details << "\nTopology Analysis:\n";
            details << "  Solids: " << fileStat.solids << "\n";
            details << "  Shells: " << fileStat.shells << "\n";
            details << "  Faces: " << fileStat.faces << "\n";
            details << "  Wires: " << fileStat.wires << "\n";
            details << "  Edges: " << fileStat.edges << "\n";
            details << "  Vertices: " << fileStat.vertices << "\n";
            details << "  Shape validity: " << (fileStat.shapeValid ? "VALID" : "INVALID") << "\n";
            details << "  Shape closure: " << (fileStat.shapeClosed ? "CLOSED" : "OPEN") << "\n";
        }

        // Mesh details
        if (fileStat.meshVertices > 0) {
            details << "\nMesh Information:\n";
            details << "  Vertices: " << fileStat.meshVertices << "\n";
            details << "  Triangles: " << fileStat.meshTriangles << "\n";
            details << "  Build time: " << wxString::Format("%.2f ms", fileStat.meshBuildTime) << "\n";
            details << "  Normal calculation: " << wxString::Format("%.2f ms", fileStat.normalCalculationTime) << "\n";
            details << "  Normal smoothing: " << wxString::Format("%.2f ms", fileStat.normalSmoothingTime) << "\n";
        }

        // Material details
        if (!fileStat.materialDiffuse.empty()) {
            details << "\nMaterial Information:\n";
            details << "  Diffuse: " << fileStat.materialDiffuse << "\n";
            details << "  Ambient: " << fileStat.materialAmbient << "\n";
            details << "  Transparency: " << wxString::Format("%.3f", fileStat.materialTransparency) << "\n";
            details << "  Texture enabled: " << (fileStat.textureEnabled ? "Yes" : "No") << "\n";
            details << "  Blend mode: " << fileStat.blendMode << "\n";
        }

        m_fileDetailsText->SetValue(details);
    }
}
