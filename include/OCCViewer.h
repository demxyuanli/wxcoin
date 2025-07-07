#pragma once

#include <memory>
#include <vector>
#include <wx/wx.h>
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
class OCCViewer {
public:
    OCCViewer(SceneManager* sceneManager);
    ~OCCViewer();

    // Geometry object management
    void addGeometry(std::shared_ptr<OCCGeometry> geometry);
    void removeGeometry(std::shared_ptr<OCCGeometry> geometry);
    void removeGeometry(const std::string& name);
    void clearAll();
    
    std::shared_ptr<OCCGeometry> findGeometry(const std::string& name);
    std::vector<std::shared_ptr<OCCGeometry>> getAllGeometry() const;
    
    // Display control
    void setGeometryVisible(const std::string& name, bool visible);
    void setGeometrySelected(const std::string& name, bool selected);
    void setGeometryColor(const std::string& name, const Quantity_Color& color);
    void setGeometryTransparency(const std::string& name, double transparency);
    
    // Batch operations
    void hideAll();
    void showAll();
    void selectAll();
    void deselectAll();
    void setAllColor(const Quantity_Color& color);
    
    // View operations
    void fitAll();
    void fitGeometry(const std::string& name);
    void resetView();
    
    // Picking and selection
    std::shared_ptr<OCCGeometry> pickGeometry(int x, int y);
    std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometry() const;
    
    // Measurement tools
    double measureDistance(const gp_Pnt& point1, const gp_Pnt& point2);
    double measureAngle(const gp_Pnt& vertex, const gp_Pnt& point1, const gp_Pnt& point2);
    
    // Rendering settings
    void setWireframeMode(bool wireframe);
    void setShadingMode(bool shaded);
    void setShowEdges(bool showEdges);
    void setAntiAliasing(bool enabled);
    
    // Normal display settings
    void setShowNormals(bool showNormals);
    void setNormalLength(double length);
    void setNormalColor(const Quantity_Color& correctColor, const Quantity_Color& incorrectColor);
    void fixNormals();  // Fix incorrect face normals
    
    bool isWireframeMode() const { return m_wireframeMode; }
    bool isShadingMode() const { return m_shadingMode; }
    bool isShowEdges() const { return m_showEdges; }
    bool isAntiAliasingEnabled() const { return m_antiAliasing; }
    bool isShowNormals() const { return m_showNormals; }
    double getNormalLength() const { return m_normalLength; }
    bool isShowingEdges() const;
    
    // Update and refresh
    void updateAll();
    void refresh();
    
    // Event handling
    void onGeometryChanged(std::shared_ptr<OCCGeometry> geometry);
    void onSelectionChanged();

private:
    void initializeViewer();
    void updateSceneGraph();
    void updateGeometryNode(std::shared_ptr<OCCGeometry> geometry);
    void updateNormalDisplay();
    void createNormalNodes();
    
    SceneManager* m_sceneManager;
    SoSeparator* m_occRoot;  // Root node for OCC geometry
    SoSeparator* m_normalRoot; // Root node for normal display
    
    std::vector<std::shared_ptr<OCCGeometry>> m_geometries;
    std::vector<std::shared_ptr<OCCGeometry>> m_selectedGeometries;
    
    // Rendering settings
    bool m_wireframeMode;
    bool m_shadingMode;
    bool m_showEdges;
    bool m_antiAliasing;
    
    // Normal display settings
    bool m_showNormals;
    double m_normalLength;
    Quantity_Color m_correctNormalColor;   // Red for correct normals
    Quantity_Color m_incorrectNormalColor; // Green for incorrect normals
    
    // Default settings
    Quantity_Color m_defaultColor;
    double m_defaultTransparency;
    double m_meshDeflection;
}; 