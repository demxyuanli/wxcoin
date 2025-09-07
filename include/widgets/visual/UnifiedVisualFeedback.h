#pragma once

#include "../IVisualFeedback.h"
#include "../DockGuides.h"
#include "../GhostWindow.h"
#include <memory>

namespace wxcoin {

// Forward declarations
class IDockGuidesRenderer;
class IDragPreviewRenderer;
class IAreaHighlighter;

// Unified visual feedback manager implementation
// This consolidates all visual feedback components into a single manager
class UnifiedVisualFeedback : public IVisualFeedback {
public:
    UnifiedVisualFeedback();
    virtual ~UnifiedVisualFeedback() = default;

    // Dock guides management
    void ShowDockGuides(const wxPoint& pos, UnifiedDockArea targetArea) override;
    void HideDockGuides() override;
    void UpdateDockGuides(const wxPoint& pos) override;
    bool AreDockGuidesVisible() const override;
    
    // Dock guide configuration
    void SetDockGuideConfig(const DockGuideConfig& config) override;
    DockGuideConfig GetDockGuideConfig() const override;
    void SetEnabledDirections(bool left, bool right, bool top, bool bottom, bool center) override;
    void SetCentralVisible(bool visible) override;
    bool IsDirectionEnabled(DockGuideDirection direction) const override;
    
    // Drag preview management
    void ShowDragPreview(wxWindow* content, const wxRect& rect) override;
    void HideDragPreview() override;
    void UpdateDragPreview(const wxRect& rect) override;
    bool IsDragPreviewVisible() const override;
    void SetDragPreviewOpacity(int opacity) override;
    int GetDragPreviewOpacity() const override;
    
    // Area highlighting
    void HighlightDropArea(UnifiedDockArea area, const wxRect& rect) override;
    void ClearAreaHighlights() override;
    void SetHighlightColor(const wxColour& color) override;
    wxColour GetHighlightColor() const override;
    void SetHighlightStyle(int style) override;
    int GetHighlightStyle() const override;
    
    // Splitter indicators
    void ShowSplitterIndicator(const wxRect& rect, SplitterOrientation orientation) override;
    void HideSplitterIndicator() override;
    void UpdateSplitterIndicator(const wxRect& rect, SplitterOrientation orientation) override;
    bool IsSplitterIndicatorVisible() const override;
    
    // Tab indicators
    void ShowTabIndicator(const wxRect& rect, const wxString& title) override;
    void HideTabIndicator() override;
    void UpdateTabIndicator(const wxRect& rect, const wxString& title) override;
    bool IsTabIndicatorVisible() const override;
    
    // Animation and transitions
    void EnableAnimations(bool enable) override;
    bool AreAnimationsEnabled() const override;
    void SetAnimationDuration(int milliseconds) override;
    int GetAnimationDuration() const override;
    void SetAnimationEasing(int easing) override;
    int GetAnimationEasing() const override;
    
    // Visual effects
    void EnableVisualEffects(bool enable) override;
    bool AreVisualEffectsEnabled() const override;
    void SetBlurEffect(bool enable) override;
    bool IsBlurEffectEnabled() const override;
    void SetShadowEffect(bool enable) override;
    bool IsShadowEffectEnabled() const override;
    
    // Color and theme management
    void SetTheme(const wxString& themeName) override;
    wxString GetCurrentTheme() const override;
    wxArrayString GetAvailableThemes() const override;
    void SetCustomColors(const wxColour& primary, const wxColour& secondary, const wxColour& accent) override;
    void GetCustomColors(wxColour& primary, wxColour& secondary, wxColour& accent) const override;
    
    // DPI and scaling support
    void SetDPIScale(double scale) override;
    double GetDPIScale() const override;
    void EnableAutoDPIScaling(bool enable) override;
    bool IsAutoDPIScalingEnabled() const override;
    void RefreshForDPIChange() override;
    
    // Performance and quality settings
    void SetRenderingQuality(int quality) override;
    int GetRenderingQuality() const override;
    void EnableDoubleBuffering(bool enable) override;
    bool IsDoubleBufferingEnabled() const override;
    void SetVSync(bool enable) override;
    bool IsVSyncEnabled() const override;
    
    // Event handling
    void BindVisualEvent(wxEventType eventType, 
                        std::function<void(const DockEventData&)> handler) override;
    void UnbindVisualEvent(wxEventType eventType) override;
    void ProcessVisualEvents() override;
    
    // State management
    void SaveVisualState() override;
    void RestoreVisualState() override;
    void ResetToDefaultVisualState() override;
    bool LoadVisualStateFromFile(const wxString& filename) override;
    bool SaveVisualStateToFile(const wxString& filename) override;
    
    // Utility functions
    void RefreshAll() override;
    void UpdateAll() override;
    void ForceRedraw() override;
    void InvalidateCache() override;
    void ClearCache() override;
    
    // Debug and development
    void EnableDebugMode(bool enable) override;
    bool IsDebugModeEnabled() const override;
    void ShowDebugInfo(bool show) override;
    bool IsDebugInfoVisible() const override;
    void DumpVisualState() const override;
    
    // Statistics and monitoring
    int GetRenderedFrameCount() const override;
    double GetAverageFrameTime() const override;
    int GetMemoryUsage() const override;
    wxString GetPerformanceStatistics() const override;
    void ResetPerformanceCounters() override;
    
    // Platform-specific features
    bool IsPlatformFeatureSupported(const wxString& feature) const override;
    void EnablePlatformFeature(const wxString& feature, bool enable) override;
    bool IsPlatformFeatureEnabled(const wxString& feature) const override;
    wxArrayString GetSupportedPlatformFeatures() const override;

private:
    // Component renderers
    std::unique_ptr<IDockGuidesRenderer> m_guidesRenderer;
    std::unique_ptr<IDragPreviewRenderer> m_previewRenderer;
    std::unique_ptr<IAreaHighlighter> m_areaHighlighter;
    std::unique_ptr<ISplitterIndicator> m_splitterIndicator;
    
    // Internal state
    DockGuideConfig m_guideConfig;
    bool m_animationsEnabled;
    bool m_visualEffectsEnabled;
    bool m_debugModeEnabled;
    double m_dpiScale;
    int m_renderingQuality;
    int m_animationDuration;
    int m_animationEasing;
    
    // Theme and colors
    wxString m_currentTheme;
    wxColour m_primaryColor;
    wxColour m_secondaryColor;
    wxColour m_accentColor;
    
    // Performance counters
    int m_renderedFrameCount;
    double m_averageFrameTime;
    int m_memoryUsage;
    
    // Event handlers
    std::map<wxEventType, std::function<void(const DockEventData&)>> m_eventHandlers;
    
    // Helper methods
    void InitializeDefaultTheme();
    void UpdateAllComponents();
    void LogVisualEvent(const DockEventData& event);
};

} // namespace wxcoin

