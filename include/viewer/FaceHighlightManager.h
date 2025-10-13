#pragma once

#include <memory>
#include <wx/gdicmn.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <opencascade/TopoDS_Face.hxx>

class SceneManager;
class OCCGeometry;
class PickingService;

/**
 * @brief Manager for highlighting individual faces on mouse hover
 */
class FaceHighlightManager {
public:
    FaceHighlightManager(SceneManager* sceneManager, 
                        SoSeparator* occRoot,
                        PickingService* pickingService);
    ~FaceHighlightManager();

    void updateHoverHighlightAt(const wxPoint& screenPos);
    void clearHighlight();
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
    void setHighlightColor(float r, float g, float b);
    void setHighlightLineWidth(float width);

private:
    void highlightFace(std::shared_ptr<OCCGeometry> geometry, int faceId);
    void buildFaceHighlight(const TopoDS_Face& face);
    void attachHighlightToScene();
    void detachHighlightFromScene();

    SceneManager* m_sceneManager;
    SoSeparator* m_occRoot;
    PickingService* m_pickingService;
    
    SoSeparator* m_highlightNode;
    SoMaterial* m_material;
    SoDrawStyle* m_drawStyle;
    SoCoordinate3* m_coordinates;
    SoIndexedLineSet* m_lineSet;
    
    bool m_enabled;
    int m_lastFaceId;
    std::weak_ptr<OCCGeometry> m_lastGeometry;
    bool m_highlightAttached;
};
