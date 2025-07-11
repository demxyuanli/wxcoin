#ifndef FLATUIGALLERY_H
#define FLATUIGALLERY_H

#include <wx/wx.h>
#include <wx/vector.h>

// Forward declaration
class FlatUIPanel;

class FlatUIGallery : public wxControl
{
public:
    // Gallery Item Style
    enum class ItemStyle {
        DEFAULT,        // Default style with no border
        BORDERED,       // Items with border
        ROUNDED,        // Rounded corners
        SHADOWED,       // Shadow effect
        HIGHLIGHTED     // Highlighted appearance
    };
    
    // Gallery Item Border Style
    enum class ItemBorderStyle {
        SOLID,          // Solid line border
        DASHED,         // Dashed line border
        DOTTED,         // Dotted line border
        DOUBLE,         // Double line border
        ROUNDED         // Rounded corners
    };
    
    // Gallery Layout Style
    enum class LayoutStyle {
        HORIZONTAL,     // Items arranged horizontally (default)
        GRID,           // Items in grid layout
        FLOW            // Flow layout with wrapping
    };

    FlatUIGallery(FlatUIPanel* parent);
    virtual ~FlatUIGallery();

    void AddItem(const wxBitmap& bitmap, int id);
    size_t GetItemCount() const { return m_items.size(); }
    
    // Style configuration methods
    void SetItemStyle(ItemStyle style);
    ItemStyle GetItemStyle() const { return m_itemStyle; }
    
    void SetItemBorderStyle(ItemBorderStyle style);
    ItemBorderStyle GetItemBorderStyle() const { return m_itemBorderStyle; }
    
    void SetLayoutStyle(LayoutStyle style);
    LayoutStyle GetLayoutStyle() const { return m_layoutStyle; }
    
    // Item appearance configuration
    void SetItemBackgroundColour(const wxColour& colour);
    wxColour GetItemBackgroundColour() const { return m_itemBgColour; }
    
    void SetItemHoverBackgroundColour(const wxColour& colour);
    wxColour GetItemHoverBackgroundColour() const { return m_itemHoverBgColour; }
    
    void SetItemSelectedBackgroundColour(const wxColour& colour);
    wxColour GetItemSelectedBackgroundColour() const { return m_itemSelectedBgColour; }
    
    void SetItemBorderColour(const wxColour& colour);
    wxColour GetItemBorderColour() const { return m_itemBorderColour; }
    
    void SetItemBorderWidth(int width);
    int GetItemBorderWidth() const { return m_itemBorderWidth; }
    
    void SetItemCornerRadius(int radius);
    int GetItemCornerRadius() const { return m_itemCornerRadius; }
    
    // Item spacing and padding
    void SetItemSpacing(int spacing);
    int GetItemSpacing() const { return m_itemSpacing; }
    
    void SetItemPadding(int padding);
    int GetItemPadding() const { return m_itemPadding; }
    
    // Gallery background
    void SetGalleryBackgroundColour(const wxColour& colour);
    wxColour GetGalleryBackgroundColour() const { return m_galleryBgColour; }
    
    // Gallery border configuration
    void SetGalleryBorderColour(const wxColour& colour);
    wxColour GetGalleryBorderColour() const { return m_galleryBorderColour; }
    
    void SetGalleryBorderWidth(int width);
    int GetGalleryBorderWidth() const { return m_galleryBorderWidth; }
    
    // Selection management
    void SetSelectedItem(int index);
    int GetSelectedItem() const { return m_selectedItem; }
    
    // Enable/disable hover effects
    void SetHoverEffectsEnabled(bool enabled);
    bool GetHoverEffectsEnabled() const { return m_hoverEffectsEnabled; }
    
    // Enable/disable selection
    void SetSelectionEnabled(bool enabled);
    bool GetSelectionEnabled() const { return m_selectionEnabled; }

    void OnMouseDown(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnPaint(wxPaintEvent& evt);

protected:
    wxSize DoGetBestSize() const override;

private:
    struct ItemInfo
    {
        wxBitmap bitmap;
        int id;
        wxRect rect;
        bool hovered = false;
        bool selected = false;
    };
    wxVector<ItemInfo> m_items;
    
    // Style members
    ItemStyle m_itemStyle;
    ItemBorderStyle m_itemBorderStyle;
    LayoutStyle m_layoutStyle;
    wxColour m_itemBgColour;
    wxColour m_itemHoverBgColour;
    wxColour m_itemSelectedBgColour;
    wxColour m_itemBorderColour;
    wxColour m_galleryBgColour;
    wxColour m_galleryBorderColour;
    int m_itemBorderWidth;
    int m_itemCornerRadius;
    int m_itemSpacing;
    int m_itemPadding;
    int m_galleryBorderWidth;
    int m_selectedItem;
    int m_hoveredItem;
    int m_dropdownWidth;
    bool m_hoverEffectsEnabled;
    bool m_selectionEnabled;
    bool m_hasDropdown;
    
    void RecalculateLayout();
    void DrawItem(wxDC& dc, const ItemInfo& item, int index);
    void DrawItemBackground(wxDC& dc, const wxRect& rect, bool isHovered, bool isSelected);
    void DrawItemBorder(wxDC& dc, const wxRect& rect, bool isHovered, bool isSelected);
};

#endif // FLATUIGALLERY_H 