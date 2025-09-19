#include "PropertyPanel.h"
#include "GeometryObject.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"
#include <wx/propgrid/propgrid.h>
#include <wx/sizer.h>
#include <memory>

PropertyPanel::PropertyPanel(wxWindow* parent)
	: FlatUITitledPanel(parent, "Object Properties")
{
	LOG_INF_S("PropertyPanel initializing");
    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER);
	m_mainSizer->Add(m_propGrid, 1, wxEXPAND | wxALL, 2);

    m_propGrid->Bind(wxEVT_PG_CHANGED, &PropertyPanel::onPropertyChanged, this);

    // Apply theme colours
    m_propGrid->SetBackgroundColour(CFG_COLOUR("PanelContentBgColour"));
    m_propGrid->SetForegroundColour(CFG_COLOUR("PanelTextColour"));
    m_propGrid->SetCaptionBackgroundColour(CFG_COLOUR("PanelHeaderColour"));
    m_propGrid->SetCaptionTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    m_propGrid->SetLineColour(CFG_COLOUR("PanelSeparatorBgColour"));
}

PropertyPanel::~PropertyPanel()
{
	LOG_INF_S("PropertyPanel destroying");
}

void PropertyPanel::updateProperties(GeometryObject* object)
{
	if (!object) {
		LOG_WRN_S("Attempted to update properties for null object");
		m_propGrid->Clear();
		m_currentObject = nullptr;
		m_currentOCCGeometry = nullptr;
		return;
	}

	LOG_INF_S("Updating properties for object: " + object->getName());
	m_currentObject = object;
	m_currentOCCGeometry = nullptr;
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
		LOG_WRN_S("No transform available for object: " + object->getName());
	}

	m_propGrid->Append(new wxBoolProperty("Visible", "Visible", object->isVisible()));
	m_propGrid->Append(new wxBoolProperty("Selected", "Selected", object->isSelected()));
}

void PropertyPanel::updateProperties(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) {
		LOG_WRN_S("Attempted to update properties for null OCCGeometry");
		m_propGrid->Clear();
		m_currentObject = nullptr;
		m_currentOCCGeometry = nullptr;
		return;
	}

	LOG_INF_S("Updating properties for OCCGeometry: " + geometry->getName());
	m_currentObject = nullptr;
	m_currentOCCGeometry = geometry;
	m_propGrid->Clear();

	// Basic properties
	m_propGrid->Append(new wxStringProperty("Name", "Name", geometry->getName()));
	m_propGrid->Append(new wxBoolProperty("Visible", "Visible", geometry->isVisible()));
	m_propGrid->Append(new wxBoolProperty("Selected", "Selected", geometry->isSelected()));

	// Position properties
	gp_Pnt position = geometry->getPosition();
	m_propGrid->Append(new wxFloatProperty("Position X", "PosX", position.X()));
	m_propGrid->Append(new wxFloatProperty("Position Y", "PosY", position.Y()));
	m_propGrid->Append(new wxFloatProperty("Position Z", "PosZ", position.Z()));

	// Scale property
	m_propGrid->Append(new wxFloatProperty("Scale", "Scale", geometry->getScale()));

	// Transparency property
	m_propGrid->Append(new wxFloatProperty("Transparency", "Transparency", geometry->getTransparency()));

	// Color properties
	Quantity_Color color = geometry->getColor();
	m_propGrid->Append(new wxFloatProperty("Color R", "ColorR", color.Red()));
	m_propGrid->Append(new wxFloatProperty("Color G", "ColorG", color.Green()));
	m_propGrid->Append(new wxFloatProperty("Color B", "ColorB", color.Blue()));

	// Geometry type specific properties
	if (auto box = std::dynamic_pointer_cast<OCCBox>(geometry)) {
		double width, height, depth;
		box->getSize(width, height, depth);
		m_propGrid->Append(new wxFloatProperty("Width", "Width", width));
		m_propGrid->Append(new wxFloatProperty("Height", "Height", height));
		m_propGrid->Append(new wxFloatProperty("Depth", "Depth", depth));
	}
	else if (auto cylinder = std::dynamic_pointer_cast<OCCCylinder>(geometry)) {
		double radius, height;
		cylinder->getSize(radius, height);
		m_propGrid->Append(new wxFloatProperty("Radius", "Radius", radius));
		m_propGrid->Append(new wxFloatProperty("Height", "Height", height));
	}
	else if (auto sphere = std::dynamic_pointer_cast<OCCSphere>(geometry)) {
		m_propGrid->Append(new wxFloatProperty("Radius", "Radius", sphere->getRadius()));
	}
	else if (auto cone = std::dynamic_pointer_cast<OCCCone>(geometry)) {
		double bottomRadius, topRadius, height;
		cone->getSize(bottomRadius, topRadius, height);
		m_propGrid->Append(new wxFloatProperty("Bottom Radius", "BottomRadius", bottomRadius));
		m_propGrid->Append(new wxFloatProperty("Top Radius", "TopRadius", topRadius));
		m_propGrid->Append(new wxFloatProperty("Height", "Height", height));
	}
}

void PropertyPanel::onPropertyChanged(wxPropertyGridEvent& event)
{
	wxPGProperty* property = event.GetProperty();
	if (!property) {
		LOG_ERR_S("Invalid property in onPropertyChanged");
		return;
	}

	LOG_INF_S("Property changed: " + property->GetName().ToStdString() + " to " + property->GetValueAsString().ToStdString());

	// Handle OCCGeometry properties
	if (m_currentOCCGeometry) {
		handleOCCGeometryPropertyChange(property);
		return;
	}

	// Handle legacy GeometryObject properties
	if (!m_currentObject) {
		LOG_WRN_S("Property changed but no object selected");
		return;
	}

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
			LOG_WRN_S("No transform available for property update: " + m_currentObject->getName());
		}
	}
	else if (property->GetName() == "Visible") {
		m_currentObject->setVisible(property->GetValue().GetBool());
	}
	else if (property->GetName() == "Selected") {
		m_currentObject->setSelected(property->GetValue().GetBool());
	}
}

void PropertyPanel::handleOCCGeometryPropertyChange(wxPGProperty* property)
{
	if (!m_currentOCCGeometry) return;

	if (property->GetName() == "Name") {
		// Note: OCCGeometry name is read-only for now
		LOG_INF_S("OCCGeometry name change ignored (read-only): " + property->GetValueAsString().ToStdString());
	}
	else if (property->GetName() == "Visible") {
		m_currentOCCGeometry->setVisible(property->GetValue().GetBool());
	}
	else if (property->GetName() == "Selected") {
		m_currentOCCGeometry->setSelected(property->GetValue().GetBool());
	}
	else if (property->GetName() == "PosX" || property->GetName() == "PosY" || property->GetName() == "PosZ") {
		gp_Pnt currentPos = m_currentOCCGeometry->getPosition();
		double x = currentPos.X();
		double y = currentPos.Y();
		double z = currentPos.Z();

		if (property->GetName() == "PosX") {
			x = property->GetValue().GetDouble();
		}
		else if (property->GetName() == "PosY") {
			y = property->GetValue().GetDouble();
		}
		else if (property->GetName() == "PosZ") {
			z = property->GetValue().GetDouble();
		}

		m_currentOCCGeometry->setPosition(gp_Pnt(x, y, z));
	}
	else if (property->GetName() == "Scale") {
		m_currentOCCGeometry->setScale(property->GetValue().GetDouble());
	}
	else if (property->GetName() == "Transparency") {
		m_currentOCCGeometry->setTransparency(property->GetValue().GetDouble());
	}
	else if (property->GetName() == "ColorR" || property->GetName() == "ColorG" || property->GetName() == "ColorB") {
		Quantity_Color currentColor = m_currentOCCGeometry->getColor();
		double r = currentColor.Red();
		double g = currentColor.Green();
		double b = currentColor.Blue();

		if (property->GetName() == "ColorR") {
			r = property->GetValue().GetDouble();
		}
		else if (property->GetName() == "ColorG") {
			g = property->GetValue().GetDouble();
		}
		else if (property->GetName() == "ColorB") {
			b = property->GetValue().GetDouble();
		}

		m_currentOCCGeometry->setColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
	}
	else if (property->GetName() == "Width" || property->GetName() == "Height" || property->GetName() == "Depth") {
		if (auto box = std::dynamic_pointer_cast<OCCBox>(m_currentOCCGeometry)) {
			double width, height, depth;
			box->getSize(width, height, depth);

			if (property->GetName() == "Width") {
				width = property->GetValue().GetDouble();
			}
			else if (property->GetName() == "Height") {
				height = property->GetValue().GetDouble();
			}
			else if (property->GetName() == "Depth") {
				depth = property->GetValue().GetDouble();
			}

			box->setDimensions(width, height, depth);
		}
	}
	else if (property->GetName() == "Radius" || property->GetName() == "Height") {
		if (auto cylinder = std::dynamic_pointer_cast<OCCCylinder>(m_currentOCCGeometry)) {
			double radius, height;
			cylinder->getSize(radius, height);

			if (property->GetName() == "Radius") {
				radius = property->GetValue().GetDouble();
			}
			else if (property->GetName() == "Height") {
				height = property->GetValue().GetDouble();
			}

			cylinder->setDimensions(radius, height);
		}
		else if (auto sphere = std::dynamic_pointer_cast<OCCSphere>(m_currentOCCGeometry)) {
			if (property->GetName() == "Radius") {
				sphere->setRadius(property->GetValue().GetDouble());
			}
		}
	}
	else if (property->GetName() == "BottomRadius" || property->GetName() == "TopRadius") {
		if (auto cone = std::dynamic_pointer_cast<OCCCone>(m_currentOCCGeometry)) {
			double bottomRadius, topRadius, height;
			cone->getSize(bottomRadius, topRadius, height);

			if (property->GetName() == "BottomRadius") {
				bottomRadius = property->GetValue().GetDouble();
			}
			else if (property->GetName() == "TopRadius") {
				topRadius = property->GetValue().GetDouble();
			}

			cone->setDimensions(bottomRadius, topRadius, height);
		}
	}
}

void PropertyPanel::clearProperties()
{
	m_propGrid->Clear();
	m_currentObject = nullptr;
	m_currentOCCGeometry = nullptr;
}