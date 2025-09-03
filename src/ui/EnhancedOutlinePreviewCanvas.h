#pragma once

#include <wx/glcanvas.h>
#include <wx/colordlg.h>
#include <memory>

class SoSeparator;
class SoPerspectiveCamera;
class SoRotationXYZ;
class SoGLRenderAction;

// 轮廓渲染方法枚举
enum class OutlineMethod {
    BASIC,              // 基础渲染（无轮廓）
    INVERTED_HULL,      // 反向外壳法
    SCREEN_SPACE,       // 屏幕空间边缘检测
    GEOMETRY_SILHOUETTE,// 几何轮廓提取
    STENCIL_BUFFER,     // 模板缓冲法
    MULTI_PASS          // 多通道混合
};

// 扩展的轮廓参数
struct EnhancedOutlineParams {
    // 基础参数（兼容原有的ImageOutlineParams）
    float depthWeight = 1.0f;
    float normalWeight = 1.0f;
    float depthThreshold = 0.002f;
    float normalThreshold = 0.1f;
    float edgeIntensity = 1.0f;
    float thickness = 1.0f;
    
    // 扩展参数
    float creaseAngle = 45.0f;      // 折痕角度阈值（度）
    float fadeDistance = 50.0f;      // 淡出距离
    bool adaptiveThickness = false;  // 自适应厚度
    bool antiAliasing = true;        // 抗锯齿
    int sampleCount = 4;             // 采样数量
    
    // 高级参数
    float innerEdgeIntensity = 0.3f; // 内部边缘强度
    float silhouetteBoost = 1.5f;    // 轮廓增强
    bool useColorGradient = false;   // 使用颜色渐变
    wxColour gradientColor1 = wxColour(0, 0, 0);      // 渐变色1
    wxColour gradientColor2 = wxColour(64, 64, 64);   // 渐变色2
};

// 增强版轮廓预览画布
class EnhancedOutlinePreviewCanvas : public wxGLCanvas {
public:
    EnhancedOutlinePreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize);
    virtual ~EnhancedOutlinePreviewCanvas();
    
    // 轮廓参数设置
    void updateOutlineParams(const EnhancedOutlineParams& params);
    EnhancedOutlineParams getOutlineParams() const { return m_outlineParams; }
    
    // 轮廓开关
    void setOutlineEnabled(bool enabled) { m_outlineEnabled = enabled; m_needsRedraw = true; Refresh(false); }
    bool isOutlineEnabled() const { return m_outlineEnabled; }
    
    // 轮廓方法选择
    void setOutlineMethod(OutlineMethod method);
    OutlineMethod getOutlineMethod() const { return m_outlineMethod; }
    
    // 颜色设置
    void setGeometryColor(const wxColour& color) { m_geomColor = color; createBasicModels(); m_needsRedraw = true; Refresh(false); }
    void setBackgroundColor(const wxColour& color) { m_bgColor = color; m_needsRedraw = true; Refresh(false); }
    void setOutlineColor(const wxColour& color) { m_outlineColor = color; m_needsRedraw = true; Refresh(false); }
    
    // 相机控制
    void resetCamera();
    void setCameraDistance(float distance);
    
    // 性能统计
    struct PerformanceStats {
        float renderTime = 0.0f;      // 渲染时间（毫秒）
        int drawCalls = 0;            // 绘制调用次数
        int triangles = 0;            // 三角形数量
        OutlineMethod currentMethod;  // 当前方法
    };
    
    PerformanceStats getPerformanceStats() const { return m_stats; }
    
protected:
    // 事件处理
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void onIdle(wxIdleEvent& event);
    
private:
    // 场景初始化
    void initializeScene();
    void setupLighting();
    void createBasicModels();
    void createCube(SoSeparator* parent);
    void createSphere(SoSeparator* parent);
    void createCylinder(SoSeparator* parent);
    void createCone(SoSeparator* parent);
    
    // 渲染方法
    void render();
    void renderBasic(SoGLRenderAction& action);
    void renderInvertedHull(SoGLRenderAction& action);
    void renderScreenSpace(SoGLRenderAction& action);
    void renderGeometrySilhouette(SoGLRenderAction& action);
    void renderStencilBuffer(SoGLRenderAction& action);
    void renderMultiPass(SoGLRenderAction& action);
    
    // 辅助方法
    void beginPerformanceTimer();
    void endPerformanceTimer();
    void extractSilhouetteEdges();
    
private:
    // OpenGL上下文
    wxGLContext* m_glContext = nullptr;
    
    // Coin3D场景图
    SoSeparator* m_sceneRoot = nullptr;
    SoSeparator* m_modelRoot = nullptr;
    SoSeparator* m_outlineRoot = nullptr;
    SoPerspectiveCamera* m_camera = nullptr;
    SoRotationXYZ* m_rotationNode = nullptr;
    
    // 状态
    bool m_initialized = false;
    bool m_needsRedraw = false;
    bool m_mouseDown = false;
    wxPoint m_lastMousePos;
    
    // 轮廓设置
    bool m_outlineEnabled = true;
    OutlineMethod m_outlineMethod = OutlineMethod::INVERTED_HULL;
    EnhancedOutlineParams m_outlineParams;
    
    // 颜色
    wxColour m_bgColor = wxColour(240, 240, 240);
    wxColour m_geomColor = wxColour(100, 150, 200);
    wxColour m_outlineColor = wxColour(0, 0, 0);
    wxColour m_hoverColor = wxColour(255, 128, 0);
    
    // 性能统计
    PerformanceStats m_stats;
    
    DECLARE_EVENT_TABLE()
};