# Stack Overflow Fix in Modern Dock Framework

## Problem Description

A stack overflow error occurred during dock panel drag operations:

```
Stack overflow (0xC00000FD) in CADNav.exe
Call stack:
- wxGetWindowRect(HWND__)
- wxWindow::DoGetPosition(int*, int*)
- wxWindowBase::GetPosition(int*, int*)
- wxWindowBase::GetRect()
- ModernDockManager::HitTest(const wxPoint&)
- DragDropController::FindTargetPanel(const wxPoint&)
```

## Root Cause Analysis

The stack overflow was caused by a recursive event loop during drag operations:

1. **Mouse Move Event** → `ModernDockManager::OnMouseMove`
2. **Drag Update** → `UpdateDrag` → `DragDropController::UpdateDrag`
3. **Target Detection** → `FindTargetPanel` → `ModernDockManager::HitTest`
4. **Layout Trigger** → `panel->GetRect()` triggers position calculation
5. **Event Cascade** → Position calculation generates more mouse events
6. **Infinite Loop** → Back to step 1, causing stack overflow

## Solution Implemented

### 1. Re-entrance Guards
Added static boolean flags to prevent recursive calls:

**In `ModernDockManager::OnMouseMove`:**
```cpp
void ModernDockManager::OnMouseMove(wxMouseEvent& event)
{
    static bool inMouseMove = false;
    if (inMouseMove) {
        event.Skip();
        return;
    }
    
    inMouseMove = true;
    // ... existing logic ...
    inMouseMove = false;
    event.Skip();
}
```

**In `DragDropController::FindTargetPanel`:**
```cpp
ModernDockPanel* DragDropController::FindTargetPanel(const wxPoint& screenPos) const
{
    static bool inFindTarget = false;
    if (inFindTarget) return nullptr;
    
    inFindTarget = true;
    ModernDockPanel* result = m_manager->HitTest(screenPos);
    inFindTarget = false;
    
    return result;
}
```

### 2. Safe Hit Testing
Modified `ModernDockManager::HitTest` to avoid layout-triggering calls:

**Before (problematic):**
```cpp
if (panel->IsShown() && panel->GetRect().Contains(clientPos)) {
    return panel;
}
```

**After (safe):**
```cpp
if (panel->IsShown()) {
    // Use screen coordinates to avoid layout triggers
    wxPoint panelScreenPos = panel->GetScreenPosition();
    wxSize panelSize = panel->GetSize();
    wxRect screenRect(panelScreenPos, panelSize);
    
    if (screenRect.Contains(screenPos)) {
        return panel;
    }
}
```

## Key Improvements

1. **Prevents Infinite Recursion**: Re-entrance guards break the recursive event loop
2. **Avoids Layout Triggers**: Direct use of screen coordinates instead of `GetRect()`
3. **Maintains Functionality**: Drag operations continue to work correctly
4. **Performance Boost**: Eliminates unnecessary layout recalculations

## Technical Notes

- **Re-entrance Guards**: Use static variables for simplicity and thread-safety in single-threaded UI
- **Screen Coordinates**: More reliable for hit testing during drag operations
- **Event Handling**: Proper `event.Skip()` calls maintain event propagation
- **Backward Compatibility**: No changes to public API or behavior

## Verification

- ✅ Compiles successfully without errors
- ✅ Drag operations work without crashes
- ✅ No stack overflow exceptions
- ✅ All dock framework features preserved

This fix ensures stable dock panel drag operations while maintaining the full feature set of the modern dock framework.
