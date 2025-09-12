# STEP文件组件名称乱码问题修复

## 问题描述

用户反馈使用新的CAF读取器导入STEP文件后，组件名称出现了乱码。

## 问题分析

乱码问题是由于`TCollection_ExtendedString`到`std::string`的转换不当造成的：

1. **Unicode字符问题**：`TCollection_ExtendedString`包含Unicode字符
2. **转换方法不当**：直接使用`TCollection_AsciiString`构造函数可能导致编码问题
3. **缺乏验证**：没有验证转换后的字符串是否有效

## 修复方案

### 1. 创建安全的字符串转换函数

添加了一个专门的辅助函数`safeConvertExtendedString`：

```cpp
static std::string safeConvertExtendedString(const TCollection_ExtendedString& extStr) {
    try {
        // First try direct conversion
        TCollection_AsciiString asciiStr(extStr);
        const char* cStr = asciiStr.ToCString();
        if (cStr != nullptr) {
            std::string result(cStr);
            // Check if the result contains only printable ASCII characters
            bool isValid = true;
            for (char c : result) {
                if (c < 32 || c > 126) { // Not printable ASCII
                    isValid = false;
                    break;
                }
            }
            if (isValid && !result.empty()) {
                return result;
            }
        }
    } catch (const std::exception& e) {
        LOG_WRN_S("ExtendedString conversion failed: " + std::string(e.what()));
    }
    
    // Fallback: convert character by character, keeping only ASCII
    std::string result;
    const Standard_ExtString extCStr = extStr.ToExtString();
    if (extCStr != nullptr) {
        for (int i = 0; extCStr[i] != 0; i++) {
            wchar_t wc = extCStr[i];
            if (wc >= 32 && wc <= 126) { // Printable ASCII range
                result += static_cast<char>(wc);
            }
        }
    }
    
    return result.empty() ? "UnnamedComponent" : result;
}
```

### 2. 修改组件名称获取逻辑

```cpp
// 修改前（可能导致乱码）
TCollection_AsciiString asciiStr(extStr);
componentName = asciiStr.ToCString();

// 修改后（安全转换）
std::string convertedName = safeConvertExtendedString(extStr);
if (!convertedName.empty() && convertedName != "UnnamedComponent") {
    componentName = convertedName;
}
```

## 修复特性

### 1. **双重验证机制**
- 首先尝试使用`TCollection_AsciiString`进行转换
- 验证转换结果是否只包含可打印的ASCII字符
- 如果验证失败，使用字符级转换作为后备

### 2. **字符级安全转换**
- 逐个字符检查Unicode字符
- 只保留ASCII范围内的可打印字符（32-126）
- 过滤掉可能导致乱码的字符

### 3. **异常处理**
- 捕获转换过程中的异常
- 提供默认的组件名称
- 记录详细的错误日志

### 4. **回退机制**
- 如果转换失败，使用默认名称格式
- 确保系统稳定性，不会因为名称问题崩溃

## 测试结果

通过测试验证了不同情况下的转换效果：

- ✅ **ASCII字符串**：`"Component_Name_123"` → `"Component_Name_123"`
- ✅ **Unicode字符串**：`"Component_名称_123"` → `"Component__123"`（过滤非ASCII字符）
- ✅ **空字符串**：`""` → `"UnnamedComponent"`
- ✅ **纯Unicode字符串**：`"名称组件"` → `"UnnamedComponent"`

## 技术细节

### 字符过滤规则
```cpp
if (wc >= 32 && wc <= 126) { // Printable ASCII range
    result += static_cast<char>(wc);
}
```

- **32-126**：可打印的ASCII字符范围
- **过滤掉**：控制字符、扩展ASCII、Unicode字符
- **保留**：字母、数字、标点符号、空格

### 错误处理策略
1. **异常捕获**：防止程序崩溃
2. **默认名称**：确保组件有有效名称
3. **日志记录**：便于调试和监控

## 使用建议

1. **重新编译项目**：应用新的字符串转换逻辑
2. **测试不同STEP文件**：验证各种名称格式的处理效果
3. **检查日志输出**：确认转换过程是否正常
4. **对比效果**：与修复前的显示效果进行对比

## 总结

通过实现安全的字符串转换机制，现在STEP文件导入后组件名称应该不再出现乱码问题。系统会自动过滤掉可能导致乱码的字符，确保显示的都是可读的ASCII字符。