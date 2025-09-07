# 轮廓功能编译错误修复

## 修复的错误

### 1. SceneManager::getSceneRoot() 不存在
**错误信息**：
```
error C2039: "getSceneRoot": 不是 "SceneManager" 的成员
```

**原因**：
SceneManager类没有公开的getSceneRoot()方法

**修复方案**：
使用临时场景根的方法，手动组合相机和几何体：
```cpp
SoSeparator* tempSceneRoot = new SoSeparator;
tempSceneRoot->ref();
SoCamera* camera = m_sceneManager->getCamera();
if (camera) {
    tempSceneRoot->addChild(camera);
}
tempSceneRoot->addChild(m_captureRoot);
```

### 2. SoLightModel 未定义
**错误信息**：
```
error C2061: 语法错误: 标识符"SoLightModel"
error C2065: "BASE_COLOR": 未声明的标识符
```

**原因**：
缺少必要的头文件包含

**修复方案**：
添加头文件：
```cpp
#include <Inventor/nodes/SoLightModel.h>
```

### 3. SoMaterial 未定义
**错误信息**：
```
error C2664: "void SoGroup::addChild(SoNode *)": 无法将参数 1 从"int" 转换为"SoNode *"
```

**原因**：
auto* material = new SoMaterial; 由于SoMaterial未定义，被推导为int类型

**修复方案**：
添加头文件：
```cpp
#include <Inventor/nodes/SoMaterial.h>
```

## 修改的文件

1. **ImageOutlinePass.cpp**
   - 添加了 SoLightModel.h 和 SoMaterial.h 包含
   - 修改了场景纹理设置逻辑，使用临时场景根

2. **OutlinePreviewCanvas.cpp**
   - 添加了 SoLightModel.h 包含

## 关键改进

### 临时场景根方法
由于SceneManager不暴露完整的场景根，我们创建了一个临时场景根来包含相机和几何体，这样SoSceneTexture2可以正确渲染深度信息：

```cpp
// 创建包含相机的临时场景根
SoSeparator* tempSceneRoot = new SoSeparator;
tempSceneRoot->ref();

// 添加相机
SoCamera* camera = m_sceneManager->getCamera();
if (camera) {
    tempSceneRoot->addChild(camera);
}

// 添加几何体
tempSceneRoot->addChild(m_captureRoot);

// 设置为渲染目标
m_colorTexture->scene = tempSceneRoot;
m_depthTexture->scene = tempSceneRoot;

// 保存引用以便清理
m_tempSceneRoot = tempSceneRoot;
```

这个方法确保了深度纹理能够正确渲染，因为它包含了必要的相机变换信息。

## 注意事项

1. 记得在析构函数和detachOverlay中清理m_tempSceneRoot
2. 确保所有Coin3D节点类型都包含了相应的头文件
3. 使用ref()和unref()正确管理Coin3D节点的引用计数