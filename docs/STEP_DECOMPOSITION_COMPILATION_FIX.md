# STEP文件面组分解功能编译错误修复

## 问题描述

编译时遇到以下错误：

```
error C3861: "decomposeByFaceGroups": 找不到标识符
error C3861: "areFacesSimilar": 找不到标识符
```

## 问题原因

函数声明顺序问题：
- `decomposeShape`函数在第151行调用了`decomposeByFaceGroups`
- `decomposeByFaceGroups`函数在第198行调用了`areFacesSimilar`
- 但这些函数在被调用之前还没有声明

## 修复方案

### 添加前向声明

在函数定义之前添加前向声明：

```cpp
// Forward declarations
static void decomposeByFaceGroups(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes);
static bool areFacesSimilar(const TopoDS_Face& face1, const TopoDS_Face& face2);
```

### 函数调用顺序

修复后的函数调用顺序：

1. **前向声明**：声明函数签名
2. **decomposeShape**：主分解函数，调用decomposeByFaceGroups
3. **decomposeByFaceGroups**：面组分解函数，调用areFacesSimilar
4. **areFacesSimilar**：面相似性判断函数

## 技术细节

### 前向声明的必要性

在C++中，当函数A调用函数B，而函数B的定义在函数A之后时，需要前向声明：

```cpp
// 错误：函数B未声明就被调用
void functionA() {
    functionB(); // 编译错误：找不到标识符
}

void functionB() {
    // 函数实现
}

// 正确：添加前向声明
void functionB(); // 前向声明

void functionA() {
    functionB(); // 现在可以正常调用
}

void functionB() {
    // 函数实现
}
```

### 静态函数的前向声明

对于静态函数，前向声明格式：

```cpp
static ReturnType functionName(ParameterList);
```

## 修复效果

修复后的代码结构：

```cpp
// 前向声明
static void decomposeByFaceGroups(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes);
static bool areFacesSimilar(const TopoDS_Face& face1, const TopoDS_Face& face2);

// 主分解函数
static void decomposeShape(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
    // ... 实现
    decomposeByFaceGroups(shape, subShapes); // 现在可以正常调用
}

// 面组分解函数
static void decomposeByFaceGroups(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
    // ... 实现
    if (areFacesSimilar(face, groupFace)) { // 现在可以正常调用
        // ... 处理
    }
}

// 面相似性判断函数
static bool areFacesSimilar(const TopoDS_Face& face1, const TopoDS_Face& face2) {
    // ... 实现
}
```

## 验证

修复后应该能够：

1. **正常编译**：不再出现"找不到标识符"错误
2. **功能正常**：面组分解功能正常工作
3. **调用正确**：函数调用关系正确

## 总结

通过添加前向声明，解决了函数调用顺序问题，现在代码应该能够正常编译，面组分解功能也能正常工作。