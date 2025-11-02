/***************************************************************************
 *   Background Image Node for Coin3D                                      *
 ***************************************************************************/

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "SoFCBackgroundImage.h"
#include <GL/gl.h>
#include <wx/image.h>
#include <wx/filename.h>
#include <wx/filesys.h>
#include <vector>

SO_NODE_SOURCE(SoFCBackgroundImage)

void SoFCBackgroundImage::finish()
{
    atexit_cleanup();
}

void SoFCBackgroundImage::initClass()
{
    SO_NODE_INIT_CLASS(SoFCBackgroundImage, SoNode, "Node");
}

SoFCBackgroundImage::SoFCBackgroundImage()
{
    SO_NODE_CONSTRUCTOR(SoFCBackgroundImage);
    m_imagePath = "";
    m_opacity = 1.0f;
    m_fitMode = 0; // Default: Fill (tile)
    m_maintainAspect = true;
    m_textureId = 0;
    m_textureLoaded = false;
    m_textureWidth = 0;
    m_textureHeight = 0;
}

SoFCBackgroundImage::~SoFCBackgroundImage()
{
    // Clean up texture if loaded
    if (m_textureLoaded && m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
    }
}

void SoFCBackgroundImage::setImagePath(const std::string& path)
{
    if (m_imagePath != path) {
        // Unload old texture
        if (m_textureLoaded && m_textureId != 0) {
            glDeleteTextures(1, &m_textureId);
            m_textureId = 0;
            m_textureLoaded = false;
        }
        
        m_imagePath = path;
        
        // Load new texture if path is not empty
        if (!path.empty()) {
            unsigned int textureId = 0;
            int width = 0, height = 0;
            if (loadTexture(path, textureId, width, height)) {
                m_textureId = textureId;
                m_textureWidth = width;
                m_textureHeight = height;
                m_textureLoaded = true;
            }
        }
    }
}

void SoFCBackgroundImage::setOpacity(float opacity)
{
    m_opacity = opacity;
}

void SoFCBackgroundImage::setFitMode(int fit)
{
    m_fitMode = fit;
}

void SoFCBackgroundImage::setMaintainAspect(bool maintainAspect)
{
    m_maintainAspect = maintainAspect;
}

void SoFCBackgroundImage::GLRender(SoGLRenderAction * /*action*/)
{
    if (m_imagePath.empty() || !m_textureLoaded) {
        return;
    }

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Save and set OpenGL states
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    // Get viewport size
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int viewportWidth = viewport[2];
    int viewportHeight = viewport[3];

    // Calculate texture coordinates and vertex positions based on fit mode
    float texCoordLeft = 0.0f;
    float texCoordRight = 1.0f;
    float texCoordBottom = 0.0f;
    float texCoordTop = 1.0f;
    
    float vertexLeft = -1.0f;
    float vertexRight = 1.0f;
    float vertexBottom = -1.0f;
    float vertexTop = 1.0f;

    if (m_textureWidth > 0 && m_textureHeight > 0 && viewportWidth > 0 && viewportHeight > 0) {
        float textureAspect = static_cast<float>(m_textureWidth) / static_cast<float>(m_textureHeight);
        float viewportAspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

        switch (m_fitMode) {
        case 0: // Fill (Tile/Repeat)
        {
            // Calculate tiling based on viewport and texture sizes
            float tileX = static_cast<float>(viewportWidth) / static_cast<float>(m_textureWidth);
            float tileY = static_cast<float>(viewportHeight) / static_cast<float>(m_textureHeight);
            texCoordRight = tileX;
            texCoordTop = tileY;
            break;
        }
        case 1: // Fit (contain, show entire image)
        {
            // Fit mode: scale image to fit within viewport while maintaining aspect ratio
            // Calculate scale factor based on which dimension limits the fit
            float scaleX = static_cast<float>(viewportWidth) / static_cast<float>(m_textureWidth);
            float scaleY = static_cast<float>(viewportHeight) / static_cast<float>(m_textureHeight);
            
            // Use the smaller scale to ensure image fits entirely
            float scale = (scaleX < scaleY) ? scaleX : scaleY;
            
            // Calculate image size in pixels after scaling
            float scaledImageWidth = m_textureWidth * scale;
            float scaledImageHeight = m_textureHeight * scale;
            
            // Convert to NDC coordinates (-1 to 1)
            // In NDC space, we need to account for viewport aspect
            float imageWidthNDC = (scaledImageWidth / viewportWidth) * 2.0f;
            float imageHeightNDC = (scaledImageHeight / viewportHeight) * 2.0f;
            
            // Center the image
            vertexLeft = -imageWidthNDC / 2.0f;
            vertexRight = imageWidthNDC / 2.0f;
            vertexBottom = -imageHeightNDC / 2.0f;
            vertexTop = imageHeightNDC / 2.0f;
            
            // Texture coordinates stay 0-1
            break;
        }
        case 2: // Stretch (fill entire viewport, may distort)
        {
            // Use full viewport, texture coordinates stay 0-1
            // No adjustment needed, vertices are already -1 to 1
            break;
        }
        default:
            // Default to fill/tile
            {
                float tileX = static_cast<float>(viewportWidth) / static_cast<float>(m_textureWidth);
                float tileY = static_cast<float>(viewportHeight) / static_cast<float>(m_textureHeight);
                texCoordRight = tileX;
                texCoordTop = tileY;
                break;
            }
        }
    }

    // Set texture wrapping based on fit mode
    if (m_fitMode == 0) {
        // Fill mode: use repeat for tiling
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        // Fit and Stretch: use clamp (GL_CLAMP for compatibility, or GL_CLAMP_TO_EDGE if available)
        // GL_CLAMP clamps to [0,1] range with border color, which is acceptable for our use case
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }

    // Draw quad with calculated coordinates
    glColor4f(1.0f, 1.0f, 1.0f, m_opacity);
    glBegin(GL_QUADS);

    // Bottom-left
    glTexCoord2f(texCoordLeft, texCoordBottom);
    glVertex2f(vertexLeft, vertexBottom);

    // Bottom-right
    glTexCoord2f(texCoordRight, texCoordBottom);
    glVertex2f(vertexRight, vertexBottom);

    // Top-right
    glTexCoord2f(texCoordRight, texCoordTop);
    glVertex2f(vertexRight, vertexTop);

    // Top-left
    glTexCoord2f(texCoordLeft, texCoordTop);
    glVertex2f(vertexLeft, vertexTop);

    glEnd();

    // Restore OpenGL states
    glPopAttrib();
    glPopMatrix(); // Restore modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

bool SoFCBackgroundImage::loadTexture(const std::string& imagePath, unsigned int& textureId, int& width, int& height)
{
    // Convert path to wxString and normalize it
    wxString wxPath;
    try {
        // Try to convert from UTF-8
        wxPath = wxString::FromUTF8(imagePath);
    }
    catch (...) {
        // If UTF-8 conversion fails, try as-is
        wxPath = wxString(imagePath);
    }

    // Normalize the path to handle spaces and special characters
    wxFileName fileName(wxPath);
    if (!fileName.IsAbsolute()) {
        // If it's a relative path, make it absolute
        fileName.MakeAbsolute();
    }

    // Get the normalized full path
    wxString normalizedPath = fileName.GetFullPath();

    // Load image using wxImage with proper path handling
    wxImage image;
    bool loadSuccess = false;

    // Try multiple loading methods
    if (image.LoadFile(normalizedPath)) {
        loadSuccess = true;
    }
    else if (image.LoadFile(wxPath)) {
        loadSuccess = true;
    }
    else {
        // Try with wxFileSystem for better Unicode support
        wxFileSystem fs;
        wxFSFile* file = fs.OpenFile(wxPath);
        if (file) {
            wxInputStream* stream = file->GetStream();
            if (stream) {
                if (image.LoadFile(*stream)) {
                    loadSuccess = true;
                }
            }
            delete file;
        }
    }

    if (!loadSuccess) {
        return false;
    }

    // Get image data
    width = image.GetWidth();
    height = image.GetHeight();
    unsigned char* data = image.GetData();
    unsigned char* alpha = image.GetAlpha();

    if (!data) {
        return false;
    }

    // Generate OpenGL texture
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture parameters for tiling (GL_REPEAT instead of GL_CLAMP_TO_EDGE)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    if (alpha) {
        // Image has alpha channel - convert to RGBA
        std::vector<unsigned char> rgbaData(width * height * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIndex = (y * width + x) * 3;
                int dstIndex = ((height - 1 - y) * width + x) * 4; // Flip Y coordinate

                // wxImage stores data as RGB, convert to RGBA
                rgbaData[dstIndex + 0] = data[srcIndex + 0]; // R
                rgbaData[dstIndex + 1] = data[srcIndex + 1]; // G
                rgbaData[dstIndex + 2] = data[srcIndex + 2]; // B
                rgbaData[dstIndex + 3] = alpha[y * width + x]; // A
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData.data());
    }
    else {
        // Image has no alpha channel - convert RGB to RGBA
        std::vector<unsigned char> rgbaData(width * height * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIndex = (y * width + x) * 3;
                int dstIndex = ((height - 1 - y) * width + x) * 4; // Flip Y coordinate

                rgbaData[dstIndex + 0] = data[srcIndex + 0]; // R
                rgbaData[dstIndex + 1] = data[srcIndex + 1]; // G
                rgbaData[dstIndex + 2] = data[srcIndex + 2]; // B
                rgbaData[dstIndex + 3] = 255; // A (fully opaque)
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData.data());
    }

    return true;
}

