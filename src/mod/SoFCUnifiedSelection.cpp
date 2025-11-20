#include "mod/SoFCUnifiedSelection.h"
#include "mod/SoHighlightElementAction.h"
#include "mod/SoSelectionElementAction.h"
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/elements/SoCacheElement.h>

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