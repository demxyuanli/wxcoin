# 轮廓设置对话框预览功能实现

## 功能概述

在OutlineSettingsDialog中添加了实时预览功能，用户可以：
1. 在调整参数时实时看到轮廓效果
2. 通过鼠标拖拽旋转预览模型
3. 直观地了解各参数对轮廓效果的影响

## 实现方案

### 1. 新增组件

#### OutlinePreviewCanvas
- 继承自wxGLCanvas的预览视图组件
- 包含基本的3D场景渲染功能
- 支持鼠标交互（旋转）

#### 预览场景内容
- 立方体（Cube）
- 球体（Sphere）
- 圆柱体（Cylinder）
- 圆锥体（Cone）

### 2. 对话框布局

使用wxSplitterWindow将对话框分为两部分：

**左侧 - 参数控制面板**
- 标题和分隔线
- 6个参数滑块，每个包含：
  - 参数名称标签
  - 滑动条（实时更新）
  - 当前值显示
- 参数说明文本
- 底部按钮（Reset/OK/Cancel）

**右侧 - 预览面板**
- 标题"Preview"
- 3D预览视图
- 操作提示文本

### 3. 轮廓渲染实现

由于预览环境的限制，使用简化的轮廓渲染方法：

```cpp
// 第一遍：渲染线框轮廓
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glLineWidth(thickness * 2.0f);
glPolygonOffset(-1.0f, -1.0f);  // 避免Z-fighting

// 第二遍：渲染实体模型
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
```

### 4. 参数实时更新

```cpp
void onSliderChange(wxCommandEvent& event) {
    updateLabels();    // 更新数值显示
    updatePreview();   // 更新预览效果
}
```

### 5. 交互功能

- **鼠标左键拖拽**：旋转模型
- **参数滑块**：实时调整轮廓参数
- **Reset按钮**：恢复默认参数

## 参数说明

| 参数 | 范围 | 默认值 | 说明 |
|-----|------|--------|------|
| Depth Weight | 0.0-2.0 | 1.5 | 深度边缘检测权重 |
| Normal Weight | 0.0-2.0 | 1.0 | 法线边缘检测权重 |
| Depth Threshold | 0.0-0.05 | 0.001 | 深度变化阈值 |
| Normal Threshold | 0.0-2.0 | 0.4 | 法线角度阈值 |
| Edge Intensity | 0.0-2.0 | 1.0 | 轮廓强度 |
| Thickness | 0.1-4.0 | 1.5 | 轮廓线宽 |

## 使用方式

1. 打开轮廓设置对话框
2. 调整左侧参数滑块
3. 在右侧预览窗口实时查看效果
4. 鼠标拖拽旋转查看不同角度
5. 点击Reset恢复默认值
6. 点击OK应用设置

## 技术细节

### OpenGL上下文管理
```cpp
m_glContext = new wxGLContext(this);
SetCurrent(*m_glContext);
```

### 场景图结构
```
SceneRoot
├── Camera (PerspectiveCamera)
├── Light (DirectionalLight)
└── ModelRoot
    ├── RotationXYZ (动画旋转)
    ├── Cube
    ├── Sphere
    ├── Cylinder
    └── Cone
```

### 性能优化
- 使用m_needsRedraw标志避免不必要的重绘
- 仅在参数变化时更新渲染
- 使用wxIdleEvent进行渲染调度

## 优势

1. **直观性**：用户可以立即看到参数调整的效果
2. **交互性**：支持模型旋转，多角度查看
3. **效率**：避免在主视图中反复测试参数
4. **学习性**：帮助用户理解各参数的作用

## 后续改进建议

1. 添加更多预览模型选项
2. 支持加载自定义模型
3. 添加不同的背景颜色选项
4. 实现更精确的轮廓渲染算法
5. 添加预设参数组合