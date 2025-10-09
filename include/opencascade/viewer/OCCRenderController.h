#pragma once

#include <memory>

// Forward declarations
class OCCGeometryManager;
struct MeshParameters;

/**
 * @brief OCCRenderController - 渲染控制器
 *
 * 管理渲染参数和渲染状态控制
 */
class OCCRenderController
{
public:
    OCCRenderController(OCCGeometryManager* geometryManager);
    virtual ~OCCRenderController() = default;

    // 基本渲染控制
    virtual void setWireframeMode(bool wireframe);
    virtual void setShowEdges(bool showEdges);
    virtual void setAntiAliasing(bool enabled);

    virtual bool isWireframeMode() const;
    virtual bool isShowEdges() const;

    // 网格参数控制
    virtual void setMeshDeflection(double deflection, bool remesh = true);
    virtual void setAngularDeflection(double deflection, bool remesh = true);
    virtual const MeshParameters& getMeshParameters() const;

    // LOD控制
    virtual void setLODEnabled(bool enabled);
    virtual void setLODRoughDeflection(double deflection);
    virtual void setLODFineDeflection(double deflection);
    virtual void setLODTransitionTime(int milliseconds);
    virtual void setLODMode(bool roughMode);
    virtual void startLODInteraction();

    virtual bool isLODEnabled() const;
    virtual double getLODRoughDeflection() const;
    virtual double getLODFineDeflection() const;
    virtual int getLODTransitionTime() const;
    virtual bool isLODRoughMode() const;

    // 细分控制
    virtual void setSubdivisionEnabled(bool enabled);
    virtual void setSubdivisionLevel(int level);
    virtual void setSubdivisionMethod(int method);
    virtual void setSubdivisionCreaseAngle(double angle);

    virtual bool isSubdivisionEnabled() const;
    virtual int getSubdivisionLevel() const;
    virtual int getSubdivisionMethod() const;
    virtual double getSubdivisionCreaseAngle() const;

    // 平滑控制
    virtual void setSmoothingEnabled(bool enabled);
    virtual void setSmoothingMethod(int method);
    virtual void setSmoothingIterations(int iterations);
    virtual void setSmoothingStrength(double strength);
    virtual void setSmoothingCreaseAngle(double angle);

    virtual bool isSmoothingEnabled() const;
    virtual int getSmoothingMethod() const;
    virtual int getSmoothingIterations() const;
    virtual double getSmoothingStrength() const;
    virtual double getSmoothingCreaseAngle() const;

    // 网格质量控制
    virtual void validateMeshParameters();
    virtual void logCurrentMeshSettings();
    virtual void compareMeshQuality(const std::string& geometryName);
    virtual std::string getMeshQualityReport() const;
    virtual void exportMeshStatistics(const std::string& filename);
    virtual bool verifyParameterApplication(const std::string& parameterName, double expectedValue);

    // 参数监控
    virtual void enableParameterMonitoring(bool enabled);
    virtual bool isParameterMonitoringEnabled() const;

protected:
    OCCGeometryManager* m_geometryManager;
    MeshParameters m_meshParams;

    // 渲染状态
    bool m_wireframeMode;
    bool m_showEdges;
    bool m_antiAliasing;

    // LOD状态
    bool m_lodEnabled;
    double m_lodRoughDeflection;
    double m_lodFineDeflection;
    int m_lodTransitionTime;
    bool m_lodRoughMode;

    // 细分状态
    bool m_subdivisionEnabled;
    int m_subdivisionLevel;
    int m_subdivisionMethod;
    double m_subdivisionCreaseAngle;

    // 平滑状态
    bool m_smoothingEnabled;
    int m_smoothingMethod;
    int m_smoothingIterations;
    double m_smoothingStrength;
    double m_smoothingCreaseAngle;

    // 监控状态
    bool m_parameterMonitoringEnabled;
};
