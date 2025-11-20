#pragma once

#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoSubAction.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/details/SoDetail.h>
#include <Inventor/SoPath.h>

/**
 * @brief Action for applying selection highlighting
 *
 * Similar to FreeCAD's SoSelectionElementAction, this action applies
 * selection colors to elements specified by SoDetail objects.
 */
class SoSelectionElementAction : public SoAction {
    SO_ACTION_HEADER(SoSelectionElementAction);

public:
    enum Type {
        None,     // Clear selection
        Append,   // Add to selection
        Remove,   // Remove from selection
        All       // Select all (whole object)
    };

    SoSelectionElementAction(Type type = None);
    virtual ~SoSelectionElementAction();

    // Set action type
    void setType(Type type) { m_actionType = type; }
    Type getType() const { return m_actionType; }

    // Set selection color
    void setColor(const SbColor& color) { m_selectionColor = color; }
    const SbColor& getColor() const { return m_selectionColor; }

    // Set the detail that specifies which element to select
    void setElement(const SoDetail* detail) { m_elementDetail = detail; }
    const SoDetail* getElement() const { return m_elementDetail; }

    // For secondary actions (used in partial rendering)
    void setSecondary(bool secondary) { m_secondary = secondary; }
    bool isSecondary() const { return m_secondary; }

    // Override base class methods
    virtual void beginTraversal(SoNode* node);

    // Apply methods for different targets
    void apply(SoNode* node);
    void apply(SoPath* path);
    void apply(const SoPathList& pathList, SbBool obeysRules = FALSE);

    static void initClass();

private:
    Type m_actionType;
    SbColor m_selectionColor;
    const SoDetail* m_elementDetail;
    bool m_secondary;
};
