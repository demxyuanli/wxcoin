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
    FixNormals,
    ShowEdges,
    SetTransparency,
    ToggleWireframe,
    ToggleShading,
    ToggleEdges,
    ToggleOutline,
    OutlineSettings,



    // Edit
    Undo,
    Redo,

    // Navigation / settings
    NavCubeConfig,
    ZoomSpeed,
    MeshQualityDialog,
    RenderingSettings,

    // Help
    HelpAbout,

    // Refresh operations
    RefreshView,
    RefreshScene,
    RefreshObject,
    RefreshMaterial,
    RefreshGeometry,
    RefreshUI,

    // Sentinel
    Unknown
};
// clang-format on

// Convert enum to string (the legacy command identifier)
inline const std::string& to_string(CommandType type) {
    try {
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
            {CommandType::FixNormals, "FIX_NORMALS"},
            {CommandType::ShowEdges, "SHOW_EDGES"},
            {CommandType::SetTransparency, "SET_TRANSPARENCY"},
            {CommandType::ToggleWireframe, "TOGGLE_WIREFRAME"},
            {CommandType::ToggleShading, "TOGGLE_SHADING"},
            {CommandType::ToggleEdges, "TOGGLE_EDGES"},
            {CommandType::ToggleOutline, "TOGGLE_OUTLINE"},
            {CommandType::OutlineSettings, "OUTLINE_SETTINGS"},



            {CommandType::Undo, "UNDO"},
            {CommandType::Redo, "REDO"},

            {CommandType::NavCubeConfig, "NAV_CUBE_CONFIG"},
            {CommandType::ZoomSpeed, "ZOOM_SPEED"},
            {CommandType::MeshQualityDialog, "MESH_QUALITY_DIALOG"},
            {CommandType::RenderingSettings, "RENDERING_SETTINGS"},

            {CommandType::HelpAbout, "HELP_ABOUT"},

            {CommandType::RefreshView, "REFRESH_VIEW"},
            {CommandType::RefreshScene, "REFRESH_SCENE"},
            {CommandType::RefreshObject, "REFRESH_OBJECT"},
            {CommandType::RefreshMaterial, "REFRESH_MATERIAL"},
            {CommandType::RefreshGeometry, "REFRESH_GEOMETRY"},
            {CommandType::RefreshUI, "REFRESH_UI"},

            {CommandType::Unknown, "UNKNOWN"}
        };
        
        // Check if the static map is still valid
        if (kEnumToString.empty()) {
            static const std::string fallback = "UNKNOWN";
            return fallback;
        }
        
        auto it = kEnumToString.find(type);
        if (it != kEnumToString.end()) {
            return it->second;
        }
        
        // Fallback to Unknown
        auto unknownIt = kEnumToString.find(CommandType::Unknown);
        if (unknownIt != kEnumToString.end()) {
            return unknownIt->second;
        }
        
        // Last resort fallback
        static const std::string fallback = "UNKNOWN";
        return fallback;
    } catch (...) {
        // Return a safe fallback string if the static map is corrupted
        static const std::string fallback = "UNKNOWN";
        return fallback;
    }
}

// Convert legacy string identifier to enum; returns Unknown if not found
inline CommandType from_string(const std::string& str) {
    try {
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
            {"FIX_NORMALS", CommandType::FixNormals},
            {"SHOW_EDGES", CommandType::ShowEdges},
            {"SET_TRANSPARENCY", CommandType::SetTransparency},
            {"TOGGLE_WIREFRAME", CommandType::ToggleWireframe},
            {"TOGGLE_SHADING", CommandType::ToggleShading},
            {"TOGGLE_EDGES", CommandType::ToggleEdges},
            {"TOGGLE_OUTLINE", CommandType::ToggleOutline},
            {"OUTLINE_SETTINGS", CommandType::OutlineSettings},



            {"UNDO", CommandType::Undo},
            {"REDO", CommandType::Redo},

            {"NAV_CUBE_CONFIG", CommandType::NavCubeConfig},
            {"ZOOM_SPEED", CommandType::ZoomSpeed},
            {"MESH_QUALITY_DIALOG", CommandType::MeshQualityDialog},
            {"RENDERING_SETTINGS", CommandType::RenderingSettings},

            {"HELP_ABOUT", CommandType::HelpAbout},

            {"REFRESH_VIEW", CommandType::RefreshView},
            {"REFRESH_SCENE", CommandType::RefreshScene},
            {"REFRESH_OBJECT", CommandType::RefreshObject},
            {"REFRESH_MATERIAL", CommandType::RefreshMaterial},
            {"REFRESH_GEOMETRY", CommandType::RefreshGeometry},
            {"REFRESH_UI", CommandType::RefreshUI}
        };
        
        // Check if the static map is still valid
        if (kStringToEnum.empty()) {
            return CommandType::Unknown;
        }
        
        auto it = kStringToEnum.find(str);
        return it == kStringToEnum.end() ? CommandType::Unknown : it->second;
    } catch (...) {
        // Return Unknown if the static map is corrupted
        return CommandType::Unknown;
    }
}

} // namespace cmd 