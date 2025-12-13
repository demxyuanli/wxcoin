#include "rendering/VBOEdgeRenderer.h"
#include "logger/Logger.h"
#include <set>
#include <utility>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

VBOEdgeRenderer::VBOEdgeRenderer()
    : m_vboId(0)
    , m_edgeCount(0)
    , m_vboValid(false)
{
}

VBOEdgeRenderer::~VBOEdgeRenderer()
{
    shutdown();
}

bool VBOEdgeRenderer::initialize()
{
    if (m_vboValid) {
        return true;
    }
    
    // Generate VBO
    glGenBuffers(1, &m_vboId);
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERR_S("Failed to generate VBO: OpenGL error " + std::to_string(err));
        m_vboValid = false;
        return false;
    }
    
    m_vboValid = true;
    LOG_INF_S("VBO Edge Renderer initialized");
    return true;
}

void VBOEdgeRenderer::shutdown()
{
    if (m_vboId != 0) {
        glDeleteBuffers(1, &m_vboId);
        m_vboId = 0;
    }
    m_vboValid = false;
    m_edgeCount = 0;
}

void VBOEdgeRenderer::extractUniqueEdges(const TriangleMesh& mesh, std::vector<float>& vertices)
{
    // Deduplicate edges using set
    std::set<std::pair<int, int>> uniqueEdges;
    
    auto makeEdge = [](int a, int b) {
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    };
    
    // Collect all unique edges from triangles
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v1 = mesh.triangles[i];
        int v2 = mesh.triangles[i + 1];
        int v3 = mesh.triangles[i + 2];
        
        if (v1 < static_cast<int>(mesh.vertices.size()) && 
            v2 < static_cast<int>(mesh.vertices.size()) && 
            v3 < static_cast<int>(mesh.vertices.size())) {
            
            uniqueEdges.insert(makeEdge(v1, v2));
            uniqueEdges.insert(makeEdge(v2, v3));
            uniqueEdges.insert(makeEdge(v3, v1));
        }
    }
    
    // Convert to vertex array (each edge = 2 vertices = 6 floats)
    vertices.reserve(uniqueEdges.size() * 6);
    
    for (const auto& edge : uniqueEdges) {
        const gp_Pnt& p1 = mesh.vertices[edge.first];
        const gp_Pnt& p2 = mesh.vertices[edge.second];
        
        vertices.push_back(static_cast<float>(p1.X()));
        vertices.push_back(static_cast<float>(p1.Y()));
        vertices.push_back(static_cast<float>(p1.Z()));
        vertices.push_back(static_cast<float>(p2.X()));
        vertices.push_back(static_cast<float>(p2.Y()));
        vertices.push_back(static_cast<float>(p2.Z()));
    }
    
    m_edgeCount = uniqueEdges.size();
}

void VBOEdgeRenderer::convertPointsToVertices(const std::vector<gp_Pnt>& edgePoints, std::vector<float>& vertices)
{
    // edgePoints should contain point pairs (each pair represents an edge)
    vertices.reserve(edgePoints.size() * 3);
    
    for (const auto& pt : edgePoints) {
        vertices.push_back(static_cast<float>(pt.X()));
        vertices.push_back(static_cast<float>(pt.Y()));
        vertices.push_back(static_cast<float>(pt.Z()));
    }
    
    m_edgeCount = edgePoints.size() / 2;
}

bool VBOEdgeRenderer::createEdgeBuffer(const TriangleMesh& mesh)
{
    if (!m_vboValid && !initialize()) {
        return false;
    }
    
    std::vector<float> vertices;
    extractUniqueEdges(mesh, vertices);
    
    if (vertices.empty()) {
        LOG_WRN_S("No edges to render");
        m_edgeCount = 0;
        return false;
    }
    
    // Upload to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
    glBufferData(GL_ARRAY_BUFFER, 
                 vertices.size() * sizeof(float), 
                 vertices.data(), 
                 GL_STATIC_DRAW);
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERR_S("Failed to upload edge buffer to GPU: OpenGL error " + std::to_string(err));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return false;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    LOG_DBG_S("Created VBO edge buffer: " + std::to_string(m_edgeCount) + " edges, " +
              std::to_string(vertices.size() * sizeof(float)) + " bytes");
    
    return true;
}

bool VBOEdgeRenderer::createEdgeBuffer(const std::vector<gp_Pnt>& edgePoints)
{
    if (!m_vboValid && !initialize()) {
        return false;
    }
    
    if (edgePoints.empty() || edgePoints.size() % 2 != 0) {
        LOG_WRN_S("Invalid edge points: must be even number of points (pairs)");
        m_edgeCount = 0;
        return false;
    }
    
    std::vector<float> vertices;
    convertPointsToVertices(edgePoints, vertices);
    
    if (vertices.empty()) {
        LOG_WRN_S("No edges to render");
        m_edgeCount = 0;
        return false;
    }
    
    // Upload to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
    glBufferData(GL_ARRAY_BUFFER, 
                 vertices.size() * sizeof(float), 
                 vertices.data(), 
                 GL_STATIC_DRAW);
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERR_S("Failed to upload edge buffer to GPU: OpenGL error " + std::to_string(err));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return false;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    LOG_DBG_S("Created VBO edge buffer: " + std::to_string(m_edgeCount) + " edges");
    
    return true;
}

void VBOEdgeRenderer::render(const Quantity_Color& color, float lineWidth)
{
    if (!m_vboValid || m_edgeCount == 0 || m_vboId == 0) {
        return;
    }
    
    // Save current OpenGL state
    GLboolean lightingEnabled = glIsEnabled(GL_LIGHTING);
    GLboolean textureEnabled = glIsEnabled(GL_TEXTURE_2D);
    GLfloat currentLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &currentLineWidth);
    
    // Set rendering state
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(lineWidth);
    glColor3f(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    
    // Enable vertex array and bind VBO
    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    
    // Render lines
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_edgeCount * 2));
    
    // Restore state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    if (lightingEnabled) glEnable(GL_LIGHTING);
    if (textureEnabled) glEnable(GL_TEXTURE_2D);
    glLineWidth(currentLineWidth);
}

void VBOEdgeRenderer::clear()
{
    if (m_vboId != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    m_edgeCount = 0;
}

