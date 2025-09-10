#pragma once

#include <wx/glcanvas.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/colour.h>
#include <wx/timer.h>
#include <wx/event.h>
#include <memory>
#include <chrono>
#include <vector>
#include <map>
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
 * - 交互式模型选择
 * - 自动旋转和动画
 */
class EnhancedOutlinePreviewCanvas : public wxGLCanvas {
public:
    EnhancedOutlinePreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize,
                                long style = wxWANTS_CHARS);
    ~EnhancedOutlinePreviewCanvas();

    // 参数更新
    void updateOutlineParams(const EnhancedOutlineParams& params);
    EnhancedOutlineParams getOutlineParams() const;

    void updateSelectionConfig(const SelectionOutlineConfig& config);
    SelectionOutlineConfig getSelectionConfig() const;

    // 调试模式
    void setDebugMode(OutlineDebugMode mode);
    OutlineDebugMode getDebugMode() const;

    // 性能模式
    void setPerformanceMode(bool enabled);
    void setQualityMode(bool enabled);
    void setBalancedMode();

    // 颜色设置
    void setBackgroundColor(const wxColour& color);
    void setOutlineColor(const wxColour& color);
    void setGlowColor(const wxColour& color);
    void setGeometryColor(const wxColour& color);
    void setHoverColor(const wxColour& color);
    void setSelectionColor(const wxColour& color);

    // 选择管理
    void setSelectedObjects(const std::vector<int>& objects);
    void setHoveredObject(int objectId);
    void clearSelection();
    void clearHover();

    // 性能信息
    struct PerformanceInfo {
        float frameTime{ 0.0f };
        float averageFrameTime{ 0.0f };
        int frameCount{ 0 };
        std::string performanceMode{ "Balanced" };
        bool isOptimized{ false };
    };
    
    PerformanceInfo getPerformanceInfo() const;

    // 预览控制
    void setPreviewEnabled(bool enabled);
    void setAutoRotate(bool enabled);
    void setRotationSpeed(float speed);

    // 模型管理
    void addPreviewModel(const wxString& name, SoSeparator* model);
    void removePreviewModel(const wxString& name);
    void setActiveModel(const wxString& name);
    wxArrayString getAvailableModels() const;

protected:
    // 事件处理
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void onIdle(wxIdleEvent& event);
    void onTimer(wxTimerEvent& event);

private:
    // 初始化
    void initializeScene();
    void createBasicModels();
    void createAdvancedModels();

    // 渲染
    void render();
    void updatePerformanceStats();
    void logPerformanceInfo();

    // 交互
    int getObjectAtPosition(const wxPoint& pos);
    void updateObjectSelection();
    void simulateSelectionState();

    // OpenGL上下文
    wxGLContext* m_glContext{ nullptr };

    // Coin3D场景
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_modelRoot{ nullptr };
    SoCamera* m_camera{ nullptr };

    // 轮廓渲染
    std::unique_ptr<EnhancedOutlinePass> m_outlinePass;
    OutlinePassManager::OutlineMode m_currentMode{ OutlinePassManager::OutlineMode::EnhancedOutline };

    // 颜色设置
    wxColour m_bgColor{ 240, 240, 240 };
    wxColour m_outlineColor{ 0, 0, 0 };
    wxColour m_glowColor{ 255, 255, 0 };
    wxColour m_geomColor{ 200, 200, 200 };
    wxColour m_hoverColor{ 255, 128, 0 };
    wxColour m_selectionColor{ 0, 128, 255 };

    // 选择状态
    std::vector<int> m_selectedObjects;
    int m_hoveredObject{ -1 };

    // 性能监控
    bool m_performanceMode{ false };
    bool m_qualityMode{ false };
    wxTimer* m_performanceTimer{ nullptr };
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    std::vector<float> m_frameTimes;

    // 交互状态
    bool m_mouseDown{ false };
    wxPoint m_lastMousePos;
    bool m_autoRotate{ false };
    float m_rotationSpeed{ 1.0f };

    // 场景管理
    SceneManager* m_sceneManager{ nullptr };
    
    // Parameters
    EnhancedOutlineParams m_outlineParams;
    SelectionOutlineConfig m_selectionConfig;
    OutlineDebugMode m_debugMode{ OutlineDebugMode::None };
    
    // State management
    bool m_initialized{ false };
    bool m_needsRedraw{ true };
    bool m_previewEnabled{ true };
    
    // Performance monitoring
    PerformanceInfo m_performanceInfo;
    
    // Model management
    std::map<wxString, SoSeparator*> m_previewModels;
    wxString m_activeModel;

    DECLARE_EVENT_TABLE()
};