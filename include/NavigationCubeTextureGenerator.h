#pragma once

#include <string>
#include <vector>
#include <map>
#include "NavigationCubeTypes.h" // For PickId enum

class SoTexture2;
class wxColour;
class wxBitmap;

class NavigationCubeTextureGenerator {
public:
    NavigationCubeTextureGenerator();
    ~NavigationCubeTextureGenerator();

    // Initialize font sizes for faces (called after geometry setup)
    void initializeFontSizes();

    // Create cube face textures (6 main face textures)
    void createCubeFaceTextures();

    // Generate texture for a specific face
    bool generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height,
                           const wxColour& bgColor, float faceSize, PickId pickId);

    // Create SoTexture2 object for a face
    SoTexture2* createTextureForFace(const std::string& faceName, bool isHover);

    // Calculate vertical balance for text centering (FreeCAD-style)
    int calculateVerticalBalance(const wxBitmap& bitmap, int fontSizeHint) const;

    // Generate and cache all textures
    void generateAndCacheTextures();

    // Get cached textures
    SoTexture2* getNormalTexture(const std::string& faceName) const;
    SoTexture2* getHoverTexture(const std::string& faceName) const;

    // Font size management
    void setFaceFontSize(PickId pickId, float fontSize);
    float getFaceFontSize(PickId pickId) const;

    // Texture cache management
    void clearTextureCache();

private:
    // Font sizes for each face
    std::map<PickId, float> m_faceFontSizes;

    // Cached textures
    std::map<std::string, SoTexture2*> m_normalTextures;
    std::map<std::string, SoTexture2*> m_hoverTextures;

    // Get face label for a given PickId
    std::string getFaceLabel(PickId pickId);

    std::string getTextureDirectory() const;
    std::string getTextureFilePath(const std::string& faceName) const;
};
