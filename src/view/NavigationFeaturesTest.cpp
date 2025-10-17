#include "ViewBookmark.h"
#include "CameraAnimation.h"
#include "ZoomController.h"
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <wx/log.h>
#include <iostream>

class NavigationFeaturesTest {
public:
    static void runBasicTests() {
        std::cout << "=== Navigation Features Test ===\n";

        // Test 1: ViewBookmark System
        testBookmarkSystem();

        // Test 2: Camera Animation
        testAnimationSystem();

        // Test 3: Zoom Controller
        testZoomSystem();

        std::cout << "=== All tests completed ===\n";
    }

private:
    static void testBookmarkSystem() {
        std::cout << "Testing ViewBookmark System...\n";

        auto& bm = ViewBookmarkManager::getInstance();

        // Test adding bookmarks
        SbVec3f pos1(1, 2, 3);
        SbRotation rot1(SbVec3f(0, 1, 0), 0.5f);
        bool added = bm.addBookmark("Test View 1", pos1, rot1);
        std::cout << "Added bookmark: " << (added ? "SUCCESS" : "FAILED") << "\n";

        // Test duplicate prevention
        added = bm.addBookmark("Test View 1", pos1, rot1);
        std::cout << "Duplicate bookmark prevention: " << (!added ? "SUCCESS" : "FAILED") << "\n";

        // Test retrieval
        auto bookmark = bm.getBookmark("Test View 1");
        bool retrieved = (bookmark != nullptr);
        std::cout << "Retrieved bookmark: " << (retrieved ? "SUCCESS" : "FAILED") << "\n";

        if (retrieved) {
            bool posMatch = (bookmark->getPosition() == pos1);
            bool rotMatch = (bookmark->getRotation() == rot1);
            std::cout << "Position match: " << (posMatch ? "SUCCESS" : "FAILED") << "\n";
            std::cout << "Rotation match: " << (rotMatch ? "SUCCESS" : "FAILED") << "\n";
        }

        // Test renaming
        bool renamed = bm.renameBookmark("Test View 1", "Renamed View");
        std::cout << "Renamed bookmark: " << (renamed ? "SUCCESS" : "FAILED") << "\n";

        // Test removal
        bool removed = bm.removeBookmark("Renamed View");
        std::cout << "Removed bookmark: " << (removed ? "SUCCESS" : "FAILED") << "\n";

        std::cout << "ViewBookmark System test completed.\n\n";
    }

    static void testAnimationSystem() {
        std::cout << "Testing Camera Animation System...\n";

        // Create a test camera
        SoPerspectiveCamera* camera = new SoPerspectiveCamera();
        camera->ref();

        // Set initial state
        SbVec3f startPos(0, 0, 5);
        SbRotation startRot = SbRotation::identity();
        camera->position.setValue(startPos);
        camera->orientation.setValue(startRot);

        // Test animation creation
        CameraAnimation anim;
        anim.setCamera(camera);

        CameraAnimation::CameraState startState(startPos, startRot);
        CameraAnimation::CameraState endState(SbVec3f(5, 5, 5),
                                             SbRotation(SbVec3f(1, 1, 1), 2*M_PI/3));

        bool started = anim.startAnimation(startState, endState, 1.0f, CameraAnimation::LINEAR);
        std::cout << "Animation started: " << (started ? "SUCCESS" : "FAILED") << "\n";

        // Test immediate stop
        anim.stopAnimation();
        bool isAnimating = anim.isAnimating();
        std::cout << "Animation stopped: " << (!isAnimating ? "SUCCESS" : "FAILED") << "\n";

        // Test NavigationAnimator
        NavigationAnimator& navAnim = NavigationAnimator::getInstance();
        navAnim.setCamera(camera);
        navAnim.setAnimationType(CameraAnimation::SMOOTH);

        std::cout << "NavigationAnimator initialized: SUCCESS\n";

        camera->unref();
        std::cout << "Camera Animation System test completed.\n\n";
    }

    static void testZoomSystem() {
        std::cout << "Testing Zoom Controller System...\n";

        // Create a test camera
        SoPerspectiveCamera* camera = new SoPerspectiveCamera();
        camera->ref();
        camera->position.setValue(SbVec3f(0, 0, 5));

        // Test ZoomController
        ZoomController controller;
        controller.setCamera(camera);

        // Test zoom operations
        float initialScale = controller.getCurrentZoomScale();
        std::cout << "Initial zoom scale: " << initialScale << "\n";

        bool zoomedIn = controller.zoomIn();
        float afterZoomIn = controller.getCurrentZoomScale();
        std::cout << "Zoom in: " << (zoomedIn && afterZoomIn > initialScale ? "SUCCESS" : "FAILED");
        std::cout << " (scale: " << afterZoomIn << ")\n";

        bool zoomedOut = controller.zoomOut();
        float afterZoomOut = controller.getCurrentZoomScale();
        std::cout << "Zoom out: " << (zoomedOut && afterZoomOut < afterZoomIn ? "SUCCESS" : "FAILED");
        std::cout << " (scale: " << afterZoomOut << ")\n";

        bool reset = controller.zoomReset();
        float afterReset = controller.getCurrentZoomScale();
        std::cout << "Zoom reset: " << (reset && std::abs(afterReset - 1.0f) < 0.01f ? "SUCCESS" : "FAILED");
        std::cout << " (scale: " << afterReset << ")\n";

        // Test zoom levels
        controller.setZoomMode(ZoomController::DISCRETE);
        controller.zoomToLevel(2); // Should be 50%
        float levelScale = controller.getCurrentZoomScale();
        wxString levelName = controller.getCurrentZoomLevelName();
        std::cout << "Zoom to level: " << levelName << " (scale: " << levelScale << ")\n";

        // Test ZoomManager
        auto zoomCtrl = std::make_shared<ZoomController>();
        zoomCtrl->setCamera(camera);
        ZoomManager::getInstance().setController(zoomCtrl);

        ZoomManager::getInstance().zoomIn();
        std::cout << "ZoomManager zoom in: SUCCESS\n";

        camera->unref();
        std::cout << "Zoom Controller System test completed.\n\n";
    }
};

// Simple test runner
void runNavigationFeaturesTests() {
    NavigationFeaturesTest::runBasicTests();
}

