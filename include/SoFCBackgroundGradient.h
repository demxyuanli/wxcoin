/***************************************************************************
 *   Adapted from FreeCAD SoFCBackgroundGradient                           *
 ***************************************************************************/

#ifndef SOFCBACKGROUNDGRADIENT_H
#define SOFCBACKGROUNDGRADIENT_H

#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

class SbColor;
class SoGLRenderAction;

class SoFCBackgroundGradient : public SoNode {
    using inherited = SoNode;

    SO_NODE_HEADER(SoFCBackgroundGradient);

public:
    enum Gradient {
        LINEAR = 0,
        RADIAL = 1
    };
    static void initClass();
    static void finish();
    SoFCBackgroundGradient();

    void GLRender(SoGLRenderAction *action) override;
    void setGradient(Gradient grad);
    Gradient getGradient() const;
    void setColorGradient(const SbColor& fromColor, const SbColor& toColor);
    void setColorGradient(const SbColor& fromColor, const SbColor& toColor, const SbColor& midColor);

private:
    Gradient gradient;

protected:
    ~SoFCBackgroundGradient() override;

    SbColor fCol, tCol, mCol;
};

#endif // SOFCBACKGROUNDGRADIENT_H



