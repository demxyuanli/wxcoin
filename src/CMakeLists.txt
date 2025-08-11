# src directory CMakeLists.txt - manages all submodules

# Set module build order (by dependency relationships)
# 1. Core module (no dependencies)
add_subdirectory(core)

# 2. Logger module (no dependencies)
add_subdirectory(logger)

# 3. Config module (depends on core and logger modules)
add_subdirectory(config)

# 4. OpenCASCADE module (depends on core module)
add_subdirectory(opencascade)

# 5. Commands module (depends on core and occ)
add_subdirectory(commands)

# 6. Rendering module (depends on core module)
add_subdirectory(rendering)

# 6.2. View module (depends on rendering)
add_subdirectory(view)

# 6.5. Rendering Toolkit module (depends on core, logger, config modules)
add_subdirectory(rendering_toolkit)

# 7. Input module (depends on core module)
add_subdirectory(input)

# 8. Navigation module (depends on core and rendering modules)
add_subdirectory(navigation)

# 9. FlatUI module (depends on core and config modules)
add_subdirectory(flatui)

# 9.5. Widgets module (depends on core and config modules)
add_subdirectory(widgets)

# 10. UI module (depends on all other modules)
add_subdirectory(ui)
add_subdirectory(renderpreview)

# Set module folder structure (for IDE organization)
set_target_properties(CADCore PROPERTIES FOLDER "Modules")
set_target_properties(CADLogger PROPERTIES FOLDER "Modules")
set_target_properties(CADConfig PROPERTIES FOLDER "Modules")
set_target_properties(CADOCC PROPERTIES FOLDER "Modules")
set_target_properties(CADCommands PROPERTIES FOLDER "Modules")
set_target_properties(CADRendering PROPERTIES FOLDER "Modules")
set_target_properties(CADView PROPERTIES FOLDER "Modules")
set_target_properties(CADRenderingToolkit PROPERTIES FOLDER "Modules")
set_target_properties(CADInput PROPERTIES FOLDER "Modules")
set_target_properties(NavCubeLib PROPERTIES FOLDER "Modules")
set_target_properties(CADFlatUI PROPERTIES FOLDER "Modules")
set_target_properties(widgets PROPERTIES FOLDER "Modules")
set_target_properties(UIPanels PROPERTIES FOLDER "Modules")
set_target_properties(UIDialogs PROPERTIES FOLDER "Modules")
set_target_properties(UIFrame PROPERTIES FOLDER "Modules")