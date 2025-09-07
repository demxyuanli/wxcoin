#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

class SceneManager;
class SoSeparator;
class OCCGeometry;
class DynamicSilhouetteRenderer;

class OutlineDisplayManager {
public:
	OutlineDisplayManager(SceneManager* sceneManager,
		SoSeparator* occRoot,
		std::vector<std::shared_ptr<OCCGeometry>>* geometries);
	~OutlineDisplayManager();

	void setEnabled(bool enabled);
	bool isEnabled() const { return m_enabled; }

	// Hover mode support
	void setHoverMode(bool hover) { m_hoverMode = hover; }
	bool isHoverMode() const { return m_hoverMode; }
	void setHoveredGeometry(std::shared_ptr<OCCGeometry> geometry);

	void onGeometryAdded(const std::shared_ptr<OCCGeometry>& geometry);
	void updateAll();
	void clearAll();

	// Parameter control
	void setParams(const struct ImageOutlineParams& params);
	struct ImageOutlineParams getParams() const;

	void refreshOutlineAll();

private:
	SceneManager* m_sceneManager;
	SoSeparator* m_occRoot;
	std::vector<std::shared_ptr<OCCGeometry>>* m_geometries;
	bool m_enabled{ false };
	bool m_hoverMode{ true }; // Default to hover mode
	std::weak_ptr<OCCGeometry> m_hoveredGeometry;

	std::map<std::string, std::unique_ptr<DynamicSilhouetteRenderer>> m_outlineByName;

	void ensureForGeometry(const std::shared_ptr<OCCGeometry>& geometry);
	// Image-space pass (preferred)
	std::unique_ptr<class ImageOutlinePass> m_imagePass;
};
