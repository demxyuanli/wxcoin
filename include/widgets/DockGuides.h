#ifndef DOCK_GUIDES_H
#define DOCK_GUIDES_H

#include <wx/window.h>
#include <wx/graphics.h>
#include <wx/timer.h>
#include <memory>
#include "widgets/DockTypes.h"
#include "widgets/UnifiedDockTypes.h"
#include "widgets/IDockManager.h"

class ModernDockManager;
class ModernDockPanel;

// Individual dock guide button
class DockGuideButton {
public:
    DockGuideButton(DockPosition position, const wxRect& rect);
    
    void Render(wxGraphicsContext* gc, bool highlighted, double dpiScale);
    bool HitTest(const wxPoint& pos) const;
    
    DockPosition GetPosition() const { return m_position; }
    wxRect GetRect() const { return m_rect; }
    void SetRect(const wxRect& rect) { m_rect = rect; }

private:
    void DrawGuideIcon(wxGraphicsContext* gc, bool highlighted, double dpiScale);
    void DrawLeftArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size);
    void DrawRightArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size);
    void DrawUpArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size);
    void DrawDownArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size);
    void DrawCenterIcon(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size);
    
    DockPosition m_position;
    wxRect m_rect;
};

// Central dock guides (5-way directional)
class CentralDockGuides : public wxWindow {
public:
    CentralDockGuides(wxWindow* parent, IDockManager* manager);
    
    void ShowAt(const wxPoint& screenPos);
    void Hide();
    void UpdateHighlight(const wxPoint& mousePos);
    DockPosition GetHighlightedPosition() const { return m_highlightedPosition; }
    void SetEnabledDirections(bool center, bool left, bool right, bool top, bool bottom) {
        m_enableCenter = center;
        m_enableLeft = left;
        m_enableRight = right;
        m_enableTop = top;
        m_enableBottom = bottom;
    }
    
protected:
    void OnPaint(wxPaintEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

private:
    void CreateGuideButtons();
    void RenderBackground(wxGraphicsContext* gc);
    void RenderGuideButtons(wxGraphicsContext* gc);
    
    IDockManager* m_manager;
    std::vector<std::unique_ptr<DockGuideButton>> m_buttons;
    DockPosition m_highlightedPosition;
    wxSize m_guideSize;
    bool m_enableCenter{true};
    bool m_enableLeft{true};
    bool m_enableRight{true};
    bool m_enableTop{true};
    bool m_enableBottom{true};
    
    static constexpr int GUIDE_SIZE = 120;
    static constexpr int BUTTON_SIZE = 24;
    static constexpr int CENTER_SIZE = 32;

    wxDECLARE_EVENT_TABLE();
};

// Edge dock guides (window borders)
class EdgeDockGuides : public wxWindow {
public:
    EdgeDockGuides(wxWindow* parent, IDockManager* manager);
    
    void ShowForTarget(ModernDockPanel* target);
    void ShowForManager(IDockManager* manager);
    void Hide();
    void UpdateHighlight(const wxPoint& mousePos);
    DockPosition GetHighlightedPosition() const { return m_highlightedPosition; }
    void SetEnabledDirections(bool left, bool right, bool top, bool bottom) {
        m_enableLeft = left; m_enableRight = right; m_enableTop = top; m_enableBottom = bottom;
    }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

private:
    void CreateEdgeButtons(const wxRect& targetRect);
    void RenderEdgeButton(wxGraphicsContext* gc, DockGuideButton* button, bool highlighted);
    
    IDockManager* m_manager;
    ModernDockPanel* m_targetPanel;
    std::vector<std::unique_ptr<DockGuideButton>> m_edgeButtons;
    DockPosition m_highlightedPosition;
    bool m_enableLeft{true};
    bool m_enableRight{true};
    bool m_enableTop{true};
    bool m_enableBottom{true};
    
    static constexpr int EDGE_BUTTON_SIZE = 20;
    static constexpr int EDGE_MARGIN = 30;

    wxDECLARE_EVENT_TABLE();
};

// Main dock guides controller
class DockGuides {
public:
    DockGuides(IDockManager* manager);
    ~DockGuides();
    
    void ShowGuides(ModernDockPanel* target, const wxPoint& mousePos);
    void HideGuides();
    void UpdateGuides(const wxPoint& mousePos);
    
    DockPosition GetActivePosition() const;
    ModernDockPanel* GetCurrentTarget() const { return m_currentTarget; }
    bool IsVisible() const { return m_visible; }
    void SetEnabledDirections(bool center, bool left, bool right, bool top, bool bottom) {
        m_centerEnabled = center; m_leftEnabled = left; m_rightEnabled = right; m_topEnabled = top; m_bottomEnabled = bottom;
        if (m_edgeGuides) {
            m_edgeGuides->SetEnabledDirections(left, right, top, bottom);
        }
    }
    void SetCentralVisible(bool visible) { m_showCentral = visible; }

private:
    IDockManager* m_manager;
    std::unique_ptr<CentralDockGuides> m_centralGuides;
    std::unique_ptr<EdgeDockGuides> m_edgeGuides;
    ModernDockPanel* m_currentTarget;
    bool m_visible;
    bool m_centerEnabled{true};
    bool m_leftEnabled{true};
    bool m_rightEnabled{true};
    bool m_topEnabled{true};
    bool m_bottomEnabled{true};
    bool m_showCentral{true};
};

#endif // DOCK_GUIDES_H
