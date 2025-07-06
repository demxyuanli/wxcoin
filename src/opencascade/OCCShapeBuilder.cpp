#include "OCCShapeBuilder.h"
#include "Logger.h"

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Geom_Surface.hxx>
#include <GeomLProp_SLProps.hxx>
#include <gp_Trsf.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Vec.hxx>
#include <Standard_Failure.hxx>

TopoDS_Shape OCCShapeBuilder::createBox(double width, double height, double depth, const gp_Pnt& position)
{
    try {
        BRepPrimAPI_MakeBox boxMaker(width, height, depth);
        boxMaker.Build();
        if (!boxMaker.IsDone()) {
            LOG_ERR("Failed to create box: algorithm is not done.");
            return TopoDS_Shape();
        }
        
        TopoDS_Shape box = boxMaker.Shape();
        
        // Apply translation if position is not origin
        if (position.X() != 0.0 || position.Y() != 0.0 || position.Z() != 0.0) {
            gp_Trsf transform;
            transform.SetTranslation(gp_Vec(position.XYZ()));
            BRepBuilderAPI_Transform transformMaker(box, transform);
            if (!transformMaker.IsDone()) {
                LOG_ERR("Failed to translate box.");
                return TopoDS_Shape();
            }
            box = transformMaker.Shape();
        }
        
        return box;
    }
    catch (const Standard_Failure& e) {
        LOG_ERR("OCC exception creating box: " + std::string(e.GetMessageString()));
        return TopoDS_Shape();
    }
    catch (const std::exception& e) {
        LOG_ERR("Std exception creating box: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::createSphere(double radius, const gp_Pnt& center)
{
    try {
        BRepPrimAPI_MakeSphere sphereMaker(radius);
        if (!sphereMaker.IsDone()) {
            LOG_ERR("Failed to create sphere");
            return TopoDS_Shape();
        }
        
        TopoDS_Shape sphere = sphereMaker.Shape();
        
        // Apply translation if center is not origin
        if (center.X() != 0.0 || center.Y() != 0.0 || center.Z() != 0.0) {
            gp_Trsf transform;
            transform.SetTranslation(gp_Vec(center.XYZ()));
            sphere = BRepBuilderAPI_Transform(sphere, transform).Shape();
        }
        
        return sphere;
    } catch (const std::exception& e) {
        LOG_ERR("Exception creating sphere: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::createCylinder(double radius, double height, 
                                             const gp_Pnt& position, const gp_Dir& direction)
{
    try {
        gp_Ax2 axis(position, direction);
        BRepPrimAPI_MakeCylinder cylinderMaker(axis, radius, height);
        cylinderMaker.Build();
        if (!cylinderMaker.IsDone()) {
            LOG_ERR("Failed to create cylinder: algorithm is not done.");
            return TopoDS_Shape();
        }
        
        return cylinderMaker.Shape();
    }
    catch (const Standard_Failure& e) {
        LOG_ERR("OCC exception creating cylinder: " + std::string(e.GetMessageString()));
        return TopoDS_Shape();
    }
    catch (const std::exception& e) {
        LOG_ERR("Std exception creating cylinder: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::createCone(double bottomRadius, double topRadius, double height,
                                         const gp_Pnt& position, const gp_Dir& direction)
{
    try {
        gp_Ax2 axis(position, direction);
        BRepPrimAPI_MakeCone coneMaker(axis, bottomRadius, topRadius, height);
        if (!coneMaker.IsDone()) {
            LOG_ERR("Failed to create cone");
            return TopoDS_Shape();
        }
        
        return coneMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception creating cone: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::createTorus(double majorRadius, double minorRadius,
                                          const gp_Pnt& center, const gp_Dir& direction)
{
    try {
        gp_Ax2 axis(center, direction);
        BRepPrimAPI_MakeTorus torusMaker(axis, majorRadius, minorRadius);
        if (!torusMaker.IsDone()) {
            LOG_ERR("Failed to create torus");
            return TopoDS_Shape();
        }
        
        return torusMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception creating torus: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::booleanUnion(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    try {
        if (shape1.IsNull() || shape2.IsNull()) {
            LOG_ERR("Cannot perform boolean union on null shapes");
            return TopoDS_Shape();
        }
        
        BRepAlgoAPI_Fuse fuseMaker(shape1, shape2);
        if (!fuseMaker.IsDone()) {
            LOG_ERR("Boolean union failed");
            return TopoDS_Shape();
        }
        
        return fuseMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception in boolean union: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::booleanIntersection(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    try {
        if (shape1.IsNull() || shape2.IsNull()) {
            LOG_ERR("Cannot perform boolean intersection on null shapes");
            return TopoDS_Shape();
        }
        
        BRepAlgoAPI_Common commonMaker(shape1, shape2);
        if (!commonMaker.IsDone()) {
            LOG_ERR("Boolean intersection failed");
            return TopoDS_Shape();
        }
        
        return commonMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception in boolean intersection: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::booleanDifference(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    try {
        if (shape1.IsNull() || shape2.IsNull()) {
            LOG_ERR("Cannot perform boolean difference on null shapes");
            return TopoDS_Shape();
        }
        
        BRepAlgoAPI_Cut cutMaker(shape1, shape2);
        if (!cutMaker.IsDone()) {
            LOG_ERR("Boolean difference failed");
            return TopoDS_Shape();
        }
        
        return cutMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception in boolean difference: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::translate(const TopoDS_Shape& shape, const gp_Vec& translation)
{
    try {
        if (shape.IsNull()) {
            return TopoDS_Shape();
        }
        
        gp_Trsf transform;
        transform.SetTranslation(translation);
        
        BRepBuilderAPI_Transform transformMaker(shape, transform);
        if (!transformMaker.IsDone()) {
            LOG_ERR("Translation failed");
            return TopoDS_Shape();
        }
        
        return transformMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception in translation: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::rotate(const TopoDS_Shape& shape, const gp_Pnt& center, 
                                     const gp_Dir& axis, double angle)
{
    try {
        if (shape.IsNull()) {
            return TopoDS_Shape();
        }
        
        gp_Ax1 rotationAxis(center, axis);
        gp_Trsf transform;
        transform.SetRotation(rotationAxis, angle);
        
        BRepBuilderAPI_Transform transformMaker(shape, transform);
        if (!transformMaker.IsDone()) {
            LOG_ERR("Rotation failed");
            return TopoDS_Shape();
        }
        
        return transformMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception in rotation: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::scale(const TopoDS_Shape& shape, const gp_Pnt& center, double factor)
{
    try {
        if (shape.IsNull()) {
            return TopoDS_Shape();
        }
        
        gp_Trsf transform;
        transform.SetScale(center, factor);
        
        BRepBuilderAPI_Transform transformMaker(shape, transform);
        if (!transformMaker.IsDone()) {
            LOG_ERR("Scaling failed");
            return TopoDS_Shape();
        }
        
        return transformMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception in scaling: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

bool OCCShapeBuilder::isValid(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return false;
    }
    
    try {
        BRepCheck_Analyzer analyzer(shape);
        return analyzer.IsValid();
    } catch (const std::exception& e) {
        LOG_ERR("Exception in shape validation: " + std::string(e.what()));
        return false;
    }
}

double OCCShapeBuilder::getVolume(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return 0.0;
    }
    
    try {
        GProp_GProps props;
        BRepGProp::VolumeProperties(shape, props);
        return props.Mass();
    } catch (const std::exception& e) {
        LOG_ERR("Exception calculating volume: " + std::string(e.what()));
        return 0.0;
    }
}

double OCCShapeBuilder::getSurfaceArea(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return 0.0;
    }
    
    try {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(shape, props);
        return props.Mass();
    } catch (const std::exception& e) {
        LOG_ERR("Exception calculating surface area: " + std::string(e.what()));
        return 0.0;
    }
}

void OCCShapeBuilder::getBoundingBox(const TopoDS_Shape& shape, gp_Pnt& minPoint, gp_Pnt& maxPoint)
{
    if (shape.IsNull()) {
        minPoint = gp_Pnt(0, 0, 0);
        maxPoint = gp_Pnt(0, 0, 0);
        return;
    }
    
    try {
        Bnd_Box box;
        BRepBndLib::Add(shape, box);
        
        if (!box.IsVoid()) {
            double xMin, yMin, zMin, xMax, yMax, zMax;
            box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            minPoint = gp_Pnt(xMin, yMin, zMin);
            maxPoint = gp_Pnt(xMax, yMax, zMax);
        } else {
            minPoint = gp_Pnt(0, 0, 0);
            maxPoint = gp_Pnt(0, 0, 0);
        }
    } catch (const std::exception& e) {
        LOG_ERR("Exception calculating bounding box: " + std::string(e.what()));
        minPoint = gp_Pnt(0, 0, 0);
        maxPoint = gp_Pnt(0, 0, 0);
    }
}

// Filleting and chamfering implementations
TopoDS_Shape OCCShapeBuilder::createFillet(const TopoDS_Shape& shape, double radius)
{
    try {
        if (shape.IsNull()) {
            return TopoDS_Shape();
        }
        BRepFilletAPI_MakeFillet filletMaker(shape);
        for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
            filletMaker.Add(radius, TopoDS::Edge(ex.Current()));
        }
        if (!filletMaker.IsDone()) {
            LOG_ERR("Fillet creation failed");
            return TopoDS_Shape();
        }
        return filletMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception creating fillet: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape OCCShapeBuilder::createChamfer(const TopoDS_Shape& shape, double distance)
{
    try {
        if (shape.IsNull()) {
            return TopoDS_Shape();
        }
        BRepFilletAPI_MakeChamfer chamferMaker(shape);
        for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
            chamferMaker.Add(distance, TopoDS::Edge(ex.Current()));
        }
        if (!chamferMaker.IsDone()) {
            LOG_ERR("Chamfer creation failed");
            return TopoDS_Shape();
        }
        return chamferMaker.Shape();
    } catch (const std::exception& e) {
        LOG_ERR("Exception creating chamfer: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

// Debug and analysis methods implementation
void OCCShapeBuilder::analyzeShapeTopology(const TopoDS_Shape& shape, const std::string& shapeName)
{
    if (shape.IsNull()) {
        LOG_ERR("Cannot analyze null shape: " + shapeName);
        return;
    }
    
    std::string name = shapeName.empty() ? "Unknown" : shapeName;
    LOG_INF("=== Shape Topology Analysis: " + name + " ===");
    
    // Count different types of topological entities
    int solidCount = 0, shellCount = 0, faceCount = 0, wireCount = 0, edgeCount = 0, vertexCount = 0;
    
    for (TopExp_Explorer ex(shape, TopAbs_SOLID); ex.More(); ex.Next()) {
        solidCount++;
    }
    for (TopExp_Explorer ex(shape, TopAbs_SHELL); ex.More(); ex.Next()) {
        shellCount++;
    }
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
        faceCount++;
    }
    for (TopExp_Explorer ex(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        wireCount++;
    }
    for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
        edgeCount++;
    }
    for (TopExp_Explorer ex(shape, TopAbs_VERTEX); ex.More(); ex.Next()) {
        vertexCount++;
    }
    
    LOG_INF("Solids: " + std::to_string(solidCount));
    LOG_INF("Shells: " + std::to_string(shellCount));
    LOG_INF("Faces: " + std::to_string(faceCount));
    LOG_INF("Wires: " + std::to_string(wireCount));
    LOG_INF("Edges: " + std::to_string(edgeCount));
    LOG_INF("Vertices: " + std::to_string(vertexCount));
    
    // Check shape validity
    bool isValidShape = isValid(shape);
    LOG_INF("Shape validity: " + std::string(isValidShape ? "VALID" : "INVALID"));
    
    // Check closure
    bool isClosed = checkShapeClosure(shape, name);
    LOG_INF("Shape closure: " + std::string(isClosed ? "CLOSED" : "OPEN"));
    
    LOG_INF("=== End Topology Analysis ===");
}

void OCCShapeBuilder::outputFaceNormalsAndIndices(const TopoDS_Shape& shape, const std::string& shapeName)
{
    if (shape.IsNull()) {
        LOG_ERR("Cannot output face normals for null shape: " + shapeName);
        return;
    }
    
    std::string name = shapeName.empty() ? "Unknown" : shapeName;
    LOG_INF("=== Face Normals and Indices: " + name + " ===");
    
    try {
        int faceIndex = 0;
        for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
            TopoDS_Face face = TopoDS::Face(ex.Current());
            
            // Get face surface
            Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
            if (surface.IsNull()) {
                LOG_WRN("Face " + std::to_string(faceIndex) + ": No surface found");
                faceIndex++;
                continue;
            }
            
            // Get face bounds
            Standard_Real uMin, uMax, vMin, vMax;
            BRepTools::UVBounds(face, uMin, uMax, vMin, vMax);
            
            // Calculate normal at face center
            Standard_Real uMid = (uMin + uMax) / 2.0;
            Standard_Real vMid = (vMin + vMax) / 2.0;
            
            gp_Pnt point;
            gp_Vec normalVec;
            
            // Get point and normal at parameter space center
            GeomLProp_SLProps props(surface, uMid, vMid, 1, 1e-6);
            if (props.IsNormalDefined()) {
                point = props.Value();
                normalVec = props.Normal();
                
                // Check face orientation
                if (face.Orientation() == TopAbs_REVERSED) {
                    normalVec.Reverse();
                }
                
                LOG_INF("Face " + std::to_string(faceIndex) + ":");
                LOG_INF("  Center: (" + std::to_string(point.X()) + ", " + 
                       std::to_string(point.Y()) + ", " + std::to_string(point.Z()) + ")");
                LOG_INF("  Normal: (" + std::to_string(normalVec.X()) + ", " + 
                       std::to_string(normalVec.Y()) + ", " + std::to_string(normalVec.Z()) + ")");
                LOG_INF("  Orientation: " + std::string(face.Orientation() == TopAbs_FORWARD ? "FORWARD" : 
                       face.Orientation() == TopAbs_REVERSED ? "REVERSED" : "OTHER"));
            } else {
                LOG_WRN("Face " + std::to_string(faceIndex) + ": Normal not defined");
            }
            
            faceIndex++;
        }
    } catch (const std::exception& e) {
        LOG_ERR("Exception outputting face normals: " + std::string(e.what()));
    }
    
    LOG_INF("=== End Face Normals Output ===");
}

bool OCCShapeBuilder::checkShapeClosure(const TopoDS_Shape& shape, const std::string& shapeName)
{
    if (shape.IsNull()) {
        LOG_ERR("Cannot check closure of null shape: " + shapeName);
        return false;
    }
    
    std::string name = shapeName.empty() ? "Unknown" : shapeName;
    LOG_INF("=== Checking Shape Closure: " + name + " ===");
    
    try {
        bool isClosed = true;
        
        // Check if shape is a solid
        TopAbs_ShapeEnum shapeType = shape.ShapeType();
        LOG_INF("Shape type: " + std::string(
            shapeType == TopAbs_SOLID ? "SOLID" :
            shapeType == TopAbs_SHELL ? "SHELL" :
            shapeType == TopAbs_FACE ? "FACE" :
            shapeType == TopAbs_WIRE ? "WIRE" :
            shapeType == TopAbs_EDGE ? "EDGE" :
            shapeType == TopAbs_VERTEX ? "VERTEX" : "COMPOUND"
        ));
        
        // For solids, check if all shells are closed
        if (shapeType == TopAbs_SOLID) {
            for (TopExp_Explorer ex(shape, TopAbs_SHELL); ex.More(); ex.Next()) {
                TopoDS_Shell shell = TopoDS::Shell(ex.Current());
                if (!BRep_Tool::IsClosed(shell)) {
                    LOG_WRN("Found open shell in solid");
                    isClosed = false;
                }
            }
        }
        // For shells, check if the shell is closed
        else if (shapeType == TopAbs_SHELL) {
            TopoDS_Shell shell = TopoDS::Shell(shape);
            if (!BRep_Tool::IsClosed(shell)) {
                LOG_WRN("Shell is not closed");
                isClosed = false;
            }
        }
        
        // Check for free edges (edges that belong to only one face)
        int freeEdgeCount = 0;
        TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
        TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
        
        for (int i = 1; i <= edgeFaceMap.Extent(); i++) {
            const TopTools_ListOfShape& faces = edgeFaceMap(i);
            if (faces.Extent() == 1) {
                freeEdgeCount++;
                const TopoDS_Edge& edge = TopoDS::Edge(edgeFaceMap.FindKey(i));
                
                // Get edge endpoints for debugging
                TopoDS_Vertex v1, v2;
                TopExp::Vertices(edge, v1, v2);
                gp_Pnt p1 = BRep_Tool::Pnt(v1);
                gp_Pnt p2 = BRep_Tool::Pnt(v2);
                
                LOG_WRN("Free edge found: (" + std::to_string(p1.X()) + "," + 
                       std::to_string(p1.Y()) + "," + std::to_string(p1.Z()) + ") to (" +
                       std::to_string(p2.X()) + "," + std::to_string(p2.Y()) + "," + 
                       std::to_string(p2.Z()) + ")");
            }
        }
        
        LOG_INF("Free edges count: " + std::to_string(freeEdgeCount));
        
        if (freeEdgeCount > 0) {
            isClosed = false;
        }
        
        // Additional checks using BRepCheck_Analyzer
        BRepCheck_Analyzer analyzer(shape);
        if (!analyzer.IsValid()) {
            LOG_WRN("Shape failed BRepCheck_Analyzer validation");
            isClosed = false;
        }
        
        LOG_INF("Final closure result: " + std::string(isClosed ? "CLOSED" : "OPEN"));
        LOG_INF("=== End Closure Check ===");
        
        return isClosed;
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception checking shape closure: " + std::string(e.what()));
        return false;
    }
}

void OCCShapeBuilder::analyzeShapeProperties(const TopoDS_Shape& shape, const std::string& shapeName)
{
    if (shape.IsNull()) {
        LOG_ERR("Cannot analyze properties of null shape: " + shapeName);
        return;
    }
    
    std::string name = shapeName.empty() ? "Unknown" : shapeName;
    LOG_INF("=== Shape Properties Analysis: " + name + " ===");
    
    try {
        // Basic properties
        double volume = getVolume(shape);
        double surfaceArea = getSurfaceArea(shape);
        
        LOG_INF("Volume: " + std::to_string(volume));
        LOG_INF("Surface Area: " + std::to_string(surfaceArea));
        
        // Bounding box
        gp_Pnt minPt, maxPt;
        getBoundingBox(shape, minPt, maxPt);
        LOG_INF("Bounding Box:");
        LOG_INF("  Min: (" + std::to_string(minPt.X()) + ", " + 
               std::to_string(minPt.Y()) + ", " + std::to_string(minPt.Z()) + ")");
        LOG_INF("  Max: (" + std::to_string(maxPt.X()) + ", " + 
               std::to_string(maxPt.Y()) + ", " + std::to_string(maxPt.Z()) + ")");
        
        // Center of mass
        GProp_GProps props;
        BRepGProp::VolumeProperties(shape, props);
        if (props.Mass() > 0) {
            gp_Pnt centerOfMass = props.CentreOfMass();
            LOG_INF("Center of Mass: (" + std::to_string(centerOfMass.X()) + ", " + 
                   std::to_string(centerOfMass.Y()) + ", " + std::to_string(centerOfMass.Z()) + ")");
        }
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception analyzing shape properties: " + std::string(e.what()));
    }
    
    LOG_INF("=== End Properties Analysis ===");
} 