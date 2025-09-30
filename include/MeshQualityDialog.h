#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/choice.h>

class OCCViewer;
class wxSpinDoubleEvent;
class wxSpinEvent;

class MeshQualityDialog : public FramelessModalPopup
{
public:
	MeshQualityDialog(wxWindow* parent, OCCViewer* occViewer);
	virtual ~MeshQualityDialog();

private:
	void createControls();
	void layoutControls();
	void bindEvents();
	void updateControls();

	// Page creation methods
	void createBasicQualityPage();
	void createSubdivisionPage();
	void createSmoothingPage();
	void createAdvancedPage();
	void createSurfaceSmoothingPresetsPage();

	// Basic quality event handlers
	void onDeflectionSlider(wxCommandEvent& event);
	void onDeflectionSpinCtrl(wxSpinDoubleEvent& event);
	void onAngularDeflectionSlider(wxCommandEvent& event);
	void onAngularDeflectionSpinCtrl(wxSpinDoubleEvent& event);
	void onLODEnable(wxCommandEvent& event);
	void onLODRoughDeflectionSlider(wxCommandEvent& event);
	void onLODRoughDeflectionSpinCtrl(wxSpinDoubleEvent& event);
	void onLODFineDeflectionSlider(wxCommandEvent& event);
	void onLODFineDeflectionSpinCtrl(wxSpinDoubleEvent& event);
	void onLODTransitionTimeSlider(wxCommandEvent& event);
	void onLODTransitionTimeSpinCtrl(wxSpinEvent& event);
	void onRealTimePreviewToggle(wxCommandEvent& event);

	// Subdivision event handlers
	void onSubdivisionEnable(wxCommandEvent& event);
	void onSubdivisionLevelSlider(wxCommandEvent& event);
	void onSubdivisionLevelSpinCtrl(wxSpinEvent& event);
	void onSubdivisionMethodChoice(wxCommandEvent& event);
	void onSubdivisionCreaseAngleSlider(wxCommandEvent& event);
	void onSubdivisionCreaseAngleSpinCtrl(wxSpinDoubleEvent& event);

	// Smoothing event handlers
	void onSmoothingEnable(wxCommandEvent& event);
	void onSmoothingMethodChoice(wxCommandEvent& event);
	void onSmoothingIterationsSlider(wxCommandEvent& event);
	void onSmoothingIterationsSpinCtrl(wxSpinEvent& event);
	void onSmoothingStrengthSlider(wxCommandEvent& event);
	void onSmoothingStrengthSpinCtrl(wxSpinDoubleEvent& event);
	void onSmoothingCreaseAngleSlider(wxCommandEvent& event);
	void onSmoothingCreaseAngleSpinCtrl(wxSpinDoubleEvent& event);

	// Advanced tessellation event handlers
	void onTessellationMethodChoice(wxCommandEvent& event);
	void onTessellationQualitySlider(wxCommandEvent& event);
	void onTessellationQualitySpinCtrl(wxSpinEvent& event);
	void onFeaturePreservationSlider(wxCommandEvent& event);
	void onFeaturePreservationSpinCtrl(wxSpinDoubleEvent& event);
	void onParallelProcessingCheckBox(wxCommandEvent& event);
	void onAdaptiveMeshingCheckBox(wxCommandEvent& event);

	// Dialog event handlers
	void onApply(wxCommandEvent& event);
	void onValidate(wxCommandEvent& event);
	void onExportReport(wxCommandEvent& event);
	void onReset(wxCommandEvent& event);
	void onCancel(wxCommandEvent& event);
	void onOK(wxCommandEvent& event);
	
	// Preset handlers
	void onPerformancePreset(wxCommandEvent& event);
	void onBalancedPreset(wxCommandEvent& event);
	void onQualityPreset(wxCommandEvent& event);
	void onUltraQualityPreset(wxCommandEvent& event);
	void onGamingPreset(wxCommandEvent& event);
	void onCADPreset(wxCommandEvent& event);
	void onRenderingPreset(wxCommandEvent& event);
	void onCustomPreset(wxCommandEvent& event);
	
	// Parameter dependency and linking
	void updateParameterDependencies(const std::string& parameter, double value);
	void syncAllUI();
	void forceImmediateVisualUpdate();
	
	// Surface smoothing preset handlers
	void onCoarseSurfacePreset(wxCommandEvent& event);
	void onStandardSurfacePreset(wxCommandEvent& event);
	void onSmoothSurfacePreset(wxCommandEvent& event);
	void onFineSurfacePreset(wxCommandEvent& event);
	void onUltraFineSurfacePreset(wxCommandEvent& event);
	void onCustomSurfacePreset(wxCommandEvent& event);
	
	// Helper methods
	void applyPreset(double deflection, bool lodEnabled, double roughDeflection, 
	                 double fineDeflection, bool parallelProcessing);
	void applySurfacePreset(double deflection, double angularDeflection,
		bool subdivisionEnabled, int subdivisionLevel,
		bool smoothingEnabled, int smoothingIterations, double smoothingStrength,
		bool lodEnabled, double lodFineDeflection, double lodRoughDeflection,
		int tessellationQuality, double featurePreservation, double smoothingCreaseAngle);

	OCCViewer* m_occViewer;

	// Notebook for tabbed interface
	wxNotebook* m_notebook;

	// Basic quality controls
	wxSlider* m_deflectionSlider;
	wxSpinCtrlDouble* m_deflectionSpinCtrl;
	wxSlider* m_angularDeflectionSlider;
	wxSpinCtrlDouble* m_angularDeflectionSpinCtrl;
	wxCheckBox* m_lodEnableCheckBox;
	wxSlider* m_lodRoughDeflectionSlider;
	wxSpinCtrlDouble* m_lodRoughDeflectionSpinCtrl;
	wxSlider* m_lodFineDeflectionSlider;
	wxSpinCtrlDouble* m_lodFineDeflectionSpinCtrl;
	wxSlider* m_lodTransitionTimeSlider;
	wxSpinCtrl* m_lodTransitionTimeSpinCtrl;

	// Subdivision controls
	wxCheckBox* m_subdivisionEnableCheckBox;
	wxSlider* m_subdivisionLevelSlider;
	wxSpinCtrl* m_subdivisionLevelSpinCtrl;
	wxChoice* m_subdivisionMethodChoice;
	wxSlider* m_subdivisionCreaseAngleSlider;
	wxSpinCtrlDouble* m_subdivisionCreaseAngleSpinCtrl;

	// Smoothing controls
	wxCheckBox* m_smoothingEnableCheckBox;
	wxChoice* m_smoothingMethodChoice;
	wxSlider* m_smoothingIterationsSlider;
	wxSpinCtrl* m_smoothingIterationsSpinCtrl;
	wxSlider* m_smoothingStrengthSlider;
	wxSpinCtrlDouble* m_smoothingStrengthSpinCtrl;
	wxSlider* m_smoothingCreaseAngleSlider;
	wxSpinCtrlDouble* m_smoothingCreaseAngleSpinCtrl;

	// Advanced tessellation controls
	wxChoice* m_tessellationMethodChoice;
	wxSlider* m_tessellationQualitySlider;
	wxSpinCtrl* m_tessellationQualitySpinCtrl;
	wxSlider* m_featurePreservationSlider;
	wxSpinCtrlDouble* m_featurePreservationSpinCtrl;
	wxCheckBox* m_parallelProcessingCheckBox;
	wxCheckBox* m_adaptiveMeshingCheckBox;
	
	// Real-time preview control
	wxCheckBox* m_realTimePreviewCheckBox;

	// Current parameter values
	double m_currentDeflection;
	double m_currentAngularDeflection;
	bool m_currentLODEnabled;
	double m_currentLODRoughDeflection;
	double m_currentLODFineDeflection;
	int m_currentLODTransitionTime;
	
	// Real-time preview control
	bool m_enableRealTimePreview;

	// Subdivision parameters
	bool m_currentSubdivisionEnabled;
	int m_currentSubdivisionLevel;
	int m_currentSubdivisionMethod;
	double m_currentSubdivisionCreaseAngle;

	// Smoothing parameters
	bool m_currentSmoothingEnabled;
	int m_currentSmoothingMethod;
	int m_currentSmoothingIterations;
	double m_currentSmoothingStrength;
	double m_currentSmoothingCreaseAngle;

	// Advanced tessellation parameters
	int m_currentTessellationMethod;
	int m_currentTessellationQuality;
	double m_currentFeaturePreservation;
	bool m_currentParallelProcessing;
	bool m_currentAdaptiveMeshing;
};