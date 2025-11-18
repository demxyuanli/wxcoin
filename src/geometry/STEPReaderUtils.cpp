#include "STEPReaderUtils.h"
#include "logger/Logger.h"

#include <OpenCASCADE/GProp_GProps.hxx>
#include <OpenCASCADE/BRepGProp.hxx>
#include <OpenCASCADE/BRepBndLib.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopoDS_Shell.hxx>
#include <OpenCASCADE/ShapeFix_Shell.hxx>

double STEPReaderUtils::safeCalculateArea(const TopoDS_Shape& shape) {
    try {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(shape, props);
        return props.Mass();
    } catch (const std::exception&) {
        return 0.0;
    }
}

double STEPReaderUtils::safeCalculateVolume(const TopoDS_Shape& shape) {
    try {
        GProp_GProps props;
        BRepGProp::VolumeProperties(shape, props);
        return props.Mass();
    } catch (const std::exception&) {
        return 0.0;
    }
}

gp_Pnt STEPReaderUtils::safeCalculateCentroid(const TopoDS_Shape& shape) {
    try {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(shape, props);
        return props.CentreOfMass();
    } catch (const std::exception&) {
        return gp_Pnt(0, 0, 0);
    }
}

Bnd_Box STEPReaderUtils::safeCalculateBoundingBox(const TopoDS_Shape& shape) {
    Bnd_Box box;
    try {
        BRepBndLib::Add(shape, box);
    } catch (const std::exception&) {
        // Return empty box on error
    }
    return box;
}

void STEPReaderUtils::logCount(const std::string& prefix, size_t count, const std::string& suffix) {
    std::string message = prefix + std::to_string(count) + suffix;
    LOG_INF_S(message);
}

void STEPReaderUtils::logSuccess(const std::string& operation, size_t count, const std::string& unit) {
    logCount(operation + " processed ", count, " " + unit);
}

TopoDS_Shape STEPReaderUtils::createCompoundFromShapes(const std::vector<TopoDS_Shape>& shapes) {
    if (shapes.empty()) return TopoDS_Shape();

    if (shapes.size() == 1) return shapes[0];

    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);

    for (const auto& shape : shapes) {
        if (!shape.IsNull()) {
            builder.Add(compound, shape);
        }
    }

    return compound;
}

TopoDS_Shape STEPReaderUtils::tryCreateShellFromFaces(const std::vector<TopoDS_Shape>& faces) {
    try {
        if (faces.empty()) return TopoDS_Shape();

        BRep_Builder builder;
        TopoDS_Shell shell;
        builder.MakeShell(shell);

        for (const auto& face : faces) {
            builder.Add(shell, face);
        }

        // Try to fix and close the shell
        ShapeFix_Shell shellFixer;
        shellFixer.Init(shell);
        shellFixer.SetPrecision(1e-6);
        shellFixer.Perform();

        return shellFixer.Shell();
    } catch (const std::exception& e) {
        LOG_WRN_S("Failed to create shell from faces: " + std::string(e.what()));
        return TopoDS_Shape();
    }
}

