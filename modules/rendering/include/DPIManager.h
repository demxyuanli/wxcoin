#pragma once

#include <wx/wx.h>
#include <wx/font.h>
#include <memory>
#include <unordered_map>
#include <functional>

class DPIManager {
public:
    static DPIManager& getInstance();
    
    // DPI scale management
    void updateDPIScale(float dpiScale);
    float getDPIScale() const { return m_dpiScale; }
    
    // Font scaling functions
    wxFont getScaledFont(const wxFont& baseFont) const;
    wxFont getScaledFont(int baseSizePoints, const wxString& faceName = wxEmptyString, 
                        bool bold = false, bool italic = false) const;
    int getScaledFontSize(int baseSizePoints) const;
    
    // Line width and point size scaling
    float getScaledLineWidth(float baseWidth) const;
    float getScaledPointSize(float baseSize) const;
    
    // Texture resolution scaling
    int getScaledTextureSize(int baseSize) const;
    wxSize getScaledImageSize(const wxSize& baseSize) const;
    
    // UI element scaling
    int getScaledSize(int baseSize) const;
    wxSize getScaledSize(const wxSize& baseSize) const;
    
    // High-DPI texture cache management
    struct TextureInfo {
        int width;
        int height;
        int channels;
        std::unique_ptr<unsigned char[]> data;
        
        TextureInfo(int w, int h, int c, unsigned char* d) 
            : width(w), height(h), channels(c), data(d) {}
    };
    
    std::shared_ptr<TextureInfo> getOrCreateScaledTexture(
        const std::string& key, 
        int baseSize,
        std::function<bool(unsigned char*, int, int)> generator);
    
    void clearTextureCache();
    
    // Constants for scaling policies
    static constexpr float MIN_DPI_SCALE = 1.0f;
    static constexpr float MAX_DPI_SCALE = 4.0f;
    static constexpr int MIN_FONT_SIZE = 8;
    static constexpr int MAX_FONT_SIZE = 72;
    static constexpr float MIN_LINE_WIDTH = 0.5f;
    static constexpr float MAX_LINE_WIDTH = 10.0f;

private:
    DPIManager() = default;
    ~DPIManager() = default;
    DPIManager(const DPIManager&) = delete;
    DPIManager& operator=(const DPIManager&) = delete;
    
    float m_dpiScale = 1.0f;
    std::unordered_map<std::string, std::shared_ptr<TextureInfo>> m_textureCache;
    
    // Helper functions
    float clampScale(float scale) const;
    int clampFontSize(int size) const;
    float clampLineWidth(float width) const;
}; 