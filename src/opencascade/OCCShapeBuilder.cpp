#include "OCCShapeBuilder.h"
#include "Logger.h"

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
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
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <gp_Trsf.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
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