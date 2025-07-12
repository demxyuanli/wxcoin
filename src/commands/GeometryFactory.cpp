#include "GeometryFactory.h"
#include "GeometryObject.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "Command.h"
#include "CreateCommand.h"
#include "logger/Logger.h"
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
    LOG_INF_S("GeometryFactory initializing with OCC support");
}

GeometryFactory::~GeometryFactory() {
    LOG_INF_S("GeometryFactory destroying");
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
        LOG_ERR_S("Unknown geometry type: " + type);
        return;
    }

    if (object) {
        auto command = std::make_shared<CreateCommand>(std::move(object), m_root, m_treePanel, m_propPanel);
        m_cmdManager->executeCommand(command);
    }
}

void GeometryFactory::createOCCGeometry(const std::string& type, const SbVec3f& position) {
    if (!m_occViewer) {
        LOG_ERR_S("OCC Viewer not available for creating OpenCASCADE geometry");
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
        LOG_ERR_S("Unknown OCC geometry type: " + type);
        return;
    }
    
    if (geometry) {
        m_occViewer->addGeometry(geometry);
        LOG_INF_S("Created OCC geometry: " + geometry->getName());
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
    
    try {
        // Improved wrench dimensions
        double totalLength = 12.0;
        double handleLength = 8.0;
        double handleWidth = 1.5;
        double handleThickness = 0.8;
        
        double headLength = 4.0;  // Total head length
        double headWidth = 3.0;
        double headThickness = 0.8;
        double jawOpening = 1.0; 
        
        LOG_INF_S("Creating improved wrench with better closure...");

        // Create handle as a centered box - now positioned at the specified location
        TopoDS_Shape handle = OCCShapeBuilder::createBox(
            handleLength, 
            handleWidth, 
            handleThickness,
            gp_Pnt(position[0] - handleLength/2.0, position[1] - handleWidth/2.0, position[2] - handleThickness/2.0)
        );
        
        if (handle.IsNull()) {
            LOG_ERR_S("Failed to create wrench handle");
            return nullptr;
        }
        
        // Create left jaw head (fixed jaw) - positioned relative to the specified location
        double leftJawLength = headLength * 0.6;
        TopoDS_Shape leftJawOuter = OCCShapeBuilder::createBox(
            leftJawLength, 
            headWidth, 
            headThickness,
            gp_Pnt(position[0] - handleLength/2.0 - leftJawLength, position[1] - headWidth/2.0, position[2] - headThickness/2.0)
        );
        
        if (leftJawOuter.IsNull()) {
            LOG_ERR_S("Failed to create left jaw outer");
            return nullptr;
        }
        
        // Create right jaw head (adjustable jaw) - positioned relative to the specified location
        double rightJawLength = headLength * 0.4;
        TopoDS_Shape rightJawOuter = OCCShapeBuilder::createBox(
            rightJawLength, 
            headWidth, 
            headThickness,
            gp_Pnt(position[0] + handleLength/2.0, position[1] - headWidth/2.0, position[2] - headThickness/2.0)
        );
        
        if (rightJawOuter.IsNull()) {
            LOG_ERR_S("Failed to create right jaw outer");
            return nullptr;
        }
        
        // Union all main parts first
        TopoDS_Shape wrenchBody = OCCShapeBuilder::booleanUnion(handle, leftJawOuter);
        if (wrenchBody.IsNull()) {
            LOG_ERR_S("Failed to union handle with left jaw");
            return nullptr;
        }
        
        wrenchBody = OCCShapeBuilder::booleanUnion(wrenchBody, rightJawOuter);
        if (wrenchBody.IsNull()) {
            LOG_ERR_S("Failed to union with right jaw");
            return nullptr;
        }
        
        LOG_INF_S("Basic wrench body created, now adding openings...");
        
        // Create left jaw opening (smaller and more centered) - positioned relative to the specified location
        double leftSlotWidth = jawOpening * 0.8;
        double leftSlotDepth = headWidth * 0.7;
        double leftSlotHeight = headThickness * 0.9;  // Slightly smaller than head thickness
        
        TopoDS_Shape leftSlot = OCCShapeBuilder::createBox(
            leftSlotWidth,
            leftSlotDepth,
            leftSlotHeight,
            gp_Pnt(position[0] - handleLength/2.0 - leftJawLength + leftSlotWidth/2.0, 
                   position[1] - leftSlotDepth/2.0, 
                   position[2] - leftSlotHeight/2.0)
        );
        
        if (!leftSlot.IsNull()) {
            TopoDS_Shape tempResult = OCCShapeBuilder::booleanDifference(wrenchBody, leftSlot);
            if (!tempResult.IsNull()) {
                wrenchBody = tempResult;
                LOG_INF_S("Created left jaw opening");
            } else {
                LOG_WRN_S("Failed to create left jaw opening, continuing without it");
            }
        }
        
        // Create right jaw opening (smaller and more centered) - positioned relative to the specified location
        double rightSlotWidth = jawOpening * 0.6;
        double rightSlotDepth = headWidth * 0.6;
        double rightSlotHeight = headThickness * 0.9;

        TopoDS_Shape rightSlot = OCCShapeBuilder::createBox(
            rightSlotWidth,
            rightSlotDepth,
            rightSlotHeight,
            gp_Pnt(position[0] + handleLength/2.0 + rightJawLength - rightSlotWidth - 0.2, 
                   position[1] - rightSlotDepth/2.0, 
                   position[2] - rightSlotHeight/2.0)
        );
        
        if (!rightSlot.IsNull() && !wrenchBody.IsNull()) {
            TopoDS_Shape tempResult = OCCShapeBuilder::booleanDifference(wrenchBody, rightSlot);
            if (!tempResult.IsNull()) {
                wrenchBody = tempResult;
                LOG_INF_S("Created right jaw opening");
            } else {
                LOG_WRN_S("Failed to create right jaw opening, continuing without it");
            }
        }
        
        // Add grip texture (smaller grooves) - positioned relative to the specified location
        for (int i = 0; i < 3; i++) {
            double grooveX = position[0] - handleLength/3.0 + i * handleLength/3.0;
            double grooveWidth = 0.2;
            double grooveDepth = handleWidth * 0.6;
            double grooveHeight = 0.15;
            
            TopoDS_Shape groove = OCCShapeBuilder::createBox(
                grooveWidth,
                grooveDepth,
                grooveHeight,
                gp_Pnt(grooveX - grooveWidth/2.0, 
                       position[1] - grooveDepth/2.0, 
                       position[2] + handleThickness/2.0 - grooveHeight/2.0)
            );
            
            if (!groove.IsNull() && !wrenchBody.IsNull()) {
                TopoDS_Shape tempResult = OCCShapeBuilder::booleanDifference(wrenchBody, groove);
                if (!tempResult.IsNull()) {
                    wrenchBody = tempResult;
                }
            }
        }
        
        if (wrenchBody.IsNull()) {
            LOG_ERR_S("Final wrench shape is null");
            return nullptr;
        }
        
        // Validate the final shape
        if (!OCCShapeBuilder::isValid(wrenchBody)) {
            LOG_WRN_S("Wrench shape validation failed, but proceeding anyway");
        } else {
            LOG_INF_S("Wrench shape is valid");
        }
        
        // Debug: Analyze the wrench shape in detail
        OCCShapeBuilder::analyzeShapeTopology(wrenchBody, name);
        OCCShapeBuilder::outputFaceNormalsAndIndices(wrenchBody, name);
        OCCShapeBuilder::analyzeShapeProperties(wrenchBody, name);
        
        auto geometry = std::make_shared<OCCGeometry>(name);
        geometry->setShape(wrenchBody);
        // Set position to the specified location - this ensures the geometry is properly positioned
        geometry->setPosition(gp_Pnt(position[0], position[1], position[2]));
        
        LOG_INF_S("Created improved wrench model: " + name);
        return geometry;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating wrench: " + std::string(e.what()));
        return nullptr;
    }
} 
