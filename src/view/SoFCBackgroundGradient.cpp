/***************************************************************************
 *   Adapted from FreeCAD SoFCBackgroundGradient                           *
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 ***************************************************************************/

#include "SoFCBackgroundGradient.h"
#include <cmath>
#include <array>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>

// Pre-calculated circle vertices for radial gradient
// Use constexpr to match FreeCAD implementation
static const std::array<GLfloat[2], 32> big_circle = []{
    constexpr float pi = 3.14159265358979323846f;
    constexpr float sqrt2 = 1.4142135623730950488f;
    std::array<GLfloat[2], 32> result; int c = 0;
    for (GLfloat i = 0; i < 2 * pi; i += 2 * pi / 32, c++) {
        result[c][0] = sqrt2 * cosf(i);
        result[c][1] = sqrt2 * sinf(i);
    }
    return result;
}();

static const std::array<GLfloat[2], 32> small_oval = []{
    constexpr float pi = 3.14159265358979323846f;
    constexpr float sqrt2 = 1.4142135623730950488f;
    const float sqrt1_2 = std::sqrt(0.5f);
    std::array<GLfloat[2], 32> result; int c = 0;
    for (GLfloat i = 0; i < 2 * pi; i += 2 * pi / 32, c++) {
        result[c][0] = 0.3f * sqrt2 * cosf(i);
        result[c][1] = sqrt1_2 * sinf(i);
    }
    return result;
}();

SO_NODE_SOURCE(SoFCBackgroundGradient)

void SoFCBackgroundGradient::finish()
{
    atexit_cleanup();
}

SoFCBackgroundGradient::SoFCBackgroundGradient()
{
    SO_NODE_CONSTRUCTOR(SoFCBackgroundGradient);
    fCol.setValue(0.5f, 0.5f, 0.8f);  // Default from color
    tCol.setValue(0.7f, 0.7f, 0.9f);  // Default to color
    mCol.setValue(1.0f, 1.0f, 1.0f);  // Default mid color
    gradient = Gradient::LINEAR;
}

SoFCBackgroundGradient::~SoFCBackgroundGradient() = default;

void SoFCBackgroundGradient::initClass()
{
    SO_NODE_INIT_CLASS(SoFCBackgroundGradient, SoNode, "Node");
}

void SoFCBackgroundGradient::GLRender(SoGLRenderAction * /*action*/)
{
    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Save and set OpenGL states
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    if (gradient == Gradient::LINEAR) {
        glBegin(GL_TRIANGLE_STRIP);
        if (mCol[0] < 0) {
            // Two-color gradient
            glColor3f(fCol[0], fCol[1], fCol[2]); glVertex2f(-1, 1);
            glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2f(-1, -1);
            glColor3f(fCol[0], fCol[1], fCol[2]); glVertex2f(1, 1);
            glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2f(1, -1);
        } else {
            // Three-color gradient
            glColor3f(fCol[0], fCol[1], fCol[2]); glVertex2f(-1, 1);
            glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2f(-1, 0);
            glColor3f(fCol[0], fCol[1], fCol[2]); glVertex2f(1, 1);
            glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2f(1, 0);
            glEnd();
            glBegin(GL_TRIANGLE_STRIP);
            glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2f(-1, 0);
            glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2f(-1, -1);
            glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2f(1, 0);
            glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2f(1, -1);
        }
        glEnd();
    } else {
        // Radial gradient
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(fCol[0], fCol[1], fCol[2]); glVertex2f(0.0f, 0.0f);

        if (mCol[0] < 0) {
            // Two-color radial gradient
            glColor3f(tCol[0], tCol[1], tCol[2]);
            for (const GLfloat *vertex : big_circle)
                glVertex2fv(vertex);
            glVertex2fv(big_circle.front());
        } else {
            // Three-color radial gradient
            glColor3f(mCol[0], mCol[1], mCol[2]);
            for (const GLfloat *vertex : small_oval)
                glVertex2fv(vertex);
            glVertex2fv(small_oval.front());
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            for (std::size_t i = 0; i < small_oval.size(); i++) {
                glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2fv(small_oval[i]);
                glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2fv(big_circle[i]);
            }
            glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2fv(small_oval.front());
            glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2fv(big_circle.front());
        }
        glEnd();
    }

    // Restore OpenGL states
    glPopAttrib();
    glPopMatrix(); // Restore modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void SoFCBackgroundGradient::setGradient(SoFCBackgroundGradient::Gradient grad)
{
    gradient = grad;
}

SoFCBackgroundGradient::Gradient SoFCBackgroundGradient::getGradient() const
{
    return gradient;
}

void SoFCBackgroundGradient::setColorGradient(const SbColor& fromColor, const SbColor& toColor)
{
    fCol = fromColor;
    tCol = toColor;
    mCol[0] = -1.0f; // Mark mid color as unused
}

void SoFCBackgroundGradient::setColorGradient(const SbColor& fromColor, const SbColor& toColor, const SbColor& midColor)
{
    fCol = fromColor;
    tCol = toColor;
    mCol = midColor;
}

