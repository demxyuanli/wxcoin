#include "edges/EdgeExtractionUIHelper.h"
#include "FlatFrame.h"
#include "flatui/FlatUIStatusBar.h"
#include "logger/AsyncLogger.h"
#include <sstream>
#include <iomanip>

std::string EdgeExtractionUIHelper::Statistics::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Edges: " << totalEdges;
    
    if (intersectionNodes > 0) {
        oss << " | Nodes: " << intersectionNodes;
    }
    
    if (sampledPoints > 0) {
        oss << " | Points: " << sampledPoints;
    }
    
    oss << " | Time: " << extractionTime << "s";
    
    if (intersectionTime > 0.001) {
        oss << " (+" << intersectionTime << "s intersection)";
    }
    
    return oss.str();
}

EdgeExtractionUIHelper::EdgeExtractionUIHelper(wxFrame* frame)
    : m_frame(frame)
    , m_statusBar(nullptr)
    , m_cursorChanged(false)
    , m_progressEnabled(false)
{
    if (m_frame) {
        // Try to get FlatFrame and its status bar
        FlatFrame* flatFrame = dynamic_cast<FlatFrame*>(m_frame);
        if (flatFrame) {
            m_statusBar = flatFrame->GetFlatUIStatusBar();
        }
        
        if (!m_statusBar) {
            LOG_WRN_S_ASYNC("EdgeExtractionUIHelper: Status bar not available, progress will not be shown");
        }
        
        // Save original cursor
        m_originalCursor = m_frame->GetCursor();
    }
}

EdgeExtractionUIHelper::~EdgeExtractionUIHelper() {
    endOperation();
}

void EdgeExtractionUIHelper::beginOperation(const std::string& operationName) {
    m_operationName = operationName;
    m_startTime = std::chrono::high_resolution_clock::now();
    
    setWaitingCursor();
    enableProgressBar();
    
    updateProgress(0, operationName + " starting...");
}

void EdgeExtractionUIHelper::endOperation() {
    if (m_progressEnabled) {
        updateProgress(100, m_operationName + " completed");
        wxMilliSleep(500); // Brief pause to show 100%
        disableProgressBar();
    }
    
    restoreCursor();
    
    if (hasUI()) {
        showFinalStatistics();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double totalTime = std::chrono::duration<double>(endTime - m_startTime).count();
}

void EdgeExtractionUIHelper::updateProgress(int progress, const std::string& message) {
	if (!m_statusBar) return;
	
	progress = std::max(0, std::min(100, progress));
	
	m_statusBar->SetGaugeValue(progress);
	m_statusBar->SetStatusText(wxString::FromUTF8(message), 0);
	m_statusBar->Refresh();
	wxYield(); // Allow UI to update
}

void EdgeExtractionUIHelper::setIndeterminateProgress(bool indeterminate, const std::string& message) {
	if (!m_statusBar) return;
	
	if (indeterminate) {
		// Enable progress bar if not already enabled
		if (!m_progressEnabled) {
			enableProgressBar();
		}
		
		// Set to indeterminate mode with animation
		m_statusBar->SetGaugeIndeterminate(true);
	} else {
		// Set back to normal progress mode
		m_statusBar->SetGaugeIndeterminate(false);
	}
	
	if (!message.empty()) {
		m_statusBar->SetStatusText(wxString::FromUTF8(message), 0);
	}
	m_statusBar->Refresh();
	wxYield(); // Allow UI to update
}

void EdgeExtractionUIHelper::setStatistics(const Statistics& stats) {
    m_stats = stats;
}

std::function<void(int, const std::string&)> EdgeExtractionUIHelper::getProgressCallback() {
    return [this](int progress, const std::string& message) {
        updateProgress(progress, message);
    };
}

void EdgeExtractionUIHelper::showFinalStatistics() {
    if (!m_statusBar) return;
    
    std::string statsText = m_stats.toString();
    m_statusBar->SetStatusText(wxString::FromUTF8(statsText), 0);
    m_statusBar->Refresh();
}

void EdgeExtractionUIHelper::setWaitingCursor() {
    if (!m_frame || m_cursorChanged) return;
    
    m_frame->SetCursor(wxCURSOR_WAIT);
    m_cursorChanged = true;
}

void EdgeExtractionUIHelper::restoreCursor() {
    if (!m_frame || !m_cursorChanged) return;
    
    m_frame->SetCursor(m_originalCursor);
    m_cursorChanged = false;
}

void EdgeExtractionUIHelper::enableProgressBar() {
    if (!m_statusBar || m_progressEnabled) return;
    
    m_statusBar->EnableProgressGauge(true);
    m_statusBar->SetGaugeRange(100);
    m_statusBar->SetGaugeValue(0);
    m_progressEnabled = true;
}

void EdgeExtractionUIHelper::disableProgressBar() {
    if (!m_statusBar || !m_progressEnabled) return;
    
    m_statusBar->EnableProgressGauge(false);
    m_progressEnabled = false;
}

void EdgeExtractionUIHelper::updateStatusText(const wxString& text) {
    if (!m_statusBar) return;
    
    m_statusBar->SetStatusText(text, 0);
    m_statusBar->Refresh();
}


