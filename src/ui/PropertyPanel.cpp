#include "PropertyPanel.h"
#include "GeometryObject.h"
#include "Logger.h"
#include <wx/propgrid/propgrid.h>
#include <wx/sizer.h>

PropertyPanel::PropertyPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    LOG_INF("PropertyPanel initializing");
    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_propGrid, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);

    m_propGrid->Bind(wxEVT_PG_CHANGED, &PropertyPanel::onPropertyChanged, this);
}

PropertyPanel::~PropertyPanel()
{
    LOG_INF("PropertyPanel destroying");
}

void PropertyPanel::updateProperties(GeometryObject* object)
{
    if (!object) {
        LOG_WRN("Attempted to update properties for null object");
        m_propGrid->Clear();
        m_currentObject = nullptr;
        return;
    }

    LOG_INF("Updating properties for object: " + object->getName());
    m_currentObject = object;
    m_propGrid->Clear();

    m_propGrid->Append(new wxStringProperty("Name", "Name", object->getName()));

    SoTransform* transform = object->getTransform();
    if (transform) {
        SbVec3f translation;
        transform->translation.getValue().getValue(translation[0], translation[1], translation[2]);

        m_propGrid->Append(new wxFloatProperty("Position X", "PosX", translation[0]));
        m_propGrid->Append(new wxFloatProperty("Position Y", "PosY", translation[1]));
        m_propGrid->Append(new wxFloatProperty("Position Z", "PosZ", translation[2]));
    }
    else {
        LOG_WRN("No transform available for object: " + object->getName());
    }

    m_propGrid->Append(new wxBoolProperty("Visible", "Visible", object->isVisible()));
    m_propGrid->Append(new wxBoolProperty("Selected", "Selected", object->isSelected()));
}

void PropertyPanel::onPropertyChanged(wxPropertyGridEvent& event)
{
    if (!m_currentObject) {
        LOG_WRN("Property changed but no object selected");
        return;
    }

    wxPGProperty* property = event.GetProperty();
    if (!property) {
        LOG_ERR("Invalid property in onPropertyChanged");
        return;
    }

    LOG_INF("Property changed: " + property->GetName().ToStdString() + " to " + property->GetValueAsString().ToStdString());

    if (property->GetName() == "Name") {
        m_currentObject->setName(property->GetValueAsString().ToStdString());
    }
    else if (property->GetName() == "PosX" || property->GetName() == "PosY" || property->GetName() == "PosZ") {
        SoTransform* transform = m_currentObject->getTransform();
        if (transform) {
            SbVec3f translation;
            transform->translation.getValue().getValue(translation[0], translation[1], translation[2]);

            if (property->GetName() == "PosX") {
                translation[0] = property->GetValue().GetDouble();
            }
            else if (property->GetName() == "PosY") {
                translation[1] = property->GetValue().GetDouble();
            }
            else if (property->GetName() == "PosZ") {
                translation[2] = property->GetValue().GetDouble();
            }

            m_currentObject->setPosition(translation);
        }
        else {
            LOG_WRN("No transform available for property update: " + m_currentObject->getName());
        }
    }
    else if (property->GetName() == "Visible") {
        m_currentObject->setVisible(property->GetValue().GetBool());
    }
    else if (property->GetName() == "Selected") {
        m_currentObject->setSelected(property->GetValue().GetBool());
    }
}

void PropertyPanel::clearProperties()
{
    m_propGrid->Clear();
    m_currentObject = nullptr;
}