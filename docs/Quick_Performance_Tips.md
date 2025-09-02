# 快速性能优化提示

## 🚀 快速解决方案

### 轻微卡顿
```
工具栏 → Mesh Quality → Deflection 滑块拖到 1.0 → Apply
```

### 中度卡顿
```
1. 工具栏 → Mesh Quality
2. Deflection 设为 1.5
3. 勾选 Enable LOD
4. Apply
```

### 严重卡顿
```
1. 工具栏 → Mesh Quality
2. Deflection 设为 2.0
3. 勾选 Enable LOD
4. Rough Deflection 设为 3.0
5. 勾选 Enable Parallel Processing
6. Apply
```

## 📊 参数速查表

| 性能问题程度 | Deflection | LOD | Rough Deflection | 预期提升 |
|------------|------------|-----|-----------------|---------|
| 轻微       | 1.0        | 关  | -               | 2x      |
| 中度       | 1.5        | 开  | 2.0             | 3-4x    |
| 严重       | 2.0        | 开  | 3.0             | 5-8x    |
| 极限优化   | 3.0+       | 开  | 5.0             | 10x+    |

## ⚡ 一键优化脚本

在控制台中执行（开发者模式）：
```javascript
// 性能模式
viewer.setMeshDeflection(2.0, true);
viewer.setLODEnabled(true);
viewer.setLODRoughDeflection(3.0);

// 质量模式
viewer.setMeshDeflection(0.2, true);
viewer.setLODEnabled(true);
viewer.setLODRoughDeflection(0.5);
```

## 🎯 特定场景优化

### 大装配体
- Deflection: 1.5-2.0
- LOD: 必须开启
- Subdivision: 关闭
- Parallel Processing: 开启

### 曲面零件
- Deflection: 0.5-1.0
- LOD: 开启
- Subdivision: 可选开启（Level 2）
- Smoothing: 开启

### 机械零件
- Deflection: 1.0
- LOD: 可选
- Feature Preservation: 0.8
- Crease Angle: 45°

## 💡 专业提示

1. **导入前设置**：在导入STEP文件前先调整默认网格参数
2. **批量处理**：对多个零件使用相同设置时，使用"Apply to All"
3. **性能监控**：按F12打开性能监控面板
4. **渐进式加载**：先用粗糙网格快速查看，再逐步细化

## ⚠️ 注意事项

- 过高的Deflection值会影响尺寸测量精度
- LOD切换可能在某些显卡上有延迟
- Subdivision会大幅增加内存使用
- 保存文件前考虑是否需要保留网格设置