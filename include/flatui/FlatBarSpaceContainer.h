#ifndef FLATBARSPACE_CONTAINER_H
#define FLATBARSPACE_CONTAINER_H

#include <wx/wx.h>
#include <memory>

// Forward declarations
class FlatUIHomeSpace;
class FlatUISystemButtons;
class FlatUIFunctionSpace;
class FlatUIProfileSpace;
class FlatUITabDropdown;

class FlatBarSpaceContainer : public wxControl
{
public:
    FlatBarSpaceContainer(wxWindow* parent, wxWindowID id = wxID_ANY, 
                         const wxPoint& pos = wxDefaultPosition, 
                         const wxSize& size = wxDefaultSize, 
                         long style = 0);
    virtual ~FlatBarSpaceContainer();

    // Component management
    void SetHomeSpace(FlatUIHomeSpace* homeSpace);
    void SetSystemButtons(FlatUISystemButtons* systemButtons);
    void SetFunctionSpace(FlatUIFunctionSpace* functionSpace);
    void SetProfileSpace(FlatUIProfileSpace* profileSpace);
    
    // Tab area management
    void SetTabAreaRect(const wxRect& rect);
    wxRect GetTabAreaRect() const { return m_tabAreaRect; }
    
    // Layout configuration
    void SetFunctionSpaceCenterAlign(bool center) { m_functionSpaceCenterAlign = center; }
    bool GetFunctionSpaceCenterAlign() const { return m_functionSpaceCenterAlign; }
    
    void SetProfileSpaceRightAlign(bool rightAlign) { m_profileSpaceRightAlign = rightAlign; }
    bool GetProfileSpaceRightAlign() const { return m_profileSpaceRightAlign; }

    // Component access
    FlatUIHomeSpace* GetHomeSpace() const { return m_homeSpace; }
    FlatUISystemButtons* GetSystemButtons() const { return m_systemButtons; }
    FlatUIFunctionSpace* GetFunctionSpace() const { return m_functionSpace; }
    FlatUIProfileSpace* GetProfileSpace() const { return m_profileSpace; }
    FlatUITabDropdown* GetTabDropdown() const { return m_tabDropdown; }

    // Override for best size calculation
    virtual wxSize DoGetBestSize() const override;
    
    // Public layout update method
    void UpdateLayout();

protected:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);

private:
    // Layout management
    void PositionComponents();
    
    // Drawing
    void DrawBackground(wxDC& dc);
    void PaintTabs(wxDC& dc);
    
    // Drag functionality
    void StartDrag(const wxPoint& startPos);
    void UpdateDrag(const wxPoint& currentPos);
    void EndDrag();
    bool IsInDragArea(const wxPoint& pos) const;
    
    // Tab interaction
    bool HandleTabClick(const wxPoint& pos);
    
    // Tab overflow management
    void UpdateTabOverflow();
    std::vector<size_t> GetVisibleTabIndices() const;
    std::vector<size_t> GetHiddenTabIndices() const;
    
    // Helper methods
    int CalculateTabWidth(wxDC& dc, const wxString& label) const;
    
    // Component pointers
    FlatUIHomeSpace* m_homeSpace;
    FlatUISystemButtons* m_systemButtons;
    FlatUIFunctionSpace* m_functionSpace;
    FlatUIProfileSpace* m_profileSpace;
    FlatUITabDropdown* m_tabDropdown;
    
    // Layout data
    wxRect m_tabAreaRect;
    bool m_functionSpaceCenterAlign;
    bool m_profileSpaceRightAlign;
    
    // Drag state
    bool m_isDragging;
    wxPoint m_dragStartPos;
    
    // Tab overflow state
    std::vector<size_t> m_visibleTabIndices;
    std::vector<size_t> m_hiddenTabIndices;
    bool m_hasTabOverflow;
    
    // Layout constants
    static constexpr int ELEMENT_SPACING = 5;
    static constexpr int BAR_PADDING = 2;
    static constexpr int MIN_DRAG_DISTANCE = 3;
    static constexpr int DROPDOWN_BUTTON_WIDTH = 20;

    wxDECLARE_EVENT_TABLE();
};

#endif // FLATBARSPACE_CONTAINER_H 