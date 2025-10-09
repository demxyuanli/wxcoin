#include "geometry/OCCGeometryCore.h"
#include "logger/Logger.h"
#include <Standard_Failure.hxx>

OCCGeometryCore::OCCGeometryCore(const std::string& name)
    : m_name(name)
{
    LOG_INF_S("Created OCCGeometryCore: " + m_name);
}

void OCCGeometryCore::setShape(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        LOG_WRN_S("Attempting to set null shape for geometry: " + m_name);
        return;
    }
    
    m_shape = shape;
    LOG_INF_S("Shape set for geometry: " + m_name);
}
