#include "mod/SoSelectionElementAction.h"
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
SO_ACTION_SOURCE(SoSelectionElementAction);

// ===== SoSelectionElementAction Implementation =====

SoSelectionElementAction::SoSelectionElementAction(Type type)
    : m_actionType(type)
    , m_selectionColor(0.0f, 0.5f, 1.0f) // Default cyan selection
    , m_elementDetail(nullptr)
    , m_secondary(false)
{
    SO_ACTION_CONSTRUCTOR(SoSelectionElementAction);
}

SoSelectionElementAction::~SoSelectionElementAction()
{
}

void SoSelectionElementAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoSelectionElementAction, SoAction);

    SO_ENABLE(SoSelectionElementAction, SoSwitchElement);
    SO_ENABLE(SoSelectionElementAction, SoModelMatrixElement);
    SO_ENABLE(SoSelectionElementAction, SoCoordinateElement);

    SO_ACTION_ADD_METHOD(SoNode, nullAction);

    SO_ACTION_ADD_METHOD(SoGroup, callDoAction);
    SO_ACTION_ADD_METHOD(SoSeparator, callDoAction);
    SO_ACTION_ADD_METHOD(SoSwitch, callDoAction);
    SO_ACTION_ADD_METHOD(SoCoordinate3, callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedLineSet, callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedFaceSet, callDoAction);
    SO_ACTION_ADD_METHOD(SoPointSet, callDoAction);
}

void SoSelectionElementAction::beginTraversal(SoNode* node)
{
    traverse(node);
}

void SoSelectionElementAction::callDoAction(SoAction* action, SoNode* node)
{
    node->doAction(action);
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




