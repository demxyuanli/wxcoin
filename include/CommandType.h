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
		CreateNavCube,

		// View
		ViewAll,
		ViewTop,
		ViewFront,
		ViewRight,
		ViewIsometric,

		// View Bookmarks
		ViewBookmarkSave,
		ViewBookmarkRestore,
		ViewBookmarkFront,
		ViewBookmarkBack,
		ViewBookmarkLeft,
		ViewBookmarkRight,
		ViewBookmarkTop,
		ViewBookmarkBottom,
		ViewBookmarkIsometric,
		ViewBookmarkManager,

		// Animation Types
		AnimationTypeLinear,
		AnimationTypeSmooth,
		AnimationTypeEaseIn,
		AnimationTypeEaseOut,
		AnimationTypeBounce,

		// Zoom Controls
		ZoomIn,
		ZoomOut,
		ZoomReset,
		ZoomToFit,
		ZoomSettings,
		ZoomLevel25,
		ZoomLevel50,
		ZoomLevel100,
		ZoomLevel200,
		ZoomLevel400,

		// Viewer toggles / actions
		ShowNormals,
		ShowFaces,
		FixNormals,
		NormalFixDialog,
		// ShowSilhouetteEdges, // removed, use Outline toggle
		ToggleOutline,
		SetTransparency,
		ToggleWireframe,
		// Removed ToggleShading - functionality not needed
		ToggleEdges,
		ShowOriginalEdges,
		ShowFeatureEdges, // New: show feature edges
		ShowMeshEdges, // New: show mesh edges
		ShowFaceNormals, // New: show face normals
		FaceSelectionTool, // New: face selection tool
		FaceQueryTool, // New: face query tool
		ShowPointView, // New: show point view

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
		NavigationMode,
		MeshQualityDialog,
		RenderingSettings,
		EdgeSettings,
		LightingSettings,
		ToggleCoordinateSystem,
		ToggleReferenceGrid,
		ToggleChessboardGrid,

		// Split View
		SplitViewSingle,
		SplitViewHorizontal2,
		SplitViewVertical2,
		SplitViewQuad,
		SplitViewSix,
		SplitViewToggleSync,

		// Help
		HelpAbout,

		// Render Preview
		RenderPreviewSystem,

		// Render Modes (similar to FreeCAD)
		RenderModeNoShading,
		RenderModePoints,
		RenderModeWireframe,
		RenderModeFlatLines,
		RenderModeShaded,
		RenderModeShadedWireframe,
		RenderModeHiddenLine,

		// Flat Widgets Example
		ShowFlatWidgetsExample,

		// Docking
		DockLayoutConfig,

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
			{CommandType::CreateNavCube, "CREATE_NAV_CUBE"},

			{CommandType::ViewAll, "VIEW_ALL"},
			{CommandType::ViewTop, "VIEW_TOP"},
			{CommandType::ViewFront, "VIEW_FRONT"},
			{CommandType::ViewRight, "VIEW_RIGHT"},
			{CommandType::ViewIsometric, "VIEW_ISOMETRIC"},

			{CommandType::ViewBookmarkSave, "VIEW_BOOKMARK_SAVE"},
			{CommandType::ViewBookmarkRestore, "VIEW_BOOKMARK_RESTORE"},
			{CommandType::ViewBookmarkFront, "VIEW_BOOKMARK_FRONT"},
			{CommandType::ViewBookmarkBack, "VIEW_BOOKMARK_BACK"},
			{CommandType::ViewBookmarkLeft, "VIEW_BOOKMARK_LEFT"},
			{CommandType::ViewBookmarkRight, "VIEW_BOOKMARK_RIGHT"},
			{CommandType::ViewBookmarkTop, "VIEW_BOOKMARK_TOP"},
			{CommandType::ViewBookmarkBottom, "VIEW_BOOKMARK_BOTTOM"},
			{CommandType::ViewBookmarkIsometric, "VIEW_BOOKMARK_ISOMETRIC"},
			{CommandType::ViewBookmarkManager, "VIEW_BOOKMARK_MANAGER"},

			{CommandType::AnimationTypeLinear, "ANIMATION_TYPE_LINEAR"},
			{CommandType::AnimationTypeSmooth, "ANIMATION_TYPE_SMOOTH"},
			{CommandType::AnimationTypeEaseIn, "ANIMATION_TYPE_EASE_IN"},
			{CommandType::AnimationTypeEaseOut, "ANIMATION_TYPE_EASE_OUT"},
			{CommandType::AnimationTypeBounce, "ANIMATION_TYPE_BOUNCE"},

			{CommandType::ZoomIn, "ZOOM_IN"},
			{CommandType::ZoomOut, "ZOOM_OUT"},
			{CommandType::ZoomReset, "ZOOM_RESET"},
			{CommandType::ZoomToFit, "ZOOM_TO_FIT"},
			{CommandType::ZoomSettings, "ZOOM_SETTINGS"},
			{CommandType::ZoomLevel25, "ZOOM_LEVEL_25"},
			{CommandType::ZoomLevel50, "ZOOM_LEVEL_50"},
			{CommandType::ZoomLevel100, "ZOOM_LEVEL_100"},
			{CommandType::ZoomLevel200, "ZOOM_LEVEL_200"},
			{CommandType::ZoomLevel400, "ZOOM_LEVEL_400"},

			{CommandType::ShowNormals, "SHOW_NORMALS"},
			{CommandType::ShowFaces, "SHOW_FACES"},
			{CommandType::FixNormals, "FIX_NORMALS"},
			// {CommandType::ShowSilhouetteEdges, "SHOW_SILHOUETTE_EDGES"},
			{CommandType::ToggleOutline, "TOGGLE_OUTLINE"},
			{CommandType::SetTransparency, "SET_TRANSPARENCY"},
			{CommandType::ToggleWireframe, "TOGGLE_WIREFRAME"},
			// Removed ToggleShading mapping
			{CommandType::ToggleEdges, "TOGGLE_EDGES"},
			{CommandType::ShowOriginalEdges, "SHOW_ORIGINAL_EDGES"},
			{CommandType::ShowFeatureEdges, "SHOW_FEATURE_EDGES"},
			{CommandType::ShowMeshEdges, "SHOW_MESH_EDGES"},
			{CommandType::ShowFaceNormals, "SHOW_FACE_NORMALS"},
			{CommandType::FaceSelectionTool, "FACE_SELECTION_TOOL"},
			{CommandType::FaceQueryTool, "FACE_QUERY_TOOL"},
			{CommandType::ShowPointView, "SHOW_POINT_VIEW"},

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
			{CommandType::NavigationMode, "NAVIGATION_MODE"},
			{CommandType::MeshQualityDialog, "MESH_QUALITY_DIALOG"},
			{CommandType::RenderingSettings, "RENDERING_SETTINGS"},
			{CommandType::EdgeSettings, "EDGE_SETTINGS"},
			{CommandType::LightingSettings, "LIGHTING_SETTINGS"},
			{CommandType::ToggleCoordinateSystem, "TOGGLE_COORDINATE_SYSTEM"},
			{CommandType::ToggleReferenceGrid, "TOGGLE_REFERENCE_GRID"},
			{CommandType::ToggleChessboardGrid, "TOGGLE_CHESSBOARD_GRID"},

			{CommandType::SplitViewSingle, "SPLIT_VIEW_SINGLE"},
			{CommandType::SplitViewHorizontal2, "SPLIT_VIEW_HORIZONTAL_2"},
			{CommandType::SplitViewVertical2, "SPLIT_VIEW_VERTICAL_2"},
			{CommandType::SplitViewQuad, "SPLIT_VIEW_QUAD"},
			{CommandType::SplitViewSix, "SPLIT_VIEW_SIX"},
			{CommandType::SplitViewToggleSync, "SPLIT_VIEW_TOGGLE_SYNC"},

			{CommandType::HelpAbout, "HELP_ABOUT"},
			{CommandType::RenderPreviewSystem, "RENDER_PREVIEW_SYSTEM"},
			{CommandType::RenderModeNoShading, "RENDER_MODE_NO_SHADING"},
			{CommandType::RenderModePoints, "RENDER_MODE_POINTS"},
			{CommandType::RenderModeWireframe, "RENDER_MODE_WIREFRAME"},
			{CommandType::RenderModeFlatLines, "RENDER_MODE_FLAT_LINES"},
			{CommandType::RenderModeShaded, "RENDER_MODE_SHADED"},
			{CommandType::RenderModeShadedWireframe, "RENDER_MODE_SHADED_WIREFRAME"},
			{CommandType::RenderModeHiddenLine, "RENDER_MODE_HIDDEN_LINE"},
			{CommandType::ShowFlatWidgetsExample, "SHOW_FLAT_WIDGETS_EXAMPLE"},
			{CommandType::DockLayoutConfig, "DOCK_LAYOUT_CONFIG"},

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
			{"CREATE_NAV_CUBE", CommandType::CreateNavCube},

			{"VIEW_ALL", CommandType::ViewAll},
			{"VIEW_TOP", CommandType::ViewTop},
			{"VIEW_FRONT", CommandType::ViewFront},
			{"VIEW_RIGHT", CommandType::ViewRight},
			{"VIEW_ISOMETRIC", CommandType::ViewIsometric},

			{"VIEW_BOOKMARK_SAVE", CommandType::ViewBookmarkSave},
			{"VIEW_BOOKMARK_RESTORE", CommandType::ViewBookmarkRestore},
			{"VIEW_BOOKMARK_FRONT", CommandType::ViewBookmarkFront},
			{"VIEW_BOOKMARK_BACK", CommandType::ViewBookmarkBack},
			{"VIEW_BOOKMARK_LEFT", CommandType::ViewBookmarkLeft},
			{"VIEW_BOOKMARK_RIGHT", CommandType::ViewBookmarkRight},
			{"VIEW_BOOKMARK_TOP", CommandType::ViewBookmarkTop},
			{"VIEW_BOOKMARK_BOTTOM", CommandType::ViewBookmarkBottom},
			{"VIEW_BOOKMARK_ISOMETRIC", CommandType::ViewBookmarkIsometric},
			{"VIEW_BOOKMARK_MANAGER", CommandType::ViewBookmarkManager},

			{"ANIMATION_TYPE_LINEAR", CommandType::AnimationTypeLinear},
			{"ANIMATION_TYPE_SMOOTH", CommandType::AnimationTypeSmooth},
			{"ANIMATION_TYPE_EASE_IN", CommandType::AnimationTypeEaseIn},
			{"ANIMATION_TYPE_EASE_OUT", CommandType::AnimationTypeEaseOut},
			{"ANIMATION_TYPE_BOUNCE", CommandType::AnimationTypeBounce},

			{"ZOOM_IN", CommandType::ZoomIn},
			{"ZOOM_OUT", CommandType::ZoomOut},
			{"ZOOM_RESET", CommandType::ZoomReset},
			{"ZOOM_TO_FIT", CommandType::ZoomToFit},
			{"ZOOM_SETTINGS", CommandType::ZoomSettings},
			{"ZOOM_LEVEL_25", CommandType::ZoomLevel25},
			{"ZOOM_LEVEL_50", CommandType::ZoomLevel50},
			{"ZOOM_LEVEL_100", CommandType::ZoomLevel100},
			{"ZOOM_LEVEL_200", CommandType::ZoomLevel200},
			{"ZOOM_LEVEL_400", CommandType::ZoomLevel400},

			{"SHOW_NORMALS", CommandType::ShowNormals},
			{"SHOW_FACES", CommandType::ShowFaces},
			{"FIX_NORMALS", CommandType::FixNormals},
			// {"SHOW_SILHOUETTE_EDGES", CommandType::ShowSilhouetteEdges},
			{"TOGGLE_OUTLINE", CommandType::ToggleOutline},
			{"SET_TRANSPARENCY", CommandType::SetTransparency},
			{"TOGGLE_WIREFRAME", CommandType::ToggleWireframe},
			// Removed ToggleShading reverse mapping
			{"TOGGLE_EDGES", CommandType::ToggleEdges},
			{"SHOW_ORIGINAL_EDGES", CommandType::ShowOriginalEdges},
			{"SHOW_FEATURE_EDGES", CommandType::ShowFeatureEdges},
			{"SHOW_MESH_EDGES", CommandType::ShowMeshEdges},
			{"SHOW_FACE_NORMALS", CommandType::ShowFaceNormals},
			{"FACE_SELECTION_TOOL", CommandType::FaceSelectionTool},
			{"FACE_QUERY_TOOL", CommandType::FaceQueryTool},
			{"SHOW_POINT_VIEW", CommandType::ShowPointView},

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
			{"NAVIGATION_MODE", CommandType::NavigationMode},
			{"MESH_QUALITY_DIALOG", CommandType::MeshQualityDialog},
			{"RENDERING_SETTINGS", CommandType::RenderingSettings},
			{"EDGE_SETTINGS", CommandType::EdgeSettings},
			{"LIGHTING_SETTINGS", CommandType::LightingSettings},
			{"TOGGLE_COORDINATE_SYSTEM", CommandType::ToggleCoordinateSystem},
			{"TOGGLE_REFERENCE_GRID", CommandType::ToggleReferenceGrid},
			{"TOGGLE_CHESSBOARD_GRID", CommandType::ToggleChessboardGrid},

			{"SPLIT_VIEW_SINGLE", CommandType::SplitViewSingle},
			{"SPLIT_VIEW_HORIZONTAL_2", CommandType::SplitViewHorizontal2},
			{"SPLIT_VIEW_VERTICAL_2", CommandType::SplitViewVertical2},
			{"SPLIT_VIEW_QUAD", CommandType::SplitViewQuad},
			{"SPLIT_VIEW_SIX", CommandType::SplitViewSix},
			{"SPLIT_VIEW_TOGGLE_SYNC", CommandType::SplitViewToggleSync},

			{"HELP_ABOUT", CommandType::HelpAbout},
			{"RENDER_PREVIEW_SYSTEM", CommandType::RenderPreviewSystem},
			{"RENDER_MODE_NO_SHADING", CommandType::RenderModeNoShading},
			{"RENDER_MODE_POINTS", CommandType::RenderModePoints},
			{"RENDER_MODE_WIREFRAME", CommandType::RenderModeWireframe},
			{"RENDER_MODE_FLAT_LINES", CommandType::RenderModeFlatLines},
			{"RENDER_MODE_SHADED", CommandType::RenderModeShaded},
			{"RENDER_MODE_SHADED_WIREFRAME", CommandType::RenderModeShadedWireframe},
			{"RENDER_MODE_HIDDEN_LINE", CommandType::RenderModeHiddenLine},
			{"SHOW_FLAT_WIDGETS_EXAMPLE", CommandType::ShowFlatWidgetsExample},
			{"DOCK_LAYOUT_CONFIG", CommandType::DockLayoutConfig}
		};
		auto it = kStringToEnum.find(str);
		return it == kStringToEnum.end() ? CommandType::Unknown : it->second;
	}
} // namespace cmd 