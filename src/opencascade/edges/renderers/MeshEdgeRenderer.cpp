#include "edges/renderers/MeshEdgeRenderer.h"
#include "rendering/GeometryProcessor.h"
#include "rendering/GPUEdgeRenderer.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>

MeshEdgeRenderer::MeshEdgeRenderer()
    : meshEdgeNode(nullptr)
    , normalLineNode(nullptr)
    , faceNormalLineNode(nullptr)
    , m_gpuRenderer(nullptr)
    , m_gpuAccelerationEnabled(false)
    , m_gpuMeshEdgeNode(nullptr) {

    // Initialize GPU renderer if available
    try {
        m_gpuRenderer = new GPUEdgeRenderer();
        if (m_gpuRenderer && m_gpuRenderer->initialize()) {
            m_gpuAccelerationEnabled = true;
            LOG_INF_S("GPU mesh edge renderer initialized successfully");
        } else {
            m_gpuAccelerationEnabled = false;
            LOG_WRN_S("GPU mesh edge renderer initialization failed, using CPU fallback");
        }
    } catch (const std::exception& e) {
        LOG_WRN_S("Failed to initialize GPU mesh edge renderer: " + std::string(e.what()));
        m_gpuAccelerationEnabled = false;
    }
}

MeshEdgeRenderer::~MeshEdgeRenderer() {
    if (meshEdgeNode) {
        try {
            meshEdgeNode->unref();
        } catch (...) {
            // Node was already deleted or corrupted
            LOG_WRN_S("Warning: meshEdgeNode was corrupted during destruction");
        }
        meshEdgeNode = nullptr;
    }
    if (normalLineNode) {
        try {
            normalLineNode->unref();
        } catch (...) {
            // Node was already deleted or corrupted
            LOG_WRN_S("Warning: normalLineNode was corrupted during destruction");
        }
        normalLineNode = nullptr;
    }
    if (faceNormalLineNode) {
        try {
            faceNormalLineNode->unref();
        } catch (...) {
            // Node was already deleted or corrupted
            LOG_WRN_S("Warning: faceNormalLineNode was corrupted during destruction");
        }
        faceNormalLineNode = nullptr;
    }
    if (m_gpuRenderer) {
        delete m_gpuRenderer;
        m_gpuRenderer = nullptr;
    }
    if (m_gpuMeshEdgeNode) {
        m_gpuMeshEdgeNode->unref();
        m_gpuMeshEdgeNode = nullptr;
    }
}

SoSeparator* MeshEdgeRenderer::generateNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width,
    int style) {

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Note: We don't clean up existing node here - that's the caller's responsibility
    // to avoid double unref() calls. We just clear our internal pointer.
    meshEdgeNode = nullptr;

    if (points.empty()) return nullptr;

    // CPU fallback: use SoIndexedLineSet (slower but compatible)
    meshEdgeNode = createLineNode(points, color, width, style);
    return meshEdgeNode;
}

SoSeparator* MeshEdgeRenderer::generateNodeFromMesh(
    const TriangleMesh& mesh,
    const Quantity_Color& color,
    double width) {
    
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    // Clean up existing GPU node
    if (m_gpuMeshEdgeNode) {
        m_gpuMeshEdgeNode->unref();
        m_gpuMeshEdgeNode = nullptr;
    }
    
    if (mesh.vertices.empty() || mesh.triangles.empty()) {
        return nullptr;
    }
    
    // Try GPU acceleration first
    if (m_gpuAccelerationEnabled && m_gpuRenderer && m_gpuRenderer->isAvailable()) {
        GPUEdgeRenderer::EdgeRenderSettings settings;
        settings.color = color;
        settings.lineWidth = static_cast<float>(width);
        settings.depthOffset = 0.0001f;
        settings.antiAliasing = true;
        settings.depthTest = true;
        settings.mode = GPUEdgeRenderer::RenderMode::GeometryShader;
        
        m_gpuMeshEdgeNode = m_gpuRenderer->createGPUEdgeNode(mesh, settings);
        if (m_gpuMeshEdgeNode) {
            LOG_DBG_S("Using GPU-accelerated mesh edge rendering");
            return m_gpuMeshEdgeNode;
        }
    }
    
    // CPU fallback: extract edges and use SoIndexedLineSet
    // Note: This will use the deduplicated edge extraction
    LOG_DBG_S("Using CPU fallback for mesh edge rendering");
    return nullptr; // Caller should use generateNode with extracted points
}

void MeshEdgeRenderer::updateAppearance(
    SoSeparator* node,
    const Quantity_Color& color,
    double width,
    int style) {

    if (!node) return;

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Update material and draw style
    for (int i = 0; i < node->getNumChildren(); ++i) {
        SoNode* child = node->getChild(i);
        if (child->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(child);
            material->diffuseColor.setValue(
                static_cast<float>(color.Red()),
                static_cast<float>(color.Green()),
                static_cast<float>(color.Blue())
            );
        } else if (child->isOfType(SoDrawStyle::getClassTypeId())) {
            SoDrawStyle* drawStyle = static_cast<SoDrawStyle*>(child);
            drawStyle->lineWidth.setValue(static_cast<float>(width));
        }
    }
}

SoSeparator* MeshEdgeRenderer::generateNormalLineNode(
    const TriangleMesh& mesh,
    double length,
    const Quantity_Color& color) {

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Note: We don't clean up existing node here - that's the caller's responsibility
    // to avoid double unref() calls. We just clear our internal pointer.
    normalLineNode = nullptr;

    if (mesh.vertices.empty() || mesh.normals.empty()) return nullptr;

    normalLineNode = new SoSeparator();
    normalLineNode->ref();

    // Material for normal lines
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    normalLineNode->addChild(material);

    // Generate vertex normal lines
    std::vector<gp_Pnt> normalPoints;
    size_t numNormals = std::min(mesh.vertices.size(), mesh.normals.size());

    for (size_t i = 0; i < numNormals; ++i) {
        const gp_Pnt& vertex = mesh.vertices[i];
        const gp_Vec& normal = mesh.normals[i];

        // Skip zero-length normals
        if (normal.Magnitude() < 1e-7) continue;

        // Create normal line from vertex
        normalPoints.push_back(vertex);
        normalPoints.push_back(vertex.Translated(normal * length));
    }

    if (normalPoints.empty()) {
        // Clean up the node we just created since we can't use it
        if (normalLineNode) {
            try {
                normalLineNode->unref();
            } catch (...) {
                // Node was corrupted
                LOG_WRN_S("Warning: normalLineNode was corrupted when no normals found");
            }
            normalLineNode = nullptr;
        }
        return nullptr;
    }

    // Create coordinate and line set nodes
    SoCoordinate3* coords = new SoCoordinate3();
    coords->point.setNum(static_cast<int>(normalPoints.size()));
    for (size_t i = 0; i < normalPoints.size(); ++i) {
        coords->point.set1Value(static_cast<int>(i),
            static_cast<float>(normalPoints[i].X()),
            static_cast<float>(normalPoints[i].Y()),
            static_cast<float>(normalPoints[i].Z()));
    }
    normalLineNode->addChild(coords);

    SoIndexedLineSet* lineSet = new SoIndexedLineSet();
    int coordIndex = 0;
    for (size_t i = 0; i < normalPoints.size(); i += 2) {
        lineSet->coordIndex.set1Value(coordIndex++, static_cast<int>(i));
        lineSet->coordIndex.set1Value(coordIndex++, static_cast<int>(i + 1));
        lineSet->coordIndex.set1Value(coordIndex++, -1);
    }
    normalLineNode->addChild(lineSet);

    return normalLineNode;
}

SoSeparator* MeshEdgeRenderer::generateFaceNormalLineNode(
    const TriangleMesh& mesh,
    double length,
    const Quantity_Color& color) {

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Note: We don't clean up existing node here - that's the caller's responsibility
    // to avoid double unref() calls. We just clear our internal pointer.
    faceNormalLineNode = nullptr;

    if (mesh.vertices.empty() || mesh.triangles.empty()) return nullptr;

    faceNormalLineNode = new SoSeparator();
    faceNormalLineNode->ref();

    // Material for face normal lines
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    faceNormalLineNode->addChild(material);

    // Generate face normal lines for each triangle
    std::vector<gp_Pnt> normalPoints;

    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        if (i + 2 >= mesh.triangles.size()) break;

        int v1 = mesh.triangles[i];
        int v2 = mesh.triangles[i + 1];
        int v3 = mesh.triangles[i + 2];

        if (v1 >= static_cast<int>(mesh.vertices.size()) ||
            v2 >= static_cast<int>(mesh.vertices.size()) ||
            v3 >= static_cast<int>(mesh.vertices.size())) continue;

        // Calculate triangle center
        double x = (mesh.vertices[v1].X() + mesh.vertices[v2].X() + mesh.vertices[v3].X()) / 3.0;
        double y = (mesh.vertices[v1].Y() + mesh.vertices[v2].Y() + mesh.vertices[v3].Y()) / 3.0;
        double z = (mesh.vertices[v1].Z() + mesh.vertices[v2].Z() + mesh.vertices[v3].Z()) / 3.0;
        gp_Pnt center(x, y, z);

        // Calculate face normal using cross product of two edges
        gp_Vec edge1(mesh.vertices[v1], mesh.vertices[v2]);
        gp_Vec edge2(mesh.vertices[v1], mesh.vertices[v3]);
        gp_Vec normal = edge1.Crossed(edge2);

        // Skip degenerate triangles
        if (normal.Magnitude() < 1e-7) continue;

        normal.Normalize();

        // Create normal line from triangle center
        normalPoints.push_back(center);
        normalPoints.push_back(center.Translated(normal * length));
    }

    if (normalPoints.empty()) {
        // Clean up the node we just created since we can't use it
        if (faceNormalLineNode) {
            try {
                faceNormalLineNode->unref();
            } catch (...) {
                // Node was corrupted
                LOG_WRN_S("Warning: faceNormalLineNode was corrupted when no face normals found");
            }
            faceNormalLineNode = nullptr;
        }
        return nullptr;
    }

    // Create coordinate and line set nodes
    SoCoordinate3* coords = new SoCoordinate3();
    coords->point.setNum(static_cast<int>(normalPoints.size()));
    for (size_t i = 0; i < normalPoints.size(); ++i) {
        coords->point.set1Value(static_cast<int>(i),
            static_cast<float>(normalPoints[i].X()),
            static_cast<float>(normalPoints[i].Y()),
            static_cast<float>(normalPoints[i].Z()));
    }
    faceNormalLineNode->addChild(coords);

    SoIndexedLineSet* lineSet = new SoIndexedLineSet();
    int coordIndex = 0;
    for (size_t i = 0; i < normalPoints.size(); i += 2) {
        lineSet->coordIndex.set1Value(coordIndex++, static_cast<int>(i));
        lineSet->coordIndex.set1Value(coordIndex++, static_cast<int>(i + 1));
        lineSet->coordIndex.set1Value(coordIndex++, -1);
    }
    faceNormalLineNode->addChild(lineSet);

    return faceNormalLineNode;
}

void MeshEdgeRenderer::clearMeshEdgeNode() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Note: We don't call unref() here because the owner (ModularEdgeComponent)
    // is responsible for managing the node's reference count.
    // We just clear our internal pointer.
    meshEdgeNode = nullptr;

    // Same for GPU node
    m_gpuMeshEdgeNode = nullptr;

    LOG_DBG_S("MeshEdgeRenderer: Cleared mesh edge nodes (reference counting handled by owner)");
}
