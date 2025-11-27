#include "ui/AsyncEnginePanel.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <algorithm>

wxBEGIN_EVENT_TABLE(AsyncEnginePanel, wxPanel)
    EVT_TIMER(wxID_ANY, AsyncEnginePanel::OnTimer)
    EVT_PAINT(AsyncEnginePanel::OnPaint)
    EVT_SIZE(AsyncEnginePanel::OnSize)
wxEND_EVENT_TABLE()

AsyncEnginePanel::AsyncEnginePanel(wxWindow* parent, async::AsyncEngineIntegration* asyncEngine)
    : wxPanel(parent)
    , m_timer(this)
    , m_asyncEngine(asyncEngine)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    SetMinSize(wxSize(280, 180));

    // Set minimum client size for responsive behavior
    SetSizeHints(wxSize(280, 180), wxDefaultSize);
    
    SetBackgroundColour(wxColour(245, 245, 245));
    
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
    
    m_timer.Start(500);
}

AsyncEnginePanel::~AsyncEnginePanel() {
    m_timer.Stop();
}

void AsyncEnginePanel::setAsyncEngine(async::AsyncEngineIntegration* engine) {
    m_asyncEngine = engine;
}

void AsyncEnginePanel::OnTimer(wxTimerEvent&) {
    fetchLatest();
    Refresh(false);
}

void AsyncEnginePanel::OnSize(wxSizeEvent& event) {
    Refresh(false);
    event.Skip();
}

void AsyncEnginePanel::fetchLatest() {
    if (!m_asyncEngine) {
        return;
    }
    
    auto stats = m_asyncEngine->getStatistics();
    
    m_queuedTasks = stats.queuedTasks;
    m_runningTasks = stats.runningTasks;
    m_completedTasks = stats.completedTasks;
    m_failedTasks = stats.failedTasks;
    m_totalProcessed = stats.totalProcessedTasks;
    m_avgExecutionTimeMs = stats.avgExecutionTimeMs;
    m_isRunning = m_asyncEngine->getEngine()->isRunning();
    
    // Update history
    clampPush(m_histQueuedTasks, m_queuedTasks, kHistorySize);
    clampPush(m_histRunningTasks, m_runningTasks, kHistorySize);
    clampPush(m_histCompletedTasks, m_completedTasks, kHistorySize);
    clampPush(m_histAvgTimeMs, static_cast<size_t>(m_avgExecutionTimeMs), kHistorySize);
    
    // Smooth values for display
    auto ema = [](double prev, double v) { return prev <= 0.0 ? v : prev * 0.7 + v * 0.3; };
    m_dispQueuedTasks = ema(m_dispQueuedTasks, static_cast<double>(m_queuedTasks));
    m_dispRunningTasks = ema(m_dispRunningTasks, static_cast<double>(m_runningTasks));
    m_dispAvgTimeMs = ema(m_dispAvgTimeMs, m_avgExecutionTimeMs);
}

void AsyncEnginePanel::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    
    if (!m_asyncEngine) {
        wxFont font = dc.GetFont();
        font.SetPointSize(10);
        dc.SetFont(font);
        dc.SetTextForeground(wxColour(150, 150, 150));
        dc.DrawText("Async Engine not initialized", 10, 10);
        return;
    }
    
    int w, h;
    GetClientSize(&w, &h);
    
    const int margin = 8;
    const int padding = 8;
    const int headerHeight = 30;
    
    wxColour bgColor = GetBackgroundColour();
    bool isDarkBackground = (bgColor.Red() + bgColor.Green() + bgColor.Blue()) / 3 < 128;
    
    wxColour textStrong = isDarkBackground ? wxColour(255, 255, 255) : wxColour(40, 40, 40);
    wxColour textNormal = isDarkBackground ? wxColour(250, 250, 250) : wxColour(60, 60, 60);
    wxColour textDim = isDarkBackground ? wxColour(220, 220, 220) : wxColour(120, 120, 120);
    
    // Draw panel header
    wxFont titleFont = dc.GetFont();
    titleFont.SetPointSize(11);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    dc.SetFont(titleFont);
    dc.SetTextForeground(textStrong);
    
    int statusX = margin + 150;
    dc.DrawText("Async Compute Engine", margin, margin);
    
    // Draw status indicator
    drawStatusIndicator(dc, statusX, margin + 5, 12, m_isRunning);
    
    wxFont statusFont = dc.GetFont();
    statusFont.SetPointSize(9);
    dc.SetFont(statusFont);
    dc.SetTextForeground(m_isRunning ? m_accentGreen : m_accentRed);
    dc.DrawText(m_isRunning ? "RUNNING" : "STOPPED", statusX + 18, margin + 3);
    
    int yPos = margin + headerHeight;
    
    // Calculate layout with improved responsive breakpoints
    int cols = (w >= 700) ? 4 : (w >= 450 ? 2 : 1);
    int cardW = (w - (cols + 1) * margin) / cols;
    int cardH = 80;
    
    wxFont labelFont = dc.GetFont();
    labelFont.SetPointSize(9);
    
    wxFont valueFont = dc.GetFont();
    valueFont.SetPointSize(18);
    valueFont.SetWeight(wxFONTWEIGHT_BOLD);
    
    wxFont subtitleFont = dc.GetFont();
    subtitleFont.SetPointSize(8);
    
    // Row 1: Key metrics
    int x = margin;
    int y = yPos;
    
    // Card 1: Queued Tasks
    drawCardBackground(dc, x, y, cardW, cardH, m_accentBlue);
    dc.SetFont(labelFont);
    dc.SetTextForeground(textDim);
    dc.DrawText("Queued Tasks", x + padding, y + padding);
    
    dc.SetFont(valueFont);
    dc.SetTextForeground(textStrong);
    dc.DrawText(wxString::Format("%zu", m_queuedTasks), x + padding, y + padding + 20);
    
    dc.SetFont(subtitleFont);
    dc.SetTextForeground(textDim);
    dc.DrawText(wxString::Format("%.0f avg", m_dispQueuedTasks), x + padding, y + cardH - padding - 12);
    
    if (cols > 1) {
        x += cardW + margin;
        
        // Card 2: Running Tasks
        drawCardBackground(dc, x, y, cardW, cardH, m_accentGreen);
        dc.SetFont(labelFont);
        dc.SetTextForeground(textDim);
        dc.DrawText("Running Tasks", x + padding, y + padding);
        
        dc.SetFont(valueFont);
        dc.SetTextForeground(textStrong);
        dc.DrawText(wxString::Format("%zu", m_runningTasks), x + padding, y + padding + 20);
        
        dc.SetFont(subtitleFont);
        dc.SetTextForeground(textDim);
        size_t maxThreads = std::thread::hardware_concurrency();
        dc.DrawText(wxString::Format("of %zu threads", maxThreads), x + padding, y + cardH - padding - 12);
    }
    
    if (cols > 2) {
        x += cardW + margin;
        
        // Card 3: Completed Tasks
        drawCardBackground(dc, x, y, cardW, cardH, m_accentPurple);
        dc.SetFont(labelFont);
        dc.SetTextForeground(textDim);
        dc.DrawText("Completed", x + padding, y + padding);
        
        dc.SetFont(valueFont);
        dc.SetTextForeground(textStrong);
        dc.DrawText(wxString::Format("%zu", m_completedTasks), x + padding, y + padding + 20);
        
        dc.SetFont(subtitleFont);
        dc.SetTextForeground(textDim);
        dc.DrawText(wxString::Format("%zu total", m_totalProcessed), x + padding, y + cardH - padding - 12);
    }
    
    if (cols > 3) {
        x += cardW + margin;
        
        // Card 4: Avg Execution Time
        drawCardBackground(dc, x, y, cardW, cardH, m_accentOrange);
        dc.SetFont(labelFont);
        dc.SetTextForeground(textDim);
        dc.DrawText("Avg Time", x + padding, y + padding);
        
        dc.SetFont(valueFont);
        dc.SetTextForeground(textStrong);
        dc.DrawText(wxString::Format("%.0f", m_dispAvgTimeMs), x + padding, y + padding + 20);
        
        dc.SetFont(subtitleFont);
        dc.SetTextForeground(textDim);
        dc.DrawText("ms/task", x + padding + 65, y + padding + 32);
        
        if (m_failedTasks > 0) {
            dc.SetTextForeground(m_accentRed);
            dc.DrawText(wxString::Format("%zu failed", m_failedTasks), x + padding, y + cardH - padding - 12);
        }
    }
    
    // Row 2: Sparklines
    if (h > 200) {
        y += cardH + margin;
        int sparkHeight = 60;
        int sparkWidth = (w - (cols + 1) * margin) / cols;
        
        x = margin;
        
        if (!m_histQueuedTasks.empty()) {
            drawSparkline(dc, x, y, sparkWidth, sparkHeight, 
                         m_histQueuedTasks, m_accentBlue, "Queue History");
        }
        
        if (cols > 1 && !m_histRunningTasks.empty()) {
            x += sparkWidth + margin;
            drawSparkline(dc, x, y, sparkWidth, sparkHeight,
                         m_histRunningTasks, m_accentGreen, "Active Tasks");
        }
        
        if (cols > 2 && !m_histAvgTimeMs.empty()) {
            x += sparkWidth + margin;
            drawSparkline(dc, x, y, sparkWidth, sparkHeight,
                         m_histAvgTimeMs, m_accentOrange, "Avg Time (ms)");
        }
    }
}

void AsyncEnginePanel::drawCardBackground(wxDC& dc, int x, int y, int w, int h, const wxColour& borderColor) {
    // Draw card with subtle shadow and border
    wxColour cardBg = GetBackgroundColour().ChangeLightness(105);
    wxColour shadowColor = GetBackgroundColour().ChangeLightness(90);
    
    // Shadow
    dc.SetBrush(wxBrush(shadowColor));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(x + 2, y + 2, w, h, 4);
    
    // Card background
    dc.SetBrush(wxBrush(cardBg));
    dc.SetPen(wxPen(borderColor.ChangeLightness(130), 1));
    dc.DrawRoundedRectangle(x, y, w, h, 4);
    
    // Top accent line
    dc.SetPen(wxPen(borderColor, 2));
    dc.DrawLine(x + 4, y, x + w - 4, y);
}

void AsyncEnginePanel::drawStatusIndicator(wxDC& dc, int x, int y, int size, bool active) {
    wxColour color = active ? m_accentGreen : m_accentRed;
    
    dc.SetBrush(wxBrush(color));
    dc.SetPen(wxPen(color.ChangeLightness(80), 1));
    dc.DrawCircle(x + size/2, y + size/2, size/2);
    
    if (active) {
        // Pulsing effect (simplified, just a lighter inner circle)
        dc.SetBrush(wxBrush(color.ChangeLightness(150)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawCircle(x + size/2, y + size/2, size/4);
    }
}

void AsyncEnginePanel::drawSparkline(wxDC& dc, int x, int y, int w, int h,
                                     const std::vector<size_t>& history,
                                     const wxColour& color,
                                     const wxString& label) {
    if (history.empty()) return;
    
    // Background
    wxColour bgColor = GetBackgroundColour().ChangeLightness(105);
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(wxPen(color.ChangeLightness(130), 1));
    dc.DrawRoundedRectangle(x, y, w, h, 4);
    
    // Label
    wxFont labelFont = dc.GetFont();
    labelFont.SetPointSize(8);
    dc.SetFont(labelFont);
    dc.SetTextForeground(wxColour(120, 120, 120));
    dc.DrawText(label, x + 6, y + 4);
    
    // Find max value
    size_t maxVal = dynamicMaxFromHist(history, 10);
    if (maxVal == 0) maxVal = 1;
    
    // Draw sparkline
    const int padding = 6;
    const int chartY = y + 22;
    const int chartH = h - 28;
    const int chartW = w - padding * 2;
    
    if (history.size() > 1) {
        wxPoint* points = new wxPoint[history.size()];
        double step = static_cast<double>(chartW) / (history.size() - 1);
        
        for (size_t i = 0; i < history.size(); ++i) {
            int px = x + padding + static_cast<int>(i * step);
            int py = chartY + chartH - static_cast<int>((history[i] * chartH) / maxVal);
            points[i] = wxPoint(px, py);
        }
        
        // Fill area
        wxColour fillColor = color;
        fillColor.Set(fillColor.Red(), fillColor.Green(), fillColor.Blue(), 60);
        dc.SetBrush(wxBrush(fillColor));
        dc.SetPen(*wxTRANSPARENT_PEN);
        
        wxPoint* fillPoints = new wxPoint[history.size() + 2];
        for (size_t i = 0; i < history.size(); ++i) {
            fillPoints[i] = points[i];
        }
        fillPoints[history.size()] = wxPoint(x + padding + chartW, chartY + chartH);
        fillPoints[history.size() + 1] = wxPoint(x + padding, chartY + chartH);
        
        dc.DrawPolygon(history.size() + 2, fillPoints);
        delete[] fillPoints;
        
        // Draw line
        dc.SetPen(wxPen(color, 2));
        dc.DrawLines(history.size(), points);
        
        delete[] points;
    }
    
    // Draw current value
    dc.SetFont(labelFont);
    dc.SetTextForeground(color);
    wxString valueText = wxString::Format("%zu", history.back());
    dc.DrawText(valueText, x + w - 35, y + 4);
}

void AsyncEnginePanel::clampPush(std::vector<size_t>& hist, size_t value, size_t maxSize) {
    hist.push_back(value);
    if (hist.size() > maxSize) {
        hist.erase(hist.begin());
    }
}

size_t AsyncEnginePanel::dynamicMaxFromHist(const std::vector<size_t>& hist, size_t minCap) {
    if (hist.empty()) return minCap;
    size_t maxVal = *std::max_element(hist.begin(), hist.end());
    return std::max(maxVal, minCap);
}

wxColour AsyncEnginePanel::lerpColor(const wxColour& a, const wxColour& b, double t) {
    t = std::clamp(t, 0.0, 1.0);
    return wxColour(
        static_cast<unsigned char>(a.Red() + (b.Red() - a.Red()) * t),
        static_cast<unsigned char>(a.Green() + (b.Green() - a.Green()) * t),
        static_cast<unsigned char>(a.Blue() + (b.Blue() - a.Blue()) * t)
    );
}

bool AsyncEnginePanel::hasEnoughContrast(const wxColour& textColor, const wxColour& bgColor) {
    auto luminance = [](const wxColour& c) {
        double r = c.Red() / 255.0;
        double g = c.Green() / 255.0;
        double b = c.Blue() / 255.0;
        r = (r <= 0.03928) ? r / 12.92 : std::pow((r + 0.055) / 1.055, 2.4);
        g = (g <= 0.03928) ? g / 12.92 : std::pow((g + 0.055) / 1.055, 2.4);
        b = (b <= 0.03928) ? b / 12.92 : std::pow((b + 0.055) / 1.055, 2.4);
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    };
    
    double l1 = luminance(textColor);
    double l2 = luminance(bgColor);
    double contrast = (std::max(l1, l2) + 0.05) / (std::min(l1, l2) + 0.05);
    return contrast >= 4.5;
}


