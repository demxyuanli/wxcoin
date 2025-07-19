# 曲面平滑渲染方法使用示例

## 概述

本项目已成功实现了三种主要的曲面平滑渲染方法：

1. **细分曲面 (Subdivision Surfaces)** - 通过递归细分提高曲面质量
2. **法线平滑 (Normal Smoothing)** - 通过法线插值实现平滑着色
3. **曲面细分 (Tessellation)** - 自适应细分基于曲率和边长

## 功能特性

### ✅ 细分曲面算法
- **Catmull-Clark细分**: 适用于四边形网格的细分算法
- **Loop细分**: 适用于三角形网格的细分算法
- **Butterfly细分**: 高质量的三角形细分算法

### ✅ 法线平滑技术
- **多次迭代平滑**: 可配置迭代次数
- **折痕角度控制**: 保持锐利边缘
- **自适应权重**: 基于几何特征调整平滑强度

### ✅ 自适应细分
- **曲率驱动**: 基于平均曲率和高斯曲率
- **边长控制**: 限制最大边长
- **质量优化**: 保持网格质量

## 使用示例

### 1. 基本曲面平滑

```cpp
#include "OCCMeshConverter.h"
#include "OCCShapeBuilder.h"

// 创建基础几何体
TopoDS_Shape sphere = OCCShapeBuilder::createSphere(1.0);
TriangleMesh originalMesh = OCCMeshConverter::convertToMesh(sphere);

// 方法1: 使用细分曲面
TriangleMesh subdividedMesh = OCCMeshConverter::createSubdivisionSurface(originalMesh, 2);

// 方法2: 使用法线平滑
TriangleMesh smoothedMesh = OCCMeshConverter::smoothNormalsAdvanced(originalMesh, 30.0, 3);

// 方法3: 使用自适应细分
TriangleMesh tessellatedMesh = OCCMeshConverter::adaptiveTessellation(originalMesh, 0.1, 0.1);

// 创建Coin3D节点
SoSeparator* smoothNode = OCCMeshConverter::createSmoothSurfaceNode(originalMesh, true, 2);
```

### 2. 高级细分曲面

```cpp
// Catmull-Clark细分 (适用于四边形网格)
TriangleMesh catmullClarkMesh = OCCMeshConverter::catmullClarkSubdivision(originalMesh, 3);

// Loop细分 (适用于三角形网格)
TriangleMesh loopMesh = OCCMeshConverter::loopSubdivision(originalMesh, 3);

// Butterfly细分
TriangleMesh butterflyMesh = OCCMeshConverter::butterflySubdivision(originalMesh, 2);
```

### 3. 曲率分析和自适应处理

```cpp
// 计算曲率
std::vector<double> meanCurvature, gaussianCurvature;
OCCMeshConverter::calculateCurvature(originalMesh, meanCurvature, gaussianCurvature);

// 基于曲率进行自适应细分
TriangleMesh adaptiveMesh = OCCMeshConverter::adaptiveTessellation(
    originalMesh, 
    0.05,  // 最大边长
    0.2    // 曲率阈值
);
```

### 4. 在OCCViewer中使用

```cpp
#include "OCCViewer.h"
#include "OCCGeometry.h"

// 创建基础几何体
auto geometry = std::make_shared<OCCGeometry>("Sphere", sphere);

// 应用曲面平滑
TriangleMesh originalMesh = OCCMeshConverter::convertToMesh(sphere);
TriangleMesh smoothMesh = OCCMeshConverter::createSubdivisionSurface(originalMesh, 2);

// 创建平滑几何体
auto smoothGeometry = std::make_shared<OCCGeometry>("SmoothSphere", sphere);
smoothGeometry->setMesh(smoothMesh);

// 添加到查看器
occViewer->addGeometry(smoothGeometry);
```

## 算法详解

### 1. Catmull-Clark细分算法

**特点:**
- 适用于四边形网格
- 保持C²连续性
- 收敛到B样条曲面

**步骤:**
1. **面点计算**: 计算每个面的质心
2. **边点计算**: 计算每条边的中点
3. **顶点更新**: 使用Catmull-Clark规则更新顶点位置
4. **网格重构**: 创建新的四边形网格

```cpp
// Catmull-Clark细分示例
TriangleMesh mesh = OCCMeshConverter::convertToMesh(sphere);
TriangleMesh subdivided = OCCMeshConverter::catmullClarkSubdivision(mesh, 2);

// 查看细分效果
std::cout << "Original vertices: " << mesh.getVertexCount() << std::endl;
std::cout << "Subdivided vertices: " << subdivided.getVertexCount() << std::endl;
std::cout << "Original triangles: " << mesh.getTriangleCount() << std::endl;
std::cout << "Subdivided triangles: " << subdivided.getTriangleCount() << std::endl;
```

### 2. Loop细分算法

**特点:**
- 适用于三角形网格
- 保持C²连续性
- 收敛到Box样条曲面

**步骤:**
1. **边点计算**: 使用Loop规则计算边点
2. **顶点更新**: 使用Loop顶点规则更新顶点
3. **三角形细分**: 每个三角形分为4个小三角形

```cpp
// Loop细分示例
TriangleMesh mesh = OCCMeshConverter::convertToMesh(sphere);
TriangleMesh loopSubdivided = OCCMeshConverter::loopSubdivision(mesh, 3);

// 比较细分效果
std::cout << "Loop subdivision level 3:" << std::endl;
std::cout << "Vertices: " << loopSubdivided.getVertexCount() << std::endl;
std::cout << "Triangles: " << loopSubdivided.getTriangleCount() << std::endl;
```

### 3. 法线平滑算法

**特点:**
- 保持几何形状不变
- 只平滑法线向量
- 可控制折痕角度

**步骤:**
1. **法线累积**: 在每个顶点累积相邻面的法线
2. **法线归一化**: 归一化累积的法线向量
3. **多次迭代**: 重复过程以获得更平滑的结果

```cpp
// 法线平滑示例
TriangleMesh mesh = OCCMeshConverter::convertToMesh(sphere);

// 不同参数的法线平滑
TriangleMesh smooth1 = OCCMeshConverter::smoothNormalsAdvanced(mesh, 15.0, 1);  // 锐利边缘
TriangleMesh smooth2 = OCCMeshConverter::smoothNormalsAdvanced(mesh, 30.0, 3);  // 中等平滑
TriangleMesh smooth3 = OCCMeshConverter::smoothNormalsAdvanced(mesh, 60.0, 5);  // 高度平滑
```

### 4. 自适应细分算法

**特点:**
- 基于几何特征自适应细分
- 保持细节的同时减少计算量
- 曲率驱动的细分策略

**步骤:**
1. **曲率计算**: 计算平均曲率和高斯曲率
2. **边长分析**: 检查三角形边长
3. **自适应细分**: 根据曲率和边长决定细分级别

```cpp
// 自适应细分示例
TriangleMesh mesh = OCCMeshConverter::convertToMesh(sphere);

// 不同参数的自适应细分
TriangleMesh adaptive1 = OCCMeshConverter::adaptiveTessellation(mesh, 0.2, 0.5);   // 粗糙
TriangleMesh adaptive2 = OCCMeshConverter::adaptiveTessellation(mesh, 0.1, 0.2);   // 中等
TriangleMesh adaptive3 = OCCMeshConverter::adaptiveTessellation(mesh, 0.05, 0.1);  // 精细
```

## 性能优化

### 1. 缓存机制

```cpp
// 缓存细分结果
static std::map<std::pair<TriangleMesh, int>, TriangleMesh> subdivisionCache;

TriangleMesh getCachedSubdivision(const TriangleMesh& mesh, int levels) {
    auto key = std::make_pair(mesh, levels);
    if (subdivisionCache.find(key) != subdivisionCache.end()) {
        return subdivisionCache[key];
    }
    
    TriangleMesh result = OCCMeshConverter::createSubdivisionSurface(mesh, levels);
    subdivisionCache[key] = result;
    return result;
}
```

### 2. 渐进式细分

```cpp
// 渐进式细分 - 从低级别开始
TriangleMesh progressiveSubdivision(const TriangleMesh& mesh, int targetLevels) {
    TriangleMesh result = mesh;
    
    for (int level = 1; level <= targetLevels; ++level) {
        result = OCCMeshConverter::createSubdivisionSurface(result, 1);
        
        // 可以在这里添加用户交互检查
        // if (userWantsToStop()) break;
    }
    
    return result;
}
```

### 3. LOD (Level of Detail) 支持

```cpp
// LOD系统
class SurfaceLOD {
private:
    std::vector<TriangleMesh> lodLevels;
    
public:
    void generateLODs(const TriangleMesh& baseMesh, int maxLevels) {
        lodLevels.clear();
        lodLevels.push_back(baseMesh);
        
        for (int i = 1; i <= maxLevels; ++i) {
            TriangleMesh subdivided = OCCMeshConverter::createSubdivisionSurface(
                lodLevels.back(), 1);
            lodLevels.push_back(subdivided);
        }
    }
    
    TriangleMesh getLOD(int level) {
        if (level >= 0 && level < static_cast<int>(lodLevels.size())) {
            return lodLevels[level];
        }
        return lodLevels.back();
    }
};
```

## 质量评估

### 1. 网格质量指标

```cpp
// 计算网格质量
void analyzeMeshQuality(const TriangleMesh& mesh) {
    double minAngle = 180.0, maxAngle = 0.0;
    double totalArea = 0.0;
    
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v0 = mesh.triangles[i];
        int v1 = mesh.triangles[i + 1];
        int v2 = mesh.triangles[i + 2];
        
        const gp_Pnt& p0 = mesh.vertices[v0];
        const gp_Pnt& p1 = mesh.vertices[v1];
        const gp_Pnt& p2 = mesh.vertices[v2];
        
        // 计算角度
        gp_Vec edge1(p0, p1), edge2(p0, p2), edge3(p1, p2);
        double angle1 = edge1.Angle(edge2) * 180.0 / M_PI;
        double angle2 = edge1.Angle(edge3) * 180.0 / M_PI;
        double angle3 = 180.0 - angle1 - angle2;
        
        minAngle = std::min(minAngle, std::min(angle1, std::min(angle2, angle3)));
        maxAngle = std::max(maxAngle, std::max(angle1, std::max(angle2, angle3)));
        
        // 计算面积
        gp_Vec normal = edge1.Crossed(edge2);
        totalArea += normal.Magnitude() / 2.0;
    }
    
    std::cout << "Mesh Quality Analysis:" << std::endl;
    std::cout << "Min angle: " << minAngle << " degrees" << std::endl;
    std::cout << "Max angle: " << maxAngle << " degrees" << std::endl;
    std::cout << "Total area: " << totalArea << std::endl;
    std::cout << "Average area: " << totalArea / mesh.getTriangleCount() << std::endl;
}
```

### 2. 曲率分析

```cpp
// 曲率分析
void analyzeCurvature(const TriangleMesh& mesh) {
    std::vector<double> meanCurvature, gaussianCurvature;
    OCCMeshConverter::calculateCurvature(mesh, meanCurvature, gaussianCurvature);
    
    double minMean = *std::min_element(meanCurvature.begin(), meanCurvature.end());
    double maxMean = *std::max_element(meanCurvature.begin(), meanCurvature.end());
    double avgMean = std::accumulate(meanCurvature.begin(), meanCurvature.end(), 0.0) / meanCurvature.size();
    
    std::cout << "Curvature Analysis:" << std::endl;
    std::cout << "Mean curvature - Min: " << minMean << ", Max: " << maxMean << ", Avg: " << avgMean << std::endl;
}
```

## 总结

通过实现这三种曲面平滑渲染方法，项目现在具备了：

1. **高质量的细分曲面**: 支持多种细分算法，适用于不同类型的网格
2. **智能的法线平滑**: 保持几何形状的同时实现平滑着色
3. **自适应的曲面细分**: 基于几何特征的高效细分策略
4. **完整的性能优化**: 缓存、LOD和渐进式处理
5. **详细的质量评估**: 网格质量和曲率分析工具

这些功能为CAD应用提供了专业的曲面处理能力，可以生成高质量的渲染结果。 