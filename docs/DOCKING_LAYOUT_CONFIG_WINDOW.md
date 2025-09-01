# Docking 布局配置窗口

## 功能概述

实现了一个完整的布局配置窗口，允许用户自定义 dock 区域的大小和显示选项。

## 主要特性

### 1. 配置结构 (DockLayoutConfig)

```cpp
struct DockLayoutConfig {
    // 像素大小设置
    int topAreaHeight = 150;
    int bottomAreaHeight = 200;
    int leftAreaWidth = 250;
    int rightAreaWidth = 250;
    int centerMinWidth = 400;
    int centerMinHeight = 300;
    
    // 百分比设置
    bool usePercentage = false;
    int topAreaPercent = 20;
    int bottomAreaPercent = 25;
    int leftAreaPercent = 20;
    int rightAreaPercent = 20;
    
    // 其他选项
    int minAreaSize = 100;
    int splitterWidth = 4;
    bool showTopArea = true;
    bool showBottomArea = true;
    bool showLeftArea = true;
    bool showRightArea = true;
    bool enableAnimation = true;
    int animationDuration = 200;
};
```

### 2. 配置对话框 (DockLayoutConfigDialog)

采用多标签页设计，组织各种设置：

#### Sizes 标签页
- 切换像素/百分比模式
- 设置各区域的大小
- 设置中心区域最小尺寸

#### Visibility 标签页
- 控制各区域的显示/隐藏
- 可选择显示哪些 dock 区域

#### Options 标签页
- 最小区域大小设置
- 分割条宽度设置
- 动画效果开关和时长

### 3. 实时预览 (DockLayoutPreview)

- 显示布局的实时预览
- 不同颜色表示不同区域：
  - Top: 浅蓝色
  - Bottom: 浅绿色
  - Left: 浅红色
  - Right: 浅黄色
  - Center: 浅灰色
- 预览会根据配置实时更新

### 4. 集成到应用程序

#### 菜单项
在 View 菜单中添加了 "Configure Layout..." 选项

#### 使用方式
```cpp
void OnConfigureLayout(wxCommandEvent&) {
    DockLayoutConfig config = m_dockManager->getLayoutConfig();
    DockLayoutConfigDialog dlg(this, config);
    
    if (dlg.ShowModal() == wxID_OK) {
        config = dlg.GetConfig();
        m_dockManager->setLayoutConfig(config);
    }
}
```

### 5. 配置持久化

- 配置自动保存到 wxConfig
- 程序启动时自动加载上次的配置
- 提供重置为默认值功能

### 6. 动态应用配置

DockContainerWidget 中的所有固定大小值都替换为动态获取：

```cpp
// 替代原来的固定值 250
rootSplitter->SetSashPosition(getConfiguredAreaSize(area));

// 支持像素和百分比两种模式
int getConfiguredAreaSize(DockWidgetArea area) const {
    if (config.usePercentage) {
        // 基于容器大小计算百分比
        return containerSize * config.areaPercent / 100;
    } else {
        // 直接使用像素值
        return config.areaPixelSize;
    }
}
```

## 使用流程

1. 用户通过菜单打开配置窗口
2. 在不同标签页中调整设置
3. 实时预览显示效果
4. 点击 OK 保存配置
5. 配置在下次布局重建时生效

## 优点

1. **灵活性**: 支持像素和百分比两种模式
2. **直观性**: 实时预览让用户立即看到效果
3. **易用性**: 组织良好的界面，分类清晰
4. **持久性**: 配置自动保存和加载
5. **可扩展性**: 易于添加新的配置选项