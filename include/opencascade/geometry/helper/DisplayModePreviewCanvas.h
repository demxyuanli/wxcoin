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
class PointViewBuilder;
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
    
private:
    void initializeScene();
    void setupCamera();
    void setupLighting();
    void setupMaterial();
    void createGeometry();
    void updateGeometryFromConfig(const DisplayModeConfig& config);
    void performViewAll();
    
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    
    wxGLContext* m_glContext{ nullptr };
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_geometryRoot{ nullptr };
    SoSeparator* m_surfaceNode{ nullptr };
    SoSeparator* m_edgesNode{ nullptr };
    SoSeparator* m_pointsNode{ nullptr };
    SoCamera* m_camera{ nullptr };
    
    SoMaterial* m_material{ nullptr };
    SoDrawStyle* m_drawStyle{ nullptr };
    SoLightModel* m_lightModel{ nullptr };
    SoShapeHints* m_shapeHints{ nullptr };
    SoPolygonOffset* m_polygonOffset{ nullptr };
    SoSwitch* m_surfaceSwitch{ nullptr };
    SoSwitch* m_edgesSwitch{ nullptr };
    SoSwitch* m_pointsSwitch{ nullptr };
    
    TopoDS_Shape m_shape;
    TriangleMesh* m_mesh{ nullptr };
    MeshParameters m_meshParams;
    std::unique_ptr<ModularEdgeComponent> m_edgeComponent;
    std::unique_ptr<PointViewBuilder> m_pointViewBuilder;
    
    RenderingConfig::DisplayMode m_currentMode{ RenderingConfig::DisplayMode::Solid };
    DisplayModeConfig m_currentConfig;
    
    bool m_initialized{ false };
    bool m_needsRedraw{ true };
    
    bool m_mouseDown{ false };
    wxPoint m_lastMousePos;
    
    DECLARE_EVENT_TABLE()
};






