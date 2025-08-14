#include "ui/PerformancePanel.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>
#include "utils/PerformanceBus.h"

PerformancePanel::PerformancePanel(wxWindow* parent)
	: wxPanel(parent), m_timer(this)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	SetMinSize(wxSize(360, 180));
	Bind(wxEVT_TIMER, &PerformancePanel::OnTimer, this);
	Bind(wxEVT_PAINT, &PerformancePanel::OnPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&){});
	m_timer.Start(500);
}

PerformancePanel::~PerformancePanel() {
	m_timer.Stop();
}

void PerformancePanel::OnTimer(wxTimerEvent&) {
    fetchLatest();
	Refresh(false);
}

void PerformancePanel::fetchLatest() {
    m_scene = perf::PerformanceBus::instance().getScene();
    m_engine = perf::PerformanceBus::instance().getEngine();
    m_canvas = perf::PerformanceBus::instance().getCanvas();
	auto ema = [](double prev, double v){ return prev <= 0.0 ? v : prev * 0.7 + v * 0.3; };
	if (m_scene) {
		m_dispSceneCoinMs = ema(m_dispSceneCoinMs, m_scene->coinSceneMs);
		m_dispSceneTotalMs = ema(m_dispSceneTotalMs, m_scene->totalSceneMs);
		clampPush(m_histSceneTotalMs, m_scene->totalSceneMs, kHistory);
	}
	if (m_engine) {
		m_dispEngineSceneMs = ema(m_dispEngineSceneMs, m_engine->sceneMs);
		m_dispEngineTotalMs = ema(m_dispEngineTotalMs, m_engine->totalMs);
		clampPush(m_histEngineTotalMs, m_engine->totalMs, kHistory);
	}
	if (m_canvas) {
		m_dispCanvasMainMs = ema(m_dispCanvasMainMs, m_canvas->mainSceneMs);
		m_dispCanvasTotalMs = ema(m_dispCanvasTotalMs, m_canvas->totalMs);
		clampPush(m_histCanvasTotalMs, m_canvas->totalMs, kHistory);
	}
}

void PerformancePanel::OnPaint(wxPaintEvent&) {
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	int w, h; GetClientSize(&w, &h);
    const int margin = 6;
    const int cardPad = 6;
    const int sparkH = 24;

    // Responsive columns: 3 / 2 / 1 by width
    int cols = (w >= 900) ? 3 : (w >= 600 ? 2 : 1);
    const int totalCards = 3;
    int rows = (totalCards + cols - 1) / cols;
    int cardW = (w - (cols + 1) * margin) / cols;
    int cardH = (h - (rows + 1) * margin) / rows;

    // Theme colours (fallback to previous tones if theme not present)
    wxColour textStrong = CFG_COLOUR("PanelHeaderTextColour");
    wxColour textNormal = CFG_COLOUR("PanelHeaderTextColour");
    wxColour textDim = CFG_COLOUR("PanelTextColour");
    wxColour sparkColour = CFG_COLOUR("AccentBlueColour");
    if (!sparkColour.IsOk()) sparkColour = wxColour(66,134,244);

    auto drawColumn = [&](int xCard, int yCard, const char* title,
                          double fps, double v1, double v2,
                          const std::vector<int>& hist, int dynMin, int dynMax,
                          const wxString& line1, const wxString& line2,
                          const wxString& barLabel1, const wxString& barLabel2) {
        drawCardBackground(dc, xCard, yCard, cardW, cardH);
        int x = xCard + cardPad;
        int y = yCard + cardPad;
        // Two compact text lines per column
        if (!line1.empty()) { dc.SetTextForeground(textStrong.IsOk()?textStrong:wxColour(230,230,240)); dc.SetFont(CFG_DEFAULTFONT()); dc.DrawText(line1, x, y); y += 16; }
        if (!line2.empty()) { dc.SetTextForeground(textDim.IsOk()?textDim:wxColour(180,180,190)); dc.SetFont(CFG_DEFAULTFONT()); dc.DrawText(line2, x, y); y += 16; }

        int dynSparkMax = std::clamp(dynamicMaxFromHist(hist, v2, dynMin, dynMax), dynMin, dynMax);
        // Sparkline with legend
        drawSparkline(dc, x, y + 4, cardW - 2 * cardPad, sparkH, hist, dynSparkMax, sparkColour);
        // legend
        int legendY = y + 4 + sparkH + 2;
        dc.SetPen(wxPen(sparkColour, 2));
        dc.DrawLine(x, legendY, x + 14, legendY);
        dc.SetTextForeground(textDim.IsOk()?textDim:wxColour(150,150,160));
        dc.SetFont(CFG_DEFAULTFONT());
        dc.DrawText("Total ms (history)", x + 18, legendY - 7);
        y = legendY + 10;

        // Reserve space for legend below bars to avoid overlap
        const int legendH = 20;
        int barAreaH = cardH - (y - yCard) - cardPad - legendH;
        int barW = cardW - 2 * cardPad;
        int maxMs = std::clamp(dynamicMaxFromHist(hist, v2, 30, 200), 30, 200);
        drawGrid(dc, x, y, barW, barAreaH, maxMs);
        drawBars(dc, x, y, barW, barAreaH, static_cast<int>(v1), static_cast<int>(v2), maxMs, "", "");

        // Bar legend (v1/v2)
        double r1 = std::clamp(v1 / std::max(1, maxMs), 0.0, 1.0);
        double r2 = std::clamp(v2 / std::max(1, maxMs), 0.0, 1.0);
        wxColour c1 = chooseSolidColor(r1);
        wxColour c2 = chooseSolidColor(r2);
        int ly = y + barAreaH + 6;
        dc.SetPen(wxPen(c1, 6)); dc.DrawLine(x, ly, x + 20, ly); dc.SetTextForeground(textNormal.IsOk()?textNormal:wxColour(200,200,210)); dc.DrawText(barLabel1, x + 26, ly - 10);
        dc.SetPen(wxPen(c2, 6)); dc.DrawLine(x + 120, ly, x + 140, ly); dc.SetTextForeground(textNormal.IsOk()?textNormal:wxColour(200,200,210)); dc.DrawText(barLabel2, x + 146, ly - 10);
    };

    // Prepare card data list
    struct CardDef { const char* title; double fps; double v1; double v2; const std::vector<int>* hist; int dynMin; int dynMax; wxString l1; wxString l2; wxString b1; wxString b2; bool valid; };
    CardDef cards[3];
    // Scene
    cards[0].title = "Scene";
    cards[0].valid = static_cast<bool>(m_scene);
    cards[0].fps = cards[0].valid ? m_scene->fps : 0.0;
    cards[0].v1 = m_dispSceneCoinMs; cards[0].v2 = m_dispSceneTotalMs; cards[0].hist = &m_histSceneTotalMs; cards[0].dynMin = 16; cards[0].dynMax = 120;
    cards[0].l1 = cards[0].valid ? wxString::Format("FPS %.1f", m_scene->fps) : "";
    cards[0].l2 = wxString::Format("Coin3D %.0f ms  Total %.0f ms", m_dispSceneCoinMs, m_dispSceneTotalMs);
    cards[0].b1 = "Coin3D"; cards[0].b2 = "Total";
    // Engine
    cards[1].title = "Engine";
    cards[1].valid = static_cast<bool>(m_engine);
    cards[1].fps = cards[1].valid ? m_engine->fps : 0.0;
    cards[1].v1 = m_dispEngineSceneMs; cards[1].v2 = m_dispEngineTotalMs; cards[1].hist = &m_histEngineTotalMs; cards[1].dynMin = 16; cards[1].dynMax = 120;
    cards[1].l1 = cards[1].valid ? wxString::Format("FPS %.1f", m_engine->fps) : "";
    cards[1].l2 = wxString::Format("Scene %.0f ms  Total %.0f ms", m_dispEngineSceneMs, m_dispEngineTotalMs);
    cards[1].b1 = "Scene"; cards[1].b2 = "Total";
    // Canvas
    cards[2].title = "Canvas";
    cards[2].valid = static_cast<bool>(m_canvas);
    cards[2].fps = cards[2].valid ? m_canvas->fps : 0.0;
    cards[2].v1 = m_dispCanvasMainMs; cards[2].v2 = m_dispCanvasTotalMs; cards[2].hist = &m_histCanvasTotalMs; cards[2].dynMin = 16; cards[2].dynMax = 120;
    cards[2].l1 = cards[2].valid ? wxString::Format("FPS %.1f", m_canvas->fps) : "";
    cards[2].l2 = wxString::Format("Main %.0f ms  Total %.0f ms", m_dispCanvasMainMs, m_dispCanvasTotalMs);
    cards[2].b1 = "Main"; cards[2].b2 = "Total";

    for (int i = 0; i < totalCards; ++i) {
        int row = i / cols;
        int col = i % cols;
        int xCard = margin + col * (cardW + margin);
        int yCard = margin + row * (cardH + margin);
        if (cards[i].valid) {
            drawColumn(xCard, yCard, cards[i].title, cards[i].fps, cards[i].v1, cards[i].v2,
                       *cards[i].hist, cards[i].dynMin, cards[i].dynMax,
                       cards[i].l1, cards[i].l2, cards[i].b1, cards[i].b2);
        } else {
            drawCardBackground(dc, xCard, yCard, cardW, cardH);
        }
    }
}

void PerformancePanel::drawBars(wxDC& dc, int x, int y, int w, int h, int vMs, int vMs2, int maxMs,
	const wxString& label1, const wxString& label2) {
	dc.SetPen(*wxTRANSPARENT_PEN);
	int bw = (w - 10) / 2;

	int thickness = 6;
	double ratio1 = std::clamp(static_cast<double>(vMs) / std::max(1, maxMs), 0.0, 1.0);
	double ratio2 = std::clamp(static_cast<double>(vMs2) / std::max(1, maxMs), 0.0, 1.0);
	int len1 = static_cast<int>(bw * ratio1);
	int len2 = static_cast<int>(bw * ratio2);
	wxColour c1 = chooseSolidColor(ratio1);
	wxColour c2 = chooseSolidColor(ratio2);

	dc.SetBrush(wxBrush(c1));
	dc.DrawRectangle(x, y + (h - thickness) / 2, len1, thickness);

	dc.SetBrush(wxBrush(c2));
	dc.DrawRectangle(x + bw + 10, y + (h - thickness) / 2, len2, thickness);
}

void PerformancePanel::drawCardBackground(wxDC& dc, int x, int y, int w, int h) {
	dc.SetPen(wxPen(wxColour(70,70,80)));
	dc.SetBrush(wxBrush(wxColour(40,40,50)));
	dc.DrawRoundedRectangle(x, y, w, h, 6);
}

void PerformancePanel::drawGrid(wxDC& dc, int x, int y, int w, int h, int maxMs) {
	dc.SetPen(wxPen(wxColour(70,70,80)));
	for (int i = 0; i <= 4; ++i) {
		int yy = y + h - (h * i / 4);
		dc.DrawLine(x, yy, x + w, yy);
	}
}

void PerformancePanel::drawSparkline(wxDC& dc, int x, int y, int w, int h, const std::vector<int>& hist, int maxMs, const wxColour& color) {
	if (hist.empty()) return;
	dc.SetPen(wxPen(color, 2));
	int n = static_cast<int>(hist.size());
	for (int i = 1; i < n; ++i) {
		int x0 = x + (w * (i - 1)) / std::max(1, n - 1);
		int x1 = x + (w * i) / std::max(1, n - 1);
		int y0 = y + h - std::min(h, hist[i - 1] * h / std::max(1, maxMs));
		int y1 = y + h - std::min(h, hist[i] * h / std::max(1, maxMs));
		dc.DrawLine(x0, y0, x1, y1);
	}
}

void PerformancePanel::clampPush(std::vector<int>& hist, int v, std::size_t max) {
	hist.push_back(v);
	if (hist.size() > max) hist.erase(hist.begin());
}

int PerformancePanel::dynamicMaxFromHist(const std::vector<int>& hist, double current, int minCap, int maxCap) {
	int peak = static_cast<int>(current);
	for (int v : hist) peak = std::max(peak, v);
	int scaled = static_cast<int>(peak * 1.25);
	return std::clamp(scaled, minCap, maxCap);
}

wxColour PerformancePanel::lerpColor(const wxColour& a, const wxColour& b, double t) {
	auto L = [](int x, int y, double t){ return static_cast<unsigned char>(x + (y - x) * t); };
	return wxColour(L(a.Red(), b.Red(), t), L(a.Green(), b.Green(), t), L(a.Blue(), b.Blue(), t));
}

void PerformancePanel::chooseGradient(double ratio, wxColour& cStart, wxColour& cEnd) {
	wxColour green(72, 201, 176), yellow(253, 203, 110), red(253, 89, 90);
	if (ratio < 0.5) {
		double t = ratio / 0.5;
		cStart = lerpColor(green, yellow, t * 0.5);
		cEnd   = lerpColor(green, yellow, t);
	} else {
		double t = (ratio - 0.5) / 0.5;
		cStart = lerpColor(yellow, red, t * 0.5);
		cEnd   = lerpColor(yellow, red, t);
	}
}

wxColour PerformancePanel::chooseSolidColor(double ratio) {
	if (ratio < 0.33) return wxColour(72, 201, 176);
	if (ratio < 0.66) return wxColour(253, 203, 110);
	return wxColour(253, 89, 90);
}


