#pragma once

/**
 * @file OCCGeometry.h
 * @brief Compatibility wrapper for the refactored OCCGeometry modules
 * 
 * This file provides backward compatibility by including the new modular geometry headers.
 * The old OCCGeometry class has been split into focused modules for better management:
 * 
 * - OCCGeometryCore: Basic geometry data (shape, name, file)
 * - OCCGeometryTransform: Transform properties (position, rotation, scale)
 * - OCCGeometryMaterial: Material properties (ambient, diffuse, specular, etc.)
 * - OCCGeometryAppearance: Visual appearance (color, transparency, texture, blend)
 * - OCCGeometryDisplay: Display modes (edges, vertices, wireframe)
 * - OCCGeometryQuality: Rendering quality (LOD, shadows, lighting)
 * - OCCGeometryMesh: Coin3D mesh generation and management
 * - OCCGeometryPrimitives: Primitive shapes (Box, Cylinder, Sphere, etc.)
 * 
 * The main OCCGeometry class now uses composition to combine all these modules.
 */

// Include all geometry modules
#include "geometry/OCCGeometry.h"
#include "geometry/OCCGeometryCore.h"
#include "geometry/OCCGeometryTransform.h"
#include "geometry/OCCGeometryMaterial.h"
#include "geometry/OCCGeometryAppearance.h"
#include "geometry/OCCGeometryDisplay.h"
#include "geometry/OCCGeometryQuality.h"
#include "geometry/OCCGeometryMesh.h"
#include "geometry/OCCGeometryPrimitives.h"

// Re-export primitive classes for backward compatibility
using OCCBox = OCCBox;
using OCCCylinder = OCCCylinder;
using OCCSphere = OCCSphere;
using OCCCone = OCCCone;
using OCCTorus = OCCTorus;
using OCCTruncatedCylinder = OCCTruncatedCylinder;
