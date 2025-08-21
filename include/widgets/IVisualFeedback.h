#pragma once

#include "UnifiedDockTypes.h"
#include <wx/window.h>
#include <wx/rect.h>
#include <wx/point.h>
#include <wx/colour.h>
#include <functional>

namespace wxcoin {

// Forward declarations
class ModernDockPanel;

// Abstract visual feedback interface
class IVisualFeedback {
public:
    virtual ~IVisualFeedback() = default;

    // Dock guides management
    virtual void ShowDockGuides(const wxPoint& pos, UnifiedDockArea targetArea) = 0;
    virtual void HideDockGuides() = 0;
    virtual void UpdateDockGuides(const wxPoint& pos) = 0;
    virtual bool AreDockGuidesVisible() const = 0;
    
    // Dock guide configuration
    virtual void SetDockGuideConfig(const DockGuideConfig& config) = 0;
    virtual DockGuideConfig GetDockGuideConfig() const = 0;
    virtual void SetEnabledDirections(bool left, bool right, bool top, bool bottom, bool center) = 0;
    virtual void SetCentralVisible(bool visible) = 0;
    virtual bool IsDirectionEnabled(DockGuideDirection direction) const = 0;
    
    // Drag preview management
    virtual void ShowDragPreview(wxWindow* content, const wxRect& rect) = 0;
    virtual void HideDragPreview() = 0;
    virtual void UpdateDragPreview(const wxRect& rect) = 0;
    virtual bool IsDragPreviewVisible() const = 0;
    virtual void SetDragPreviewOpacity(int opacity) = 0;
    virtual int GetDragPreviewOpacity() const = 0;
    
    // Area highlighting
    virtual void HighlightDropArea(UnifiedDockArea area, const wxRect& rect) = 0;
    virtual void ClearAreaHighlights() = 0;
    virtual void SetHighlightColor(const wxColour& color) = 0;
    virtual wxColour GetHighlightColor() const = 0;
    virtual void SetHighlightStyle(int style) = 0;
    virtual int GetHighlightStyle() const = 0;
    
    // Splitter indicators
    virtual void ShowSplitterIndicator(const wxRect& rect, SplitterOrientation orientation) = 0;
    virtual void HideSplitterIndicator() = 0;
    virtual void UpdateSplitterIndicator(const wxRect& rect, SplitterOrientation orientation) = 0;
    virtual bool IsSplitterIndicatorVisible() const = 0;
    
    // Tab indicators
    virtual void ShowTabIndicator(const wxRect& rect, const wxString& title) = 0;
    virtual void HideTabIndicator() = 0;
    virtual void UpdateTabIndicator(const wxRect& rect, const wxString& title) = 0;
    virtual bool IsTabIndicatorVisible() const = 0;
    
    // Animation and transitions
    virtual void EnableAnimations(bool enable) = 0;
    virtual bool AreAnimationsEnabled() const = 0;
    virtual void SetAnimationDuration(int milliseconds) = 0;
    virtual int GetAnimationDuration() const = 0;
    virtual void SetAnimationEasing(int easing) = 0;
    virtual int GetAnimationEasing() const = 0;
    
    // Visual effects
    virtual void EnableVisualEffects(bool enable) = 0;
    virtual bool AreVisualEffectsEnabled() const = 0;
    virtual void SetBlurEffect(bool enable) = 0;
    virtual bool IsBlurEffectEnabled() const = 0;
    virtual void SetShadowEffect(bool enable) = 0;
    virtual bool IsShadowEffectEnabled() const = 0;
    
    // Color and theme management
    virtual void SetTheme(const wxString& themeName) = 0;
    virtual wxString GetCurrentTheme() const = 0;
    virtual wxArrayString GetAvailableThemes() const = 0;
    virtual void SetCustomColors(const wxColour& primary, const wxColour& secondary, const wxColour& accent) = 0;
    virtual void GetCustomColors(wxColour& primary, wxColour& secondary, wxColour& accent) const = 0;
    
    // DPI and scaling support
    virtual void SetDPIScale(double scale) = 0;
    virtual double GetDPIScale() const = 0;
    virtual void EnableAutoDPIScaling(bool enable) = 0;
    virtual bool IsAutoDPIScalingEnabled() const = 0;
    virtual void RefreshForDPIChange() = 0;
    
    // Performance and quality settings
    virtual void SetRenderingQuality(int quality) = 0;
    virtual int GetRenderingQuality() const = 0;
    virtual void EnableDoubleBuffering(bool enable) = 0;
    virtual bool IsDoubleBufferingEnabled() const = 0;
    virtual void SetVSync(bool enable) = 0;
    virtual bool IsVSyncEnabled() const = 0;
    
    // Event handling
    virtual void BindVisualEvent(wxEventType eventType, 
                                std::function<void(const DockEventData&)> handler) = 0;
    virtual void UnbindVisualEvent(wxEventType eventType) = 0;
    virtual void ProcessVisualEvents() = 0;
    
    // State management
    virtual void SaveVisualState() = 0;
    virtual void RestoreVisualState() = 0;
    virtual void ResetToDefaultVisualState() = 0;
    virtual bool LoadVisualStateFromFile(const wxString& filename) = 0;
    virtual bool SaveVisualStateToFile(const wxString& filename) = 0;
    
    // Utility functions
    virtual void RefreshAll() = 0;
    virtual void UpdateAll() = 0;
    virtual void ForceRedraw() = 0;
    virtual void InvalidateCache() = 0;
    virtual void ClearCache() = 0;
    
    // Debug and development
    virtual void EnableDebugMode(bool enable) = 0;
    virtual bool IsDebugModeEnabled() const = 0;
    virtual void ShowDebugInfo(bool show) = 0;
    virtual bool IsDebugInfoVisible() const = 0;
    virtual void DumpVisualState() const = 0;
    
    // Statistics and monitoring
    virtual int GetRenderedFrameCount() const = 0;
    virtual double GetAverageFrameTime() const = 0;
    virtual int GetMemoryUsage() const = 0;
    virtual wxString GetPerformanceStatistics() const = 0;
    virtual void ResetPerformanceCounters() = 0;
    
    // Platform-specific features
    virtual bool IsPlatformFeatureSupported(const wxString& feature) const = 0;
    virtual void EnablePlatformFeature(const wxString& feature, bool enable) = 0;
    virtual bool IsPlatformFeatureEnabled(const wxString& feature) const = 0;
    virtual wxArrayString GetSupportedPlatformFeatures() const = 0;
};

} // namespace wxcoin

