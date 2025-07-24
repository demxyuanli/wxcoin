#pragma once

#include "OCCGeometry.h"
#include "rendering/GeometryProcessor.h"
#include "ViewRefreshManager.h"
#include <vector>
#include <memory>
#include <wx/wx.h>
#include <wx/timer.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"

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
    std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometries() const;

    // Batch operations for performance optimization
    void beginBatchOperation();
    void endBatchOperation();
    bool isBatchOperationActive() const;
    
    // Batch geometry addition for better performance
    void addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);
    
    // Deferred ObjectTree updates
    void updateObjectTreeDeferred();

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
    void setShowEdges(bool showEdges);
    void setAntiAliasing(bool enabled);
    bool isWireframeMode() const;
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

    // Subdivision surface control
    void setSubdivisionEnabled(bool enabled);
    bool isSubdivisionEnabled() const;
    void setSubdivisionLevel(int level);
    int getSubdivisionLevel() const;
    void setSubdivisionMethod(int method);
    int getSubdivisionMethod() const;
    void setSubdivisionCreaseAngle(double angle);
    double getSubdivisionCreaseAngle() const;
    
    // Mesh smoothing control
    void setSmoothingEnabled(bool enabled);
    bool isSmoothingEnabled() const;
    void setSmoothingMethod(int method);
    int getSmoothingMethod() const;
    void setSmoothingIterations(int iterations);
    int getSmoothingIterations() const;
    void setSmoothingStrength(double strength);
    double getSmoothingStrength() const;
    void setSmoothingCreaseAngle(double angle);
    double getSmoothingCreaseAngle() const;
    
    // Advanced tessellation control
    void setTessellationMethod(int method);
    int getTessellationMethod() const;
    void setTessellationQuality(int quality);
    int getTessellationQuality() const;
    void setFeaturePreservation(double preservation);
    double getFeaturePreservation() const;
    void setParallelProcessing(bool enabled);
    bool isParallelProcessing() const;
    void setAdaptiveMeshing(bool enabled);
    bool isAdaptiveMeshing() const;

    // Callbacks
    void onSelectionChanged();
    void onGeometryChanged(std::shared_ptr<OCCGeometry> geometry);

    // Normals display
    void setShowNormals(bool showNormals);
    void setNormalLength(double length);
    void setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect);
    void updateNormalsDisplay();
    
    // View refresh
    void requestViewRefresh();
    
   
    // Advanced geometry creation - NEW
    std::shared_ptr<OCCGeometry> addGeometryWithAdvancedRendering(const TopoDS_Shape& shape, const std::string& name);
    std::shared_ptr<OCCGeometry> addBezierCurve(const std::vector<gp_Pnt>& controlPoints, const std::string& name);
    std::shared_ptr<OCCGeometry> addBezierSurface(const std::vector<std::vector<gp_Pnt>>& controlPoints, const std::string& name);
    std::shared_ptr<OCCGeometry> addBSplineCurve(const std::vector<gp_Pnt>& poles, const std::vector<double>& weights, const std::string& name);
    
    // Upgrade existing geometries - NEW
    void upgradeGeometryToAdvanced(const std::string& name);
    void upgradeAllGeometriesToAdvanced();
    
    // Mesh quality validation and debugging
    void validateMeshParameters();
    void logCurrentMeshSettings();
    void compareMeshQuality(const std::string& geometryName);
    std::string getMeshQualityReport() const;
    void exportMeshStatistics(const std::string& filename);
    bool verifyParameterApplication(const std::string& parameterName, double expectedValue);
    
    // Real-time parameter monitoring
    void enableParameterMonitoring(bool enabled);
    bool isParameterMonitoringEnabled() const;
    void logParameterChange(const std::string& parameterName, double oldValue, double newValue);
    
    // Force mesh regeneration
    void remeshAllGeometries();

    // Display flags
    void setShowEdgesFlag(bool flag);
    void setShowWireframeFlag(bool flag);
    void setShowFacesFlag(bool flag);
    void setShowNormalsFlag(bool flag);
    bool isShowEdgesFlag() const;
    bool isShowWireframeFlag() const;
    bool isShowFacesFlag() const;
    bool isShowNormalsFlag() const;
    void updateDisplay();

    EdgeDisplayFlags globalEdgeFlags;
    void setShowOriginalEdges(bool show);
    void setShowFeatureEdges(bool show);
    void setShowMeshEdges(bool show);
    void setShowHighlightEdges(bool show);
    void setShowNormalLines(bool show);
    void setShowFaceNormalLines(bool show);
    void toggleEdgeType(EdgeType type, bool show);
    bool isEdgeTypeEnabled(EdgeType type) const;
    void updateAllEdgeDisplays();

private:
    void initializeViewer();
    void onLODTimer();
    void createNormalVisualization(std::shared_ptr<OCCGeometry> geometry);

    SceneManager* m_sceneManager;
    SoSeparator* m_occRoot;
    SoSeparator* m_normalRoot;

    std::vector<std::shared_ptr<OCCGeometry>> m_geometries;
    std::vector<std::shared_ptr<OCCGeometry>> m_selectedGeometries;

    bool m_wireframeMode;
    bool m_shadingMode;
    bool m_showEdges;
    bool m_antiAliasing;

    MeshParameters m_meshParams;

    // LOD settings
    bool m_lodEnabled;
    bool m_lodRoughMode;
    double m_lodRoughDeflection;
    double m_lodFineDeflection;
    int m_lodTransitionTime;
    wxTimer m_lodTimer;
    
    // Subdivision settings
    bool m_subdivisionEnabled;
    int m_subdivisionLevel;
    int m_subdivisionMethod;
    double m_subdivisionCreaseAngle;
    
    // Smoothing settings
    bool m_smoothingEnabled;
    int m_smoothingMethod;
    int m_smoothingIterations;
    double m_smoothingStrength;
    double m_smoothingCreaseAngle;
    
    // Advanced tessellation settings
    int m_tessellationMethod;
    int m_tessellationQuality;
    double m_featurePreservation;
    bool m_parallelProcessing;
    bool m_adaptiveMeshing;

    // Normal display settings
    bool m_showNormals;
    double m_normalLength;
    Quantity_Color m_correctNormalColor;
    Quantity_Color m_incorrectNormalColor;

    Quantity_Color m_defaultColor;
    double m_defaultTransparency;
    
    // Batch operation state
    bool m_batchOperationActive;
    bool m_needsViewRefresh;

    // Performance optimization
    bool m_meshRegenerationNeeded;
    MeshParameters m_lastMeshParams;
    
    // Deferred ObjectTree updates
    std::vector<std::shared_ptr<OCCGeometry>> m_pendingObjectTreeUpdates;

    // Parameter monitoring
    bool m_parameterMonitoringEnabled;

    struct DisplayFlags {
        bool showEdges = false;
        bool showWireframe = false;
        bool showFaces = true;
        bool showNormals = false;
    };
    DisplayFlags m_displayFlags;
    void drawFaces();
    void drawWireframe();
    // drawEdges() declaration removed - edge display is now handled by EdgeComponent system
    void drawNormals();
};