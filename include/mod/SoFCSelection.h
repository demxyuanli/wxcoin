#pragma once

#include <Inventor/nodes/SoGroup.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include "mod/SoFCSelectionContext.h"
#include <memory>

class SoFullPath;
class SoPickedPoint;
class SoDetail;

namespace mod {

// Forward declarations - full definitions needed in .cpp
class SoHighlightElementAction;
class SoSelectionElementAction;

/**
 * @brief Selection node for managing highlight and selection state
 *
 * Similar to FreeCAD's SoFCSelection, this node responds to
 * SoHighlightElementAction and SoSelectionElementAction to manage
 * visual highlighting and selection of geometry elements.
 */
class SoFCSelection : public SoGroup {
    SO_NODE_HEADER(SoFCSelection);

public:
    static void initClass();
    static void finish();

    SoFCSelection();
    virtual ~SoFCSelection();

    enum PreselectionModes {
        AUTO, ON, OFF
    };

    enum SelectionModes {
        SEL_ON, SEL_OFF
    };

    enum Selected {
        NOTSELECTED, SELECTED
    };

    SoSFColor colorHighlight;
    SoSFColor colorSelection;
    SoSFEnum selected;
    SoSFEnum preselectionMode;
    SoSFEnum selectionMode;
    SoSFString documentName;
    SoSFString objectName;
    SoSFString subElementName;
    SoSFBool useNewSelection;

    void doAction(SoAction* action) override;
    void GLRender(SoGLRenderAction* action) override;
    void handleEvent(SoHandleEventAction* action) override;
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void GLRenderInPath(SoGLRenderAction* action) override;

    bool isHighlighted() const { return highlighted; }

protected:
    virtual void redrawHighlighted(SoAction* act, bool flag);

private:
    static int getPriority(const SoPickedPoint* p);
    const SoPickedPoint* getPickedPoint(SoHandleEventAction* action) const;
    
    SoFCSelectionContextPtr getActionContext(SoAction* action, bool create = true);
    SoFCSelectionContextPtr getRenderContext();

    bool highlighted;
    SoFCSelectionContextPtr selContext;
    SoFCSelectionContextPtr selContext2;
    static SoFullPath* currenthighlight;
};

} // namespace mod

