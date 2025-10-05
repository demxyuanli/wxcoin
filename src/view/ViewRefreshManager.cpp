#include "ViewRefreshManager.h"
#include "Canvas.h"
#include "logger/Logger.h"
#include <wx/app.h>      // wxCallAfter
#include <wx/thread.h>   // wxThread::IsMain

wxBEGIN_EVENT_TABLE(ViewRefreshManager, wxEvtHandler)
EVT_TIMER(wxID_ANY, ViewRefreshManager::onDebounceTimer)
wxEND_EVENT_TABLE()

ViewRefreshManager::ViewRefreshManager(Canvas* canvas)
	: m_canvas(canvas)
	, m_debounceTimer(this)
	, m_pendingReason(RefreshReason::MANUAL_REQUEST)
	, m_hasPendingRefresh(false)
	, m_debounceTime(16)  // ~60fps
	, m_enabled(true)
{
	LOG_INF_S("ViewRefreshManager: Initialized");
}

ViewRefreshManager::~ViewRefreshManager() {
	m_debounceTimer.Stop();
	removeAllListeners();
	LOG_INF_S("ViewRefreshManager: Destroyed");
}

void ViewRefreshManager::requestRefresh(RefreshReason reason, bool immediate) {
	if (!m_enabled || !m_canvas) {
		LOG_WRN_S("VIEW REFRESH: Manager disabled or no canvas available");
		return;
	}

	LOG_INF_S("=== VIEW REFRESH: REQUESTING REFRESH (reason=" +
		std::to_string(static_cast<int>(reason)) + ", immediate=" +
		std::string(immediate ? "true" : "false") + ") ===");

	// Ensure all UI-related operations run on the main thread
	if (!wxThread::IsMain()) {
		LOG_INF_S("VIEW REFRESH: Switching to main thread for refresh");
		this->CallAfter([this, reason, immediate]() {
			// Re-enter on the GUI thread
			this->requestRefresh(reason, immediate);
			});
		return;
	}

	if (immediate) {
		// Cancel any pending debounced refresh
		m_debounceTimer.Stop();
		m_hasPendingRefresh = false;
		performRefresh(reason);
	}
	else {
		// Use debouncing to avoid excessive refreshes
		m_pendingReason = reason;
		m_hasPendingRefresh = true;

		if (!m_debounceTimer.IsRunning()) {
			m_debounceTimer.Start(m_debounceTime, wxTIMER_ONE_SHOT);
		}
		LOG_INF_S("VIEW REFRESH: Debounced refresh scheduled");
	}
}

void ViewRefreshManager::addRefreshListener(RefreshListener listener) {
	m_listeners.push_back(listener);
}

void ViewRefreshManager::removeAllListeners() {
	m_listeners.clear();
}

void ViewRefreshManager::performRefresh(RefreshReason reason) {
	if (!m_canvas) {
		LOG_WRN_S("VIEW REFRESH: No canvas available for refresh");
		return;
	}

	LOG_INF_S("=== VIEW REFRESH: PERFORMING REFRESH (reason=" + std::to_string(static_cast<int>(reason)) + ") ===");

	// Notify all listeners before refresh
	if (!m_listeners.empty()) {
		LOG_INF_S("VIEW REFRESH: Notifying " + std::to_string(m_listeners.size()) + " listeners");
		for (const auto& listener : m_listeners) {
			try {
				listener(reason);
			}
			catch (const std::exception& e) {
				LOG_ERR_S("VIEW REFRESH: Listener exception: " + std::string(e.what()));
			}
		}
	}

	// Perform the actual refresh: use wxWidgets paint system
	LOG_INF_S("VIEW REFRESH: Calling canvas Refresh()");
	m_canvas->Refresh(false);

	// If immediate update is needed, also call Update()
	if (reason == RefreshReason::CAMERA_MOVED || reason == RefreshReason::SELECTION_CHANGED) {
		LOG_INF_S("VIEW REFRESH: Calling canvas Update() for immediate refresh");
		m_canvas->Update();  // Force immediate paint for interactive operations
	}

	LOG_INF_S("=== VIEW REFRESH: REFRESH COMPLETED ===");
}

void ViewRefreshManager::onDebounceTimer(wxTimerEvent& event) {
	if (m_hasPendingRefresh) {
		performRefresh(m_pendingReason);
		m_hasPendingRefresh = false;
	}
}