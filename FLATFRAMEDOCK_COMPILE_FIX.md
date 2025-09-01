# FlatFrameDock 编译错误修复

## 错误描述
```
error C2065: "m_outputDock": 未声明的标识符
```

## 原因
在重构过程中，我们将 `m_outputDock` 重命名为 `m_messageDock`，但在析构函数或重置函数中仍然使用了旧的变量名。

## 修复
在 `FlatFrameDocking.cpp` 中，将所有 `m_outputDock` 替换为 `m_messageDock`：

```cpp
// 错误的代码
m_outputDock = nullptr;

// 修正后的代码
m_messageDock = nullptr;
m_performanceDock = nullptr;  // 同时添加新的面板
```

## 完整的清理代码
```cpp
// Clear references
m_propertyDock = nullptr;
m_objectTreeDock = nullptr;
m_canvasDock = nullptr;
m_messageDock = nullptr;      // 原 m_outputDock
m_performanceDock = nullptr;  // 新增
m_toolboxDock = nullptr;      // 保留用于兼容性
```

## 注意事项
1. 确保所有旧变量名都已更新
2. 新增的 `m_performanceDock` 也需要在相应位置初始化和清理
3. `m_toolboxDock` 虽然不再使用，但为了向后兼容性仍然保留