#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <type_traits>
#include <variant>
#include <mutex>
#include <atomic>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/RenderingConfig.h"

/**
 * @brief 参数值类型定义
 */
using ParameterValue = std::variant<
    bool, int, double, std::string, 
    Quantity_Color,
    RenderingConfig::TextureMode,
    RenderingConfig::BlendMode,
    RenderingConfig::ShadingMode,
    RenderingConfig::DisplayMode,
    RenderingConfig::RenderingQuality,
    RenderingConfig::ShadowMode,
    RenderingConfig::LightingModel
>;

/**
 * @brief 参数变更回调函数类型
 */
using ParameterChangedCallback = std::function<void(const std::string&, const ParameterValue&)>;

/**
 * @brief 批量更新回调函数类型
 */
using BatchUpdateCallback = std::function<void(const std::vector<std::string>&)>;

/**
 * @brief 参数节点类型枚举
 */
enum class ParameterNodeType {
    Root,           // 根节点
    Category,       // 分类节点
    Group,          // 组节点
    Parameter       // 参数节点
};

/**
 * @brief 参数节点基类
 */
class ParameterNode {
public:
    ParameterNode(const std::string& name, ParameterNodeType type, ParameterNode* parent = nullptr);
    virtual ~ParameterNode() = default;

    // 基本信息
    const std::string& getName() const { return m_name; }
    ParameterNodeType getType() const { return m_type; }
    ParameterNode* getParent() const { return m_parent; }
    const std::string& getPath() const { return m_path; }

    // 子节点管理
    void addChild(std::shared_ptr<ParameterNode> child);
    void removeChild(const std::string& name);
    std::shared_ptr<ParameterNode> getChild(const std::string& name) const;
    std::vector<std::shared_ptr<ParameterNode>> getChildren() const;
    bool hasChild(const std::string& name) const;

    // 路径操作
    std::string getFullPath() const;
    static std::vector<std::string> parsePath(const std::string& path);

protected:
    void updatePath();

    std::string m_name;
    ParameterNodeType m_type;
    ParameterNode* m_parent;
    std::string m_path;
    std::map<std::string, std::shared_ptr<ParameterNode>> m_children;
    mutable std::mutex m_childrenMutex;
};

/**
 * @brief 参数节点实现
 */
class Parameter : public ParameterNode {
public:
    Parameter(const std::string& name, const ParameterValue& defaultValue, ParameterNode* parent = nullptr);
    
    // 参数值操作
    const ParameterValue& getValue() const { return m_value; }
    void setValue(const ParameterValue& value);
    const ParameterValue& getDefaultValue() const { return m_defaultValue; }
    void resetToDefault();

    // 类型信息
    template<typename T>
    T getValueAs() const {
        return std::get<T>(m_value);
    }

    template<typename T>
    bool isType() const {
        return std::holds_alternative<T>(m_value);
    }

    // 回调管理
    void addChangedCallback(ParameterChangedCallback callback);
    void removeChangedCallback(ParameterChangedCallback callback);
    void notifyChanged();

    // 验证
    void setValidator(std::function<bool(const ParameterValue&)> validator);
    bool validate(const ParameterValue& value) const;

private:
    ParameterValue m_value;
    ParameterValue m_defaultValue;
    std::vector<ParameterChangedCallback> m_callbacks;
    std::function<bool(const ParameterValue&)> m_validator;
    mutable std::mutex m_valueMutex;
};

/**
 * @brief 参数树管理器
 */
class ParameterTree {
public:
    static ParameterTree& getInstance();

    // 树结构管理
    std::shared_ptr<ParameterNode> getRoot() const { return m_root; }
    std::shared_ptr<ParameterNode> findNode(const std::string& path) const;
    std::shared_ptr<Parameter> findParameter(const std::string& path) const;

    // 参数注册
    std::shared_ptr<Parameter> registerParameter(
        const std::string& path, 
        const ParameterValue& defaultValue,
        std::function<bool(const ParameterValue&)> validator = nullptr
    );

    // 参数操作
    bool setParameterValue(const std::string& path, const ParameterValue& value);
    ParameterValue getParameterValue(const std::string& path) const;
    bool hasParameter(const std::string& path) const;

    // 批量操作
    void beginBatchUpdate();
    void endBatchUpdate();
    void setBatchUpdateCallback(BatchUpdateCallback callback);

    // 回调管理
    void addGlobalChangedCallback(ParameterChangedCallback callback);
    void removeGlobalChangedCallback(ParameterChangedCallback callback);

    // 路径操作
    std::vector<std::string> getAllParameterPaths() const;
    std::vector<std::string> getParameterPathsInCategory(const std::string& category) const;

    // 序列化
    std::string serializeToJson() const;
    bool deserializeFromJson(const std::string& json);

private:
    ParameterTree();
    ~ParameterTree() = default;
    ParameterTree(const ParameterTree&) = delete;
    ParameterTree& operator=(const ParameterTree&) = delete;

    std::shared_ptr<ParameterNode> createNodePath(const std::string& path, ParameterNodeType type);
    void notifyParameterChanged(const std::string& path, const ParameterValue& value);
    void notifyBatchUpdate(const std::vector<std::string>& changedPaths);

    std::shared_ptr<ParameterNode> m_root;
    std::vector<ParameterChangedCallback> m_globalCallbacks;
    BatchUpdateCallback m_batchUpdateCallback;
    std::vector<std::string> m_batchChangedPaths;
    bool m_inBatchUpdate;
    mutable std::mutex m_treeMutex;
};

/**
 * @brief 参数树构建器 - 用于初始化参数树结构
 */
class ParameterTreeBuilder {
public:
    static void buildGeometryParameterTree();
    static void buildRenderingParameterTree();
    static void buildDisplayParameterTree();
    static void buildQualityParameterTree();
    static void buildLightingParameterTree();
    static void buildMaterialParameterTree();
    static void buildTextureParameterTree();
    static void buildShadowParameterTree();

private:
    static void addGeometryParameters();
    static void addRenderingParameters();
    static void addDisplayParameters();
    static void addQualityParameters();
    static void addLightingParameters();
    static void addMaterialParameters();
    static void addTextureParameters();
    static void addShadowParameters();
};