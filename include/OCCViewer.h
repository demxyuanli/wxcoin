#pragma once

#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include <vector>
#include <memory>
#include <wx/wx.h>
#include <wx/timer.h>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <Quantity_Color.hxx>

// Forward declarations
class OCCGeometry;
class SoSeparator;
class SceneManager;

/**
 * @brief OpenCASCADE viewer integration
 * 
 * Manages OpenCASCADE geometry objects display in 3D scene
 */
class OCCViewer : public wxEvtHandler
{
public:
    explicit OCCViewer(SceneManager* sceneManager);
    ~OCCViewer();

    // Geometry management
    void addGeometry(std::shared_ptr<OCCGeometry> geometry);
    void removeGeometry(std::shared_ptr<OCCGeometry> geometry);
    void removeGeometry(const std::string& name);
    void clearAll();
    std::shared_ptr<OCCGeometry> findGeometry(const std::string& name);
    std::vector<std::shared_ptr<OCCGeometry>> getAllGeometry() const;

    // View manipulation
    void setGeometryVisible(const std::string& name, bool visible);
    void setGeometrySelected(const std::string& name, bool selected);
    void setGeometryColor(const std::string& name, const Quantity_Color& color);
    void setGeometryTransparency(const std::string& name, double transparency);
    void hideAll();
    void showAll();
    void selectAll();
    void deselectAll();
    void setAllColor(const Quantity_Color& color);
    void fitAll();
    void fitGeometry(const std::string& name);

    // Picking
    std::shared_ptr<OCCGeometry> pickGeometry(int x, int y);

    // Display modes
    void setWireframeMode(bool wireframe);
    void setShadingMode(bool shaded);
    void setShowEdges(bool showEdges);
    void setAntiAliasing(bool enabled);
    bool isWireframeMode() const;
    bool isShadingMode() const;
    bool isShowEdges() const;
    bool isShowNormals() const;

    // Mesh quality
    void setMeshDeflection(double deflection, bool remesh = true);
    double getMeshDeflection() const;

    // LOD (Level of Detail) control
    void setLODEnabled(bool enabled);
    bool isLODEnabled() const;
    void setLODRoughDeflection(double deflection);
    double getLODRoughDeflection() const;
    void setLODFineDeflection(double deflection);
    double getLODFineDeflection() const;
    void setLODTransitionTime(int milliseconds);
    int getLODTransitionTime() const;
    void setLODMode(bool roughMode);
    bool isLODRoughMode() const;
    void startLODInteraction();

    // Callbacks
    void onSelectionChanged();
    void onGeometryChanged(std::shared_ptr<OCCGeometry> geometry);

    // Normals display
    void setShowNormals(bool showNormals);
    void setNormalLength(double length);
    void setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect);
    void updateNormalsDisplay();

private:
    void initializeViewer();
    void remeshAllGeometries();
    void onLODTimer();

    SceneManager* m_sceneManager;
    SoSeparator* m_occRoot;
    SoSeparator* m_normalRoot;

    std::vector<std::shared_ptr<OCCGeometry>> m_geometries;
    std::vector<std::shared_ptr<OCCGeometry>> m_selectedGeometries;

    bool m_wireframeMode;
    bool m_shadingMode;
    bool m_showEdges;
    bool m_antiAliasing;

    OCCMeshConverter::MeshParameters m_meshParams;

    // LOD settings
    bool m_lodEnabled;
    bool m_lodRoughMode;
    double m_lodRoughDeflection;
    double m_lodFineDeflection;
    int m_lodTransitionTime;
    wxTimer m_lodTimer;

    // Normal display settings
    bool m_showNormals;
    double m_normalLength;
    Quantity_Color m_correctNormalColor;
    Quantity_Color m_incorrectNormalColor;

    Quantity_Color m_defaultColor;
    double m_defaultTransparency;
}; 