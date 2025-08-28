#pragma once

#include <memory>
#include <map>
#include <string>

#include <wx/gdicmn.h>
#include "DynamicSilhouetteRenderer.h"

class SceneManager;
class SoSeparator;
class OCCGeometry;
class PickingService;
// DynamicSilhouetteRenderer is used as a concrete implementation here

class HoverSilhouetteManager {
public:
	HoverSilhouetteManager(SceneManager* sceneManager,
		SoSeparator* occRoot,
		PickingService* pickingService);
	~HoverSilhouetteManager();

	void setHoveredSilhouette(std::shared_ptr<OCCGeometry> geometry);
	void updateHoverSilhouetteAt(const wxPoint& screenPos);
	void disableAll();

private:
	SceneManager* m_sceneManager;
	SoSeparator* m_occRoot;
	PickingService* m_pickingService;

	std::map<std::string, std::unique_ptr<DynamicSilhouetteRenderer>> m_silhouetteRenderers;
	std::weak_ptr<OCCGeometry> m_lastHoverGeometry;
};
