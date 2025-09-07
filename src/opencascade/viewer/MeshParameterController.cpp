#include <vector>
#include "viewer/MeshParameterController.h"
#include "viewer/MeshingService.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"

MeshParameterController::MeshParameterController(OCCViewer* viewer,
	MeshingService* mesher,
	MeshParameters* params,
	std::vector<std::shared_ptr<OCCGeometry>>* geometries)
	: m_viewer(viewer), m_mesher(mesher), m_params(params), m_geometries(geometries) {
}

void MeshParameterController::setMeshDeflection(double deflection, bool remesh) {
	if (!m_params) return;
	if (m_params->deflection != deflection) {
		m_params->deflection = deflection;
		if (remesh) applyRemesh();
	}
}

void MeshParameterController::setSmoothingEnabled(bool enabled) { m_smoothingEnabled = enabled; applyRemesh(); }
void MeshParameterController::setSmoothingMethod(int method) { m_smoothingMethod = method; if (m_smoothingEnabled) applyRemesh(); }
void MeshParameterController::setSmoothingIterations(int iterations) { m_smoothingIterations = iterations; if (m_smoothingEnabled) applyRemesh(); }
void MeshParameterController::setSmoothingStrength(double strength) { m_smoothingStrength = strength; if (m_smoothingEnabled) applyRemesh(); }
void MeshParameterController::setSmoothingCreaseAngle(double angle) { m_smoothingCreaseAngle = angle; if (m_smoothingEnabled) applyRemesh(); }

void MeshParameterController::setSubdivisionEnabled(bool enabled) { m_subdivisionEnabled = enabled; applyRemesh(); }
void MeshParameterController::setSubdivisionLevel(int level) { m_subdivisionLevel = level; if (m_subdivisionEnabled) applyRemesh(); }
void MeshParameterController::setSubdivisionMethod(int method) { m_subdivisionMethod = method; if (m_subdivisionEnabled) applyRemesh(); }
void MeshParameterController::setSubdivisionCreaseAngle(double angle) { m_subdivisionCreaseAngle = angle; if (m_subdivisionEnabled) applyRemesh(); }

void MeshParameterController::setTessellationMethod(int method) { m_tessellationMethod = method; applyRemesh(); }
void MeshParameterController::setTessellationQuality(int quality) { m_tessellationQuality = quality; applyRemesh(); }
void MeshParameterController::setFeaturePreservation(double preservation) { m_featurePreservation = preservation; applyRemesh(); }
void MeshParameterController::setAdaptiveMeshing(bool enabled) { m_adaptiveMeshing = enabled; applyRemesh(); }
void MeshParameterController::setParallelProcessing(bool enabled) { m_parallelProcessing = enabled; }

void MeshParameterController::remeshAll() { applyRemesh(); }

bool MeshParameterController::isSmoothingEnabled() const { return m_smoothingEnabled; }
int MeshParameterController::getSmoothingMethod() const { return m_smoothingMethod; }
int MeshParameterController::getSmoothingIterations() const { return m_smoothingIterations; }
double MeshParameterController::getSmoothingStrength() const { return m_smoothingStrength; }
double MeshParameterController::getSmoothingCreaseAngle() const { return m_smoothingCreaseAngle; }
bool MeshParameterController::isSubdivisionEnabled() const { return m_subdivisionEnabled; }
int MeshParameterController::getSubdivisionLevel() const { return m_subdivisionLevel; }
int MeshParameterController::getSubdivisionMethod() const { return m_subdivisionMethod; }
double MeshParameterController::getSubdivisionCreaseAngle() const { return m_subdivisionCreaseAngle; }
int MeshParameterController::getTessellationMethod() const { return m_tessellationMethod; }
int MeshParameterController::getTessellationQuality() const { return m_tessellationQuality; }
double MeshParameterController::getFeaturePreservation() const { return m_featurePreservation; }
bool MeshParameterController::isAdaptiveMeshing() const { return m_adaptiveMeshing; }
bool MeshParameterController::isParallelProcessing() const { return m_parallelProcessing; }

void MeshParameterController::applyRemesh() {
	if (!m_mesher || !m_params || !m_geometries) return;
	m_mesher->applyAndRemesh(*m_params, *m_geometries,
		m_smoothingEnabled, m_smoothingIterations, m_smoothingStrength, m_smoothingCreaseAngle,
		m_subdivisionEnabled, m_subdivisionLevel, m_subdivisionMethod, m_subdivisionCreaseAngle,
		m_tessellationMethod, m_tessellationQuality, m_featurePreservation, m_adaptiveMeshing, m_parallelProcessing);
}