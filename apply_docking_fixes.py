#!/usr/bin/env python3
"""
Script to apply docking system fixes to resolve compilation errors
Run this from your project root: D:\source\repos\wxcoin
"""

import os
import re

# Dictionary of files and their required fixes
fixes = {
    'include/docking/DockWidget.h': [
        # Fix InsertMode parameter
        (r'void setWidget\(wxWindow\* widget, InsertMode insertMode = AutoScrollArea\);',
         r'void setWidget(wxWindow* widget, InsertMode insertMode = InsertMode::AutoScrollArea);'),
        
        # Fix event declarations
        (r'wxDECLARE_EVENT\(EVT_DOCK_WIDGET_CLOSED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_CLOSED;'),
        (r'wxDECLARE_EVENT\(EVT_DOCK_WIDGET_CLOSING, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_CLOSING;'),
        (r'wxDECLARE_EVENT\(EVT_DOCK_WIDGET_VISIBILITY_CHANGED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_VISIBILITY_CHANGED;'),
        (r'wxDECLARE_EVENT\(EVT_DOCK_WIDGET_FEATURES_CHANGED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_FEATURES_CHANGED;'),
    ],
    
    'include/docking/DockArea.h': [
        # Fix DockArea events
        (r'wxDECLARE_EVENT\(EVT_DOCK_AREA_CURRENT_CHANGED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_CURRENT_CHANGED;'),
        (r'wxDECLARE_EVENT\(EVT_DOCK_AREA_CLOSING, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_CLOSING;'),
        (r'wxDECLARE_EVENT\(EVT_DOCK_AREA_CLOSED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_CLOSED;'),
        (r'wxDECLARE_EVENT\(EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE;'),
        
        # Fix DockAreaTabBar events
        (r'wxDECLARE_EVENT\(EVT_TAB_CLOSE_REQUESTED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_TAB_CLOSE_REQUESTED;'),
        (r'wxDECLARE_EVENT\(EVT_TAB_CURRENT_CHANGED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_TAB_CURRENT_CHANGED;'),
        (r'wxDECLARE_EVENT\(EVT_TAB_MOVED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_TAB_MOVED;'),
        
        # Fix DockAreaTitleBar events
        (r'wxDECLARE_EVENT\(EVT_TITLE_BAR_BUTTON_CLICKED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_TITLE_BAR_BUTTON_CLICKED;'),
    ],
    
    'include/docking/DockContainerWidget.h': [
        (r'wxDECLARE_EVENT\(EVT_DOCK_AREAS_ADDED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREAS_ADDED;'),
        (r'wxDECLARE_EVENT\(EVT_DOCK_AREAS_REMOVED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREAS_REMOVED;'),
    ],
    
    'include/docking/FloatingDockContainer.h': [
        # Add forward declarations after namespace opening
        (r'(namespace ads \{[^}]*class DockContainerWidget;)',
         r'\1\n\n// Import types - these should be defined in DockManager.h\nenum DockWidgetArea : int;\nenum DockManagerFeature : int;'),
        
        # Fix events
        (r'wxDECLARE_EVENT\(EVT_FLOATING_CONTAINER_CLOSING, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_FLOATING_CONTAINER_CLOSING;'),
        (r'wxDECLARE_EVENT\(EVT_FLOATING_CONTAINER_CLOSED, wxCommandEvent\);',
         r'static wxEventTypeTag<wxCommandEvent> EVT_FLOATING_CONTAINER_CLOSED;'),
    ],
    
    'include/docking/DockOverlay.h': [
        (r'void setAllowedAreas\(DockWidgetAreas areas\);',
         r'void setAllowedAreas(int areas);'),
        (r'DockWidgetAreas allowedAreas\(\) const \{ return m_allowedAreas; \}',
         r'int allowedAreas() const { return m_allowedAreas; }'),
        (r'DockWidgetAreas m_allowedAreas;',
         r'int m_allowedAreas;'),
    ],
    
    'src/docking/DockManager.cpp': [
        # Add missing include after other includes
        (r'(#include <wx/xml/xml\.h>)',
         r'\1\n#include <wx/sstream.h>'),
    ],
    
    'src/docking/DockWidget.cpp': [
        # Fix event definitions
        (r'wxDEFINE_EVENT\(DockWidget::EVT_DOCK_WIDGET_CLOSED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_CLOSED("DockWidget::EVT_DOCK_WIDGET_CLOSED");'),
        (r'wxDEFINE_EVENT\(DockWidget::EVT_DOCK_WIDGET_CLOSING, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_CLOSING("DockWidget::EVT_DOCK_WIDGET_CLOSING");'),
        (r'wxDEFINE_EVENT\(DockWidget::EVT_DOCK_WIDGET_VISIBILITY_CHANGED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_VISIBILITY_CHANGED("DockWidget::EVT_DOCK_WIDGET_VISIBILITY_CHANGED");'),
        (r'wxDEFINE_EVENT\(DockWidget::EVT_DOCK_WIDGET_FEATURES_CHANGED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_FEATURES_CHANGED("DockWidget::EVT_DOCK_WIDGET_FEATURES_CHANGED");'),
        
        # Fix InsertMode usage
        (r'if \(insertMode == AutoScrollArea \|\| insertMode == ForceScrollArea\)',
         r'if (insertMode == InsertMode::AutoScrollArea || insertMode == InsertMode::ForceScrollArea)'),
        (r'if \(insertMode == ForceScrollArea\)',
         r'if (insertMode == InsertMode::ForceScrollArea)'),
        
        # Fix const cast
        (r'return m_dockManager->isAutoHide\(this\);',
         r'return m_dockManager->isAutoHide(const_cast<DockWidget*>(this));'),
    ],
    
    'src/docking/DockArea.cpp': [
        # Fix event definitions
        (r'wxDEFINE_EVENT\(DockArea::EVT_DOCK_AREA_CURRENT_CHANGED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_CURRENT_CHANGED("DockArea::EVT_DOCK_AREA_CURRENT_CHANGED");'),
        (r'wxDEFINE_EVENT\(DockArea::EVT_DOCK_AREA_CLOSING, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_CLOSING("DockArea::EVT_DOCK_AREA_CLOSING");'),
        (r'wxDEFINE_EVENT\(DockArea::EVT_DOCK_AREA_CLOSED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_CLOSED("DockArea::EVT_DOCK_AREA_CLOSED");'),
        (r'wxDEFINE_EVENT\(DockArea::EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE("DockArea::EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE");'),
        
        (r'wxDEFINE_EVENT\(DockAreaTabBar::EVT_TAB_CLOSE_REQUESTED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockAreaTabBar::EVT_TAB_CLOSE_REQUESTED("DockAreaTabBar::EVT_TAB_CLOSE_REQUESTED");'),
        (r'wxDEFINE_EVENT\(DockAreaTabBar::EVT_TAB_CURRENT_CHANGED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockAreaTabBar::EVT_TAB_CURRENT_CHANGED("DockAreaTabBar::EVT_TAB_CURRENT_CHANGED");'),
        (r'wxDEFINE_EVENT\(DockAreaTabBar::EVT_TAB_MOVED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockAreaTabBar::EVT_TAB_MOVED("DockAreaTabBar::EVT_TAB_MOVED");'),
        
        (r'wxDEFINE_EVENT\(DockAreaTitleBar::EVT_TITLE_BAR_BUTTON_CLICKED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockAreaTitleBar::EVT_TITLE_BAR_BUTTON_CLICKED("DockAreaTitleBar::EVT_TITLE_BAR_BUTTON_CLICKED");'),
        
        # Fix SetVisible
        (r'SetVisible\(open\);',
         r'Show(open);'),
        
        # Fix unicode characters
        (r'"▶ "',
         r'"> "'),
        (r'"📌"',
         r'"P"'),
    ],
    
    'src/docking/DockContainerWidget.cpp': [
        # Fix event definitions
        (r'wxDEFINE_EVENT\(DockContainerWidget::EVT_DOCK_AREAS_ADDED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_ADDED("DockContainerWidget::EVT_DOCK_AREAS_ADDED");'),
        (r'wxDEFINE_EVENT\(DockContainerWidget::EVT_DOCK_AREAS_REMOVED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_REMOVED("DockContainerWidget::EVT_DOCK_AREAS_REMOVED");'),
        
        # Fix splitter issues
        (r'm_rootSplitter->Detach\(area\);',
         r'if (wxSizer* sizer = m_rootSplitter->GetSizer()) {\n            sizer->Detach(area);\n        }'),
        
        # Fix splitter window casting
        (r'if \(m_rootSplitter->GetWindow1\(\) == nullptr\) \{',
         r'if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(m_rootSplitter)) {\n        if (splitter->GetWindow1() == nullptr) {'),
    ],
    
    'src/docking/FloatingDockContainer.cpp': [
        # Fix event definitions
        (r'wxDEFINE_EVENT\(FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSING, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSING("FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSING");'),
        (r'wxDEFINE_EVENT\(FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSED, wxCommandEvent\);',
         r'wxEventTypeTag<wxCommandEvent> FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSED("FloatingDockContainer::EVT_FLOATING_CONTAINER_CLOSED");'),
        
        # Fix method signature
        (r'void FloatingDockContainer::addDockWidget\(DockWidget\* dockWidget, DockWidgetArea area\)',
         r'void FloatingDockContainer::addDockWidget(DockWidget* dockWidget)'),
        
        # Fix method call
        (r'addDockWidget\(dockWidget, CenterDockWidgetArea\);',
         r'addDockWidget(dockWidget);'),
        
        # Fix return type
        (r'void FloatingDockContainer::testConfigFlag\(DockManagerFeature flag\) const \{',
         r'bool FloatingDockContainer::testConfigFlag(DockManagerFeature flag) const {'),
    ],
    
    'src/docking/AutoHideContainer.cpp': [
        # Fix unicode character
        (r'"📌"',
         r'"P"'),
    ],
}

def apply_fixes():
    """Apply all fixes to the source files"""
    fixed_files = []
    errors = []
    
    for filepath, replacements in fixes.items():
        try:
            full_path = os.path.join(os.getcwd(), filepath)
            
            if not os.path.exists(full_path):
                errors.append(f"File not found: {full_path}")
                continue
            
            # Read file
            with open(full_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            
            # Apply replacements
            for pattern, replacement in replacements:
                content = re.sub(pattern, replacement, content)
            
            # Write back if changed
            if content != original_content:
                with open(full_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                fixed_files.append(filepath)
                print(f"✓ Fixed: {filepath}")
            else:
                print(f"  No changes needed: {filepath}")
                
        except Exception as e:
            errors.append(f"Error processing {filepath}: {str(e)}")
    
    # Summary
    print("\n" + "="*60)
    print(f"Fixed {len(fixed_files)} files")
    
    if errors:
        print(f"\nErrors encountered:")
        for error in errors:
            print(f"  - {error}")
    
    print("\nNext steps:")
    print("1. Review the changes")
    print("2. Run: cmake --build ../build --config Release")

if __name__ == "__main__":
    print("Applying docking system compilation fixes...")
    print("="*60)
    apply_fixes()