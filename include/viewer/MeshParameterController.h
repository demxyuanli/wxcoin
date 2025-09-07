#pragma once

#include <memory>
#include <vector>

class OCCGeometry;
struct MeshParameters; // defined in rendering/GeometryProcessor.h

class OCCViewer;
class MeshingService;

class MeshParameterController {
public:
	MeshParameterController(OCCViewer* viewer,
		MeshingService* mesher,
		MeshParameters* params,
		std::vector<std::shared_ptr<OCCGeometry>>* geometries);

	void setMeshDeflection(double deflection, bool remesh);

	// Smoothing
	void setSmoothingEnabled(bool enabled);
	void setSmoothingMethod(int method);
	void setSmoothingIterations(int iterations);
	void setSmoothingStrength(double strength);
	void setSmoothingCreaseAngle(double angle);

	// Subdivision
	void setSubdivisionEnabled(bool enabled);
	void setSubdivisionLevel(int level);
	void setSubdivisionMethod(int method);
	void setSubdivisionCreaseAngle(double angle);

	// Tessellation
	void setTessellationMethod(int method);
	void setTessellationQuality(int quality);
	void setFeaturePreservation(double preservation);
	void setAdaptiveMeshing(bool enabled);
	void setParallelProcessing(bool enabled);

	void remeshAll();

	// Accessors used by OCCViewer for queries
	bool isSmoothingEnabled() const; int getSmoothingMethod() const; int getSmoothingIterations() const;
	double getSmoothingStrength() const; double getSmoothingCreaseAngle() const;
	bool isSubdivisionEnabled() const; int getSubdivisionLevel() const; int getSubdivisionMethod() const; double getSubdivisionCreaseAngle() const;
	int getTessellationMethod() const; int getTessellationQuality() const; double getFeaturePreservation() const; bool isAdaptiveMeshing() const; bool isParallelProcessing() const;

private:
	void applyRemesh();

	OCCViewer* m_viewer;
	MeshingService* m_mesher;
	MeshParameters* m_params;
	std::vector<std::shared_ptr<OCCGeometry>>* m_geometries;

	// state mirrors of OCCViewer fields
	bool m_smoothingEnabled{ false };
	int m_smoothingMethod{ 0 };
	int m_smoothingIterations{ 2 };
	double m_smoothingStrength{ 0.5 };
	double m_smoothingCreaseAngle{ 30.0 };

	bool m_subdivisionEnabled{ false };
	int m_subdivisionLevel{ 2 };
	int m_subdivisionMethod{ 0 };
	double m_subdivisionCreaseAngle{ 30.0 };

	int m_tessellationMethod{ 0 };
	int m_tessellationQuality{ 2 };
	double m_featurePreservation{ 0.5 };
	bool m_adaptiveMeshing{ false };
	bool m_parallelProcessing{ true };
};
