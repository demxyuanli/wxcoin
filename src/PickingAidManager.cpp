#include "PickingAidManager.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "DPIAwareRendering.h"
#include "Logger.h"
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoText2.h>

PickingAidManager::PickingAidManager(SceneManager* sceneManager, Canvas* canvas)
    : m_sceneManager(sceneManager)
    , m_canvas(canvas)
    , m_pickingAidSeparator(nullptr)
    , m_pickingAidTransform(nullptr)
    , m_pickingAidVisible(false)
{
    LOG_INF("PickingAidManager initializing");
    createPickingAidLines();
}

PickingAidManager::~PickingAidManager() {
    if (m_pickingAidSeparator) {
        m_pickingAidSeparator->unref();
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