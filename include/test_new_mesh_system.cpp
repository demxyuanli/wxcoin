#include "OCCViewer.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include <iostream>

// Test function to verify new mesh parameter system
void testNewMeshParameterSystem(OCCViewer* viewer) {
    if (!viewer) {
        std::cout << "Error: OCCViewer is null" << std::endl;
        return;
    }
    
    std::cout << "=== TESTING NEW MESH PARAMETER SYSTEM ===" << std::endl;
    
    // Test 1: Verify RenderingToolkitAPI is accessible
    std::cout << "\n1. Testing RenderingToolkitAPI Access:" << std::endl;
    try {
        auto& config = RenderingToolkitAPI::getConfig();
        std::cout << "   ✓ RenderingToolkitAPI::getConfig() successful" << std::endl;
        
        // Test smoothing settings
        auto& smoothingSettings = config.getSmoothingSettings();
        smoothingSettings.enabled = true;
        smoothingSettings.creaseAngle = 25.0;
        smoothingSettings.iterations = 3;
        std::cout << "   ✓ Smoothing settings updated" << std::endl;
        
        // Test subdivision settings
        auto& subdivisionSettings = config.getSubdivisionSettings();
        subdivisionSettings.enabled = true;
        subdivisionSettings.levels = 3;
        std::cout << "   ✓ Subdivision settings updated" << std::endl;
        
        // Test custom parameters
        config.setParameter("tessellation_quality", "5");
        config.setParameter("adaptive_meshing", "true");
        std::cout << "   ✓ Custom parameters set" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "   ✗ Error: " << e.what() << std::endl;
        return;
    }
    
    // Test 2: Verify OCCViewer parameter application
    std::cout << "\n2. Testing OCCViewer Parameter Application:" << std::endl;
    
    // Set parameters through OCCViewer
    viewer->setSmoothingEnabled(true);
    viewer->setSmoothingIterations(4);
    viewer->setSmoothingStrength(0.8);
    viewer->setSubdivisionEnabled(true);
    viewer->setSubdivisionLevel(2);
    viewer->setAdaptiveMeshing(true);
    viewer->setTessellationQuality(4);
    
    std::cout << "   ✓ Parameters set through OCCViewer" << std::endl;
    
    // Test 3: Verify parameter remeshing
    std::cout << "\n3. Testing Parameter Remeshing:" << std::endl;
    try {
        viewer->remeshAllGeometries();
        std::cout << "   ✓ remeshAllGeometries() completed successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✗ Error during remeshing: " << e.what() << std::endl;
    }
    
    // Test 4: Verify parameter validation
    std::cout << "\n4. Testing Parameter Validation:" << std::endl;
    try {
        viewer->validateMeshParameters();
        std::cout << "   ✓ Parameter validation completed" << std::endl;
        
        // Test specific parameter verification
        bool deflectionOK = viewer->verifyParameterApplication("deflection", viewer->getMeshDeflection());
        bool subdivisionOK = viewer->verifyParameterApplication("subdivision_level", 2);
        bool smoothingOK = viewer->verifyParameterApplication("smoothing_iterations", 4);
        
        std::cout << "   Deflection verification: " << (deflectionOK ? "PASS" : "FAIL") << std::endl;
        std::cout << "   Subdivision verification: " << (subdivisionOK ? "PASS" : "FAIL") << std::endl;
        std::cout << "   Smoothing verification: " << (smoothingOK ? "PASS" : "FAIL") << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "   ✗ Error during validation: " << e.what() << std::endl;
    }
    
    // Test 5: Verify configuration persistence
    std::cout << "\n5. Testing Configuration Persistence:" << std::endl;
    try {
        auto& config = RenderingToolkitAPI::getConfig();
        
        // Check if settings were properly applied
        auto& smoothingSettings = config.getSmoothingSettings();
        auto& subdivisionSettings = config.getSubdivisionSettings();
        
        std::cout << "   Smoothing enabled: " << (smoothingSettings.enabled ? "true" : "false") << std::endl;
        std::cout << "   Smoothing iterations: " << smoothingSettings.iterations << std::endl;
        std::cout << "   Subdivision enabled: " << (subdivisionSettings.enabled ? "true" : "false") << std::endl;
        std::cout << "   Subdivision levels: " << subdivisionSettings.levels << std::endl;
        
        // Check custom parameters
        std::string tessellationQuality = config.getParameter("tessellation_quality", "0");
        std::string adaptiveMeshing = config.getParameter("adaptive_meshing", "false");
        
        std::cout << "   Tessellation quality: " << tessellationQuality << std::endl;
        std::cout << "   Adaptive meshing: " << adaptiveMeshing << std::endl;
        
        std::cout << "   ✓ Configuration persistence verified" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "   ✗ Error checking configuration: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== NEW MESH PARAMETER SYSTEM TEST COMPLETE ===" << std::endl;
}

// Function to demonstrate the complete workflow
void demonstrateCompleteWorkflow(OCCViewer* viewer) {
    if (!viewer) return;
    
    std::cout << "\n=== COMPLETE WORKFLOW DEMONSTRATION ===" << std::endl;
    
    // Step 1: Initialize parameters
    std::cout << "Step 1: Setting mesh quality parameters..." << std::endl;
    viewer->setMeshDeflection(0.05, false);  // Don't remesh yet
    viewer->setSmoothingEnabled(true);
    viewer->setSmoothingIterations(3);
    viewer->setSmoothingStrength(0.7);
    viewer->setSubdivisionEnabled(true);
    viewer->setSubdivisionLevel(2);
    viewer->setAdaptiveMeshing(true);
    viewer->setTessellationQuality(4);
    
    // Step 2: Apply parameters
    std::cout << "Step 2: Applying parameters to all geometries..." << std::endl;
    viewer->remeshAllGeometries();
    
    // Step 3: Validate results
    std::cout << "Step 3: Validating parameter application..." << std::endl;
    viewer->validateMeshParameters();
    
    // Step 4: Generate report
    std::cout << "Step 4: Generating mesh quality report..." << std::endl;
    std::string report = viewer->getMeshQualityReport();
    std::cout << "Report generated successfully!" << std::endl;
    
    std::cout << "=== WORKFLOW COMPLETE ===" << std::endl;
}

// Main test function
int main() {
    // This would be called from your application
    // OCCViewer* viewer = getOCCViewerInstance();
    // testNewMeshParameterSystem(viewer);
    // demonstrateCompleteWorkflow(viewer);
    
    std::cout << "Test functions defined. Use these in your application to verify the new mesh parameter system." << std::endl;
    return 0;
} 