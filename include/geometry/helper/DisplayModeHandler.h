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
 * @brief Rendering state structure for display mode
 * 
 * This structure stores all rendering state that needs to be applied
 * after the display mode switch determines what to render.
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




