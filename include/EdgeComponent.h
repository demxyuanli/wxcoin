#pragma once

/**
 * @file EdgeComponent.h
 * @brief Compatibility wrapper for the refactored EdgeComponent modules
 * 
 * This file provides backward compatibility by including the new modular edge headers.
 * The old EdgeComponent class has been split into focused modules:
 * 
 * - EdgeExtractor: Edge extraction logic (original, feature, mesh, silhouette)
 * - EdgeRenderer: Edge visualization and Coin3D node generation
 * 
 * The EdgeComponent class now uses composition to combine extraction and rendering.
 */

// Include all edge modules
#include "edges/EdgeComponent.h"
#include "edges/EdgeExtractor.h"
#include "edges/EdgeRenderer.h"
