#include "GeometryObject.h"
#include "Logger.h"
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoMaterial.h>

GeometryObject::GeometryObject(const std::string& name)
    : m_name(name), m_visible(true), m_selected(false)
{
    LOG_INF("Creating GeometryObject: " + name);
    m_root = new SoSeparator;
    m_root->ref();
    m_transform = new SoTransform;
    m_root->addChild(m_transform);
}

GeometryObject::~GeometryObject()
{
    LOG_INF("Destroying GeometryObject: " + m_name);
    if (m_root)
        m_root->unref();
}

void GeometryObject::setName(const std::string& name)
{
    LOG_INF("Renaming GeometryObject from " + m_name + " to " + name);
    m_name = name;
}

void GeometryObject::setPosition(const SbVec3f& position)
{
    LOG_INF("Setting position for " + m_name + ": (" + std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]) + ")");
    m_transform->translation.setValue(position);
}

void GeometryObject::setVisible(bool visible)
{
    LOG_INF("Setting visibility for " + m_name + ": " + (visible ? "true" : "false"));
    m_visible = visible;
}

void GeometryObject::setSelected(bool selected)
{
    LOG_INF("Setting selection for " + m_name + ": " + (selected ? "true" : "false"));
    m_selected = selected;
}

Box::Box(float width, float height, float depth)
    : GeometryObject("Box")
{
    LOG_INF("Creating Box with dimensions: " + std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth));
    SoCube* cube = new SoCube;
    cube->width.setValue(width);
    cube->height.setValue(height);
    cube->depth.setValue(depth);

    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);

    m_root->addChild(material);
    m_root->addChild(cube);
}

Sphere::Sphere(float radius)
    : GeometryObject("Sphere")
{
    LOG_INF("Creating Sphere with radius: " + std::to_string(radius));
    SoSphere* sphere = new SoSphere;
    sphere->radius.setValue(radius);

    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);

    m_root->addChild(material);
    m_root->addChild(sphere);
}

Cylinder::Cylinder(float radius, float height)
    : GeometryObject("Cylinder")
{
    LOG_INF("Creating Cylinder with radius: " + std::to_string(radius) + ", height: " + std::to_string(height));
    SoCylinder* cylinder = new SoCylinder;
    cylinder->radius.setValue(radius);
    cylinder->height.setValue(height);

    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);

    m_root->addChild(material);
    m_root->addChild(cylinder);
}

Cone::Cone(float bottomRadius, float height)
    : GeometryObject("Cone")
{
    LOG_INF("Creating Cone with bottom radius: " + std::to_string(bottomRadius) + ", height: " + std::to_string(height));
    SoCone* cone = new SoCone;
    cone->bottomRadius.setValue(bottomRadius);
    cone->height.setValue(height);

    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);

    m_root->addChild(material);
    m_root->addChild(cone);
}