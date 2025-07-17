#include "GeometryFactory.h"
#include "GeometryObject.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "logger/Logger.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "OCCShapeBuilder.h"
#include "PositionDialog.h" // For GeometryParameters
#include "optimizer/PerformanceOptimizer.h"
#include <gp_Dir.hxx>
#include <memory>
#include <gp_Pnt.hxx>

GeometryFactory::GeometryFactory(SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel, 
                                OCCViewer* occViewer)
    : m_root(root)
    , m_treePanel(treePanel)
    , m_propPanel(propPanel)
    , m_occViewer(occViewer)
    , m_defaultGeometryType(GeometryType::OPENCASCADE)
{
    LOG_INF_S("GeometryFactory initializing with OCC support");
}

GeometryFactory::~GeometryFactory() {
    LOG_INF_S("GeometryFactory destroying");
}

void GeometryFactory::createOCCGeometry(const std::string& type, const SbVec3f& position) {
    START_PERFORMANCE_TIMING(geometry_creation);
    
    std::shared_ptr<OCCGeometry> geometry;
    
    // Use optimized geometry cache if available
    if (g_performanceOptimizer && g_performanceOptimizer->getGeometryCache()) {
        auto cache = g_performanceOptimizer->getGeometryCache();
        
        // Create geometry key for caching
        std::vector<double> params = {position[0], position[1], position[2]};
        optimizer::GeometryComputationCache::GeometryKey key(type, params);
        
        // Try to get from cache or create asynchronously
        auto future = cache->createGeometryAsync(key, [this, type, position]() -> TopoDS_Shape {
            if (type == "Box") {
                return createOCCBoxShape(position);
            } else if (type == "Sphere") {
                return createOCCSphereShape(position);
            } else if (type == "Cylinder") {
                return createOCCCylinderShape(position);
            } else if (type == "Cone") {
                return createOCCConeShape(position);
            } else if (type == "Torus") {
                return createOCCTorusShape(position);
            } else if (type == "TruncatedCylinder") {
                return createOCCTruncatedCylinderShape(position);
            } else if (type == "Wrench") {
                return createOCCWrenchShape(position);
            }
            return TopoDS_Shape();
        });
        
        // Wait for completion and create geometry object
        auto shape = future.get();
        if (!shape.IsNull()) {
            geometry = createGeometryFromShape(type, shape, position);
        }
    } else {
        // Fallback to original implementation
        if (type == "Box") {
            geometry = createOCCBox(position);
        } else if (type == "Sphere") {
            geometry = createOCCSphere(position);
        } else if (type == "Cylinder") {
            geometry = createOCCCylinder(position);
        } else if (type == "Cone") {
            geometry = createOCCCone(position);
        } else if (type == "Torus") {
            geometry = createOCCTorus(position);
        } else if (type == "TruncatedCylinder") {
            geometry = createOCCTruncatedCylinder(position);
        } else if (type == "Wrench") {
            geometry = createOCCWrench(position);
        } else {
            LOG_ERR_S("Unknown geometry type: " + type);
            END_PERFORMANCE_TIMING(geometry_creation);
            return;
        }
    }
    
    if (geometry) {
        m_treePanel->addOCCGeometry(geometry);
        m_occViewer->addGeometry(geometry);
        LOG_INF_S("Created OCC geometry: " + type);
    } else {
        LOG_ERR_S("Failed to create OCC geometry: " + type);
    }
    
    END_PERFORMANCE_TIMING(geometry_creation);
}

void GeometryFactory::createOCCGeometryWithParameters(const std::string& type, const SbVec3f& position, const GeometryParameters& params) {
    std::shared_ptr<OCCGeometry> geometry;
    
    LOG_INF_S("Creating geometry of type: " + type + " at position: " + 
              std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]));
    
    try {
        if (type == "Box") {
            geometry = createOCCBox(position, params.width, params.height, params.depth);
        } else if (type == "Sphere") {
            geometry = createOCCSphere(position, params.radius);
        } else if (type == "Cylinder") {
            geometry = createOCCCylinder(position, params.cylinderRadius, params.cylinderHeight);
        } else if (type == "Cone") {
            geometry = createOCCCone(position, params.bottomRadius, params.topRadius, params.coneHeight);
        } else if (type == "Torus") {
            geometry = createOCCTorus(position, params.majorRadius, params.minorRadius);
        } else if (type == "TruncatedCylinder") {
            geometry = createOCCTruncatedCylinder(position, params.truncatedBottomRadius, params.truncatedTopRadius, params.truncatedHeight);
        } else if (type == "Wrench") {
            geometry = createOCCWrench(position);
        } else {
            LOG_ERR_S("Unknown geometry type: " + type);
            return;
        }
        
        if (geometry) {
            LOG_INF_S("Geometry created successfully, adding to tree panel and viewer");
            m_treePanel->addOCCGeometry(geometry);
            m_occViewer->addGeometry(geometry);
            LOG_INF_S("Created OCC geometry with parameters: " + type);
        } else {
            LOG_ERR_S("Failed to create OCC geometry with parameters: " + type);
        }
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in createOCCGeometryWithParameters: " + std::string(e.what()));
    }
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCBox(const SbVec3f& position) {
    return createOCCBox(position, 2.0, 2.0, 2.0);
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCBox(const SbVec3f& position, double width, double height, double depth) {
    static int boxCounter = 0;
    std::string name = "OCCBox_" + std::to_string(++boxCounter);
    
    try {
        auto box = std::make_shared<OCCBox>(name, width, height, depth);
        
        if (box && !box->getShape().IsNull()) {
            box->setPosition(gp_Pnt(position[0], position[1], position[2]));
            LOG_INF_S("Created OCCBox: " + name + " with dimensions " + 
                     std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth));
            return box;
        } else {
            LOG_ERR_S("Failed to create box shape");
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating OCCBox: " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCSphere(const SbVec3f& position) {
    return createOCCSphere(position, 1.0);
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCSphere(const SbVec3f& position, double radius) {
    static int sphereCounter = 0;
    std::string name = "OCCSphere_" + std::to_string(++sphereCounter);
    
    try {
        auto sphere = std::make_shared<OCCSphere>(name, radius);
        
        if (sphere && !sphere->getShape().IsNull()) {
            sphere->setPosition(gp_Pnt(position[0], position[1], position[2]));
            LOG_INF_S("Created OCCSphere: " + name + " with radius " + std::to_string(radius));
            return sphere;
        } else {
            LOG_ERR_S("Failed to create sphere shape");
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating OCCSphere: " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCCylinder(const SbVec3f& position) {
    return createOCCCylinder(position, 1.0, 2.0);
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCCylinder(const SbVec3f& position, double radius, double height) {
    static int cylinderCounter = 0;
    std::string name = "OCCCylinder_" + std::to_string(++cylinderCounter);
    
    try {
        auto cylinder = std::make_shared<OCCCylinder>(name, radius, height);
        
        if (cylinder && !cylinder->getShape().IsNull()) {
            cylinder->setPosition(gp_Pnt(position[0], position[1], position[2]));
            LOG_INF_S("Created OCCCylinder: " + name + " with radius " + std::to_string(radius) + " height " + std::to_string(height));
            return cylinder;
        } else {
            LOG_ERR_S("Failed to create cylinder shape");
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating OCCCylinder: " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCCone(const SbVec3f& position) {
    return createOCCCone(position, 1.0, 0.5, 2.0);
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCCone(const SbVec3f& position, double bottomRadius, double topRadius, double height) {
    static int coneCounter = 0;
    std::string name = "OCCCone_" + std::to_string(++coneCounter);
    
    try {
        auto cone = std::make_shared<OCCCone>(name, bottomRadius, topRadius, height);
        
        if (cone && !cone->getShape().IsNull()) {
            cone->setPosition(gp_Pnt(position[0], position[1], position[2]));
            LOG_INF_S("Created OCCCone: " + name + " with bottom radius " + std::to_string(bottomRadius) + 
                     " top radius " + std::to_string(topRadius) + " height " + std::to_string(height));
            return cone;
        } else {
            LOG_ERR_S("Failed to create cone shape");
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating OCCCone: " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCTorus(const SbVec3f& position) {
    return createOCCTorus(position, 2.0, 0.5);
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCTorus(const SbVec3f& position, double majorRadius, double minorRadius) {
    static int torusCounter = 0;
    std::string name = "OCCTorus_" + std::to_string(++torusCounter);
    
    try {
        auto torus = std::make_shared<OCCTorus>(name, majorRadius, minorRadius);
        
        if (torus && !torus->getShape().IsNull()) {
            torus->setPosition(gp_Pnt(position[0], position[1], position[2]));
            LOG_INF_S("Created OCCTorus: " + name + " with major radius " + std::to_string(majorRadius) + 
                     " minor radius " + std::to_string(minorRadius));
            return torus;
        } else {
            LOG_ERR_S("Failed to create torus shape");
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating OCCTorus: " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCTruncatedCylinder(const SbVec3f& position) {
    return createOCCTruncatedCylinder(position, 1.0, 0.5, 2.0);
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCTruncatedCylinder(const SbVec3f& position, double bottomRadius, double topRadius, double height) {
    static int truncatedCylinderCounter = 0;
    std::string name = "OCCTruncatedCylinder_" + std::to_string(++truncatedCylinderCounter);
    
    try {
        auto truncatedCylinder = std::make_shared<OCCTruncatedCylinder>(name, bottomRadius, topRadius, height);
        
        if (truncatedCylinder && !truncatedCylinder->getShape().IsNull()) {
            truncatedCylinder->setPosition(gp_Pnt(position[0], position[1], position[2]));
            LOG_INF_S("Created OCCTruncatedCylinder: " + name + " with bottom radius " + std::to_string(bottomRadius) + 
                     " top radius " + std::to_string(topRadius) + " height " + std::to_string(height));
            return truncatedCylinder;
        } else {
            LOG_ERR_S("Failed to create truncated cylinder shape");
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating OCCTruncatedCylinder: " + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<OCCGeometry> GeometryFactory::createOCCWrench(const SbVec3f& position) {
    static int wrenchCounter = 0;
    std::string name = "OCCWrench_" + std::to_string(++wrenchCounter);
    
    try {
        // Realistic wrench dimensions (in cm) - based on real adjustable wrenches
        double totalLength = 25.0;           // Total length of wrench
        double handleLength = 15.0;          // Handle length
        double handleWidth = 2.5;            // Handle width
        double handleThickness = 1.2;        // Handle thickness
        
        double headLength = 10.0;            // Head length
        double headWidth = 5.0;              // Head width
        double headThickness = 1.5;          // Head thickness
        double jawOpening = 1.5;             // Increased jaw opening for better visibility
        double jawDepth = 3.5;               // Increased jaw depth
        
        LOG_INF_S("Creating professional wrench with proper connection...");

        // 1. Create main handle with ergonomic design
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
        
        // 2. Create fixed jaw (left side) - more substantial and realistic
        double fixedJawLength = headLength * 0.6;
        TopoDS_Shape fixedJaw = OCCShapeBuilder::createBox(
            fixedJawLength, 
            headWidth, 
            headThickness,
            gp_Pnt(position[0] - handleLength/2.0 - fixedJawLength, position[1] - headWidth/2.0, position[2] - headThickness/2.0)
        );
        
        if (fixedJaw.IsNull()) {
            LOG_ERR_S("Failed to create fixed jaw");
            return nullptr;
        }
        
        // 3. Create movable jaw (right side) - smaller and adjustable
        double movableJawLength = headLength * 0.2;
        TopoDS_Shape movableJaw = OCCShapeBuilder::createBox(
            movableJawLength, 
            headWidth, 
            headThickness,
            gp_Pnt(position[0] + handleLength/2.0, position[1] - headWidth/2.0, position[2] - headThickness/2.0)
        );
        
        if (movableJaw.IsNull()) {
            LOG_ERR_S("Failed to create movable jaw");
            return nullptr;
        }
        
        // 4. Create connection bridge between fixed and movable jaws
        double bridgeLength = headLength * 0.2; // 20% of head length for connection
        double bridgeWidth = headWidth * 0.8;   // Slightly narrower than head
        double bridgeThickness = headThickness * 0.6; // Thinner than head for realistic look
        
        TopoDS_Shape connectionBridge = OCCShapeBuilder::createBox(
            bridgeLength,
            bridgeWidth,
            bridgeThickness,
            gp_Pnt(position[0] - handleLength/2.0 - fixedJawLength + bridgeLength/2.0, 
                   position[1] - bridgeWidth/2.0, 
                   position[2] - bridgeThickness/2.0)
        );
        
        if (connectionBridge.IsNull()) {
            LOG_ERR_S("Failed to create connection bridge");
            return nullptr;
        }
        
        // 5. Union all main parts to create connected structure
        TopoDS_Shape wrenchBody = OCCShapeBuilder::booleanUnion(handle, fixedJaw);
        if (wrenchBody.IsNull()) {
            LOG_ERR_S("Failed to union handle with fixed jaw");
            return nullptr;
        }
        
        wrenchBody = OCCShapeBuilder::booleanUnion(wrenchBody, connectionBridge);
        if (wrenchBody.IsNull()) {
            LOG_ERR_S("Failed to union with connection bridge");
            return nullptr;
        }
        
        wrenchBody = OCCShapeBuilder::booleanUnion(wrenchBody, movableJaw);
        if (wrenchBody.IsNull()) {
            LOG_ERR_S("Failed to union with movable jaw");
            return nullptr;
        }
        
        LOG_INF_S("Connected wrench body created, now adding jaw openings...");
        
        // 6. Create larger, more visible jaw openings
        // Fixed jaw opening - much larger and more visible
        double fixedSlotWidth = jawOpening * 0.8;  // Increased from 0.6
        double fixedSlotDepth = jawDepth * 0.9;     // Increased from 0.8
        double fixedSlotHeight = headThickness * 0.98; // Almost full height
        
        TopoDS_Shape fixedSlot = OCCShapeBuilder::createBox(
            fixedSlotWidth,
            fixedSlotDepth,
            fixedSlotHeight,
            gp_Pnt(position[0] - handleLength/2.0 - fixedJawLength + fixedSlotWidth/2.0, 
                   position[1] - fixedSlotDepth/2.0, 
                   position[2] - fixedSlotHeight/2.0)
        );
        
        if (!fixedSlot.IsNull()) {
            TopoDS_Shape tempResult = OCCShapeBuilder::booleanDifference(wrenchBody, fixedSlot);
            if (!tempResult.IsNull()) {
                wrenchBody = tempResult;
                LOG_INF_S("Created large fixed jaw opening");
            }
        }
        
        // 7. Create movable jaw opening - also larger and more visible
        double movableSlotWidth = jawOpening * 0.6;  // Increased from 0.4
        double movableSlotDepth = jawDepth * 0.8;     // Increased from 0.6
        double movableSlotHeight = headThickness * 0.98; // Almost full height

        TopoDS_Shape movableSlot = OCCShapeBuilder::createBox(
            movableSlotWidth,
            movableSlotDepth,
            movableSlotHeight,
            gp_Pnt(position[0] + handleLength/2.0 + movableJawLength - movableSlotWidth - 0.1, 
                   position[1] - movableSlotDepth/2.0, 
                   position[2] - movableSlotHeight/2.0)
        );
        
        if (!movableSlot.IsNull() && !wrenchBody.IsNull()) {
            TopoDS_Shape tempResult = OCCShapeBuilder::booleanDifference(wrenchBody, movableSlot);
            if (!tempResult.IsNull()) {
                wrenchBody = tempResult;
                LOG_INF_S("Created large movable jaw opening");
            }
        }
        
        // 8. Add threaded adjustment mechanism with realistic proportions
        double threadDiameter = 1.0;
        double threadLength = 4.0;
        
        TopoDS_Shape adjustmentThread = OCCShapeBuilder::createCylinder(
            threadDiameter/2.0,
            threadLength,
            gp_Pnt(position[0] + handleLength/2.0 + movableJawLength + threadLength/2.0, 
                   position[1], 
                   position[2]),
            gp_Dir(1, 0, 0)
        );
        
        if (!adjustmentThread.IsNull()) {
            TopoDS_Shape tempResult = OCCShapeBuilder::booleanUnion(wrenchBody, adjustmentThread);
            if (!tempResult.IsNull()) {
                wrenchBody = tempResult;
                LOG_INF_S("Added adjustment thread");
            }
        }
        
        // 9. Add adjustment knob with knurling pattern
        double knobDiameter = 2.0;
        double knobThickness = 0.8;
        
        TopoDS_Shape adjustmentKnob = OCCShapeBuilder::createCylinder(
            knobDiameter/2.0,
            knobThickness,
            gp_Pnt(position[0] + handleLength/2.0 + movableJawLength + threadLength + knobThickness/2.0, 
                   position[1], 
                   position[2]),
            gp_Dir(1, 0, 0)
        );
        
        if (!adjustmentKnob.IsNull()) {
            TopoDS_Shape tempResult = OCCShapeBuilder::booleanUnion(wrenchBody, adjustmentKnob);
            if (!tempResult.IsNull()) {
                wrenchBody = tempResult;
                LOG_INF_S("Added adjustment knob");
            }
        }
        
        // 10. Add knurling pattern to adjustment knob (simplified)
        for (int i = 0; i < 6; i++) {
            double angle = i * 60.0; // 6 grooves around the knob
            double angleRad = angle * M_PI / 180.0;
            double grooveWidth = 0.2;
            double grooveDepth = knobDiameter * 0.25;
            double grooveHeight = knobThickness * 0.7;
            
            // Position groove on knob surface
            double grooveX = position[0] + handleLength/2.0 + movableJawLength + threadLength + knobThickness/2.0;
            double grooveY = position[1] + (knobDiameter/2.0 - grooveDepth/2.0) * cos(angleRad);
            double grooveZ = position[2] + (knobDiameter/2.0 - grooveDepth/2.0) * sin(angleRad);
            
            TopoDS_Shape groove = OCCShapeBuilder::createBox(
                grooveWidth,
                grooveDepth,
                grooveHeight,
                gp_Pnt(grooveX - grooveWidth/2.0, 
                       grooveY - grooveDepth/2.0, 
                       grooveZ - grooveHeight/2.0)
            );
            
            if (!groove.IsNull() && !wrenchBody.IsNull()) {
                TopoDS_Shape tempResult = OCCShapeBuilder::booleanDifference(wrenchBody, groove);
                if (!tempResult.IsNull()) {
                    wrenchBody = tempResult;
                }
            }
        }
        
        // 11. Add ergonomic handle grip pattern (multiple grooves with varying depths)
        for (int i = 0; i < 6; i++) {
            double grooveX = position[0] - handleLength/3.0 + i * handleLength/6.0;
            double grooveWidth = 0.4;
            double grooveDepth = handleWidth * 0.8;
            double grooveHeight = 0.25 + (i % 2) * 0.1; // Varying depths for better grip
            
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
        
        // 12. Add fillets for better appearance and safety
        if (!wrenchBody.IsNull()) {
            TopoDS_Shape filletedWrench = OCCShapeBuilder::createFillet(wrenchBody, 0.15);
            if (!filletedWrench.IsNull()) {
                wrenchBody = filletedWrench;
                LOG_INF_S("Added fillets to wrench");
            }
        }
        
        // 13. Add chamfers to sharp edges for better finish
        if (!wrenchBody.IsNull()) {
            TopoDS_Shape chamferedWrench = OCCShapeBuilder::createChamfer(wrenchBody, 0.1);
            if (!chamferedWrench.IsNull()) {
                wrenchBody = chamferedWrench;
                LOG_INF_S("Added chamfers to wrench");
            }
        }
        
        if (wrenchBody.IsNull()) {
            LOG_ERR_S("Final wrench shape is null");
            return nullptr;
        }
        
        LOG_INF_S("Final wrench shape created successfully - Shape is null: " + std::string(wrenchBody.IsNull() ? "yes" : "no"));
        
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
        
        LOG_INF_S("Creating OCCGeometry object for wrench: " + name);
        auto geometry = std::make_shared<OCCGeometry>(name);
        
        LOG_INF_S("Setting shape for wrench geometry: " + name);
        geometry->setShape(wrenchBody);
        
        // Set position to the specified location - this ensures the geometry is properly positioned
        LOG_INF_S("Setting position for wrench geometry: " + name + " at (" + 
                  std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]) + ")");
        geometry->setPosition(gp_Pnt(position[0], position[1], position[2]));
        
        LOG_INF_S("Created connected professional wrench model: " + name);
        return geometry;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating wrench: " + std::string(e.what()));
        return nullptr;
    }
}

// ============================================================================
// Optimized Shape Creation Methods for Caching
// ============================================================================

TopoDS_Shape GeometryFactory::createOCCBoxShape(const SbVec3f& position) {
    return OCCShapeBuilder::createBox(2.0, 2.0, 2.0, gp_Pnt(position[0], position[1], position[2]));
}

TopoDS_Shape GeometryFactory::createOCCSphereShape(const SbVec3f& position) {
    return OCCShapeBuilder::createSphere(1.0, gp_Pnt(position[0], position[1], position[2]));
}

TopoDS_Shape GeometryFactory::createOCCCylinderShape(const SbVec3f& position) {
    return OCCShapeBuilder::createCylinder(1.0, 2.0, gp_Pnt(position[0], position[1], position[2]), gp_Dir(0, 0, 1));
}

TopoDS_Shape GeometryFactory::createOCCConeShape(const SbVec3f& position) {
    return OCCShapeBuilder::createCone(1.0, 0.5, 2.0, gp_Pnt(position[0], position[1], position[2]), gp_Dir(0, 0, 1));
}

TopoDS_Shape GeometryFactory::createOCCTorusShape(const SbVec3f& position) {
    return OCCShapeBuilder::createTorus(2.0, 0.5, gp_Pnt(position[0], position[1], position[2]), gp_Dir(0, 0, 1));
}

TopoDS_Shape GeometryFactory::createOCCTruncatedCylinderShape(const SbVec3f& position) {
    // Create a cylinder and then truncate it using boolean operations
    auto cylinder = OCCShapeBuilder::createCylinder(1.0, 2.0, gp_Pnt(position[0], position[1], position[2]), gp_Dir(0, 0, 1));
    
    // Create a smaller cylinder to subtract for truncation
    auto truncateCylinder = OCCShapeBuilder::createCylinder(0.5, 2.0, gp_Pnt(position[0], position[1], position[2]), gp_Dir(0, 0, 1));
    
    // Perform boolean difference to create truncated cylinder
    return OCCShapeBuilder::booleanDifference(cylinder, truncateCylinder);
}

TopoDS_Shape GeometryFactory::createOCCWrenchShape(const SbVec3f& position) {
    // Simplified wrench shape for caching - use basic box shape
    return OCCShapeBuilder::createBox(8.0, 1.0, 1.0, gp_Pnt(position[0], position[1], position[2]));
}

std::shared_ptr<OCCGeometry> GeometryFactory::createGeometryFromShape(const std::string& type, const TopoDS_Shape& shape, const SbVec3f& position) {
    if (shape.IsNull()) {
        LOG_ERR_S("Cannot create geometry from null shape");
        return nullptr;
    }
    
    static int counter = 0;
    std::string name = "OCC" + type + "_" + std::to_string(++counter);
    
    auto geometry = std::make_shared<OCCGeometry>(name);
    geometry->setShape(shape);
    geometry->setPosition(gp_Pnt(position[0], position[1], position[2]));
    
    LOG_INF_S("Created geometry from cached shape: " + name);
    return geometry;
}
