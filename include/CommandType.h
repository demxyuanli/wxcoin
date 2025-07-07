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
    ImportSTEP,
    FileExit,

    // Geometry creation
    CreateBox,
    CreateSphere,
    CreateCylinder,
    CreateCone,
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

    // Edit
    Undo,
    Redo,

    // Navigation / settings
    NavCubeConfig,
    ZoomSpeed,

    // Help
    HelpAbout,

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
        {CommandType::ImportSTEP, "IMPORT_STEP"},
        {CommandType::FileExit, "FILE_EXIT"},

        {CommandType::CreateBox, "CREATE_BOX"},
        {CommandType::CreateSphere, "CREATE_SPHERE"},
        {CommandType::CreateCylinder, "CREATE_CYLINDER"},
        {CommandType::CreateCone, "CREATE_CONE"},
        {CommandType::CreateWrench, "CREATE_WRENCH"},

        {CommandType::ViewAll, "VIEW_ALL"},
        {CommandType::ViewTop, "VIEW_TOP"},
        {CommandType::ViewFront, "VIEW_FRONT"},
        {CommandType::ViewRight, "VIEW_RIGHT"},
        {CommandType::ViewIsometric, "VIEW_ISOMETRIC"},

        {CommandType::ShowNormals, "SHOW_NORMALS"},
        {CommandType::FixNormals, "FIX_NORMALS"},
        {CommandType::ShowEdges, "SHOW_EDGES"},

        {CommandType::Undo, "UNDO"},
        {CommandType::Redo, "REDO"},

        {CommandType::NavCubeConfig, "NAV_CUBE_CONFIG"},
        {CommandType::ZoomSpeed, "ZOOM_SPEED"},

        {CommandType::HelpAbout, "HELP_ABOUT"},

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
        {"IMPORT_STEP", CommandType::ImportSTEP},
        {"FILE_EXIT", CommandType::FileExit},

        {"CREATE_BOX", CommandType::CreateBox},
        {"CREATE_SPHERE", CommandType::CreateSphere},
        {"CREATE_CYLINDER", CommandType::CreateCylinder},
        {"CREATE_CONE", CommandType::CreateCone},
        {"CREATE_WRENCH", CommandType::CreateWrench},

        {"VIEW_ALL", CommandType::ViewAll},
        {"VIEW_TOP", CommandType::ViewTop},
        {"VIEW_FRONT", CommandType::ViewFront},
        {"VIEW_RIGHT", CommandType::ViewRight},
        {"VIEW_ISOMETRIC", CommandType::ViewIsometric},

        {"SHOW_NORMALS", CommandType::ShowNormals},
        {"FIX_NORMALS", CommandType::FixNormals},
        {"SHOW_EDGES", CommandType::ShowEdges},

        {"UNDO", CommandType::Undo},
        {"REDO", CommandType::Redo},

        {"NAV_CUBE_CONFIG", CommandType::NavCubeConfig},
        {"ZOOM_SPEED", CommandType::ZoomSpeed},

        {"HELP_ABOUT", CommandType::HelpAbout}
    };
    auto it = kStringToEnum.find(str);
    return it == kStringToEnum.end() ? CommandType::Unknown : it->second;
}

} // namespace cmd 