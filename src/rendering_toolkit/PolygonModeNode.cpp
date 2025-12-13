#include "rendering/PolygonModeNode.h"
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/elements/SoGLPolygonOffsetElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include "logger/Logger.h"

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

SO_NODE_SOURCE(PolygonModeNode);

void PolygonModeNode::initClass()
{
    SO_NODE_INIT_CLASS(PolygonModeNode, SoNode, "Node");
}

PolygonModeNode::PolygonModeNode()
{
    SO_NODE_CONSTRUCTOR(PolygonModeNode);
    
    SO_NODE_ADD_FIELD(mode, (FILL));
    SO_NODE_ADD_FIELD(lineWidth, (1.0f));
    SO_NODE_ADD_FIELD(disableLighting, (TRUE));  // Default: disable lighting for LINE/POINT (FreeCAD style)
    SO_NODE_ADD_FIELD(polygonOffsetFactor, (-1.0f));  // Default: push lines forward
    SO_NODE_ADD_FIELD(polygonOffsetUnits, (-1.0f));    // Default: push lines forward
    
    SO_NODE_DEFINE_ENUM_VALUE(PolygonMode, FILL);
    SO_NODE_DEFINE_ENUM_VALUE(PolygonMode, LINE);
    SO_NODE_DEFINE_ENUM_VALUE(PolygonMode, POINT);
    SO_NODE_SET_SF_ENUM_TYPE(mode, PolygonMode);
    
    isBuiltIn = TRUE;
}

PolygonModeNode::~PolygonModeNode()
{
}

void PolygonModeNode::saveGLState(int* savedMode, float* savedLineWidth, unsigned char* savedLighting)
{
    if (savedMode) {
        GLint mode[2];
        glGetIntegerv(GL_POLYGON_MODE, mode);
        savedMode[0] = static_cast<int>(mode[0]);
        savedMode[1] = static_cast<int>(mode[1]);
    }
    if (savedLineWidth) {
        GLfloat width;
        glGetFloatv(GL_LINE_WIDTH, &width);
        *savedLineWidth = width;
    }
    if (savedLighting) {
        *savedLighting = glIsEnabled(GL_LIGHTING) ? 1 : 0;
    }
}

void PolygonModeNode::restoreGLState(int savedMode, float savedLineWidth, unsigned char savedLighting)
{
    glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(savedMode));
    glLineWidth(savedLineWidth);
    if (savedLighting) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
}

void PolygonModeNode::GLRender(SoGLRenderAction* action)
{
    if (!action) return;
    
    // Save current OpenGL state (FreeCAD approach)
    int savedMode[2];
    float savedLineWidth;
    unsigned char savedLighting;
    saveGLState(savedMode, &savedLineWidth, &savedLighting);
    
    // Apply polygon mode based on field value
    GLenum glMode = GL_FILL;
    bool needsPolygonOffset = false;
    
    switch (mode.getValue()) {
        case FILL:
            glMode = GL_FILL;
            break;
            
        case LINE:
            glMode = GL_LINE;
            // Set line width (FreeCAD style)
            glLineWidth(lineWidth.getValue());
            // Enable polygon offset to prevent z-fighting (FreeCAD approach)
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(polygonOffsetFactor.getValue(), polygonOffsetUnits.getValue());
            needsPolygonOffset = true;
            // Disable lighting for wireframe (FreeCAD style)
            if (disableLighting.getValue()) {
                glDisable(GL_LIGHTING);
            }
            break;
            
        case POINT:
            glMode = GL_POINT;
            // Disable lighting for points (FreeCAD style)
            if (disableLighting.getValue()) {
                glDisable(GL_LIGHTING);
            }
            break;
    }
    
    // Apply polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, glMode);
    
    // Render children
    SoNode::GLRender(action);
    
    // Restore OpenGL state (FreeCAD approach: always restore)
    if (needsPolygonOffset) {
        glDisable(GL_POLYGON_OFFSET_LINE);
    }
    restoreGLState(savedMode[0], savedLineWidth, savedLighting);
}

void PolygonModeNode::doAction(SoAction* action)
{
    // For non-GL actions, just traverse children
    SoNode::doAction(action);
}

