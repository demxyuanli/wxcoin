# 高级轮廓设置实现

## 改进内容

### 1. 扩大界面尺寸
- **对话框尺寸**: 从 800x600 扩大到 1200x800
- **预览视口**: 设置最小尺寸为 500x500，默认 600x600
- **分割面板**: 控制面板宽度从 350 增加到 450
- **最小面板尺寸**: 从 300 增加到 400

### 2. 颜色配置选项

#### 新增的颜色设置：
1. **背景颜色** (Background Color)
   - 默认: RGB(51, 51, 51) - 深灰色
   - 控制预览视口的背景色

2. **轮廓颜色** (Outline Color)
   - 默认: RGB(0, 0, 0) - 黑色
   - 非悬停状态下的轮廓颜色

3. **悬停颜色** (Hover Color)
   - 默认: RGB(255, 128, 0) - 橙色
   - 鼠标悬停时的轮廓颜色

4. **几何体颜色** (Geometry Color)
   - 默认: RGB(200, 200, 200) - 浅灰色
   - 3D模型的材质颜色

### 3. 实现细节

#### ExtendedOutlineParams 结构体
```cpp
struct ExtendedOutlineParams : public ImageOutlineParams {
    wxColour backgroundColor{ 51, 51, 51 };
    wxColour outlineColor{ 0, 0, 0 };
    wxColour hoverColor{ 255, 128, 0 };
    wxColour geometryColor{ 200, 200, 200 };
};
```

#### 颜色选择器
- 使用 `wxColourPickerCtrl` 控件
- 实时更新预览
- 每个颜色选择器都有标签说明

#### 动态更新
- 背景色：立即更新 OpenGL 清除颜色
- 轮廓色/悬停色：立即更新渲染时使用的颜色
- 几何体色：重建模型以应用新的材质颜色

### 4. 用户界面布局

```
┌─────────────────────────────────────────────────┐
│                Outline Settings                  │
├─────────────────┬───────────────────────────────┤
│                 │                               │
│ Outline Params  │        Preview                │
│ ─────────────   │   ┌───────────────┐          │
│ Depth Weight    │   │               │          │
│ Normal Weight   │   │   3D Models   │          │
│ Depth Threshold │   │               │          │
│ Normal Threshold│   │               │          │
│ Edge Intensity  │   └───────────────┘          │
│ Thickness       │                               │
│                 │                               │
│ Color Settings  │                               │
│ ─────────────   │                               │
│ Background  [■] │                               │
│ Outline     [■] │                               │
│ Hover       [■] │                               │
│ Geometry    [■] │                               │
│                 │                               │
│ [Reset] [OK][X] │                               │
└─────────────────┴───────────────────────────────┘
```

### 5. 特性

1. **实时预览**
   - 所有参数改变立即反映在预览中
   - 颜色改变无需重启

2. **悬停效果**
   - 鼠标悬停在模型上时显示不同颜色
   - 支持自定义悬停颜色

3. **材质更新**
   - 几何体颜色改变时自动重建模型
   - 保持其他设置不变

4. **用户友好**
   - 清晰的参数分组
   - 直观的颜色选择器
   - 实时数值显示

## 使用方法

1. 点击 "Outline Settings" 按钮打开设置对话框
2. 调整滑块控制轮廓检测参数
3. 使用颜色选择器自定义颜色方案
4. 在预览窗口中实时查看效果
5. 鼠标悬停在预览模型上查看悬停效果
6. 点击 "OK" 应用设置

## 技术要点

- 使用 `wxColourPickerCtrl` 进行颜色选择
- OpenGL 颜色值需要从 0-255 转换为 0.0-1.0
- 几何体颜色改变需要重建 Coin3D 场景图
- 所有颜色设置都存储在 `ExtendedOutlineParams` 中