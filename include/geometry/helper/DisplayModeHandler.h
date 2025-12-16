#pragma once

#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include "geometry/GeometryRenderContext.h"
#include "EdgeTypes.h"
#include "rendering/GeometryProcessor.h"
#include <OpenCASCADE/Quantity_Color.hxx>
#include <memory>

class SoSeparator;
class SoNode;
class SoSwitch;
class ModularEdgeComponent;
class RenderNodeBuilder;
class WireframeBuilder;
class PointViewBuilder;
class TopoDS_Shape;
struct MeshParameters;
class BRepDisplayModeHandler;
class MeshDisplayModeHandler;

/**
 * @brief Display mode configuration structure - Data-driven approach
 * 
 * This structure defines what geometry nodes to display, how to render them,
 * and what post-processing to apply. Each display mode has a pre-configured
 * instance of this structure.
 */
struct DisplayModeConfig {
    // ========== Data Node Requirements ==========
    // Which geometry nodes should be displayed
    struct NodeRequirements {
        bool requireSurface = false;         // Surface/faces geometry node
        bool requireOriginalEdges = false;   // Original geometric edges node (BREP only)
        bool requireMeshEdges = false;       // Mesh edges node
        bool requirePoints = false;          // Vertex points node
        bool surfaceWithPoints = false;      // Show surface together with points
    } nodes;
    
    // ========== Rendering Properties ==========
    struct RenderingProperties {
        // Lighting model: BASE_COLOR (no lighting) or PHONG (full lighting)
        enum class LightModel {
            BASE_COLOR,
            PHONG
        } lightModel = LightModel::PHONG;
        
        // Material override (if useMaterialOverride is true, these values override context material)
        struct MaterialOverride {
            bool enabled = false;
            Quantity_Color ambientColor;
            Quantity_Color diffuseColor;
            Quantity_Color specularColor;
            Quantity_Color emissiveColor;
            double shininess = 0.0;
            double transparency = 0.0;
        } materialOverride;
        
        // Texture
        bool textureEnabled = false;
        
        // Blending
        RenderingConfig::BlendMode blendMode = RenderingConfig::BlendMode::None;
    } rendering;
    
    // ========== Edge Configuration ==========
    struct EdgeConfig {
        // Edge type and color (for BREP models)
        struct OriginalEdge {
            bool enabled = false;
            Quantity_Color color;
            double width = 1.0;
        } originalEdge;
        
        // Mesh edge type and color (for mesh models or HiddenLine mode)
        struct MeshEdge {
            bool enabled = false;
            Quantity_Color color;
            double width = 1.0;
            // Special color selection for HiddenLine mode
            bool useEffectiveColor = false;  // If true, use black if color is too light
        } meshEdge;
    } edges;
    
    // ========== Post-Processing ==========
    struct PostProcessing {
        // Polygon offset for depth sorting (used in HiddenLine mode)
        struct PolygonOffset {
            bool enabled = false;
            float factor = 0.0f;
            float units = 0.0f;
        } polygonOffset;
    } postProcessing;
    
    DisplayModeConfig() = default;
};

/**
 * @brief Legacy rendering state structure (deprecated, kept for compatibility)
 * 
 * This is being replaced by DisplayModeConfig for data-driven architecture.
 */
struct DisplayModeRenderState {
    // Rendering components
    bool showSurface = false;           // Show surface/faces (merged with facesVisible)
    bool showOriginalEdges = false;    // Show original geometric edges (from shape topology)
    bool showMeshEdges = false;        // Show mesh edges (from triangulation)
    
    // Surface properties
    bool wireframeMode = false;         // Surface rendering mode (false = filled, true = wireframe)
    bool textureEnabled = true;
    
    // Material properties
    Quantity_Color surfaceAmbientColor;
    Quantity_Color surfaceDiffuseColor;
    Quantity_Color surfaceSpecularColor;
    Quantity_Color surfaceEmissiveColor;
    double shininess = 0.0;
    double transparency = 0.0;
    
    // Edge properties
    Quantity_Color originalEdgeColor;   // Color for original edges
    Quantity_Color meshEdgeColor;       // Color for mesh edges
    double originalEdgeWidth = 1.0;
    double meshEdgeWidth = 1.0;
    
    // Lighting
    bool lightingEnabled = true;
    
    // Blend mode
    RenderingConfig::BlendMode blendMode = RenderingConfig::BlendMode::None;
    
    // Point view
    bool showPoints = false;
    bool showSolidWithPoints = false;
    
    // Display mode override (for internal rendering passes)
    RenderingConfig::DisplayMode surfaceDisplayMode = RenderingConfig::DisplayMode::Solid;
    
    DisplayModeRenderState() 
        : surfaceAmbientColor(0.5, 0.5, 0.5, Quantity_TOC_RGB)
        , surfaceDiffuseColor(0.95, 0.95, 0.95, Quantity_TOC_RGB)
        , surfaceSpecularColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
        , surfaceEmissiveColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
        , originalEdgeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
        , meshEdgeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
    {
    }
};

class BRepDisplayModeHandler;
class MeshDisplayModeHandler;

/**
 * @brief Display mode configuration factory
 * 
 * Provides pre-configured DisplayModeConfig instances for each display mode.
 * These configurations define what nodes to display, how to render them,
 * and what post-processing to apply.
 */
class DisplayModeConfigFactory {
public:
    /**
     * Get configuration for a specific display mode
     * @param mode Display mode
     * @param context Geometry render context (for material color extraction)
     * @return Pre-configured DisplayModeConfig for the mode
     */
    static DisplayModeConfig getConfig(RenderingConfig::DisplayMode mode, 
                                       const GeometryRenderContext& context);
    
private:
    // Helper methods to create configurations for each mode
    static DisplayModeConfig createNoShadingConfig(const GeometryRenderContext& context);
    static DisplayModeConfig createPointsConfig(const GeometryRenderContext& context);
    static DisplayModeConfig createWireframeConfig(const GeometryRenderContext& context);
    static DisplayModeConfig createSolidConfig(const GeometryRenderContext& context);
    static DisplayModeConfig createFlatLinesConfig(const GeometryRenderContext& context);
    static DisplayModeConfig createTransparentConfig(const GeometryRenderContext& context);
    static DisplayModeConfig createHiddenLineConfig(const GeometryRenderContext& context);
};

class BRepDisplayModeHandler;
class MeshDisplayModeHandler;

class DisplayModeHandler {
public:
    DisplayModeHandler();
    ~DisplayModeHandler();
    
    void setModeSwitch(SoSwitch* modeSwitch);
    
    void updateDisplayMode(SoSeparator* coinNode, RenderingConfig::DisplayMode mode,
                          ModularEdgeComponent* edgeComponent,
                          const Quantity_Color* originalDiffuseColor = nullptr);
    
    void handleDisplayMode(SoSeparator* coinNode, 
                           const GeometryRenderContext& context,
                           const TopoDS_Shape& shape,
                           const MeshParameters& params,
                           ModularEdgeComponent* edgeComponent,
                           bool useModularEdgeComponent,
                           RenderNodeBuilder* renderBuilder,
                           WireframeBuilder* wireframeBuilder,
                           PointViewBuilder* pointViewBuilder = nullptr);

    // Overload for direct mesh creation (for STL/OBJ mesh-only geometries)
    void handleDisplayMode(SoSeparator* coinNode, 
                           const GeometryRenderContext& context,
                           const TriangleMesh& mesh,
                           const MeshParameters& params,
                           ModularEdgeComponent* edgeComponent,
                           bool useModularEdgeComponent,
                           RenderNodeBuilder* renderBuilder,
                           WireframeBuilder* wireframeBuilder,
                           PointViewBuilder* pointViewBuilder = nullptr);

    // Check if geometry scene graph has been fully built
    bool isGeometryBuilt() const;
    
    // Mark geometry as built after first handleDisplayMode
    void setGeometryBuilt(bool built);

private:
    std::unique_ptr<BRepDisplayModeHandler> m_brepHandler;
    std::unique_ptr<MeshDisplayModeHandler> m_meshHandler;
    SoSwitch* m_modeSwitch;
    bool m_useSwitchMode;
    static bool m_geometryBuilt;  // Track if geometry has been built (to avoid double rebuild)
};




