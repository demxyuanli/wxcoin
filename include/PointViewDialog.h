#pragma once

#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/colour.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/RenderingConfig.h"
#include "widgets/FramelessModalPopup.h"

class OCCViewer;
class RenderingEngine;

/**
 * @brief Point view settings dialog
 *
 * Provides controls for configuring point view display parameters
 */
class PointViewDialog : public FramelessModalPopup
{
public:
    PointViewDialog(wxWindow* parent, OCCViewer* occViewer, RenderingEngine* renderingEngine);
    virtual ~PointViewDialog();

    // Point view settings accessors
    bool isPointViewEnabled() const { return m_showPointView; }
    double getPointSize() const { return m_pointSize; }
    Quantity_Color getPointColor() const { return m_pointColor; }
    int getPointShape() const { return m_pointShape; }
    bool isShowSolidEnabled() const { return m_showSolid; }

private:
    void createControls();
    void layoutControls();
    void bindEvents();
    void updateControls();

    // Event handlers
    void onPointSizeSlider(wxCommandEvent& event);
    void onPointColorButton(wxCommandEvent& event);
    void onPointShapeChoice(wxCommandEvent& event);
    void onShowPointViewCheckbox(wxCommandEvent& event);
    void onShowSolidCheckbox(wxCommandEvent& event);
    void onApply(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);

    // Helper methods
    void applySettings();
    void resetToDefaults();
    wxColour quantityColorToWxColour(const Quantity_Color& color);
    Quantity_Color wxColourToQuantityColor(const wxColour& color);
    void updateColorButton(wxButton* button, const wxColour& color);

    OCCViewer* m_occViewer;
    RenderingEngine* m_renderingEngine;

    // UI components
    wxCheckBox* m_showPointViewCheckbox;
    wxCheckBox* m_showSolidCheckbox;
    wxSlider* m_pointSizeSlider;
    wxStaticText* m_pointSizeLabel;
    wxButton* m_pointColorButton;
    wxChoice* m_pointShapeChoice;

    // Dialog buttons
    wxButton* m_applyButton;
    wxButton* m_cancelButton;
    wxButton* m_okButton;
    wxButton* m_resetButton;

    // Settings values
    bool m_showPointView;
    bool m_showSolid;
    double m_pointSize;
    Quantity_Color m_pointColor;
    int m_pointShape;
};
