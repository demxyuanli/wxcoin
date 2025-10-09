#include "geometry/OCCGeometryMesh.h"
#include "EdgeComponent.h"
#include "logger/Logger.h"
#include "rendering/RenderingToolkitAPI.h"
#include "OCCMeshConverter.h"
#include <Inventor/nodes/SoSeparator.h>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

OCCGeometryMesh::OCCGeometryMesh()
    : m_coinNode(nullptr)
    , m_coinNeedsUpdate(true)
    , m_meshRegenerationNeeded(true)
    , m_assemblyLevel(0)
{
    edgeComponent = std::make_unique<EdgeComponent>();
}

OCCGeometryMesh::~OCCGeometryMesh()
{
    if (m_coinNode) {
        m_coinNode->unref();
        m_coinNode = nullptr;
    }
}

void OCCGeometryMesh::setCoinNode(SoSeparator* node)
{
    if (m_coinNode) {
        m_coinNode->unref();
    }
    m_coinNode = node;
    if (m_coinNode) {
        m_coinNode->ref();
    }
}

void OCCGeometryMesh::regenerateMesh(const TopoDS_Shape& shape, const MeshParameters& params)
{
    LOG_INF_S("Regenerating mesh");
    m_meshRegenerationNeeded = true;
    m_lastMeshParams = params;
    buildCoinRepresentation(shape, params);
}

void OCCGeometryMesh::buildCoinRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    // This is a framework implementation
    // The actual implementation should be migrated from OCCGeometry.cpp buildCoinRepresentation()
    
    LOG_INF_S("Building Coin3D representation");
    
    if (shape.IsNull()) {
        LOG_WRN_S("Cannot build coin representation for null shape");
        return;
    }
    
    // Create or clear coin node
    if (!m_coinNode) {
        m_coinNode = new SoSeparator();
        m_coinNode->ref();
    } else {
        m_coinNode->removeAllChildren();
    }
    
    // TODO: Migrate mesh generation code from OCCGeometry.cpp
    // This includes:
    // 1. Tessellation/mesh generation using GeometryProcessor
    // 2. Material application
    // 3. Texture application
    // 4. Transform application
    // 5. Coin3D node construction
    
    m_coinNeedsUpdate = false;
    m_meshRegenerationNeeded = false;
    m_lastMeshParams = params;
    
    LOG_INF_S("Coin representation built (framework)");
}

void OCCGeometryMesh::buildCoinRepresentation(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const Quantity_Color& diffuseColor,
    const Quantity_Color& ambientColor,
    const Quantity_Color& specularColor,
    const Quantity_Color& emissiveColor,
    double shininess,
    double transparency)
{
    // Framework implementation with material parameters
    LOG_INF_S("Building Coin representation with explicit materials");
    
    // TODO: Implement material-aware mesh generation
    // This version takes material parameters directly
    
    buildCoinRepresentation(shape, params);
}

void OCCGeometryMesh::updateCoinRepresentationIfNeeded(const TopoDS_Shape& shape, const MeshParameters& params)
{
    if (m_meshRegenerationNeeded || m_coinNeedsUpdate) {
        buildCoinRepresentation(shape, params);
    }
}

void OCCGeometryMesh::forceCoinRepresentationRebuild(const TopoDS_Shape& shape, const MeshParameters& params)
{
    m_meshRegenerationNeeded = true;
    m_coinNeedsUpdate = true;
    buildCoinRepresentation(shape, params);
}

void OCCGeometryMesh::setEdgeDisplayType(EdgeType type, bool show)
{
    if (edgeComponent) {
        edgeComponent->setEdgeDisplayType(type, show);
    }
}

bool OCCGeometryMesh::isEdgeDisplayTypeEnabled(EdgeType type) const
{
    return edgeComponent ? edgeComponent->isEdgeDisplayTypeEnabled(type) : false;
}

void OCCGeometryMesh::updateEdgeDisplay()
{
    if (edgeComponent && m_coinNode) {
        edgeComponent->updateEdgeDisplay(m_coinNode);
    }
}

bool OCCGeometryMesh::hasOriginalEdges() const
{
    return edgeComponent && edgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Original);
}

int OCCGeometryMesh::getGeometryFaceIdForTriangle(int triangleIndex) const
{
    if (!hasFaceIndexMapping()) {
        return -1;
    }

    for (const auto& mapping : m_faceIndexMappings) {
        auto it = std::find(mapping.triangleIndices.begin(), mapping.triangleIndices.end(), triangleIndex);
        if (it != mapping.triangleIndices.end()) {
            return mapping.geometryFaceId;
        }
    }

    return -1;
}

std::vector<int> OCCGeometryMesh::getTrianglesForGeometryFace(int geometryFaceId) const
{
    if (!hasFaceIndexMapping()) {
        return {};
    }

    for (const auto& mapping : m_faceIndexMappings) {
        if (mapping.geometryFaceId == geometryFaceId) {
            return mapping.triangleIndices;
        }
    }

    return {};
}

void OCCGeometryMesh::buildFaceIndexMapping(const TopoDS_Shape& shape, const MeshParameters& params)
{
    try {
        if (shape.IsNull()) {
            LOG_WRN_S("Cannot build face index mapping for null shape");
            return;
        }

        m_faceIndexMappings.clear();

        // Extract all faces from the shape
        std::vector<TopoDS_Face> faces;
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            faces.push_back(TopoDS::Face(exp.Current()));
        }

        if (faces.empty()) {
            LOG_WRN_S("No faces found in shape for index mapping");
            return;
        }

        LOG_INF_S("Building face index mapping for " + std::to_string(faces.size()) + " faces");

        // TODO: Implement face mapping using GeometryProcessor
        // This requires integration with the mesh generation code

        LOG_INF_S("Face index mapping built (framework)");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to build face index mapping: " + std::string(e.what()));
        m_faceIndexMappings.clear();
    }
}

void OCCGeometryMesh::releaseTemporaryData()
{
    // Release any temporary mesh generation data
    LOG_INF_S("Releasing temporary mesh data");
}

void OCCGeometryMesh::optimizeMemory()
{
    // Optimize memory usage
    m_faceIndexMappings.shrink_to_fit();
    LOG_INF_S("Memory optimized");
}

void OCCGeometryMesh::createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    // TODO: Implement wireframe representation
    LOG_INF_S("Creating wireframe representation (framework)");
}
