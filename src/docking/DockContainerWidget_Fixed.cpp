// 这是修复后的布局管理策略示例

void DockContainerWidget::createFixedLayout() {
    // 创建固定的五区域布局结构
    // 
    // 整体结构：
    // +--------------------------------+
    // |          Top Area              |
    // +--------+----------------+------+
    // | Left   |   Center       | Right|
    // | Area   |   Area         | Area |
    // +--------+----------------+------+
    // |          Bottom Area           |
    // +--------------------------------+
    
    // 1. 创建主垂直分割器 (分割成上、中、下三部分)
    DockSplitter* mainVerticalSplitter = new DockSplitter(this);
    mainVerticalSplitter->setOrientation(wxVERTICAL);
    
    // 2. 创建中间的水平分割器 (分割成左、中、右三部分)
    DockSplitter* middleHorizontalSplitter = new DockSplitter(mainVerticalSplitter);
    middleHorizontalSplitter->setOrientation(wxHORIZONTAL);
    
    // 3. 创建五个占位容器
    wxPanel* topContainer = new wxPanel(mainVerticalSplitter);
    wxPanel* leftContainer = new wxPanel(middleHorizontalSplitter);
    wxPanel* centerContainer = new wxPanel(middleHorizontalSplitter);
    wxPanel* rightContainer = new wxPanel(middleHorizontalSplitter);
    wxPanel* bottomContainer = new wxPanel(mainVerticalSplitter);
    
    // 4. 设置中间水平分割器
    middleHorizontalSplitter->SplitVertically(leftContainer, centerContainer);
    DockSplitter* rightSplitter = new DockSplitter(middleHorizontalSplitter);
    middleHorizontalSplitter->ReplaceWindow(centerContainer, rightSplitter);
    rightSplitter->SplitVertically(centerContainer, rightContainer);
    
    // 5. 设置主垂直分割器
    mainVerticalSplitter->SplitHorizontally(topContainer, middleHorizontalSplitter);
    DockSplitter* bottomSplitter = new DockSplitter(mainVerticalSplitter);
    mainVerticalSplitter->ReplaceWindow(middleHorizontalSplitter, bottomSplitter);
    bottomSplitter->SplitHorizontally(middleHorizontalSplitter, bottomContainer);
    
    // 6. 将主分割器添加到布局
    m_layout->Add(mainVerticalSplitter, 1, wxEXPAND);
    
    // 7. 保存区域引用
    m_areaContainers[TopDockWidgetArea] = topContainer;
    m_areaContainers[LeftDockWidgetArea] = leftContainer;
    m_areaContainers[CenterDockWidgetArea] = centerContainer;
    m_areaContainers[RightDockWidgetArea] = rightContainer;
    m_areaContainers[BottomDockWidgetArea] = bottomContainer;
    
    // 8. 初始隐藏空容器，设置最小尺寸
    topContainer->Hide();
    leftContainer->Hide();
    rightContainer->Hide();
    bottomContainer->Hide();
    
    topContainer->SetMinSize(wxSize(-1, 100));
    leftContainer->SetMinSize(wxSize(200, -1));
    centerContainer->SetMinSize(wxSize(400, 300));
    rightContainer->SetMinSize(wxSize(200, -1));
    bottomContainer->SetMinSize(wxSize(-1, 150));
}

DockArea* DockContainerWidget::addDockWidgetFixed(DockWidgetArea area, DockWidget* dockWidget) {
    // 获取对应区域的容器
    wxPanel* container = m_areaContainers[area];
    if (!container) {
        // 如果没有预定义的区域，回退到原来的方法
        return addDockWidget(area, dockWidget, nullptr, -1);
    }
    
    // 检查容器是否已有 DockArea
    DockArea* existingArea = nullptr;
    for (auto* child : container->GetChildren()) {
        if ((existingArea = dynamic_cast<DockArea*>(child))) {
            break;
        }
    }
    
    if (existingArea) {
        // 如果已有 DockArea，添加为标签页
        existingArea->addDockWidget(dockWidget);
        return existingArea;
    } else {
        // 创建新的 DockArea
        DockArea* newArea = new DockArea(m_dockManager, container);
        newArea->addDockWidget(dockWidget);
        
        // 创建容器的布局
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(newArea, 1, wxEXPAND);
        container->SetSizer(sizer);
        
        // 显示容器
        container->Show();
        
        // 调整分割器位置
        adjustSplittersForArea(area);
        
        m_dockAreas.push_back(newArea);
        return newArea;
    }
}

void DockContainerWidget::adjustSplittersForArea(DockWidgetArea area) {
    // 根据添加的区域调整分割器的位置
    // 这里需要根据实际需求调整各个区域的默认大小
    
    switch (area) {
        case TopDockWidgetArea:
            // 设置顶部区域占 20% 的高度
            if (auto* splitter = getTopSplitter()) {
                int totalHeight = splitter->GetSize().GetHeight();
                splitter->SetSashPosition(totalHeight * 0.2);
            }
            break;
            
        case BottomDockWidgetArea:
            // 设置底部区域占 25% 的高度
            if (auto* splitter = getBottomSplitter()) {
                int totalHeight = splitter->GetSize().GetHeight();
                splitter->SetSashPosition(totalHeight * 0.75);
            }
            break;
            
        case LeftDockWidgetArea:
            // 设置左侧区域占 20% 的宽度
            if (auto* splitter = getLeftSplitter()) {
                int totalWidth = splitter->GetSize().GetWidth();
                splitter->SetSashPosition(totalWidth * 0.2);
            }
            break;
            
        case RightDockWidgetArea:
            // 设置右侧区域占 20% 的宽度
            if (auto* splitter = getRightSplitter()) {
                int totalWidth = splitter->GetSize().GetWidth();
                splitter->SetSashPosition(totalWidth * 0.8);
            }
            break;
            
        default:
            break;
    }
}