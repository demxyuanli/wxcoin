#pragma once

#include <Inventor/SbColor.h>

struct ViewportConfig {
	// Size parameters (logical pixels)
	int defaultMargin = 20;
	int cubeOutlineSize = 120;
	int navigationCubeSize = 80;
	int coordinateSystemSize = 80;
	
	// Color parameters
	SbColor normalShapeColor = SbColor(0.8f, 0.8f, 0.8f);      // Gray
	SbColor hoverShapeColor = SbColor(1.0f, 0.7f, 0.3f);       // Orange
	SbColor greenShapeColor = SbColor(0.8f, 1.0f, 0.8f);       // Green
	
	// Geometry parameters
	float shapeScale = 0.95f;              // Shape scale factor
	float cubeGeometrySize = 100;          // Cube geometric size
	float sphereScaleFactor = 0.475f;      // Small sphere scale factor
	
	// Interaction parameters
	bool enableHoverEffect = true;
	bool enablePickingCache = false;       // Will be enabled in Phase 2
	int pickingCacheThreshold = 5;         // Mouse movement threshold (pixels)
	
	// Debug parameters
	bool enableDetailedLogging = false;
	bool visualizeViewportBounds = false;
	
	// Rendering parameters
	bool enableSmoothing = true;
	bool enableTransparency = true;
	
	// Static singleton instance
	static ViewportConfig& getInstance() {
		static ViewportConfig instance;
		return instance;
	}
	
private:
	ViewportConfig() = default;
	~ViewportConfig() = default;
	ViewportConfig(const ViewportConfig&) = delete;
	ViewportConfig& operator=(const ViewportConfig&) = delete;
};
