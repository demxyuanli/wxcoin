#include "rendering/Coin3DBackend.h"
#include "rendering/OpenCASCADEProcessor.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoTexture2.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Coin3DBackendImpl::Coin3DBackendImpl() 
    : m_config(RenderConfig::getInstance())
    , m_geometryProcessor(std::make_unique<OpenCASCADEProcessor>()) {
    LOG_INF_S("Coin3DBackendImpl created");
}

Coin3DBackendImpl::~Coin3DBackendImpl() {
    LOG_INF_S("Coin3DBackendImpl destroyed");
}

bool Coin3DBackendImpl::initialize(const std::string& config) {
    LOG_INF_S("Coin3DBackendImpl initialized");
    return true;
}

void Coin3DBackendImpl::shutdown() {
    LOG_INF_S("Coin3DBackendImpl shutdown");
}

SoSeparatorPtr Coin3DBackendImpl::createSceneNode(const TriangleMesh& mesh, bool selected) {
    if (mesh.isEmpty()) {
        LOG_WRN_S("Cannot create Coin3D node from empty mesh");
        return SoSeparatorPtr(nullptr, SoSeparatorDeleter());
    }

    SoSeparator* root = new SoSeparator;
    root->ref();

    // Build common structure
    buildCoinNodeStructure(root, mesh, selected);

    root->unrefNoDelete();
    return SoSeparatorPtr(root, SoSeparatorDeleter());
}

void Coin3DBackendImpl::updateSceneNode(SoSeparator* node, const TriangleMesh& mesh) {
    // This is a placeholder implementation
    LOG_WRN_S("Coin3DBackendImpl::updateSceneNode not fully implemented yet");
}

void Coin3DBackendImpl::updateSceneNode(SoSeparator* node, const TopoDS_Shape& shape, const MeshParameters& params) {
    // This is a placeholder implementation
    LOG_WRN_S("Coin3DBackendImpl::updateSceneNode(shape) not fully implemented yet");
}

SoSeparatorPtr Coin3DBackendImpl::createSceneNode(const TopoDS_Shape& shape, 
                                                 const MeshParameters& params,
                                                 bool selected) {
    if (shape.IsNull()) {
        LOG_WRN_S("Cannot create Coin3D node from null shape");
        return SoSeparatorPtr(nullptr, SoSeparatorDeleter());
    }

    // Convert shape to mesh first
    TriangleMesh mesh;
    if (m_geometryProcessor) {
        mesh = m_geometryProcessor->convertToMesh(shape, params);
    } else {
        LOG_ERR_S("No geometry processor available");
        return SoSeparatorPtr(nullptr, SoSeparatorDeleter());
    }

    // Create scene node from mesh
    return createSceneNode(mesh, selected);
}

void Coin3DBackendImpl::setEdgeSettings(bool show, double angle) {
    m_config.getEdgeSettings().showEdges = show;
    m_config.getEdgeSettings().featureEdgeAngle = angle;
}

void Coin3DBackendImpl::setSmoothingSettings(bool enabled, double creaseAngle, int iterations) {
    m_config.getSmoothingSettings().enabled = enabled;
    m_config.getSmoothingSettings().creaseAngle = creaseAngle;
    m_config.getSmoothingSettings().iterations = iterations;
}

void Coin3DBackendImpl::setSubdivisionSettings(bool enabled, int levels) {
    m_config.getSubdivisionSettings().enabled = enabled;
    m_config.getSubdivisionSettings().levels = levels;
}

bool Coin3DBackendImpl::isAvailable() const {
    return true; // Placeholder
}

SoSeparator* Coin3DBackendImpl::createCoinNode(const TriangleMesh& mesh, bool selected) {
    if (mesh.isEmpty()) {
        LOG_WRN_S("Cannot create Coin3D node from empty mesh");
        return nullptr;
    }

    SoSeparator* root = new SoSeparator;
    root->ref();

    // Build common structure
    buildCoinNodeStructure(root, mesh, selected);

    root->unrefNoDelete();
    return root;
}

void Coin3DBackendImpl::updateCoinNode(SoSeparator* node, const TriangleMesh& mesh) {
    // This is a placeholder implementation
    LOG_WRN_S("Coin3DBackendImpl::updateCoinNode not fully implemented yet");
}

SoCoordinate3* Coin3DBackendImpl::createCoordinateNode(const TriangleMesh& mesh) {
    if (mesh.vertices.empty()) {
        return nullptr;
    }

    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(static_cast<int>(mesh.vertices.size()));

    SbVec3f* points = coords->point.startEditing();
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const gp_Pnt& vertex = mesh.vertices[i];
        points[i].setValue(
            static_cast<float>(vertex.X()),
            static_cast<float>(vertex.Y()),
            static_cast<float>(vertex.Z())
        );
    }
    coords->point.finishEditing();

    return coords;
}

SoIndexedFaceSet* Coin3DBackendImpl::createFaceSetNode(const TriangleMesh& mesh) {
    if (mesh.triangles.empty()) {
        return nullptr;
    }

    SoIndexedFaceSet* faceSet = new SoIndexedFaceSet;

    // Set up coordinate indices
    int numIndices = static_cast<int>(mesh.triangles.size()) + mesh.getTriangleCount(); // +1 for each triangle separator
    faceSet->coordIndex.setNum(numIndices);

    int32_t* indices = faceSet->coordIndex.startEditing();
    int indexPos = 0;

    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        indices[indexPos++] = mesh.triangles[i];
        indices[indexPos++] = mesh.triangles[i + 1];
        indices[indexPos++] = mesh.triangles[i + 2];
        indices[indexPos++] = -1; // Triangle separator
    }

    faceSet->coordIndex.finishEditing();

    return faceSet;
}

SoNormal* Coin3DBackendImpl::createNormalNode(const TriangleMesh& mesh) {
    if (mesh.normals.empty()) {
        return nullptr;
    }

    SoNormal* normals = new SoNormal;
    normals->vector.setNum(static_cast<int>(mesh.normals.size()));

    SbVec3f* normalVecs = normals->vector.startEditing();
    for (size_t i = 0; i < mesh.normals.size(); i++) {
        const gp_Vec& normal = mesh.normals[i];
        normalVecs[i].setValue(
            static_cast<float>(normal.X()),
            static_cast<float>(normal.Y()),
            static_cast<float>(normal.Z())
        );
    }
    normals->vector.finishEditing();

    return normals;
}

SoIndexedLineSet* Coin3DBackendImpl::createEdgeSetNode(const TriangleMesh& mesh) {
    if (mesh.triangles.empty()) {
        return nullptr;
    }

    // Simple edge set - just show all triangle edges
    std::vector<int32_t> indices;
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v0 = mesh.triangles[i];
        int v1 = mesh.triangles[i + 1];
        int v2 = mesh.triangles[i + 2];
        
        // Add edges
        indices.push_back(v0);
        indices.push_back(v1);
        indices.push_back(SO_END_LINE_INDEX);
        
        indices.push_back(v1);
        indices.push_back(v2);
        indices.push_back(SO_END_LINE_INDEX);
        
        indices.push_back(v2);
        indices.push_back(v0);
        indices.push_back(SO_END_LINE_INDEX);
    }

    if (indices.empty()) {
        return nullptr;
    }
    
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
    return lineSet;
}

void Coin3DBackendImpl::buildCoinNodeStructure(SoSeparator* node, const TriangleMesh& mesh, bool selected) {
    if (!node || mesh.isEmpty()) {
        return;
    }

    // Add shape hints
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
    hints->creaseAngle = static_cast<float>(m_config.getSmoothingSettings().creaseAngle * M_PI / 180.0);
    node->addChild(hints);

    // Add coordinate node
    SoCoordinate3* coords = createCoordinateNode(mesh);
    if (coords) {
        node->addChild(coords);
    }

    // Add normal node with binding
    if (!mesh.normals.empty()) {
        SoNormal* normals = createNormalNode(mesh);
        if (normals) {
            node->addChild(normals);
            
            SoNormalBinding* binding = new SoNormalBinding;
            binding->value = SoNormalBinding::PER_VERTEX_INDEXED;
            node->addChild(binding);
        }
    }

    // Add face set
    SoIndexedFaceSet* faceSet = createFaceSetNode(mesh);
    if (faceSet) {
        node->addChild(faceSet);
    }

    // Add edge set if enabled
    if (m_config.getEdgeSettings().showEdges) {
        SoSeparator* edgeGroup = new SoSeparator;
        SoTexture2* disableTexture = new SoTexture2;
        edgeGroup->addChild(disableTexture);

        SoMaterial* edgeMaterial = new SoMaterial;
        if (m_config.getEdgeSettings().edgeColorEnabled) {
            Quantity_Color edgeColor = m_config.getEdgeSettings().edgeColor;
            edgeMaterial->diffuseColor.setValue(edgeColor.Red(), edgeColor.Green(), edgeColor.Blue());
            edgeMaterial->emissiveColor.setValue(edgeColor.Red() * 0.5f, edgeColor.Green() * 0.5f, edgeColor.Blue() * 0.5f);
        } else {
            edgeMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.0f);
            edgeMaterial->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
        }
        edgeGroup->addChild(edgeMaterial);

        SoIndexedLineSet* edgeSet = createEdgeSetNode(mesh);
        if (edgeSet) {
            edgeGroup->addChild(edgeSet);
        }

        node->addChild(edgeGroup);
    }
} 