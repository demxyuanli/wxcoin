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
}

void ExplodeController::apply(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
	if (!m_enabled || geometries.size() <= 1) return;
	// Store originals once when applying
	m_originalPositions.clear();
	for (auto& g : geometries) {
		if (!g) continue;
		m_originalPositions[g->getName()] = g->getPosition();
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
	double baseOffset = std::max(0.1, sceneSize * 0.15) * m_factor;

	for (auto& g : geometries) {
		if (!g) continue;
		gp_Pnt gmin, gmax;
		OCCShapeBuilder::getBoundingBox(g->getShape(), gmin, gmax);
		gp_Pnt gc((gmin.X() + gmax.X()) * 0.5, (gmin.Y() + gmax.Y()) * 0.5, (gmin.Z() + gmax.Z()) * 0.5);
		gp_Pnt pos = g->getPosition();
		gp_Vec dir;
		switch (m_mode) {
		case ExplodeMode::Radial: {
			dir = gp_Vec(center, gc);
			if (dir.Magnitude() < 1e-9) dir = gp_Vec(1, 0, 0);
			dir.Normalize();
			break;
		}
		case ExplodeMode::AxisX: {
			double sign = (gc.X() - center.X()) >= 0 ? 1.0 : -1.0;
			dir = gp_Vec(sign, 0, 0);
			break;
		}
		case ExplodeMode::AxisY: {
			double sign = (gc.Y() - center.Y()) >= 0 ? 1.0 : -1.0;
			dir = gp_Vec(0, sign, 0);
			break;
		}
		case ExplodeMode::AxisZ: {
			double sign = (gc.Z() - center.Z()) >= 0 ? 1.0 : -1.0;
			dir = gp_Vec(0, 0, sign);
			break;
		}
		}
		gp_Pnt newPos(pos.X() + dir.X() * baseOffset, pos.Y() + dir.Y() * baseOffset, pos.Z() + dir.Z() * baseOffset);
		g->setPosition(newPos);
	}
}