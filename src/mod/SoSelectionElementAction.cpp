#include "mod/SoSelectionElementAction.h"

// Initialize class statics
SO_ACTION_SOURCE(SoSelectionElementAction);

// ===== SoSelectionElementAction Implementation =====

SoSelectionElementAction::SoSelectionElementAction(Type type)
    : m_actionType(type)
    , m_selectionColor(0.0f, 0.5f, 1.0f) // Default cyan selection
    , m_elementDetail(nullptr)
    , m_secondary(false)
{
}

SoSelectionElementAction::~SoSelectionElementAction()
{
}

void SoSelectionElementAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoSelectionElementAction, SoAction);
}

void SoSelectionElementAction::beginTraversal(SoNode* node)
{
    // Default implementation - traverse children
    traverse(node);
}

void SoSelectionElementAction::apply(SoNode* node)
{
    if (node) {
        beginTraversal(node);
    }
}

void SoSelectionElementAction::apply(SoPath* path)
{
    if (path) {
        beginTraversal(path->getTail());
    }
}

void SoSelectionElementAction::apply(const SoPathList& pathList, SbBool obeysRules)
{
    for (int i = 0; i < pathList.getLength(); i++) {
        apply(pathList[i]);
    }
}



