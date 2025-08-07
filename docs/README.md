# Flat Widgets

基于PyQt-Fluent-Widgets设计理念的现代化wxWidgets控件库。

## 概述

Flat Widgets是一组现代化的wxWidgets控件，设计灵感来源于PyQt-Fluent-Widgets。这些控件提供了现代化的外观和用户体验，包括：

- 圆角设计
- 现代化的颜色方案
- 平滑的动画效果
- 一致的设计语言
- 良好的可访问性支持

## 控件列表

### 1. FlatButton - 现代化按钮

支持多种样式：
- PRIMARY - 主要按钮（蓝色背景）
- SECONDARY - 次要按钮（灰色背景）
- TRANSPARENT - 透明按钮
- OUTLINE - 轮廓按钮
- TEXT - 文本按钮
- ICON - 图标按钮
- ICON_TEXT - 图标+文本按钮

```cpp
// 创建主要按钮
auto* button = new FlatButton(this, wxID_ANY, "Click Me", 
                             wxDefaultPosition, wxDefaultSize,
                             FlatButton::ButtonStyle::PRIMARY);

// 绑定事件
button->Bind(wxEVT_FLAT_BUTTON_CLICKED, &MyFrame::OnButtonClicked, this);

// 设置颜色
button->SetBackgroundColor(wxColour(0, 120, 215));
button->SetTextColor(wxColour(255, 255, 255));
```

### 2. FlatLineEdit - 现代化文本输入框

支持多种样式：
- NORMAL - 普通文本输入
- SEARCH - 搜索输入框
- PASSWORD - 密码输入框
- CLEARABLE - 可清除输入框

```cpp
// 创建搜索输入框
auto* lineEdit = new FlatLineEdit(this, wxID_ANY, "Search...",
                                 wxDefaultPosition, wxDefaultSize,
                                 FlatLineEdit::LineEditStyle::SEARCH);

// 设置占位符文本
lineEdit->SetPlaceholderText("Enter search term");

// 绑定事件
lineEdit->Bind(wxEVT_FLAT_LINE_EDIT_TEXT_CHANGED, &MyFrame::OnTextChanged, this);
```

### 3. FlatComboBox - 现代化下拉框

支持多种样式：
- NORMAL - 普通下拉框
- EDITABLE - 可编辑下拉框
- SEARCH - 可搜索下拉框
- MULTI_SELECT - 多选下拉框

```cpp
// 创建下拉框
auto* comboBox = new FlatComboBox(this, wxID_ANY, "Select item",
                                 wxDefaultPosition, wxDefaultSize,
                                 FlatComboBox::ComboBoxStyle::NORMAL);

// 添加项目
comboBox->Append("Item 1");
comboBox->Append("Item 2");
comboBox->Append("Item 3");

// 绑定事件
comboBox->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, &MyFrame::OnSelectionChanged, this);
```

### 4. FlatCheckBox - 现代化复选框

支持多种样式：
- NORMAL - 普通复选框
- SWITCH - 开关样式
- RADIO - 单选样式
- CUSTOM - 自定义样式

```cpp
// 创建复选框
auto* checkBox = new FlatCheckBox(this, wxID_ANY, "Check me",
                                 wxDefaultPosition, wxDefaultSize,
                                 FlatCheckBox::CheckBoxStyle::NORMAL);

// 绑定事件
checkBox->Bind(wxEVT_FLAT_CHECK_BOX_CLICKED, &MyFrame::OnCheckBoxClicked, this);
```

### 5. FlatSlider - 现代化滑块

支持多种样式：
- NORMAL - 普通滑块
- PROGRESS - 进度条样式
- RANGE - 范围滑块
- VERTICAL - 垂直滑块

```cpp
// 创建滑块
auto* slider = new FlatSlider(this, wxID_ANY, 50, 0, 100,
                             wxDefaultPosition, wxDefaultSize,
                             FlatSlider::SliderStyle::NORMAL);

// 设置显示选项
slider->SetShowValue(true);
slider->SetShowTicks(true);

// 绑定事件
slider->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, &MyFrame::OnSliderChanged, this);
```

### 6. FlatProgressBar - 现代化进度条

支持多种样式：
- NORMAL - 普通进度条
- INDETERMINATE - 不确定进度条
- CIRCULAR - 圆形进度条
- STRIPED - 条纹进度条

```cpp
// 创建进度条
auto* progressBar = new FlatProgressBar(this, wxID_ANY, 75, 0, 100,
                                       wxDefaultPosition, wxDefaultSize,
                                       FlatProgressBar::ProgressBarStyle::NORMAL);

// 设置显示选项
progressBar->SetShowPercentage(true);
progressBar->SetShowValue(true);

// 绑定事件
progressBar->Bind(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, &MyFrame::OnProgressChanged, this);
```

### 7. FlatSwitch - 现代化开关

支持多种样式：
- NORMAL - 普通开关
- ROUND - 圆形开关
- SQUARE - 方形开关
- CUSTOM - 自定义开关

```cpp
// 创建开关
auto* switchControl = new FlatSwitch(this, wxID_ANY, false,
                                    wxDefaultPosition, wxDefaultSize,
                                    FlatSwitch::SwitchStyle::NORMAL);

// 设置标签
switchControl->SetLabel("Enable feature");

// 绑定事件
switchControl->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &MyFrame::OnSwitchToggled, this);
```

## 安装和使用

### 1. 包含头文件

```cpp
#include "widgets/FlatButton.h"
#include "widgets/FlatLineEdit.h"
#include "widgets/FlatComboBox.h"
#include "widgets/FlatCheckBox.h"
#include "widgets/FlatSlider.h"
#include "widgets/FlatProgressBar.h"
#include "widgets/FlatSwitch.h"
```

### 2. 链接库

在CMakeLists.txt中添加：

```cmake
# 添加widgets库
add_subdirectory(widgets)

# 链接widgets库
target_link_libraries(your_target widgets)
```

### 3. 编译

```bash
mkdir build
cd build
cmake ..
make
```

## 主题和样式

所有控件都支持自定义主题：

```cpp
// 设置颜色主题
button->SetBackgroundColor(wxColour(0, 120, 215));
button->SetTextColor(wxColour(255, 255, 255));
button->SetBorderColor(wxColour(0, 102, 184));

// 设置圆角
button->SetCornerRadius(8);

// 设置边框宽度
button->SetBorderWidth(1);
```

## 事件处理

所有控件都提供了自定义事件：

```cpp
// 按钮事件
wxEVT_FLAT_BUTTON_CLICKED
wxEVT_FLAT_BUTTON_HOVER

// 文本输入框事件
wxEVT_FLAT_LINE_EDIT_TEXT_CHANGED
wxEVT_FLAT_LINE_EDIT_FOCUS_GAINED
wxEVT_FLAT_LINE_EDIT_FOCUS_LOST

// 下拉框事件
wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED
wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED
wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED

// 复选框事件
wxEVT_FLAT_CHECK_BOX_CLICKED
wxEVT_FLAT_CHECK_BOX_STATE_CHANGED

// 滑块事件
wxEVT_FLAT_SLIDER_VALUE_CHANGED
wxEVT_FLAT_SLIDER_THUMB_DRAGGED

// 进度条事件
wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED
wxEVT_FLAT_PROGRESS_BAR_COMPLETED

// 开关事件
wxEVT_FLAT_SWITCH_TOGGLED
wxEVT_FLAT_SWITCH_STATE_CHANGED
```

## 示例

查看 `FlatWidgetsExample.h` 文件获取完整的使用示例。

## 贡献

欢迎提交问题和改进建议！

## 许可证

本项目采用MIT许可证。
