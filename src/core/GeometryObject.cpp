#include "GeometryObject.h"
#include "DPIAwareRendering.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>

GeometryObject::GeometryObject(const std::string& name)
	: m_name(name), m_visible(true), m_selected(false)
{
	LOG_INF_S("Creating GeometryObject: " + name);
	m_root = new SoSeparator;
	m_root->ref();
	m_transform = new SoTransform;
	m_root->addChild(m_transform);
}

GeometryObject::~GeometryObject()
{
	LOG_INF_S("Destroying GeometryObject: " + m_name);
	if (m_root)
		m_root->unref();
}

void GeometryObject::setName(const std::string& name)
{
	LOG_INF_S("Renaming GeometryObject from " + m_name + " to " + name);
	m_name = name;
}

void GeometryObject::setPosition(const SbVec3f& position)
{
	LOG_INF_S("Setting position for " + m_name + ": (" + std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]) + ")");
	m_transform->translation.setValue(position);
}

void GeometryObject::setVisible(bool visible)
{
	LOG_INF_S("Setting visibility for " + m_name + ": " + (visible ? "true" : "false"));
	m_visible = visible;
}

void GeometryObject::setSelected(bool selected)
{
	LOG_INF_S("Setting selection for " + m_name + ": " + (selected ? "true" : "false"));
	m_selected = selected;
}