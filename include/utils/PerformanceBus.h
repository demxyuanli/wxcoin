#pragma once

#include <mutex>
#include <optional>

namespace perf {
	struct ScenePerfSample {
		int width{ 0 }; int height{ 0 }; const char* mode{ "QUALITY" };
		int viewportUs{ 0 }; int glSetupUs{ 0 }; int coinSceneMs{ 0 }; int totalSceneMs{ 0 }; double fps{ 0.0 };
	};
	struct EnginePerfSample {
		int contextUs{ 0 }; int clearUs{ 0 }; int viewportUs{ 0 }; int sceneMs{ 0 }; int totalMs{ 0 }; double fps{ 0.0 };
	};
	struct CanvasPerfSample {
		const char* mode{ "QUALITY" }; int mainSceneMs{ 0 }; int swapMs{ 0 }; int totalMs{ 0 }; double fps{ 0.0 };
	};

	class PerformanceBus {
	public:
		static PerformanceBus& instance();

		void setScene(const ScenePerfSample& s);
		void setEngine(const EnginePerfSample& e);
		void setCanvas(const CanvasPerfSample& c);

		std::optional<ScenePerfSample> getScene() const;
		std::optional<EnginePerfSample> getEngine() const;
		std::optional<CanvasPerfSample> getCanvas() const;

	private:
		PerformanceBus() = default;
		mutable std::mutex mtx_;
		std::optional<ScenePerfSample> scene_;
		std::optional<EnginePerfSample> engine_;
		std::optional<CanvasPerfSample> canvas_;
	};
}
