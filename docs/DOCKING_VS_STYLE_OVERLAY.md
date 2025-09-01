# Visual Studio 风格的 Docking 方向指示器

## 设计特点

参照 Visual Studio 的停靠指示器风格，实现了以下特性：

### 1. 颜色方案

- **正常状态**：
  - 背景：半透明灰色 (177, 177, 177, 200)
  - 边框：深灰色 (142, 142, 142)
  - 图标：深灰色 (70, 70, 70)

- **高亮状态**：
  - 背景：半透明 VS 蓝色 (0, 122, 204, 200)
  - 边框：VS 蓝色 (0, 122, 204)
  - 图标：白色 (255, 255, 255)
  - 预览区域：非常透明的蓝色 (0, 122, 204, 60)

### 2. 布局设计

#### DockArea Overlay（围绕目标）
```
        [↑]
         |
    [←]--[□]--[→]
         |
        [↓]
```
- 四个方向指示器围绕中心指示器
- 间距适中，紧凑布局

#### Container Overlay（整个窗口）
```
         [↑]
         
[←]      [□]      [→]

         [↓]
```
- 四个方向指示器靠近窗口边缘
- 中心指示器在窗口中央
- 更大的间距，覆盖整个窗口

### 3. 图标设计

- **方向箭头**：
  - 使用填充的三角形箭头
  - 箭头大小：10px
  - 包含箭杆，清晰指示方向

- **中心图标**：
  - 四个小方块排列成 2x2 网格
  - 表示创建标签页分组
  - 方块大小：6x6px，间距：2px

### 4. 交互效果

- 鼠标悬停时：
  - 指示器背景变为蓝色
  - 图标变为白色
  - 显示半透明的预览区域

### 5. 实现细节

```cpp
// 绘制 VS 风格的箭头
wxPoint arrow[3];
arrow[0] = wxPoint(center.x, center.y - iconSize/3);
arrow[1] = wxPoint(center.x - arrowSize/2, center.y - iconSize/3 + arrowSize);
arrow[2] = wxPoint(center.x + arrowSize/2, center.y - iconSize/3 + arrowSize);
dc.DrawPolygon(3, arrow);

// 绘制中心指示器
dc.DrawRectangle(center.x - rectSize - spacing/2, 
                center.y - rectSize - spacing/2, 
                rectSize, rectSize);
```

## 使用方式

1. 拖动窗口时，overlay 自动显示
2. 根据鼠标位置高亮对应的指示器
3. 释放鼠标时，窗口停靠到高亮的位置

## 优势

1. **清晰直观**：箭头明确指示停靠方向
2. **视觉反馈**：高亮和预览让用户知道停靠效果
3. **专业外观**：与 Visual Studio 风格一致
4. **易于识别**：颜色对比度高，图标清晰