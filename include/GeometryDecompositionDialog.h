#pragma once

#include "widgets/FramelessModalPopup.h"
#include "GeometryReader.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/button.h>

/**
 * @brief Dialog for configuring geometry decomposition options
 */
class GeometryDecompositionDialog : public FramelessModalPopup
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param options Current decomposition options to modify
     * @param isLargeComplexGeometry Whether the geometry is large or complex (limits high-quality options)
     */
    GeometryDecompositionDialog(wxWindow* parent, GeometryReader::DecompositionOptions& options, bool isLargeComplexGeometry = false);

    /**
     * @brief Destructor
     */
    ~GeometryDecompositionDialog();

    /**
     * @brief Get the configured decomposition options
     * @return DecompositionOptions struct
     */
    GeometryReader::DecompositionOptions getDecompositionOptions() const;

    /**
     * @brief Check if geometry is large or complex based on file size
     * @param filePaths List of file paths to check
     * @return true if any file is large or complex
     */
    static bool isLargeComplexGeometry(const std::vector<std::string>& filePaths);

    /**
     * @brief Check if geometry is complex based on face count and assembly count
     * @param faceCount Total number of faces in the geometry
     * @param assemblyCount Total number of assembly components
     * @return true if geometry is complex
     */
    static bool isComplexGeometryByCounts(int faceCount, int assemblyCount);

private:
    /**
     * @brief Create dialog controls
     */
    void createControls();

    /**
     * @brief Create decomposition page
     */
    void createDecompositionPage();

    /**
     * @brief Create mesh quality page
     */
    void createMeshQualityPage();

    /**
     * @brief Create smooth surface page
     */
    void createSmoothSurfacePage();

    /**
     * @brief Layout dialog controls
     */
    void layoutControls();

    /**
     * @brief Bind event handlers
     */
    void bindEvents();

    /**
     * @brief Update preview text based on current settings
     */
    void updatePreview();

    /**
     * @brief Update color scheme preview
     */
    void updateColorPreview();

    /**
     * @brief OK button handler
     */
    void onOK(wxCommandEvent& event);

    /**
     * @brief Cancel button handler
     */
    void onCancel(wxCommandEvent& event);

    /**
     * @brief Decomposition level change handler
     */
    void onDecompositionLevelChange(wxCommandEvent& event);

    /**
     * @brief Color scheme change handler
     */
    void onColorSchemeChange(wxCommandEvent& event);

private:
    wxDECLARE_EVENT_TABLE();

    // Reference to the options being configured
    GeometryReader::DecompositionOptions& m_options;

    // Current settings
    bool m_enableDecomposition;
    GeometryReader::DecompositionLevel m_decompositionLevel;
    GeometryReader::ColorScheme m_colorScheme;
    bool m_useConsistentColoring;
    GeometryReader::MeshQualityPreset m_meshQualityPreset;
    double m_customMeshDeflection;
    double m_customAngularDeflection;
    
    // Smooth surface settings
    bool m_subdivisionEnabled;
    int m_subdivisionLevel;
    bool m_smoothingEnabled;
    int m_smoothingIterations;
    double m_smoothingStrength;
    bool m_lodEnabled;
    double m_lodFineDeflection;
    double m_lodRoughDeflection;
    int m_tessellationQuality;
    double m_featurePreservation;
    double m_smoothingCreaseAngle;
    
    // Flag to prevent recursive calls
    bool m_updatingMeshQuality;

    // Flag to indicate if geometry is large/complex (limits high-quality options)
    bool m_isLargeComplexGeometry;

    // Tab notebook
    wxNotebook* m_notebook;
    wxPanel* m_decompositionPage;
    wxPanel* m_meshQualityPage;
    wxPanel* m_smoothSurfacePage;

    // Decomposition UI controls
    wxCheckBox* m_enableDecompositionCheckBox;
    wxChoice* m_decompositionLevelChoice;
    wxChoice* m_colorSchemeChoice;
    wxCheckBox* m_consistentColoringCheckBox;
    wxStaticText* m_previewText;
    wxPanel* m_previewPanel;
    wxPanel* m_colorPreviewPanel;

    // Mesh quality controls
    wxButton* m_fastPresetBtn;
    wxButton* m_balancedPresetBtn;
    wxButton* m_highQualityPresetBtn;
    wxButton* m_ultraQualityPresetBtn;
    wxButton* m_customPresetBtn;
    wxTextCtrl* m_customDeflectionCtrl;
    wxTextCtrl* m_customAngularCtrl;
    wxStaticText* m_meshQualityPreviewText;

    // Smooth surface controls
    wxCheckBox* m_subdivisionEnabledCheckBox;
    wxTextCtrl* m_subdivisionLevelCtrl;
    wxCheckBox* m_smoothingEnabledCheckBox;
    wxTextCtrl* m_smoothingIterationsCtrl;
    wxTextCtrl* m_smoothingStrengthCtrl;
    wxCheckBox* m_lodEnabledCheckBox;
    wxTextCtrl* m_lodFineDeflectionCtrl;
    wxTextCtrl* m_lodRoughDeflectionCtrl;
    wxTextCtrl* m_tessellationQualityCtrl;
    wxTextCtrl* m_featurePreservationCtrl;
    wxTextCtrl* m_smoothingCreaseAngleCtrl;

    /**
     * @brief Update mesh quality controls visibility
     */
    void updateMeshQualityControls();

    /**
     * @brief Update preset button colors to show selected preset
     */
    void updatePresetButtonColors();

    /**
     * @brief Fast preset button handler
     */
    void onFastPreset(wxCommandEvent& event);

    /**
     * @brief Balanced preset button handler
     */
    void onBalancedPreset(wxCommandEvent& event);

    /**
     * @brief High quality preset button handler
     */
    void onHighQualityPreset(wxCommandEvent& event);

    /**
     * @brief Ultra quality preset button handler
     */
    void onUltraQualityPreset(wxCommandEvent& event);

    /**
     * @brief Custom preset button handler
     */
    void onCustomPreset(wxCommandEvent& event);

    /**
     * @brief Apply restrictions for large complex geometries
     */
    void applyLargeComplexGeometryRestrictions();
};