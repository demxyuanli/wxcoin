# 轮廓渲染工作方案

## 当前状态
- ✅ 黑色轮廓线可见（但不连贯）
- ❌ 橙色悬停轮廓不可见

## 问题分析

### 黑色轮廓不连贯的原因
线框模式(`GL_LINE`)会显示所有多边形边缘，包括：
- 物体内部的三角形边
- 曲面细分产生的边
- 不该显示的内部结构

### 橙色看不到的原因
之前的实现对整个场景设置颜色，而不是单个对象。

## 当前实现方案

### 算法：背面剔除轮廓
```
1. 清除深度缓冲
2. 渲染放大的背面（轮廓色）
3. 渲染正常的前面（覆盖中心）
```

### 特点
- 每个对象单独处理
- 悬停对象用橙色
- 其他对象用黑色
- 轮廓连续平滑

## 如果还有问题

### 1. 轮廓太细
增加缩放系数：
```cpp
float scale = 1.0f + (m_outlineParams.thickness * 0.01f);  // 改为0.01f或更大
```

### 2. 深度冲突
调整多边形偏移：
```cpp
glPolygonOffset(2.0f, 2.0f);  // 增大偏移
```

### 3. 看不到橙色
检查悬停检测：
- 确认`m_hoveredObjectIndex`正确更新
- 确认鼠标事件正确处理

## 技术要点

### 为什么这个方法更好
1. **真正的轮廓**：只显示物体边界
2. **连续平滑**：没有内部线条
3. **颜色分离**：每个对象可以有不同颜色

### 关键技术
- `glCullFace(GL_FRONT)`：只显示背面
- `glClear(GL_DEPTH_BUFFER_BIT)`：确保轮廓在后面
- 单独渲染每个对象：允许不同颜色

## 调试步骤

如果橙色还是看不到：
1. 在`glColor3f(1.0f, 0.5f, 0.0f)`后添加：
   ```cpp
   printf("Rendering orange outline for object %d\n", m_hoveredObjectIndex);
   ```

2. 检查悬停索引：
   ```cpp
   if (hasHover) {
       printf("Hover index: %d\n", m_hoveredObjectIndex);
   }
   ```

3. 验证对象数量：
   ```cpp
   printf("Model root children: %d\n", m_modelRoot->getNumChildren());
   ```