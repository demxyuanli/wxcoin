# 性能优化指南 - CADNav

当导入复杂的几何模型后出现性能问题时，您可以通过以下方法优化性能：

## 1. 网格质量对话框 (Mesh Quality Dialog)

### 访问方式
- 点击工具栏中的 **"Mesh Quality"** 按钮（网格图标）
- 或通过菜单：**Tools → Mesh Quality**

### 主要设置

#### 基础网格设置 (Basic Mesh Settings)
- **Deflection（偏差）**: 控制网格精度
  - 默认值：0.5
  - 性能优化：增加到 1.0 - 2.0
  - 高质量：减少到 0.1 - 0.3
  - 说明：值越大，网格越粗糙，性能越好

#### LOD设置 (Level of Detail)
- **Enable LOD**: 勾选以启用细节层次切换
- **Rough Deflection（粗糙偏差）**: 交互时使用
  - 默认值：1.0
  - 建议范围：0.5 - 2.0
- **Fine Deflection（精细偏差）**: 静止时使用
  - 默认值：0.5
  - 建议范围：0.2 - 1.0
- **Transition Time（过渡时间）**: 从粗糙到精细的切换时间
  - 默认值：500ms
  - 建议：200-1000ms

### 快速优化建议
1. **轻度卡顿**：将Deflection设置为 1.0
2. **中度卡顿**：将Deflection设置为 1.5，启用LOD
3. **严重卡顿**：将Deflection设置为 2.0，启用LOD，Rough Deflection设为 2.0

## 2. 细分功能 (Subdivision)

### 适用场景
- 需要平滑的曲面表现
- 原始模型面片较少但需要高质量渲染

### 设置方法
1. 在Mesh Quality Dialog中切换到 **Subdivision** 标签
2. 勾选 **Enable Subdivision**
3. 设置参数：
   - **Subdivision Level**: 细分级别（1-4）
     - 1：轻度细分，性能影响小
     - 2：中度细分（推荐）
     - 3-4：高度细分，仅用于高端硬件
   - **Method**: 选择细分算法
     - Catmull-Clark：适合四边形网格
     - Loop：适合三角形网格
   - **Crease Angle**: 保留锐边的角度阈值

### 注意事项
- 细分会增加面片数量，可能降低性能
- 建议先尝试调整Deflection参数

## 3. 高级优化选项

### 平滑功能 (Smoothing)
- 用于改善网格质量而不增加面片数
- **Iterations**: 平滑迭代次数（1-5）
- **Strength**: 平滑强度（0.1-1.0）

### 并行处理 (Parallel Processing)
- 勾选 **Enable Parallel Processing** 以利用多核CPU
- 对大型模型特别有效

### 自适应网格 (Adaptive Meshing)
- 根据曲率自动调整网格密度
- 在平坦区域使用较少面片，在曲面区域使用更多面片

## 4. 性能优化工作流程

### 步骤1：评估性能问题
- 观察FPS（帧率）
- 检查CPU使用率
- 注意鼠标操作的响应速度

### 步骤2：快速优化
1. 打开Mesh Quality Dialog
2. 将Deflection增加到 1.0
3. 启用LOD
4. 点击 **Apply** 测试效果

### 步骤3：精细调整
- 如果仍有卡顿，继续增加Deflection
- 调整LOD参数以平衡质量和性能
- 考虑启用并行处理

### 步骤4：特殊情况处理
- **超大模型**：使用极高的Deflection值（2.0-5.0）
- **精细零件**：保持较低的Deflection，使用LOD
- **装配体**：对不同零件使用不同的网格精度

## 5. 预设配置

### 性能优先
```
Deflection: 2.0
LOD Enabled: Yes
Rough Deflection: 3.0
Fine Deflection: 1.0
Parallel Processing: Yes
```

### 平衡模式
```
Deflection: 1.0
LOD Enabled: Yes
Rough Deflection: 1.5
Fine Deflection: 0.5
Parallel Processing: Yes
```

### 质量优先
```
Deflection: 0.2
LOD Enabled: Yes
Rough Deflection: 0.5
Fine Deflection: 0.1
Subdivision: Enabled (Level 2)
```

## 6. 故障排除

### 问题：导入后程序卡死几秒
**解决方案**：
- 在导入前先设置较高的Deflection值
- 使用 **File → Import Settings** 预设网格参数

### 问题：鼠标操作延迟
**解决方案**：
- 启用LOD并增加Rough Deflection
- 减少Transition Time以更快切换

### 问题：模型显示不清晰
**解决方案**：
- 适度降低Deflection值
- 使用Subdivision改善表面质量
- 启用抗锯齿

## 7. 最佳实践

1. **先粗后细**：先使用较高的Deflection值查看模型，再根据需要降低
2. **按需调整**：不同模型需要不同的设置
3. **保存预设**：为常用配置创建预设
4. **监控性能**：使用性能面板监控FPS和渲染时间

## 8. 键盘快捷键

- `Ctrl+M`: 打开Mesh Quality Dialog
- `L`: 切换LOD开关
- `Shift+L`: 强制使用粗糙模式
- `Ctrl+Shift+L`: 强制使用精细模式

通过合理使用这些功能，您可以在保持良好视觉质量的同时获得流畅的操作体验。