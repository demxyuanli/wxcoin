#include "CoordinateTransformer.h"

CoordinateTransformer::CoordinateTransformer(int canvasWidth, int canvasHeight)
	: m_canvasWidth(canvasWidth), m_canvasHeight(canvasHeight) {
}

float CoordinateTransformer::wxXToGlX(float wxX) const {
	return wxX;  // X coordinate is the same in both systems
}

float CoordinateTransformer::wxYToGlY(float wxY) const {
	return m_canvasHeight - wxY;  // Y is flipped: GL uses bottom-up, wx uses top-down
}

float CoordinateTransformer::glXToWxX(float glX) const {
	return glX;  // X coordinate is the same in both systems
}

float CoordinateTransformer::glYToWxY(float glY) const {
	return m_canvasHeight - glY;  // Y is flipped back
}

int CoordinateTransformer::viewportGlYToWxY(int glY, int viewportHeight) const {
	// Viewport.y is in GL coordinates (from bottom)
	// Visual position (wx Y coordinate) = canvas height - (viewport.y + viewport.height)
	return m_canvasHeight - (glY + viewportHeight);
}

int CoordinateTransformer::wxYToViewportGlY(int wxY, int viewportHeight) const {
	// Inverse of viewportGlYToWxY
	return m_canvasHeight - wxY - viewportHeight;
}

SbVec2s CoordinateTransformer::wxToPick(float wxX, float wxY, int viewportHeight) const {
	// For picking, Y needs to be flipped within the viewport
	// localY = distance from visual top of viewport
	// pickY = distance from GL bottom of viewport = viewportHeight - localY - 1
	short pickX = static_cast<short>(wxX);
	short pickY = static_cast<short>(viewportHeight - wxY - 1);
	return SbVec2s(pickX, pickY);
}

wxPoint CoordinateTransformer::pickToWx(const SbVec2s& pickPoint, int viewportHeight, 
                                         int viewportX, int viewportY) const {
	// Inverse of wxToPick
	int wxX = viewportX + pickPoint[0];
	int wxY = viewportY + (viewportHeight - pickPoint[1] - 1);
	return wxPoint(wxX, wxY);
}

int CoordinateTransformer::globalXToLocal(float globalX, int viewportX) const {
	return static_cast<int>(globalX) - viewportX;
}

int CoordinateTransformer::globalYToLocal(float globalY, int viewportY, int viewportHeight) const {
	// globalY is in wx coordinates (from top)
	// viewportY is the visual top of the viewport in wx coordinates
	return static_cast<int>(globalY) - viewportY;
}

int CoordinateTransformer::localXToGlobal(int localX, int viewportX) const {
	return localX + viewportX;
}

int CoordinateTransformer::localYToGlobal(int localY, int viewportY, int viewportHeight) const {
	return localY + viewportY;
}

bool CoordinateTransformer::isPointInViewport(float x, float y, int vpX, int vpY, 
                                              int vpWidth, int vpHeight) const {
	return x >= vpX && x <= (vpX + vpWidth) && 
	       y >= vpY && y <= (vpY + vpHeight);
}

void CoordinateTransformer::updateCanvasSize(int newWidth, int newHeight) {
	m_canvasWidth = newWidth;
	m_canvasHeight = newHeight;
}
