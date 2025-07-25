#pragma once

#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <opencascade/TopoDS_Shape.hxx>
#include <opencascade/TopoDS_Face.hxx>
#include <opencascade/TopoDS_Edge.hxx>
#include <opencascade/gp_Pnt.hxx>
#include <opencascade/gp_Vec.hxx>
#include <vector>

class DynamicSilhouetteRenderer {
public:
    DynamicSilhouetteRenderer(SoSeparator* sceneRoot = nullptr);
    ~DynamicSilhouetteRenderer();

    // Set the shape to render silhouettes for
    void setShape(const TopoDS_Shape& shape);
    
    // Get the Coin3D node that will render dynamic silhouettes
    SoSeparator* getSilhouetteNode();
    
    // Update silhouettes based on current camera position
    void updateSilhouettes(const gp_Pnt& cameraPos, const SbMatrix* modelMatrix = nullptr);
    
    // Enable/disable silhouette rendering
    void setEnabled(bool enabled);
    bool isEnabled() const;

private:
    // Dynamic silhouette calculation
    void calculateSilhouettes(const gp_Pnt& cameraPos, const SbMatrix* modelMatrix = nullptr);
    
    // Helper function to get face normal at a point
    static gp_Vec getNormalAt(const TopoDS_Face& face, const gp_Pnt& p);
    
    // Coin3D rendering callback
    static void renderCallback(void* userData, SoAction* action);

private:
    TopoDS_Shape m_shape;
    SoSeparator* m_silhouetteNode;
    SoSeparator* m_sceneRoot;  // 主场景根节点
    SoMaterial* m_material;
    SoDrawStyle* m_drawStyle;
    SoCoordinate3* m_coordinates;
    SoIndexedLineSet* m_lineSet;
    SoCallback* m_renderCallback;
    
    std::vector<gp_Pnt> m_silhouettePoints;
    std::vector<int32_t> m_silhouetteIndices;
    
    bool m_enabled;
    bool m_needsUpdate;
}; 