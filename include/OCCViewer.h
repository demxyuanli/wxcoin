#pragma once

/**
 * @file OCCViewer.h
 * @brief Compatibility wrapper for the refactored OCCViewer modules
 * 
 * This file provides backward compatibility by including the new modular viewer headers.
 * The old OCCViewer class has been refactored to use composition with specialized controllers:
 * 
 * - ViewportController: View manipulation (fit, camera, refresh)
 * - RenderingController: Display modes (wireframe, edges, normals)
 * - MeshParameterController: Mesh quality control (already existed)
 * - LODController: Level of detail control (already existed)
 * - SliceController: Clipping plane control (already existed)
 * - ExplodeController: Assembly explode control (already existed)
 * - EdgeDisplayManager: Edge display management (already existed)
 * - OutlineDisplayManager: Outline effects (already existed)
 * 
 * The main OCCViewer class delegates operations to these specialized controllers.
 */

// Include the refactored viewer
#include "viewer/OCCViewer.h"
#include "viewer/ViewportController.h"
#include "viewer/RenderingController.h"
