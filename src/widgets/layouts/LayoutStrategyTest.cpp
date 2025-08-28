#include "widgets/layouts/LayoutStrategyTest.h"
#include "widgets/layouts/LayoutStrategyFactory.h"
#include "widgets/layouts/IDELayoutStrategy.h"
#include "widgets/layouts/FlexibleLayoutStrategy.h"
#include "widgets/LayoutEngine.h"
#include "widgets/ModernDockPanel.h"
#include <wx/window.h>
#include <iostream>
#include <cassert>

LayoutStrategyTest::LayoutStrategyTest() {
	// Initialize test environment
	m_testRoot = std::make_unique<LayoutNode>(LayoutNodeType::Root, nullptr);
	m_factory = &LayoutStrategyFactory::Instance();
}

LayoutStrategyTest::~LayoutStrategyTest() = default;

bool LayoutStrategyTest::RunAllTests() {
	std::cout << "=== Layout Strategy Test Suite ===" << std::endl;

	bool allPassed = true;

	// Test 1: Strategy Creation
	allPassed &= TestStrategyCreation();

	// Test 2: IDE Strategy Functionality
	allPassed &= TestIDEStrategy();

	// Test 3: Flexible Strategy Functionality
	allPassed &= TestFlexibleStrategy();

	// Test 4: Strategy Switching
	allPassed &= TestStrategySwitching();

	// Test 5: Layout Persistence
	allPassed &= TestLayoutPersistence();

	// Test 6: Performance Tests
	allPassed &= TestPerformance();

	// Test 7: Error Handling
	allPassed &= TestErrorHandling();

	std::cout << "=== Test Results: " << (allPassed ? "ALL PASSED" : "SOME FAILED") << " ===" << std::endl;
	return allPassed;
}

bool LayoutStrategyTest::TestStrategyCreation() {
	std::cout << "Testing Strategy Creation..." << std::endl;

	try {
		// Test IDE strategy creation
		auto ideStrategy = m_factory->CreateStrategy(LayoutStrategy::IDE);
		if (!ideStrategy) {
			std::cout << "FAILED: Could not create IDE strategy" << std::endl;
			return false;
		}

		// Test Flexible strategy creation
		auto flexibleStrategy = m_factory->CreateStrategy(LayoutStrategy::Flexible);
		if (!flexibleStrategy) {
			std::cout << "FAILED: Could not create Flexible strategy" << std::endl;
			return false;
		}

		// Test strategy availability
		if (!m_factory->IsStrategyAvailable(LayoutStrategy::IDE)) {
			std::cout << "FAILED: IDE strategy not available" << std::endl;
			return false;
		}

		if (!m_factory->IsStrategyAvailable(LayoutStrategy::Flexible)) {
			std::cout << "FAILED: Flexible strategy not available" << std::endl;
			return false;
		}

		// Test strategy names
		std::string ideName = m_factory->GetStrategyName(LayoutStrategy::IDE);
		std::string flexibleName = m_factory->GetStrategyName(LayoutStrategy::Flexible);

		if (ideName != "IDE") {
			std::cout << "FAILED: IDE strategy name mismatch: " << ideName << std::endl;
			return false;
		}

		if (flexibleName != "Flexible") {
			std::cout << "FAILED: Flexible strategy name mismatch: " << flexibleName << std::endl;
			return false;
		}

		std::cout << "PASSED: Strategy Creation" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cout << "FAILED: Exception in strategy creation: " << e.what() << std::endl;
		return false;
	}
}

bool LayoutStrategyTest::TestIDEStrategy() {
	std::cout << "Testing IDE Strategy..." << std::endl;

	try {
		auto strategy = m_factory->CreateStrategy(LayoutStrategy::IDE);
		if (!strategy) return false;

		// Reset test root
		m_testRoot->GetChildren().clear();

		// Test layout creation
		strategy->CreateLayout(m_testRoot.get());

		// Verify IDE structure was created
		if (m_testRoot->GetChildren().empty()) {
			std::cout << "FAILED: IDE strategy did not create layout structure" << std::endl;
			return false;
		}

		// Test panel addition
		auto testPanel = CreateTestPanel("TestPanel");
		strategy->AddPanel(m_testRoot.get(), testPanel.get(), UnifiedDockArea::Left);

		// Verify panel was added
		if (!strategy->ValidateLayout(m_testRoot.get())) {
			std::cout << "FAILED: IDE strategy layout validation failed after adding panel" << std::endl;
			return false;
		}

		// Test layout calculation
		wxRect testRect(0, 0, 1200, 800);
		strategy->CalculateLayout(m_testRoot.get(), testRect);

		// Test serialization
		std::string serialized = strategy->SerializeLayout(m_testRoot.get());
		if (serialized.empty()) {
			std::cout << "FAILED: IDE strategy serialization failed" << std::endl;
			return false;
		}

		// Test deserialization
		auto newRoot = std::make_unique<LayoutNode>(LayoutNodeType::Root);
		if (!strategy->DeserializeLayout(newRoot.get(), serialized)) {
			std::cout << "FAILED: IDE strategy deserialization failed" << std::endl;
			return false;
		}

		std::cout << "PASSED: IDE Strategy" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cout << "FAILED: Exception in IDE strategy test: " << e.what() << std::endl;
		return false;
	}
}

bool LayoutStrategyTest::TestFlexibleStrategy() {
	std::cout << "Testing Flexible Strategy..." << std::endl;

	try {
		auto strategy = m_factory->CreateStrategy(LayoutStrategy::Flexible);
		if (!strategy) return false;

		// Reset test root
		m_testRoot->GetChildren().clear();

		// Test layout creation
		strategy->CreateLayout(m_testRoot.get());

		// Verify flexible structure was created
		if (m_testRoot->GetChildren().empty()) {
			std::cout << "FAILED: Flexible strategy did not create layout structure" << std::endl;
			return false;
		}

		// Test multiple panel additions
		auto panel1 = CreateTestPanel("Panel1");
		auto panel2 = CreateTestPanel("Panel2");
		auto panel3 = CreateTestPanel("Panel3");

		strategy->AddPanel(m_testRoot.get(), panel1.get(), UnifiedDockArea::Left);
		strategy->AddPanel(m_testRoot.get(), panel2.get(), UnifiedDockArea::Center);
		strategy->AddPanel(m_testRoot.get(), panel3.get(), UnifiedDockArea::Right);

		// Verify all panels were added
		if (!strategy->ValidateLayout(m_testRoot.get())) {
			std::cout << "FAILED: Flexible strategy layout validation failed after adding panels" << std::endl;
			return false;
		}

		// Test panel movement
		strategy->MovePanel(m_testRoot.get(), panel1.get(), UnifiedDockArea::Bottom);

		// Test panel swapping
		strategy->SwapPanels(m_testRoot.get(), panel2.get(), panel3.get());

		// Test layout optimization
		strategy->OptimizeLayout(m_testRoot.get());

		// Test layout calculation
		wxRect testRect(0, 0, 1200, 800);
		strategy->CalculateLayout(m_testRoot.get(), testRect);

		// Test serialization
		std::string serialized = strategy->SerializeLayout(m_testRoot.get());
		if (serialized.empty()) {
			std::cout << "FAILED: Flexible strategy serialization failed" << std::endl;
			return false;
		}

		// Test deserialization
		auto newRoot = std::make_unique<LayoutNode>(LayoutNodeType::Root);
		if (!strategy->DeserializeLayout(newRoot.get(), serialized)) {
			std::cout << "FAILED: Flexible strategy deserialization failed" << std::endl;
			return false;
		}

		std::cout << "PASSED: Flexible Strategy" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cout << "FAILED: Exception in Flexible strategy test: " << e.what() << std::endl;
		return false;
	}
}

bool LayoutStrategyTest::TestStrategySwitching() {
	std::cout << "Testing Strategy Switching..." << std::endl;

	try {
		// Create a layout with IDE strategy
		auto ideStrategy = m_factory->CreateStrategy(LayoutStrategy::IDE);
		if (!ideStrategy) return false;

		// Reset test root
		m_testRoot->GetChildren().clear();

		// Add panels with IDE strategy
		auto panel1 = CreateTestPanel("IDE_Panel1");
		auto panel2 = CreateTestPanel("IDE_Panel2");

		ideStrategy->CreateLayout(m_testRoot.get());
		ideStrategy->AddPanel(m_testRoot.get(), panel1.get(), UnifiedDockArea::Left);
		ideStrategy->AddPanel(m_testRoot.get(), panel2.get(), UnifiedDockArea::Center);

		// Serialize IDE layout
		std::string ideLayout = ideStrategy->SerializeLayout(m_testRoot.get());

		// Switch to Flexible strategy
		auto flexibleStrategy = m_factory->CreateStrategy(LayoutStrategy::Flexible);
		if (!flexibleStrategy) return false;

		// Try to deserialize IDE layout with Flexible strategy
		// This should fail gracefully
		auto newRoot = std::make_unique<LayoutNode>(LayoutNodeType::Root);
		bool deserialized = flexibleStrategy->DeserializeLayout(newRoot.get(), ideLayout);

		// Create new layout with Flexible strategy
		flexibleStrategy->CreateLayout(newRoot.get());
		flexibleStrategy->AddPanel(newRoot.get(), panel1.get(), UnifiedDockArea::Left);
		flexibleStrategy->AddPanel(newRoot.get(), panel2.get(), UnifiedDockArea::Center);

		// Verify both strategies work independently
		if (!ideStrategy->ValidateLayout(m_testRoot.get())) {
			std::cout << "FAILED: IDE strategy validation failed after switching" << std::endl;
			return false;
		}

		if (!flexibleStrategy->ValidateLayout(newRoot.get())) {
			std::cout << "FAILED: Flexible strategy validation failed after switching" << std::endl;
			return false;
		}

		std::cout << "PASSED: Strategy Switching" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cout << "FAILED: Exception in strategy switching test: " << e.what() << std::endl;
		return false;
	}
}

bool LayoutStrategyTest::TestLayoutPersistence() {
	std::cout << "Testing Layout Persistence..." << std::endl;

	try {
		// Test IDE strategy persistence
		auto ideStrategy = m_factory->CreateStrategy(LayoutStrategy::IDE);
		if (!ideStrategy) return false;

		m_testRoot->GetChildren().clear();
		ideStrategy->CreateLayout(m_testRoot.get());

		auto panel = CreateTestPanel("PersistPanel");
		ideStrategy->AddPanel(m_testRoot.get(), panel.get(), UnifiedDockArea::Left);

		std::string ideSerialized = ideStrategy->SerializeLayout(m_testRoot.get());

		// Test Flexible strategy persistence
		auto flexibleStrategy = m_factory->CreateStrategy(LayoutStrategy::Flexible);
		if (!flexibleStrategy) return false;

		auto flexibleRoot = std::make_unique<LayoutNode>(LayoutNodeType::Root);
		flexibleStrategy->CreateLayout(flexibleRoot.get());

		auto panel2 = CreateTestPanel("PersistPanel2");
		flexibleStrategy->AddPanel(flexibleRoot.get(), panel2.get(), UnifiedDockArea::Center);

		std::string flexibleSerialized = flexibleStrategy->SerializeLayout(flexibleRoot.get());

		// Verify serialized data is different (different strategies)
		if (ideSerialized == flexibleSerialized) {
			std::cout << "FAILED: Different strategies produced identical serialization" << std::endl;
			return false;
		}

		// Test round-trip serialization
		auto newRoot = std::make_unique<LayoutNode>(LayoutNodeType::Root);
		if (!ideStrategy->DeserializeLayout(newRoot.get(), ideSerialized)) {
			std::cout << "FAILED: IDE strategy round-trip serialization failed" << std::endl;
			return false;
		}

		// Verify the restored layout is valid
		if (!ideStrategy->ValidateLayout(newRoot.get())) {
			std::cout << "FAILED: Restored IDE layout is invalid" << std::endl;
			return false;
		}

		std::cout << "PASSED: Layout Persistence" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cout << "FAILED: Exception in layout persistence test: " << e.what() << std::endl;
		return false;
	}
}

bool LayoutStrategyTest::TestPerformance() {
	std::cout << "Testing Performance..." << std::endl;

	try {
		// Test with large number of panels
		const int panelCount = 50;

		// Test IDE strategy performance
		auto ideStrategy = m_factory->CreateStrategy(LayoutStrategy::IDE);
		if (!ideStrategy) return false;

		m_testRoot->GetChildren().clear();
		ideStrategy->CreateLayout(m_testRoot.get());

		auto startTime = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < panelCount; ++i) {
			auto panel = CreateTestPanel("PerfPanel" + std::to_string(i));
			ideStrategy->AddPanel(m_testRoot.get(), panel.get(), UnifiedDockArea::Center);
		}

		auto endTime = std::chrono::high_resolution_clock::now();
		auto ideDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

		// Test Flexible strategy performance
		auto flexibleStrategy = m_factory->CreateStrategy(LayoutStrategy::Flexible);
		if (!flexibleStrategy) return false;

		auto flexibleRoot = std::make_unique<LayoutNode>(LayoutNodeType::Root);
		flexibleStrategy->CreateLayout(flexibleRoot.get());

		startTime = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < panelCount; ++i) {
			auto panel = CreateTestPanel("PerfPanel" + std::to_string(i));
			flexibleStrategy->AddPanel(flexibleRoot.get(), panel.get(), UnifiedDockArea::Center);
		}

		endTime = std::chrono::high_resolution_clock::now();
		auto flexibleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

		std::cout << "Performance Results:" << std::endl;
		std::cout << "  IDE Strategy: " << ideDuration.count() << "ms for " << panelCount << " panels" << std::endl;
		std::cout << "  Flexible Strategy: " << flexibleDuration.count() << "ms for " << panelCount << " panels" << std::endl;

		// Both should complete within reasonable time (less than 1 second)
		if (ideDuration.count() > 1000 || flexibleDuration.count() > 1000) {
			std::cout << "FAILED: Performance test took too long" << std::endl;
			return false;
		}

		std::cout << "PASSED: Performance Test" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cout << "FAILED: Exception in performance test: " << e.what() << std::endl;
		return false;
	}
}

bool LayoutStrategyTest::TestErrorHandling() {
	std::cout << "Testing Error Handling..." << std::endl;

	try {
		auto strategy = m_factory->CreateStrategy(LayoutStrategy::IDE);
		if (!strategy) return false;

		// Test with null pointers
		strategy->CreateLayout(nullptr);
		strategy->AddPanel(nullptr, nullptr, UnifiedDockArea::Center);
		strategy->RemovePanel(nullptr, nullptr);

		// Test with invalid data
		if (strategy->DeserializeLayout(m_testRoot.get(), "InvalidData")) {
			std::cout << "FAILED: Strategy should reject invalid data" << std::endl;
			return false;
		}

		// Test error state
		if (strategy->HasErrors()) {
			std::cout << "FAILED: Strategy should not have errors after null operations" << std::endl;
			return false;
		}

		// Test error clearing
		strategy->ClearLastError();
		if (strategy->HasErrors()) {
			std::cout << "FAILED: Errors should be cleared" << std::endl;
			return false;
		}

		std::cout << "PASSED: Error Handling" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cout << "FAILED: Exception in error handling test: " << e.what() << std::endl;
		return false;
	}
}

std::unique_ptr<ModernDockPanel> LayoutStrategyTest::CreateTestPanel(const std::string& name) {
	// Create a mock panel for testing
	auto panel = std::make_unique<ModernDockPanel>(nullptr, nullptr, wxString(name));
	// Note: SetTitle is called on the pointer, not the unique_ptr
	(*panel).SetTitle(wxString(name));
	return panel;
}

void LayoutStrategyTest::PrintTestResults() {
	std::cout << "\n=== Detailed Test Results ===" << std::endl;

	// Test strategy creation
	std::cout << "Strategy Creation: ";
	if (TestStrategyCreation()) {
		std::cout << "PASSED" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}

	// Test IDE strategy
	std::cout << "IDE Strategy: ";
	if (TestIDEStrategy()) {
		std::cout << "PASSED" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}

	// Test Flexible strategy
	std::cout << "Flexible Strategy: ";
	if (TestFlexibleStrategy()) {
		std::cout << "PASSED" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}

	// Test strategy switching
	std::cout << "Strategy Switching: ";
	if (TestStrategySwitching()) {
		std::cout << "PASSED" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}

	// Test layout persistence
	std::cout << "Layout Persistence: ";
	if (TestLayoutPersistence()) {
		std::cout << "PASSED" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}

	// Test performance
	std::cout << "Performance: ";
	if (TestPerformance()) {
		std::cout << "PASSED" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}

	// Test error handling
	std::cout << "Error Handling: ";
	if (TestErrorHandling()) {
		std::cout << "PASSED" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}
}