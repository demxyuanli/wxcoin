#pragma once

#include "ILayoutStrategy.h"
#include "LayoutEngine.h"
#include "ModernDockPanel.h"
#include "LayoutStrategyFactory.h"
#include <memory>
#include <chrono>

// Test suite for layout strategies
class LayoutStrategyTest {
public:
    LayoutStrategyTest();
    ~LayoutStrategyTest();

    // Run all tests
    bool RunAllTests();
    
    // Individual test methods
    bool TestStrategyCreation();
    bool TestIDEStrategy();
    bool TestFlexibleStrategy();
    bool TestStrategySwitching();
    bool TestLayoutPersistence();
    bool TestPerformance();
    bool TestErrorHandling();
    
    // Utility methods
    void PrintTestResults();
    std::unique_ptr<ModernDockPanel> CreateTestPanel(const std::string& name);

private:
    std::unique_ptr<LayoutNode> m_testRoot;
    LayoutStrategyFactory* m_factory;
};


