#pragma once

#include <wx/panel.h>
#include <wx/propgrid/propgrid.h>
#include "GeometryObject.h"

class PropertyPanel : public wxPanel
{
public:
    PropertyPanel(wxWindow* parent);
    ~PropertyPanel();

    void updateProperties(GeometryObject* object);
    void clearProperties();

private:
    void onPropertyChanged(wxPropertyGridEvent& event);

    wxPropertyGrid* m_propGrid;
    GeometryObject* m_currentObject;
};