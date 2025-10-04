#ifndef MESH_PARAMETER_MANAGER_H
#define MESH_PARAMETER_MANAGER_H

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <map>
#include "rendering/GeometryProcessor.h"

// Forward declarations
class OCCGeometry;
class OCCViewer;

/**
 * Unified mesh parameter management system
 * Centralizes all parameter storage and provides consistent application to geometries
 */
class MeshParameterManager {
public:
    // Parameter categories
    enum class Category {
        BASIC_MESH,      // deflection, angularDeflection, relative, inParallel
        SUBDIVISION,     // subdivision parameters
        SMOOTHING,       // smoothing parameters  
        TESSELLATION,    // tessellation quality and methods
        LOD,             // level of detail parameters
        PERFORMANCE      // parallel processing, adaptive meshing
    };

    // Parameter change notification
    struct ParameterChange {
        Category category;
        std::string name;
        double oldValue;
        double newValue;
    };

    using ParameterChangeCallback = std::function<void(const ParameterChange&)>;

    static MeshParameterManager& getInstance();

    // === Parameter Management ===
    
    /**
     * Set parameter value by category and name
     * Triggers automatic validation and dependency updates
     */
    void setParameter(Category category, const std::string& name, double value);

    /**
     * Get parameter value by category and name
     * Returns default value if not set
     */
    double getParameter(Category category, const std::string& name, double defaultValue = 0.0) const;
    
    /**
     * Set multiple parameters at once for atomic updates
     * Only triggers callbacks after all parameters are set
     */
    void setParameters(const std::map<std::pair<Category, std::string>, double>& parameters);

    /**
     * Validate parameter dependencies and constraints
     * Auto-adjusts dependent parameters if needed
     */
    void validateAndAdjustParameters(Category category = Category::BASIC_MESH);

    // === Mesh Parameters Integration ===
    
    /**
     * Get current MeshParameters object with all current settings
     * Used for actual mesh generation
     */
    MeshParameters getCurrentMeshParameters() const;

    /**
     * Update MeshParameters from current parameter state
     */
    void syncToMeshParameters(MeshParameters& params) const;

    // === Geometry Application ===
    
    /**
     * Apply current parameters to a single geometry
     * Updates both Coin3D representation and EdgeComponent
     */
    void applyToGeometry(std::shared_ptr<OCCGeometry> geometry);

    /**
     * Apply current parameters to multiple geometries
     * Uses batch processing for efficiency
     */
    void applyToGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);

    /**
     * Force regeneration of all geometries with current parameters
     * Should be called after significant parameter changes
     */
    void regenerateAllGeometries(std::shared_ptr<OCCViewer> viewer);

    // === Callback Management ===
    
    /**
     * Register callback for parameter changes
     * Returns callback ID for removal
     */
    int registerParameterChangeCallback(ParameterChangeCallback callback);

    /**
     * Remove parameter change callback
     */
    void unregisterParameterChangeCallback(int callbackId);

    // === Preset Management ===
    
    /**
     * Apply a mesh quality preset
     * Preset includes all necessary parameters for a specific quality level
     */
    void applyPreset(const std::string& presetName);
    
    /**
     * Save current parameters as a named preset
     */
    void savePreset(const std::string& presetName);
    
    /**
     * Get list of available presets
     */
    std::vector<std::string> getAvailablePresets() const;

    // === Configuration ===
    
    /**
     * Load parameters from configuration file
     */
    void loadFromConfig();
    
    /**
     * Save parameters to configuration file
     */
    void saveToConfig();

    // === Debug and Validation ===
    
    /**
     * Validate current parameter state
     * Returns true if all parameters are consistent
     */
    bool validateCurrentParameters() const;
    
    /**
     * Get parameter validation report
     */
    std::string getParameterReport() const;

private:
    MeshParameterManager();
    ~MeshParameterManager();

    // Parameter storage organized by category
    std::unordered_map<Category, std::unordered_map<std::string, double>> m_parameters;
    
    // Special parameter values (non-double)
    struct SpecialParameters {
        bool subdivisionEnabled;
        bool smoothingEnabled;
        bool lodEnabled;
        bool parallelProcessing;
        bool adaptiveMeshing;
    } m_specialParams;

    // Callback management
    std::unordered_map<int, ParameterChangeCallback> m_callbacks;
    int m_nextCallbackId;

    // Internal helper methods
    void initializeDefaultParameters();
    void updateParameterDependencies(Category category, const std::string& name, double value);
    void notifyParameterChange(Category category, const std::string& name, double oldValue, double newValue);
    void loadPresets();
    void savePresets();

    // Parameter validation rules
    bool validateParameter(Category category, const std::string& name, double value) const;
    std::string getParameterDisplayName(Category category, const std::string& name) const;
};

// Convenience macros for parameter access
#define MESH_PARAM_VALUE(cat, name) MeshParameterManager::getInstance().getParameter(MeshParameterManager::Category::cat, #name)
#define MESH_PARAM_SET(cat, name, val) MeshParameterManager::getInstance().setParameter(MeshParameterManager::Category::cat, #name, val)


#endif // MESH_PARAMETER_MANAGER_H
