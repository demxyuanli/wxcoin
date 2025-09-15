#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/ExplodeController.h"
#include "OCCGeometry.h"
#include "OCCShapeBuilder.h"
#include <Inventor/nodes/SoSeparator.h>
#include <algorithm>
#include <random>

ExplodeController::ExplodeController(SoSeparator* sceneRoot)
	: m_root(sceneRoot) {
}

void ExplodeController::setEnabled(bool enabled, double factor) {
	m_enabled = enabled;
	m_factor = factor;
}

void ExplodeController::setParams(ExplodeMode mode, double factor) {
	m_mode = mode;
	m_factor = factor;
	m_params.primaryMode = mode;
	m_params.baseFactor = factor;
}

void ExplodeController::apply(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
	if (!m_enabled || geometries.size() <= 1) return;
	// Store originals only once; on subsequent apply calls, reset to originals to avoid cumulative offsets
	if (m_originalPositions.empty()) {
		for (auto& g : geometries) {
			if (!g) continue;
			m_originalPositions[g->getName()] = g->getPosition();
		}
	}
	else {
		for (auto& g : geometries) {
			if (!g) continue;
			auto it = m_originalPositions.find(g->getName());
			if (it != m_originalPositions.end()) {
				g->setPosition(it->second);
			}
		}
	}
	computeAndApplyOffsets(geometries);
}

void ExplodeController::clear(std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
	if (m_originalPositions.empty()) return;
	for (auto& g : geometries) {
		if (!g) continue;
		auto it = m_originalPositions.find(g->getName());
		if (it != m_originalPositions.end()) g->setPosition(it->second);
	}
	m_originalPositions.clear();
}

void ExplodeController::computeAndApplyOffsets(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
	gp_Pnt minPt, maxPt;
	bool init = false;
	for (auto& g : geometries) {
		if (!g) continue;
		gp_Pnt gmin, gmax;
		OCCShapeBuilder::getBoundingBox(g->getShape(), gmin, gmax);
		if (!init) { minPt = gmin; maxPt = gmax; init = true; }
		else {
			if (gmin.X() < minPt.X()) minPt.SetX(gmin.X());
			if (gmin.Y() < minPt.Y()) minPt.SetY(gmin.Y());
			if (gmin.Z() < minPt.Z()) minPt.SetZ(gmin.Z());
			if (gmax.X() > maxPt.X()) maxPt.SetX(gmax.X());
			if (gmax.Y() > maxPt.Y()) maxPt.SetY(gmax.Y());
			if (gmax.Z() > maxPt.Z()) maxPt.SetZ(gmax.Z());
		}
	}
	if (!init) return;

	gp_Pnt center((minPt.X() + maxPt.X()) * 0.5, (minPt.Y() + maxPt.Y()) * 0.5, (minPt.Z() + maxPt.Z()) * 0.5);
	double sceneSize = std::max({ maxPt.X() - minPt.X(), maxPt.Y() - minPt.Y(), maxPt.Z() - minPt.Z() });
	double baseOffset = std::max(0.1, sceneSize * 0.2) * (m_params.baseFactor > 0 ? m_params.baseFactor : m_factor);

	std::mt19937 rng(12345);
	std::uniform_real_distribution<double> dist(-1.0, 1.0);

	for (auto& g : geometries) {
		if (!g) continue;
		gp_Pnt gmin, gmax;
		OCCShapeBuilder::getBoundingBox(g->getShape(), gmin, gmax);
		gp_Pnt gc((gmin.X() + gmax.X()) * 0.5, (gmin.Y() + gmax.Y()) * 0.5, (gmin.Z() + gmax.Z()) * 0.5);
		gp_Pnt pos = g->getPosition();

		// Aggregate direction by weights
		gp_Vec dirAgg(0, 0, 0);
		// Radial component
		if (m_params.weights.radial > 0.0 || m_mode == ExplodeMode::Radial || m_mode == ExplodeMode::Assembly) {
			gp_Vec vr(center, gc);
			if (vr.Magnitude() < 1e-9) vr = gp_Vec(1, 0, 0);
			vr.Normalize();
			dirAgg += vr * (m_params.weights.radial > 0.0 ? m_params.weights.radial : (m_mode == ExplodeMode::Radial || m_mode == ExplodeMode::Assembly ? 1.0 : 0.0));
		}
		// Axis components
		dirAgg += gp_Vec(1, 0, 0) * m_params.weights.axisX;
		dirAgg += gp_Vec(0, 1, 0) * m_params.weights.axisY;
		dirAgg += gp_Vec(0, 0, 1) * m_params.weights.axisZ;
		// Diagonal component
		if (m_params.weights.diagonal > 0.0 || m_mode == ExplodeMode::Diagonal) {
			gp_Vec vd(1, 1, 1); vd.Normalize();
			dirAgg += vd * (m_params.weights.diagonal > 0.0 ? m_params.weights.diagonal : (m_mode == ExplodeMode::Diagonal ? 1.0 : 0.0));
		}
		// Stack fallback by legacy mode if no advanced weights present
		if (dirAgg.Magnitude() < 1e-12) {
			switch (m_mode) {
			case ExplodeMode::AxisX: dirAgg = gp_Vec(1, 0, 0); break;
			case ExplodeMode::AxisY: dirAgg = gp_Vec(0, 1, 0); break;
			case ExplodeMode::AxisZ: dirAgg = gp_Vec(0, 0, 1); break;
			case ExplodeMode::StackX: dirAgg = gp_Vec((gc.X() - center.X()) >= 0 ? 1.0 : -1.0, 0, 0); break;
			case ExplodeMode::StackY: dirAgg = gp_Vec(0, (gc.Y() - center.Y()) >= 0 ? 1.0 : -1.0, 0); break;
			case ExplodeMode::StackZ: dirAgg = gp_Vec(0, 0, (gc.Z() - center.Z()) >= 0 ? 1.0 : -1.0); break;
			default: break;
			}
		}
		if (dirAgg.Magnitude() < 1e-9) dirAgg = gp_Vec(1, 0, 0);
		dirAgg.Normalize();

		// Apply hierarchy level scaling for Assembly mode
		if (m_mode == ExplodeMode::Assembly) {
			int level = g->getAssemblyLevel();
			double levelScale = 1.0 + std::max(0, level) * std::max(0.0, m_params.perLevelScale);
			dirAgg.Multiply(levelScale);
		}

		// Size influence scaling
		if (m_params.sizeInfluence > 0.0) {
			double partSize = std::max({ gmax.X() - gmin.X(), gmax.Y() - gmin.Y(), gmax.Z() - gmin.Z() });
			double ratio = partSize / std::max(1e-6, sceneSize);
			double sizeScale = 1.0 + std::max(0.0, std::min(2.0, m_params.sizeInfluence)) * ratio;
			dirAgg.Multiply(sizeScale);
		}

		// Jitter
		if (m_params.jitter > 0.0) {
			gp_Vec jv(dist(rng), dist(rng), dist(rng));
			jv.Normalize();
			dirAgg += jv * (m_params.jitter * 0.1); // small perturbation
		}

		gp_Pnt newPos(pos.X() + dirAgg.X() * baseOffset,
			pos.Y() + dirAgg.Y() * baseOffset,
			pos.Z() + dirAgg.Z() * baseOffset);
		// Simple min spacing: if movement too small, boost to minSpacing fraction of baseOffset
		if (m_params.minSpacing > 0.0) {
			gp_Vec moved(pos, newPos);
			double minMove = m_params.minSpacing * baseOffset;
			double mag = moved.Magnitude();
			if (mag > 1e-9 && mag < minMove) {
				moved.Normalize();
				newPos = gp_Pnt(pos.X() + moved.X() * minMove,
								 pos.Y() + moved.Y() * minMove,
								 pos.Z() + moved.Z() * minMove);
			}
		}
		g->setPosition(newPos);
	}
}