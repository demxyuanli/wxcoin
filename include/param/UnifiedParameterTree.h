#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <any>
#include <chrono>
#include <mutex>
#include <atomic>
#include <queue>
#include <set>

// Forward declarations
class ParameterNode;
class ParameterRegistry;
class UpdateCoordinator;

/**
 * @brief 参数值类型定义
 * 支持多种数据类型的参数值
 */
using ParameterValue = std::variant<
    bool,           // 布尔值
    int,            // 整数
    double,         // 浮点数
    std::string,    // 字符串
    std::vector<double>, // 向量（用于颜色、位置等）
    std::any        // 任意类型（用于复杂对象）
>;

/**
 * @brief 参数变更事件
 */
struct ParameterChangeEvent {
    std::string path;                    // 参数路径
    ParameterValue oldValue;             // 旧值
    ParameterValue newValue;             // 新值
    std::chrono::steady_clock::time_point timestamp; // 时间戳
    std::string source;                  // 变更来源
    bool isBatchUpdate;                  // 是否为批量更新
};

/**
 * @brief 参数节点基类
 * 表示参数树中的一个节点，可以是参数值或容器节点
 */
class ParameterNode {
public:
    enum class Type {
        CONTAINER,      // 容器节点
        PARAMETER,      // 参数节点
        GROUP           // 分组节点
    };

    ParameterNode(const std::string& name, Type type = Type::CONTAINER);
    virtual ~ParameterNode() = default;

    // 基本信息
    const std::string& getName() const { return m_name; }
    Type getType() const { return m_type; }
    const std::string& getPath() const { return m_path; }
    ParameterNode* getParent() const { return m_parent; }

    // 子节点管理
    void addChild(std::shared_ptr<ParameterNode> child);
    void removeChild(const std::string& name);
    std::shared_ptr<ParameterNode> getChild(const std::string& name) const;
    std::vector<std::shared_ptr<ParameterNode>> getChildren() const;
    bool hasChild(const std::string& name) const;

    // 路径操作
    std::string getFullPath() const;
    std::vector<std::string> getPathComponents() const;

    // 参数值操作（仅对参数节点有效）
    virtual bool setValue(const ParameterValue& value);
    virtual ParameterValue getValue() const;
    virtual bool hasValue() const { return false; }

    // 元数据
    void setDescription(const std::string& desc) { m_description = desc; }
    const std::string& getDescription() const { return m_description; }
    void setTags(const std::vector<std::string>& tags) { m_tags = tags; }
    const std::vector<std::string>& getTags() const { return m_tags; }

    // 依赖关系
    void addDependency(const std::string& paramPath);
    void removeDependency(const std::string& paramPath);
    const std::set<std::string>& getDependencies() const { return m_dependencies; }

    // 验证
    virtual bool validateValue(const ParameterValue& value) const { return true; }

protected:
    void setPath(const std::string& path) { m_path = path; }
    void setParent(ParameterNode* parent) { m_parent = parent; }
    void updateChildrenPaths();

private:
    std::string m_name;
    Type m_type;
    std::string m_path;
    ParameterNode* m_parent;
    std::unordered_map<std::string, std::shared_ptr<ParameterNode>> m_children;
    std::string m_description;
    std::vector<std::string> m_tags;
    std::set<std::string> m_dependencies; // 依赖的其他参数路径
};

/**
 * @brief 参数值节点
 * 存储实际参数值的叶子节点
 */
class ParameterValueNode : public ParameterNode {
public:
    ParameterValueNode(const std::string& name, const ParameterValue& defaultValue = ParameterValue{});
    ~ParameterValueNode() override = default;

    // 参数值操作
    bool setValue(const ParameterValue& value) override;
    ParameterValue getValue() const override;
    bool hasValue() const override { return true; }

    // 默认值
    void setDefaultValue(const ParameterValue& value) { m_defaultValue = value; }
    ParameterValue getDefaultValue() const { return m_defaultValue; }
    void resetToDefault() { setValue(m_defaultValue); }

    // 值范围限制
    void setMinValue(const ParameterValue& min) { m_minValue = min; }
    void setMaxValue(const ParameterValue& max) { m_maxValue = max; }
    ParameterValue getMinValue() const { return m_minValue; }
    ParameterValue getMaxValue() const { return m_maxValue; }

    // 验证
    bool validateValue(const ParameterValue& value) const override;

private:
    ParameterValue m_value;
    ParameterValue m_defaultValue;
    ParameterValue m_minValue;
    ParameterValue m_maxValue;
    std::mutex m_valueMutex;
};

/**
 * @brief 参数组节点
 * 用于逻辑分组的容器节点
 */
class ParameterGroupNode : public ParameterNode {
public:
    ParameterGroupNode(const std::string& name, const std::string& description = "");
    ~ParameterGroupNode() override = default;

    // 组特定操作
    void setCollapsed(bool collapsed) { m_collapsed = collapsed; }
    bool isCollapsed() const { return m_collapsed; }
    void setIcon(const std::string& icon) { m_icon = icon; }
    const std::string& getIcon() const { return m_icon; }

private:
    bool m_collapsed = false;
    std::string m_icon;
};

/**
 * @brief 统一参数树管理器
 * 管理整个参数树结构，提供统一的参数访问接口
 */
class UnifiedParameterTree {
public:
    UnifiedParameterTree();
    ~UnifiedParameterTree();

    // 树结构管理
    std::shared_ptr<ParameterNode> getRoot() const { return m_root; }
    std::shared_ptr<ParameterNode> getNode(const std::string& path) const;
    std::shared_ptr<ParameterNode> createNode(const std::string& path, ParameterNode::Type type = ParameterNode::Type::CONTAINER);
    std::shared_ptr<ParameterValueNode> createParameter(const std::string& path, const ParameterValue& defaultValue = ParameterValue{});
    std::shared_ptr<ParameterGroupNode> createGroup(const std::string& path, const std::string& description = "");

    // 参数值操作
    bool setParameterValue(const std::string& path, const ParameterValue& value);
    ParameterValue getParameterValue(const std::string& path) const;
    bool hasParameter(const std::string& path) const;

    // 批量操作
    bool setParameterValues(const std::unordered_map<std::string, ParameterValue>& values);
    std::unordered_map<std::string, ParameterValue> getParameterValues(const std::vector<std::string>& paths) const;

    // 路径操作
    std::vector<std::string> getAllParameterPaths() const;
    std::vector<std::string> getParameterPathsByTag(const std::string& tag) const;
    std::vector<std::string> getChildPaths(const std::string& parentPath) const;

    // 依赖关系管理
    void addDependency(const std::string& paramPath, const std::string& dependencyPath);
    void removeDependency(const std::string& paramPath, const std::string& dependencyPath);
    std::vector<std::string> getDependentParameters(const std::string& paramPath) const;

    // 事件回调
    using ParameterChangeCallback = std::function<void(const ParameterChangeEvent&)>;
    int registerChangeCallback(ParameterChangeCallback callback);
    void unregisterChangeCallback(int callbackId);
    void notifyParameterChange(const ParameterChangeEvent& event);

    // 序列化
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    // 验证
    bool validateAllParameters() const;
    std::vector<std::string> getValidationErrors() const;

private:
    std::shared_ptr<ParameterNode> m_root;
    std::unordered_map<int, ParameterChangeCallback> m_callbacks;
    int m_nextCallbackId;
    mutable std::mutex m_treeMutex;

    // 内部辅助方法
    std::shared_ptr<ParameterNode> findOrCreateNode(const std::string& path);
    std::vector<std::string> splitPath(const std::string& path) const;
    void validatePath(const std::string& path) const;
    void updateDependentParameters(const std::string& changedPath);
};

/**
 * @brief 参数树工厂
 * 提供预定义的参数树结构创建功能
 */
class ParameterTreeFactory {
public:
    // 几何表示参数
    static std::shared_ptr<UnifiedParameterTree> createGeometryParameterTree();
    
    // 渲染控制参数
    static std::shared_ptr<UnifiedParameterTree> createRenderingParameterTree();
    
    // 网格参数
    static std::shared_ptr<UnifiedParameterTree> createMeshParameterTree();
    
    // 光照参数
    static std::shared_ptr<UnifiedParameterTree> createLightingParameterTree();
    
    // 完整参数树（包含所有参数）
    static std::shared_ptr<UnifiedParameterTree> createCompleteParameterTree();

private:
    static void addGeometryParameters(std::shared_ptr<UnifiedParameterTree> tree);
    static void addRenderingParameters(std::shared_ptr<UnifiedParameterTree> tree);
    static void addMeshParameters(std::shared_ptr<UnifiedParameterTree> tree);
    static void addLightingParameters(std::shared_ptr<UnifiedParameterTree> tree);
};