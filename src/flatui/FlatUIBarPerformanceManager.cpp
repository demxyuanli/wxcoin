#include "flatui/FlatUIBarPerformanceManager.h"
#include "flatui/FlatUIBar.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include <wx/display.h>
#include <algorithm>

#ifdef __WXMSW__
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

FlatUIBarPerformanceManager::FlatUIBarPerformanceManager(FlatUIBar* bar)
	: m_bar(bar)
	, m_currentDPIScale(1.0)
	, m_hardwareAcceleration(true)
	, m_batchPainting(false)
	, m_optimizationFlags(PerformanceOptimization::ALL)
	, m_cachedGraphicsContext(nullptr)
	, m_lastContextSize(0, 0)
	, m_contextNeedsUpdate(true)
	, m_hasInvalidRegions(false)
{
	UpdateDPIScale();

	// Enable hardware acceleration by default on Windows
#ifdef __WXMSW__
	EnableWindowsComposition();
#endif

	LOG_INF("FlatUIBarPerformanceManager initialized with DPI scale: " +
		std::to_string(m_currentDPIScale), "PerformanceManager");
}

FlatUIBarPerformanceManager::~FlatUIBarPerformanceManager()
{
	// Clean up cached graphics context
	if (m_cachedGraphicsContext) {
		delete m_cachedGraphicsContext;
		m_cachedGraphicsContext = nullptr;
	}
	
	ClearResourceCache();
	LogPerformanceStats();
}

double FlatUIBarPerformanceManager::GetCurrentDPIScale() const
{
	return m_currentDPIScale;
}

void FlatUIBarPerformanceManager::OnDPIChanged()
{
	double oldScale = m_currentDPIScale;
	UpdateDPIScale();

	if (oldScale != m_currentDPIScale) {
		// Clear caches when DPI changes
		ClearResourceCache();
		InvalidateGraphicsContext();  // Invalidate cached graphics context
		InvalidateAll();

		LOG_INF("DPI changed from " + std::to_string(oldScale) + " to " +
			std::to_string(m_currentDPIScale), "PerformanceManager");
	}
}

wxSize FlatUIBarPerformanceManager::FromDIP(const wxSize& size) const
{
	if (m_currentDPIScale == 1.0) return size;

	return wxSize(
		static_cast<int>(size.GetWidth() * m_currentDPIScale),
		static_cast<int>(size.GetHeight() * m_currentDPIScale)
	);
}

wxPoint FlatUIBarPerformanceManager::FromDIP(const wxPoint& point) const
{
	if (m_currentDPIScale == 1.0) return point;

	return wxPoint(
		static_cast<int>(point.x * m_currentDPIScale),
		static_cast<int>(point.y * m_currentDPIScale)
	);
}

int FlatUIBarPerformanceManager::FromDIP(int value) const
{
	if (m_currentDPIScale == 1.0) return value;
	return static_cast<int>(value * m_currentDPIScale);
}

wxFont FlatUIBarPerformanceManager::GetDPIAwareFont(const wxString& fontKey) const
{
	if (!IsOptimizationEnabled(PerformanceOptimization::DPI_OPTIMIZATION)) {
		return CFG_DEFAULTFONT();
	}

	return GetCachedFont(fontKey, CFG_DEFAULTFONT());
}

wxBitmap FlatUIBarPerformanceManager::GetDPIAwareBitmap(const wxString& key, const wxBitmap& originalBitmap)
{
	if (!IsOptimizationEnabled(PerformanceOptimization::RESOURCE_CACHING) ||
		!originalBitmap.IsOk() || m_currentDPIScale == 1.0) {
		return originalBitmap;
	}

	wxString cacheKey = GenerateCacheKey(key, m_currentDPIScale);

	auto it = m_bitmapCache.find(cacheKey);
	if (it != m_bitmapCache.end() && it->second.scaleFactor == m_currentDPIScale) {
		return it->second.bitmap;
	}

	// Create DPI-scaled bitmap
	wxSize scaledSize = FromDIP(originalBitmap.GetSize());
	wxBitmap scaledBitmap = originalBitmap.ConvertToImage()
		.Scale(scaledSize.GetWidth(), scaledSize.GetHeight(), wxIMAGE_QUALITY_HIGH);

	m_bitmapCache[cacheKey] = DPIAwareResource(m_currentDPIScale, scaledBitmap);

	LOG_DBG("Created DPI-aware bitmap for key: " + key.ToStdString() +
		" at scale: " + std::to_string(m_currentDPIScale), "PerformanceManager");

	return scaledBitmap;
}

wxFont FlatUIBarPerformanceManager::GetCachedFont() const
{
	return CFG_DEFAULTFONT();
}

wxFont FlatUIBarPerformanceManager::GetCachedFont(const wxString& key, const wxFont& originalFont) const
{
	if (!IsOptimizationEnabled(PerformanceOptimization::RESOURCE_CACHING) ||
		m_currentDPIScale == 1.0) {
		return originalFont;
	}

	wxString cacheKey = GenerateCacheKey(key, m_currentDPIScale);

	auto it = m_fontCache.find(cacheKey);
	if (it != m_fontCache.end() && it->second.scaleFactor == m_currentDPIScale) {
		return it->second.font;
	}

	// Create DPI-scaled font
	wxFont scaledFont = originalFont;
	int newSize = FromDIP(originalFont.GetPointSize());
	scaledFont.SetPointSize(newSize);

	m_fontCache[cacheKey] = DPIAwareResource(m_currentDPIScale, scaledFont);

	return scaledFont;
}

int FlatUIBarPerformanceManager::GetDPIAwareValue(const wxString& key, int originalValue)
{
	if (!IsOptimizationEnabled(PerformanceOptimization::DPI_OPTIMIZATION) ||
		m_currentDPIScale == 1.0) {
		return originalValue;
	}

	wxString cacheKey = GenerateCacheKey(key, m_currentDPIScale);

	auto it = m_valueCache.find(cacheKey);
	if (it != m_valueCache.end() && it->second.scaleFactor == m_currentDPIScale) {
		return it->second.intValue;
	}

	int scaledValue = FromDIP(originalValue);
	m_valueCache[cacheKey] = DPIAwareResource(m_currentDPIScale, scaledValue);

	return scaledValue;
}

void FlatUIBarPerformanceManager::ClearResourceCache()
{
	m_bitmapCache.clear();
	m_fontCache.clear();
	m_valueCache.clear();

	LOG_DBG("Resource cache cleared", "PerformanceManager");
}

void FlatUIBarPerformanceManager::EnableHardwareAcceleration(bool enable)
{
	m_hardwareAcceleration = enable;

	if (enable) {
		m_optimizationFlags = m_optimizationFlags | PerformanceOptimization::HARDWARE_ACCELERATION;
	}
	else {
		m_optimizationFlags = static_cast<PerformanceOptimization>(
			static_cast<int>(m_optimizationFlags) & ~static_cast<int>(PerformanceOptimization::HARDWARE_ACCELERATION)
			);
	}

	LOG_INF("Hardware acceleration " + std::string(enable ? "enabled" : "disabled"), "PerformanceManager");
}

bool FlatUIBarPerformanceManager::IsHardwareAccelerationEnabled() const
{
	return m_hardwareAcceleration && IsOptimizationEnabled(PerformanceOptimization::HARDWARE_ACCELERATION);
}

wxGraphicsContext* FlatUIBarPerformanceManager::CreateOptimizedGraphicsContext(wxDC& dc)
{
	// Check if we can reuse cached graphics context
	if (m_bar && !m_contextNeedsUpdate && m_cachedGraphicsContext) {
		wxSize currentSize = m_bar->GetClientSize();
		if (currentSize == m_lastContextSize) {
			// Reuse cached context - fastest path
			return m_cachedGraphicsContext;
		}
	}

	// Clean up old context if size changed
	if (m_cachedGraphicsContext) {
		delete m_cachedGraphicsContext;
		m_cachedGraphicsContext = nullptr;
	}

	// Create new graphics context with minimal overhead
	wxGraphicsContext* gc = nullptr;

	// Use window-based creation as primary method - most reliable
	if (m_bar) {
		gc = wxGraphicsContext::Create(m_bar);
	}

	// Hardware acceleration fallback on Windows
	if (!gc && IsHardwareAccelerationEnabled()) {
#ifdef __WXMSW__
		if (wxGraphicsRenderer* renderer = wxGraphicsRenderer::GetDirect2DRenderer()) {
			gc = wxGraphicsContext::Create(m_bar);
		}
#endif
	}

	// Configure and cache the graphics context
	if (gc) {
		gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);
		
		// Cache for reuse
		m_cachedGraphicsContext = gc;
		m_lastContextSize = m_bar ? m_bar->GetClientSize() : wxSize(0, 0);
		m_contextNeedsUpdate = false;
	}

	return gc;
}

void FlatUIBarPerformanceManager::InvalidateGraphicsContext()
{
	m_contextNeedsUpdate = true;
}

void FlatUIBarPerformanceManager::InvalidateRegion(const wxRect& region)
{
	if (!IsOptimizationEnabled(PerformanceOptimization::DIRTY_REGION_TRACKING)) {
		return;
	}

	m_invalidRegions.push_back(region);
	m_hasInvalidRegions = true;

	// Optimize by merging overlapping regions
	if (m_invalidRegions.size() > 10) {
		std::sort(m_invalidRegions.begin(), m_invalidRegions.end(),
			[](const wxRect& a, const wxRect& b) { return a.x < b.x; });

		std::vector<wxRect> merged;
		merged.reserve(m_invalidRegions.size());

		for (const auto& rect : m_invalidRegions) {
			if (merged.empty() || !merged.back().Intersects(rect)) {
				merged.push_back(rect);
			}
			else {
				merged.back() = merged.back().Union(rect);
			}
		}

		m_invalidRegions = std::move(merged);
	}
}

void FlatUIBarPerformanceManager::InvalidateAll()
{
	m_invalidRegions.clear();
	if (m_bar) {
		m_invalidRegions.push_back(m_bar->GetClientRect());
	}
	m_hasInvalidRegions = true;
}

bool FlatUIBarPerformanceManager::HasInvalidRegions() const
{
	return m_hasInvalidRegions && !m_invalidRegions.empty();
}

std::vector<wxRect> FlatUIBarPerformanceManager::GetInvalidRegions() const
{
	return m_invalidRegions;
}

void FlatUIBarPerformanceManager::ClearInvalidRegions()
{
	m_invalidRegions.clear();
	m_hasInvalidRegions = false;
}

void FlatUIBarPerformanceManager::BeginBatchPaint()
{
	if (!IsOptimizationEnabled(PerformanceOptimization::BATCH_PAINTING)) {
		return;
	}

	m_batchPainting = true;
	m_queuedOperations.clear();

	StartPerformanceTimer("BatchPaint");
}

void FlatUIBarPerformanceManager::EndBatchPaint()
{
	if (!m_batchPainting) return;

	m_batchPainting = false;

	// Execute all queued operations in a single paint cycle
	if (!m_queuedOperations.empty() && m_bar) {
		wxClientDC dc(m_bar);
		wxGraphicsContext* gc = CreateOptimizedGraphicsContext(dc);

		if (gc) {
			for (const auto& operation : m_queuedOperations) {
				operation(gc);
			}
			// NOTE: Do NOT delete gc here - it's cached by the performance manager
		}

		m_queuedOperations.clear();
	}

	EndPerformanceTimer("BatchPaint");
}

bool FlatUIBarPerformanceManager::IsBatchPainting() const
{
	return m_batchPainting;
}

void FlatUIBarPerformanceManager::QueuePaintOperation(std::function<void(wxGraphicsContext*)> operation)
{
	if (m_batchPainting) {
		m_queuedOperations.push_back(operation);
	}
	else {
		// Execute immediately if not batching
		if (m_bar) {
			wxClientDC dc(m_bar);
			wxGraphicsContext* gc = CreateOptimizedGraphicsContext(dc);
			if (gc) {
				operation(gc);
				// NOTE: Do NOT delete gc here - it's cached by the performance manager
			}
		}
	}
}

void FlatUIBarPerformanceManager::StartPerformanceTimer(const wxString& operation)
{
	m_performanceTimers[operation] = wxGetLocalTimeMillis();
}

void FlatUIBarPerformanceManager::EndPerformanceTimer(const wxString& operation)
{
	auto it = m_performanceTimers.find(operation);
	if (it != m_performanceTimers.end()) {
		wxLongLong elapsed = wxGetLocalTimeMillis() - it->second;
		m_performanceStats[operation].push_back(elapsed.ToDouble());
		m_performanceTimers.erase(it);
	}
}

void FlatUIBarPerformanceManager::LogPerformanceStats() const
{
	for (const auto& stat : m_performanceStats) {
		if (stat.second.empty()) continue;

		double total = 0.0;
		double max_time = 0.0;
		double min_time = stat.second[0];

		for (double time : stat.second) {
			total += time;
			max_time = (std::max)(max_time, time);
			min_time = std::min(min_time, time);
		}

		double average = total / stat.second.size();

		LOG_INF("Performance stats for " + stat.first.ToStdString() +
			": avg=" + std::to_string(average) + "ms" +
			", min=" + std::to_string(min_time) + "ms" +
			", max=" + std::to_string(max_time) + "ms" +
			", count=" + std::to_string(stat.second.size()), "PerformanceManager");
	}
}

void FlatUIBarPerformanceManager::SetOptimizationFlags(PerformanceOptimization flags)
{
	m_optimizationFlags = flags;
	LOG_INF("Optimization flags set to: " + std::to_string(static_cast<int>(flags)), "PerformanceManager");
}

PerformanceOptimization FlatUIBarPerformanceManager::GetOptimizationFlags() const
{
	return m_optimizationFlags;
}

void FlatUIBarPerformanceManager::OptimizeMemoryUsage()
{
	CleanupExpiredCacheEntries();

	// Compact performance stats if they get too large
	for (auto& stat : m_performanceStats) {
		if (stat.second.size() > 1000) {
			// Keep only the last 100 entries
			stat.second.erase(stat.second.begin(), stat.second.end() - 100);
		}
	}

	LOG_DBG("Memory optimization completed", "PerformanceManager");
}

void FlatUIBarPerformanceManager::PreloadResources()
{
	if (!IsOptimizationEnabled(PerformanceOptimization::RESOURCE_CACHING)) {
		return;
	}

	// Preload common resources
	wxFont defaultFont = CFG_DEFAULTFONT();
	GetCachedFont("DefaultFont", defaultFont);

	// Preload common DPI values
	std::vector<wxString> commonKeys = {
		"BarPadding", "BarTabPadding", "BarTabSpacing", "BarRenderHeight"
	};

	for (const auto& key : commonKeys) {
		GetDPIAwareValue(key, CFG_INT(key.ToStdString()));
	}

	LOG_INF("Resource preloading completed", "PerformanceManager");
}

#ifdef __WXMSW__
void FlatUIBarPerformanceManager::EnableWindowsComposition()
{
	if (!m_bar) return;

	HWND hwnd = (HWND)m_bar->GetHandle();
	if (!hwnd) return;

	// Enable DWM composition for better performance
	BOOL dwmEnabled = FALSE;
	if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled)) && dwmEnabled) {
		// Enable hardware acceleration
		long exStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
		::SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED);

		LOG_INF("Windows composition enabled for hardware acceleration", "PerformanceManager");
	}
}

void FlatUIBarPerformanceManager::SetLayeredWindowAttributes()
{
	if (!m_bar) return;

	HWND hwnd = (HWND)m_bar->GetHandle();
	if (!hwnd) return;

	// Enable per-pixel alpha for better rendering quality
	long exStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
	::SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

	// Set alpha blending for smooth rendering
	::SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

	LOG_DBG("Layered window attributes set", "PerformanceManager");
}
#endif

void FlatUIBarPerformanceManager::UpdateDPIScale()
{
	if (!m_bar) {
		m_currentDPIScale = 1.0;
		return;
	}

#ifdef __WXMSW__
	HDC hdc = ::GetDC(NULL);
	if (hdc) {
		int dpiX = ::GetDeviceCaps(hdc, LOGPIXELSX);
		m_currentDPIScale = dpiX / 96.0;
		::ReleaseDC(NULL, hdc);
	}
#else
	m_currentDPIScale = m_bar->GetContentScaleFactor();
#endif
}

wxString FlatUIBarPerformanceManager::GenerateCacheKey(const wxString& baseKey, double scaleFactor) const
{
	return baseKey + "_" + wxString::Format("%.2f", scaleFactor);
}

void FlatUIBarPerformanceManager::CleanupExpiredCacheEntries()
{
	// Remove cache entries that don't match current DPI scale
	auto cleanupCache = [this](auto& cache) {
		for (auto it = cache.begin(); it != cache.end();) {
			if (std::abs(it->second.scaleFactor - m_currentDPIScale) > 0.01) {
				it = cache.erase(it);
			}
			else {
				++it;
			}
		}
		};

	cleanupCache(m_bitmapCache);
	cleanupCache(m_fontCache);
	cleanupCache(m_valueCache);
}

bool FlatUIBarPerformanceManager::IsOptimizationEnabled(PerformanceOptimization opt) const
{
	return (m_optimizationFlags & opt) == opt;
}