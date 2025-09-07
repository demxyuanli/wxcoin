# FlatUI Theme-Aware Pattern

## 概述

`FlatUIThemeAware` 是一个基类，为所有需要响应主题变化的FlatUI组件提供标准化的主题监听和刷新机制。通过继承这个基类，组件可以自动获得主题变化感知能力，无需手动注册监听器。

## 设计目标

1. **标准化**：所有主题感知组件都遵循相同的模式
2. **简化**：减少重复代码，自动处理监听器注册/注销
3. **多态**：通过虚函数实现统一接口
4. **类型安全**：提供类型安全的主题值访问方法

## 使用方法

### 1. 继承FlatUIThemeAware

```cpp
#include "flatui/FlatUIThemeAware.h"

class MyComponent : public FlatUIThemeAware
{
public:
    MyComponent(wxWindow* parent) 
        : FlatUIThemeAware(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        // 初始化代码
        InitializeThemeValues();
    }

    virtual ~MyComponent() = default;

    // 重写主题变化方法
    virtual void OnThemeChanged() override
    {
        // 更新主题相关的颜色和设置
        UpdateThemeValues();
        
        // 刷新子控件
        RefreshChildControls();
        
        // 强制重绘
        Refresh(true);
        Update();
    }

private:
    void InitializeThemeValues()
    {
        // 使用基类提供的辅助方法获取主题值
        m_bgColour = GetThemeColour("MyComponentBgColour");
        m_textColour = GetThemeColour("MyComponentTextColour");
        m_borderWidth = GetThemeInt("MyComponentBorderWidth");
        m_font = GetThemeFont();
    }

    void UpdateThemeValues()
    {
        // 重新获取主题值
        m_bgColour = GetThemeColour("MyComponentBgColour");
        m_textColour = GetThemeColour("MyComponentTextColour");
        m_borderWidth = GetThemeInt("MyComponentBorderWidth");
        
        // 更新控件属性
        SetFont(GetThemeFont());
        SetBackgroundColour(m_bgColour);
    }

    void RefreshChildControls()
    {
        // 刷新所有子控件
        wxWindowList& children = GetChildren();
        for (wxWindow* child : children) {
            child->Refresh(true);
            child->Update();
        }
    }

private:
    wxColour m_bgColour;
    wxColour m_textColour;
    int m_borderWidth;
    wxFont m_font;
};
```

### 2. 基类提供的辅助方法

#### 主题值访问
```cpp
// 获取颜色
wxColour bgColour = GetThemeColour("BackgroundColour");

// 获取整数值
int borderWidth = GetThemeInt("BorderWidth");

// 获取字符串值
std::string fontName = GetThemeString("FontName");

// 获取字体
wxFont font = GetThemeFont();
```

#### 主题刷新
```cpp
// 手动触发主题刷新
component->RefreshTheme();

// 或者直接调用虚函数
component->OnThemeChanged();
```

### 3. 在FlatUIFrame中的使用

```cpp
void FlatUIFrame::RefreshAllUI()
{
    std::function<void(wxWindow*)> refreshRecursive = [&](wxWindow* window) {
        if (!window) return;
        
        // 检查是否是主题感知组件
        FlatUIThemeAware* themeAware = dynamic_cast<FlatUIThemeAware*>(window);
        if (themeAware) {
            // 直接调用标准化的刷新方法
            themeAware->RefreshTheme();
        } else {
            // 对于非主题感知组件，使用原有的刷新逻辑
            // ... 原有的刷新代码 ...
        }
        
        // 递归刷新子控件
        wxWindowList& children = window->GetChildren();
        for (wxWindow* child : children) {
            refreshRecursive(child);
        }
    };
    
    refreshRecursive(this);
}
```

## 优势

### 1. 代码简化
**之前**：
```cpp
// 每个组件都需要手动注册监听器
ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
    RefreshTheme();
});

// 每个组件都需要手动移除监听器
ThemeManager::getInstance().removeThemeChangeListener(this);

// 每个组件都需要实现RefreshTheme方法
void MyComponent::RefreshTheme() { /* ... */ }
```

**现在**：
```cpp
// 只需要继承基类并重写OnThemeChanged
class MyComponent : public FlatUIThemeAware
{
    virtual void OnThemeChanged() override { /* ... */ }
};
```

### 2. 类型安全
```cpp
// 基类提供类型安全的访问方法
wxColour colour = GetThemeColour("Key");  // 返回wxColour
int value = GetThemeInt("Key");           // 返回int
std::string str = GetThemeString("Key");  // 返回std::string
```

### 3. 统一接口
所有主题感知组件都提供相同的接口：
- `RefreshTheme()` - 公共方法，手动触发刷新
- `OnThemeChanged()` - 虚函数，子类重写实现具体逻辑

### 4. 自动生命周期管理
- 构造函数自动注册监听器
- 析构函数自动移除监听器
- 无需手动管理监听器生命周期

## 迁移指南

### 从现有代码迁移

1. **修改继承关系**：
   ```cpp
   // 从
   class MyComponent : public wxControl
   // 改为
   class MyComponent : public FlatUIThemeAware
   ```

2. **修改构造函数**：
   ```cpp
   // 从
   MyComponent::MyComponent(wxWindow* parent)
       : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
   // 改为
   MyComponent::MyComponent(wxWindow* parent)
       : FlatUIThemeAware(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
   ```

3. **移除手动监听器注册**：
   ```cpp
   // 删除这些代码
   ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
       RefreshTheme();
   });
   ```

4. **移除手动监听器注销**：
   ```cpp
   // 删除这些代码
   ThemeManager::getInstance().removeThemeChangeListener(this);
   ```

5. **重命名方法**：
   ```cpp
   // 从
   void RefreshTheme()
   // 改为
   virtual void OnThemeChanged() override
   ```

6. **使用基类辅助方法**：
   ```cpp
   // 从
   m_colour = CFG_COLOUR("Key");
   m_value = CFG_INT("Key");
   SetFont(CFG_DEFAULTFONT());
   // 改为
   m_colour = GetThemeColour("Key");
   m_value = GetThemeInt("Key");
   SetFont(GetThemeFont());
   ```

## 注意事项

1. **虚函数调用**：确保在构造函数中不要调用虚函数，因为此时子类还没有完全构造
2. **内存管理**：基类会自动管理监听器，无需手动清理
3. **性能考虑**：主题变化时会触发所有注册的组件刷新，注意性能影响
4. **线程安全**：主题变化通知应该在主线程中处理

## 示例组件

以下组件已经迁移到新的模式：
- `FlatUIButtonBar`
- `FlatUIPanel`
- `FlatUIBar`
- `FlatUIGallery`
- `FlatUIPage`
- `FlatUIHomeSpace`
- `FlatUIProfileSpace`
- `FlatUISystemButtons` 