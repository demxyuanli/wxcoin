#include "CameraAnimation.h"
#include "ViewBookmark.h"
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <wx/log.h>
#include <cmath>

//==============================================================================
// CameraAnimation Implementation
//==============================================================================

wxBEGIN_EVENT_TABLE(CameraAnimation, wxEvtHandler)
    EVT_TIMER(wxID_ANY, CameraAnimation::onTimer)
wxEND_EVENT_TABLE()

CameraAnimation::CameraAnimation()
    : m_camera(nullptr)
    , m_animationType(SMOOTH)
    , m_orbitCenter(0.0f, 0.0f, 0.0f)
    , m_alignOrientationToOrbit(false)
    , m_duration(1.0f)
    , m_elapsedTime(0.0f)
    , m_isAnimating(false)
{
    m_timer.SetOwner(this);
}

CameraAnimation::~CameraAnimation() {
    stopAnimation();
}

bool CameraAnimation::startAnimation(const CameraState& startState, const CameraState& endState,
                                   float durationSeconds, AnimationType type) {
    if (m_isAnimating) {
        stopAnimation();
    }

    m_startState = startState;
    m_endState = endState;

    if (m_alignOrientationToOrbit) {
        alignStateWithOrbitCenter(m_startState);
        alignStateWithOrbitCenter(m_endState);
    }

    m_currentState = m_startState;

    m_duration = durationSeconds;
    m_animationType = type;
    m_elapsedTime = 0.0f;
    m_isAnimating = true;

    // Start timer (60 FPS = ~16ms intervals)
    m_timer.Start(16, false);

    wxLogDebug("CameraAnimation: Started animation (duration: %.2fs, type: %d)",
               m_duration, static_cast<int>(m_animationType));

    return true;
}

void CameraAnimation::stopAnimation() {
    if (m_isAnimating) {
        m_timer.Stop();
        m_isAnimating = false;

        wxLogDebug("CameraAnimation: Animation stopped");
    }
}

void CameraAnimation::onTimer(wxTimerEvent& event) {
    if (!m_isAnimating) return;

    m_elapsedTime += 0.016f; // ~16ms per frame
    float progress = std::min(m_elapsedTime / m_duration, 1.0f);

    // Apply easing function
    float easedProgress = calculateEasing(progress);

    // Interpolate camera states
    m_currentState = interpolateStates(m_startState, m_endState, easedProgress);

    // Update camera
    updateCamera();

    // Call progress callback
    if (m_progressCallback) {
        m_progressCallback(easedProgress);
    }

    // Check if animation completed
    if (progress >= 1.0f) {
        stopAnimation();

        // Snap to final position to avoid floating point errors
        m_currentState = m_endState;
        updateCamera();

        // Call completion callback
        if (m_completionCallback) {
            m_completionCallback();
        }

        wxLogDebug("CameraAnimation: Animation completed");
    }
}

float CameraAnimation::calculateEasing(float t) const {
    switch (m_animationType) {
        case LINEAR:
            return t;

        case SMOOTH: {
            // Smooth ease-in-out (cubic)
            return t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
        }

        case EASE_IN: {
            // Cubic ease-in
            return t * t * t;
        }

        case EASE_OUT: {
            // Cubic ease-out
            return 1 - std::pow(1 - t, 3);
        }

        case BOUNCE: {
            // Bounce effect
            const float n1 = 7.5625f;
            const float d1 = 2.75f;

            if (t < 1/d1) {
                return n1 * t * t;
            } else if (t < 2/d1) {
                return n1 * (t -= 1.5f/d1) * t + 0.75f;
            } else if (t < 2.5f/d1) {
                return n1 * (t -= 2.25f/d1) * t + 0.9375f;
            } else {
                return n1 * (t -= 2.625f/d1) * t + 0.984375f;
            }
        }

        default:
            return t;
    }
}

void CameraAnimation::alignStateWithOrbitCenter(CameraState& state) const {
    SbVec3f lookDir = m_orbitCenter - state.position;
    if (lookDir.normalize() == 0.0f) {
        return;
    }
    SbVec3f defaultDir(0.0f, 0.0f, -1.0f);
    state.rotation = SbRotation(defaultDir, lookDir);
}

CameraAnimation::CameraState CameraAnimation::interpolateStates(
    const CameraState& start, const CameraState& end, float t) const {

    CameraState result;

    // Interpolate position using spherical path around orbit center when possible
    const float epsilon = 1e-5f;
    SbVec3f startOffset = start.position - m_orbitCenter;
    SbVec3f endOffset = end.position - m_orbitCenter;
    float startRadius = startOffset.length();
    float endRadius = endOffset.length();

    if (startRadius > epsilon && endRadius > epsilon) {
        SbVec3f startDir = startOffset;
        startDir.normalize();
        SbVec3f endDir = endOffset;
        endDir.normalize();

        float dot = startDir.dot(endDir);
        if (dot > 1.0f) {
            dot = 1.0f;
        } else if (dot < -1.0f) {
            dot = -1.0f;
        }

        SbVec3f interpolatedDir;
        if (dot < -0.999f) {
            // Handle nearly opposite vectors by rotating around an orthogonal axis
            SbVec3f axis(0.0f, 0.0f, 1.0f);
            if (std::fabs(startDir.dot(axis)) > 0.9f) {
                axis.setValue(0.0f, 1.0f, 0.0f);
            }
            const float pi = 3.14159265358979323846f;
            SbRotation halfTurn(axis, pi);
            SbRotation identityRotation;
            SbRotation slerpRotation = SbRotation::slerp(identityRotation, halfTurn, t);
            interpolatedDir = startDir;
            slerpRotation.multVec(interpolatedDir, interpolatedDir);
        } else {
            SbRotation rotation(startDir, endDir);
            SbRotation identityRotation;
            SbRotation slerpRotation = SbRotation::slerp(identityRotation, rotation, t);
            interpolatedDir = startDir;
            slerpRotation.multVec(interpolatedDir, interpolatedDir);
        }

        float radius = startRadius + (endRadius - startRadius) * t;
        result.position = m_orbitCenter + interpolatedDir * radius;
    } else {
        result.position = start.position + (end.position - start.position) * t;
    }

    // Interpolate rotation
    if (m_alignOrientationToOrbit) {
        SbVec3f lookDir = m_orbitCenter - result.position;
        if (lookDir.normalize() != 0.0f) {
            SbVec3f defaultDir(0.0f, 0.0f, -1.0f);
            result.rotation = SbRotation(defaultDir, lookDir);
        } else {
            result.rotation = SbRotation::slerp(start.rotation, end.rotation, t);
        }
    } else {
        result.rotation = SbRotation::slerp(start.rotation, end.rotation, t);
    }

    // Interpolate focal distance
    result.focalDistance = start.focalDistance + (end.focalDistance - start.focalDistance) * t;

    // Interpolate height (for orthographic cameras)
    result.height = start.height + (end.height - start.height) * t;

    return result;
}

void CameraAnimation::updateCamera() {
    if (!m_camera) return;

    // Update position and rotation
    m_camera->position.setValue(m_currentState.position);
    m_camera->orientation.setValue(m_currentState.rotation);

    // Update camera-specific properties
    if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        SoPerspectiveCamera* perspCam = static_cast<SoPerspectiveCamera*>(m_camera);
        perspCam->focalDistance.setValue(m_currentState.focalDistance);
    } else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
        SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
        orthoCam->height.setValue(m_currentState.height);
    }

    // Mark camera as modified for Open Inventor
    if (m_camera) {
        m_camera->touch();
    }

    // Trigger view refresh after camera update
    if (m_viewRefreshCallback) {
        m_viewRefreshCallback();
    }
}

//==============================================================================
// NavigationAnimator Implementation
//==============================================================================

NavigationAnimator& NavigationAnimator::getInstance() {
    static NavigationAnimator instance;
    return instance;
}

NavigationAnimator::NavigationAnimator()
    : m_currentAnimation(std::make_unique<CameraAnimation>())
    , m_camera(nullptr)
    , m_defaultDuration(1.0f)
    , m_orbitCenter(0.0f, 0.0f, 0.0f)
    , m_alignOrientationToOrbit(true)
{
    // Set completion callback
    m_currentAnimation->setCompletionCallback([this]() {
        onAnimationCompleted();
    });
    m_currentAnimation->setOrbitCenter(m_orbitCenter);
    m_currentAnimation->setAlignOrientationToOrbit(m_alignOrientationToOrbit);
}

void NavigationAnimator::animateToPosition(const SbVec3f& targetPosition,
                                         const SbRotation& targetRotation,
                                         float duration,
                                         float targetFocalDistance,
                                         float targetHeight,
                                         bool alignOrientationToOrbit) {
    if (!m_camera) {
        wxLogWarning("NavigationAnimator: No camera set for animation");
        return;
    }

    // Get current state
    CameraAnimation::CameraState startState;
    startState.position = m_camera->position.getValue();
    startState.rotation = m_camera->orientation.getValue();

    // Get focal distance/height
    if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        SoPerspectiveCamera* perspCam = static_cast<SoPerspectiveCamera*>(m_camera);
        startState.focalDistance = perspCam->focalDistance.getValue();
    } else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
        SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
        startState.height = orthoCam->height.getValue();
    }

    // Create end state
    CameraAnimation::CameraState endState(targetPosition, targetRotation);

    // Apply target focal distance/height when provided
    if (std::isnan(targetFocalDistance)) {
        endState.focalDistance = startState.focalDistance;
    } else {
        endState.focalDistance = targetFocalDistance;
    }
    if (std::isnan(targetHeight)) {
        endState.height = startState.height;
    } else {
        endState.height = targetHeight;
    }

    // Start animation
    m_currentAnimation->setOrbitCenter(m_orbitCenter);
    m_alignOrientationToOrbit = alignOrientationToOrbit;
    m_currentAnimation->setAlignOrientationToOrbit(m_alignOrientationToOrbit);
    m_currentAnimation->startAnimation(startState, endState, duration);
}

void NavigationAnimator::animateToBookmark(const wxString& bookmarkName, float duration) {
    auto& bookmarkManager = ViewBookmarkManager::getInstance();
    auto bookmark = bookmarkManager.getBookmark(bookmarkName);

    if (!bookmark) {
        wxLogWarning("NavigationAnimator: Bookmark '%s' not found", bookmarkName);
        return;
    }

    animateToPosition(bookmark->getPosition(), bookmark->getRotation(), duration,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        false);
}

void NavigationAnimator::stopCurrentAnimation() {
    if (m_currentAnimation) {
        m_currentAnimation->stopAnimation();
    }
}

bool NavigationAnimator::isAnimating() const {
    return m_currentAnimation && m_currentAnimation->isAnimating();
}

void NavigationAnimator::setCamera(SoCamera* camera) {
    m_camera = camera;
    m_currentAnimation->setCamera(camera);
}

void NavigationAnimator::setAnimationType(CameraAnimation::AnimationType type) {
    if (m_currentAnimation) {
        m_currentAnimation->setAnimationType(type);
    }
}

void NavigationAnimator::setOrbitCenter(const SbVec3f& center) {
    m_orbitCenter = center;
    if (m_currentAnimation) {
        m_currentAnimation->setOrbitCenter(m_orbitCenter);
    }
}

void NavigationAnimator::setViewRefreshCallback(std::function<void()> callback) {
    m_viewRefreshCallback = callback;
    if (m_currentAnimation) {
        m_currentAnimation->setViewRefreshCallback(callback);
    }
}

void NavigationAnimator::onAnimationCompleted() {
    wxLogDebug("NavigationAnimator: Animation completed");
    // Call view refresh callback if set
    if (m_viewRefreshCallback) {
        m_viewRefreshCallback();
    }
}
