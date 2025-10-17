#pragma once

#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbViewportRegion.h>
#include <wx/gdicmn.h>

class CoordinateTransformer {
public:
	CoordinateTransformer(int canvasWidth, int canvasHeight);
	
	// wxWidgets coordinate system (top-left origin, Y increases downward)
	// OpenGL coordinate system (bottom-left origin, Y increases upward)
	
	// Convert between coordinate systems
	float wxXToGlX(float wxX) const;
	float wxYToGlY(float wxY) const;
	
	float glXToWxX(float glX) const;
	float glYToWxY(float glY) const;
	
	// Viewport-specific conversions
	// Viewport.y is stored in GL coordinates (from bottom)
	// Visual position requires conversion to wx coordinates (from top)
	int viewportGlYToWxY(int glY, int viewportHeight) const;
	int wxYToViewportGlY(int wxY, int viewportHeight) const;
	
	// Picking point conversion (for ray picking)
	SbVec2s wxToPick(float wxX, float wxY, int viewportHeight) const;
	wxPoint pickToWx(const SbVec2s& pickPoint, int viewportHeight, 
	                   int viewportX, int viewportY) const;
	
	// Local viewport coordinates (relative to viewport origin)
	int globalXToLocal(float globalX, int viewportX) const;
	int globalYToLocal(float globalY, int viewportY, int viewportHeight) const;
	
	int localXToGlobal(int localX, int viewportX) const;
	int localYToGlobal(int localY, int viewportY, int viewportHeight) const;
	
	// Utility function
	bool isPointInViewport(float x, float y, int vpX, int vpY, 
	                        int vpWidth, int vpHeight) const;
	
	// Update canvas size when window is resized
	void updateCanvasSize(int newWidth, int newHeight);
	
	int getCanvasWidth() const { return m_canvasWidth; }
	int getCanvasHeight() const { return m_canvasHeight; }
	
private:
	int m_canvasWidth;
	int m_canvasHeight;
};
