#include "mod/SoFCUnifiedSelection.h"
#include "mod/SoFCSelection.h"
#include "mod/SoFCSelectionContext.h"
#include "mod/SoHighlightElementAction.h"
#include "mod/SoSelectionElementAction.h"
#include "mod/EventConverter.h"
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/elements/SoCacheElement.h>
#include <wx/event.h>

SO_NODE_SOURCE(SoFCUnifiedSelection);

// Constructor
SoFCUnifiedSelection::SoFCUnifiedSelection()
    : m_preselectionPath(nullptr)
    , m_selectionPath(nullptr)
    , m_preselectionDetail(nullptr)
    , m_selectionDetail(nullptr)
{
    SO_NODE_CONSTRUCTOR(SoFCUnifiedSelection);

    // Initialize fields
    SO_NODE_ADD_FIELD(selectionMode, (SEL_AUTO));
    SO_NODE_ADD_FIELD(preselectionMode, (PRESEL_AUTO));
    SO_NODE_ADD_FIELD(selectionColor, (0.0f, 0.5f, 1.0f));  // Cyan
    SO_NODE_ADD_FIELD(highlightColor, (1.0f, 1.0f, 0.0f));  // Yellow
    SO_NODE_ADD_FIELD(useNewSelection, (TRUE));

    // Set field ranges
    selectionMode.setValue(SEL_AUTO);
    preselectionMode.setValue(PRESEL_AUTO);
}

SoFCUnifiedSelection::~SoFCUnifiedSelection()
{
    clearHighlighting(m_preselectionPath, m_preselectionDetail);
    clearHighlighting(m_selectionPath, m_selectionDetail);
}

void SoFCUnifiedSelection::initClass()
{
    SO_NODE_INIT_CLASS(SoFCUnifiedSelection, SoSeparator, "Separator");

    // Initialize action classes
    SoHighlightElementAction::initClass();
    SoSelectionElementAction::initClass();
    
    // Initialize SoFCSelection node class
    mod::SoFCSelection::initClass();
}

void SoFCUnifiedSelection::setPreselection(const std::string& elementName, float x, float y, float z)
{
    if (preselectionMode.getValue() == PRESEL_OFF) {
        return;
    }

    clearPreselection();
    m_currentPreselection = elementName;
    updatePreselectionHighlighting();
}

void SoFCUnifiedSelection::clearPreselection()
{
    if (!m_currentPreselection.empty()) {
        clearHighlighting(m_preselectionPath, m_preselectionDetail);
        m_currentPreselection.clear();
    }
}

void SoFCUnifiedSelection::setSelection(const std::string& elementName, float x, float y, float z)
{
    if (selectionMode.getValue() == SEL_OFF) {
        return;
    }

    clearSelection();
    m_currentSelection = elementName;
    updateSelectionHighlighting();
}

void SoFCUnifiedSelection::clearSelection()
{
    if (!m_currentSelection.empty()) {
        clearHighlighting(m_selectionPath, m_selectionDetail);
        m_currentSelection.clear();
    }
}

void SoFCUnifiedSelection::updatePreselectionHighlighting()
{
    if (m_currentPreselection.empty()) {
        return;
    }

    // This is a simplified implementation
    // In a full implementation, we would need to:
    // 1. Find the ViewProvider for the geometry
    // 2. Call getDetailPath() to get path and detail
    // 3. Apply SoHighlightElementAction

    // For now, just log the operation
    // TODO: Implement proper highlighting logic
}

void SoFCUnifiedSelection::updateSelectionHighlighting()
{
    if (m_currentSelection.empty()) {
        return;
    }

    // This is a simplified implementation
    // Similar to preselection, but using selection color and action

    // TODO: Implement proper selection logic
}

void SoFCUnifiedSelection::clearHighlighting(SoPath*& path, SoDetail*& detail)
{
    if (path) {
        // Apply clear highlight action
        SoHighlightElementAction highlightAction;
        highlightAction.setHighlighted(false);
        highlightAction.setElement(detail);
        highlightAction.apply(path);

        path->unref();
        path = nullptr;
    }

    if (detail) {
        delete detail;
        detail = nullptr;
    }
}

void SoFCUnifiedSelection::doAction(SoAction* action)
{
    // Handle different action types
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        // During rendering, ensure highlights are applied
        updatePreselectionHighlighting();
        updateSelectionHighlighting();
    }

    // Call parent method
    SoSeparator::doAction(action);
}

void SoFCUnifiedSelection::GLRender(SoGLRenderAction* action)
{
    // Custom GL rendering if needed
    SoSeparator::GLRender(action);
}

void SoFCUnifiedSelection::pick(SoPickAction* action)
{
    // Custom picking behavior if needed
    SoSeparator::pick(action);
}

void SoFCUnifiedSelection::handleEvent(SoHandleEventAction* action)
{
    const SoEvent* event = action->getEvent();
    if (!event) {
        SoSeparator::handleEvent(action);
        return;
    }

    // Handle mouse button events for selection
    if (event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent* mouseEvent = static_cast<const SoMouseButtonEvent*>(event);

        if (mouseEvent->getState() == SoButtonEvent::DOWN &&
            mouseEvent->getButton() == SoMouseButtonEvent::BUTTON1) {

            // Get picked point using priority system (Face > Line > Point)
            const SoPickedPoint* picked = getPickedPoint(action);

            if (picked) {
                const SoDetail* detail = picked->getDetail();

                // Extract element information from detail
                if (detail) {
                    if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
                        const SoFaceDetail* faceDetail = static_cast<const SoFaceDetail*>(detail);
                        int faceIndex = faceDetail->getFaceIndex();

                        SbVec3f point = picked->getPoint();
                        setSelection("Face" + std::to_string(faceIndex),
                                   point[0], point[1], point[2]);
                    } else if (detail->isOfType(SoLineDetail::getClassTypeId())) {
                        const SoLineDetail* lineDetail = static_cast<const SoLineDetail*>(detail);
                        int lineIndex = lineDetail->getLineIndex();

                        SbVec3f point = picked->getPoint();
                        setSelection("Edge" + std::to_string(lineIndex),
                                   point[0], point[1], point[2]);
                    } else if (detail->isOfType(SoPointDetail::getClassTypeId())) {
                        const SoPointDetail* pointDetail = static_cast<const SoPointDetail*>(detail);
                        int pointIndex = pointDetail->getCoordinateIndex();

                        SbVec3f point = picked->getPoint();
                        setSelection("Vertex" + std::to_string(pointIndex),
                                   point[0], point[1], point[2]);
                    }
                }
            }
        }
    }

    // Handle mouse motion for preselection
    if (event->isOfType(SoLocation2Event::getClassTypeId())) {
        const SoLocation2Event* locEvent = static_cast<const SoLocation2Event*>(event);

        // Get picked point for preselection using priority system
        const SoPickedPoint* picked = getPickedPoint(action);

        if (picked) {
            const SoDetail* detail = picked->getDetail();

            if (detail) {
                if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
                    const SoFaceDetail* faceDetail = static_cast<const SoFaceDetail*>(detail);
                    int faceIndex = faceDetail->getFaceIndex();

                    SbVec3f point = picked->getPoint();
                    setPreselection("Face" + std::to_string(faceIndex),
                                   point[0], point[1], point[2]);
                } else if (detail->isOfType(SoLineDetail::getClassTypeId())) {
                    const SoLineDetail* lineDetail = static_cast<const SoLineDetail*>(detail);
                    int lineIndex = lineDetail->getLineIndex();

                    SbVec3f point = picked->getPoint();
                    setPreselection("Edge" + std::to_string(lineIndex),
                                   point[0], point[1], point[2]);
                } else if (detail->isOfType(SoPointDetail::getClassTypeId())) {
                    const SoPointDetail* pointDetail = static_cast<const SoPointDetail*>(detail);
                    int pointIndex = pointDetail->getCoordinateIndex();

                    SbVec3f point = picked->getPoint();
                    setPreselection("Vertex" + std::to_string(pointIndex),
                                   point[0], point[1], point[2]);
                }
            } else {
                clearPreselection();
            }
        } else {
            clearPreselection();
        }
    }

    SoSeparator::handleEvent(action);
}

const SoPickedPoint* SoFCUnifiedSelection::getPickedPoint(SoHandleEventAction* action) const
{
    const SoPickedPointList& points = action->getPickedPointList();
    if (points.getLength() == 0) {
        return nullptr;
    }

    if (points.getLength() == 1) {
        return points[0];
    }

    // Use priority system: Face > Line > Point
    const SoPickedPoint* picked = points[0];
    int pickedPriority = getPriority(picked);
    const SbVec3f& pickedPt = picked->getPoint();

    for (int i = 1; i < points.getLength(); i++) {
        const SoPickedPoint* cur = points[i];
        int curPriority = getPriority(cur);
        const SbVec3f& curPt = cur->getPoint();

        if ((curPriority > pickedPriority) &&
            pickedPt.equals(curPt, 0.01f)) {
            picked = cur;
            pickedPriority = curPriority;
        }
    }

    return picked;
}

int SoFCUnifiedSelection::getPriority(const SoPickedPoint* p)
{
    const SoDetail* detail = p->getDetail();
    if (!detail) {
        return 0;
    }

    if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
        return 1; // Face has highest priority
    }

    if (detail->isOfType(SoLineDetail::getClassTypeId())) {
        return 2; // Line has medium priority
    }

    if (detail->isOfType(SoPointDetail::getClassTypeId())) {
        return 3; // Point has lowest priority
    }

    return 0;
}