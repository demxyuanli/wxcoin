#include "CoordinateSystemRenderer.h"
#include "DPIAwareRendering.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoTransform.h>
#include <algorithm>

const float CoordinateSystemRenderer::DEFAULT_COORD_PLANE_SIZE = 4.0f;
const float CoordinateSystemRenderer::COORD_PLANE_TRANSPARENCY = 1.0f;

CoordinateSystemRenderer::CoordinateSystemRenderer(SoSeparator* objectRoot)
	: m_objectRoot(objectRoot)
	, m_coordSystemSeparator(nullptr)
	, m_coordSystemSwitch(nullptr)
	, m_currentPlaneSize(DEFAULT_COORD_PLANE_SIZE)
	, m_visible(true)
{
	// Initialize with default colors (assuming light background)
	m_planeColor[0] = m_planeColor[1] = m_planeColor[2] = 0.7f;  // Light gray for planes
	m_lineColor[0] = m_lineColor[1] = m_lineColor[2] = 0.5f;    // Medium gray for lines

	LOG_INF_S("CoordinateSystemRenderer initializing");
	createCoordinateSystem();
}

CoordinateSystemRenderer::~CoordinateSystemRenderer() {
	LOG_INF_S("CoordinateSystemRenderer destroying");
}

void CoordinateSystemRenderer::updateCoordinateSystemSize(float sceneSize)
{
	// Calculate appropriate coordinate system size based on scene size
	// Make coordinate system occupy about 60% of the scene size, with reasonable bounds
	float newSize = (std::max)(1.0f, (std::min)(sceneSize * 0.6f, sceneSize * 2.0f));

	if (std::abs(newSize - m_currentPlaneSize) > 0.1f) {
		m_currentPlaneSize = newSize;
		LOG_INF_S("Updating coordinate system size to: " + std::to_string(m_currentPlaneSize));
		rebuildCoordinateSystem();
	}
}

void CoordinateSystemRenderer::setCoordinateSystemScale(float scale)
{
	m_currentPlaneSize = DEFAULT_COORD_PLANE_SIZE * scale;
	LOG_INF_S("Setting coordinate system scale to: " + std::to_string(scale) +
		" (size: " + std::to_string(m_currentPlaneSize) + ")");
	rebuildCoordinateSystem();
}

void CoordinateSystemRenderer::rebuildCoordinateSystem()
{
	// Remove existing coordinate system if it exists
	if (m_coordSystemSwitch && m_objectRoot) {
		m_objectRoot->removeChild(m_coordSystemSwitch);
		m_coordSystemSwitch = nullptr;
		m_coordSystemSeparator = nullptr;
	}

	// Create new coordinate system with current size
	createCoordinateSystem();
}

void CoordinateSystemRenderer::createCoordinateSystem() {
	// Create switch node for visibility control
	m_coordSystemSwitch = new SoSwitch;
	m_coordSystemSwitch->whichChild.setValue(SO_SWITCH_ALL); // Show all children by default

	m_coordSystemSeparator = new SoSeparator;
	SoTransform* originTransform = new SoTransform;
	originTransform->translation.setValue(0.0f, 0.0f, 0.0f);
	originTransform->rotation.setValue(SbRotation::identity());
	originTransform->scaleFactor.setValue(1.0f, 1.0f, 1.0f);
	m_coordSystemSeparator->addChild(originTransform);

	SoShapeHints* hints = new SoShapeHints;
	hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
	hints->shapeType = SoShapeHints::SOLID;
	m_coordSystemSeparator->addChild(hints);

	SoDrawStyle* globalStyle = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.0f);
	globalStyle->linePattern = 0xFFFF;
	m_coordSystemSeparator->addChild(globalStyle);

	// Use current plane size instead of constant
	float s = m_currentPlaneSize / 2.0f;

	// X plane (YZ plane) - use dynamic color
	SoSeparator* xPlaneSep = new SoSeparator;
	SoMaterial* xMaterial = new SoMaterial;
	xMaterial->diffuseColor.setValue(m_planeColor[0], m_planeColor[1], m_planeColor[2]);
	xMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
	xPlaneSep->addChild(xMaterial);

	SoDrawStyle* xDrawStyle = new SoDrawStyle;
	xDrawStyle->style = SoDrawStyle::FILLED;
	xPlaneSep->addChild(xDrawStyle);

	SoFaceSet* xFaceSet = new SoFaceSet;
	SoVertexProperty* xVertices = new SoVertexProperty;
	xVertices->vertex.set1Value(0, SbVec3f(0.0f, -s, -s));
	xVertices->vertex.set1Value(1, SbVec3f(0.0f, s, -s));
	xVertices->vertex.set1Value(2, SbVec3f(0.0f, s, s));
	xVertices->vertex.set1Value(3, SbVec3f(0.0f, -s, s));
	xFaceSet->vertexProperty = xVertices;
	xFaceSet->numVertices.set1Value(0, 4);
	xPlaneSep->addChild(xFaceSet);

	SoSeparator* xLineSep = new SoSeparator;
	SoMaterial* xLineMaterial = new SoMaterial;
	xLineMaterial->diffuseColor.setValue(m_lineColor[0], m_lineColor[1], m_lineColor[2]);
	xLineMaterial->transparency.setValue(0.0f);
	xLineSep->addChild(xLineMaterial);

	SoDrawStyle* xLineStyle = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.0f);
	xLineStyle->style = SoDrawStyle::LINES;
	xLineSep->addChild(xLineStyle);

	SoIndexedLineSet* xLines = new SoIndexedLineSet;
	xLines->vertexProperty = xVertices;
	xLines->coordIndex.set1Value(0, 0);
	xLines->coordIndex.set1Value(1, 1);
	xLines->coordIndex.set1Value(2, 2);
	xLines->coordIndex.set1Value(3, 3);
	xLines->coordIndex.set1Value(4, 0);
	xLines->coordIndex.set1Value(5, -1);
	xLineSep->addChild(xLines);
	xPlaneSep->addChild(xLineSep);
	m_coordSystemSeparator->addChild(xPlaneSep);

	// Y plane (XZ plane) - use dynamic color
	SoSeparator* yPlaneSep = new SoSeparator;
	SoMaterial* yMaterial = new SoMaterial;
	yMaterial->diffuseColor.setValue(m_planeColor[0], m_planeColor[1], m_planeColor[2]);
	yMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
	yPlaneSep->addChild(yMaterial);

	SoDrawStyle* yDrawStyle = new SoDrawStyle;
	yDrawStyle->style = SoDrawStyle::FILLED;
	yPlaneSep->addChild(yDrawStyle);

	SoFaceSet* yFaceSet = new SoFaceSet;
	SoVertexProperty* yVertices = new SoVertexProperty;
	yVertices->vertex.set1Value(0, SbVec3f(-s, 0.0f, -s));
	yVertices->vertex.set1Value(1, SbVec3f(s, 0.0f, -s));
	yVertices->vertex.set1Value(2, SbVec3f(s, 0.0f, s));
	yVertices->vertex.set1Value(3, SbVec3f(-s, 0.0f, s));
	yFaceSet->vertexProperty = yVertices;
	yFaceSet->numVertices.set1Value(0, 4);
	yPlaneSep->addChild(yFaceSet);

	SoSeparator* yLineSep = new SoSeparator;
	SoMaterial* yLineMaterial = new SoMaterial;
	yLineMaterial->diffuseColor.setValue(m_lineColor[0], m_lineColor[1], m_lineColor[2]);
	yLineMaterial->transparency.setValue(0.0f);
	yLineSep->addChild(yLineMaterial);

	SoDrawStyle* yLineStyle = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.0f);
	yLineStyle->style = SoDrawStyle::LINES;
	yLineSep->addChild(yLineStyle);

	SoIndexedLineSet* yLines = new SoIndexedLineSet;
	yLines->vertexProperty = yVertices;
	yLines->coordIndex.set1Value(0, 0);
	yLines->coordIndex.set1Value(1, 1);
	yLines->coordIndex.set1Value(2, 2);
	yLines->coordIndex.set1Value(3, 3);
	yLines->coordIndex.set1Value(4, 0);
	yLines->coordIndex.set1Value(5, -1);
	yLineSep->addChild(yLines);
	yPlaneSep->addChild(yLineSep);
	m_coordSystemSeparator->addChild(yPlaneSep);

	// Z plane (XY plane) - use dynamic color
	SoSeparator* zPlaneSep = new SoSeparator;
	SoMaterial* zMaterial = new SoMaterial;
	zMaterial->diffuseColor.setValue(m_planeColor[0], m_planeColor[1], m_planeColor[2]);
	zMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
	zPlaneSep->addChild(zMaterial);

	SoDrawStyle* zDrawStyle = new SoDrawStyle;
	zDrawStyle->style = SoDrawStyle::FILLED;
	zPlaneSep->addChild(zDrawStyle);

	SoFaceSet* zFaceSet = new SoFaceSet;
	SoVertexProperty* zVertices = new SoVertexProperty;
	zVertices->vertex.set1Value(0, SbVec3f(-s, -s, 0.0f));
	zVertices->vertex.set1Value(1, SbVec3f(s, -s, 0.0f));
	zVertices->vertex.set1Value(2, SbVec3f(s, s, 0.0f));
	zVertices->vertex.set1Value(3, SbVec3f(-s, s, 0.0f));
	zFaceSet->vertexProperty = zVertices;
	zFaceSet->numVertices.set1Value(0, 4);
	zPlaneSep->addChild(zFaceSet);

	SoSeparator* zLineSep = new SoSeparator;
	SoMaterial* zLineMaterial = new SoMaterial;
	zLineMaterial->diffuseColor.setValue(m_lineColor[0], m_lineColor[1], m_lineColor[2]);
	zLineMaterial->transparency.setValue(0.0f);
	zLineSep->addChild(zLineMaterial);

	SoDrawStyle* zLineStyle = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.0f);
	zLineStyle->style = SoDrawStyle::LINES;
	zLineSep->addChild(zLineStyle);

	SoIndexedLineSet* zLines = new SoIndexedLineSet;
	zLines->vertexProperty = zVertices;
	zLines->coordIndex.set1Value(0, 0);
	zLines->coordIndex.set1Value(1, 1);
	zLines->coordIndex.set1Value(2, 2);
	zLines->coordIndex.set1Value(3, 3);
	zLines->coordIndex.set1Value(4, 0);
	zLines->coordIndex.set1Value(5, -1);
	zLineSep->addChild(zLines);
	zPlaneSep->addChild(zLineSep);
	m_coordSystemSeparator->addChild(zPlaneSep);

	SoSeparator* xAxisSep = new SoSeparator;
	SoMaterial* xAxisMaterial = new SoMaterial;
	xAxisMaterial->diffuseColor.setValue(m_lineColor[0], m_lineColor[1], m_lineColor[2]);
	xAxisMaterial->transparency.setValue(0.0f);
	xAxisSep->addChild(xAxisMaterial);

	SoDrawStyle* xAxisStyle = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.0f);
	xAxisSep->addChild(xAxisStyle);

	SoCoordinate3* xAxisCoords = new SoCoordinate3;
	xAxisCoords->point.set1Value(0, SbVec3f(-s, 0.0f, 0.0f));
	xAxisCoords->point.set1Value(1, SbVec3f(s, 0.0f, 0.0f));
	xAxisSep->addChild(xAxisCoords);

	SoLineSet* xAxisLine = new SoLineSet;
	xAxisLine->numVertices.setValue(2);
	xAxisSep->addChild(xAxisLine);
	m_coordSystemSeparator->addChild(xAxisSep);

	SoSeparator* yAxisSep = new SoSeparator;
	SoMaterial* yAxisMaterial = new SoMaterial;
	yAxisMaterial->diffuseColor.setValue(m_lineColor[0], m_lineColor[1], m_lineColor[2]);
	yAxisMaterial->transparency.setValue(0.0f);
	yAxisSep->addChild(yAxisMaterial);

	SoDrawStyle* yAxisStyle = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.0f);
	yAxisSep->addChild(yAxisStyle);

	SoCoordinate3* yAxisCoords = new SoCoordinate3;
	yAxisCoords->point.set1Value(0, SbVec3f(0.0f, -s, 0.0f));
	yAxisCoords->point.set1Value(1, SbVec3f(0.0f, s, 0.0f));
	yAxisSep->addChild(yAxisCoords);

	SoLineSet* yAxisLine = new SoLineSet;
	yAxisLine->numVertices.setValue(2);
	yAxisSep->addChild(yAxisLine);
	m_coordSystemSeparator->addChild(yAxisSep);

	SoSeparator* zAxisSep = new SoSeparator;
	SoMaterial* zAxisMaterial = new SoMaterial;
	zAxisMaterial->diffuseColor.setValue(m_lineColor[0], m_lineColor[1], m_lineColor[2]);
	zAxisMaterial->transparency.setValue(0.0f);
	zAxisSep->addChild(zAxisMaterial);

	SoDrawStyle* zAxisStyle = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.0f);
	zAxisSep->addChild(zAxisStyle);

	SoCoordinate3* zAxisCoords = new SoCoordinate3;
	zAxisCoords->point.set1Value(0, SbVec3f(0.0f, 0.0f, -s));
	zAxisCoords->point.set1Value(1, SbVec3f(0.0f, 0.0f, s));
	zAxisSep->addChild(zAxisCoords);

	SoLineSet* zAxisLine = new SoLineSet;
	zAxisLine->numVertices.setValue(2);
	zAxisSep->addChild(zAxisLine);
	m_coordSystemSeparator->addChild(zAxisSep);

	// Add coordinate system separator to switch
	m_coordSystemSwitch->addChild(m_coordSystemSeparator);

	// Add switch to object root
	m_objectRoot->addChild(m_coordSystemSwitch);
}

void CoordinateSystemRenderer::setVisible(bool visible)
{
	if (m_visible != visible) {
		m_visible = visible;

		if (m_coordSystemSwitch) {
			if (visible) {
				m_coordSystemSwitch->whichChild.setValue(SO_SWITCH_ALL); // Show all children
			}
			else {
				m_coordSystemSwitch->whichChild.setValue(SO_SWITCH_NONE); // Hide all children
			}

			// Force a refresh of the switch and its parent
			m_coordSystemSwitch->touch();
			if (m_objectRoot) {
				m_objectRoot->touch();
			}

			LOG_INF_S("Coordinate system visibility set to: " + std::string(visible ? "ON" : "OFF"));
		}
		else
		{
			LOG_ERR_S("Coordinate system switch is null, cannot set visibility");
		}
	}
}

void CoordinateSystemRenderer::calculateColorsForBackground(float backgroundBrightness)
{
	// Calculate contrast colors based on background brightness
	// If background is bright (>0.5), use dark colors
	// If background is dark (<=0.5), use light colors

	if (backgroundBrightness > 0.5f) {
		// Light background - use dark colors
		m_planeColor[0] = m_planeColor[1] = m_planeColor[2] = 0.7f;  // Light gray for planes
		m_lineColor[0] = m_lineColor[1] = m_lineColor[2] = 0.5f;    // Medium gray for lines
	} else {
		// Dark background - use light colors
		m_planeColor[0] = m_planeColor[1] = m_planeColor[2] = 0.7f;  // Light gray for planes
		m_lineColor[0] = m_lineColor[1] = m_lineColor[2] = 0.9f;    // Very light for lines
	}

	LOG_INF_S("CoordinateSystemRenderer: Updated colors for background brightness: " +
		std::to_string(backgroundBrightness) +
		" (plane RGB: " + std::to_string(m_planeColor[0]) + "," + std::to_string(m_planeColor[1]) + "," + std::to_string(m_planeColor[2]) +
		", lines RGB: " + std::to_string(m_lineColor[0]) + "," + std::to_string(m_lineColor[1]) + "," + std::to_string(m_lineColor[2]) + ")");
}

void CoordinateSystemRenderer::updateColorsForBackground(float backgroundBrightness)
{
	calculateColorsForBackground(backgroundBrightness);

	// Rebuild coordinate system with new colors
	if (m_coordSystemSeparator) {
		rebuildCoordinateSystem();
	}
}