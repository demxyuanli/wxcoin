#ifndef FLATUIPAGE_MANAGER_H
#define FLATUIPAGE_MANAGER_H

#include <wx/wx.h>
#include <vector>

// Forward declarations
class FlatUIPage;
class FlatUIFixPanel;
class FlatUIFloatPanel;

class FlatUIPageManager {
public:
    FlatUIPageManager();
    ~FlatUIPageManager() = default;
    
    // Page collection management
    void AddPage(FlatUIPage* page);
    void RemovePage(size_t index);
    void RemovePage(FlatUIPage* page);
    void ClearPages();
    
    // Page access
    FlatUIPage* GetPage(size_t index) const;
    size_t GetPageCount() const { return m_pages.size(); }
    bool HasPages() const { return !m_pages.empty(); }
    
    // Page search
    size_t FindPageIndex(FlatUIPage* page) const;
    bool ContainsPage(FlatUIPage* page) const;
    
    // Page state management
    void SetPageActive(size_t index, bool active);
    void SetAllPagesInactive();
    FlatUIPage* GetActivePage() const;
    
    // Container management
    void ShowPageInFixPanel(FlatUIPage* page, FlatUIFixPanel* fixPanel);
    void ShowPageInFloatPanel(FlatUIPage* page, FlatUIFloatPanel* floatPanel);
    void ReturnPageToOriginalParent(FlatUIPage* page, wxWindow* originalParent);
    void ReturnAllPagesToOriginalParent(wxWindow* originalParent);
    
    // Page transitions
    void TransferPageBetweenContainers(FlatUIPage* page, wxWindow* fromContainer, wxWindow* toContainer);
    void SafeReparentPage(FlatUIPage* page, wxWindow* newParent);
    
    // Validation
    bool IsValidPageIndex(size_t index) const;
    bool ValidatePagePointer(FlatUIPage* page) const;
    
    // Utilities
    void HideAllPages();
    void ShowOnlyPage(FlatUIPage* page);
    wxString GetPageListString() const;
    
private:
    std::vector<FlatUIPage*> m_pages;
    
    // Helper methods
    void NotifyPageAdded(FlatUIPage* page);
    void NotifyPageRemoved(FlatUIPage* page);
    void ValidatePageCollection() const;
    
    // Container operations
    void PreparePageForContainer(FlatUIPage* page, wxWindow* container);
    void CleanupPageFromContainer(FlatUIPage* page, wxWindow* container);
    
    // Safety checks
    bool CanAddPage(FlatUIPage* page) const;
    bool CanRemovePage(FlatUIPage* page) const;
    
    static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);
};

#endif // FLATUIPAGE_MANAGER_H 