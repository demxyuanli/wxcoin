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
#include <cmath>
#include <queue>

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

double ExplodeController::getBBoxDiagonal(const std::shared_ptr<OCCGeometry>& geom) {
	if (!geom) return 0.0;
	gp_Pnt gmin, gmax;
	OCCShapeBuilder::getBoundingBox(geom->OCCGeometryCore::getShape(), gmin, gmax);
	double dx = gmax.X() - gmin.X();
	double dy = gmax.Y() - gmin.Y();
	double dz = gmax.Z() - gmin.Z();
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

gp_Dir ExplodeController::clusterDirections(const std::vector<gp_Dir>& directions, int maxIterations) {
	if (directions.empty()) return gp_Dir(0, 0, 1);
	if (directions.size() == 1) return directions[0];
	
	// K-Means clustering with K=3
	const int K = 3;
	std::vector<gp_Vec> centers(K);
	
	// Random initialization
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, static_cast<int>(directions.size()) - 1);
	
	for (int i = 0; i < K; ++i) {
		int idx = dis(gen);
		centers[i] = gp_Vec(directions[idx].X(), directions[idx].Y(), directions[idx].Z());
		double mag = centers[i].Magnitude();
		if (mag > 1e-9) centers[i] /= mag;
	}
	
	std::vector<int> labels(directions.size());
	
	for (int iter = 0; iter < maxIterations; ++iter) {
		// Assignment step
		for (size_t i = 0; i < directions.size(); ++i) {
			gp_Vec point(directions[i].X(), directions[i].Y(), directions[i].Z());
			double minDist = 1e9;
			int bestCluster = 0;
			
			for (int k = 0; k < K; ++k) {
				gp_Vec diff = point - centers[k];
				double dist = diff.Magnitude();
				if (dist < minDist) {
					minDist = dist;
					bestCluster = k;
				}
			}
			labels[i] = bestCluster;
		}
		
		// Update step
		std::vector<gp_Vec> sums(K, gp_Vec(0, 0, 0));
		std::vector<int> counts(K, 0);
		
		for (size_t i = 0; i < directions.size(); ++i) {
			int cluster = labels[i];
			sums[cluster] += gp_Vec(directions[i].X(), directions[i].Y(), directions[i].Z());
			counts[cluster]++;
		}
		
		for (int k = 0; k < K; ++k) {
			if (counts[k] > 0) {
				centers[k] = sums[k] / static_cast<double>(counts[k]);
				double mag = centers[k].Magnitude();
				if (mag > 1e-9) centers[k] /= mag;
			}
		}
	}
	
	// Select the largest cluster
	std::vector<int> counts(K, 0);
	for (int label : labels) counts[label]++;
	int bestCluster = static_cast<int>(std::max_element(counts.begin(), counts.end()) - counts.begin());
	
	return gp_Dir(centers[bestCluster].X(), centers[bestCluster].Y(), centers[bestCluster].Z());
}

gp_Dir ExplodeController::analyzeConstraintsDirection(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
	// Collect constraint directions from params
	std::vector<gp_Dir> directions;
	
	for (const auto& constraint : m_params.constraints) {
		directions.push_back(constraint.direction);
		// Also consider opposite direction
		directions.push_back(gp_Dir(-constraint.direction.X(), -constraint.direction.Y(), -constraint.direction.Z()));
	}
	
	if (directions.empty()) {
		// Fallback: analyze geometric distribution
		gp_Pnt minPt, maxPt;
		bool init = false;
		for (auto& g : geometries) {
			if (!g) continue;
			gp_Pnt gmin, gmax;
			OCCShapeBuilder::getBoundingBox(g->OCCGeometryCore::getShape(), gmin, gmax);
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
		
		// Use longest axis as primary direction
		double dx = maxPt.X() - minPt.X();
		double dy = maxPt.Y() - minPt.Y();
		double dz = maxPt.Z() - minPt.Z();
		
		if (dz >= dx && dz >= dy) return gp_Dir(0, 0, 1);
		else if (dy >= dx) return gp_Dir(0, 1, 0);
		else return gp_Dir(1, 0, 0);
	}
	
	return clusterDirections(directions);
}

void ExplodeController::resolveCollisions(std::vector<gp_Vec>& offsets, 
                                          const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
                                          const gp_Dir& mainDirection) {
	const size_t N = geometries.size();
	if (N != offsets.size()) return;
	
	// Compute new centers after applying offsets
	std::vector<gp_Pnt> newCenters(N);
	std::vector<double> diagonals(N);
	
	for (size_t i = 0; i < N; ++i) {
		if (!geometries[i]) continue;
		gp_Pnt gmin, gmax;
		OCCShapeBuilder::getBoundingBox(geometries[i]->OCCGeometryCore::getShape(), gmin, gmax);
		gp_Pnt center((gmin.X() + gmax.X()) * 0.5, (gmin.Y() + gmax.Y()) * 0.5, (gmin.Z() + gmax.Z()) * 0.5);
		
		gp_Pnt pos = geometries[i]->getPosition();
		newCenters[i] = gp_Pnt(pos.X() + offsets[i].X(), 
		                        pos.Y() + offsets[i].Y(), 
		                        pos.Z() + offsets[i].Z());
		
		diagonals[i] = getBBoxDiagonal(geometries[i]);
	}
	
	// Simple pairwise collision detection and resolution
	const int maxPasses = 3;
	for (int pass = 0; pass < maxPasses; ++pass) {
		bool hadCollision = false;
		
		for (size_t i = 0; i < N; ++i) {
			if (!geometries[i]) continue;
			
			for (size_t j = i + 1; j < N; ++j) {
				if (!geometries[j]) continue;
				
				double dist = newCenters[i].Distance(newCenters[j]);
				double minDist = (diagonals[i] + diagonals[j]) * m_params.collisionThreshold * 0.5;
				
				if (dist < minDist && dist > 1e-9) {
					hadCollision = true;
					
					// Push apart along main direction
					gp_Vec pushDir(mainDirection.X(), mainDirection.Y(), mainDirection.Z());
					double pushAmount = (minDist - dist) * 0.5;
					
					// Determine which part to push in which direction based on position along main axis
					gp_Vec centerDiff(newCenters[i], newCenters[j]);
					double dotProd = centerDiff.Dot(pushDir);
					
					if (dotProd > 0) {
						// i is ahead, push i forward and j backward
						offsets[i] += pushDir * pushAmount;
						offsets[j] -= pushDir * pushAmount;
						newCenters[i].Translate(pushDir * pushAmount);
						newCenters[j].Translate(-pushDir * pushAmount);
					} else {
						// j is ahead, push j forward and i backward
						offsets[i] -= pushDir * pushAmount;
						offsets[j] += pushDir * pushAmount;
						newCenters[i].Translate(-pushDir * pushAmount);
						newCenters[j].Translate(pushDir * pushAmount);
					}
				}
			}
		}
		
		if (!hadCollision) break;
	}
}

void ExplodeController::computeAndApplyOffsets(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
	gp_Pnt minPt, maxPt;
	bool init = false;
	for (auto& g : geometries) {
		if (!g) continue;
		gp_Pnt gmin, gmax;
		OCCShapeBuilder::getBoundingBox(g->OCCGeometryCore::getShape(), gmin, gmax);
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
	
	// Smart mode: determine main direction from constraints
	gp_Dir smartMainDir(0, 0, 1);
	if (m_mode == ExplodeMode::Smart) {
		smartMainDir = analyzeConstraintsDirection(geometries);
	}
	
	// Store offsets for collision resolution
	std::vector<gp_Vec> offsets(geometries.size());

	for (size_t idx = 0; idx < geometries.size(); ++idx) {
		auto& g = geometries[idx];
		if (!g) continue;
		
		gp_Pnt gmin, gmax;
		OCCShapeBuilder::getBoundingBox(g->OCCGeometryCore::getShape(), gmin, gmax);
		gp_Pnt gc((gmin.X() + gmax.X()) * 0.5, (gmin.Y() + gmax.Y()) * 0.5, (gmin.Z() + gmax.Z()) * 0.5);
		gp_Pnt pos = g->getPosition();

		// Aggregate direction by weights
		gp_Vec dirAgg(0, 0, 0);
		
		// Smart mode: use clustered direction
		if (m_mode == ExplodeMode::Smart) {
			// Use smart direction, but also consider radial component
			gp_Vec vr(center, gc);
			if (vr.Magnitude() < 1e-9) vr = gp_Vec(1, 0, 0);
			vr.Normalize();
			
			gp_Vec smartVec(smartMainDir.X(), smartMainDir.Y(), smartMainDir.Z());
			// Blend smart direction with radial
			dirAgg = smartVec * 0.7 + vr * 0.3;
		}
		// Radial component
		else if (m_params.weights.radial > 0.0 || m_mode == ExplodeMode::Radial || m_mode == ExplodeMode::Assembly) {
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
			gp_Vec vd(1, 1, 1); 
			double mag = vd.Magnitude();
			if (mag > 1e-9) vd /= mag;
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
		double mag = dirAgg.Magnitude();
		if (mag > 1e-9) dirAgg /= mag;

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
			double jmag = jv.Magnitude();
			if (jmag > 1e-9) jv /= jmag;
			dirAgg += jv * (m_params.jitter * 0.1); // small perturbation
		}

		offsets[idx] = dirAgg * baseOffset;
	}
	
	// Apply collision resolution if enabled
	if (m_params.enableCollisionResolution) {
		gp_Dir mainDir = (m_mode == ExplodeMode::Smart) ? smartMainDir : gp_Dir(0, 0, 1);
		resolveCollisions(offsets, geometries, mainDir);
	}
	
	// Apply final offsets to geometries
	for (size_t idx = 0; idx < geometries.size(); ++idx) {
		auto& g = geometries[idx];
		if (!g) continue;
		
		gp_Pnt pos = g->getPosition();
		gp_Pnt newPos(pos.X() + offsets[idx].X(),
		              pos.Y() + offsets[idx].Y(),
		              pos.Z() + offsets[idx].Z());
		
		// Simple min spacing: if movement too small, boost to minSpacing fraction of baseOffset
		if (m_params.minSpacing > 0.0) {
			gp_Vec moved(pos, newPos);
			double minMove = m_params.minSpacing * baseOffset;
			double moveMag = moved.Magnitude();
			if (moveMag > 1e-9 && moveMag < minMove) {
				moved /= moveMag;
				newPos = gp_Pnt(pos.X() + moved.X() * minMove,
				                pos.Y() + moved.Y() * minMove,
				                pos.Z() + moved.Z() * minMove);
			}
		}
		
		g->setPosition(newPos);
	}
}
