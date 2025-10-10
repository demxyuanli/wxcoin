# OpenCASCADE module - CAD geometry kernel and modeling
set(OPENCASCADE_SOURCES
    # Main implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCGeometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCShapeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCBrepConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCMeshConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EdgeComponent.cpp
    
    # Modular geometry implementations (internal)
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryCore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryTransform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryMaterial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryAppearance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryQuality.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryMesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryPrimitives.cpp
    
    # Modular viewer implementations (internal)
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ViewportController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/RenderingController.cpp
    
    # Existing viewer controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SliceController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ExplodeController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/LODController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/PickingService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SelectionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ObjectTreeSync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/GeometryRepository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SceneAttachmentService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ViewUpdateService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshingService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshParameterController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshParameterAdvisor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/HoverSilhouetteManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/BatchOperationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/OutlineDisplayManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SelectionOutlineManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ImageOutlinePass.cpp
    
    # Edge display modules
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeDisplayManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeGenerationService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeRenderApplier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeGeometryCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeLODManager.cpp
)

set(OPENCASCADE_HEADERS
    # Main headers
    ${CMAKE_SOURCE_DIR}/include/OCCGeometry.h
    ${CMAKE_SOURCE_DIR}/include/OCCViewer.h
    ${CMAKE_SOURCE_DIR}/include/EdgeComponent.h
    ${CMAKE_SOURCE_DIR}/include/OCCShapeBuilder.h
    ${CMAKE_SOURCE_DIR}/include/OCCBrepConverter.h
    
    # Internal geometry module headers
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryCore.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryTransform.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryMaterial.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryAppearance.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryDisplay.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryQuality.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryMesh.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryPrimitives.h
    
    # Internal viewer module headers
    ${CMAKE_SOURCE_DIR}/include/viewer/ViewportController.h
    ${CMAKE_SOURCE_DIR}/include/viewer/RenderingController.h
    
    # Internal edge module headers
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeExtractor.h
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeRenderer.h
    ${CMAKE_SOURCE_DIR}/include/viewer/SliceController.h
    ${CMAKE_SOURCE_DIR}/include/viewer/ExplodeController.h
    ${CMAKE_SOURCE_DIR}/include/viewer/ExplodeTypes.h
    ${CMAKE_SOURCE_DIR}/include/viewer/LODController.h
    ${CMAKE_SOURCE_DIR}/include/viewer/PickingService.h
    ${CMAKE_SOURCE_DIR}/include/viewer/SelectionManager.h
    ${CMAKE_SOURCE_DIR}/include/viewer/ObjectTreeSync.h
    ${CMAKE_SOURCE_DIR}/include/viewer/GeometryRepository.h
    ${CMAKE_SOURCE_DIR}/include/viewer/SceneAttachmentService.h
    ${CMAKE_SOURCE_DIR}/include/viewer/ViewUpdateService.h
    ${CMAKE_SOURCE_DIR}/include/viewer/MeshingService.h
    ${CMAKE_SOURCE_DIR}/include/viewer/MeshParameterController.h
    ${CMAKE_SOURCE_DIR}/include/viewer/HoverSilhouetteManager.h
    ${CMAKE_SOURCE_DIR}/include/viewer/BatchOperationManager.h
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeDisplayManager.h
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeGenerationService.h
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeRenderApplier.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/IGeometryApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/ISelectionApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/IRenderModesApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/IMeshControlApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/ILODApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/IEdgeDisplayApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/ISliceApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/IExplodeApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/IViewApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/interfaces/IOutlineApi.h
    ${CMAKE_SOURCE_DIR}/include/viewer/OutlineDisplayManager.h
    ${CMAKE_SOURCE_DIR}/include/viewer/SelectionOutlineManager.h
    ${CMAKE_SOURCE_DIR}/include/viewer/ImageOutlinePass.h

)

# Create OpenCASCADE static library
add_library(CADOCC STATIC ${OPENCASCADE_SOURCES} ${OPENCASCADE_HEADERS})

# Set header directories
target_include_directories(CADOCC PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

# Enable precompiled headers for faster compilation
if(MSVC)
    target_precompile_headers(CADOCC PRIVATE
        <vector>
        <string>
        <memory>
        <map>
        <unordered_map>
        <algorithm>
        <Inventor/nodes/SoSeparator.h>
        <Inventor/nodes/SoCoordinate3.h>
        <Inventor/nodes/SoIndexedFaceSet.h>
    )
endif()

# Link dependencies
target_link_libraries(CADOCC PUBLIC
    CADCore
    CADLogger
    CADView
    CADRenderingToolkit
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Set library export properties
set_target_properties(CADOCC PROPERTIES
    FOLDER "OpenCASCADE"
    EXPORT_NAME OCC
) 