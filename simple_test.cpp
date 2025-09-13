#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/bitmap.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <vector>
#include <memory>
#include <functional>
#include <map>

// Simplified FlatTreeItem for testing
class FlatTreeItem
{
public:
    enum class ItemType {
        ROOT,
        FOLDER,
        FILE,
        SKETCH,
        BODY,
        PAD,
        ORIGIN,
        REFERENCE
    };

    FlatTreeItem(const wxString& text, ItemType type = ItemType::FOLDER)
        : m_text(text), m_type(type), m_visible(true), m_selected(false), m_expanded(false), m_parent(nullptr)
    {
    }

    void SetText(const wxString& text) { m_text = text; }
    wxString GetText() const { return m_text; }
    void SetType(ItemType type) { m_type = type; }
    ItemType GetType() const { return m_type; }
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    void SetSelected(bool selected) { m_selected = selected; }
    bool IsSelected() const { return m_selected; }
    void SetExpanded(bool expanded) { m_expanded = expanded; }
    bool IsExpanded() const { return m_expanded; }

    void AddChild(std::shared_ptr<FlatTreeItem> child) {
        if (child) {
            child->SetParent(this);
            m_children.push_back(child);
        }
    }

    std::vector<std::shared_ptr<FlatTreeItem>>& GetChildren() { return m_children; }
    const std::vector<std::shared_ptr<FlatTreeItem>>& GetChildren() const { return m_children; }
    void SetParent(FlatTreeItem* parent) { m_parent = parent; }
    FlatTreeItem* GetParent() const { return m_parent; }
    bool HasChildren() const { return !m_children.empty(); }

private:
    wxString m_text;
    ItemType m_type;
    bool m_visible;
    bool m_selected;
    bool m_expanded;
    FlatTreeItem* m_parent;
    std::vector<std::shared_ptr<FlatTreeItem>> m_children;
};

// Simplified FlatTreeView for testing
class FlatTreeView : public wxScrolledWindow
{
public:
    FlatTreeView(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0)
        : wxScrolledWindow(parent, id, pos, size, style | wxBORDER_NONE)
        , m_itemHeight(22)
        , m_scrollY(0)
        , m_totalHeight(0)
        , m_needsLayout(true)
    {
        SetBackgroundColour(wxColour(255, 255, 255));
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetDoubleBuffered(true);
        SetScrollRate(0, m_itemHeight);
    }

    void SetRoot(std::shared_ptr<FlatTreeItem> root) {
        m_root = root;
        m_needsLayout = true;
        Refresh();
    }

    void OnPaint(wxPaintEvent& event) {
        wxUnusedVar(event);
        wxAutoBufferedPaintDC dc(this);

        if (m_needsLayout) {
            CalculateLayout();
        }

        DrawBackground(dc);
        DrawHeader(dc);
        DrawItems(dc);
    }

    void OnSize(wxSizeEvent& event) {
        m_needsLayout = true;
        Refresh();
        event.Skip();
    }

    void OnScroll(wxScrollWinEvent& event) {
        int orient = event.GetOrientation();
        if (orient == wxVERTICAL) {
            int scrollPos = GetScrollPos(wxVERTICAL);
            m_scrollY = scrollPos;
            Refresh();
        }
        event.Skip();
    }

private:
    void DrawBackground(wxDC& dc) {
        dc.SetBackground(wxBrush(wxColour(255, 255, 255)));
        dc.Clear();
    }

    void DrawHeader(wxDC& dc) {
        dc.SetPen(wxPen(wxColour(200, 200, 200)));
        dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
        dc.DrawRectangle(0, 0, GetClientSize().GetWidth(), m_itemHeight);
        
        // Draw header text
        dc.SetTextForeground(wxColour(0, 0, 0));
        dc.SetFont(GetFont());
        dc.DrawText("Tree View Header", 5, 2);
        
        // Draw bottom line
        dc.DrawLine(0, m_itemHeight, GetClientSize().GetWidth(), m_itemHeight);
    }

    void DrawItems(wxDC& dc) {
        if (!m_root) return;

        wxSize cs = GetClientSize();
        int headerY = m_itemHeight + 1;

        // Set clipping region for content area only
        dc.SetClippingRegion(0, headerY, cs.GetWidth(), cs.GetHeight() - headerY);

        // Calculate drawing start position
        // Content should start from headerY position, then offset by scroll
        int startY = headerY - m_scrollY;

        DrawItemRecursive(dc, m_root, startY, 0);

        dc.DestroyClippingRegion();
    }

    void DrawItemRecursive(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int& y, int level) {
        if (!item || !item->IsVisible()) return;

        DrawItem(dc, item, y, level);
        y += m_itemHeight;

        if (item->IsExpanded()) {
            for (auto& child : item->GetChildren()) {
                DrawItemRecursive(dc, child, y, level + 1);
            }
        }
    }

    void DrawItem(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int y, int level) {
        if (!item) return;

        // Check if item is visible in current view
        // y is now in screen coordinates (already includes header offset)
        wxSize cs = GetClientSize();
        int headerY = m_itemHeight + 1;

        // y is already in screen coordinates, check visibility directly
        if (y + m_itemHeight < headerY || y > cs.GetHeight()) {
            return; // Item is not visible
        }

        // Draw selection background
        if (item->IsSelected()) {
            dc.SetBrush(wxBrush(wxColour(0, 120, 215)));
            dc.SetPen(wxPen(wxColour(0, 120, 215)));
            dc.DrawRectangle(0, y, GetClientSize().GetWidth(), m_itemHeight);
        }

        // Draw item text
        dc.SetTextForeground(item->IsSelected() ? wxColour(255, 255, 255) : wxColour(0, 0, 0));
        dc.SetFont(GetFont());

        wxString text = item->GetText();
        int textX = 5 + level * 16; // Simple indentation
        int textY = y + (m_itemHeight - dc.GetTextExtent(text).GetHeight()) / 2;
        dc.DrawText(text, textX, textY);
    }

    void CalculateLayout() {
        if (!m_root) {
            m_totalHeight = 0;
            SetScrollbar(wxVERTICAL, 0, 0, 0, true);
            return;
        }

        m_totalHeight = CalculateItemHeightRecursive(m_root);
        int clientH = GetClientSize().GetHeight();
        int headerY = m_itemHeight + 1;
        int visibleHeight = clientH - headerY;

        if (m_totalHeight > visibleHeight) {
            int range = m_totalHeight - visibleHeight;
            int thumb = visibleHeight;
            int pos = std::min(m_scrollY, range);
            SetScrollbar(wxVERTICAL, pos, thumb, m_totalHeight, true);
        } else {
            SetScrollbar(wxVERTICAL, 0, 0, 0, true);
            m_scrollY = 0;
            SetScrollPos(wxVERTICAL, 0);
        }

        m_needsLayout = false;
    }

    int CalculateItemHeightRecursive(std::shared_ptr<FlatTreeItem> item) {
        if (!item || !item->IsVisible()) return 0;

        int height = m_itemHeight;

        if (item->IsExpanded()) {
            for (auto& child : item->GetChildren()) {
                height += CalculateItemHeightRecursive(child);
            }
        }

        return height;
    }

    std::shared_ptr<FlatTreeItem> m_root;
    int m_itemHeight;
    int m_scrollY;
    int m_totalHeight;
    bool m_needsLayout;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(FlatTreeView, wxScrolledWindow)
EVT_PAINT(FlatTreeView::OnPaint)
EVT_SIZE(FlatTreeView::OnSize)
EVT_SCROLLWIN(FlatTreeView::OnScroll)
END_EVENT_TABLE()

class TestFrame : public wxFrame
{
public:
    TestFrame() : wxFrame(nullptr, wxID_ANY, "FlatTreeView Scroll Test", wxDefaultPosition, wxSize(800, 600))
    {
        // Create FlatTreeView
        m_treeView = new FlatTreeView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        
        // Add some test data
        auto root = std::make_shared<FlatTreeItem>("Root", FlatTreeItem::ItemType::ROOT);
        m_treeView->SetRoot(root);
        
        // Add many child items to test scrolling
        for (int i = 0; i < 50; ++i) {
            auto child = std::make_shared<FlatTreeItem>(wxString::Format("[STEP]ATU010%d...", i), FlatTreeItem::ItemType::FILE);
            root->AddChild(child);
        }
        
        // Expand root to show children
        root->SetExpanded(true);
        
        // Set up layout
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_treeView, 1, wxEXPAND | wxALL, 5);
        SetSizer(sizer);
    }
    
private:
    FlatTreeView* m_treeView;
};

class TestApp : public wxApp
{
public:
    bool OnInit() override
    {
        TestFrame* frame = new TestFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);