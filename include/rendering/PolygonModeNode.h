#pragma once

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>

/**
 * @brief Custom Coin3D node for setting OpenGL polygon mode (FreeCAD-style)
 * 
 * Similar to FreeCAD's approach: uses glPolygonMode for fast wireframe rendering.
 * Used in HiddenLine mode to render triangle edges using glPolygonMode(GL_LINE)
 * instead of extracting and rendering mesh edges separately.
 * 
 * FreeCAD approach:
 * - Uses glPolygonMode(GL_LINE) for wireframe rendering
 * - Enables GL_POLYGON_OFFSET_LINE to prevent z-fighting
 * - Disables lighting for wireframe mode
 * - Uses polygon offset to push lines slightly forward
 * 
 * Performance: Much faster than SoIndexedLineSet for large meshes.
 */
class PolygonModeNode : public SoNode {
    SO_NODE_HEADER(PolygonModeNode);

public:
    enum PolygonMode {
        FILL = 0,
        LINE = 1,
        POINT = 2
    };

    PolygonModeNode();
    
    static void initClass();
    static void exitClass() {}  // Optional cleanup, not always needed

    SoSFEnum mode;              // Polygon mode: FILL, LINE, or POINT
    SoSFFloat lineWidth;         // Line width for LINE mode
    SoSFBool disableLighting;   // Disable lighting for LINE/POINT modes (FreeCAD style)
    SoSFFloat polygonOffsetFactor;  // Polygon offset factor (default: -1.0)
    SoSFFloat polygonOffsetUnits;   // Polygon offset units (default: -1.0)

protected:
    virtual ~PolygonModeNode();
    
    virtual void GLRender(SoGLRenderAction* action) override;
    virtual void doAction(SoAction* action) override;

private:
    // Save/restore OpenGL state
    void saveGLState(int* savedMode, float* savedLineWidth, unsigned char* savedLighting);
    void restoreGLState(int savedMode, float savedLineWidth, unsigned char savedLighting);
};

