# OpenCASCADE module - CAD geometry kernel and modeling
set(OPENCASCADE_SOURCES
    # Main implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCGeometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCShapeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCBrepConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OCCMeshConverter.cpp

    
    # Modular geometry implementations (internal)
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryCore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryTransform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryMaterial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryAppearance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryQuality.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/GeomCoinRepresentation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/OCCGeometryPrimitives.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/VertexExtractor.cpp
    
    # Helper classes for GeomCoinRepresentation
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/CoinNodeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/RenderNodeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/BRepDisplayModeHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/MeshDisplayModeHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeNodeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/DisplayModeConfigFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/WireframeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/PointViewBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/helper/FaceDomainMapper.cpp
    
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
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/NormalDisplayService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/RenderModeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/GeometryManagementService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ViewOperationsService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/GeometryFactoryService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ConfigurationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshQualityService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshParameterAdvisor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/HoverSilhouetteManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/BatchOperationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/OutlineDisplayManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SelectionOutlineManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/ImageOutlinePass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/MeshQualityValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/SelectionAcceleratorService.cpp
    
    # Edge display modules
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeDisplayManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeGenerationService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeRenderApplier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeGeometryCache.cpp
    
    # Edge extractors (modular)
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/BaseEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/OriginalEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/FeatureEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/MeshEdgeExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/extractors/SilhouetteEdgeExtractor.cpp

    # Edge intersection accelerator (performance optimization)
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeIntersectionAccelerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeExtractionUIHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/AsyncIntersectionTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/AsyncIntersectionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/AsyncEdgeIntersectionComputer.cpp
    
    # Edge renderers (modular)
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/BaseEdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/OriginalEdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/FeatureEdgeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/renderers/MeshEdgeRenderer.cpp

    # Modular edge component
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/ModularEdgeComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeProcessorFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/edges/EdgeLODManager.cpp
)

set(OPENCASCADE_HEADERS
    # Main headers
    ${CMAKE_SOURCE_DIR}/include/OCCGeometry.h
    ${CMAKE_SOURCE_DIR}/include/OCCViewer.h

    ${CMAKE_SOURCE_DIR}/include/OCCShapeBuilder.h
    ${CMAKE_SOURCE_DIR}/include/OCCBrepConverter.h
    
    # Internal geometry module headers
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryCore.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryTransform.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryMaterial.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryAppearance.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryDisplay.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryQuality.h
    ${CMAKE_SOURCE_DIR}/include/geometry/GeomCoinRepresentation.h
    ${CMAKE_SOURCE_DIR}/include/geometry/VertexExtractor.h
    ${CMAKE_SOURCE_DIR}/include/geometry/OCCGeometryPrimitives.h
    
    # Helper class headers
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/CoinNodeManager.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/RenderNodeBuilder.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/DisplayModeHandler.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/BRepDisplayModeHandler.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/MeshDisplayModeHandler.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/DisplayModeStateManager.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/DisplayModeNodeManager.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/DisplayModeRenderer.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/WireframeBuilder.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/PointViewBuilder.h
    ${CMAKE_SOURCE_DIR}/include/geometry/helper/FaceDomainMapper.h
    
    # Internal viewer module headers
    ${CMAKE_SOURCE_DIR}/include/viewer/ViewportController.h
    ${CMAKE_SOURCE_DIR}/include/viewer/RenderingController.h
    


    # Modular edge component headers
    ${CMAKE_SOURCE_DIR}/include/edges/ModularEdgeComponent.h
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeProcessorFactory.h

    # Modular extractors headers
    ${CMAKE_SOURCE_DIR}/include/edges/extractors/BaseEdgeExtractor.h
    ${CMAKE_SOURCE_DIR}/include/edges/extractors/OriginalEdgeExtractor.h
    ${CMAKE_SOURCE_DIR}/include/edges/extractors/FeatureEdgeExtractor.h
    ${CMAKE_SOURCE_DIR}/include/edges/extractors/MeshEdgeExtractor.h
    ${CMAKE_SOURCE_DIR}/include/edges/extractors/SilhouetteEdgeExtractor.h

    # Edge intersection accelerator header
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeIntersectionAccelerator.h
    ${CMAKE_SOURCE_DIR}/include/edges/EdgeExtractionUIHelper.h
    ${CMAKE_SOURCE_DIR}/include/edges/AsyncIntersectionTask.h
    ${CMAKE_SOURCE_DIR}/include/edges/AsyncIntersectionManager.h
    ${CMAKE_SOURCE_DIR}/include/edges/AsyncEdgeIntersectionComputer.h

    # Modular renderers headers
    ${CMAKE_SOURCE_DIR}/include/edges/renderers/BaseEdgeRenderer.h
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
    ${CMAKE_SOURCE_DIR}/include/viewer/MeshQualityValidator.h
    ${CMAKE_SOURCE_DIR}/include/viewer/config/SubdivisionConfig.h
    ${CMAKE_SOURCE_DIR}/include/viewer/config/SmoothingConfig.h
    ${CMAKE_SOURCE_DIR}/include/viewer/config/TessellationConfig.h
    ${CMAKE_SOURCE_DIR}/include/viewer/config/NormalDisplayConfig.h
    ${CMAKE_SOURCE_DIR}/include/viewer/config/OriginalEdgesConfig.h
    ${CMAKE_SOURCE_DIR}/include/viewer/SelectionAcceleratorService.h
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
    AsyncEngine
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Set library export properties
set_target_properties(CADOCC PROPERTIES
    FOLDER "OpenCASCADE"
    EXPORT_NAME OCC
) 