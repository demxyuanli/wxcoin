#pragma once

#include <memory>
#include <unordered_map>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Dir.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include "viewer/ExplodeTypes.h"

class OCCGeometry;
class SoSeparator;

class ExplodeController {
public:
	ExplodeController(SoSeparator* sceneRoot);

	void setEnabled(bool enabled, double factor);
	bool isEnabled() const { return m_enabled; }

	void setParams(ExplodeMode mode, double factor);
	void getParams(ExplodeMode& mode, double& factor) const { mode = m_mode; factor = m_factor; }

	// Advanced params support
	void setParamsAdvanced(const ExplodeParams& params) { m_params = params; }
	ExplodeParams getParamsAdvanced() const { return m_params; }

	void apply(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);
	void clear(std::vector<std::shared_ptr<OCCGeometry>>& geometries);

private:
	void computeAndApplyOffsets(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);
	
	// Direction clustering algorithm (K-Means)
	gp_Dir clusterDirections(const std::vector<gp_Dir>& directions, int maxIterations = 20);
	
	// Collision detection and resolution
	void resolveCollisions(std::vector<gp_Vec>& offsets, 
	                       const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
	                       const gp_Dir& mainDirection);
	
	// Smart mode: analyze constraints to determine main direction
	gp_Dir analyzeConstraintsDirection(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);
	
	// Helper to get bounding box diagonal
	double getBBoxDiagonal(const std::shared_ptr<OCCGeometry>& geom);

private:
	SoSeparator* m_root{ nullptr };
	bool m_enabled{ false };
	double m_factor{ 1.0 };
	ExplodeMode m_mode{ ExplodeMode::Radial };
	ExplodeParams m_params{};
	std::unordered_map<std::string, gp_Pnt> m_originalPositions;
};
