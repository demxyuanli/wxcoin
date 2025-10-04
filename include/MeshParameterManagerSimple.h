#ifndef MESH_PARAMETER_MANAGER_SIMPLE_H
#define MESH_PARAMETER_MANAGER_SIMPLE_H

#include <string>
#include <functional>
#include <map>
#include <memory>

class OCCGeometry;
class OCCViewer;

/**
 * Simplified MeshParameterManager that works incrementally with existing code
 * Provides basic parameter centralization without breaking existing functionality
 */
class MeshParameterManagerSimple {
public:
    // Singleton instance
    static MeshParameterManagerSimple& getInstance();

    /**
     * Application methods that forward to existing MeshQualityDialog
     * These methods maintain backward compatibility while using centralized parameters
     */
    static void applyPreset(std::shared_ptr<OCCViewer> viewer, 
                            double deflection, bool lodEnabled,
                            double roughDeflection, double fineDeflection,
                            bool parallelProcessing);

    static void applySurfacePreset(std::shared_ptr<OCCViewer> viewer,
                                   double deflection, double angularDeflection,
                                   bool subdivisionEnabled, int subdivisionLevel,
                                   bool smoothingEnabled, int smoothingIterations, double smoothingStrength,
                                   bool lodEnabled, double lodFineDeflection, double lodRoughDeflection,
                                   int tessellationQuality, double featurePreservation, double smoothingCreaseAngle);

    // Basic parameter setters/getters for incremental integration
    void setDeflection(double value) { m_deflection = value; }
    double getDeflection() const { return m_deflection; }
    
    void setAngularDeflection(double value) { m_angularDeflection = value; }
    double getAngularDeflection() const { return m_angularDeflection; }
    
    void setLODEnabled(bool enabled) { m_lodEnabled = enabled; }
    bool isLODEnabled() const { return m_lodEnabled; }
    
    void setLODRoughDeflection(double value) { m_lodRoughDeflection = value; }
    double getLODRoughDeflection() const { return m_lodRoughDeflection; }
    
    void setLODFineDeflection(double value) { m_lodFineDeflection = value; }
    double getLODFineDeflection() const { return m_lodFineDeflection; }

    // Initialize from current viewer state (for seamless integration)
    void initializeFromViewer(std::shared_ptr<OCCViewer> viewer);
    
    // Sync viewer with current parameters
    void syncToViewer(std::shared_ptr<OCCViewer> viewer);
    
    // Reset to default values (useful for clearing state)
    void resetToDefaults();
    
    // Force update all parameters (useful for ensuring state consistency)
    void forceUpdateAll(std::shared_ptr<OCCViewer> viewer);

private:
    MeshParameterManagerSimple();
    ~MeshParameterManagerSimple() = default;

    // Current parameter values
    double m_deflection;
    double m_angularDeflection;
    bool m_lodEnabled;
    double m_lodRoughDeflection;
    double m_lodFineDeflection;
    bool m_hasInitialized;
};

#endif // MESH_PARAMETER_MANAGER_SIMPLE_H

