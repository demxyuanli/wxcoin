#pragma once
#include <string>
#include <optional>
#include <unordered_map>

// Strongly typed enumeration representing every command in the system
// NOTE: When adding new commands, always update the mapping tables below.
// This ensures compile-time safety and centralized management.

namespace cmd {

// clang-format off
enum class CommandType {
    // File
    FileNew,
    FileOpen,
    FileSave,
    FileSaveAs,
    ImportSTEP,
    FileExit,

    // Geometry creation
    CreateBox,
    CreateSphere,
    CreateCylinder,
    CreateCone,
    CreateTorus,
    CreateTruncatedCylinder,
    CreateWrench,

    // View
    ViewAll,
    ViewTop,
    ViewFront,
    ViewRight,
    ViewIsometric,

    // Viewer toggles / actions
    ShowNormals,
    ShowFaces,
    FixNormals,
    ShowSilhouetteEdges,
    SetTransparency,
    ToggleWireframe,
    // Removed ToggleShading - functionality not needed
    ToggleEdges,
    ShowOriginalEdges, 
    ShowFeatureEdges, // New: show feature edges
    ShowMeshEdges, // New: show mesh edges
    ShowFaceNormals, // New: show face normals

    // Texture modes
    TextureModeDecal,
    TextureModeModulate,
    TextureModeReplace,
    TextureModeBlend,

    // Assembly
    ExplodeAssembly,
    // Slice
    SliceToggle,
    // Edit
    Undo,
    Redo,

    // Navigation / settings
    NavCubeConfig,
    ZoomSpeed,
    MeshQualityDialog,
    RenderingSettings,
    EdgeSettings,
    LightingSettings,
    ToggleCoordinateSystem,
    ToggleReferenceGrid,
    ToggleChessboardGrid,

    // Help
    HelpAbout,

    // Render Preview
    RenderPreviewSystem,

    // Flat Widgets Example
    ShowFlatWidgetsExample,

    // Sentinel
    Unknown
};
// clang-format on

// Convert enum to string (the legacy command identifier)
inline const std::string& to_string(CommandType type) {
    static const std::unordered_map<CommandType, std::string> kEnumToString{
        {CommandType::FileNew, "FILE_NEW"},
        {CommandType::FileOpen, "FILE_OPEN"},
        {CommandType::FileSave, "FILE_SAVE"},
        {CommandType::FileSaveAs, "FILE_SAVE_AS"},
        {CommandType::ImportSTEP, "IMPORT_STEP"},
        {CommandType::FileExit, "FILE_EXIT"},

        {CommandType::CreateBox, "CREATE_BOX"},
        {CommandType::CreateSphere, "CREATE_SPHERE"},
        {CommandType::CreateCylinder, "CREATE_CYLINDER"},
        {CommandType::CreateCone, "CREATE_CONE"},
        {CommandType::CreateTorus, "CREATE_TORUS"},
        {CommandType::CreateTruncatedCylinder, "CREATE_TRUNCATED_CYLINDER"},
        {CommandType::CreateWrench, "CREATE_WRENCH"},

        {CommandType::ViewAll, "VIEW_ALL"},
        {CommandType::ViewTop, "VIEW_TOP"},
        {CommandType::ViewFront, "VIEW_FRONT"},
        {CommandType::ViewRight, "VIEW_RIGHT"},
        {CommandType::ViewIsometric, "VIEW_ISOMETRIC"},

        {CommandType::ShowNormals, "SHOW_NORMALS"},
        {CommandType::ShowFaces, "SHOW_FACES"},
        {CommandType::FixNormals, "FIX_NORMALS"},
        {CommandType::ShowSilhouetteEdges, "SHOW_SILHOUETTE_EDGES"},
        {CommandType::SetTransparency, "SET_TRANSPARENCY"},
        {CommandType::ToggleWireframe, "TOGGLE_WIREFRAME"},
        // Removed ToggleShading mapping
        {CommandType::ToggleEdges, "TOGGLE_EDGES"},
        {CommandType::ShowOriginalEdges, "SHOW_ORIGINAL_EDGES"},
        {CommandType::ShowFeatureEdges, "SHOW_FEATURE_EDGES"},
        {CommandType::ShowMeshEdges, "SHOW_MESH_EDGES"},
        {CommandType::ShowFaceNormals, "SHOW_FACE_NORMALS"},

        {CommandType::TextureModeDecal, "TEXTURE_MODE_DECAL"},
        {CommandType::TextureModeModulate, "TEXTURE_MODE_MODULATE"},
        {CommandType::TextureModeReplace, "TEXTURE_MODE_REPLACE"},
        {CommandType::TextureModeBlend, "TEXTURE_MODE_BLEND"},
        {CommandType::ExplodeAssembly, "EXPLODE_ASSEMBLY"},
        {CommandType::SliceToggle, "SLICE_TOGGLE"},



        {CommandType::Undo, "UNDO"},
        {CommandType::Redo, "REDO"},

        {CommandType::NavCubeConfig, "NAV_CUBE_CONFIG"},
        {CommandType::ZoomSpeed, "ZOOM_SPEED"},
        {CommandType::MeshQualityDialog, "MESH_QUALITY_DIALOG"},
        {CommandType::RenderingSettings, "RENDERING_SETTINGS"},
        {CommandType::EdgeSettings, "EDGE_SETTINGS"},
        {CommandType::LightingSettings, "LIGHTING_SETTINGS"},
        {CommandType::ToggleCoordinateSystem, "TOGGLE_COORDINATE_SYSTEM"},
        {CommandType::ToggleReferenceGrid, "TOGGLE_REFERENCE_GRID"},
        {CommandType::ToggleChessboardGrid, "TOGGLE_CHESSBOARD_GRID"},

        {CommandType::HelpAbout, "HELP_ABOUT"},
        {CommandType::RenderPreviewSystem, "RENDER_PREVIEW_SYSTEM"},
        {CommandType::ShowFlatWidgetsExample, "SHOW_FLAT_WIDGETS_EXAMPLE"},

        {CommandType::Unknown, "UNKNOWN"}
    };
    auto it = kEnumToString.find(type);
    return it != kEnumToString.end() ? it->second : kEnumToString.at(CommandType::Unknown);
}

// Convert legacy string identifier to enum; returns Unknown if not found
inline CommandType from_string(const std::string& str) {
    static const std::unordered_map<std::string, CommandType> kStringToEnum{
        {"FILE_NEW", CommandType::FileNew},
        {"FILE_OPEN", CommandType::FileOpen},
        {"FILE_SAVE", CommandType::FileSave},
        {"FILE_SAVE_AS", CommandType::FileSaveAs},
        {"IMPORT_STEP", CommandType::ImportSTEP},
        {"FILE_EXIT", CommandType::FileExit},

        {"CREATE_BOX", CommandType::CreateBox},
        {"CREATE_SPHERE", CommandType::CreateSphere},
        {"CREATE_CYLINDER", CommandType::CreateCylinder},
        {"CREATE_CONE", CommandType::CreateCone},
        {"CREATE_TORUS", CommandType::CreateTorus},
        {"CREATE_TRUNCATED_CYLINDER", CommandType::CreateTruncatedCylinder},
        {"CREATE_WRENCH", CommandType::CreateWrench},

        {"VIEW_ALL", CommandType::ViewAll},
        {"VIEW_TOP", CommandType::ViewTop},
        {"VIEW_FRONT", CommandType::ViewFront},
        {"VIEW_RIGHT", CommandType::ViewRight},
        {"VIEW_ISOMETRIC", CommandType::ViewIsometric},

        {"SHOW_NORMALS", CommandType::ShowNormals},
        {"SHOW_FACES", CommandType::ShowFaces},
        {"FIX_NORMALS", CommandType::FixNormals},
        {"SHOW_SILHOUETTE_EDGES", CommandType::ShowSilhouetteEdges},
        {"SET_TRANSPARENCY", CommandType::SetTransparency},
        {"TOGGLE_WIREFRAME", CommandType::ToggleWireframe},
        // Removed ToggleShading reverse mapping
        {"TOGGLE_EDGES", CommandType::ToggleEdges},
        {"SHOW_ORIGINAL_EDGES", CommandType::ShowOriginalEdges},
        {"SHOW_FEATURE_EDGES", CommandType::ShowFeatureEdges},
        {"SHOW_MESH_EDGES", CommandType::ShowMeshEdges},
        {"SHOW_FACE_NORMALS", CommandType::ShowFaceNormals},

        {"TEXTURE_MODE_DECAL", CommandType::TextureModeDecal},
        {"TEXTURE_MODE_MODULATE", CommandType::TextureModeModulate},
        {"TEXTURE_MODE_REPLACE", CommandType::TextureModeReplace},
        {"TEXTURE_MODE_BLEND", CommandType::TextureModeBlend},
        {"EXPLODE_ASSEMBLY", CommandType::ExplodeAssembly},
        {"SLICE_TOGGLE", CommandType::SliceToggle},



        {"UNDO", CommandType::Undo},
        {"REDO", CommandType::Redo},

        {"NAV_CUBE_CONFIG", CommandType::NavCubeConfig},
        {"ZOOM_SPEED", CommandType::ZoomSpeed},
        {"MESH_QUALITY_DIALOG", CommandType::MeshQualityDialog},
        {"RENDERING_SETTINGS", CommandType::RenderingSettings},
        {"EDGE_SETTINGS", CommandType::EdgeSettings},
        {"LIGHTING_SETTINGS", CommandType::LightingSettings},
        {"TOGGLE_COORDINATE_SYSTEM", CommandType::ToggleCoordinateSystem},
        {"TOGGLE_REFERENCE_GRID", CommandType::ToggleReferenceGrid},
        {"TOGGLE_CHESSBOARD_GRID", CommandType::ToggleChessboardGrid},

        {"HELP_ABOUT", CommandType::HelpAbout},
        {"RENDER_PREVIEW_SYSTEM", CommandType::RenderPreviewSystem},
        {"SHOW_FLAT_WIDGETS_EXAMPLE", CommandType::ShowFlatWidgetsExample}
    };
    auto it = kStringToEnum.find(str);
    return it == kStringToEnum.end() ? CommandType::Unknown : it->second;
}

} // namespace cmd 