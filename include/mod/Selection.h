#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

class OCCGeometry;

namespace mod {

/**
 * @brief Selection change message types (similar to FreeCAD)
 */
enum class SelectionChangeType {
	AddSelection,      // Add to selection
	RemoveSelection,   // Remove from selection
	SetSelection,      // Set selection (replace all)
	ClearSelection,    // Clear all selection
	SetPreselect,      // Set preselection (hover)
	RemovePreselect,   // Remove preselection
	MovePreselect      // Move preselection (mouse move)
};

/**
 * @brief Selection change message (similar to FreeCAD SelectionChanges)
 */
struct SelectionChange {
	SelectionChangeType type;
	std::string geometryName;    // Name of the geometry object
	std::string subElementName;  // Sub-element name like "Face5", "Edge12", or empty for whole object
	std::string elementType;     // "Face", "Edge", "Vertex", or empty
	float x, y, z;               // 3D coordinates where selection occurred
	
	SelectionChange(SelectionChangeType t = SelectionChangeType::ClearSelection,
		const std::string& geomName = "",
		const std::string& subName = "",
		const std::string& elemType = "",
		float px = 0.0f, float py = 0.0f, float pz = 0.0f)
		: type(t), geometryName(geomName), subElementName(subName), elementType(elemType)
		, x(px), y(py), z(pz) {}
};

/**
 * @brief Selection observer callback type
 */
using SelectionObserverCallback = std::function<void(const SelectionChange&)>;

/**
 * @brief Selection system (similar to FreeCAD SelectionSingleton)
 * 
 * Manages selection and preselection state, and notifies observers of changes.
 */
class Selection {
public:
	static Selection& getInstance();
	
	// Selection management
	bool addSelection(const std::string& geometryName, const std::string& subElementName = "",
		const std::string& elementType = "", float x = 0.0f, float y = 0.0f, float z = 0.0f);
	bool removeSelection(const std::string& geometryName, const std::string& subElementName = "");
	void setSelection(const std::string& geometryName, const std::string& subElementName = "",
		const std::string& elementType = "", float x = 0.0f, float y = 0.0f, float z = 0.0f);
	void clearSelection();
	
	// Preselection management (hover)
	int setPreselect(const std::string& geometryName, const std::string& subElementName = "",
		const std::string& elementType = "", float x = 0.0f, float y = 0.0f, float z = 0.0f);
	void removePreselect();
	const SelectionChange& getPreselection() const { return m_preselection; }
	
	// Selection query
	bool isSelected(const std::string& geometryName, const std::string& subElementName = "") const;
	std::vector<SelectionChange> getSelection() const { return m_selection; }
	
	// Observer management
	void addObserver(SelectionObserverCallback callback);
	void removeObserver(SelectionObserverCallback callback);
	
private:
	Selection() = default;
	~Selection() = default;
	Selection(const Selection&) = delete;
	Selection& operator=(const Selection&) = delete;
	
	void notifyObservers(const SelectionChange& change);
	
	std::vector<SelectionChange> m_selection;
	SelectionChange m_preselection;
	std::vector<SelectionObserverCallback> m_observers;
};

} // namespace mod

