
# src directory CMakeLists.txt - manages all submodules

# Set module build order (by dependency relationships)
# 1. Core module (no dependencies)
add_subdirectory(core)

# 2. Logger module (no dependencies)
add_subdirectory(logger)

# 3. Param module (depends on core and logger modules)
add_subdirectory(param)

# 3. Config module (depends on core and logger modules)
add_subdirectory(config)

# 4. OpenCASCADE module (depends on core module)
add_subdirectory(opencascade)

# 4.1. Async module (depends on logger and occ modules)
add_subdirectory(async)

# 5. Geometry module (depends on core, logger, and occ modules)
add_subdirectory(geometry)

# 6. Commands module (depends on core and occ)
add_subdirectory(commands)

# 6.1. Optimizer module (depends on core, logger, commands, and occ)
add_subdirectory(optimizer)

# 7. Rendering module (depends on core module)
add_subdirectory(rendering)

# 7.1. View module (depends on rendering)
add_subdirectory(view)

# 7.2. Rendering Toolkit module (depends on core, logger, config modules)
add_subdirectory(rendering_toolkit)

# 8. Input module (depends on core module)
add_subdirectory(input)

# 9. Navigation module (depends on core and rendering modules)
add_subdirectory(navigation)

# 10. FlatUI module (depends on core and config modules)
add_subdirectory(flatui)

# 10.1. Widgets module (depends on core and config modules)
add_subdirectory(widgets)

# 10.2. Docking module (depends on core module)
add_subdirectory(docking)

# 11. UI module (depends on all other modules)
add_subdirectory(ui)
add_subdirectory(renderpreview)

# Set module folder structure (for IDE organization)
set_target_properties(CADCore PROPERTIES FOLDER "Modules")
set_target_properties(CADLogger PROPERTIES FOLDER "Modules")
set_target_properties(CADParam PROPERTIES FOLDER "Modules")
set_target_properties(CADConfig PROPERTIES FOLDER "Modules")
set_target_properties(CADOCC PROPERTIES FOLDER "Modules")
set_target_properties(AsyncEngine PROPERTIES FOLDER "Modules")
set_target_properties(CADGeometry PROPERTIES FOLDER "Modules")
set_target_properties(CADCommands PROPERTIES FOLDER "Modules")
set_target_properties(optimizer PROPERTIES FOLDER "Modules")
set_target_properties(CADRendering PROPERTIES FOLDER "Modules")
set_target_properties(CADView PROPERTIES FOLDER "Modules")
set_target_properties(CADRenderingToolkit PROPERTIES FOLDER "Modules")
set_target_properties(CADInput PROPERTIES FOLDER "Modules")
set_target_properties(NavCubeLib PROPERTIES FOLDER "Modules")
set_target_properties(CADFlatUI PROPERTIES FOLDER "Modules")
set_target_properties(widgets PROPERTIES FOLDER "Modules")
set_target_properties(docking PROPERTIES FOLDER "Modules")
set_target_properties(UIPanels PROPERTIES FOLDER "Modules")
set_target_properties(UIDialogs PROPERTIES FOLDER "Modules")
set_target_properties(UIFrame PROPERTIES FOLDER "Modules")