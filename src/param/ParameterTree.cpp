#include "param/ParameterTree.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <json/json.h>

// ParameterNode 实现
ParameterNode::ParameterNode(const std::string& name, ParameterNodeType type, ParameterNode* parent)
    : m_name(name), m_type(type), m_parent(parent) {
    updatePath();
}

void ParameterNode::addChild(std::shared_ptr<ParameterNode> child) {
    std::lock_guard<std::mutex> lock(m_childrenMutex);
    child->m_parent = this;
    child->updatePath();
    m_children[child->getName()] = child;
}

void ParameterNode::removeChild(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_childrenMutex);
    auto it = m_children.find(name);
    if (it != m_children.end()) {
        it->second->m_parent = nullptr;
        m_children.erase(it);
    }
}

std::shared_ptr<ParameterNode> ParameterNode::getChild(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_childrenMutex);
    auto it = m_children.find(name);
    return (it != m_children.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<ParameterNode>> ParameterNode::getChildren() const {
    std::lock_guard<std::mutex> lock(m_childrenMutex);
    std::vector<std::shared_ptr<ParameterNode>> children;
    children.reserve(m_children.size());
    for (const auto& pair : m_children) {
        children.push_back(pair.second);
    }
    return children;
}

bool ParameterNode::hasChild(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_childrenMutex);
    return m_children.find(name) != m_children.end();
}

std::string ParameterNode::getFullPath() const {
    if (m_parent == nullptr) {
        return m_name;
    }
    return m_parent->getFullPath() + "/" + m_name;
}

std::vector<std::string> ParameterNode::parsePath(const std::string& path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

void ParameterNode::updatePath() {
    m_path = getFullPath();
}

// Parameter 实现
Parameter::Parameter(const std::string& name, const ParameterValue& defaultValue, ParameterNode* parent)
    : ParameterNode(name, ParameterNodeType::Parameter, parent)
    , m_value(defaultValue)
    , m_defaultValue(defaultValue) {
}

void Parameter::setValue(const ParameterValue& value) {
    std::lock_guard<std::mutex> lock(m_valueMutex);
    
    if (m_validator && !m_validator(value)) {
        return; // 验证失败，不设置值
    }
    
    m_value = value;
    notifyChanged();
}

void Parameter::resetToDefault() {
    setValue(m_defaultValue);
}

void Parameter::addChangedCallback(ParameterChangedCallback callback) {
    std::lock_guard<std::mutex> lock(m_valueMutex);
    m_callbacks.push_back(callback);
}

void Parameter::removeChangedCallback(ParameterChangedCallback callback) {
    std::lock_guard<std::mutex> lock(m_valueMutex);
    m_callbacks.erase(
        std::remove_if(m_callbacks.begin(), m_callbacks.end(),
            [&callback](const ParameterChangedCallback& cb) {
                return cb.target_type() == callback.target_type();
            }),
        m_callbacks.end()
    );
}

void Parameter::notifyChanged() {
    for (const auto& callback : m_callbacks) {
        callback(getFullPath(), m_value);
    }
}

void Parameter::setValidator(std::function<bool(const ParameterValue&)> validator) {
    std::lock_guard<std::mutex> lock(m_valueMutex);
    m_validator = validator;
}

bool Parameter::validate(const ParameterValue& value) const {
    std::lock_guard<std::mutex> lock(m_valueMutex);
    return !m_validator || m_validator(value);
}

// ParameterTree 实现
ParameterTree& ParameterTree::getInstance() {
    static ParameterTree instance;
    return instance;
}

ParameterTree::ParameterTree() 
    : m_inBatchUpdate(false) {
    m_root = std::make_shared<ParameterNode>("root", ParameterNodeType::Root);
}

std::shared_ptr<ParameterNode> ParameterTree::findNode(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    
    auto parts = ParameterNode::parsePath(path);
    std::shared_ptr<ParameterNode> current = m_root;
    
    for (const auto& part : parts) {
        current = current->getChild(part);
        if (!current) {
            return nullptr;
        }
    }
    
    return current;
}

std::shared_ptr<Parameter> ParameterTree::findParameter(const std::string& path) const {
    auto node = findNode(path);
    if (node && node->getType() == ParameterNodeType::Parameter) {
        return std::static_pointer_cast<Parameter>(node);
    }
    return nullptr;
}

std::shared_ptr<Parameter> ParameterTree::registerParameter(
    const std::string& path, 
    const ParameterValue& defaultValue,
    std::function<bool(const ParameterValue&)> validator) {
    
    std::lock_guard<std::mutex> lock(m_treeMutex);
    
    // 检查参数是否已存在
    auto existing = findParameter(path);
    if (existing) {
        return existing;
    }
    
    // 创建路径
    auto parts = ParameterNode::parsePath(path);
    if (parts.empty()) {
        return nullptr;
    }
    
    std::shared_ptr<ParameterNode> current = m_root;
    
    // 创建中间节点
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        auto child = current->getChild(parts[i]);
        if (!child) {
            child = std::make_shared<ParameterNode>(parts[i], ParameterNodeType::Category);
            current->addChild(child);
        }
        current = child;
    }
    
    // 创建参数节点
    auto param = std::make_shared<Parameter>(parts.back(), defaultValue, current.get());
    param->setValidator(validator);
    current->addChild(param);
    
    return param;
}

bool ParameterTree::setParameterValue(const std::string& path, const ParameterValue& value) {
    auto param = findParameter(path);
    if (!param) {
        return false;
    }
    
    param->setValue(value);
    
    if (m_inBatchUpdate) {
        m_batchChangedPaths.push_back(path);
    } else {
        notifyParameterChanged(path, value);
    }
    
    return true;
}

ParameterValue ParameterTree::getParameterValue(const std::string& path) const {
    auto param = findParameter(path);
    return param ? param->getValue() : ParameterValue{};
}

bool ParameterTree::hasParameter(const std::string& path) const {
    return findParameter(path) != nullptr;
}

void ParameterTree::beginBatchUpdate() {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    m_inBatchUpdate = true;
    m_batchChangedPaths.clear();
}

void ParameterTree::endBatchUpdate() {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    m_inBatchUpdate = false;
    
    if (!m_batchChangedPaths.empty()) {
        notifyBatchUpdate(m_batchChangedPaths);
        m_batchChangedPaths.clear();
    }
}

void ParameterTree::setBatchUpdateCallback(BatchUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    m_batchUpdateCallback = callback;
}

void ParameterTree::addGlobalChangedCallback(ParameterChangedCallback callback) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    m_globalCallbacks.push_back(callback);
}

void ParameterTree::removeGlobalChangedCallback(ParameterChangedCallback callback) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    m_globalCallbacks.erase(
        std::remove_if(m_globalCallbacks.begin(), m_globalCallbacks.end(),
            [&callback](const ParameterChangedCallback& cb) {
                return cb.target_type() == callback.target_type();
            }),
        m_globalCallbacks.end()
    );
}

std::vector<std::string> ParameterTree::getAllParameterPaths() const {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    std::vector<std::string> paths;
    
    std::function<void(std::shared_ptr<ParameterNode>)> collectPaths = 
        [&](std::shared_ptr<ParameterNode> node) {
            if (node->getType() == ParameterNodeType::Parameter) {
                paths.push_back(node->getFullPath());
            }
            for (auto child : node->getChildren()) {
                collectPaths(child);
            }
        };
    
    collectPaths(m_root);
    return paths;
}

std::vector<std::string> ParameterTree::getParameterPathsInCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    std::vector<std::string> paths;
    
    auto categoryNode = findNode(category);
    if (!categoryNode) {
        return paths;
    }
    
    std::function<void(std::shared_ptr<ParameterNode>)> collectPaths = 
        [&](std::shared_ptr<ParameterNode> node) {
            if (node->getType() == ParameterNodeType::Parameter) {
                paths.push_back(node->getFullPath());
            }
            for (auto child : node->getChildren()) {
                collectPaths(child);
            }
        };
    
    collectPaths(categoryNode);
    return paths;
}

void ParameterTree::notifyParameterChanged(const std::string& path, const ParameterValue& value) {
    for (const auto& callback : m_globalCallbacks) {
        callback(path, value);
    }
}

void ParameterTree::notifyBatchUpdate(const std::vector<std::string>& changedPaths) {
    if (m_batchUpdateCallback) {
        m_batchUpdateCallback(changedPaths);
    }
}

std::string ParameterTree::serializeToJson() const {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    Json::Value root;
    
    std::function<void(std::shared_ptr<ParameterNode>, Json::Value&)> serializeNode = 
        [&](std::shared_ptr<ParameterNode> node, Json::Value& jsonNode) {
            if (node->getType() == ParameterNodeType::Parameter) {
                auto param = std::static_pointer_cast<Parameter>(node);
                // 这里需要根据ParameterValue的类型进行序列化
                // 简化实现，实际应用中需要完整的序列化支持
                jsonNode["value"] = "serialized_value";
            } else {
                Json::Value children(Json::objectValue);
                for (auto child : node->getChildren()) {
                    serializeNode(child, children[child->getName()]);
                }
                jsonNode = children;
            }
        };
    
    serializeNode(m_root, root);
    
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}

bool ParameterTree::deserializeFromJson(const std::string& json) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::istringstream stream(json);
    if (!Json::parseFromStream(builder, stream, &root, &errors)) {
        return false;
    }
    
    // 这里需要实现反序列化逻辑
    // 简化实现，实际应用中需要完整的反序列化支持
    
    return true;
}