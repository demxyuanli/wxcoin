#pragma once

#include <string>

class IMeshControlApi {
public:
	virtual ~IMeshControlApi() = default;

	virtual void setMeshDeflection(double deflection, bool remesh = true) = 0;
	virtual double getMeshDeflection() const = 0;
	virtual void setAngularDeflection(double deflection, bool remesh = true) = 0;
	virtual double getAngularDeflection() const = 0;

	virtual void setSubdivisionEnabled(bool enabled) = 0;
	virtual bool isSubdivisionEnabled() const = 0;
	virtual void setSubdivisionLevel(int level) = 0;
	virtual int getSubdivisionLevel() const = 0;
	virtual void setSubdivisionMethod(int method) = 0;
	virtual int getSubdivisionMethod() const = 0;
	virtual void setSubdivisionCreaseAngle(double angle) = 0;
	virtual double getSubdivisionCreaseAngle() const = 0;

	virtual void setSmoothingEnabled(bool enabled) = 0;
	virtual bool isSmoothingEnabled() const = 0;
	virtual void setSmoothingMethod(int method) = 0;
	virtual int getSmoothingMethod() const = 0;
	virtual void setSmoothingIterations(int iterations) = 0;
	virtual int getSmoothingIterations() const = 0;
	virtual void setSmoothingStrength(double strength) = 0;
	virtual double getSmoothingStrength() const = 0;
	virtual void setSmoothingCreaseAngle(double angle) = 0;
	virtual double getSmoothingCreaseAngle() const = 0;

	virtual void setTessellationMethod(int method) = 0;
	virtual int getTessellationMethod() const = 0;
	virtual void setTessellationQuality(int quality) = 0;
	virtual int getTessellationQuality() const = 0;
	virtual void setFeaturePreservation(double preservation) = 0;
	virtual double getFeaturePreservation() const = 0;
	virtual void setParallelProcessing(bool enabled) = 0;
	virtual bool isParallelProcessing() const = 0;
	virtual void setAdaptiveMeshing(bool enabled) = 0;
	virtual bool isAdaptiveMeshing() const = 0;

	// Diagnostics
	virtual void validateMeshParameters() = 0;
	virtual void logCurrentMeshSettings() = 0;
	virtual void compareMeshQuality(const std::string& geometryName) = 0;
	virtual std::string getMeshQualityReport() const = 0;
	virtual void exportMeshStatistics(const std::string& filename) = 0;
	virtual bool verifyParameterApplication(const std::string& parameterName, double expectedValue) = 0;
	virtual void enableParameterMonitoring(bool enabled) = 0;
	virtual bool isParameterMonitoringEnabled() const = 0;
	virtual void logParameterChange(const std::string& parameterName, double oldValue, double newValue) = 0;
	virtual void remeshAllGeometries() = 0;
};
