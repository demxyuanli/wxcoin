# 移除旧 Docking 系统方案

## 当前状况分析

### 1. 基类 FlatFrame
- 声明了 `wxAuiManager m_auiManager` 但似乎未使用
- 有 `m_mainSplitter` 和 `m_leftSplitter` 成员变量
- 没有直接创建 Canvas、PropertyPanel、ObjectTreePanel

### 2. FlatFrameDocking 
- 完全使用新的 docking 系统（ads::DockManager）
- 自己创建所有面板（Canvas、PropertyPanel 等）
- 不依赖基类的布局系统

## 清理方案

### 第一步：确认基类中未使用的组件

需要从 FlatFrame 中移除：
1. `wxAuiManager m_auiManager` - 未使用的 AUI 管理器
2. `m_mainSplitter` 和 `m_leftSplitter` - 如果只在旧布局中使用

### 第二步：修改 FlatFrame.h

```cpp
// 移除这些：
// #include <wx/aui/aui.h>
// wxAuiManager m_auiManager;
// wxSplitterWindow* m_mainSplitter;  // 如果未使用
// wxSplitterWindow* m_leftSplitter;  // 如果未使用
```

### 第三步：检查影响

需要确认：
1. 其他类是否依赖 FlatFrame 的这些成员
2. 是否有其他使用 wxAuiManager 的代码
3. MainApplication.cpp 和 MainApplicationDocking.cpp 的区别

## 推荐的实施步骤

### 1. 创建纯净的基类

创建新的基类 `FlatFrameBase`，只包含共同功能：
- Ribbon UI
- 命令管理
- 主题管理
- 基本事件处理

### 2. 让 FlatFrameDocking 继承新基类

```cpp
class FlatFrameDocking : public FlatFrameBase {
    // 完全使用新 docking 系统
};
```

### 3. 逐步迁移

如果需要保留旧版本：
```cpp
class FlatFrameLegacy : public FlatFrameBase {
    // 使用旧的布局系统
};
```

## 具体修改建议

### A. 最小化修改（推荐）

1. 在 FlatFrame.h 中注释掉未使用的成员：
```cpp
// wxAuiManager m_auiManager;  // Not used in docking version
// wxSplitterWindow* m_mainSplitter;  // Not used in docking version
// wxSplitterWindow* m_leftSplitter;  // Not used in docking version
```

2. 在 FlatFrameDocking 中确保完全独立：
- 不调用基类的布局方法
- 自己管理所有面板

### B. 完全重构

1. 创建接口类：
```cpp
class IFlatFrame {
public:
    virtual Canvas* GetCanvas() = 0;
    virtual PropertyPanel* GetPropertyPanel() = 0;
    virtual ObjectTreePanel* GetObjectTreePanel() = 0;
    // ... 其他共同接口
};
```

2. 两种实现：
- `FlatFrameDocking : public wxFrame, public IFlatFrame`
- `FlatFrameLegacy : public wxFrame, public IFlatFrame`

## 检查清单

- [ ] 确认 m_auiManager 在基类中完全未使用
- [ ] 确认 splitter 是否在其他地方使用
- [ ] 检查所有派生类
- [ ] 测试移除后的编译和运行
- [ ] 更新相关文档

## 风险评估

**低风险**：如果 wxAuiManager 确实未使用，移除它是安全的
**中风险**：splitter 可能在某些功能中使用
**需要测试**：确保所有功能正常工作