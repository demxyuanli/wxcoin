# Docking System Migration Guide

## Overview

This guide explains how to migrate from the old docking implementations to the new `ModernDockManager` system.

## Current Architecture

### Old Implementations (Deprecated)
- **`UnifiedDockManager`** - Old unified docking implementation
- **`FlatDockManager`** - Old flat docking implementation  
- **`FlatDockContainer`** - Old container implementation

### New Implementation
- **`ModernDockManager`** - New modern docking implementation with VS2022-style features
- **`ModernDockAdapter`** - Compatibility adapter for existing code

## Migration Strategy

### Option 1: Direct Replacement (Recommended)

Since both `UnifiedDockManager` and `ModernDockManager` implement the `IDockManager` interface, you can directly replace the old implementation:

```cpp
// OLD CODE
#include "widgets/UnifiedDockManager.h"
auto* dock = new UnifiedDockManager(this);

// NEW CODE  
#include "widgets/ModernDockManager.h"
auto* dock = new ModernDockManager(this);
```

### Option 2: Use Compatibility Adapter

For code that uses the old `FlatDockManager` API, use `ModernDockAdapter`:

```cpp
// OLD CODE
#include "widgets/FlatDockManager.h"
auto* dock = new FlatDockManager(this);
dock->AddPane(panel, FlatDockManager::DockPos::LeftTop, 200);

// NEW CODE
#include "widgets/ModernDockAdapter.h"
auto* dock = new ModernDockAdapter(this);
dock->AddPane(panel, ModernDockAdapter::DockPos::LeftTop, 200);
```

## Step-by-Step Migration

### Step 1: Update Includes

Replace old includes with new ones:

```cpp
// Replace these includes:
#include "widgets/UnifiedDockManager.h"
#include "widgets/FlatDockManager.h"
#include "widgets/FlatDockContainer.h"

// With these:
#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockAdapter.h"  // If you need compatibility API
```

### Step 2: Update Class Names

```cpp
// Replace class names in variable declarations:
UnifiedDockManager* m_dockManager;
FlatDockManager* m_dockManager;

// With:
ModernDockManager* m_dockManager;
```

### Step 3: Update Constructor Calls

```cpp
// Replace constructor calls:
m_dockManager = new UnifiedDockManager(this);
m_dockManager = new FlatDockManager(this);

// With:
m_dockManager = new ModernDockManager(this);
```

### Step 4: Update Method Calls

Most method calls remain the same since both implement `IDockManager`:

```cpp
// These calls work the same way:
m_dockManager->AddPanel(content, title, area);
m_dockManager->RemovePanel(content);
m_dockManager->ShowPanel(content);
m_dockManager->HidePanel(content);
```

## API Compatibility

### Fully Compatible Methods

All methods from `IDockManager` interface are fully compatible:

- Panel management: `AddPanel`, `RemovePanel`, `ShowPanel`, `HidePanel`
- Layout strategy: `SetLayoutStrategy`, `GetLayoutStrategy`
- Layout persistence: `SaveLayout`, `RestoreLayout`
- Panel positioning: `DockPanel`, `UndockPanel`, `FloatPanel`, `TabifyPanel`
- Visual feedback: `ShowDockGuides`, `HideDockGuides`
- Drag and drop: `StartDrag`, `UpdateDrag`, `EndDrag`

### New Features in ModernDockManager

- Improved performance and memory management
- Better visual feedback during drag operations
- Enhanced layout algorithms
- Modern UI styling (VS2022-style)

## Testing Migration

### 1. Compile and Fix Errors

After replacing the includes and class names, compile your project and fix any compilation errors.

### 2. Test Basic Functionality

Test these basic operations:
- Adding panels
- Removing panels
- Docking/undocking panels
- Saving/restoring layouts

### 3. Test Advanced Features

Test advanced features if you use them:
- Layout strategies
- Drag and drop operations
- Custom docking guides

## Troubleshooting

### Common Issues

1. **Compilation Errors**: Ensure all includes are updated
2. **Runtime Errors**: Check that the new manager is properly initialized
3. **Layout Issues**: The new system may have slightly different default layouts

### Getting Help

If you encounter issues during migration:
1. Check that you're using the correct include paths
2. Verify that all old references are replaced
3. Ensure the new manager is properly initialized

## Performance Benefits

The new `ModernDockManager` provides:
- **Faster rendering**: Optimized drawing algorithms
- **Better memory usage**: Improved memory management
- **Smoother animations**: Hardware-accelerated graphics
- **Enhanced responsiveness**: Better event handling

## Future Plans

- `UnifiedDockManager` will be removed in a future version
- `FlatDockManager` will be maintained for backward compatibility
- All new features will be added to `ModernDockManager`

## Conclusion

The migration to `ModernDockManager` is straightforward and provides significant benefits. The compatibility layer ensures that existing code continues to work while you can gradually adopt the new features.



