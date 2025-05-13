#pragma once

#include <string>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

class GeometryObject
{
public:
    GeometryObject(const std::string& name);
    virtual ~GeometryObject();

    virtual SoSeparator* getRoot() const { return m_root; }
    virtual std::string getName() const { return m_name; }
    virtual SoTransform* getTransform() const { return m_transform; }

    virtual void setName(const std::string& name);
    virtual void setPosition(const SbVec3f& position);
    virtual void setVisible(bool visible);
    virtual void setSelected(bool selected);

    virtual bool isVisible() const { return m_visible; }
    virtual bool isSelected() const { return m_selected; }

protected:
    std::string m_name;
    SoSeparator* m_root;
    SoTransform* m_transform;
    bool m_visible;
    bool m_selected;
};

class Box : public GeometryObject
{
public:
    Box(float width, float height, float depth);
};

class Sphere : public GeometryObject
{
public:
    Sphere(float radius);
};

class Cylinder : public GeometryObject
{
public:
    Cylinder(float radius, float height);
};

class Cone : public GeometryObject
{
public:
    Cone(float bottomRadius, float height);
};