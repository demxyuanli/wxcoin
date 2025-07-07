#include "PickingAidManager.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "DPIAwareRendering.h"
#include "Logger.h"
#include "InputManager.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoText2.h>
#include <cstdio>

PickingAidManager::PickingAidManager(SceneManager* sceneManager, Canvas* canvas, InputManager* inputManager)
    : m_sceneManager(sceneManager)
    , m_canvas(canvas)
    , m_inputManager(inputManager)
    , m_pickingAidSeparator(nullptr)
    , m_pickingAidTransform(nullptr)
    , m_pickingAidVisible(false)
    , m_isPickingPosition(false)
    , m_referenceZ(0.0f)
    , m_referenceGridSeparator(nullptr)
    , m_referenceGridVisible(false)
{
    LOG_INF("PickingAidManager initializing");
    createPickingAidLines();
    createReferenceGrid();
}

PickingAidManager::~PickingAidManager() {
    if (m_pickingAidSeparator) {
        m_pickingAidSeparator->unref();
    }
    if (m_referenceGridSeparator) {
        m_referenceGridSeparator->unref();
    }
    LOG_INF("PickingAidManager destroying");
}

void PickingAidManager::createPickingAidLines() {
    m_pickingAidSeparator = new SoSeparator;
    m_pickingAidSeparator->ref();

    m_pickingAidTransform = new SoTransform;
    m_pickingAidSeparator->addChild(m_pickingAidTransform);

    SoDrawStyle* lineStyle = DPIAwareRendering::createDPIAwareGeometryStyle(1.0f, false);
    lineStyle->linePattern.setValue(0xFFFF);
    m_pickingAidSeparator->addChild(lineStyle);

    SoSeparator* xLineSep = new SoSeparator;
    SoMaterial* xMaterial = new SoMaterial;
    xMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
    xLineSep->addChild(xMaterial);
    SoCoordinate3* xCoords = new SoCoordinate3;
    xCoords->point.set1Value(0, SbVec3f(-1000.0f, 0.0f, 0.0f));
    xCoords->point.set1Value(1, SbVec3f(1000.0f, 0.0f, 0.0f));
    xLineSep->addChild(xCoords);
    SoLineSet* xLine = new SoLineSet;
    xLine->numVertices.setValue(2);
    xLineSep->addChild(xLine);
    m_pickingAidSeparator->addChild(xLineSep);

    SoSeparator* yLineSep = new SoSeparator;
    SoMaterial* yMaterial = new SoMaterial;
    yMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
    yLineSep->addChild(yMaterial);
    SoCoordinate3* yCoords = new SoCoordinate3;
    yCoords->point.set1Value(0, SbVec3f(0.0f, -1000.0f, 0.0f));
    yCoords->point.set1Value(1, SbVec3f(0.0f, 1000.0f, 0.0f));
    yLineSep->addChild(yCoords);
    SoLineSet* yLine = new SoLineSet;
    yLine->numVertices.setValue(2);
    yLineSep->addChild(yLine);
    m_pickingAidSeparator->addChild(yLineSep);

    SoSeparator* zLineSep = new SoSeparator;
    SoMaterial* zMaterial = new SoMaterial;
    zMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
    zLineSep->addChild(zMaterial);
    SoCoordinate3* zCoords = new SoCoordinate3;
    zCoords->point.set1Value(0, SbVec3f(0.0f, 0.0f, -1000.0f));
    zCoords->point.set1Value(1, SbVec3f(0.0f, 0.0f, 1000.0f));
    zLineSep->addChild(zCoords);
    SoLineSet* zLine = new SoLineSet;
    zLine->numVertices.setValue(2);
    zLineSep->addChild(zLine);
    m_pickingAidSeparator->addChild(zLineSep);

    SoSeparator* centerSep = new SoSeparator;
    SoMaterial* centerMaterial = new SoMaterial;
    centerMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    centerSep->addChild(centerMaterial);
    SoDrawStyle* pointStyle = new SoDrawStyle;
    pointStyle->pointSize.setValue(5.0f);
    centerSep->addChild(pointStyle);
    SoCoordinate3* centerCoord = new SoCoordinate3;
    centerCoord->point.set1Value(0, SbVec3f(0.0f, 0.0f, 0.0f));
    centerSep->addChild(centerCoord);
    SoPointSet* centerPoint = new SoPointSet;
    centerSep->addChild(centerPoint);
    m_pickingAidSeparator->addChild(centerSep);
}

void PickingAidManager::showPickingAidLines(const SbVec3f& position) {
    if (!m_pickingAidSeparator) {
        createPickingAidLines();
    }

    m_pickingAidTransform->translation.setValue(position);

    SoSeparator* textSep = nullptr;
    int lastChildIndex = m_pickingAidSeparator->getNumChildren() - 1;
    if (lastChildIndex >= 0) {
        textSep = dynamic_cast<SoSeparator*>(m_pickingAidSeparator->getChild(lastChildIndex));
        if (!textSep || textSep->getNumChildren() == 0 || !dynamic_cast<SoText2*>(textSep->getChild(textSep->getNumChildren() - 1))) {
            textSep = new SoSeparator;
            SoMaterial* textMaterial = new SoMaterial;
            textMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
            textSep->addChild(textMaterial);
            SoTransform* textTransform = new SoTransform;
            textTransform->translation.setValue(0.1f, 0.1f, 0.1f);
            textSep->addChild(textTransform);
            SoText2* coordText = new SoText2;
            textSep->addChild(coordText);
            m_pickingAidSeparator->addChild(textSep);
        }
        SoText2* coordText = dynamic_cast<SoText2*>(textSep->getChild(textSep->getNumChildren() - 1));
        if (coordText) {
            char coordStr[64];
            snprintf(coordStr, sizeof(coordStr), "(%.2f, %.2f, %.2f)", position[0], position[1], position[2]);
            coordText->string.setValue(coordStr);
        }
    }

    if (!m_pickingAidVisible) {
        m_sceneManager->getObjectRoot()->addChild(m_pickingAidSeparator);
        m_pickingAidVisible = true;
    }

    m_canvas->Refresh(true);
}

void PickingAidManager::hidePickingAidLines() {
    if (!m_pickingAidSeparator || !m_pickingAidVisible) return;
    m_sceneManager->getObjectRoot()->removeChild(m_pickingAidSeparator);
    m_pickingAidVisible = false;
    m_canvas->Refresh(true);
}

void PickingAidManager::createReferenceGrid() {
    m_referenceGridSeparator = new SoSeparator;
    m_referenceGridSeparator->ref();
    
    // Create grid lines with more visible styling
    SoDrawStyle* gridStyle = DPIAwareRendering::createDPIAwareGeometryStyle(2.0f, false);  // Increased line width
    gridStyle->linePattern.setValue(0xFFFF); // Solid line pattern instead of dashed
    m_referenceGridSeparator->addChild(gridStyle);
    
    SoMaterial* gridMaterial = new SoMaterial;
    gridMaterial->diffuseColor.setValue(0.2f, 0.8f, 1.0f);  // Bright cyan color
    gridMaterial->transparency.setValue(0.3f);  // Less transparent for better visibility
    gridMaterial->emissiveColor.setValue(0.1f, 0.4f, 0.5f);  // Add slight glow
    m_referenceGridSeparator->addChild(gridMaterial);
    
    // Create grid coordinates with finer spacing for better detail
    SoCoordinate3* gridCoords = new SoCoordinate3;
    int coordIndex = 0;
    
    // Create major grid lines in X direction (every 5 units)
    for (float y = -10.0f; y <= 10.0f; y += 5.0f) {
        gridCoords->point.set1Value(coordIndex++, SbVec3f(-10.0f, y, 0.0f));
        gridCoords->point.set1Value(coordIndex++, SbVec3f(10.0f, y, 0.0f));
    }
    
    // Create major grid lines in Y direction (every 5 units)
    for (float x = -10.0f; x <= 10.0f; x += 5.0f) {
        gridCoords->point.set1Value(coordIndex++, SbVec3f(x, -10.0f, 0.0f));
        gridCoords->point.set1Value(coordIndex++, SbVec3f(x, 10.0f, 0.0f));
    }
    
    m_referenceGridSeparator->addChild(gridCoords);
    
    // Create line set for the major grid
    SoLineSet* gridLines = new SoLineSet;
    int majorLines = 10; // 5 lines in X + 5 lines in Y
    for (int i = 0; i < majorLines; i++) {
        gridLines->numVertices.set1Value(i, 2);
    }
    m_referenceGridSeparator->addChild(gridLines);
    
    // Add minor grid lines with different styling
    SoSeparator* minorGridSep = new SoSeparator;
    
    SoDrawStyle* minorGridStyle = DPIAwareRendering::createDPIAwareGeometryStyle(1.0f, false);
    minorGridStyle->linePattern.setValue(0xCCCC); // Dotted pattern for minor lines
    minorGridSep->addChild(minorGridStyle);
    
    SoMaterial* minorGridMaterial = new SoMaterial;
    minorGridMaterial->diffuseColor.setValue(0.1f, 0.6f, 0.8f);  // Slightly darker cyan
    minorGridMaterial->transparency.setValue(0.5f);  // More transparent for minor lines
    minorGridSep->addChild(minorGridMaterial);
    
    // Create minor grid coordinates (every 1 unit)
    SoCoordinate3* minorGridCoords = new SoCoordinate3;
    int minorCoordIndex = 0;
    
    // Minor lines in X direction
    for (float y = -10.0f; y <= 10.0f; y += 1.0f) {
        // Skip major grid positions
        if (static_cast<int>(y) % 5 != 0) {
            minorGridCoords->point.set1Value(minorCoordIndex++, SbVec3f(-10.0f, y, 0.0f));
            minorGridCoords->point.set1Value(minorCoordIndex++, SbVec3f(10.0f, y, 0.0f));
        }
    }
    
    // Minor lines in Y direction
    for (float x = -10.0f; x <= 10.0f; x += 1.0f) {
        // Skip major grid positions
        if (static_cast<int>(x) % 5 != 0) {
            minorGridCoords->point.set1Value(minorCoordIndex++, SbVec3f(x, -10.0f, 0.0f));
            minorGridCoords->point.set1Value(minorCoordIndex++, SbVec3f(x, 10.0f, 0.0f));
        }
    }
    
    minorGridSep->addChild(minorGridCoords);
    
    // Create line set for minor grid
    SoLineSet* minorGridLines = new SoLineSet;
    int minorLineCount = minorCoordIndex / 2;
    for (int i = 0; i < minorLineCount; i++) {
        minorGridLines->numVertices.set1Value(i, 2);
    }
    minorGridSep->addChild(minorGridLines);
    
    m_referenceGridSeparator->addChild(minorGridSep);
}

void PickingAidManager::showReferenceGrid(bool show) {
    if (!m_referenceGridSeparator) return;
    
    if (show && !m_referenceGridVisible) {
        // Calculate dynamic scale factor based on scene bounding box size
        float sceneSize = 0.0f;
        if (m_sceneManager) {
            sceneSize = m_sceneManager->getSceneBoundingBoxSize();
        }
        float halfExtent = sceneSize * 0.5f;
        float scaleFactor = (halfExtent > 0.0f) ? (halfExtent / 10.0f) : 1.0f;
        SoTransform* gridTransform = new SoTransform;
        gridTransform->scaleFactor.setValue(scaleFactor, scaleFactor, 1.0f);
        gridTransform->translation.setValue(0.0f, 0.0f, m_referenceZ);
        m_referenceGridSeparator->insertChild(gridTransform, 0);
        
        m_sceneManager->getObjectRoot()->addChild(m_referenceGridSeparator);
        m_referenceGridVisible = true;
        LOG_INF("Reference grid shown at Z=" + std::to_string(m_referenceZ));
    } else if (!show && m_referenceGridVisible) {
        m_sceneManager->getObjectRoot()->removeChild(m_referenceGridSeparator);
        m_referenceGridVisible = false;
        LOG_INF("Reference grid hidden");
    }
    
    m_canvas->Refresh(true);
}

void PickingAidManager::updatePickingAidColor(const SbVec3f& color) {
    if (!m_pickingAidSeparator) return;
    
    // Update material colors for all line separators
    for (int i = 0; i < m_pickingAidSeparator->getNumChildren(); i++) {
        SoSeparator* lineSep = dynamic_cast<SoSeparator*>(m_pickingAidSeparator->getChild(i));
        if (lineSep) {
            for (int j = 0; j < lineSep->getNumChildren(); j++) {
                SoMaterial* material = dynamic_cast<SoMaterial*>(lineSep->getChild(j));
                if (material) {
                    material->diffuseColor.setValue(color);
                }
            }
        }
    }
    
    m_canvas->Refresh(true);
}

void PickingAidManager::startPicking() {
    m_isPickingPosition = true;
    m_canvas->setPickingCursor(true);
    if (m_inputManager) {
        m_inputManager->enterPickingState();
    }
    LOG_INF("PickingAidManager: Started position picking mode.");
}

void PickingAidManager::stopPicking() {
    m_isPickingPosition = false;
    m_canvas->setPickingCursor(false);
    hidePickingAidLines();
    if (m_inputManager) {
        m_inputManager->enterDefaultState();
    }
    LOG_INF("PickingAidManager: Stopped position picking mode.");
}

bool PickingAidManager::isPicking() const {
    return m_isPickingPosition;
}