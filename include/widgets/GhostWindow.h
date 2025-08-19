#ifndef GHOST_WINDOW_H
#define GHOST_WINDOW_H

#include <wx/frame.h>
#include <wx/graphics.h>
#include <wx/timer.h>
#include <wx/bitmap.h>

class ModernDockPanel;

// Ghost window for drag preview
class GhostWindow : public wxFrame {
public:
    GhostWindow();
    ~GhostWindow() override;
    
    // Show ghost window for dragged panel
    void ShowGhost(ModernDockPanel* panel, const wxPoint& startPos);
    void HideGhost();
    void UpdatePosition(const wxPoint& pos);
    
    // Visual effects
    void SetTransparency(int alpha); // 0-255
    void SetContent(const wxBitmap& bitmap);
    void SetTitle(const wxString& title);
    
    // Animation support
    void StartFadeIn();
    void StartFadeOut();
    
    bool IsVisible() const { return m_visible; }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnClose(wxCloseEvent& event);

private:
    void CreateGhostContent(ModernDockPanel* panel);
    void RenderGhostFrame(wxGraphicsContext* gc);
    void UpdateTransparency();
    void DrawDragIndicator(wxGraphicsContext* gc);
    
    // Content
    wxBitmap m_contentBitmap;
    wxString m_title;
    ModernDockPanel* m_sourcePanel;
    
    // Visual state
    bool m_visible;
    int m_transparency;
    int m_targetTransparency;
    bool m_animating;
    
    // Animation
    wxTimer m_animationTimer;
    int m_animationStep;
    int m_totalAnimationSteps;
    
    // Layout
    wxSize m_minSize;
    wxSize m_maxSize;
    int m_titleBarHeight;
    int m_borderWidth;
    
    // Constants
    static constexpr int DEFAULT_TRANSPARENCY = 180;
    static constexpr int FADE_STEPS = 8;
    static constexpr int FADE_TIMER_INTERVAL = 20;
    static constexpr int MIN_GHOST_WIDTH = 200;
    static constexpr int MIN_GHOST_HEIGHT = 150;
    static constexpr int MAX_GHOST_WIDTH = 400;
    static constexpr int MAX_GHOST_HEIGHT = 300;
    static constexpr int TITLE_BAR_HEIGHT = 28;
    static constexpr int BORDER_WIDTH = 1;

    wxDECLARE_EVENT_TABLE();
};

#endif // GHOST_WINDOW_H
