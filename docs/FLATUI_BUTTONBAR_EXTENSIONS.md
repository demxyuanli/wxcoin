# FlatUIButtonBar 扩展功能文档

## 概述

FlatUIButtonBar 已成功扩展支持多种控件类型，包括 ToggleButton、CheckBox、RadioButton、Choice 和 Separator。这些扩展大大增强了用户界面的灵活性和交互性。

## 新增控件类型

### 1. ButtonType 枚举

```cpp
enum class ButtonType {
    NORMAL,         // 标准按钮
    TOGGLE,         // 切换按钮 (开/关状态)
    CHECKBOX,       // 复选框
    RADIO,          // 单选按钮 (属于单选组)
    CHOICE,         // 选择/下拉控件
    SEPARATOR       // 视觉分隔符
};
```

### 2. 扩展的 ButtonInfo 结构

新增了以下属性来支持不同的控件类型：

```cpp
struct ButtonInfo {
    // 新增属性
    ButtonType type = ButtonType::NORMAL;
    bool checked = false;               // 用于 toggle、checkbox、radio
    bool enabled = true;                // 控件启用/禁用状态
    int radioGroup = -1;                // 单选按钮组 ID
    wxArrayString choiceItems;          // Choice 控件的选项
    int selectedChoice = -1;            // Choice 控件的选中项
    wxString value;                     // 当前值
    
    // 视觉属性
    wxColour customBgColor;             // 自定义背景色
    wxColour customTextColor;           // 自定义文字色
    wxColour customBorderColor;         // 自定义边框色
    bool visible = true;                // 控件可见性
};
```

## 新增 API 方法

### 添加控件方法

```cpp
// 增强的按钮方法，支持类型
void AddButton(int id, ButtonType type, const wxString& label, 
               const wxBitmap& bitmap = wxNullBitmap, 
               const wxString& tooltip = wxEmptyString);

// 专门的控件添加方法
void AddToggleButton(int id, const wxString& label, bool initialState = false, 
                     const wxBitmap& bitmap = wxNullBitmap, 
                     const wxString& tooltip = wxEmptyString);

void AddCheckBox(int id, const wxString& label, bool initialState = false, 
                 const wxString& tooltip = wxEmptyString);

void AddRadioButton(int id, const wxString& label, int radioGroup, 
                    bool initialState = false, 
                    const wxString& tooltip = wxEmptyString);

void AddChoiceControl(int id, const wxString& label, 
                      const wxArrayString& choices, int initialSelection = 0, 
                      const wxString& tooltip = wxEmptyString);

void AddSeparator();
```

### 状态管理方法

```cpp
// 检查状态控制
void SetButtonChecked(int id, bool checked);
bool IsButtonChecked(int id) const;

// 启用/禁用控制
void SetButtonEnabled(int id, bool enabled);
bool IsButtonEnabled(int id) const;

// 可见性控制
void SetButtonVisible(int id, bool visible);
bool IsButtonVisible(int id) const;

// Choice 控件专用方法
void SetChoiceItems(int id, const wxArrayString& items);
wxArrayString GetChoiceItems(int id) const;
void SetChoiceSelection(int id, int selection);
int GetChoiceSelection(int id) const;
wxString GetChoiceValue(int id) const;

// 单选组管理
void SetRadioGroupSelection(int radioGroup, int selectedId);
int GetRadioGroupSelection(int radioGroup) const;

// 值和属性设置
void SetButtonValue(int id, const wxString& value);
wxString GetButtonValue(int id) const;
void SetButtonCustomColors(int id, const wxColour& bgColor, 
                          const wxColour& textColor = wxNullColour, 
                          const wxColour& borderColor = wxNullColour);

// 控件移除
void RemoveButton(int id);
void Clear();
```

## 事件处理

### 新增事件类型

不同的控件类型会发送不同的 wxWidgets 事件：

- **ToggleButton**: `wxEVT_TOGGLEBUTTON` - 切换状态时触发
- **CheckBox**: `wxEVT_CHECKBOX` - 勾选状态改变时触发
- **RadioButton**: `wxEVT_RADIOBUTTON` - 选中状态改变时触发
- **Choice**: `wxEVT_CHOICE` - 选择项改变时触发

### 事件处理示例

```cpp
// 在事件表中绑定
BEGIN_EVENT_TABLE(MyClass, wxPanel)
    EVT_TOGGLEBUTTON(ID_TOGGLE_WIREFRAME, MyClass::OnToggleWireframe)
    EVT_CHECKBOX(ID_CHECKBOX_SHOW_EDGES, MyClass::OnShowEdges)
    EVT_RADIOBUTTON(ID_RADIO_VIEW_FRONT, MyClass::OnViewChange)
    EVT_CHOICE(ID_CHOICE_QUALITY, MyClass::OnQualityChange)
END_EVENT_TABLE()

// 事件处理函数
void MyClass::OnToggleWireframe(wxCommandEvent& event) {
    bool isChecked = event.GetInt() == 1;
    // 处理切换按钮状态
}

void MyClass::OnQualityChange(wxCommandEvent& event) {
    int selection = event.GetInt();
    wxString value = event.GetString();
    // 处理选择变化
}
```

## 视觉设计

### ToggleButton
- 选中状态使用按下的背景色
- 支持图标和文字显示
- 可设置自定义颜色

### CheckBox
- 16x16 像素的复选框指示器
- 支持勾选标记绘制
- 文字在指示器右侧显示

### RadioButton
- 16x16 像素的圆形指示器
- 选中时显示内部填充圆
- 支持单选组管理

### Choice 控件
- 类似下拉框的外观
- 右侧有下拉箭头
- 显示当前选中值
- 文字过长时自动截断显示

### Separator
- 固定宽度 8 像素
- 不响应鼠标点击
- 用于视觉分组

## 使用示例

```cpp
// 创建按钮栏
auto* buttonBar = new FlatUIButtonBar(this);

// 添加切换按钮
buttonBar->AddToggleButton(1001, "Wireframe", false, wxNullBitmap, "切换线框显示");
buttonBar->AddToggleButton(1002, "Lighting", true, wxNullBitmap, "切换光照");

// 添加分隔符
buttonBar->AddSeparator();

// 添加复选框
buttonBar->AddCheckBox(1003, "Show Edges", true, "显示边");
buttonBar->AddCheckBox(1004, "Show Vertices", false, "显示顶点");

// 添加单选按钮组（视图模式，组ID = 1）
buttonBar->AddRadioButton(1005, "Front", 1, false, "前视图");
buttonBar->AddRadioButton(1006, "Top", 1, false, "顶视图");
buttonBar->AddRadioButton(1007, "Right", 1, false, "右视图");
buttonBar->AddRadioButton(1008, "Isometric", 1, true, "等轴视图");

buttonBar->AddSeparator();

// 添加选择控件
wxArrayString qualityOptions;
qualityOptions.Add("草图");
qualityOptions.Add("普通");
qualityOptions.Add("高质量");
qualityOptions.Add("超高质量");
buttonBar->AddChoiceControl(1009, "Quality", qualityOptions, 1, "渲染质量");

// 设置自定义颜色
buttonBar->SetButtonCustomColors(1001, wxColour(0, 120, 0), *wxWHITE);

// 程序控制
buttonBar->SetButtonChecked(1002, true);
buttonBar->SetRadioGroupSelection(1, 1008);
buttonBar->SetChoiceSelection(1009, 2);
```

## 技术特性

### 向后兼容性
- 原有的 `AddButton` 方法完全保持兼容
- 现有代码无需修改即可继续使用

### 性能优化
- 智能布局计算，跳过不可见控件
- 高效的事件分发机制
- 自适应宽度计算

### 扩展性设计
- 清晰的控件类型架构
- 易于添加新的控件类型
- 灵活的自定义属性支持

### 主题支持
- 继承现有的主题系统
- 支持自定义颜色覆盖
- 响应式视觉效果

## 注意事项

1. **单选组管理**: 使用相同的 `radioGroup` ID 来创建单选组
2. **Choice 控件**: 当前实现为简单的循环选择，可扩展为真正的下拉菜单
3. **事件处理**: 新控件类型发送标准的 wxWidgets 事件
4. **布局计算**: 不同控件类型有不同的宽度计算逻辑
5. **可见性**: 隐藏的控件不参与布局计算，不响应鼠标事件

## 未来扩展建议

1. **真正的下拉菜单**: 为 Choice 控件实现弹出菜单
2. **滑块控件**: 添加 Slider 类型支持
3. **文本输入**: 添加简单的文本输入框支持
4. **分组容器**: 添加可折叠的控件组
5. **拖拽重排**: 支持控件的拖拽重新排序

这个扩展大大增强了 FlatUIButtonBar 的功能，使其成为一个功能完整的工具栏控件，适用于各种复杂的用户界面需求。 