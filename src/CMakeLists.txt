# src directory CMakeLists.txt - manages all submodules

# Set module build order (by dependency relationships)
# 1. Core module (no dependencies)
add_subdirectory(core)

# 2. OpenCASCADE module (depends on core module)
add_subdirectory(opencascade)

# 3. Commands module (depends on core and occ)
add_subdirectory(commands)

# 4. Rendering module (depends on core module)
add_subdirectory(rendering)

# 5. Input module (depends on core module)
add_subdirectory(input)

# 6. Navigation module (depends on core and rendering modules)
add_subdirectory(navigation)

# 7. UI module (depends on all other modules)
add_subdirectory(ui)

# Set module folder structure (for IDE organization)
set_target_properties(CADCore PROPERTIES FOLDER "Modules")
set_target_properties(CADOCC PROPERTIES FOLDER "Modules")
set_target_properties(CADCommands PROPERTIES FOLDER "Modules")
set_target_properties(CADRendering PROPERTIES FOLDER "Modules")
set_target_properties(CADInput PROPERTIES FOLDER "Modules")
set_target_properties(NavCubeLib PROPERTIES FOLDER "Modules")
set_target_properties(CADUI PROPERTIES FOLDER "Modules") 