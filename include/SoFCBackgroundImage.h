/***************************************************************************
 *   Background Image Node for Coin3D (similar to SoFCBackgroundGradient)  *
 ***************************************************************************/

#ifndef SOFCBACKGROUNDIMAGE_H
#define SOFCBACKGROUNDIMAGE_H

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <string>

class SoGLRenderAction;

class SoFCBackgroundImage : public SoNode {
    using inherited = SoNode;

    SO_NODE_HEADER(SoFCBackgroundImage);

public:
    static void initClass();
    static void finish();
    SoFCBackgroundImage();

    void GLRender(SoGLRenderAction *action) override;
    void setImagePath(const std::string& path);
    std::string getImagePath() const { return m_imagePath; }
    void setOpacity(float opacity);
    float getOpacity() const { return m_opacity; }
    void setFitMode(int fit); // 0=Fill, 1=Fit, 2=Stretch
    int getFitMode() const { return m_fitMode; }
    void setMaintainAspect(bool maintainAspect);
    bool getMaintainAspect() const { return m_maintainAspect; }

protected:
    ~SoFCBackgroundImage() override;

private:
    std::string m_imagePath;
    float m_opacity;
    int m_fitMode; // 0=Fill (tile), 1=Fit, 2=Stretch
    bool m_maintainAspect;
    unsigned int m_textureId;
    bool m_textureLoaded;
    int m_textureWidth;
    int m_textureHeight;

    bool loadTexture(const std::string& imagePath, unsigned int& textureId, int& width, int& height);
};

#endif // SOFCBACKGROUNDIMAGE_H

