#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/LODController.h"
#include "OCCViewer.h"
#include "logger/Logger.h"

LODController::LODController(OCCViewer* viewer)
	: m_viewer(viewer), m_timer(viewer, wxID_ANY) {
	m_timer.Bind(wxEVT_TIMER, &LODController::onTimer, this);
}

void LODController::setEnabled(bool enabled) {
	if (m_enabled == enabled) return;
	m_enabled = enabled;
	if (!enabled) {
		m_timer.Stop();
		setMode(false); // switch back to fine
	}
}

void LODController::setMode(bool roughMode) {
	if (m_roughMode == roughMode) return;
	m_roughMode = roughMode;
	if (!m_viewer) return;

	// Get adaptive deflection based on geometry count
	double target = getAdaptiveDeflection(roughMode);

	// Delay actual remeshing until transition back to fine mode
	// For rough mode, just change the parameter without remeshing
	// This significantly improves performance during interaction
	if (roughMode) {
		// Just update parameter without remeshing for rough mode
		m_viewer->setMeshDeflection(target, false);
	}
	else {
		// Only remesh when transitioning back to fine mode
		m_viewer->setMeshDeflection(target, true);
	}
}

void LODController::startInteraction() {
	if (!m_enabled) return;

	// Throttle LOD interactions to prevent excessive remeshing
	// Only allow LOD transition if enough time has passed since last interaction
	static wxLongLong lastInteractionTime = 0;
	wxLongLong currentTime = wxGetLocalTimeMillis();
	const int MIN_INTERACTION_INTERVAL = 100; // Minimum 100ms between LOD transitions

	if (currentTime - lastInteractionTime < MIN_INTERACTION_INTERVAL) {
		return; // Too soon, skip this interaction
	}

	if (!m_roughMode) {
		setMode(true);
		m_timer.Start(m_transitionMs, wxTIMER_ONE_SHOT);
		lastInteractionTime = currentTime;
	}
}

void LODController::onTimer(wxTimerEvent&) {
	setMode(false);
	m_timer.Stop();
}

double LODController::getAdaptiveDeflection(bool roughMode) const {
	if (!m_viewer) return roughMode ? m_roughDeflection : m_fineDeflection;

	try {
		// Get geometry count for adaptive adjustment
		auto geometries = m_viewer->getAllGeometry();
		size_t geometryCount = geometries.size();

		// Adaptive adjustment based on geometry count
		// More geometries = coarser deflection for better performance
		double baseDeflection = roughMode ? m_roughDeflection : m_fineDeflection;

		if (geometryCount > 50) {
			// For large numbers of geometries, make deflection coarser
			double scaleFactor = std::max(1.0, geometryCount / 50.0);
			return baseDeflection * scaleFactor;
		}
		else if (geometryCount < 10) {
			// For small numbers, can afford finer detail
			return baseDeflection * 0.8;
		}

		return baseDeflection;
	}
	catch (const std::exception& e) {
		// If there's any exception during geometry count retrieval,
		// fall back to default deflection values
		LOG_WRN_S("LODController::getAdaptiveDeflection: Exception during geometry count: " + std::string(e.what()));
		return roughMode ? m_roughDeflection : m_fineDeflection;
	}
	catch (...) {
		// Handle any other exceptions (including OpenCASCADE exceptions)
		LOG_WRN_S("LODController::getAdaptiveDeflection: Unknown exception during geometry count");
		return roughMode ? m_roughDeflection : m_fineDeflection;
	}
}