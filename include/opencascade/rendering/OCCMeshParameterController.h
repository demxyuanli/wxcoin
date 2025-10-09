#pragma once

#include <memory>
#include <vector>
#include <string>

// Forward declarations
class OCCGeometry;
struct MeshParameters;

/**
 * @brief OCCMeshParameterController - 网格参数控制器
 *
 * 管理所有几何体的网格参数设置和优化
 */
class OCCMeshParameterController
{
public:
    OCCMeshParameterController();
    virtual ~OCCMeshParameterController() = default;

    // 网格参数设置
    virtual void setMeshDeflection(double deflection, bool remesh = true);
    virtual void setAngularDeflection(double deflection, bool remesh = true);
    virtual double getMeshDeflection() const;
    virtual double getAngularDeflection() const;

    // 高级网格参数
    virtual void setSubdivisionEnabled(bool enabled);
    virtual void setSubdivisionLevel(int level);
    virtual void setSubdivisionMethod(int method);
    virtual void setSubdivisionCreaseAngle(double angle);

    virtual void setSmoothingEnabled(bool enabled);
    virtual void setSmoothingMethod(int method);
    virtual void setSmoothingIterations(int iterations);
    virtual void setSmoothingStrength(double strength);
    virtual void setSmoothingCreaseAngle(double angle);

    virtual void setTessellationMethod(int method);
    virtual void setTessellationQuality(int quality);
    virtual void setFeaturePreservation(double preservation);

    virtual void setParallelProcessing(bool enabled);
    virtual void setAdaptiveMeshing(bool enabled);

    // 参数验证和监控
    virtual void validateMeshParameters();
    virtual void logCurrentMeshSettings();
    virtual void compareMeshQuality(const std::string& geometryName);
    virtual std::string getMeshQualityReport() const;
    virtual void exportMeshStatistics(const std::string& filename);
    virtual bool verifyParameterApplication(const std::string& parameterName, double expectedValue);

    // 参数监控
    virtual void enableParameterMonitoring(bool enabled);
    virtual bool isParameterMonitoringEnabled() const;
    virtual void logParameterChange(const std::string& parameterName, double oldValue, double newValue);

    // 批量操作
    virtual void remeshAllGeometries();
    virtual void updateMeshParametersForAll(const MeshParameters& params);

protected:
    // 网格参数状态
    MeshParameters m_meshParams;

    // 高级参数状态
    bool m_subdivisionEnabled;
    int m_subdivisionLevel;
    int m_subdivisionMethod;
    double m_subdivisionCreaseAngle;

    bool m_smoothingEnabled;
    int m_smoothingMethod;
    int m_smoothingIterations;
    double m_smoothingStrength;
    double m_smoothingCreaseAngle;

    int m_tessellationMethod;
    int m_tessellationQuality;
    double m_featurePreservation;

    bool m_parallelProcessing;
    bool m_adaptiveMeshing;

    // 监控配置
    bool m_parameterMonitoringEnabled;
};
