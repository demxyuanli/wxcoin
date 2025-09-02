# FlatFrameDocking 菜单调试指南

## 检查步骤

### 1. 确认菜单项已添加

在 FlatFrameDocking.cpp 中，我们已经添加了以下代码：

```cpp
// 第 493-494 行
viewMenu->Append(ID_DOCKING_CONFIGURE_LAYOUT, "&Configure Layout...",
    "Configure dock panel sizes and layout");
```

### 2. 确认事件处理已绑定

事件表中已添加（第 31 行）：
```cpp
EVT_MENU(ID_DOCKING_CONFIGURE_LAYOUT, FlatFrameDocking::OnDockingConfigureLayout)
```

事件处理函数已实现（第 599-618 行）：
```cpp
void FlatFrameDocking::OnDockingConfigureLayout(wxCommandEvent& event) {
    // ... 实现代码 ...
}
```

### 3. 可能的问题和解决方案

#### 问题 1: 基类可能没有菜单栏
**解决方案**: 已修复 - 现在会创建菜单栏如果不存在

#### 问题 2: View 菜单可能被其他代码覆盖
**检查方法**: 
1. 在 FlatFrameDocking 构造函数末尾添加断点
2. 检查 GetMenuBar() 返回值
3. 检查 View 菜单的内容

#### 问题 3: 菜单项可能被隐藏或禁用
**检查方法**:
1. 运行程序后，打开 View 菜单
2. 查看菜单底部是否有 "Configure Layout..." 选项
3. 应该在 "Toggle Auto-hide" 下方

### 4. 测试代码

在您的主程序中添加以下测试代码：

```cpp
// 在 FlatFrameDocking 构造函数末尾添加
wxMenuBar* mb = GetMenuBar();
if (mb) {
    int viewIndex = mb->FindMenu("View");
    if (viewIndex != wxNOT_FOUND) {
        wxMenu* viewMenu = mb->GetMenu(viewIndex);
        wxMenuItem* configItem = viewMenu->FindItem(ID_DOCKING_CONFIGURE_LAYOUT);
        if (configItem) {
            wxLogMessage("Configure Layout menu item found: %s", 
                        configItem->GetItemLabelText());
        } else {
            wxLogError("Configure Layout menu item NOT found!");
        }
    }
}
```

### 5. 手动触发配置对话框

如果菜单项不可见，可以添加一个工具栏按钮或快捷键：

```cpp
// 添加工具栏按钮
wxToolBar* toolbar = CreateToolBar();
toolbar->AddTool(ID_DOCKING_CONFIGURE_LAYOUT, "Config", 
                wxArtProvider::GetBitmap(wxART_PREFERENCES),
                "Configure Layout");
toolbar->Realize();
```

或添加快捷键：
```cpp
// 在 CreateDockingMenus() 中修改
viewMenu->Append(ID_DOCKING_CONFIGURE_LAYOUT, "&Configure Layout...\tCtrl+Alt+L",
    "Configure dock panel sizes and layout");
```

## 预期结果

运行 FlatFrameDocking 后，您应该能看到：
1. View 菜单中有 "Configure Layout..." 选项
2. 点击后打开配置对话框
3. 对话框显示布局预览和配置选项
4. 可以使用预设按钮快速设置布局
5. 点击 OK 或 Apply 后布局立即更新