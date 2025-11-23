#include "mod/SoFCSelection.h"
#include "mod/SoFCSelectionContext.h"
#include "mod/SoHighlightElementAction.h"
#include "mod/SoSelectionElementAction.h"
#include <Inventor/SoFullPath.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/misc/SoState.h>

SO_NODE_SOURCE(mod::SoFCSelection);

SoFullPath* mod::SoFCSelection::currenthighlight = nullptr;

mod::SoFCSelection::SoFCSelection()
{
    SO_NODE_CONSTRUCTOR(SoFCSelection);

    SO_NODE_ADD_FIELD(colorHighlight, (SbColor(1.0f, 1.0f, 0.0f))); // Yellow
    SO_NODE_ADD_FIELD(colorSelection, (SbColor(0.0f, 0.5f, 1.0f))); // Cyan
    SO_NODE_ADD_FIELD(selected, (NOTSELECTED));
    SO_NODE_ADD_FIELD(preselectionMode, (AUTO));
    SO_NODE_ADD_FIELD(selectionMode, (SEL_ON));
    SO_NODE_ADD_FIELD(documentName, (""));
    SO_NODE_ADD_FIELD(objectName, (""));
    SO_NODE_ADD_FIELD(subElementName, (""));
    SO_NODE_ADD_FIELD(useNewSelection, (true));

    SO_NODE_DEFINE_ENUM_VALUE(PreselectionModes, AUTO);
    SO_NODE_DEFINE_ENUM_VALUE(PreselectionModes, ON);
    SO_NODE_DEFINE_ENUM_VALUE(PreselectionModes, OFF);
    SO_NODE_SET_SF_ENUM_TYPE(preselectionMode, PreselectionModes);

    SO_NODE_DEFINE_ENUM_VALUE(SelectionModes, SEL_ON);
    SO_NODE_DEFINE_ENUM_VALUE(SelectionModes, SEL_OFF);
    SO_NODE_SET_SF_ENUM_TYPE(selectionMode, SelectionModes);

    SO_NODE_DEFINE_ENUM_VALUE(Selected, NOTSELECTED);
    SO_NODE_DEFINE_ENUM_VALUE(Selected, SELECTED);
    SO_NODE_SET_SF_ENUM_TYPE(selected, Selected);

    highlighted = false;
    selContext = std::make_shared<SoFCSelectionContext>();
    selContext2 = std::make_shared<SoFCSelectionContext>();
}

mod::SoFCSelection::~SoFCSelection()
{
    if (currenthighlight && currenthighlight->getTail() == this) {
        currenthighlight->unref();
        currenthighlight = nullptr;
    }
}

void mod::SoFCSelection::initClass()
{
    SO_NODE_INIT_CLASS(SoFCSelection, SoGroup, "Group");
}

void mod::SoFCSelection::finish()
{
}

void mod::SoFCSelection::doAction(SoAction* action)
{
    if (useNewSelection.getValue() && action->getCurPathCode() != SoAction::OFF_PATH) {
        if (action->getTypeId() == ::SoHighlightElementAction::getClassTypeId()) {
            auto hlaction = static_cast<::SoHighlightElementAction*>(action);
            auto ctx = getActionContext(action, !hlaction->isHighlighted());
            if (ctx) {
                if (!hlaction->isHighlighted()) {
                    if (ctx->isHighlighted()) {
                        ctx->removeHighlight();
                        highlighted = false;
                        touch();
                    }
                } else {
                    ctx->highlightColor = hlaction->getColor();
                    if (!ctx->isHighlighted()) {
                        ctx->highlightIndex = 0;
                        highlighted = true;
                        touch();
                    }
                }
            }
            return;
        } else if (action->getTypeId() == ::SoSelectionElementAction::getClassTypeId()) {
            auto selaction = static_cast<::SoSelectionElementAction*>(action);
            auto ctx = getActionContext(action, 
                selaction->getType() != ::SoSelectionElementAction::None &&
                selaction->getType() != ::SoSelectionElementAction::Remove);
            
            if (ctx) {
                if (selaction->getType() == ::SoSelectionElementAction::All ||
                    selaction->getType() == ::SoSelectionElementAction::Append) {
                    ctx->selectionColor = selaction->getColor();
                    if (!ctx->isSelectAll()) {
                        ctx->selectAll();
                        selected.setValue(SELECTED);
                        touch();
                    }
                } else if (selaction->getType() == ::SoSelectionElementAction::None ||
                           selaction->getType() == ::SoSelectionElementAction::Remove) {
                    if (ctx->isSelected()) {
                        ctx->selectionIndex.clear();
                        selected.setValue(NOTSELECTED);
                        touch();
                    }
                }
            }
            return;
        }
    }

    SoGroup::doAction(action);
}

void mod::SoFCSelection::GLRender(SoGLRenderAction* action)
{
    SoGroup::GLRender(action);
}

void mod::SoFCSelection::GLRenderBelowPath(SoGLRenderAction* action)
{
    SoState* state = action->getState();
    auto ctx = getRenderContext();
    
    bool sel = ctx && ctx->isSelected();
    bool hl = ctx && ctx->isHighlighted();
    
    if (hl || sel || highlighted || selected.getValue() == SELECTED) {
        state->push();
        
        SbColor color;
        if (hl && ctx) {
            color = ctx->highlightColor;
        } else if (sel && ctx) {
            color = ctx->selectionColor;
        } else {
            color = highlighted ? colorHighlight.getValue() : colorSelection.getValue();
        }
        
        SoLazyElement::setEmissive(state, &color);
        SoOverrideElement::setEmissiveColorOverride(state, this, true);
        
        if (SoLazyElement::getLightModel(state) == SoLazyElement::BASE_COLOR) {
            SoLazyElement::setDiffuse(state, this, 1, &color, nullptr);
            SoOverrideElement::setDiffuseColorOverride(state, this, true);
            SoMaterialBindingElement::set(state, this, SoMaterialBindingElement::OVERALL);
            SoOverrideElement::setMaterialBindingOverride(state, this, true);
        }
        
        SoGroup::GLRenderBelowPath(action);
        
        state->pop();
    } else {
        SoGroup::GLRenderBelowPath(action);
    }
}

void mod::SoFCSelection::GLRenderInPath(SoGLRenderAction* action)
{
    if (action->getCurPathCode() == SoAction::BELOW_PATH) {
        return GLRenderBelowPath(action);
    }
    
    SoState* state = action->getState();
    auto ctx = getRenderContext();
    
    bool sel = ctx && ctx->isSelected();
    bool hl = ctx && ctx->isHighlighted();
    
    if (hl || sel || highlighted || selected.getValue() == SELECTED) {
        state->push();
        
        SbColor color;
        if (hl && ctx) {
            color = ctx->highlightColor;
        } else if (sel && ctx) {
            color = ctx->selectionColor;
        } else {
            color = highlighted ? colorHighlight.getValue() : colorSelection.getValue();
        }
        
        SoLazyElement::setEmissive(state, &color);
        SoOverrideElement::setEmissiveColorOverride(state, this, true);
        
        if (SoLazyElement::getLightModel(state) == SoLazyElement::BASE_COLOR) {
            SoLazyElement::setDiffuse(state, this, 1, &color, nullptr);
            SoOverrideElement::setDiffuseColorOverride(state, this, true);
            SoMaterialBindingElement::set(state, this, SoMaterialBindingElement::OVERALL);
            SoOverrideElement::setMaterialBindingOverride(state, this, true);
        }
        
        SoGroup::GLRenderInPath(action);
        
        state->pop();
    } else {
        SoGroup::GLRenderInPath(action);
    }
}

void mod::SoFCSelection::handleEvent(SoHandleEventAction* action)
{
    SoGroup::handleEvent(action);
}

void mod::SoFCSelection::redrawHighlighted(SoAction* act, bool flag)
{
    if (flag != highlighted) {
        highlighted = flag;
        // Redraw will happen automatically on next render
    }
}

int mod::SoFCSelection::getPriority(const SoPickedPoint* p)
{
    const SoDetail* detail = p->getDetail();
    if (!detail)
        return 0;
    if (detail->isOfType(SoFaceDetail::getClassTypeId()))
        return 1;
    if (detail->isOfType(SoLineDetail::getClassTypeId()))
        return 2;
    if (detail->isOfType(SoPointDetail::getClassTypeId()))
        return 3;
    return 0;
}

const SoPickedPoint* mod::SoFCSelection::getPickedPoint(SoHandleEventAction* action) const
{
    const SoPickedPointList& points = action->getPickedPointList();
    if (points.getLength() == 0)
        return nullptr;
    else if (points.getLength() == 1)
        return points[0];

    const SoPickedPoint* picked = points[0];
    int picked_prio = getPriority(picked);
    const SbVec3f& picked_pt = picked->getPoint();

    for (int i = 1; i < points.getLength(); i++) {
        const SoPickedPoint* cur = points[i];
        int cur_prio = getPriority(cur);
        const SbVec3f& cur_pt = cur->getPoint();

        if ((cur_prio > picked_prio) && picked_pt.equals(cur_pt, 0.01f)) {
            picked = cur;
            picked_prio = cur_prio;
        }
    }
    return picked;
}


