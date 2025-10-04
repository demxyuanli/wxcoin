#include "param/UnifiedParameterTree.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

// ParameterNode 实现
ParameterNode::ParameterNode(const std::string& name, Type type)
    : m_name(name), m_type(type), m_parent(nullptr) {
    m_path = name;
}

void ParameterNode::addChild(std::shared_ptr<ParameterNode> child) {
    if (!child) return;
    
    child->setParent(this);
    child->setPath(getFullPath() + "." + child->getName());
    m_children[child->getName()] = child;
    
    LOG_DBG_S("ParameterNode: Added child '" + child->getName() + "' to '" + m_name + "'");
}

void ParameterNode::removeChild(const std::string& name) {
    auto it = m_children.find(name);
    if (it != m_children.end()) {
        it->second->setParent(nullptr);
        m_children.erase(it);
        LOG_DBG_S("ParameterNode: Removed child '" + name + "' from '" + m_name + "'");
    }
}

std::shared_ptr<ParameterNode> ParameterNode::getChild(const std::string& name) const {
    auto it = m_children.find(name);
    return (it != m_children.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<ParameterNode>> ParameterNode::getChildren() const {
    std::vector<std::shared_ptr<ParameterNode>> children;
    children.reserve(m_children.size());
    for (const auto& pair : m_children) {
        children.push_back(pair.second);
    }
    return children;
}

bool ParameterNode::hasChild(const std::string& name) const {
    return m_children.find(name) != m_children.end();
}

std::string ParameterNode::getFullPath() const {
    if (m_parent) {
        return m_parent->getFullPath() + "." + m_name;
    }
    return m_name;
}

std::vector<std::string> ParameterNode::getPathComponents() const {
    std::vector<std::string> components;
    std::string path = getFullPath();
    
    std::istringstream iss(path);
    std::string component;
    while (std::getline(iss, component, '.')) {
        components.push_back(component);
    }
    return components;
}

bool ParameterNode::setValue(const ParameterValue& value) {
    LOG_WRN_S("ParameterNode: setValue called on non-parameter node '" + m_name + "'");
    return false;
}

ParameterValue ParameterNode::getValue() const {
    LOG_WRN_S("ParameterNode: getValue called on non-parameter node '" + m_name + "'");
    return ParameterValue{};
}

void ParameterNode::addDependency(const std::string& paramPath) {
    m_dependencies.insert(paramPath);
    LOG_DBG_S("ParameterNode: Added dependency '" + paramPath + "' to '" + m_name + "'");
}

void ParameterNode::removeDependency(const std::string& paramPath) {
    m_dependencies.erase(paramPath);
    LOG_DBG_S("ParameterNode: Removed dependency '" + paramPath + "' from '" + m_name + "'");
}

void ParameterNode::updateChildrenPaths() {
    for (auto& pair : m_children) {
        pair.second->setPath(getFullPath() + "." + pair.second->getName());
        pair.second->updateChildrenPaths();
    }
}

// ParameterValueNode 实现
ParameterValueNode::ParameterValueNode(const std::string& name, const ParameterValue& defaultValue)
    : ParameterNode(name, Type::PARAMETER), m_value(defaultValue), m_defaultValue(defaultValue) {
}

bool ParameterValueNode::setValue(const ParameterValue& value) {
    std::lock_guard<std::mutex> lock(m_valueMutex);
    
    if (!validateValue(value)) {
        LOG_ERR_S("ParameterValueNode: Invalid value for parameter '" + m_name + "'");
        return false;
    }
    
    ParameterValue oldValue = m_value;
    m_value = value;
    
    LOG_DBG_S("ParameterValueNode: Set value for parameter '" + m_name + "'");
    return true;
}

ParameterValue ParameterValueNode::getValue() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_valueMutex));
    return m_value;
}

bool ParameterValueNode::validateValue(const ParameterValue& value) const {
    // 检查值范围限制
    if (std::holds_alternative<double>(value) && std::holds_alternative<double>(m_minValue)) {
        double val = std::get<double>(value);
        double min = std::get<double>(m_minValue);
        double max = std::get<double>(m_maxValue);
        if (val < min || val > max) {
            return false;
        }
    }
    
    // 检查类型匹配
    if (value.index() != m_defaultValue.index()) {
        return false;
    }
    
    return true;
}

// ParameterGroupNode 实现
ParameterGroupNode::ParameterGroupNode(const std::string& name, const std::string& description)
    : ParameterNode(name, Type::GROUP) {
    setDescription(description);
}

// UnifiedParameterTree 实现
UnifiedParameterTree::UnifiedParameterTree() : m_nextCallbackId(1) {
    m_root = std::make_shared<ParameterNode>("root", ParameterNode::Type::CONTAINER);
    LOG_INF_S("UnifiedParameterTree: Created parameter tree with root node");
}

UnifiedParameterTree::~UnifiedParameterTree() {
    LOG_INF_S("UnifiedParameterTree: Destroying parameter tree");
}

std::shared_ptr<ParameterNode> UnifiedParameterTree::getNode(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    
    if (path.empty() || path == "root") {
        return m_root;
    }
    
    std::vector<std::string> components = splitPath(path);
    std::shared_ptr<ParameterNode> current = m_root;
    
    for (const std::string& component : components) {
        if (!current) break;
        current = current->getChild(component);
    }
    
    return current;
}

std::shared_ptr<ParameterNode> UnifiedParameterTree::createNode(const std::string& path, ParameterNode::Type type) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    
    std::vector<std::string> components = splitPath(path);
    if (components.empty()) {
        LOG_ERR_S("UnifiedParameterTree: Invalid path for node creation: " + path);
        return nullptr;
    }
    
    std::shared_ptr<ParameterNode> current = m_root;
    std::string currentPath = "root";
    
    // 创建路径上的所有父节点
    for (size_t i = 0; i < components.size() - 1; ++i) {
        const std::string& component = components[i];
        auto child = current->getChild(component);
        
        if (!child) {
            child = std::make_shared<ParameterNode>(component, ParameterNode::Type::CONTAINER);
            current->addChild(child);
            LOG_DBG_S("UnifiedParameterTree: Created container node '" + component + "'");
        }
        
        current = child;
        currentPath += "." + component;
    }
    
    // 创建目标节点
    const std::string& targetName = components.back();
    auto existingChild = current->getChild(targetName);
    if (existingChild) {
        LOG_WRN_S("UnifiedParameterTree: Node already exists: " + path);
        return existingChild;
    }
    
    std::shared_ptr<ParameterNode> newNode;
    switch (type) {
        case ParameterNode::Type::PARAMETER:
            newNode = std::make_shared<ParameterValueNode>(targetName);
            break;
        case ParameterNode::Type::GROUP:
            newNode = std::make_shared<ParameterGroupNode>(targetName);
            break;
        default:
            newNode = std::make_shared<ParameterNode>(targetName, type);
            break;
    }
    
    current->addChild(newNode);
    LOG_INF_S("UnifiedParameterTree: Created node '" + path + "' of type " + std::to_string(static_cast<int>(type)));
    
    return newNode;
}

std::shared_ptr<ParameterValueNode> UnifiedParameterTree::createParameter(const std::string& path, const ParameterValue& defaultValue) {
    auto node = createNode(path, ParameterNode::Type::PARAMETER);
    if (auto paramNode = std::dynamic_pointer_cast<ParameterValueNode>(node)) {
        paramNode->setDefaultValue(defaultValue);
        paramNode->setValue(defaultValue);
        return paramNode;
    }
    return nullptr;
}

std::shared_ptr<ParameterGroupNode> UnifiedParameterTree::createGroup(const std::string& path, const std::string& description) {
    auto node = createNode(path, ParameterNode::Type::GROUP);
    if (auto groupNode = std::dynamic_pointer_cast<ParameterGroupNode>(node)) {
        groupNode->setDescription(description);
        return groupNode;
    }
    return nullptr;
}

bool UnifiedParameterTree::setParameterValue(const std::string& path, const ParameterValue& value) {
    auto node = getNode(path);
    if (!node) {
        LOG_ERR_S("UnifiedParameterTree: Parameter not found: " + path);
        return false;
    }
    
    auto paramNode = std::dynamic_pointer_cast<ParameterValueNode>(node);
    if (!paramNode) {
        LOG_ERR_S("UnifiedParameterTree: Node is not a parameter: " + path);
        return false;
    }
    
    ParameterValue oldValue = paramNode->getValue();
    bool success = paramNode->setValue(value);
    
    if (success) {
        ParameterChangeEvent event;
        event.path = path;
        event.oldValue = oldValue;
        event.newValue = value;
        event.timestamp = std::chrono::steady_clock::now();
        event.source = "UnifiedParameterTree";
        event.isBatchUpdate = false;
        
        notifyParameterChange(event);
    }
    
    return success;
}

ParameterValue UnifiedParameterTree::getParameterValue(const std::string& path) const {
    auto node = getNode(path);
    if (!node) {
        LOG_ERR_S("UnifiedParameterTree: Parameter not found: " + path);
        return ParameterValue{};
    }
    
    auto paramNode = std::dynamic_pointer_cast<ParameterValueNode>(node);
    if (!paramNode) {
        LOG_ERR_S("UnifiedParameterTree: Node is not a parameter: " + path);
        return ParameterValue{};
    }
    
    return paramNode->getValue();
}

bool UnifiedParameterTree::hasParameter(const std::string& path) const {
    auto node = getNode(path);
    return node && std::dynamic_pointer_cast<ParameterValueNode>(node) != nullptr;
}

bool UnifiedParameterTree::setParameterValues(const std::unordered_map<std::string, ParameterValue>& values) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    
    bool allSuccess = true;
    std::vector<ParameterChangeEvent> events;
    
    for (const auto& pair : values) {
        auto node = getNode(pair.first);
        if (!node) {
            LOG_ERR_S("UnifiedParameterTree: Parameter not found: " + pair.first);
            allSuccess = false;
            continue;
        }
        
        auto paramNode = std::dynamic_pointer_cast<ParameterValueNode>(node);
        if (!paramNode) {
            LOG_ERR_S("UnifiedParameterTree: Node is not a parameter: " + pair.first);
            allSuccess = false;
            continue;
        }
        
        ParameterValue oldValue = paramNode->getValue();
        bool success = paramNode->setValue(pair.second);
        
        if (success) {
            ParameterChangeEvent event;
            event.path = pair.first;
            event.oldValue = oldValue;
            event.newValue = pair.second;
            event.timestamp = std::chrono::steady_clock::now();
            event.source = "UnifiedParameterTree";
            event.isBatchUpdate = true;
            events.push_back(event);
        } else {
            allSuccess = false;
        }
    }
    
    // 通知批量更新事件
    for (const auto& event : events) {
        notifyParameterChange(event);
    }
    
    return allSuccess;
}

std::unordered_map<std::string, ParameterValue> UnifiedParameterTree::getParameterValues(const std::vector<std::string>& paths) const {
    std::unordered_map<std::string, ParameterValue> result;
    
    for (const std::string& path : paths) {
        result[path] = getParameterValue(path);
    }
    
    return result;
}

std::vector<std::string> UnifiedParameterTree::getAllParameterPaths() const {
    std::vector<std::string> paths;
    collectParameterPaths(m_root, "", paths);
    return paths;
}

std::vector<std::string> UnifiedParameterTree::getParameterPathsByTag(const std::string& tag) const {
    std::vector<std::string> paths;
    collectParameterPathsByTag(m_root, "", tag, paths);
    return paths;
}

std::vector<std::string> UnifiedParameterTree::getChildPaths(const std::string& parentPath) const {
    auto parentNode = getNode(parentPath);
    if (!parentNode) {
        return {};
    }
    
    std::vector<std::string> childPaths;
    auto children = parentNode->getChildren();
    
    for (const auto& child : children) {
        childPaths.push_back(child->getFullPath());
    }
    
    return childPaths;
}

void UnifiedParameterTree::addDependency(const std::string& paramPath, const std::string& dependencyPath) {
    auto node = getNode(paramPath);
    if (node) {
        node->addDependency(dependencyPath);
    }
}

void UnifiedParameterTree::removeDependency(const std::string& paramPath, const std::string& dependencyPath) {
    auto node = getNode(paramPath);
    if (node) {
        node->removeDependency(dependencyPath);
    }
}

std::vector<std::string> UnifiedParameterTree::getDependentParameters(const std::string& paramPath) const {
    std::vector<std::string> dependents;
    collectDependentParameters(m_root, paramPath, dependents);
    return dependents;
}

int UnifiedParameterTree::registerChangeCallback(ParameterChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    int id = m_nextCallbackId++;
    m_callbacks[id] = callback;
    LOG_DBG_S("UnifiedParameterTree: Registered change callback with ID " + std::to_string(id));
    return id;
}

void UnifiedParameterTree::unregisterChangeCallback(int callbackId) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    auto it = m_callbacks.find(callbackId);
    if (it != m_callbacks.end()) {
        m_callbacks.erase(it);
        LOG_DBG_S("UnifiedParameterTree: Unregistered change callback with ID " + std::to_string(callbackId));
    }
}

void UnifiedParameterTree::notifyParameterChange(const ParameterChangeEvent& event) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    
    for (const auto& pair : m_callbacks) {
        try {
            pair.second(event);
        } catch (const std::exception& e) {
            LOG_ERR_S("UnifiedParameterTree: Callback execution failed: " + std::string(e.what()));
        }
    }
    
    // 更新依赖参数
    updateDependentParameters(event.path);
}

bool UnifiedParameterTree::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERR_S("UnifiedParameterTree: Failed to open file for writing: " + filename);
        return false;
    }
    
    file << "# Unified Parameter Tree Configuration\n";
    file << "# Generated on " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";
    
    saveNodeToFile(m_root, file, 0);
    
    file.close();
    LOG_INF_S("UnifiedParameterTree: Saved parameter tree to " + filename);
    return true;
}

bool UnifiedParameterTree::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERR_S("UnifiedParameterTree: Failed to open file for reading: " + filename);
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 解析参数行
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string path = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 移除空白字符
            path.erase(0, path.find_first_not_of(" \t"));
            path.erase(path.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 这里需要根据值的类型进行解析
            // 简化实现，假设都是字符串类型
            ParameterValue paramValue = value;
            setParameterValue(path, paramValue);
        }
    }
    
    file.close();
    LOG_INF_S("UnifiedParameterTree: Loaded parameter tree from " + filename);
    return true;
}

bool UnifiedParameterTree::validateAllParameters() const {
    return validateNode(m_root);
}

std::vector<std::string> UnifiedParameterTree::getValidationErrors() const {
    std::vector<std::string> errors;
    collectValidationErrors(m_root, "", errors);
    return errors;
}

// 私有辅助方法
std::vector<std::string> UnifiedParameterTree::splitPath(const std::string& path) const {
    std::vector<std::string> components;
    std::istringstream iss(path);
    std::string component;
    
    while (std::getline(iss, component, '.')) {
        if (!component.empty()) {
            components.push_back(component);
        }
    }
    
    return components;
}

void UnifiedParameterTree::validatePath(const std::string& path) const {
    if (path.empty()) {
        throw std::invalid_argument("Parameter path cannot be empty");
    }
    
    // 检查路径格式
    std::regex pathRegex("^[a-zA-Z_][a-zA-Z0-9_]*(\\.[a-zA-Z_][a-zA-Z0-9_]*)*$");
    if (!std::regex_match(path, pathRegex)) {
        throw std::invalid_argument("Invalid parameter path format: " + path);
    }
}

void UnifiedParameterTree::updateDependentParameters(const std::string& changedPath) {
    auto dependents = getDependentParameters(changedPath);
    for (const std::string& dependentPath : dependents) {
        // 这里可以触发依赖参数的重新计算或更新
        LOG_DBG_S("UnifiedParameterTree: Updating dependent parameter: " + dependentPath);
    }
}

void UnifiedParameterTree::collectParameterPaths(std::shared_ptr<ParameterNode> node, const std::string& prefix, std::vector<std::string>& paths) const {
    if (!node) return;
    
    std::string currentPath = prefix.empty() ? node->getName() : prefix + "." + node->getName();
    
    if (node->getType() == ParameterNode::Type::PARAMETER) {
        paths.push_back(currentPath);
    }
    
    auto children = node->getChildren();
    for (const auto& child : children) {
        collectParameterPaths(child, currentPath, paths);
    }
}

void UnifiedParameterTree::collectParameterPathsByTag(std::shared_ptr<ParameterNode> node, const std::string& prefix, const std::string& tag, std::vector<std::string>& paths) const {
    if (!node) return;
    
    std::string currentPath = prefix.empty() ? node->getName() : prefix + "." + node->getName();
    
    if (node->getType() == ParameterNode::Type::PARAMETER) {
        auto tags = node->getTags();
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            paths.push_back(currentPath);
        }
    }
    
    auto children = node->getChildren();
    for (const auto& child : children) {
        collectParameterPathsByTag(child, currentPath, tag, paths);
    }
}

void UnifiedParameterTree::collectDependentParameters(std::shared_ptr<ParameterNode> node, const std::string& targetPath, std::vector<std::string>& dependents) const {
    if (!node) return;
    
    auto dependencies = node->getDependencies();
    if (dependencies.find(targetPath) != dependencies.end()) {
        dependents.push_back(node->getFullPath());
    }
    
    auto children = node->getChildren();
    for (const auto& child : children) {
        collectDependentParameters(child, targetPath, dependents);
    }
}

void UnifiedParameterTree::saveNodeToFile(std::shared_ptr<ParameterNode> node, std::ofstream& file, int indent) const {
    if (!node) return;
    
    std::string indentStr(indent * 2, ' ');
    
    if (node->getType() == ParameterNode::Type::PARAMETER) {
        auto paramNode = std::dynamic_pointer_cast<ParameterValueNode>(node);
        if (paramNode) {
            file << indentStr << node->getFullPath() << "=";
            // 这里需要根据参数值类型进行序列化
            // 简化实现
            file << "\n";
        }
    }
    
    auto children = node->getChildren();
    for (const auto& child : children) {
        saveNodeToFile(child, file, indent + 1);
    }
}

bool UnifiedParameterTree::validateNode(std::shared_ptr<ParameterNode> node) const {
    if (!node) return true;
    
    if (node->getType() == ParameterNode::Type::PARAMETER) {
        auto paramNode = std::dynamic_pointer_cast<ParameterValueNode>(node);
        if (paramNode) {
            return paramNode->validateValue(paramNode->getValue());
        }
    }
    
    auto children = node->getChildren();
    for (const auto& child : children) {
        if (!validateNode(child)) {
            return false;
        }
    }
    
    return true;
}

void UnifiedParameterTree::collectValidationErrors(std::shared_ptr<ParameterNode> node, const std::string& prefix, std::vector<std::string>& errors) const {
    if (!node) return;
    
    std::string currentPath = prefix.empty() ? node->getName() : prefix + "." + node->getName();
    
    if (node->getType() == ParameterNode::Type::PARAMETER) {
        auto paramNode = std::dynamic_pointer_cast<ParameterValueNode>(node);
        if (paramNode && !paramNode->validateValue(paramNode->getValue())) {
            errors.push_back("Invalid value for parameter: " + currentPath);
        }
    }
    
    auto children = node->getChildren();
    for (const auto& child : children) {
        collectValidationErrors(child, currentPath, errors);
    }
}

// ParameterTreeFactory 实现
std::shared_ptr<UnifiedParameterTree> ParameterTreeFactory::createGeometryParameterTree() {
    auto tree = std::make_shared<UnifiedParameterTree>();
    addGeometryParameters(tree);
    return tree;
}

std::shared_ptr<UnifiedParameterTree> ParameterTreeFactory::createRenderingParameterTree() {
    auto tree = std::make_shared<UnifiedParameterTree>();
    addRenderingParameters(tree);
    return tree;
}

std::shared_ptr<UnifiedParameterTree> ParameterTreeFactory::createMeshParameterTree() {
    auto tree = std::make_shared<UnifiedParameterTree>();
    addMeshParameters(tree);
    return tree;
}

std::shared_ptr<UnifiedParameterTree> ParameterTreeFactory::createLightingParameterTree() {
    auto tree = std::make_shared<UnifiedParameterTree>();
    addLightingParameters(tree);
    return tree;
}

std::shared_ptr<UnifiedParameterTree> ParameterTreeFactory::createCompleteParameterTree() {
    auto tree = std::make_shared<UnifiedParameterTree>();
    addGeometryParameters(tree);
    addRenderingParameters(tree);
    addMeshParameters(tree);
    addLightingParameters(tree);
    return tree;
}

void ParameterTreeFactory::addGeometryParameters(std::shared_ptr<UnifiedParameterTree> tree) {
    // 几何表示参数
    tree->createGroup("geometry", "几何表示参数");
    
    // 基本几何参数
    tree->createParameter("geometry.position.x", 0.0);
    tree->createParameter("geometry.position.y", 0.0);
    tree->createParameter("geometry.position.z", 0.0);
    
    tree->createParameter("geometry.rotation.x", 0.0);
    tree->createParameter("geometry.rotation.y", 0.0);
    tree->createParameter("geometry.rotation.z", 0.0);
    
    tree->createParameter("geometry.scale.x", 1.0);
    tree->createParameter("geometry.scale.y", 1.0);
    tree->createParameter("geometry.scale.z", 1.0);
    
    // 可见性参数
    tree->createParameter("geometry.visible", true);
    tree->createParameter("geometry.selected", false);
    
    LOG_INF_S("ParameterTreeFactory: Added geometry parameters");
}

void ParameterTreeFactory::addRenderingParameters(std::shared_ptr<UnifiedParameterTree> tree) {
    // 渲染控制参数
    tree->createGroup("rendering", "渲染控制参数");
    
    // 材质参数
    tree->createGroup("rendering.material", "材质参数");
    tree->createParameter("rendering.material.ambient.r", 0.6);
    tree->createParameter("rendering.material.ambient.g", 0.6);
    tree->createParameter("rendering.material.ambient.b", 0.6);
    
    tree->createParameter("rendering.material.diffuse.r", 0.8);
    tree->createParameter("rendering.material.diffuse.g", 0.8);
    tree->createParameter("rendering.material.diffuse.b", 0.8);
    
    tree->createParameter("rendering.material.specular.r", 1.0);
    tree->createParameter("rendering.material.specular.g", 1.0);
    tree->createParameter("rendering.material.specular.b", 1.0);
    
    tree->createParameter("rendering.material.shininess", 30.0);
    tree->createParameter("rendering.material.transparency", 0.0);
    
    // 显示模式参数
    tree->createGroup("rendering.display", "显示模式参数");
    tree->createParameter("rendering.display.mode", std::string("Solid"));
    tree->createParameter("rendering.display.showEdges", false);
    tree->createParameter("rendering.display.showVertices", false);
    tree->createParameter("rendering.display.edgeWidth", 1.0);
    tree->createParameter("rendering.display.vertexSize", 2.0);
    
    LOG_INF_S("ParameterTreeFactory: Added rendering parameters");
}

void ParameterTreeFactory::addMeshParameters(std::shared_ptr<UnifiedParameterTree> tree) {
    // 网格参数
    tree->createGroup("mesh", "网格参数");
    
    // 基本网格参数
    tree->createParameter("mesh.deflection", 0.5);
    tree->createParameter("mesh.angularDeflection", 1.0);
    tree->createParameter("mesh.relative", false);
    tree->createParameter("mesh.inParallel", true);
    
    // 细分参数
    tree->createGroup("mesh.subdivision", "细分参数");
    tree->createParameter("mesh.subdivision.enabled", false);
    tree->createParameter("mesh.subdivision.levels", 2);
    
    // 平滑参数
    tree->createGroup("mesh.smoothing", "平滑参数");
    tree->createParameter("mesh.smoothing.enabled", false);
    tree->createParameter("mesh.smoothing.creaseAngle", 30.0);
    tree->createParameter("mesh.smoothing.iterations", 2);
    
    LOG_INF_S("ParameterTreeFactory: Added mesh parameters");
}

void ParameterTreeFactory::addLightingParameters(std::shared_ptr<UnifiedParameterTree> tree) {
    // 光照参数
    tree->createGroup("lighting", "光照参数");
    
    // 环境光参数
    tree->createGroup("lighting.ambient", "环境光参数");
    tree->createParameter("lighting.ambient.color.r", 0.7);
    tree->createParameter("lighting.ambient.color.g", 0.7);
    tree->createParameter("lighting.ambient.color.b", 0.7);
    tree->createParameter("lighting.ambient.intensity", 0.8);
    
    // 主光源参数
    tree->createGroup("lighting.main", "主光源参数");
    tree->createParameter("lighting.main.enabled", true);
    tree->createParameter("lighting.main.type", std::string("directional"));
    tree->createParameter("lighting.main.position.x", 0.0);
    tree->createParameter("lighting.main.position.y", 0.0);
    tree->createParameter("lighting.main.position.z", 0.0);
    tree->createParameter("lighting.main.direction.x", 0.5);
    tree->createParameter("lighting.main.direction.y", 0.5);
    tree->createParameter("lighting.main.direction.z", -1.0);
    tree->createParameter("lighting.main.color.r", 1.0);
    tree->createParameter("lighting.main.color.g", 1.0);
    tree->createParameter("lighting.main.color.b", 1.0);
    tree->createParameter("lighting.main.intensity", 1.0);
    
    LOG_INF_S("ParameterTreeFactory: Added lighting parameters");
}