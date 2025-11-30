#include "NavigationStyle.h"
#include "Canvas.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <cmath>
#include <wx/wx.h>

NavigationStyle::NavigationStyle(Canvas* canvas)
	: m_canvas(canvas)
	, m_isRotating(false)
	, m_isPanning(false)
	, m_rotationSensitivity(0.2f)
	, m_panSensitivity(0.01f)
	, m_zoomSensitivity(0.1f)
{
}

NavigationStyle::~NavigationStyle()
{
}

void NavigationStyle::handleMouseButton(const wxMouseEvent& event)
{
	if (event.LeftDown()) {
		m_isRotating = true;
		m_lastMousePos = event.GetPosition();
		m_canvas->CaptureMouse();
	}
	else if (event.RightDown()) {
		m_isPanning = true;
		m_lastMousePos = event.GetPosition();
		m_canvas->CaptureMouse();
	}
	else if (event.LeftUp()) {
		m_isRotating = false;
		if (m_canvas->HasCapture()) {
			m_canvas->ReleaseMouse();
		}
	}
	else if (event.RightUp()) {
		m_isPanning = false;
		if (m_canvas->HasCapture()) {
			m_canvas->ReleaseMouse();
		}
	}
}

void NavigationStyle::handleMouseMotion(const wxMouseEvent& event)
{
	if (m_isRotating) {
		rotateCamera(event.GetPosition());
	}
	else if (m_isPanning) {
		panCamera(event.GetPosition());
	}
}

void NavigationStyle::handleMouseWheel(const wxMouseEvent& event)
{
	int delta = event.GetWheelRotation();
	zoomCamera(delta);
}

void NavigationStyle::rotateCamera(const wxPoint& mousePos)
{
	if (!m_canvas || !m_canvas->getCamera())
		return;

	float dx = (mousePos.x - m_lastMousePos.x) * m_rotationSensitivity;
	float dy = (mousePos.y - m_lastMousePos.y) * m_rotationSensitivity;

	SoCamera* camera = m_canvas->getCamera();

	// Get the current camera position and orientation
	SbVec3f cameraPos = camera->position.getValue();
	SbRotation cameraRot = camera->orientation.getValue();

	// Get the world coordinate axes
	SbVec3f worldUp(0.0f, 1.0f, 0.0f);
	SbVec3f worldRight(1.0f, 0.0f, 0.0f);

	// Calculate rotation angles in radians
	float rotX_rad = -dy * 0.01f;
	float rotY_rad = -dx * 0.01f;

	// Create rotation around Y axis (world up)
	SbRotation rotY(worldUp, rotY_rad);

	// Create rotation around X axis (world right)
	SbRotation rotX(worldRight, rotX_rad);

	// Calculate direction from camera to origin
	SbVec3f dirToOrigin = -cameraPos;
	float distToOrigin = dirToOrigin.length();

	// Create unit vector for rotation
	SbVec3f normDir = dirToOrigin;
	normDir.normalize();

	// Apply rotations
	SbVec3f newDir;
	rotY.multVec(normDir, newDir);
	rotX.multVec(newDir, newDir);

	// Maintain distance from origin
	newDir = -newDir * distToOrigin;

	// Update camera position
	camera->position.setValue(newDir);

	// Calculate view direction (always looking at origin)
	SbVec3f viewDir = -newDir;
	viewDir.normalize();

	SbVec3f upDir;
	cameraRot.multVec(SbVec3f(0.0f, 1.0f, 0.0f), upDir);

	// Create camera orientation matrix to look at origin
	SbRotation newRot;
	newRot.setValue(SbVec3f(0.0f, 0.0f, -1.0f), viewDir);

	// Set camera orientation
	camera->orientation.setValue(newRot);

	m_lastMousePos = mousePos;

	if (m_canvas) m_canvas->Refresh(false);
}

void NavigationStyle::panCamera(const wxPoint& currentPos)
{
	SoCamera* camera = m_canvas->getCamera();
	if (!camera || !m_canvas) return;

	float dx = (currentPos.x - m_lastMousePos.x) * m_panSensitivity;
	float dy = (currentPos.y - m_lastMousePos.y) * m_panSensitivity;

	SbVec3f forward, up, right;
	SbRotation camOrientation = camera->orientation.getValue();
	camOrientation.multVec(SbVec3f(0.0f, 0.0f, -1.0f), forward);
	camOrientation.multVec(SbVec3f(0.0f, 1.0f, 0.0f), up);
	right = forward.cross(up);
	right.normalize();
	up.normalize();

	float distance = camera->focalDistance.getValue();
	if (distance < 0.1f) distance = 0.1f;

	float scale = distance * 0.1f;

	SbVec3f translationVec = (right * -dx) + (up * dy);
	translationVec *= scale;

	camera->position.setValue(camera->position.getValue() + translationVec);

	m_lastMousePos = currentPos;
	m_canvas->Refresh(false);
}

void NavigationStyle::zoomCamera(int delta)
{
	if (!m_canvas || !m_canvas->getCamera())
		return;

	SoCamera* camera = m_canvas->getCamera();
	SbVec3f cameraPos = camera->position.getValue();

	SbVec3f viewDirection;
	camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), viewDirection);

	SbVec3f focalPoint = cameraPos + viewDirection * camera->focalDistance.getValue();

	float zoomFactor = 1.0f - (delta * m_zoomSensitivity * 0.001f);

	SbVec3f directionToFocal = focalPoint - cameraPos;
	SbVec3f newCamPos = cameraPos + directionToFocal * (1.0f - zoomFactor);

	SbVec3f newDirectionToFocal = focalPoint - newCamPos;
	if (directionToFocal.dot(newDirectionToFocal) <= 0) {
	}
	else {
		float newDistToFocal = newDirectionToFocal.length();
		const float MIN_DIST_TO_FOCAL = 0.1f;
		const float MAX_DIST_TO_FOCAL = 10000.0f;

		if (newDistToFocal < MIN_DIST_TO_FOCAL) {
			SbVec3f normalizedDir = newDirectionToFocal;
			float length = normalizedDir.normalize();
			if (length > 0.0f) {
				newCamPos = focalPoint - normalizedDir * MIN_DIST_TO_FOCAL;
			}
		}
		else if (newDistToFocal > MAX_DIST_TO_FOCAL) {
			SbVec3f normalizedDir = newDirectionToFocal;
			float length = normalizedDir.normalize();
			if (length > 0.0f) {
				newCamPos = focalPoint - normalizedDir * MAX_DIST_TO_FOCAL;
			}
		}
		camera->position.setValue(newCamPos);
		newDistToFocal = (focalPoint - newCamPos).length();
		camera->focalDistance.setValue(newDistToFocal);
		
		// Update far distance to ensure geometry is not clipped when zooming out
		// Use a larger multiplier to provide more safety margin
		// FreeCAD approach: Ensure far plane is always large enough to avoid clipping
		if (SoPerspectiveCamera* persCamera = dynamic_cast<SoPerspectiveCamera*>(camera)) {
			// Use larger multiplier (10.0f instead of 2.0f) to prevent clipping when zooming out
			// Also ensure minimum far distance to handle very large scenes
			float farDist = std::max(newDistToFocal * 10.0f, 100000.0f);
			persCamera->farDistance.setValue(farDist);
		}
		else if (SoOrthographicCamera* orthoCamera = dynamic_cast<SoOrthographicCamera*>(camera)) {
			// Use larger multiplier for orthographic camera as well
			float farDist = std::max(newDistToFocal * 10.0f, 100000.0f);
			orthoCamera->farDistance.setValue(farDist);
		}
	}

	if (m_canvas) m_canvas->Refresh(false);
}

void NavigationStyle::viewAll()
{
	if (!m_canvas || !m_canvas->getCamera()) {
		return;
	}
	m_canvas->resetView();
}

void NavigationStyle::viewTop()
{
	if (!m_canvas || !m_canvas->getCamera()) {
		return;
	}
	SoCamera* camera = m_canvas->getCamera();
	float focalDist = camera->focalDistance.getValue();
	if (focalDist <= 0) focalDist = 10.0f;
	camera->position.setValue(0.0f, focalDist, 0.0f);
	camera->orientation.setValue(SbRotation(SbVec3f(1, 0, 0), -M_PI_2));
	if (m_canvas) m_canvas->Refresh(false);
}

void NavigationStyle::viewFront()
{
	if (!m_canvas || !m_canvas->getCamera()) {
		return;
	}
	SoCamera* camera = m_canvas->getCamera();
	float focalDist = camera->focalDistance.getValue();
	if (focalDist <= 0) focalDist = 10.0f;
	camera->position.setValue(0.0f, 0.0f, focalDist);
	camera->orientation.setValue(SbRotation::identity());
	if (m_canvas) m_canvas->Refresh(false);
}

void NavigationStyle::viewRight()
{
	if (!m_canvas || !m_canvas->getCamera()) {
		return;
	}
	SoCamera* camera = m_canvas->getCamera();
	float focalDist = camera->focalDistance.getValue();
	if (focalDist <= 0) focalDist = 10.0f;
	camera->position.setValue(focalDist, 0.0f, 0.0f);
	camera->orientation.setValue(SbRotation(SbVec3f(0, 1, 0), M_PI_2));
	if (m_canvas) m_canvas->Refresh(false);
}

void NavigationStyle::viewIsometric()
{
	if (!m_canvas || !m_canvas->getCamera()) {
		return;
	}
	SoCamera* camera = m_canvas->getCamera();
	float focalDist = camera->focalDistance.getValue();
	if (focalDist <= 0) focalDist = 10.0f;

	SbRotation rotY(SbVec3f(0, 1, 0), M_PI / 4.0);
	SbRotation rotX(SbVec3f(1, 0, 0), asin(tan(M_PI / 6.0)));

	camera->orientation.setValue(rotY * rotX);

	SbVec3f zAxis;
	camera->orientation.getValue().multVec(SbVec3f(0, 0, 1), zAxis);
	camera->position.setValue(zAxis * focalDist);

	if (m_canvas) m_canvas->Refresh(false);
}