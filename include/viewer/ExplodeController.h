#pragma once

#include <memory>
#include <unordered_map>
#include <OpenCASCADE/gp_Pnt.hxx>
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

private:
	SoSeparator* m_root{ nullptr };
	bool m_enabled{ false };
	double m_factor{ 1.0 };
	ExplodeMode m_mode{ ExplodeMode::Radial };
	ExplodeParams m_params{};
	std::unordered_map<std::string, gp_Pnt> m_originalPositions;
};
