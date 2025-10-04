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
 * @brief Parameter value type definition
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
 * @brief Parameter change callback function type
 */
using ParameterChangedCallback = std::function<void(const std::string&, const ParameterValue&)>;

/**
 * @brief Batch update callback function type
 */
using BatchUpdateCallback = std::function<void(const std::vector<std::string>&)>;

/**
 * @brief Parameter node type enumeration
 */
enum class ParameterNodeType {
    Root,           // Root node
    Category,       // Category node
    Group,          // Group node
    Parameter       // Parameter node
};

/**
 * @brief Base class for parameter nodes
 */
class ParameterNode {
public:
    ParameterNode(const std::string& name, ParameterNodeType type, ParameterNode* parent = nullptr);
    virtual ~ParameterNode() = default;

    // Basic information
    const std::string& getName() const { return m_name; }
    ParameterNodeType getType() const { return m_type; }
    ParameterNode* getParent() const { return m_parent; }
    const std::string& getPath() const { return m_path; }

    // Child node management
    void addChild(std::shared_ptr<ParameterNode> child);
    void removeChild(const std::string& name);
    std::shared_ptr<ParameterNode> getChild(const std::string& name) const;
    std::vector<std::shared_ptr<ParameterNode>> getChildren() const;
    bool hasChild(const std::string& name) const;

    // Path operations
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
 * @brief Parameter node implementation
 */
class Parameter : public ParameterNode {
public:
    Parameter(const std::string& name, const ParameterValue& defaultValue, ParameterNode* parent = nullptr);
    
    // Parameter value operations
    const ParameterValue& getValue() const { return m_value; }
    void setValue(const ParameterValue& value);
    const ParameterValue& getDefaultValue() const { return m_defaultValue; }
    void resetToDefault();

    // Type information
    template<typename T>
    T getValueAs() const {
        return std::get<T>(m_value);
    }

    template<typename T>
    bool isType() const {
        return std::holds_alternative<T>(m_value);
    }

    // Callback management
    void addChangedCallback(ParameterChangedCallback callback);
    void removeChangedCallback(ParameterChangedCallback callback);
    void notifyChanged();

    // Validation
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
 * @brief Parameter tree manager
 */
class ParameterTree {
public:
    static ParameterTree& getInstance();

    // Tree structure management
    std::shared_ptr<ParameterNode> getRoot() const { return m_root; }
    std::shared_ptr<ParameterNode> findNode(const std::string& path) const;
    std::shared_ptr<Parameter> findParameter(const std::string& path) const;

    // Parameter registration
    std::shared_ptr<Parameter> registerParameter(
        const std::string& path, 
        const ParameterValue& defaultValue,
        std::function<bool(const ParameterValue&)> validator = nullptr
    );

    // Parameter operations
    bool setParameterValue(const std::string& path, const ParameterValue& value);
    ParameterValue getParameterValue(const std::string& path) const;
    bool hasParameter(const std::string& path) const;

    // Batch operations
    void beginBatchUpdate();
    void endBatchUpdate();
    void setBatchUpdateCallback(BatchUpdateCallback callback);

    // Callback management
    void addGlobalChangedCallback(ParameterChangedCallback callback);
    void removeGlobalChangedCallback(ParameterChangedCallback callback);

    // Path operations
    std::vector<std::string> getAllParameterPaths() const;
    std::vector<std::string> getParameterPathsInCategory(const std::string& category) const;

    // Serialization
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
 * @brief Parameter tree builder - used to initialize parameter tree structure
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