#include "NavigationCubeTextureGenerator.h"
#include "DPIManager.h"
#include "config/ConfigManager.h"
#include <algorithm>
#include <Inventor/nodes/SoTexture2.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/image.h>
#include <wx/filename.h>
#include <wx/time.h>
#include <cmath>
#include <wx/dir.h>
#include <wx/filefn.h>
#include "logger/Logger.h"

NavigationCubeTextureGenerator::NavigationCubeTextureGenerator() {
}

NavigationCubeTextureGenerator::~NavigationCubeTextureGenerator() {
    clearTextureCache();
}

std::string NavigationCubeTextureGenerator::getTextureDirectory() const {
    ConfigManager& cm = ConfigManager::getInstance();
    wxString configPathWx = wxString::FromUTF8(cm.getConfigFilePath().c_str());

    wxString baseDir;
    if (!configPathWx.empty()) {
        wxFileName configFile(configPathWx);
        baseDir = configFile.GetPath();
    }
    if (baseDir.empty()) {
        baseDir = wxFileName::GetCwd();
    }

    wxFileName textureDir(baseDir, "");
    textureDir.AppendDir("texture");
    wxString dirPath = textureDir.GetPath();

    if (!wxDirExists(dirPath)) {
        if (!wxFileName::Mkdir(dirPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
            LOG_DBG_S("Failed to create texture directory: " + std::string(dirPath.mb_str()));
        }
    }

    return std::string(dirPath.mb_str());
}

std::string NavigationCubeTextureGenerator::getTextureFilePath(const std::string& faceName) const {
    wxString dirWx = wxString::FromUTF8(getTextureDirectory().c_str());
    wxString fileWx = wxString::FromUTF8(faceName.c_str()) + ".png";
    wxFileName fileName(dirWx, fileWx);
    return std::string(fileName.GetFullPath().mb_str());
}

void NavigationCubeTextureGenerator::initializeFontSizes() {
    m_faceFontSizes.clear();

    // Calculate font sizes for all main faces like FreeCAD
    std::vector<PickId> mains = {PickId::Front, PickId::Top, PickId::Right, PickId::Rear, PickId::Bottom, PickId::Left};
    float minFontSize = 256.0f; // Default texture size
    float maxFontSize = 0.0f;

    // First pass: calculate font sizes for each face exactly like FreeCAD
    wxFont measureFont(256, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
    wxBitmap tempBitmap(1, 1);
    wxMemoryDC tempDC;
    tempDC.SelectObject(tempBitmap);
    tempDC.SetFont(measureFont);

    for (PickId pickId : mains) {
        std::string label = getFaceLabel(pickId);
        wxSize textBounds = tempDC.GetTextExtent(label);

        // Same calculation as FreeCAD: scale = texSize / max(width, height)
        int texSize = 256;
        int availableSize = texSize - 16; // 8 pixels margin on each side
        float scale = (float)availableSize / std::max(textBounds.GetWidth(), textBounds.GetHeight());
        m_faceFontSizes[pickId] = texSize * scale;
        minFontSize = std::min(minFontSize, m_faceFontSizes[pickId]);
        maxFontSize = std::max(maxFontSize, m_faceFontSizes[pickId]);
    }

    // Apply font zoom exactly like FreeCAD
    float fontZoom = 0.3f; // Default font zoom
    if (fontZoom > 0.0f) {
        maxFontSize = minFontSize + (maxFontSize - minFontSize) * fontZoom;
    } else {
        maxFontSize = minFontSize * std::pow(2.0f, fontZoom);
    }

    // Apply calculated sizes
    for (PickId pickId : mains) {
        if (m_faceFontSizes[pickId] > 0.5f) {
            m_faceFontSizes[pickId] = std::min(m_faceFontSizes[pickId], maxFontSize) * 0.9f;
        }
    }
}

// Create cube face textures exactly like FreeCAD NaviCube
void NavigationCubeTextureGenerator::createCubeFaceTextures() {
    LOG_DBG_S("=== TEXTURE GENERATION (6 main face textures) ===");
    int texSize = 256; // Increased size for better text clarity

    // Generate textures exactly like FreeCAD
    std::vector<PickId> mains = {PickId::Front, PickId::Top, PickId::Right, PickId::Rear, PickId::Bottom, PickId::Left};

    for (PickId pickId : mains) {
        std::string label = getFaceLabel(pickId);
        LOG_DBG_S("Generating texture for face: " + label + " ('" + label + "')");

        wxImage image(texSize, texSize);
        if (!image.HasAlpha()) {
            image.InitAlpha();
        }

        // Fill with opaque white background
        for (int y = 0; y < texSize; y++) {
            for (int x = 0; x < texSize; x++) {
                image.SetRGB(x, y, 255, 255, 255);
                image.SetAlpha(x, y, 255); // Opaque background
            }
        }

        if (m_faceFontSizes[pickId] > 0.5f) {
            float finalFontSize = m_faceFontSizes[pickId];
            wxBitmap bitmap(image);
            wxMemoryDC dc;
            dc.SelectObject(bitmap);

            wxFont font(static_cast<int>(finalFontSize), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
            dc.SetFont(font);
            dc.SetTextForeground(wxColour(0, 100, 255, 255)); // Blue text for visibility
            dc.SetTextBackground(wxColour(255, 255, 255, 255)); // Opaque white background

            wxSize textSize = dc.GetTextExtent(label);
            int x = (texSize - textSize.GetWidth()) / 2;
            int y = (texSize - textSize.GetHeight()) / 2;

            dc.DrawText(label, x, y);

            // Apply vertical balance like FreeCAD's imageVerticalBalance
            int offset = calculateVerticalBalance(bitmap, static_cast<int>(finalFontSize));
            if (offset != 0) {
                wxImage redrawImage = bitmap.ConvertToImage();
                if (!redrawImage.HasAlpha()) {
                    redrawImage.InitAlpha();
                }
                unsigned char* redrawAlpha = redrawImage.GetAlpha();
                for (int i = 0; i < texSize * texSize; i++) {
                    redrawAlpha[i] = 255;
                }
                bitmap = wxBitmap(redrawImage);
                dc.SelectObject(bitmap);
                dc.SetFont(font);
                dc.SetTextForeground(wxColour(0, 100, 255, 255));
                int finalY = std::max(8, std::min(y + offset, texSize - textSize.GetHeight() - 8));
                dc.DrawText(label, x, finalY);
            }

            image = bitmap.ConvertToImage();
        }

        if (pickId == PickId::Bottom || pickId == PickId::Rear) {
            image = image.Mirror(false);
        } else if (pickId == PickId::Left) {
            image = image.Rotate90(false);
        } else if (pickId == PickId::Right) {
            image = image.Rotate90(true);
        }

        if (!image.HasAlpha()) {
            image.InitAlpha();
        }
        unsigned char* finalAlpha = image.GetAlpha();
        for (int i = 0; i < texSize * texSize; i++) {
            finalAlpha[i] = 255;
        }

        // Convert wxImage to RGBA data
        unsigned char* imageData = new unsigned char[texSize * texSize * 4];
        if (!image.HasAlpha()) {
            image.InitAlpha();
        }
        unsigned char* rgb = image.GetData();
        unsigned char* alpha = image.GetAlpha();

        for (int i = 0, j = 0; i < texSize * texSize * 4; i += 4, j += 3) {
            imageData[i] = rgb[j];
            imageData[i + 1] = rgb[j + 1];
            imageData[i + 2] = rgb[j + 2];
            imageData[i + 3] = 255; // All pixels fully opaque
        }

        // Create Open Inventor texture
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(texSize, texSize), 4, imageData);
        texture->model = SoTexture2::MODULATE;
        texture->wrapS = SoTexture2::CLAMP;
        texture->wrapT = SoTexture2::CLAMP;

        // Cache the texture
        if (m_normalTextures.find(label) != m_normalTextures.end()) {
            m_normalTextures[label]->unref();
        }
        texture->ref();
        m_normalTextures[label] = texture;

        delete[] imageData;
    }
}

// Get face label for a given PickId
std::string NavigationCubeTextureGenerator::getFaceLabel(PickId pickId) {
    switch (pickId) {
        case PickId::Front: return "FRONT";
        case PickId::Top: return "TOP";
        case PickId::Right: return "RIGHT";
        case PickId::Rear: return "REAR";
        case PickId::Bottom: return "BOTTOM";
        case PickId::Left: return "LEFT";
        default: return "";
    }
}

// Helper function to calculate vertical balance exactly like FreeCAD's imageVerticalBalance
int NavigationCubeTextureGenerator::calculateVerticalBalance(const wxBitmap& bitmap, int fontSizeHint) const {
    if (fontSizeHint < 0) {
        return 0;
    }

    wxImage image = bitmap.ConvertToImage();
    if (!image.IsOk()) {
        return 0;
    }

    int h = image.GetHeight();
    int startRow = (h - fontSizeHint) / 2;
    bool done = false;
    int x, bottom, top;

    // Find top edge of text
    for (top = startRow; top < h; top++) {
        for (x = 0; x < image.GetWidth(); x++) {
            if (image.GetAlpha(x, top) > 0) {
                done = true;
                break;
            }
        }
        if (done) break;
    }

    // Find bottom edge of text
    for (bottom = startRow; bottom < h; bottom++) {
        for (x = 0; x < image.GetWidth(); x++) {
            if (image.GetAlpha(x, h - 1 - bottom) > 0) {
                return (bottom - top) / 2;
            }
        }
    }

    return 0;
}

bool NavigationCubeTextureGenerator::generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height, const wxColour& bgColor, float faceSize, PickId pickId) {
    // Create bitmap with white background
    wxBitmap bitmap(width, height, 32);
    wxMemoryDC dc;
    dc.SelectObject(bitmap);
    if (!dc.IsOk()) {
        LOG_ERR_S("NavigationCubeTextureGenerator::generateFaceTexture: Failed to create wxMemoryDC for texture: " + text);
        return false;
    }

    // Clear bitmap with white background
    dc.SetBackground(wxBrush(wxColour(255, 255, 255, 255))); // White opaque background
    dc.Clear();

    // Ensure alpha channel is initialized
    wxImage bgImage = bitmap.ConvertToImage();
    if (!bgImage.HasAlpha()) {
        bgImage.InitAlpha();
    }
    unsigned char* bgAlpha = bgImage.GetAlpha();
    for (int i = 0; i < width * height; i++) {
        bgAlpha[i] = 255; // Set all pixels to opaque
    }
    bitmap = wxBitmap(bgImage);
    dc.SelectObject(bitmap);

    auto& dpiManager = DPIManager::getInstance();

    int baseFontSize;
    if (faceSize > 0) {
        baseFontSize = static_cast<int>(faceSize);
    } else {
        baseFontSize = 12;
    }

    int margin = 16;
    int availableWidth = std::max(8, width - margin * 2);
    int availableHeight = std::max(8, height - margin * 2);

    // Measure text with a large reference font to compute scale ratio
    const int referencePointSize = 200;
    wxFont measureFont(referencePointSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Impact");
    if (!measureFont.IsOk() || !measureFont.SetFaceName("Impact")) {
        measureFont.SetFaceName("Arial");
    }
    wxBitmap measureBmp(1, 1);
    wxMemoryDC measureDC;
    measureDC.SelectObject(measureBmp);
    measureDC.SetFont(measureFont);

    wxSize extent = measureDC.GetTextExtent(wxString::FromUTF8(text.c_str()));
    if (extent.GetWidth() <= 0 || extent.GetHeight() <= 0) {
        extent.SetWidth(referencePointSize);
        extent.SetHeight(referencePointSize);
    }

    double scaleX = static_cast<double>(availableWidth) / static_cast<double>(extent.GetWidth());
    double scaleY = static_cast<double>(availableHeight) / static_cast<double>(extent.GetHeight());
    double scale = std::min(scaleX, scaleY);

    int fittedFontSize = static_cast<int>(std::floor(referencePointSize * scale));
    fittedFontSize = std::max(fittedFontSize, baseFontSize);
    fittedFontSize = std::max(fittedFontSize, 8);

    wxFont font(fittedFontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Impact");
    if (!font.IsOk()) {
        font = wxFont(fittedFontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
    }
    if (!font.SetFaceName("Impact")) {
        font.SetFaceName("Arial");
    }
    font.SetPointSize(fittedFontSize);

    dc.SetFont(font);
    dc.SetBackground(wxBrush(wxColour(255, 255, 255, 0)));
    dc.SetBackgroundMode(wxTRANSPARENT);

    wxColour fillColor(0, 80, 220, 255);
    wxColour outlineColor(0, 60, 180, 255);
    wxString textWx = wxString::FromUTF8(text.c_str());

    wxSize textSize = dc.GetTextExtent(textWx);

    int x = (width - textSize.GetWidth()) / 2;
    int y = (height - textSize.GetHeight()) / 2;

    x = std::max(margin, std::min(x, width - textSize.GetWidth() - margin));
    y = std::max(margin, std::min(y, height - textSize.GetHeight() - margin));

    static const wxPoint outlineOffsets[8] = {
        wxPoint(-1, 0), wxPoint(1, 0), wxPoint(0, -1), wxPoint(0, 1),
        wxPoint(-1, -1), wxPoint(-1, 1), wxPoint(1, -1), wxPoint(1, 1)
    };
    dc.SetTextForeground(outlineColor);
    for (const auto& offset : outlineOffsets) {
        dc.DrawText(textWx, x + offset.x, y + offset.y);
    }

    dc.SetTextForeground(fillColor);
    dc.DrawText(textWx, x, y);

    // Apply vertical balance
    int verticalOffset = calculateVerticalBalance(bitmap, textSize.GetHeight());
    if (verticalOffset != 0) {
        wxImage redrawImage = bitmap.ConvertToImage();
        if (!redrawImage.HasAlpha()) {
            redrawImage.InitAlpha();
        }
        unsigned char* redrawAlpha = redrawImage.GetAlpha();
        for (int i = 0; i < width * height; i++) {
            redrawAlpha[i] = 255;
        }
        bitmap = wxBitmap(redrawImage);
        dc.SelectObject(bitmap);

        dc.SetFont(font);
        dc.SetBackgroundMode(wxTRANSPARENT);
        int finalY = std::max(margin, std::min(y + verticalOffset, height - textSize.GetHeight() - margin));
        dc.SetTextForeground(outlineColor);
        for (const auto& offset : outlineOffsets) {
            dc.DrawText(textWx, x + offset.x, finalY + offset.y);
        }
        dc.SetTextForeground(fillColor);
        dc.DrawText(textWx, x, finalY);
    }

    // Validate bitmap content
    wxImage image = bitmap.ConvertToImage();

        // For DECAL mode: background transparent (alpha=0), text opaque (alpha=255)
    if (!image.HasAlpha()) {
        image.InitAlpha();
    }
    unsigned char* finalAlpha = image.GetAlpha();
    unsigned char* rgb = image.GetData();

    for (int i = 0; i < width * height; i++) {
        // Check if this pixel is background (white) or text (blue)
        int r = rgb[i * 3];
        int g = rgb[i * 3 + 1];
        int b = rgb[i * 3 + 2];

        // If pixel is white (background), make it transparent
        // If pixel is blue (text), make it opaque
        if (r > 200 && g > 200 && b > 200) { // White background
            finalAlpha[i] = 0; // Transparent
        } else { // Text (blue)
            finalAlpha[i] = 255; // Opaque
        }
    }

    bitmap = wxBitmap(image);
    image = bitmap.ConvertToImage();

    if (!image.IsOk()) {
        LOG_ERR_S("NavigationCubeTextureGenerator::generateFaceTexture: Failed to convert bitmap to image for texture: " + text);
        return false;
    }

    if (!image.HasAlpha()) {
        image.InitAlpha();
    }
    unsigned char* alpha = image.GetAlpha();

    // Copy to imageData (RGBA)
    bool hasValidPixels = false;
    for (int i = 0, j = 0, k = 0; i < width * height * 4; i += 4, j += 3, k++) {
        imageData[i] = rgb[j];     // R
        imageData[i + 1] = rgb[j + 1]; // G
        imageData[i + 2] = rgb[j + 2]; // B
        imageData[i + 3] = alpha[k];
        if (imageData[i] != 0 || imageData[i + 1] != 0 || imageData[i + 2] != 0) {
            hasValidPixels = true;
        }
    }

    if (!hasValidPixels) {
        LOG_DBG_S("NavigationCubeTextureGenerator::generateFaceTexture: All pixels are black for texture: " + text);
        // Fallback: Fill with opaque white background
        for (int i = 0; i < width * height * 4; i += 4) {
            imageData[i] = 255; // R (white)
            imageData[i + 1] = 255; // G (white)
            imageData[i + 2] = 255; // B (white)
            imageData[i + 3] = 255; // Background: opaque
        }
    }

    return true;
}

SoTexture2* NavigationCubeTextureGenerator::createTextureForFace(const std::string& faceName, bool isHover) {
    LOG_DBG_S("=== Creating texture for face: " + faceName + " (hover: " + (isHover ? "true" : "false") + ") ===");

    bool hasText = (faceName == "FRONT" || faceName == "REAR" || faceName == "LEFT" ||
                   faceName == "RIGHT" || faceName == "TOP" || faceName == "BOTTOM");

    wxColour backgroundColor = wxColour(255, 255, 255, 255); // White opaque background

    int texSize = ConfigManager::getInstance().getInt("NavigationCube", "TextureBaseSize", 312);

    ConfigManager& configManager = ConfigManager::getInstance();
    wxString configPath = wxString::FromUTF8(configManager.getConfigFilePath().c_str());
    wxFileName configFile(configPath);
    wxString textureDir = configFile.GetPath();
    if (!textureDir.IsEmpty()) {
        textureDir += wxFileName::GetPathSeparator();
    }
    textureDir += "texture";

    wxString fileName = wxString::FromUTF8(faceName.c_str());
    if (isHover) {
        fileName += "_hover";
    }
    fileName += ".png";

    wxFileName textureFile(textureDir, fileName);
    wxString texturePath = textureFile.GetFullPath();

    wxImage finalImage;
    std::vector<unsigned char> imageData;
    int imageWidth = 0;
    int imageHeight = 0;
    bool loadedFromFile = false;

    if (wxFileExists(texturePath)) {
        if (finalImage.LoadFile(texturePath, wxBITMAP_TYPE_PNG)) {
            imageWidth = finalImage.GetWidth();
            imageHeight = finalImage.GetHeight();
            if (imageWidth > 0 && imageHeight > 0) {
                if (!finalImage.HasAlpha()) {
                    finalImage.InitAlpha();
                    unsigned char* alphaData = finalImage.GetAlpha();
                    if (alphaData) {
                        std::fill(alphaData, alphaData + imageWidth * imageHeight, 255);
                    }
                }
                unsigned char* rgb = finalImage.GetData();
                unsigned char* alpha = finalImage.GetAlpha();
                imageData.resize(static_cast<size_t>(imageWidth) * imageHeight * 4);
                for (int y = 0; y < imageHeight; ++y) {
                    for (int x = 0; x < imageWidth; ++x) {
                        int pixelIndex = y * imageWidth + x;
                        int srcRgbIndex = pixelIndex * 3;
                        int dstIndex = pixelIndex * 4;
                        imageData[dstIndex] = rgb[srcRgbIndex];
                        imageData[dstIndex + 1] = rgb[srcRgbIndex + 1];
                        imageData[dstIndex + 2] = rgb[srcRgbIndex + 2];
                        imageData[dstIndex + 3] = alpha ? alpha[pixelIndex] : 255;
                    }
                }
                loadedFromFile = true;
                LOG_DBG_S("  Loaded texture from file: " + std::string(texturePath.mb_str()));
            } else {
                LOG_DBG_S("  Texture file has invalid dimensions: " + std::string(texturePath.mb_str()));
            }
        } else {
            LOG_DBG_S("  Failed to load texture file: " + std::string(texturePath.mb_str()) + " - falling back to generated texture");
        }
    }

    if (!loadedFromFile) {
        int texWidth = hasText ? texSize : 2;
        int texHeight = hasText ? texSize : 2;
        imageData.assign(static_cast<size_t>(texWidth) * texHeight * 4, 0);

        std::string textureText = hasText ? faceName : "";
        float correctFontSize = 0;
        PickId pickId = PickId::Front;

        if (hasText) {
            if (faceName == "FRONT") pickId = PickId::Front;
            else if (faceName == "REAR") pickId = PickId::Rear;
            else if (faceName == "LEFT") pickId = PickId::Left;
            else if (faceName == "RIGHT") pickId = PickId::Right;
            else if (faceName == "TOP") pickId = PickId::Top;
            else if (faceName == "BOTTOM") pickId = PickId::Bottom;

            auto it = m_faceFontSizes.find(pickId);
            if (it != m_faceFontSizes.end()) {
                correctFontSize = it->second;
            } else {
                correctFontSize = static_cast<float>(texSize);
            }
        }

        if (!generateFaceTexture(textureText, imageData.data(), texWidth, texHeight, backgroundColor, correctFontSize, pickId)) {
            LOG_ERR_S("  Texture generation FAILED for face: " + faceName);
            return nullptr;
        }

        imageWidth = texWidth;
        imageHeight = texHeight;

        finalImage.Create(imageWidth, imageHeight);
        finalImage.InitAlpha();
        for (int y = 0; y < imageHeight; ++y) {
            for (int x = 0; x < imageWidth; ++x) {
                int idx = (y * imageWidth + x) * 4;
                finalImage.SetRGB(x, y, imageData[idx], imageData[idx + 1], imageData[idx + 2]);
                finalImage.SetAlpha(x, y, imageData[idx + 3]);
            }
        }

        if (!wxDirExists(textureDir)) {
            if (!wxFileName::Mkdir(textureDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
                LOG_DBG_S("  Failed to create texture directory: " + std::string(textureDir.mb_str()));
            }
        }

        if (finalImage.IsOk()) {
            if (finalImage.SaveFile(texturePath, wxBITMAP_TYPE_PNG)) {
                LOG_DBG_S("  Generated texture saved to: " + std::string(texturePath.mb_str()));
            } else {
                LOG_DBG_S("  Failed to save generated texture to: " + std::string(texturePath.mb_str()));
            }
        }
    }

    if (imageWidth <= 0 || imageHeight <= 0 || imageData.empty()) {
        LOG_ERR_S("  Texture data invalid for face: " + faceName);
        return nullptr;
    }

    std::vector<unsigned char> flippedImageData(imageData.size());
    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            int srcIndex = (y * imageWidth + x) * 4;
            int dstIndex = ((imageHeight - 1 - y) * imageWidth + x) * 4;
            flippedImageData[dstIndex] = imageData[srcIndex];
            flippedImageData[dstIndex + 1] = imageData[srcIndex + 1];
            flippedImageData[dstIndex + 2] = imageData[srcIndex + 2];
            flippedImageData[dstIndex + 3] = imageData[srcIndex + 3];
        }
    }

    SoTexture2* texture = new SoTexture2;
    texture->image.setValue(SbVec2s(imageWidth, imageHeight), 4, flippedImageData.data());

    if (hasText) {
        texture->model = SoTexture2::DECAL;
        texture->wrapS = SoTexture2::CLAMP;
        texture->wrapT = SoTexture2::CLAMP;
        LOG_DBG_S("    Texture mode: DECAL + CLAMP (text texture, " + std::to_string(imageWidth) + "x" + std::to_string(imageHeight) + ")");
    } else {
        texture->model = SoTexture2::MODULATE;
        texture->wrapS = SoTexture2::REPEAT;
        texture->wrapT = SoTexture2::REPEAT;
        LOG_DBG_S("    Texture mode: MODULATE + REPEAT (solid color texture)");
    }

    return texture;
}

void NavigationCubeTextureGenerator::generateAndCacheTextures() {
    LOG_DBG_S("=== Starting texture generation and caching for main faces ===");

    // Debug ConfigManager
    ConfigManager& cm = ConfigManager::getInstance();
    LOG_DBG_S("DEBUG: ConfigManager initialized: " + std::string(cm.getConfigFilePath()));
    LOG_DBG_S("DEBUG: ConfigManager sections: " + std::to_string(cm.getSections().size()));

    bool saveDebugTextures = cm.getBool("NavigationCube", "SaveDebugTextures", false);
    LOG_DBG_S("DEBUG: Config read - SaveDebugTextures = " + std::string(saveDebugTextures ? "true" : "false"));

    // Also test reading another known config value
    bool showTextures = cm.getBool("NavigationCube", "ShowTextures", true);
    LOG_DBG_S("DEBUG: Config read - ShowTextures = " + std::string(showTextures ? "true" : "false"));

    if (saveDebugTextures) {
        LOG_DBG_S("DEBUG: Texture debug PNG saving is ENABLED - PNG files will be saved to program directory");
    } else {
        LOG_DBG_S("DEBUG: Texture debug PNG saving is DISABLED (set SaveDebugTextures=true in config.ini to enable)");
    }

    std::vector<std::string> mainFaces = {
        "FRONT", "REAR", "LEFT", "RIGHT", "TOP", "BOTTOM"
    };

    int normalCount = 0;
    int hoverCount = 0;

    for (const auto& faceName : mainFaces) {
        LOG_DBG_S("DEBUG: Processing face: " + faceName);
        // Generate normal state texture
        SoTexture2* normalTexture = createTextureForFace(faceName, false);
        if (normalTexture) {
            normalTexture->ref();
            m_normalTextures[faceName] = normalTexture;
            normalCount++;
            LOG_DBG_S("DEBUG: Normal texture created for: " + faceName);
        } else {
            LOG_DBG_S("DEBUG: Failed to create normal texture for: " + faceName);
        }

        // Generate hover state texture
        SoTexture2* hoverTexture = createTextureForFace(faceName, true);
        if (hoverTexture) {
            hoverTexture->ref();
            m_hoverTextures[faceName] = hoverTexture;
            hoverCount++;
        }
    }

    LOG_DBG_S("=== Texture generation completed ===");
    LOG_DBG_S("  Normal textures generated: " + std::to_string(normalCount));
    LOG_DBG_S("  Hover textures generated: " + std::to_string(hoverCount));
}

SoTexture2* NavigationCubeTextureGenerator::getNormalTexture(const std::string& faceName) const {
    auto it = m_normalTextures.find(faceName);
    return (it != m_normalTextures.end()) ? it->second : nullptr;
}

SoTexture2* NavigationCubeTextureGenerator::getHoverTexture(const std::string& faceName) const {
    auto it = m_hoverTextures.find(faceName);
    return (it != m_hoverTextures.end()) ? it->second : nullptr;
}

void NavigationCubeTextureGenerator::setFaceFontSize(PickId pickId, float fontSize) {
    m_faceFontSizes[pickId] = fontSize;
}

float NavigationCubeTextureGenerator::getFaceFontSize(PickId pickId) const {
    auto it = m_faceFontSizes.find(pickId);
    return (it != m_faceFontSizes.end()) ? it->second : 0.0f;
}

void NavigationCubeTextureGenerator::clearTextureCache() {
    for (auto& pair : m_normalTextures) {
        if (pair.second) {
            pair.second->unref();
        }
    }
    for (auto& pair : m_hoverTextures) {
        if (pair.second) {
            pair.second->unref();
        }
    }
    m_normalTextures.clear();
    m_hoverTextures.clear();
}
