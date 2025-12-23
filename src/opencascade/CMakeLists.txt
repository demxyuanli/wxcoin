# OpenCASCADE module - OpenCASCADE geometry and viewer functionality

# Set source files
set(OCC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCGeometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCBrepConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCMeshConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCShapeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/GeomCoinRepresentation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryAppearance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryCore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryMaterial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryPrimitives.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryQuality.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryTransform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/VertexExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/BRepDisplayModeHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/CoinNodeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeConfigDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeConfigFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeNodeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModePreviewCanvas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/FaceDomainMapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/MeshDisplayModeHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/PointViewBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/RenderNodeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/WireframeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/AsyncEdgeIntersectionComputer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/AsyncIntersectionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/AsyncIntersectionTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeDisplayManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeExtractionUIHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeGenerationService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeGeometryCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeIntersectionAccelerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeLODManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeProcessorFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeRenderApplier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/ModularEdgeComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/BaseEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/FeatureEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/MeshEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/OriginalEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/SilhouetteEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/BaseEdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/FeatureEdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/MeshEdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/OriginalEdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/BatchOperationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ConfigurationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ExplodeController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/GeometryFactoryService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/GeometryManagementService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/GeometryRepository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/HoverSilhouetteManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ImageOutlinePass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/LODController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshingService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshParameterAdvisor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshParameterController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshQualityService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshQualityValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/NormalDisplayService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ObjectTreeSync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/OutlineDisplayManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/PickingService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/RenderingController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/RenderModeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SceneAttachmentService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SelectionAcceleratorService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SelectionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SelectionOutlineManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SliceController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ViewOperationsService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ViewportController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ViewUpdateService.cpp
)

# Set header files (for IDE organization)
set(OCC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/OCCGeometry.h
    ${CMAKE_SOURCE_DIR}/include/OCCViewer.h
    ${CMAKE_SOURCE_DIR}/include/OCCBrepConverter.h
    ${CMAKE_SOURCE_DIR}/include/OCCMeshConverter.h
    ${CMAKE_SOURCE_DIR}/include/OCCShapeBuilder.h
)

# Create OpenCASCADE library
add_library(CADOCC STATIC ${OCC_SOURCES} ${OCC_HEADERS})

# Set target properties
set_target_properties(CADOCC PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(CADOCC PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(CADOCC PUBLIC
    CADCore
    CADLogger
    widgets
    ${OpenCASCADE_LIBRARIES}
    Coin::Coin
    TBB::tbb
)

# Compiler definitions
target_compile_definitions(CADOCC PRIVATE
    CADOCC_EXPORTS
)

# MSVC specific settings
if(MSVC)
    target_compile_options(CADOCC PRIVATE /std:c++17)
endif()

# Set module folder for IDE organization
set_target_properties(CADOCC PROPERTIES FOLDER "Modules")
