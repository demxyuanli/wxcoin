#pragma once

#include <string>
#include <memory>
#include <vector>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>

// Forward declarations
class SoSeparator;
class SoShaderProgram;
class SoVertexShader;
class SoGeometryShader;
class SoFragmentShader;
struct TriangleMesh;

/**
 * @brief GPU-accelerated edge rendering using shaders
 * 
 * Provides hardware-accelerated edge rendering with features:
 * - Geometry shader-based edge generation
 * - Screen-space edge detection (SSED)
 * - Hardware anti-aliasing
 * - Automatic depth offset (no z-fighting)
 * 
 * Performance benefits:
 * - No CPU processing for edge extraction
 * - Faster rendering for complex models
 * - Better visual quality with anti-aliasing
 */
class GPUEdgeRenderer {
public:
    enum class RenderMode {
        GeometryShader,      // Use geometry shader to generate edges from triangles
        ScreenSpace,         // Post-processing edge detection from depth buffer
        Hybrid               // Combine both approaches
    };

    struct EdgeRenderSettings {
        Quantity_Color color;
        float lineWidth;
        float depthOffset;       // Offset to prevent z-fighting
        bool antiAliasing;
        bool depthTest;
        float edgeThreshold;     // For screen-space detection
        RenderMode mode;

        EdgeRenderSettings()
            : color(0.0, 0.0, 0.0, Quantity_TOC_RGB)
            , lineWidth(1.0f)
            , depthOffset(0.0001f)
            , antiAliasing(true)
            , depthTest(true)
            , edgeThreshold(0.02f)
            , mode(RenderMode::GeometryShader)
        {}
    };

    GPUEdgeRenderer();
    ~GPUEdgeRenderer();

    /**
     * @brief Initialize GPU resources and shaders
     * @return True if initialization successful
     */
    bool initialize();

    /**
     * @brief Shutdown and cleanup GPU resources
     */
    void shutdown();

    /**
     * @brief Check if GPU rendering is available
     * @return True if GPU features are supported
     */
    bool isAvailable() const;

    /**
     * @brief Create edge rendering node using GPU acceleration
     * @param mesh Triangle mesh to extract edges from
     * @param settings Rendering settings
     * @return Coin3D separator node with shader-based edge rendering
     */
    SoSeparator* createGPUEdgeNode(
        const TriangleMesh& mesh,
        const EdgeRenderSettings& settings = EdgeRenderSettings());

    /**
     * @brief Create screen-space edge detection node
     * @param sceneRoot Root scene node to apply SSED
     * @param settings Rendering settings
     * @return Post-processing node with edge overlay
     */
    SoSeparator* createScreenSpaceEdgeNode(
        SoSeparator* sceneRoot,
        const EdgeRenderSettings& settings = EdgeRenderSettings());

    /**
     * @brief Update edge rendering settings
     * @param node Edge node created by createGPUEdgeNode
     * @param settings New settings to apply
     */
    void updateSettings(SoSeparator* node, const EdgeRenderSettings& settings);

    /**
     * @brief Set rendering mode
     */
    void setRenderMode(RenderMode mode);

    /**
     * @brief Get current rendering mode
     */
    RenderMode getRenderMode() const { return m_currentMode; }

    /**
     * @brief Get shader performance statistics
     */
    struct PerformanceStats {
        double lastFrameTime;      // ms
        size_t trianglesProcessed;
        size_t edgesGenerated;
        bool gpuAccelerated;
    };
    PerformanceStats getStats() const { return m_stats; }

private:
    /**
     * @brief Create geometry shader program for edge generation
     */
    SoShaderProgram* createGeometryShaderProgram(const EdgeRenderSettings& settings);

    /**
     * @brief Create screen-space edge detection shader
     */
    SoShaderProgram* createScreenSpaceShaderProgram(const EdgeRenderSettings& settings);

    /**
     * @brief Load shader source from string
     */
    std::string getVertexShaderSource() const;
    std::string getGeometryShaderSource() const;
    std::string getFragmentShaderSource() const;
    std::string getSSEDFragmentShaderSource() const;

    /**
     * @brief Check OpenGL/shader support
     */
    bool checkShaderSupport() const;
    bool checkGeometryShaderSupport() const;

    /**
     * @brief Upload mesh data to GPU
     */
    void uploadMeshToGPU(const TriangleMesh& mesh, SoSeparator* node);

    bool m_initialized;
    bool m_available;
    RenderMode m_currentMode;
    PerformanceStats m_stats;

    // Cached shader programs
    SoShaderProgram* m_geometryShaderProgram;
    SoShaderProgram* m_screenSpaceShaderProgram;
};

