#pragma once

#include "flatui/FlatUITitledPanel.h"
#include <wx/propgrid/propgrid.h>
#include "GeometryObject.h"
#include "OCCGeometry.h"

class PropertyPanel : public FlatUITitledPanel
{
public:
	PropertyPanel(wxWindow* parent);
	~PropertyPanel();

	void updateProperties(GeometryObject* object);
	void updateProperties(std::shared_ptr<OCCGeometry> geometry);
	void clearProperties();

private:
	void onPropertyChanged(wxPropertyGridEvent& event);
	void handleOCCGeometryPropertyChange(wxPGProperty* property);

	wxPropertyGrid* m_propGrid;
	GeometryObject* m_currentObject;
	std::shared_ptr<OCCGeometry> m_currentOCCGeometry;
};