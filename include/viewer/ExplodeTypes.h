#pragma once

#include <OpenCASCADE/gp_Pnt.hxx>

// Shared explode mode enum used by viewer and controllers
enum class ExplodeMode {
	Radial,
	AxisX,
	AxisY,
	AxisZ,
	StackX,
	StackY,
	StackZ,
	Diagonal,
	Assembly
};

// Center selection for explode computation
enum class ExplodeCenterMode {
	GlobalCenter,
	SelectionCenter,
	CustomPoint
};

// Scope selection for which geometries to apply explode
enum class ExplodeScope {
	All,
	SelectionOnly,
	SelectionSubtree
};

struct ExplodeWeights {
	double axisX{ 0.0 };
	double axisY{ 0.0 };
	double axisZ{ 0.0 };
	double radial{ 1.0 };
	double diagonal{ 0.0 };
};

struct ExplodeParams {
	// Base distance factor (global scalar)
	double baseFactor{ 1.0 };
	// Directional weights (can be combined)
	ExplodeWeights weights{};
	// Per-level scale factor for hierarchical explode (Assembly)
	double perLevelScale{ 0.6 };
	// Size influence (0 = ignore part size, 1 = scale by size ratio)
	double sizeInfluence{ 0.0 };
	// Random jitter (0-0.2 typical). 0 disables jitter
	double jitter{ 0.0 };
	// Minimum spacing to avoid overlap (optional)
	double minSpacing{ 0.0 };
	// Center selection and scope
	ExplodeCenterMode centerMode{ ExplodeCenterMode::GlobalCenter };
	ExplodeScope scope{ ExplodeScope::All };
	gp_Pnt customCenter{ 0.0, 0.0, 0.0 };
	// Compatibility primary mode (optional hint)
	ExplodeMode primaryMode{ ExplodeMode::Radial };
};
