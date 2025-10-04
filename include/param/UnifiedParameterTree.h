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
 * @brief Parameter value type definition
 * Supports multiple data types for parameter values
 */
using ParameterValue = std::variant<
    bool,           // Boolean value
    int,            // Integer
    double,         // Floating point number
    std::string,    // String
    std::vector<double>, // Vector (for colors, positions, etc.)
    std::any        // Any type (for complex objects)
>;

/**
 * @brief Parameter change event
 */
struct ParameterChangeEvent {
    std::string path;                    // Parameter path
    ParameterValue oldValue;             // Old value
    ParameterValue newValue;             // New value
    std::chrono::steady_clock::time_point timestamp; // Timestamp
    std::string source;                  // Change source
    bool isBatchUpdate;                  // Whether it's a batch update
};

/**
 * @brief Parameter node base class
 * Represents a node in the parameter tree, can be a parameter value or container node
 */
class ParameterNode {
public:
    enum class Type {
        CONTAINER,      // Container node
        PARAMETER,      // Parameter node
        GROUP           // Group node
    };

    ParameterNode(const std::string& name, Type type = Type::CONTAINER);
    virtual ~ParameterNode() = default;

    // Basic information
    const std::string& getName() const { return m_name; }
    Type getType() const { return m_type; }
    const std::string& getPath() const { return m_path; }
    ParameterNode* getParent() const { return m_parent; }

    // Child node management
    void addChild(std::shared_ptr<ParameterNode> child);
    void removeChild(const std::string& name);
    std::shared_ptr<ParameterNode> getChild(const std::string& name) const;
    std::vector<std::shared_ptr<ParameterNode>> getChildren() const;
    bool hasChild(const std::string& name) const;

    // Path operations
    std::string getFullPath() const;
    std::vector<std::string> getPathComponents() const;

    // Parameter value operations (only valid for parameter nodes)
    virtual bool setValue(const ParameterValue& value);
    virtual ParameterValue getValue() const;
    virtual bool hasValue() const { return false; }

    // Metadata
    void setDescription(const std::string& desc) { m_description = desc; }
    const std::string& getDescription() const { return m_description; }
    void setTags(const std::vector<std::string>& tags) { m_tags = tags; }
    const std::vector<std::string>& getTags() const { return m_tags; }

    // Dependencies
    void addDependency(const std::string& paramPath);
    void removeDependency(const std::string& paramPath);
    const std::set<std::string>& getDependencies() const { return m_dependencies; }

    // Validation
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
    std::set<std::string> m_dependencies; // Dependent parameter paths
};

/**
 * @brief Parameter value node
 * Leaf node that stores actual parameter values
 */
class ParameterValueNode : public ParameterNode {
public:
    ParameterValueNode(const std::string& name, const ParameterValue& defaultValue = ParameterValue{});
    ~ParameterValueNode() override = default;

    // Parameter value operations
    bool setValue(const ParameterValue& value) override;
    ParameterValue getValue() const override;
    bool hasValue() const override { return true; }

    // Default values
    void setDefaultValue(const ParameterValue& value) { m_defaultValue = value; }
    ParameterValue getDefaultValue() const { return m_defaultValue; }
    void resetToDefault() { setValue(m_defaultValue); }

    // Value range constraints
    void setMinValue(const ParameterValue& min) { m_minValue = min; }
    void setMaxValue(const ParameterValue& max) { m_maxValue = max; }
    ParameterValue getMinValue() const { return m_minValue; }
    ParameterValue getMaxValue() const { return m_maxValue; }

    // Validation
    bool validateValue(const ParameterValue& value) const override;

private:
    ParameterValue m_value;
    ParameterValue m_defaultValue;
    ParameterValue m_minValue;
    ParameterValue m_maxValue;
    std::mutex m_valueMutex;
};

/**
 * @brief Parameter group node
 * Container node for logical grouping
 */
class ParameterGroupNode : public ParameterNode {
public:
    ParameterGroupNode(const std::string& name, const std::string& description = "");
    ~ParameterGroupNode() override = default;

    // Group-specific operations
    void setCollapsed(bool collapsed) { m_collapsed = collapsed; }
    bool isCollapsed() const { return m_collapsed; }
    void setIcon(const std::string& icon) { m_icon = icon; }
    const std::string& getIcon() const { return m_icon; }

private:
    bool m_collapsed = false;
    std::string m_icon;
};

/**
 * @brief Unified parameter tree manager
 * Manages the entire parameter tree structure and provides unified parameter access interface
 */
class UnifiedParameterTree {
public:
    UnifiedParameterTree();
    ~UnifiedParameterTree();

    // Tree structure management
    std::shared_ptr<ParameterNode> getRoot() const { return m_root; }
    std::shared_ptr<ParameterNode> getNode(const std::string& path) const;
    std::shared_ptr<ParameterNode> createNode(const std::string& path, ParameterNode::Type type = ParameterNode::Type::CONTAINER);
    std::shared_ptr<ParameterValueNode> createParameter(const std::string& path, const ParameterValue& defaultValue = ParameterValue{});
    std::shared_ptr<ParameterGroupNode> createGroup(const std::string& path, const std::string& description = "");

    // Parameter value operations
    bool setParameterValue(const std::string& path, const ParameterValue& value);
    ParameterValue getParameterValue(const std::string& path) const;
    bool hasParameter(const std::string& path) const;

    // Batch operations
    bool setParameterValues(const std::unordered_map<std::string, ParameterValue>& values);
    std::unordered_map<std::string, ParameterValue> getParameterValues(const std::vector<std::string>& paths) const;

    // Path operations
    std::vector<std::string> getAllParameterPaths() const;
    std::vector<std::string> getParameterPathsByTag(const std::string& tag) const;
    std::vector<std::string> getChildPaths(const std::string& parentPath) const;

    // Dependencies管理
    void addDependency(const std::string& paramPath, const std::string& dependencyPath);
    void removeDependency(const std::string& paramPath, const std::string& dependencyPath);
    std::vector<std::string> getDependentParameters(const std::string& paramPath) const;

    // Event callbacks
    using ParameterChangeCallback = std::function<void(const ParameterChangeEvent&)>;
    int registerChangeCallback(ParameterChangeCallback callback);
    void unregisterChangeCallback(int callbackId);
    void notifyParameterChange(const ParameterChangeEvent& event);

    // Serialization
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    // Validation
    bool validateAllParameters() const;
    std::vector<std::string> getValidationErrors() const;

private:
    std::shared_ptr<ParameterNode> m_root;
    std::unordered_map<int, ParameterChangeCallback> m_callbacks;
    int m_nextCallbackId;
    mutable std::mutex m_treeMutex;

    // Internal helper methods
    std::shared_ptr<ParameterNode> findOrCreateNode(const std::string& path);
    std::vector<std::string> splitPath(const std::string& path) const;
    void validatePath(const std::string& path) const;
    void updateDependentParameters(const std::string& changedPath);
};

/**
 * @brief Parameter tree factory
 * Provides predefined parameter tree structure creation functionality
 */
class ParameterTreeFactory {
public:
    // Geometry representation parameters
    static std::shared_ptr<UnifiedParameterTree> createGeometryParameterTree();
    
    // Rendering control parameters
    static std::shared_ptr<UnifiedParameterTree> createRenderingParameterTree();
    
    // Mesh parameters
    static std::shared_ptr<UnifiedParameterTree> createMeshParameterTree();
    
    // Lighting parameters
    static std::shared_ptr<UnifiedParameterTree> createLightingParameterTree();
    
    // Complete parameter tree (includes all parameters)
    static std::shared_ptr<UnifiedParameterTree> createCompleteParameterTree();

private:
    static void addGeometryParameters(std::shared_ptr<UnifiedParameterTree> tree);
    static void addRenderingParameters(std::shared_ptr<UnifiedParameterTree> tree);
    static void addMeshParameters(std::shared_ptr<UnifiedParameterTree> tree);
    static void addLightingParameters(std::shared_ptr<UnifiedParameterTree> tree);
};