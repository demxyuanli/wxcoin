#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include "utils/PerformanceBus.h"

class PerformancePanel : public wxPanel {
public:
	PerformancePanel(wxWindow* parent);
	~PerformancePanel() override;

private:
	void OnTimer(wxTimerEvent&);
	void OnPaint(wxPaintEvent&);
	void fetchLatest();
	void drawBars(wxDC& dc, int x, int y, int w, int h, int vMs, int vMs2, int maxMs, const wxString& label1, const wxString& label2, const wxColour* colorScheme = nullptr);
	void drawCardBackground(wxDC& dc, int x, int y, int w, int h);
	void drawGrid(wxDC& dc, int x, int y, int w, int h, int maxMs);
	void drawSparkline(wxDC& dc, int x, int y, int w, int h, const std::vector<int>& hist, int maxMs, const wxColour& color);
	static void clampPush(std::vector<int>& hist, int v, std::size_t max);
	static int dynamicMaxFromHist(const std::vector<int>& hist, double current, int minCap, int maxCap);
	static wxColour lerpColor(const wxColour& a, const wxColour& b, double t);
	static void chooseGradient(double ratio, wxColour& cStart, wxColour& cEnd);
	static wxColour chooseSolidColor(double ratio);
	static bool hasEnoughContrast(const wxColour& textColor, const wxColour& bgColor);

private:
	wxTimer m_timer;
	std::optional<perf::ScenePerfSample> m_scene;
	std::optional<perf::EnginePerfSample> m_engine;
	std::optional<perf::CanvasPerfSample> m_canvas;

	// History for sparklines (last N samples)
	std::vector<int> m_histSceneTotalMs;
	std::vector<int> m_histEngineTotalMs;
	std::vector<int> m_histCanvasTotalMs;
	static constexpr std::size_t kHistory = 60;

	// Smoothed display values to reduce jitter
	double m_dispSceneTotalMs{ 0.0 };
	double m_dispSceneCoinMs{ 0.0 };
	double m_dispEngineTotalMs{ 0.0 };
	double m_dispEngineSceneMs{ 0.0 };
	double m_dispCanvasTotalMs{ 0.0 };
	double m_dispCanvasMainMs{ 0.0 };
};
