#include "geometry/VertexExtractor.h"
#include "logger/Logger.h"
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoPointSet.h>
#include <set>

VertexExtractor::VertexExtractor()
    : m_cacheValid(false)
{
}

VertexExtractor::~VertexExtractor()
{
    clearCache();
}

size_t VertexExtractor::extractAndCache(const TopoDS_Shape& shape)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear previous cache
    m_cachedVertices.clear();
    m_cacheValid = false;
    
    if (shape.IsNull()) {
        LOG_WRN_S("VertexExtractor::extractAndCache: Shape is null");
        return 0;
    }
    
    try {
        // Use a set to track unique vertices (based on rounded coordinates)
        std::set<std::tuple<double, double, double>> uniquePoints;
        
        // Extract all vertices from the shape
        for (TopExp_Explorer exp(shape, TopAbs_VERTEX); exp.More(); exp.Next()) {
            const TopoDS_Vertex& vertex = TopoDS::Vertex(exp.Current());
            
            if (vertex.IsNull()) continue;
            
            gp_Pnt point = BRep_Tool::Pnt(vertex);
            
            // Round coordinates for deduplication (tolerance: 0.0001)
            double x = std::round(point.X() * 10000.0) / 10000.0;
            double y = std::round(point.Y() * 10000.0) / 10000.0;
            double z = std::round(point.Z() * 10000.0) / 10000.0;
            
            auto key = std::make_tuple(x, y, z);
            
            // Only add unique vertices
            if (uniquePoints.find(key) == uniquePoints.end()) {
                uniquePoints.insert(key);
                m_cachedVertices.push_back(point);
            }
        }
        
        m_cacheValid = true;
        
        LOG_INF_S("VertexExtractor: Cached " + std::to_string(m_cachedVertices.size()) + 
                  " unique vertices from shape");
        
        return m_cachedVertices.size();
        
    } catch (const std::exception& e) {
        LOG_ERR_S("VertexExtractor::extractAndCache: Exception: " + std::string(e.what()));
        clearCache();
        return 0;
    }
}

SoSeparator* VertexExtractor::createPointNode(const Quantity_Color& color, double pointSize) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_cacheValid || m_cachedVertices.empty()) {
        LOG_WRN_S("VertexExtractor::createPointNode: No cached vertex data available");
        return nullptr;
    }
    
    try {
        // Create root node
        SoSeparator* pointNode = new SoSeparator();
        pointNode->ref();
        
        // Add material for point color
        SoMaterial* material = new SoMaterial();
        material->diffuseColor.setValue(
            static_cast<float>(color.Red()),
            static_cast<float>(color.Green()),
            static_cast<float>(color.Blue())
        );
        pointNode->addChild(material);
        
        // Add draw style for point size
        SoDrawStyle* drawStyle = new SoDrawStyle();
        drawStyle->pointSize.setValue(static_cast<float>(pointSize));
        pointNode->addChild(drawStyle);
        
        // Add coordinates
        SoCoordinate3* coords = new SoCoordinate3();
        coords->point.setNum(m_cachedVertices.size());
        for (size_t i = 0; i < m_cachedVertices.size(); ++i) {
            const gp_Pnt& pt = m_cachedVertices[i];
            coords->point.set1Value(i,
                static_cast<float>(pt.X()),
                static_cast<float>(pt.Y()),
                static_cast<float>(pt.Z())
            );
        }
        pointNode->addChild(coords);
        
        // Add point set
        SoPointSet* pointSet = new SoPointSet();
        pointSet->numPoints.setValue(m_cachedVertices.size());
        pointNode->addChild(pointSet);
        
        LOG_INF_S("VertexExtractor: Created point node with " + 
                  std::to_string(m_cachedVertices.size()) + " points");
        
        return pointNode;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("VertexExtractor::createPointNode: Exception: " + std::string(e.what()));
        return nullptr;
    }
}

bool VertexExtractor::hasCache() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cacheValid && !m_cachedVertices.empty();
}

size_t VertexExtractor::getCachedCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cachedVertices.size();
}

void VertexExtractor::clearCache()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cachedVertices.clear();
    m_cacheValid = false;
}

bool VertexExtractor::isDuplicate(const gp_Pnt& point, double tolerance) const
{
    for (const auto& cached : m_cachedVertices) {
        if (cached.Distance(point) < tolerance) {
            return true;
        }
    }
    return false;
}



