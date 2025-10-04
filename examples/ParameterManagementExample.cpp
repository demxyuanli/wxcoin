#include "param/UnifiedParameterManager.h"
#include "OCCGeometry.h"
#include "config/RenderingConfig.h"
#include <iostream>
#include <memory>

/**
 * @brief Example demonstrating the unified parameter management system
 */
class ParameterManagementExample {
public:
    void runExample() {
        std::cout << "=== Unified Parameter Management System Example ===" << std::endl;
        
        // Initialize the parameter management system
        initializeSystem();
        
        // Demonstrate parameter operations
        demonstrateParameterOperations();
        
        // Demonstrate geometry integration
        demonstrateGeometryIntegration();
        
        // Demonstrate rendering config integration
        demonstrateRenderingConfigIntegration();
        
        // Demonstrate batch operations
        demonstrateBatchOperations();
        
        // Demonstrate performance optimization
        demonstratePerformanceOptimization();
        
        std::cout << "=== Example completed successfully ===" << std::endl;
    }

private:
    void initializeSystem() {
        std::cout << "\n1. Initializing Parameter Management System..." << std::endl;
        
        auto& manager = UnifiedParameterManager::getInstance();
        manager.initialize();
        
        std::cout << "   - Parameter tree initialized" << std::endl;
        std::cout << "   - Update manager initialized" << std::endl;
        std::cout << "   - Synchronizer initialized" << std::endl;
        std::cout << "   - System integration setup complete" << std::endl;
    }
    
    void demonstrateParameterOperations() {
        std::cout << "\n2. Demonstrating Parameter Operations..." << std::endl;
        
        auto& manager = UnifiedParameterManager::getInstance();
        
        // Set geometry parameters
        manager.setParameter("geometry/transform/position/x", 10.0);
        manager.setParameter("geometry/transform/position/y", 20.0);
        manager.setParameter("geometry/transform/position/z", 30.0);
        
        // Set material parameters
        Quantity_Color red(1.0, 0.0, 0.0, Quantity_TOC_RGB);
        manager.setParameter("material/color/diffuse", red);
        manager.setParameter("material/properties/transparency", 0.5);
        
        // Set rendering parameters
        manager.setParameter("rendering/mode/display_mode", 
            static_cast<int>(RenderingConfig::DisplayMode::Solid));
        manager.setParameter("quality/level/rendering_quality", 
            static_cast<int>(RenderingConfig::RenderingQuality::High));
        
        // Get parameter values
        double x = manager.getParameter("geometry/transform/position/x").getValueAs<double>();
        auto color = manager.getParameter("material/color/diffuse").getValueAs<Quantity_Color>();
        
        std::cout << "   - Set geometry position: (" << x << ", 20.0, 30.0)" << std::endl;
        std::cout << "   - Set material color: Red" << std::endl;
        std::cout << "   - Set transparency: 0.5" << std::endl;
        std::cout << "   - Set display mode: Solid" << std::endl;
        std::cout << "   - Set rendering quality: High" << std::endl;
    }
    
    void demonstrateGeometryIntegration() {
        std::cout << "\n3. Demonstrating Geometry Integration..." << std::endl;
        
        auto& manager = UnifiedParameterManager::getInstance();
        
        // Create a geometry object
        auto geometry = std::make_shared<OCCBox>("TestBox", 10.0, 10.0, 10.0);
        
        // Register geometry with the parameter system
        manager.registerGeometry(geometry);
        
        // Modify parameters that affect the geometry
        manager.setParameter("geometry/color/main", 
            Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB));
        manager.setParameter("geometry/transparency", 0.3);
        manager.setParameter("material/properties/shininess", 50.0);
        
        std::cout << "   - Created geometry: TestBox" << std::endl;
        std::cout << "   - Registered geometry with parameter system" << std::endl;
        std::cout << "   - Applied green color and transparency" << std::endl;
        std::cout << "   - Set material shininess to 50.0" << std::endl;
        
        // Unregister geometry
        manager.unregisterGeometry(geometry);
        std::cout << "   - Unregistered geometry from parameter system" << std::endl;
    }
    
    void demonstrateRenderingConfigIntegration() {
        std::cout << "\n4. Demonstrating Rendering Config Integration..." << std::endl;
        
        auto& manager = UnifiedParameterManager::getInstance();
        
        // Get rendering config instance
        auto& config = RenderingConfig::getInstance();
        
        // Register config with the parameter system
        manager.registerRenderingConfig(&config);
        
        // Modify rendering parameters
        manager.setParameter("lighting/ambient/intensity", 0.9);
        manager.setParameter("lighting/diffuse/intensity", 1.2);
        manager.setParameter("shadow/mode/shadow_mode", 
            static_cast<int>(RenderingConfig::ShadowMode::Soft));
        manager.setParameter("quality/antialiasing/samples", 8);
        
        std::cout << "   - Registered rendering config with parameter system" << std::endl;
        std::cout << "   - Increased ambient light intensity to 0.9" << std::endl;
        std::cout << "   - Increased diffuse light intensity to 1.2" << std::endl;
        std::cout << "   - Set shadow mode to Soft" << std::endl;
        std::cout << "   - Increased anti-aliasing samples to 8" << std::endl;
        
        // Unregister config
        manager.unregisterRenderingConfig(&config);
        std::cout << "   - Unregistered rendering config from parameter system" << std::endl;
    }
    
    void demonstrateBatchOperations() {
        std::cout << "\n5. Demonstrating Batch Operations..." << std::endl;
        
        auto& manager = UnifiedParameterManager::getInstance();
        
        // Begin batch operation
        manager.beginBatchOperation();
        
        // Set multiple parameters
        manager.setParameter("geometry/transform/position/x", 100.0);
        manager.setParameter("geometry/transform/position/y", 200.0);
        manager.setParameter("geometry/transform/position/z", 300.0);
        manager.setParameter("geometry/transform/scale", 2.0);
        manager.setParameter("material/color/diffuse", 
            Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB));
        manager.setParameter("material/properties/transparency", 0.8);
        
        // End batch operation (triggers optimized updates)
        manager.endBatchOperation();
        
        auto changedParams = manager.getChangedParameters();
        
        std::cout << "   - Began batch operation" << std::endl;
        std::cout << "   - Set multiple parameters in batch" << std::endl;
        std::cout << "   - Ended batch operation with " << changedParams.size() << " changed parameters" << std::endl;
        std::cout << "   - Batch updates were optimized and merged" << std::endl;
    }
    
    void demonstratePerformanceOptimization() {
        std::cout << "\n6. Demonstrating Performance Optimization..." << std::endl;
        
        auto& manager = UnifiedParameterManager::getInstance();
        
        // Enable optimization
        manager.enableOptimization(true);
        manager.setUpdateFrequencyLimit(30); // Limit to 30 updates per second
        manager.enableDebugMode(true);
        
        std::cout << "   - Enabled performance optimization" << std::endl;
        std::cout << "   - Set update frequency limit to 30 updates/second" << std::endl;
        std::cout << "   - Enabled debug mode" << std::endl;
        
        // Simulate rapid parameter changes
        for (int i = 0; i < 10; ++i) {
            manager.setParameter("geometry/transform/position/x", static_cast<double>(i));
            manager.setParameter("geometry/transform/position/y", static_cast<double>(i * 2));
        }
        
        std::cout << "   - Simulated rapid parameter changes" << std::endl;
        std::cout << "   - Updates were throttled and optimized" << std::endl;
        
        // Disable debug mode
        manager.enableDebugMode(false);
        std::cout << "   - Disabled debug mode" << std::endl;
    }
};

/**
 * @brief Main function demonstrating the parameter management system
 */
int main() {
    try {
        ParameterManagementExample example;
        example.runExample();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}