#pragma once

#include <Inventor/SbLinear.h>
#include <wx/timer.h>
#include <wx/event.h>
#include <functional>
#include <memory>

class SoCamera;

class CameraAnimation : public wxEvtHandler {
public:
    enum AnimationType {
        LINEAR,           // Linear interpolation
        SMOOTH,           // Smooth ease-in/out
        EASE_IN,          // Fast start, slow end
        EASE_OUT,         // Slow start, fast end
        BOUNCE            // Bouncy effect
    };

    struct CameraState {
        SbVec3f position;
        SbRotation rotation;
        float focalDistance;
        float height;        // For orthographic cameras

        CameraState() : focalDistance(5.0f), height(10.0f) {}
        CameraState(const SbVec3f& pos, const SbRotation& rot, float focalDist = 5.0f, float h = 10.0f)
            : position(pos), rotation(rot), focalDistance(focalDist), height(h) {}
    };

    CameraAnimation();
    virtual ~CameraAnimation();

    // Animation control
    bool startAnimation(const CameraState& startState, const CameraState& endState,
                       float durationSeconds = 1.0f, AnimationType type = SMOOTH);
    void stopAnimation();
    bool isAnimating() const { return m_isAnimating; }

    // Progress and callbacks
    void setProgressCallback(std::function<void(float)> callback) { m_progressCallback = callback; }
    void setCompletionCallback(std::function<void()> callback) { m_completionCallback = callback; }
    void setViewRefreshCallback(std::function<void()> callback) { m_viewRefreshCallback = callback; }

    // Camera update
    void setCamera(SoCamera* camera) { m_camera = camera; }
    void updateCamera();

    // Animation parameters
    void setAnimationType(AnimationType type) { m_animationType = type; }
    AnimationType getAnimationType() const { return m_animationType; }

private:
    void onTimer(wxTimerEvent& event);
    float calculateEasing(float t) const;
    CameraState interpolateStates(const CameraState& start, const CameraState& end, float t) const;

    wxTimer m_timer;
    CameraState m_startState;
    CameraState m_endState;
    CameraState m_currentState;

    SoCamera* m_camera;
    AnimationType m_animationType;

    float m_duration;
    float m_elapsedTime;
    bool m_isAnimating;

    std::function<void(float)> m_progressCallback;
    std::function<void()> m_completionCallback;
    std::function<void()> m_viewRefreshCallback;

    wxDECLARE_EVENT_TABLE();
};

//==============================================================================
// NavigationAnimator - High-level animation manager
//==============================================================================

class NavigationAnimator {
public:
    static NavigationAnimator& getInstance();

    // Quick animations
    void animateToPosition(const SbVec3f& targetPosition, const SbRotation& targetRotation,
                          float duration = 1.0f);
    void animateToBookmark(const wxString& bookmarkName, float duration = 1.5f);

    // Animation control
    void stopCurrentAnimation();
    bool isAnimating() const;

    // Camera setup
    void setCamera(SoCamera* camera);

    // View refresh callback
    void setViewRefreshCallback(std::function<void()> callback) { m_viewRefreshCallback = callback; }

    // Animation settings
    void setDefaultDuration(float seconds) { m_defaultDuration = seconds; }
    void setAnimationType(CameraAnimation::AnimationType type);

private:
    NavigationAnimator();
    ~NavigationAnimator() = default;

    std::unique_ptr<CameraAnimation> m_currentAnimation;
    SoCamera* m_camera;
    float m_defaultDuration;
    std::function<void()> m_viewRefreshCallback;

    void onAnimationCompleted();
};
