#pragma once

#include <memory>
#include <vector>
#include <wx/gdicmn.h>

// Forward declarations
class OCCGeometry;
class SceneManager;
class SoSeparator;

/**
 * @brief Service for picking geometry objects from screen coordinates
 * 
 * Provides functionality to convert screen coordinates to 3D geometry
 * selections using Coin3D picking mechanisms.
 */
class PickingService {
public:
    /**
     * @brief Constructor
     * @param sceneManager Scene manager for viewport access
     * @param geometries Pointer to geometry collection
     */
    PickingService(SceneManager* sceneManager, 
                   std::vector<std::shared_ptr<OCCGeometry>>* geometries);
    
    /**
     * @brief Destructor
     */
    ~PickingService();

    /**
     * @brief Pick geometry at screen coordinates
     * @param screenPos Screen position to pick from
     * @return Picked geometry object, or nullptr if nothing picked
     */
    std::shared_ptr<OCCGeometry> pickGeometryAtScreen(const wxPoint& screenPos);

    /**
     * @brief Pick geometry at screen coordinates with tolerance
     * @param screenPos Screen position to pick from
     * @param tolerance Picking tolerance in pixels
     * @return Picked geometry object, or nullptr if nothing picked
     */
    std::shared_ptr<OCCGeometry> pickGeometryAtScreen(const wxPoint& screenPos, int tolerance);

    /**
     * @brief Pick all geometries under screen coordinates
     * @param screenPos Screen position to pick from
     * @return Vector of picked geometry objects (may be empty)
     */
    std::vector<std::shared_ptr<OCCGeometry>> pickAllGeometriesAtScreen(const wxPoint& screenPos);

    /**
     * @brief Enable or disable picking
     * @param enabled True to enable picking
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if picking is enabled
     * @return True if picking is enabled
     */
    bool isEnabled() const { return m_enabled; }

private:
    bool isNodeInSubtree(SoNode* targetNode, SoNode* rootNode);

    SceneManager* m_sceneManager;
    std::vector<std::shared_ptr<OCCGeometry>>* m_geometries;
    bool m_enabled = true;
};