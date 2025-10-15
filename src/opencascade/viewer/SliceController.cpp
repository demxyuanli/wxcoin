#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/SliceController.h"
#include "SceneManager.h"
#include "OCCGeometry.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbRotation.h>

// OpenCASCADE includes for section computation
#include <BRepAlgoAPI_Section.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <gp_Dir.hxx>
#include <Geom_Curve.hxx>

SliceController::SliceController(SceneManager* sceneManager, SoSeparator* root)
	: m_sceneManager(sceneManager), m_root(root) {
}

void SliceController::attachRoot(SoSeparator* root) {
	if (m_root == root) return;
	// Detach current nodes from old root
	removeNodes();
	m_root = root;
	if (m_enabled) {
		ensureNodes();
		updateNodes();
	}
}

void SliceController::setEnabled(bool enabled) {
	if (m_enabled == enabled) return;
	m_enabled = enabled;
	if (m_enabled) {
		// Check if there are geometries available
		if (m_geometries.empty()) {
			m_enabled = false;
			return;
		}
		
		// Initialize plane at scene center for immediate visible effect
		if (m_sceneManager) {
			SbVec3f bbMin, bbMax;
			m_sceneManager->getSceneBoundingBoxMinMax(bbMin, bbMax);
			SbVec3f center = (bbMin + bbMax) * 0.5f;
			SbVec3f n = m_normal; n.normalize(); if (n.length() < 1e-6f) n = SbVec3f(0, 0, 1);
			m_offset = n.dot(center);
		}
		ensureNodes();
		updateNodes();
	}
	else {
		removeNodes();
	}
}

void SliceController::setPlane(const SbVec3f& normal, float offset) {
	m_normal = normal;
	m_offset = offset;
	if (m_enabled) {
		ensureNodes();
		updateNodes();
	}
}

void SliceController::moveAlongNormal(float delta) {
	m_offset += delta;
	if (m_enabled) updateNodes();
}

void SliceController::ensureNodes() {
	if (!m_root) return;
	if (!m_clipPlane) {
		m_clipPlane = new SoClipPlane;
		// Insert at the beginning so it affects subsequent geometry
		m_root->insertChild(m_clipPlane, 0);
	}
	if (!m_sliceVisual) {
		m_sliceVisual = new SoSeparator;
		m_root->addChild(m_sliceVisual);
		createAdaptivePlaneVisual();
		updateVisualizationSize();
	}
	if (!m_borderFrame) {
		m_borderFrame = new SoSeparator;
		m_root->addChild(m_borderFrame);
		createBorderFrame();
	}
}

void SliceController::updateNodes() {
	if (!m_clipPlane) return;
	SbVec3f n = m_normal;
	n.normalize();
	SbVec3f point = n * m_offset;
	m_clipPlane->plane.setValue(SbPlane(n, point));

	if (m_sliceVisual && m_sliceTransform) {
		// Update visual plane position and orientation to match border
		m_sliceTransform->translation.setValue(point);
		
		// Orient the plane to match the border frame orientation
		SbVec3f zAxis(0, 0, 1);
		SbRotation rot(zAxis, n);
		m_sliceTransform->rotation.setValue(rot);

		// Update material color and transparency if it exists
		for (int i = 0; i < m_sliceVisual->getNumChildren(); ++i) {
			SoNode* child = m_sliceVisual->getChild(i);
			if (SoMaterial* mat = dynamic_cast<SoMaterial*>(child)) {
				mat->diffuseColor.setValue(m_planeColor);
				mat->emissiveColor.setValue(m_planeColor * 0.1f);
				mat->transparency.setValue(m_planeOpacity);
				break;
			}
		}

		// Update visualization size based on current scene
		updateVisualizationSize();
	}

	// Update section contours if enabled
	if (m_showSectionContours) {
		updateSectionContours();
	}

	// Update border frame
	updateBorderFrame();
}

void SliceController::setShowSectionContours(bool show) {
	if (m_showSectionContours == show) return;
	m_showSectionContours = show;
	if (m_enabled) {
		if (show) {
			updateSectionContours();
		} else if (m_sectionContours) {
			m_root->removeChild(m_sectionContours);
			m_sectionContours = nullptr;
		}
	}
}

void SliceController::setPlaneColor(const SbVec3f& color) {
	m_planeColor = color;
	if (m_enabled && m_sliceVisual) {
		// Update the material color in the visual representation
		updateNodes();
	}
}

void SliceController::setPlaneOpacity(float opacity) {
	m_planeOpacity = opacity;
	if (m_enabled && m_sliceVisual) {
		// Update the material transparency in the visual representation
		updateNodes();
	}
}

void SliceController::setGeometries(const std::vector<OCCGeometry*>& geometries) {
	m_geometries = geometries;
	if (m_enabled && m_showSectionContours) {
		updateSectionContours();
	}
}

void SliceController::setDragEnabled(bool enabled) {
	m_dragEnabled = enabled;
}

void SliceController::updateVisualizationSize() {
	if (!m_sceneManager || !m_sliceVisual) return;

	// Don't update if no geometries
	if (m_geometries.empty()) return;

	SbVec3f bbMin, bbMax;
	m_sceneManager->getSceneBoundingBoxMinMax(bbMin, bbMax);

	// Calculate scene diagonal for adaptive sizing
	SbVec3f diagonal = bbMax - bbMin;
	float sceneSize = diagonal.length();

	// Calculate exact border size (same as border frame)
	float borderSize = sceneSize * 1.2f;

	// Update the scale node to exactly match border frame size
	for (int i = 0; i < m_sliceVisual->getNumChildren(); ++i) {
		SoNode* child = m_sliceVisual->getChild(i);
		if (SoScale* scale = dynamic_cast<SoScale*>(child)) {
			// Set plane size to exactly match the border frame
			// SoCube is 2x2x2 by default, so scale factor should be borderSize/2
			// to get final size of borderSize x borderSize
			float scaleFactor = borderSize * 0.5f;
			scale->scaleFactor.setValue(scaleFactor, scaleFactor, 0.005f);
			break;
		}
	}
}

void SliceController::createAdaptivePlaneVisual() {
	if (!m_sliceVisual) return;

	// Clear existing visual
	m_sliceVisual->removeAllChildren();

	// Create transform for plane positioning
	m_sliceTransform = new SoTransform;
	m_sliceVisual->addChild(m_sliceTransform);

	// Create scale for adaptive sizing
	SoScale* scale = new SoScale;
	m_sliceVisual->addChild(scale);

	// Create material for plane appearance
	SoMaterial* mat = new SoMaterial;
	mat->diffuseColor.setValue(m_planeColor);
	mat->transparency.setValue(m_planeOpacity);
	mat->emissiveColor.setValue(m_planeColor * 0.1f);
	m_sliceVisual->addChild(mat);

	// Create the plane geometry (thin box)
	SoCube* plane = new SoCube;
	m_sliceVisual->addChild(plane);
}

void SliceController::createBorderFrame() {
	if (!m_borderFrame) return;

	// Clear existing content
	m_borderFrame->removeAllChildren();

	// The border frame will be created in updateBorderFrame
	// with appropriate size based on scene bounds
}

void SliceController::updateBorderFrame() {
	if (!m_borderFrame || !m_sceneManager) return;

	// Clear existing border
	m_borderFrame->removeAllChildren();

	// Don't create border if no geometries
	if (m_geometries.empty()) return;

	SbVec3f bbMin, bbMax;
	m_sceneManager->getSceneBoundingBoxMinMax(bbMin, bbMax);
	SbVec3f diagonal = bbMax - bbMin;
	float sceneSize = diagonal.length();
	float borderSize = sceneSize * 1.2f; // Slightly larger than scene

	// Get plane position and normal
	SbVec3f n = m_normal;
	n.normalize();
	SbVec3f planeCenter = n * m_offset;

	// Create transform for border positioning
	SoTransform* borderTransform = new SoTransform;
	borderTransform->translation.setValue(planeCenter);
	
	// Orient border to match plane
	SbVec3f zAxis(0, 0, 1);
	SbRotation rot(zAxis, n);
	borderTransform->rotation.setValue(rot);
	m_borderFrame->addChild(borderTransform);

	// Create red material for border
	SoMaterial* borderMat = new SoMaterial;
	borderMat->diffuseColor.setValue(1.0f, 0.0f, 0.0f); // Red
	borderMat->emissiveColor.setValue(0.3f, 0.0f, 0.0f);
	m_borderFrame->addChild(borderMat);

	// Set line width to 3 pixels
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->lineWidth.setValue(3.0f);
	drawStyle->linePattern.setValue(0xFFFF); // Solid line
	m_borderFrame->addChild(drawStyle);

	// Create 4 line segments forming a square border
	SoCoordinate3* coords = new SoCoordinate3;
	float half = borderSize * 0.5f;
	
	// Square border vertices (in plane's local XY space, Z=0)
	SbVec3f vertices[5];
	vertices[0] = SbVec3f(-half, -half, 0);
	vertices[1] = SbVec3f(half, -half, 0);
	vertices[2] = SbVec3f(half, half, 0);
	vertices[3] = SbVec3f(-half, half, 0);
	vertices[4] = SbVec3f(-half, -half, 0); // Close the loop
	
	coords->point.setValues(0, 5, vertices);
	m_borderFrame->addChild(coords);

	// Create line set
	SoLineSet* lineSet = new SoLineSet;
	lineSet->numVertices.setValue(5); // 5 vertices form a closed square
	m_borderFrame->addChild(lineSet);
}

void SliceController::updateSectionContours() {
	if (!m_showSectionContours || m_geometries.empty()) return;

	if (!m_sectionContours) {
		m_sectionContours = new SoSeparator;
		m_root->addChild(m_sectionContours);
	}

	// Clear existing contours
	m_sectionContours->removeAllChildren();

	// Create material for contours (bright color to stand out)
	SoMaterial* contourMat = new SoMaterial;
	contourMat->diffuseColor.setValue(1.0f, 1.0f, 0.0f); // Yellow contours
	contourMat->emissiveColor.setValue(0.3f, 0.3f, 0.0f);
	m_sectionContours->addChild(contourMat);

	// Compute section for each geometry
	for (OCCGeometry* geom : m_geometries) {
		if (!geom || !geom->isValid()) continue;

		try {
			// Define the cutting plane
			gp_Pln cuttingPlane(gp_Pnt(m_normal[0] * m_offset,
									 m_normal[1] * m_offset,
									 m_normal[2] * m_offset),
							   gp_Dir(m_normal[0], m_normal[1], m_normal[2]));

			// Compute section
			BRepAlgoAPI_Section sectionAlgo(geom->getShape(), cuttingPlane, Standard_False);
			sectionAlgo.Build();

			if (sectionAlgo.IsDone()) {
				const TopoDS_Shape& sectionShape = sectionAlgo.Shape();

				// Extract edges from the section
				std::vector<SbVec3f> allPoints;
				std::vector<int32_t> lineIndices;

				for (TopExp_Explorer edgeExp(sectionShape, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
					const TopoDS_Edge& edge = TopoDS::Edge(edgeExp.Current());

					// Get curve points
					std::vector<SbVec3f> edgePoints;
					if (extractEdgePoints(edge, edgePoints)) {
						if (!edgePoints.empty()) {
							// Add points to allPoints
							size_t startIndex = allPoints.size();
							allPoints.insert(allPoints.end(), edgePoints.begin(), edgePoints.end());

							// Add line indices (connect points sequentially)
							for (size_t i = 0; i < edgePoints.size() - 1; ++i) {
								lineIndices.push_back(static_cast<int32_t>(startIndex + i));
								lineIndices.push_back(static_cast<int32_t>(startIndex + i + 1));
							}
						}
					}
				}

				// Create Coin3D nodes for this geometry's contours
				if (!allPoints.empty() && !lineIndices.empty()) {
					SoSeparator* geomContours = new SoSeparator;

					// Coordinates
					SoCoordinate3* coords = new SoCoordinate3;
					coords->point.setValues(0, allPoints.size(), &allPoints[0]);
					geomContours->addChild(coords);

					// Line set
					SoLineSet* lineSet = new SoLineSet;
					lineSet->numVertices.setValues(0, lineIndices.size(), &lineIndices[0]);
					geomContours->addChild(lineSet);

					m_sectionContours->addChild(geomContours);
				}
			}
		}
		catch (const Standard_Failure& e) {
			// Skip this geometry if section computation fails
			continue;
		}
	}
}

bool SliceController::extractEdgePoints(const TopoDS_Edge& edge, std::vector<SbVec3f>& points) {
	try {
		// Get the curve from the edge
		Standard_Real first, last;
		Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

		if (curve.IsNull()) return false;

		// Sample points along the curve
		const int numSamples = 50; // Adjust for smoothness vs performance
		points.reserve(numSamples);

		for (int i = 0; i < numSamples; ++i) {
			Standard_Real param = first + (last - first) * i / (numSamples - 1.0);
			gp_Pnt p = curve->Value(param);
			points.emplace_back(static_cast<float>(p.X()),
							   static_cast<float>(p.Y()),
							   static_cast<float>(p.Z()));
		}

		return !points.empty();
	}
	catch (const Standard_Failure&) {
		return false;
	}
}

bool SliceController::handleMousePress(const SbVec2s* mousePos, const SbViewportRegion* vp) {
	// Only handle mouse if drag mode is enabled
	if (!m_enabled || !m_borderFrame || !mousePos || !vp || !m_dragEnabled) return false;

	// Check if mouse is over the border frame
	// The border provides a clear interaction target
	if (isMouseOverBorder(*mousePos, *vp)) {
		m_isInteracting = true;
		m_lastMousePos = *mousePos;
		m_interactionOffset = m_offset;
		m_interactionNormal = m_normal;
		return true;
	}

	return false;
}

bool SliceController::handleMouseMove(const SbVec2s* mousePos, const SbViewportRegion* vp) {
	if (!m_isInteracting || !m_enabled || !mousePos) return false;

	// Calculate the movement delta
	SbVec2s delta = *mousePos - m_lastMousePos;

	// Convert screen space delta to world space movement
	// Simple approach: move along normal based on vertical mouse movement
	float moveDelta = -delta[1] * 0.01f; // Negative because Y increases downward

	// Update the plane offset
	m_offset = m_interactionOffset + moveDelta;

	// Ensure we don't go too far from scene bounds
	if (m_sceneManager) {
		SbVec3f bbMin, bbMax;
		m_sceneManager->getSceneBoundingBoxMinMax(bbMin, bbMax);
		SbVec3f diagonal = bbMax - bbMin;
		float sceneSize = diagonal.length();

		// Clamp offset to reasonable bounds
		float maxOffset = sceneSize * 2.0f;
		float minOffset = -sceneSize * 2.0f;
		m_offset = std::max(minOffset, std::min(maxOffset, m_offset));
	}

	// Update the slice plane and visuals
	updateNodes();

	m_lastMousePos = *mousePos;
	return true;
}

bool SliceController::handleMouseRelease(const SbVec2s* mousePos, const SbViewportRegion* vp) {
	if (!m_isInteracting) return false;

	m_isInteracting = false;
	return true;
}

bool SliceController::isMouseOverPlane(const SbVec2s& mousePos, const SbViewportRegion& vp) {
	// Delegate to border detection
	return isMouseOverBorder(mousePos, vp);
}

bool SliceController::isMouseOverBorder(const SbVec2s& mousePos, const SbViewportRegion& vp) {
	// TODO: Implement proper 3D picking for the border frame
	// This requires integration with the viewer's SoRayPickAction system
	// The border frame is visible and provides a clear interaction target
	// For now, return false to ensure canvas operations are not affected
	return false;
}

void SliceController::removeNodes() {
	if (!m_root) return;
	if (m_clipPlane) {
		m_root->removeChild(m_clipPlane);
		m_clipPlane = nullptr;
	}
	if (m_sliceVisual) {
		m_root->removeChild(m_sliceVisual);
		m_sliceVisual = nullptr;
		m_sliceTransform = nullptr;
	}
	if (m_sectionContours) {
		m_root->removeChild(m_sectionContours);
		m_sectionContours = nullptr;
	}
	if (m_borderFrame) {
		m_root->removeChild(m_borderFrame);
		m_borderFrame = nullptr;
	}
}