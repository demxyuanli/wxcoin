#pragma once

#include <memory>
#include <string>
#include <vector>

#include <OpenCASCADE/Quantity_Color.hxx>

class SceneManager;
class OCCGeometry;

class SelectionManager {
public:
	SelectionManager(SceneManager* sceneManager,
		std::vector<std::shared_ptr<OCCGeometry>>* allGeometries,
		std::vector<std::shared_ptr<OCCGeometry>>* selectedGeometries);

	void setGeometryVisible(const std::string& name, bool visible);
	void setGeometrySelected(const std::string& name, bool selected);
	void setGeometryColor(const std::string& name, const Quantity_Color& color);
	void setGeometryTransparency(const std::string& name, double transparency);

	void hideAll();
	void showAll();
	void selectAll();
	void deselectAll();

	void onSelectionChanged();

private:
	std::shared_ptr<OCCGeometry> findGeometry(const std::string& name);
	void requestRefreshSelectionChanged();
	void requestRefreshMaterialChanged();

	SceneManager* m_sceneManager;
	std::vector<std::shared_ptr<OCCGeometry>>* m_allGeometries;
	std::vector<std::shared_ptr<OCCGeometry>>* m_selectedGeometries;
};
