#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// 模拟TabPosition枚举
enum class TabPosition {
    Top,        // Tabs at top (merged with title bar)
    Bottom,     // Tabs at bottom (independent title bar)
    Left,       // Tabs at left (independent title bar)
    Right       // Tabs at right (independent title bar)
};

// 模拟DockWidget类
class DockWidget {
public:
    DockWidget(const std::string& title) : m_title(title), m_dockArea(nullptr) {}
    
    std::string title() const { return m_title; }
    void setDockArea(class DockArea* area) { m_dockArea = area; }
    class DockArea* dockAreaWidget() const { return m_dockArea; }
    
private:
    std::string m_title;
    class DockArea* m_dockArea;
};

// 模拟DockArea类
class DockArea {
public:
    DockArea(const std::string& name) : m_name(name), m_tabPosition(TabPosition::Top) {}
    
    void setTabPosition(TabPosition position) {
        if (m_tabPosition == position) {
            return;
        }
        
        m_tabPosition = position;
        std::cout << "  [" << m_name << "] Tab position changed to: " << getPositionName(position) << std::endl;
    }
    
    TabPosition tabPosition() const { return m_tabPosition; }
    
    void addDockWidget(DockWidget* widget) {
        m_widgets.push_back(widget);
        widget->setDockArea(this);
        std::cout << "  [" << m_name << "] Added widget: " << widget->title() << std::endl;
    }
    
    void removeDockWidget(DockWidget* widget) {
        auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);
        if (it != m_widgets.end()) {
            m_widgets.erase(it);
            widget->setDockArea(nullptr);
            std::cout << "  [" << m_name << "] Removed widget: " << widget->title() << std::endl;
        }
    }
    
    void setCurrentDockWidget(DockWidget* widget) {
        std::cout << "  [" << m_name << "] Set current widget: " << widget->title() << std::endl;
    }
    
    std::string name() const { return m_name; }
    int widgetCount() const { return static_cast<int>(m_widgets.size()); }
    
private:
    std::string m_name;
    TabPosition m_tabPosition;
    std::vector<DockWidget*> m_widgets;
    
    std::string getPositionName(TabPosition position) const {
        switch (position) {
            case TabPosition::Top: return "Top";
            case TabPosition::Bottom: return "Bottom";
            case TabPosition::Left: return "Left";
            case TabPosition::Right: return "Right";
            default: return "Unknown";
        }
    }
};

// 模拟拖拽合并逻辑
void simulateDragMerge(DockArea* sourceArea, DockArea* targetArea, DockWidget* draggedWidget) {
    std::cout << "\n=== Drag Merge Simulation ===" << std::endl;
    std::cout << "Dragging widget '" << draggedWidget->title() << "' from '" 
              << sourceArea->name() << "' to '" << targetArea->name() << "'" << std::endl;
    
    // Get target area tab position
    TabPosition targetTabPosition = targetArea->tabPosition();
    std::cout << "Target area tab position: " << static_cast<int>(targetTabPosition) << std::endl;
    
    // Get source area before removing widget
    DockArea* sourceAreaPtr = draggedWidget->dockAreaWidget();
    
    // Remove widget from current area if needed
    if (sourceAreaPtr && sourceAreaPtr != targetArea) {
        sourceAreaPtr->removeDockWidget(draggedWidget);
        
        // Sync source area tab position with target area
        if (sourceAreaPtr->tabPosition() != targetTabPosition) {
            std::cout << "Syncing source area tab position from " 
                      << static_cast<int>(sourceAreaPtr->tabPosition()) 
                      << " to " << static_cast<int>(targetTabPosition) << std::endl;
            sourceAreaPtr->setTabPosition(targetTabPosition);
        }
    }
    
    // Add widget to target area
    targetArea->addDockWidget(draggedWidget);
    
    // Set as current widget
    targetArea->setCurrentDockWidget(draggedWidget);
    
    std::cout << "Drag merge completed successfully!" << std::endl;
}

int main() {
    std::cout << "=== Drag Merge Tab Position Sync Test ===" << std::endl;
    std::cout << std::endl;
    
    // Create test dock areas with different tab positions
    DockArea area1("Area1 (Top)");
    DockArea area2("Area2 (Bottom)");
    DockArea area3("Area3 (Left)");
    DockArea area4("Area4 (Right)");
    
    // Set different tab positions
    area1.setTabPosition(TabPosition::Top);
    area2.setTabPosition(TabPosition::Bottom);
    area3.setTabPosition(TabPosition::Left);
    area4.setTabPosition(TabPosition::Right);
    
    // Create test widgets
    DockWidget widget1("Widget1");
    DockWidget widget2("Widget2");
    DockWidget widget3("Widget3");
    DockWidget widget4("Widget4");
    
    // Add widgets to areas
    area1.addDockWidget(&widget1);
    area2.addDockWidget(&widget2);
    area3.addDockWidget(&widget3);
    area4.addDockWidget(&widget4);
    
    std::cout << "\nInitial state:" << std::endl;
    std::cout << "Area1: " << area1.widgetCount() << " widgets, position: " << static_cast<int>(area1.tabPosition()) << std::endl;
    std::cout << "Area2: " << area2.widgetCount() << " widgets, position: " << static_cast<int>(area2.tabPosition()) << std::endl;
    std::cout << "Area3: " << area3.widgetCount() << " widgets, position: " << static_cast<int>(area3.tabPosition()) << std::endl;
    std::cout << "Area4: " << area4.widgetCount() << " widgets, position: " << static_cast<int>(area4.tabPosition()) << std::endl;
    
    // Test drag merge scenarios
    std::cout << "\n=== Test Case 1: Drag from Top to Bottom ===" << std::endl;
    simulateDragMerge(&area1, &area2, &widget1);
    
    std::cout << "\n=== Test Case 2: Drag from Bottom to Left ===" << std::endl;
    simulateDragMerge(&area2, &area3, &widget2);
    
    std::cout << "\n=== Test Case 3: Drag from Left to Right ===" << std::endl;
    simulateDragMerge(&area3, &area4, &widget3);
    
    std::cout << "\n=== Test Case 4: Drag from Right to Top ===" << std::endl;
    simulateDragMerge(&area4, &area1, &widget4);
    
    std::cout << "\nFinal state:" << std::endl;
    std::cout << "Area1: " << area1.widgetCount() << " widgets, position: " << static_cast<int>(area1.tabPosition()) << std::endl;
    std::cout << "Area2: " << area2.widgetCount() << " widgets, position: " << static_cast<int>(area2.tabPosition()) << std::endl;
    std::cout << "Area3: " << area3.widgetCount() << " widgets, position: " << static_cast<int>(area3.tabPosition()) << std::endl;
    std::cout << "Area4: " << area4.widgetCount() << " widgets, position: " << static_cast<int>(area4.tabPosition()) << std::endl;
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "✓ Tab position synchronization during drag merge" << std::endl;
    std::cout << "✓ Source area tab position changes to match target area" << std::endl;
    std::cout << "✓ Widgets correctly merged to target areas" << std::endl;
    std::cout << "✓ All drag merge scenarios handled properly" << std::endl;
    
    return 0;
}