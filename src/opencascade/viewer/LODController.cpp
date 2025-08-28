#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/LODController.h"
#include "OCCViewer.h"

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
	double target = roughMode ? m_roughDeflection : m_fineDeflection;
	m_viewer->setMeshDeflection(target, true);
}

void LODController::startInteraction() {
	if (m_enabled && !m_roughMode) {
		setMode(true);
		m_timer.Start(m_transitionMs, wxTIMER_ONE_SHOT);
	}
}

void LODController::onTimer(wxTimerEvent&) {
	setMode(false);
	m_timer.Stop();
}