#pragma once

#include <wx/glcanvas.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/colour.h>
#include <memory>
#include "viewer/EnhancedOutlinePass.h"

class SoSeparator;
class SoCamera;
class SceneManager;

/**
 * EnhancedOutlinePreviewCanvas - 增强版轮廓预览画布
 * 
 * 这个类提供了对EnhancedOutlinePass的完整预览支持，包括：
 * - 实时参数调整预览
 * - 多种调试模式
 * - 性能监控
 * - 交互式模型操作
 * - 选择状态模拟
 */
class EnhancedOutlinePreviewCanvas : public wxGLCanvas {
public:
    EnhancedOutlinePreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize);
    ~EnhancedOutlinePreviewCanvas();
    
    // Parameter management
    void updateOutlineParams(const EnhancedOutlineParams& params);
    EnhancedOutlineParams getOutlineParams() const;
    
    void updateSelectionConfig(const SelectionOutlineConfig& config);
    SelectionOutlineConfig getSelectionConfig() const;
    
    // Debug and visualization
    void setDebugMode(OutlineDebugMode mode);
    OutlineDebugMode getDebugMode() const;
    
    void setPerformanceMode(bool enabled);
    void setQualityMode(bool enabled);
    void setBalancedMode();
    
    // Color management
    void setBackgroundColor(const wxColour& color);
    void setOutlineColor(const wxColour& color);
    void setGlowColor(const wxColour& color);
    void setGeometryColor(const wxColour& color);
    void setHoverColor(const wxColour& color);
    void setSelectionColor(const wxColour& color);
    
    // Selection simulation
    void setSelectedObjects(const std::vector<int>& objects);
    void setHoveredObject(int objectId);
    void clearSelection();
    void clearHover();
    
    // Performance monitoring
    struct PerformanceInfo {
        float frameTime{ 0.0f };
        float averageFrameTime{ 0.0f };
        int frameCount{ 0 };
        bool isOptimized{ false };
        std::string performanceMode{ "Balanced" };
    };
    
    PerformanceInfo getPerformanceInfo() const;
    
    // Preview control
    void setPreviewEnabled(bool enabled);
    void setAutoRotate(bool enabled);
    void setRotationSpeed(float speed);
    
    // Model management
    void addPreviewModel(const wxString& name, SoSeparator* model);
    void removePreviewModel(const wxString& name);
    void setActiveModel(const wxString& name);
    wxArrayString getAvailableModels() const;

private:
    void initializeScene();
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void onIdle(wxIdleEvent& event);
    void onTimer(wxTimerEvent& event);
    
    void createBasicModels();
    void createAdvancedModels();
    void render();
    void updatePerformanceStats();
    void logPerformanceInfo();
    
    // Model interaction
    int getObjectAtPosition(const wxPoint& pos);
    void updateObjectSelection();
    void simulateSelectionState();
    
    // Scene management
    wxGLContext* m_glContext{ nullptr };
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_modelRoot{ nullptr };
    SoCamera* m_camera{ nullptr };
    
    // Outline system
    std::unique_ptr<EnhancedOutlinePass> m_outlinePass;
    SceneManager* m_sceneManager{ nullptr };
    
    // Parameters
    EnhancedOutlineParams m_outlineParams;
    SelectionOutlineConfig m_selectionConfig;
    OutlineDebugMode m_debugMode{ OutlineDebugMode::Final };
    
    // State management
    bool m_initialized{ false };
    bool m_needsRedraw{ true };
    bool m_previewEnabled{ true };
    bool m_autoRotate{ false };
    float m_rotationSpeed{ 1.0f };
    
    // Mouse interaction
    bool m_mouseDown{ false };
    wxPoint m_lastMousePos;
    int m_hoveredObject{ -1 };
    std::vector<int> m_selectedObjects;
    
    // Colors
    wxColour m_bgColor{ 51, 51, 51 };
    wxColour m_outlineColor{ 0, 0, 0 };
    wxColour m_glowColor{ 255, 255, 0 };
    wxColour m_geomColor{ 200, 200, 200 };
    wxColour m_hoverColor{ 255, 128, 0 };
    wxColour m_selectionColor{ 255, 0, 0 };
    
    // Performance monitoring
    PerformanceInfo m_performanceInfo;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    std::vector<float> m_frameTimes;
    wxTimer* m_performanceTimer{ nullptr };
    
    // Model management
    std::map<wxString, SoSeparator*> m_previewModels;
    wxString m_activeModel;
    
    // Performance modes
    bool m_performanceMode{ false };
    bool m_qualityMode{ false };
    
    DECLARE_EVENT_TABLE()
};