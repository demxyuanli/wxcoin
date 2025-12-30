#pragma once

#include <wx/glcanvas.h>
#include <wx/colour.h>
#include "geometry/helper/DisplayModeHandler.h"
#include "config/RenderingConfig.h"
#include <memory>
#include <OpenCASCADE/TopoDS_Shape.hxx>

class SoSeparator;
class SoCamera;
class SoMaterial;
class SoDrawStyle;
class SoLightModel;
class SoSwitch;
class SoShapeHints;
class SoPolygonOffset;
class ModularEdgeComponent;
namespace helper {
    class PointViewBuilder;
}
struct TriangleMesh;
struct MeshParameters;

class DisplayModePreviewCanvas : public wxGLCanvas {
public:
    DisplayModePreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY,
                            const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize);
    ~DisplayModePreviewCanvas();
    
    void updateDisplayMode(RenderingConfig::DisplayMode mode, const DisplayModeConfig& config);
    void refreshPreview();
    void performViewAll();  // Made public for dialog initialization

private:
    void initializeScene();
    void setupCamera();
    void setupLighting();
    void setupMaterial();
    void createGeometry();
    void updateGeometryFromConfig(const DisplayModeConfig& config);
    
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    
    wxGLContext* m_glContext{ nullptr };
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_geometryRoot{ nullptr };
    SoSeparator* m_surfaceNode{ nullptr };
    SoCamera* m_camera{ nullptr };

    SoMaterial* m_material{ nullptr };
    SoDrawStyle* m_drawStyle{ nullptr };
    SoLightModel* m_lightModel{ nullptr };
    SoShapeHints* m_shapeHints{ nullptr };
    SoPolygonOffset* m_polygonOffset{ nullptr };
    SoSwitch* m_surfaceSwitch{ nullptr };

    TopoDS_Shape m_shape;
    TriangleMesh* m_mesh{ nullptr };
    MeshParameters m_meshParams;

    // Edge and point components for advanced rendering
    std::unique_ptr<ModularEdgeComponent> m_edgeComponent;
    std::unique_ptr<helper::PointViewBuilder> m_pointViewBuilder;

    // Multi-pass rendering configuration
    bool m_showSurface{ true };
    bool m_showEdges{ false };
    bool m_showPoints{ false };

    RenderingConfig::DisplayMode m_currentMode{ RenderingConfig::DisplayMode::Solid };
    DisplayModeConfig m_currentConfig;

    // Material state caching for performance optimization
    struct MaterialCache {
        bool cached{ false };
        bool enabled{ false };
        float ambient[3]{ 0.3f, 0.3f, 0.4f };
        float diffuse[3]{ 0.5f, 0.5f, 0.6f };
        float specular[3]{ 1.0f, 1.0f, 1.0f };
        float emissive[3]{ 0.0f, 0.0f, 0.0f };
        float shininess{ 50.0f };
        float transparency{ 0.0f };
    } m_materialCache;
    
    bool m_initialized{ false };
    bool m_needsRedraw{ true };
    
    bool m_mouseDown{ false };
    wxPoint m_lastMousePos;
    
    // Track camera position for silhouette edge updates
    SbVec3f m_lastSilhouetteCameraPos;
    bool m_silhouetteNeedsUpdate{ true };
    
    DECLARE_EVENT_TABLE()
};






