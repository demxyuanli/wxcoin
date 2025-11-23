#pragma once

#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoSubAction.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/details/SoDetail.h>
#include <Inventor/SoPath.h>

/**
 * @brief Action for highlighting selected/preselected elements
 *
 * Similar to FreeCAD's SoHighlightElementAction, this action applies
 * highlight colors to elements specified by SoDetail objects.
 */
class SoHighlightElementAction : public SoAction {
    SO_ACTION_HEADER(SoHighlightElementAction);

public:
    SoHighlightElementAction();
    virtual ~SoHighlightElementAction();

    // Set whether to highlight or clear highlight
    void setHighlighted(bool highlight) { m_highlighted = highlight; }
    bool isHighlighted() const { return m_highlighted; }

    // Set highlight color
    void setColor(const SbColor& color) { m_highlightColor = color; }
    const SbColor& getColor() const { return m_highlightColor; }

    // Set the detail that specifies which element to highlight
    void setElement(const SoDetail* detail) { m_elementDetail = detail; }
    const SoDetail* getElement() const { return m_elementDetail; }

    // Override base class methods
    virtual void beginTraversal(SoNode* node);

    // Apply methods for different targets
    void apply(SoNode* node);
    void apply(SoPath* path);
    void apply(const SoPathList& pathList, SbBool obeysRules = FALSE);

    static void initClass();

private:
    static void nullAction(SoAction*, SoNode*) {}
    static void callDoAction(SoAction* action, SoNode* node);

    bool m_highlighted;
    SbColor m_highlightColor;
    const SoDetail* m_elementDetail;
};
