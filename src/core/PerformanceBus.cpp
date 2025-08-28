#include "utils/PerformanceBus.h"

namespace perf {
	PerformanceBus& PerformanceBus::instance() { static PerformanceBus b; return b; }

	void PerformanceBus::setScene(const ScenePerfSample& s) { std::lock_guard<std::mutex> lk(mtx_); scene_ = s; }
	void PerformanceBus::setEngine(const EnginePerfSample& e) { std::lock_guard<std::mutex> lk(mtx_); engine_ = e; }
	void PerformanceBus::setCanvas(const CanvasPerfSample& c) { std::lock_guard<std::mutex> lk(mtx_); canvas_ = c; }

	std::optional<ScenePerfSample> PerformanceBus::getScene() const { std::lock_guard<std::mutex> lk(mtx_); return scene_; }
	std::optional<EnginePerfSample> PerformanceBus::getEngine() const { std::lock_guard<std::mutex> lk(mtx_); return engine_; }
	std::optional<CanvasPerfSample> PerformanceBus::getCanvas() const { std::lock_guard<std::mutex> lk(mtx_); return canvas_; }
}