#include "mod/SoHighlightElementAction.h"
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoCoordinateElement.h>

// Initialize class statics
SO_ACTION_SOURCE(SoHighlightElementAction);

// ===== SoHighlightElementAction Implementation =====

SoHighlightElementAction::SoHighlightElementAction()
    : m_highlighted(false)
    , m_highlightColor(1.0f, 1.0f, 0.0f) // Default yellow highlight
    , m_elementDetail(nullptr)
{
    SO_ACTION_CONSTRUCTOR(SoHighlightElementAction);
}

SoHighlightElementAction::~SoHighlightElementAction()
{
}

void SoHighlightElementAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoHighlightElementAction, SoAction);

    SO_ENABLE(SoHighlightElementAction, SoSwitchElement);
    SO_ENABLE(SoHighlightElementAction, SoModelMatrixElement);
    SO_ENABLE(SoHighlightElementAction, SoCoordinateElement);

    SO_ACTION_ADD_METHOD(SoNode, nullAction);

    SO_ACTION_ADD_METHOD(SoGroup, callDoAction);
    SO_ACTION_ADD_METHOD(SoSeparator, callDoAction);
    SO_ACTION_ADD_METHOD(SoSwitch, callDoAction);
    SO_ACTION_ADD_METHOD(SoCoordinate3, callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedLineSet, callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedFaceSet, callDoAction);
    SO_ACTION_ADD_METHOD(SoPointSet, callDoAction);
}

void SoHighlightElementAction::beginTraversal(SoNode* node)
{
    traverse(node);
}

void SoHighlightElementAction::callDoAction(SoAction* action, SoNode* node)
{
    node->doAction(action);
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




