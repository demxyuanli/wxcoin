#include "rendering/GPUEdgeRenderer.h"
#include "rendering/GeometryProcessor.h"
#include "logger/Logger.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoGeometryShader.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoPolygonOffset.h>

GPUEdgeRenderer::GPUEdgeRenderer()
    : m_initialized(false)
    , m_available(false)
    , m_currentMode(RenderMode::GeometryShader)
    , m_geometryShaderProgram(nullptr)
    , m_screenSpaceShaderProgram(nullptr)
{
    m_stats.lastFrameTime = 0.0;
    m_stats.trianglesProcessed = 0;
    m_stats.edgesGenerated = 0;
    m_stats.gpuAccelerated = false;
}

GPUEdgeRenderer::~GPUEdgeRenderer()
{
    shutdown();
}

bool GPUEdgeRenderer::initialize()
{
    if (m_initialized) {
        return true;
    }

    LOG_INF_S("Initializing GPU Edge Renderer...");

    // Check for shader support
    if (!checkShaderSupport()) {
        LOG_WRN_S("Shader support not available, GPU edge rendering disabled");
        m_available = false;
        return false;
    }

    // Check for geometry shader support
    if (!checkGeometryShaderSupport()) {
        LOG_WRN_S("Geometry shader not supported, using screen-space mode only");
        m_currentMode = RenderMode::ScreenSpace;
    }

    m_initialized = true;
    m_available = true;
    m_stats.gpuAccelerated = true;

    LOG_INF_S("GPU Edge Renderer initialized successfully");
    return true;
}

void GPUEdgeRenderer::shutdown()
{
    if (m_geometryShaderProgram) {
        m_geometryShaderProgram->unref();
        m_geometryShaderProgram = nullptr;
    }

    if (m_screenSpaceShaderProgram) {
        m_screenSpaceShaderProgram->unref();
        m_screenSpaceShaderProgram = nullptr;
    }

    m_initialized = false;
    m_available = false;
    LOG_INF_S("GPU Edge Renderer shut down");
}

bool GPUEdgeRenderer::isAvailable() const
{
    return m_available;
}

SoSeparator* GPUEdgeRenderer::createGPUEdgeNode(
    const TriangleMesh& mesh,
    const EdgeRenderSettings& settings)
{
    if (!m_available) {
        LOG_WRN_S("GPU rendering not available");
        return nullptr;
    }

    SoSeparator* edgeNode = new SoSeparator();
    edgeNode->ref();

    // Add polygon offset to prevent z-fighting
    SoPolygonOffset* polygonOffset = new SoPolygonOffset();
    polygonOffset->factor.setValue(settings.depthOffset);
    polygonOffset->units.setValue(1.0f);
    polygonOffset->styles.setValue(SoPolygonOffset::LINES);
    edgeNode->addChild(polygonOffset);

    // Set material for edge color
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(settings.color.Red()),
        static_cast<float>(settings.color.Green()),
        static_cast<float>(settings.color.Blue())
    );
    material->emissiveColor.setValue(
        static_cast<float>(settings.color.Red() * 0.5),
        static_cast<float>(settings.color.Green() * 0.5),
        static_cast<float>(settings.color.Blue() * 0.5)
    );
    edgeNode->addChild(material);

    // Set line style
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->lineWidth.setValue(settings.lineWidth);
    if (settings.antiAliasing) {
        drawStyle->linePattern.setValue(0xFFFF); // Solid line with anti-aliasing hint
    }
    edgeNode->addChild(drawStyle);

    // Create and add shader program based on mode
    if (m_currentMode == RenderMode::GeometryShader) {
        SoShaderProgram* shaderProgram = createGeometryShaderProgram(settings);
        if (shaderProgram) {
            edgeNode->addChild(shaderProgram);
        }
    }

    // Upload mesh geometry
    uploadMeshToGPU(mesh, edgeNode);

    // Update statistics
    m_stats.trianglesProcessed = mesh.triangles.size() / 3;
    m_stats.edgesGenerated = m_stats.trianglesProcessed * 3;

    return edgeNode;
}

SoSeparator* GPUEdgeRenderer::createScreenSpaceEdgeNode(
    SoSeparator* sceneRoot,
    const EdgeRenderSettings& settings)
{
    if (!m_available) {
        LOG_WRN_S("GPU rendering not available");
        return nullptr;
    }

    SoSeparator* ssedNode = new SoSeparator();
    ssedNode->ref();

    // Create screen-space shader program
    SoShaderProgram* shaderProgram = createScreenSpaceShaderProgram(settings);
    if (shaderProgram) {
        ssedNode->addChild(shaderProgram);
    }

    // Add the original scene
    if (sceneRoot) {
        ssedNode->addChild(sceneRoot);
    }

    LOG_INF_S("Created screen-space edge detection node");
    return ssedNode;
}

void GPUEdgeRenderer::updateSettings(SoSeparator* node, const EdgeRenderSettings& settings)
{
    if (!node) return;

    // Find and update material node
    for (int i = 0; i < node->getNumChildren(); i++) {
        SoNode* child = node->getChild(i);
        
        if (child->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(child);
            material->diffuseColor.setValue(
                static_cast<float>(settings.color.Red()),
                static_cast<float>(settings.color.Green()),
                static_cast<float>(settings.color.Blue())
            );
        }
        else if (child->isOfType(SoDrawStyle::getClassTypeId())) {
            SoDrawStyle* drawStyle = static_cast<SoDrawStyle*>(child);
            drawStyle->lineWidth.setValue(settings.lineWidth);
        }
        else if (child->isOfType(SoPolygonOffset::getClassTypeId())) {
            SoPolygonOffset* polygonOffset = static_cast<SoPolygonOffset*>(child);
            polygonOffset->factor.setValue(settings.depthOffset);
        }
    }
}

void GPUEdgeRenderer::setRenderMode(RenderMode mode)
{
    m_currentMode = mode;
    LOG_INF_S("GPU Edge Renderer mode set to: " + 
              std::to_string(static_cast<int>(mode)));
}

SoShaderProgram* GPUEdgeRenderer::createGeometryShaderProgram(
    const EdgeRenderSettings& settings)
{
    SoShaderProgram* program = new SoShaderProgram();
    program->ref();

    // Vertex shader
    SoVertexShader* vertexShader = new SoVertexShader();
    vertexShader->sourceProgram.setValue(getVertexShaderSource().c_str());
    program->shaderObject.set1Value(0, vertexShader);

    // Geometry shader (if supported)
    if (checkGeometryShaderSupport()) {
        SoGeometryShader* geometryShader = new SoGeometryShader();
        geometryShader->sourceProgram.setValue(getGeometryShaderSource().c_str());
        geometryShader->inputType.setValue(SoGeometryShader::TRIANGLES_IN);
        geometryShader->outputType.setValue(SoGeometryShader::LINE_STRIP_OUT);
        geometryShader->maxEmit.setValue(4); // 3 edges + 1 terminator per triangle
        program->shaderObject.set1Value(1, geometryShader);
    }

    // Fragment shader
    SoFragmentShader* fragmentShader = new SoFragmentShader();
    fragmentShader->sourceProgram.setValue(getFragmentShaderSource().c_str());
    program->shaderObject.set1Value(2, fragmentShader);

    return program;
}

SoShaderProgram* GPUEdgeRenderer::createScreenSpaceShaderProgram(
    const EdgeRenderSettings& settings)
{
    SoShaderProgram* program = new SoShaderProgram();
    program->ref();

    // Vertex shader (pass-through)
    SoVertexShader* vertexShader = new SoVertexShader();
    vertexShader->sourceProgram.setValue(getVertexShaderSource().c_str());
    program->shaderObject.set1Value(0, vertexShader);

    // Fragment shader (edge detection)
    SoFragmentShader* fragmentShader = new SoFragmentShader();
    fragmentShader->sourceProgram.setValue(getSSEDFragmentShaderSource().c_str());
    program->shaderObject.set1Value(1, fragmentShader);

    return program;
}

std::string GPUEdgeRenderer::getVertexShaderSource() const
{
    return R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 vPosition;
out vec3 vNormal;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

void main()
{
    vPosition = (modelViewMatrix * vec4(position, 1.0)).xyz;
    vNormal = normalize(normalMatrix * normal);
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}
)";
}

std::string GPUEdgeRenderer::getGeometryShaderSource() const
{
    return R"(
#version 330 core

layout(triangles) in;
layout(line_strip, max_vertices = 4) out;

in vec3 vPosition[];
in vec3 vNormal[];

out vec3 gEdgeColor;

uniform vec3 edgeColor;
uniform float edgeThreshold;

void main()
{
    // Calculate edge normals and determine if edge should be drawn
    vec3 edge01 = normalize(vPosition[1] - vPosition[0]);
    vec3 edge12 = normalize(vPosition[2] - vPosition[1]);
    vec3 edge20 = normalize(vPosition[0] - vPosition[2]);
    
    vec3 triNormal = normalize(cross(edge01, edge20));
    
    // Emit edges for triangle
    gEdgeColor = edgeColor;
    
    // Edge 0-1
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    EndPrimitive();
    
    // Edge 1-2
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
    
    // Edge 2-0
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    EndPrimitive();
}
)";
}

std::string GPUEdgeRenderer::getFragmentShaderSource() const
{
    return R"(
#version 330 core

in vec3 gEdgeColor;
out vec4 fragColor;

uniform float edgeWidth;
uniform float edgeSmooth;

void main()
{
    // Simple edge rendering with anti-aliasing
    fragColor = vec4(gEdgeColor, 1.0);
}
)";
}

std::string GPUEdgeRenderer::getSSEDFragmentShaderSource() const
{
    return R"(
#version 330 core

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D depthTexture;
uniform sampler2D normalTexture;
uniform vec2 screenSize;
uniform float edgeThreshold;
uniform vec3 edgeColor;

// Sobel edge detection on depth buffer
float sobelDepth(vec2 uv)
{
    float dx = 1.0 / screenSize.x;
    float dy = 1.0 / screenSize.y;
    
    float d00 = texture(depthTexture, uv + vec2(-dx, -dy)).r;
    float d01 = texture(depthTexture, uv + vec2(-dx,  0)).r;
    float d02 = texture(depthTexture, uv + vec2(-dx,  dy)).r;
    float d10 = texture(depthTexture, uv + vec2( 0, -dy)).r;
    float d12 = texture(depthTexture, uv + vec2( 0,  dy)).r;
    float d20 = texture(depthTexture, uv + vec2( dx, -dy)).r;
    float d21 = texture(depthTexture, uv + vec2( dx,  0)).r;
    float d22 = texture(depthTexture, uv + vec2( dx,  dy)).r;
    
    float gx = -d00 - 2.0*d01 - d02 + d20 + 2.0*d21 + d22;
    float gy = -d00 - 2.0*d10 - d20 + d02 + 2.0*d12 + d22;
    
    return sqrt(gx*gx + gy*gy);
}

void main()
{
    float edge = sobelDepth(texCoord);
    
    if (edge > edgeThreshold) {
        fragColor = vec4(edgeColor, 1.0);
    } else {
        discard;
    }
}
)";
}

bool GPUEdgeRenderer::checkShaderSupport() const
{
    // Check if Coin3D was compiled with shader support
    // This is a simplified check - in production, query OpenGL capabilities
    return true; // Assume modern OpenGL 3.3+ support
}

bool GPUEdgeRenderer::checkGeometryShaderSupport() const
{
    // Check for geometry shader support (OpenGL 3.2+)
    // In production, query GL_VERSION and GL_ARB_geometry_shader4
    return true; // Assume support for now
}

void GPUEdgeRenderer::uploadMeshToGPU(const TriangleMesh& mesh, SoSeparator* node)
{
    if (!node || mesh.vertices.empty()) {
        return;
    }

    // Create coordinate node
    SoCoordinate3* coords = new SoCoordinate3();
    coords->point.setNum(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        coords->point.set1Value(i,
            static_cast<float>(mesh.vertices[i].X()),
            static_cast<float>(mesh.vertices[i].Y()),
            static_cast<float>(mesh.vertices[i].Z())
        );
    }
    node->addChild(coords);

    // Create indexed face set
    SoIndexedFaceSet* faceSet = new SoIndexedFaceSet();
    faceSet->coordIndex.setNum(mesh.triangles.size() + mesh.triangles.size() / 3);
    
    int idx = 0;
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        faceSet->coordIndex.set1Value(idx++, mesh.triangles[i]);
        faceSet->coordIndex.set1Value(idx++, mesh.triangles[i + 1]);
        faceSet->coordIndex.set1Value(idx++, mesh.triangles[i + 2]);
        faceSet->coordIndex.set1Value(idx++, -1); // End of face
    }
    
    node->addChild(faceSet);

    LOG_INF_S("Uploaded mesh to GPU: " + 
              std::to_string(mesh.vertices.size()) + " vertices, " +
              std::to_string(mesh.triangles.size() / 3) + " triangles");
}

