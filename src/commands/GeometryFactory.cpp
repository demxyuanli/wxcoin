#include "GeometryFactory.h"
#include "GeometryObject.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "Command.h"
#include "CreateCommand.h"
#include "Logger.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "OCCShapeBuilder.h"
#include <gp_Dir.hxx>
#include <memory>
#include <gp_Pnt.hxx>

GeometryFactory::GeometryFactory(SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel, 
                                CommandManager* cmdManager, OCCViewer* occViewer)
    : m_root(root)
    , m_treePanel(treePanel)
    , m_propPanel(propPanel)
    , m_cmdManager(cmdManager)
    , m_occViewer(occViewer)
    , m_defaultGeometryType(GeometryType::COIN3D)
{
    LOG_INF("GeometryFactory initializing with OCC support");
}

GeometryFactory::~GeometryFactory() {
    LOG_INF("GeometryFactory destroying");
}

void GeometryFactory::createGeometry(const std::string& type, const SbVec3f& position, GeometryType geomType) {
    if (geomType == GeometryType::OPENCASCADE) {
        createOCCGeometry(type, position);
        return;
    }
    
    // Default Coin3D geometry creation
    std::unique_ptr<GeometryObject> object = nullptr;
    if (type == "Box") {
        object = std::unique_ptr<GeometryObject>(createBox(position));
    }
    else if (type == "Sphere") {
        object = std::unique_ptr<GeometryObject>(createSphere(position));
    }
    else if (type == "Cylinder") {
        object = std::unique_ptr<GeometryObject>(createCylinder(position));
    }
    else if (type == "Cone") {
        object = std::unique_ptr<GeometryObject>(createCone(position));
    }
    else {
        LOG_ERR("Unknown geometry type: " + type);
        return;
    }

    if (object) {
        auto command = std::make_shared<CreateCommand>(std::move(object), m_root, m_treePanel, m_propPanel);
        m_cmdManager->executeCommand(command);
    }
}

void GeometryFactory::createOCCGeometry(const std::string& type, const SbVec3f& position) {
    if (!m_occViewer) {
        LOG_ERR("OCC Viewer not available for creating OpenCASCADE geometry");
        return;
    }
    
    std::shared_ptr<OCCGeometry> geometry = nullptr;
    
    if (type == "Box") {
        geometry = createOCCBox(position);
    }
    else if (type == "Sphere") {
        geometry = createOCCSphere(position);
    }
    else if (type == "Cylinder") {
        geometry = createOCCCylinder(position);
    }
    else if (type == "Cone") {
        geometry = createOCCCone(position);
    }
    else if (type == "Wrench") {
        geometry = createOCCWrench(position);
    }
    else {
        LOG_ERR("Unknown OCC geometry type: " + type);
        return;
    }
    
    if (geometry) {
        m_occViewer->addGeometry(geometry);
        LOG_INF("Created OCC geometry: " + geometry->getName());
    }
}

GeometryObject* GeometryFactory::createBox(const SbVec3f& position) {
    Box* box = new Box(1.0f, 1.0f, 1.0f);
    box->setPosition(position);
    return box;
}

GeometryObject* GeometryFactory::createSphere(const SbVec3f& position) {
    Sphere* sphere = new Sphere(0.5f);
    sphere->setPosition(position);
    return sphere;
}

GeometryObject* GeometryFactory::createCylinder(const SbVec3f& position) {
    Cylinder* cylinder = new Cylinder(0.5f, 1.0f);
    cylinder->setPosition(position);
    return cylinder;
}

GeometryObject* GeometryFactory::createCone(const SbVec3f& position) {
    Cone* cone = new Cone(0.5f, 1.0f);
    cone->setPosition(position);
    return cone;
}

// OpenCASCADE geometry creation methods
std::shared_ptr<OCCGeometry> GeometryFactory::createOCCBox(const SbVec3f& position) {
    static int boxCounter = 0;
    std::string name = "OCCBox_" + std::to_string(++boxCounter);
    
    auto box = std::make_shared<OCCBox>(name, 1.0, 1.0, 1.0);
    box->setPosition(gp_Pnt(position[0], position[1], position[2]));
    return box;
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCSphere(const SbVec3f& position) {
    static int sphereCounter = 0;
    std::string name = "OCCSphere_" + std::to_string(++sphereCounter);
    
    auto sphere = std::make_shared<OCCSphere>(name, 0.5);
    sphere->setPosition(gp_Pnt(position[0], position[1], position[2]));
    return sphere;
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCCylinder(const SbVec3f& position) {
    static int cylinderCounter = 0;
    std::string name = "OCCCylinder_" + std::to_string(++cylinderCounter);
    
    auto cylinder = std::make_shared<OCCCylinder>(name, 0.5, 1.0);
    cylinder->setPosition(gp_Pnt(position[0], position[1], position[2]));
    return cylinder;
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCCone(const SbVec3f& position) {
    static int coneCounter = 0;
    std::string name = "OCCCone_" + std::to_string(++coneCounter);
    
    auto cone = std::make_shared<OCCCone>(name, 0.5, 0.0, 1.0);
    cone->setPosition(gp_Pnt(position[0], position[1], position[2]));
    return cone;
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCWrench(const SbVec3f& position) {
    static int wrenchCounter = 0;
    std::string name = "OCCWrench_" + std::to_string(++wrenchCounter);
    // Parameters for wrench
    double length = 10.0;
    double width = 2.0;
    double thickness = 1.0;
    double holeRadius = 0.4;
    double holeSpacing = 3.0;
    double filletRadius = 0.2;
    // Create handle box
    TopoDS_Shape bar = OCCShapeBuilder::createBox(length, width, thickness);
    // Create cylindrical holes
    TopoDS_Shape hole1 = OCCShapeBuilder::createCylinder(holeRadius, thickness * 2,
        gp_Pnt(holeSpacing, width / 2.0, thickness / 2.0), gp_Dir(0, 0, 1));
    TopoDS_Shape hole2 = OCCShapeBuilder::createCylinder(holeRadius, thickness * 2,
        gp_Pnt(length - holeSpacing, width / 2.0, thickness / 2.0), gp_Dir(0, 0, 1));
    // Subtract holes from bar
    TopoDS_Shape cut1 = OCCShapeBuilder::booleanDifference(bar, hole1);
    TopoDS_Shape cut2 = OCCShapeBuilder::booleanDifference(cut1, hole2);
    // Wrap in OCCGeometry
    auto geometry = std::make_shared<OCCGeometry>(name);
    geometry->setShape(cut2);
    // Position the wrench
    geometry->setPosition(gp_Pnt(position[0], position[1], position[2]));
    return geometry;
} 