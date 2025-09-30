#pragma once

#include "widgets/FramelessModalPopup.h"
#include "GeometryReader.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/panel.h>

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
     */
    GeometryDecompositionDialog(wxWindow* parent, GeometryReader::DecompositionOptions& options);

    /**
     * @brief Destructor
     */
    ~GeometryDecompositionDialog();

    /**
     * @brief Get the configured decomposition options
     * @return DecompositionOptions struct
     */
    GeometryReader::DecompositionOptions getDecompositionOptions() const;

private:
    /**
     * @brief Create dialog controls
     */
    void createControls();

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

    // UI controls
    wxCheckBox* m_enableDecompositionCheckBox;
    wxChoice* m_decompositionLevelChoice;
    wxChoice* m_colorSchemeChoice;
    wxCheckBox* m_consistentColoringCheckBox;
    wxStaticText* m_previewText;
    wxPanel* m_previewPanel;
    wxPanel* m_colorPreviewPanel;
};