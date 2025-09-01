#pragma once

#include <wx/timer.h>

class OCCViewer;

// Manages LOD interactions (rough/fine) and timer-based transitions
class LODController {
public:
	explicit LODController(OCCViewer* viewer);

	void setEnabled(bool enabled);
	bool isEnabled() const { return m_enabled; }

	void setRoughDeflection(double deflection) { m_roughDeflection = deflection; }
	double getRoughDeflection() const { return m_roughDeflection; }

	void setFineDeflection(double deflection) { m_fineDeflection = deflection; }
	double getFineDeflection() const { return m_fineDeflection; }

	void setTransitionTimeMs(int ms) { m_transitionMs = ms; }
	int getTransitionTimeMs() const { return m_transitionMs; }

	void setMode(bool roughMode);
	bool isRoughMode() const { return m_roughMode; }

	void startInteraction();

private:
	void onTimer(wxTimerEvent&);
	double getAdaptiveDeflection(bool roughMode) const;

private:
	OCCViewer* m_viewer{ nullptr };
	bool m_enabled{ false };
	bool m_roughMode{ false };
	double m_roughDeflection{ 0.1 };
	double m_fineDeflection{ 0.01 };
	int m_transitionMs{ 500 };
	wxTimer m_timer;
};
