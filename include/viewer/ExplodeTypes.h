#pragma once

#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Dir.hxx>
#include <vector>
#include <string>

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
	Assembly,
	Smart  // Smart mode using direction clustering
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

// Constraint types for assembly relationships
enum class ConstraintType {
	MATE,
	INSERT,
	FASTENER,
	UNKNOWN
};

// Represents assembly constraint between two parts
struct AssemblyConstraint {
	std::string part1;
	std::string part2;
	ConstraintType type{ ConstraintType::UNKNOWN };
	gp_Dir direction{ 0, 0, 1 };  // Separation direction
	
	AssemblyConstraint() = default;
	AssemblyConstraint(const std::string& p1, const std::string& p2, 
	                   ConstraintType t, const gp_Dir& d)
		: part1(p1), part2(p2), type(t), direction(d) {}
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
	// Assembly constraints (optional, for smart mode)
	std::vector<AssemblyConstraint> constraints{};
	// Enable collision detection and resolution
	bool enableCollisionResolution{ false };
	// Collision resolution threshold (fraction of bbox diagonal)
	double collisionThreshold{ 0.6 };
};
