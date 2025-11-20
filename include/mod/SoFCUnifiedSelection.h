#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFString.h>
#include <memory>
#include <string>

/**
 * @brief Unified selection node for handling preselection and selection
 *
 * Similar to FreeCAD's SoFCUnifiedSelection, this node manages the selection
 * state and applies highlighting to selected/preselected elements.
 */
class SoFCUnifiedSelection : public SoSeparator {
    SO_NODE_HEADER(SoFCUnifiedSelection);

public:
    // Selection modes (similar to FreeCAD)
    enum SelectionMode {
        SEL_OFF = 0,    // Selection disabled
        SEL_ON = 1,     // Selection enabled
        SEL_AUTO = 2    // Automatic selection mode
    };

    enum PreselectionMode {
        PRESEL_OFF = 0,    // Preselection disabled
        PRESEL_ON = 1,     // Preselection enabled
        PRESEL_AUTO = 2    // Automatic preselection mode
    };

    // Fields
    SoSFEnum selectionMode;
    SoSFEnum preselectionMode;
    SoSFColor selectionColor;
    SoSFColor highlightColor;
    SoSFBool useNewSelection;

    // Constructor
    SoFCUnifiedSelection();

    // Static methods
    static void initClass();

    // Apply preselection
    void setPreselection(const std::string& elementName, float x = 0.0f, float y = 0.0f, float z = 0.0f);

    // Clear preselection
    void clearPreselection();

    // Apply selection
    void setSelection(const std::string& elementName, float x = 0.0f, float y = 0.0f, float z = 0.0f);

    // Clear selection
    void clearSelection();

    // Get current selection/preselection info
    const std::string& getCurrentPreselection() const { return m_currentPreselection; }
    const std::string& getCurrentSelection() const { return m_currentSelection; }

protected:
    virtual ~SoFCUnifiedSelection();

    // Override SoSeparator methods
    virtual void doAction(SoAction* action);
    virtual void GLRender(SoGLRenderAction* action);
    virtual void pick(SoPickAction* action);

private:
    // Internal state
    std::string m_currentPreselection;
    std::string m_currentSelection;

    // Cached paths for current selection/preselection
    SoPath* m_preselectionPath;
    SoPath* m_selectionPath;
    SoDetail* m_preselectionDetail;
    SoDetail* m_selectionDetail;

    // Helper methods
    void updatePreselectionHighlighting();
    void updateSelectionHighlighting();
    void clearHighlighting(SoPath*& path, SoDetail*& detail);
};