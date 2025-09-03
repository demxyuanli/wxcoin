#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Mesh;
class Shader;
class Framebuffer;
class Texture;

// 轮廓渲染方法枚举
enum class OutlineMethod {
    NormalExtrusion,      // 法向量扩展
    InvertedHull,        // 反向外壳
    ScreenSpace,         // 屏幕空间
    GeometryShader,      // 几何着色器
    JumpFlooding,        // 跳跃泛洪算法
    Hybrid               // 混合方法
};

// 轮廓参数结构
struct OutlineParams {
    glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);  // 轮廓颜色
    float thickness = 2.0f;                           // 轮廓厚度（像素）
    float intensity = 1.0f;                           // 轮廓强度
    
    // 深度检测参数
    float depthThreshold = 0.001f;                    // 深度阈值
    float depthBias = 1.0f;                          // 深度偏移
    
    // 法向量检测参数
    float normalThreshold = 0.4f;                     // 法向量阈值
    float creaseAngle = 45.0f;                       // 折痕角度（度）
    
    // 高级参数
    bool useAdaptiveThickness = false;               // 自适应厚度
    bool useSoftOutline = false;                     // 柔和轮廓
    float fadeDistance = 100.0f;                      // 淡出距离
};

// 轮廓渲染器基类
class OutlineRenderer {
public:
    OutlineRenderer();
    virtual ~OutlineRenderer();
    
    // 初始化
    virtual bool initialize(int width, int height);
    
    // 渲染轮廓
    virtual void renderOutline(const std::vector<std::shared_ptr<Mesh>>& meshes,
                              const glm::mat4& view,
                              const glm::mat4& projection,
                              const OutlineParams& params) = 0;
    
    // 调整大小
    virtual void resize(int width, int height);
    
    // 设置渲染方法
    void setMethod(OutlineMethod method) { m_method = method; }
    OutlineMethod getMethod() const { return m_method; }
    
protected:
    OutlineMethod m_method;
    int m_width;
    int m_height;
    
    // 共享资源
    std::shared_ptr<Shader> m_quadShader;
    unsigned int m_quadVAO;
    
    // 创建全屏四边形
    void createQuadVAO();
    void renderQuad();
};

// 法向量扩展轮廓渲染器
class NormalExtrusionOutlineRenderer : public OutlineRenderer {
public:
    NormalExtrusionOutlineRenderer();
    ~NormalExtrusionOutlineRenderer() override;
    
    bool initialize(int width, int height) override;
    void renderOutline(const std::vector<std::shared_ptr<Mesh>>& meshes,
                      const glm::mat4& view,
                      const glm::mat4& projection,
                      const OutlineParams& params) override;
    
private:
    std::shared_ptr<Shader> m_outlineShader;
};

// 屏幕空间轮廓渲染器
class ScreenSpaceOutlineRenderer : public OutlineRenderer {
public:
    ScreenSpaceOutlineRenderer();
    ~ScreenSpaceOutlineRenderer() override;
    
    bool initialize(int width, int height) override;
    void renderOutline(const std::vector<std::shared_ptr<Mesh>>& meshes,
                      const glm::mat4& view,
                      const glm::mat4& projection,
                      const OutlineParams& params) override;
    void resize(int width, int height) override;
    
private:
    std::shared_ptr<Framebuffer> m_gBuffer;
    std::shared_ptr<Texture> m_colorTexture;
    std::shared_ptr<Texture> m_depthTexture;
    std::shared_ptr<Texture> m_normalTexture;
    std::shared_ptr<Shader> m_geometryShader;
    std::shared_ptr<Shader> m_outlineShader;
    
    void createGBuffer();
    void renderToGBuffer(const std::vector<std::shared_ptr<Mesh>>& meshes,
                        const glm::mat4& view,
                        const glm::mat4& projection);
};

// 跳跃泛洪算法轮廓渲染器
class JumpFloodingOutlineRenderer : public OutlineRenderer {
public:
    JumpFloodingOutlineRenderer();
    ~JumpFloodingOutlineRenderer() override;
    
    bool initialize(int width, int height) override;
    void renderOutline(const std::vector<std::shared_ptr<Mesh>>& meshes,
                      const glm::mat4& view,
                      const glm::mat4& projection,
                      const OutlineParams& params) override;
    void resize(int width, int height) override;
    
private:
    std::shared_ptr<Framebuffer> m_silhouetteFBO;
    std::shared_ptr<Framebuffer> m_jfaFBO[2];  // 双缓冲
    std::shared_ptr<Texture> m_silhouetteTexture;
    std::shared_ptr<Texture> m_jfaTexture[2];
    std::shared_ptr<Shader> m_silhouetteShader;
    std::shared_ptr<Shader> m_jfaInitShader;
    std::shared_ptr<Shader> m_jfaStepShader;
    std::shared_ptr<Shader> m_outlineShader;
    
    void createFramebuffers();
    void renderSilhouette(const std::vector<std::shared_ptr<Mesh>>& meshes,
                         const glm::mat4& view,
                         const glm::mat4& projection);
    void performJFA(int passes);
};

// 混合轮廓渲染器（结合多种方法）
class HybridOutlineRenderer : public OutlineRenderer {
public:
    HybridOutlineRenderer();
    ~HybridOutlineRenderer() override;
    
    bool initialize(int width, int height) override;
    void renderOutline(const std::vector<std::shared_ptr<Mesh>>& meshes,
                      const glm::mat4& view,
                      const glm::mat4& projection,
                      const OutlineParams& params) override;
    void resize(int width, int height) override;
    
    // 设置各方法的权重
    void setMethodWeights(float geometryWeight, float screenSpaceWeight);
    
private:
    std::unique_ptr<NormalExtrusionOutlineRenderer> m_geometryRenderer;
    std::unique_ptr<ScreenSpaceOutlineRenderer> m_screenSpaceRenderer;
    std::shared_ptr<Framebuffer> m_combineFBO;
    std::shared_ptr<Texture> m_geometryOutline;
    std::shared_ptr<Texture> m_screenSpaceOutline;
    std::shared_ptr<Shader> m_combineShader;
    
    float m_geometryWeight = 0.5f;
    float m_screenSpaceWeight = 0.5f;
};

// 轮廓渲染管理器
class OutlineRenderManager {
public:
    OutlineRenderManager();
    ~OutlineRenderManager();
    
    // 初始化所有渲染器
    bool initialize(int width, int height);
    
    // 渲染轮廓
    void render(const std::vector<std::shared_ptr<Mesh>>& meshes,
                const glm::mat4& view,
                const glm::mat4& projection,
                const OutlineParams& params);
    
    // 设置当前渲染方法
    void setMethod(OutlineMethod method);
    OutlineMethod getCurrentMethod() const { return m_currentMethod; }
    
    // 调整大小
    void resize(int width, int height);
    
    // 获取特定渲染器
    std::shared_ptr<OutlineRenderer> getRenderer(OutlineMethod method);
    
    // 性能统计
    struct PerformanceStats {
        float renderTime = 0.0f;
        int drawCalls = 0;
        int vertices = 0;
    };
    
    const PerformanceStats& getStats() const { return m_stats; }
    
private:
    OutlineMethod m_currentMethod = OutlineMethod::ScreenSpace;
    std::unordered_map<OutlineMethod, std::shared_ptr<OutlineRenderer>> m_renderers;
    PerformanceStats m_stats;
    
    // 计时器
    void beginTimer();
    void endTimer();
    float m_timerStart = 0.0f;
};