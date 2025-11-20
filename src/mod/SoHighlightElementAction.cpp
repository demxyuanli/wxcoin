#include "mod/SoHighlightElementAction.h"

// Initialize class statics
SO_ACTION_SOURCE(SoHighlightElementAction);

// ===== SoHighlightElementAction Implementation =====

SoHighlightElementAction::SoHighlightElementAction()
    : m_highlighted(false)
    , m_highlightColor(1.0f, 1.0f, 0.0f) // Default yellow highlight
    , m_elementDetail(nullptr)
{
}

SoHighlightElementAction::~SoHighlightElementAction()
{
}

void SoHighlightElementAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoHighlightElementAction, SoAction);
}

void SoHighlightElementAction::beginTraversal(SoNode* node)
{
    // Default implementation - traverse children
    traverse(node);
}

void SoHighlightElementAction::apply(SoNode* node)
{
    if (node) {
        beginTraversal(node);
    }
}

void SoHighlightElementAction::apply(SoPath* path)
{
    if (path) {
        beginTraversal(path->getTail());
    }
}

void SoHighlightElementAction::apply(const SoPathList& pathList, SbBool obeysRules)
{
    for (int i = 0; i < pathList.getLength(); i++) {
        apply(pathList[i]);
    }
}

