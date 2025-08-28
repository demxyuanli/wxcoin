#pragma once

#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include "DPIManager.h"

class DPIAwareRendering {
public:
	// Utility functions for DPI-aware OpenGL rendering
	static void setDPIAwareLineWidth(float baseWidth);
	static void setDPIAwarePointSize(float baseSize);

	// Coin3D node helpers for DPI-aware rendering
	static void configureDPIAwareDrawStyle(SoDrawStyle* drawStyle,
		float baseLineWidth = 1.0f,
		float basePointSize = 1.0f);

	// Apply DPI scaling to existing nodes
	static void updateDrawStyleDPI(SoDrawStyle* drawStyle,
		float originalLineWidth = 1.0f,
		float originalPointSize = 1.0f);

	// Helper for creating DPI-aware coordinate system lines
	static SoDrawStyle* createDPIAwareCoordinateLineStyle(float baseWidth = 1.0f);

	// Helper for creating DPI-aware geometry outline styles
	static SoDrawStyle* createDPIAwareGeometryStyle(float baseWidth = 1.0f,
		bool filled = true);

private:
	static float getCurrentDPIScale();
};