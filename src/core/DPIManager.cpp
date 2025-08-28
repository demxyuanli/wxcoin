#include "DPIManager.h"
#include "logger/Logger.h"
#include <algorithm>
#include <cmath>

DPIManager& DPIManager::getInstance() {
	static DPIManager instance;
	return instance;
}

void DPIManager::updateDPIScale(float dpiScale) {
	float newScale = clampScale(dpiScale);
	if (std::abs(m_dpiScale - newScale) > 0.01f) {
		LOG_INF_S("DPIManager: Updating DPI scale from " + std::to_string(m_dpiScale) +
			" to " + std::to_string(newScale));
		m_dpiScale = newScale;

		// Clear texture cache when DPI changes to regenerate at new resolution
		clearTextureCache();
	}
}

wxFont DPIManager::getScaledFont(const wxFont& baseFont) const {
	int scaledSize = static_cast<int>(clampFontSize(static_cast<float>(baseFont.GetPointSize() * m_dpiScale)));

	wxFont scaledFont = baseFont;
	scaledFont.SetPointSize(scaledSize);

	LOG_DBG_S("DPIManager: Scaled font from " + std::to_string(baseFont.GetPointSize()) +
		" to " + std::to_string(scaledSize) + " points");

	return scaledFont;
}

wxFont DPIManager::getScaledFont(int baseSizePoints, const wxString& faceName,
	bool bold, bool italic) const {
	int scaledSize = getScaledFontSize(baseSizePoints);

	wxFontInfo fontInfo(scaledSize);
	if (!faceName.IsEmpty()) {
		fontInfo.FaceName(faceName);
	}
	if (bold) {
		fontInfo.Bold();
	}
	if (italic) {
		fontInfo.Italic();
	}

	return wxFont(fontInfo);
}

int DPIManager::getScaledFontSize(int baseSizePoints) const {
	return static_cast<int>(clampFontSize(static_cast<float>(baseSizePoints * m_dpiScale)));
}

float DPIManager::getScaledLineWidth(float baseWidth) const {
	float scaledWidth = baseWidth * m_dpiScale;
	return clampLineWidth(scaledWidth);
}

float DPIManager::getScaledPointSize(float baseSize) const {
	return (std::max)(0.5f, baseSize * m_dpiScale);
}

int DPIManager::getScaledTextureSize(int baseSize) const {
	// For textures, we want to maintain quality at higher DPI
	// Use power-of-2 scaling for better GPU compatibility
	int scaledSize = static_cast<int>(baseSize * m_dpiScale);

	// Round up to nearest power of 2 for optimal GPU performance
	int powerOf2 = 1;
	while (powerOf2 < scaledSize) {
		powerOf2 *= 2;
	}

	// Clamp to reasonable limits (32 to 2048)
	powerOf2 = (std::max)(32, (std::min)(2048, powerOf2));

	LOG_DBG_S("DPIManager: Scaled texture size from " + std::to_string(baseSize) +
		" to " + std::to_string(powerOf2) + " (scale: " + std::to_string(m_dpiScale) + ")");

	return powerOf2;
}

wxSize DPIManager::getScaledImageSize(const wxSize& baseSize) const {
	return wxSize(
		getScaledTextureSize(baseSize.x),
		getScaledTextureSize(baseSize.y)
	);
}

int DPIManager::getScaledSize(int baseSize) const {
	return static_cast<int>(std::round(baseSize * m_dpiScale));
}

wxSize DPIManager::getScaledSize(const wxSize& baseSize) const {
	return wxSize(
		getScaledSize(baseSize.x),
		getScaledSize(baseSize.y)
	);
}

std::shared_ptr<DPIManager::TextureInfo> DPIManager::getOrCreateScaledTexture(
	const std::string& key,
	int baseSize,
	std::function<bool(unsigned char*, int, int)> generator) {
	int scaledSize = getScaledTextureSize(baseSize);
	std::string cacheKey = key + "_" + std::to_string(scaledSize) + "_" + std::to_string(m_dpiScale);

	auto it = m_textureCache.find(cacheKey);
	if (it != m_textureCache.end()) {
		LOG_DBG_S("DPIManager: Using cached texture: " + cacheKey);
		return it->second;
	}

	// Generate new high-DPI texture
	int channels = 4; // RGBA
	auto textureData = std::make_unique<unsigned char[]>(scaledSize * scaledSize * channels);

	if (generator(textureData.get(), scaledSize, scaledSize)) {
		auto textureInfo = std::make_shared<TextureInfo>(
			scaledSize, scaledSize, channels, textureData.release()
		);

		m_textureCache[cacheKey] = textureInfo;
		LOG_INF_S("DPIManager: Generated and cached high-DPI texture: " + cacheKey +
			" (" + std::to_string(scaledSize) + "x" + std::to_string(scaledSize) + ")");

		return textureInfo;
	}

	LOG_ERR_S("DPIManager: Failed to generate texture: " + cacheKey);
	return nullptr;
}

void DPIManager::clearTextureCache() {
	size_t cacheSize = m_textureCache.size();
	m_textureCache.clear();
	LOG_INF_S("DPIManager: Cleared texture cache (" + std::to_string(cacheSize) + " textures)");
}

float DPIManager::clampScale(float scale) const {
	return (std::max)(static_cast<float>(MIN_DPI_SCALE), (std::min)(static_cast<float>(MAX_DPI_SCALE), scale));
}

float DPIManager::clampFontSize(float size) const {
	return (std::max)(static_cast<float>(MIN_FONT_SIZE), (std::min)(static_cast<float>(MAX_FONT_SIZE), size));
}

float DPIManager::clampLineWidth(float width) const {
	return (std::max)(static_cast<float>(MIN_LINE_WIDTH), (std::min)(static_cast<float>(MAX_LINE_WIDTH), width));
}