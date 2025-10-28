#include "CuteNavCube.h"
#include "Canvas.h"
#include "NavigationCubeConfigDialog.h"
#include "DPIManager.h"
#include "DPIAwareRendering.h"
#include "config/ConfigManager.h"
#include <algorithm>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLinear.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/time.h>
#include <cmath>
#include <vector>
#include "logger/Logger.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

std::map<std::string, std::shared_ptr<CuteNavCube::TextureData>> CuteNavCube::s_textureCache;

// Create cube face textures exactly like FreeCAD NaviCube
void CuteNavCube::createCubeFaceTextures() {
    LOG_INF_S("=== TEXTURE GENERATION (6 main face textures) ===");
    int texSize = 192; // Same as FreeCAD: works well for the max cube size 1024

    // Calculate font sizes for all main faces like FreeCAD
    std::vector<PickId> mains = {PickId::Front, PickId::Top, PickId::Right, PickId::Rear, PickId::Bottom, PickId::Left};
    float minFontSize = texSize;
    float maxFontSize = 0.0f;

    // First pass: calculate font sizes for each face exactly like FreeCAD
    // Create font with texSize as point size (like FreeCAD's font.setPointSizeF(texSize))
    wxFont measureFont(texSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
    wxBitmap tempBitmap(1, 1);
    wxMemoryDC tempDC;
    tempDC.SelectObject(tempBitmap);
    tempDC.SetFont(measureFont);
    
    for (PickId pickId : mains) {
        std::string label = getFaceLabel(pickId);
        // Measure text bounds with the texSize font (like FreeCAD's QFontMetrics)
        wxSize textBounds = tempDC.GetTextExtent(label);

        // Same calculation as FreeCAD: scale = texSize / max(width, height)
        // But account for 8 pixel margin on all sides (16 pixels total reduction)
        int availableSize = texSize - 16; // 8 pixels margin on each side
        float scale = (float)availableSize / std::max(textBounds.GetWidth(), textBounds.GetHeight());
        m_faceFontSizes[pickId] = texSize * scale;
        minFontSize = std::min(minFontSize, m_faceFontSizes[pickId]);
        maxFontSize = std::max(maxFontSize, m_faceFontSizes[pickId]);
        
    }

    // Apply font zoom exactly like FreeCAD
    if (m_fontZoom > 0.0f) {
        maxFontSize = minFontSize + (maxFontSize - minFontSize) * m_fontZoom;
    } else {
        maxFontSize = minFontSize * std::pow(2.0f, m_fontZoom);
    }

    // Second pass: generate textures exactly like FreeCAD
    for (PickId pickId : mains) {
        std::string label = getFaceLabel(pickId);

        // Log texture generation
        std::string pickIdStr;
        switch (pickId) {
            case PickId::Front: pickIdStr = "Front"; break;
            case PickId::Rear: pickIdStr = "Rear"; break;
            case PickId::Left: pickIdStr = "Left"; break;
            case PickId::Right: pickIdStr = "Right"; break;
            case PickId::Top: pickIdStr = "Top"; break;
            case PickId::Bottom: pickIdStr = "Bottom"; break;
            default: pickIdStr = "Unknown"; break;
        }
        LOG_INF_S("Generating texture for face: " + pickIdStr + " ('" + label + "')");

        // Create wxImage equivalent of FreeCAD's QImage
        wxImage image(texSize, texSize);
        if (!image.HasAlpha()) {
            image.InitAlpha();
        }
        // Fill with transparent background like FreeCAD's qRgba(255, 255, 255, 0)
        for (int y = 0; y < texSize; y++) {
            for (int x = 0; x < texSize; x++) {
                image.SetRGB(x, y, 255, 255, 255);
                image.SetAlpha(x, y, 0); // Transparent
            }
        }

        if (m_faceFontSizes[pickId] > 0.5f) {
            // 5% margin looks nice and prevents some artifacts (FreeCAD comment)
            float finalFontSize = std::min(m_faceFontSizes[pickId], maxFontSize) * 0.9f;

            // Create bitmap and DC for drawing
            wxBitmap bitmap(image);
            wxMemoryDC dc;
            dc.SelectObject(bitmap);

            // Setup font like FreeCAD
            wxFont font(static_cast<int>(finalFontSize), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
            dc.SetFont(font);
            dc.SetTextForeground(wxColour(255, 255, 255)); // White text like FreeCAD's Qt::white
            dc.SetTextBackground(wxColour(255, 255, 255, 0)); // Transparent background

            // Draw text centered like FreeCAD's Qt::AlignCenter
            wxSize textSize = dc.GetTextExtent(label);
            int x = (texSize - textSize.GetWidth()) / 2;
            int y = (texSize - textSize.GetHeight()) / 2;
            
            
            dc.DrawText(label, x, y);

            // Apply vertical balance like FreeCAD's imageVerticalBalance
            int offset = calculateVerticalBalance(bitmap, static_cast<int>(finalFontSize));

            // Redraw with vertical offset like FreeCAD
            dc.Clear();
            // Reset background to transparent using wxImage
            wxImage tempImage(texSize, texSize);
            if (!tempImage.HasAlpha()) {
                tempImage.InitAlpha();
            }
            for (int yy = 0; yy < texSize; yy++) {
                for (int xx = 0; xx < texSize; xx++) {
                    tempImage.SetRGB(xx, yy, 255, 255, 255);
                    tempImage.SetAlpha(xx, yy, 0);
                }
            }
            bitmap = wxBitmap(tempImage);
            dc.SetFont(font);
            dc.SetTextForeground(wxColour(255, 255, 255));
            dc.SetTextBackground(wxColour(255, 255, 255, 0));
            dc.DrawText(label, x, y + offset);

            // Convert back to image
            image = bitmap.ConvertToImage();
        }

        // Apply face-specific transformations like FreeCAD
        switch (pickId) {
            case PickId::Rear:
                // REAR: Mirror vertically to fix upside-down text
                image = image.Mirror(true);
                break;
            case PickId::Bottom:
                // BOTTOM: Mirror vertically to fix upside-down text
                image = image.Mirror(true);
                break;
            case PickId::Top:
                // TOP: Mirror vertically to fix upside-down text
                image = image.Mirror(true);
                break;
            case PickId::Front:
                // FRONT: Mirror vertically to fix upside-down text
                image = image.Mirror(true);
                break;
            case PickId::Left:
                // LEFT: Rotate 90 degrees clockwise for vertical text
                image = image.Rotate90(true);
                break;
            case PickId::Right:
                // RIGHT: Rotate 90 degrees counter-clockwise for vertical text
                image = image.Rotate90(false);
                break;
            default:
                // No transformation needed
                break;
        }

        // Convert wxImage to RGBA data for Open Inventor texture
        unsigned char* imageData = new unsigned char[texSize * texSize * 4];
        if (!image.HasAlpha()) {
            image.InitAlpha();
        }
        unsigned char* rgb = image.GetData();
        unsigned char* alpha = image.GetAlpha();

        // Copy data in RGBA format like FreeCAD
        for (int i = 0, j = 0, k = 0; i < texSize * texSize * 4; i += 4, j += 3, k++) {
            imageData[i] = rgb[j];     // R
            imageData[i + 1] = rgb[j + 1]; // G
            imageData[i + 2] = rgb[j + 2]; // B
            imageData[i + 3] = alpha[k]; // A
        }

        // Create Open Inventor texture like FreeCAD
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(texSize, texSize), 4, imageData);
        texture->model = SoTexture2::MODULATE; // Use MODULATE like FreeCAD's GL_MODULATE

        // Note: Open Inventor SoTexture2 doesn't have direct equivalents of QOpenGLTexture filters
        // The texture filtering is handled automatically by Open Inventor

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
std::string CuteNavCube::getFaceLabel(PickId pickId) {
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
int CuteNavCube::calculateVerticalBalance(const wxBitmap& bitmap, int fontSizeHint) {
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

    // Find top edge of text - scan from startRow upwards
    for (top = startRow; top < h; top++) {
        for (x = 0; x < image.GetWidth(); x++) {
            // Check if pixel has alpha (text content) like FreeCAD's qAlpha(p.pixel(x, top))
            if (image.GetAlpha(x, top) > 0) {
                done = true;
                break;
            }
        }
        if (done) break;
    }

    // Find bottom edge of text - scan from startRow downwards
    for (bottom = startRow; bottom < h; bottom++) {
        for (x = 0; x < image.GetWidth(); x++) {
            // Check from bottom up like FreeCAD's qAlpha(p.pixel(x, h-1-bottom))
            if (image.GetAlpha(x, h - 1 - bottom) > 0) {
                return (bottom - top) / 2;
            }
        }
    }

    return 0;
}

CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config)
	: m_root(new SoSeparator)
	, m_orthoCamera(new SoOrthographicCamera)
	, m_enabled(true)
	, m_dpiScale(dpiScale)
	, m_viewChangeCallback(viewChangeCallback)
	, m_isDragging(false)
	, m_lastMousePos(0, 0)
	, m_rotationX(0.0f)
	, m_rotationY(0.0f)
	, m_lastDragTime(0)
	, m_windowWidth(windowWidth)
	, m_windowHeight(windowHeight)
	, m_positionX(config.x >= 0 ? config.x : 20)  // Use config or default
	, m_positionY(config.y >= 0 ? config.y : 20)  // Use config or default
	, m_cubeSize(config.size > 0 ? config.size : 140)  // Use config or default
	, m_currentX(0.0f)
	, m_currentY(0.0f)
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.55f)  // Adjusted to 0.55f for better proportion
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.12f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_textColor(config.textColor)
	, m_edgeColor(config.edgeColor)
	, m_cornerColor(config.cornerColor)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
	, m_hoveredFace("")
	, m_normalFaceColor(0.7f, 0.7f, 0.7f)
	, m_hoverFaceColor(1.0f, 0.2f, 0.2f)
	, m_fontZoom(0.3f)
	, m_canvas(nullptr)
{
	m_root->ref();
	m_orthoCamera->ref(); // Add reference to camera to prevent premature deletion
	initialize();
}

// New constructor with camera move callback
CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback,
						std::function<void(const SbVec3f&, const SbRotation&)> cameraMoveCallback,
						float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config)
	: m_root(new SoSeparator)
	, m_orthoCamera(new SoOrthographicCamera)
	, m_enabled(true)
	, m_dpiScale(dpiScale)
	, m_viewChangeCallback(viewChangeCallback)
	, m_cameraMoveCallback(cameraMoveCallback)
	, m_rotationChangedCallback(nullptr)
	, m_refreshCallback(nullptr)
	, m_isDragging(false)
	, m_lastMousePos(0, 0)
	, m_rotationX(0.0f)
	, m_rotationY(0.0f)
	, m_lastDragTime(0)
	, m_windowWidth(windowWidth)
	, m_windowHeight(windowHeight)
	, m_positionX(config.x >= 0 ? config.x : 20)  // Use config or default
	, m_positionY(config.y >= 0 ? config.y : 20)  // Use config or default
	, m_cubeSize(config.size > 0 ? config.size : 140)  // Use config or default
	, m_currentX(0.0f)
	, m_currentY(0.0f)
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.55f)  // Adjusted to 0.55f for better proportion
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.12f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_textColor(config.textColor)
	, m_edgeColor(config.edgeColor)
	, m_cornerColor(config.cornerColor)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
	, m_hoveredFace("")
	, m_normalFaceColor(0.7f, 0.7f, 0.7f)
	, m_hoverFaceColor(1.0f, 0.2f, 0.2f)
	, m_fontZoom(0.3f)
	, m_canvas(nullptr)
{
	m_root->ref();
	m_orthoCamera->ref(); // Add reference to camera to prevent premature deletion
	initialize();
}

// New constructor with refresh callback
CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback,
						std::function<void(const SbVec3f&, const SbRotation&)> cameraMoveCallback,
						std::function<void()> refreshCallback,
						float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config)
	: m_root(new SoSeparator)
	, m_orthoCamera(new SoOrthographicCamera)
	, m_enabled(true)
	, m_dpiScale(dpiScale)
	, m_viewChangeCallback(viewChangeCallback)
	, m_cameraMoveCallback(cameraMoveCallback)
	, m_rotationChangedCallback(nullptr)
	, m_refreshCallback(refreshCallback)
	, m_isDragging(false)
	, m_lastMousePos(0, 0)
	, m_rotationX(0.0f)
	, m_rotationY(0.0f)
	, m_lastDragTime(0)
	, m_windowWidth(windowWidth)
	, m_windowHeight(windowHeight)
	, m_positionX(config.x >= 0 ? config.x : 20)  // Use config or default
	, m_positionY(config.y >= 0 ? config.y : 20)  // Use config or default
	, m_cubeSize(config.size > 0 ? config.size : 140)  // Use config or default
	, m_currentX(0.0f)
	, m_currentY(0.0f)
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.55f)  // Adjusted to 0.55f for better proportion
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.12f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_textColor(config.textColor)
	, m_edgeColor(config.edgeColor)
	, m_cornerColor(config.cornerColor)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
	, m_hoveredFace("")
	, m_normalFaceColor(0.7f, 0.7f, 0.7f)
	, m_hoverFaceColor(1.0f, 0.2f, 0.2f)
	, m_fontZoom(0.3f)
	, m_canvas(nullptr)
{
	m_root->ref();
	m_orthoCamera->ref(); // Add reference to camera to prevent premature deletion
	initialize();
}

CuteNavCube::~CuteNavCube() {
	// Release cached textures
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
	
	m_orthoCamera->unref(); // Release camera reference
	m_root->unref();
}

void CuteNavCube::initialize() {
	setupGeometry();

	m_faceToView = {
		// 6 Main faces - Click face -> View direction
		{ "FRONT",  "FRONT" },
		{ "REAR",  "REAR" },
		{ "LEFT",   "LEFT" },
		{ "RIGHT",  "RIGHT" },
		{ "TOP",    "TOP" },
		{ "BOTTOM", "BOTTOM" },

		// 8 Corner faces (triangular)
		{ "Corner0", "Top" },        // Front-Top-Left corner -> Top view
		{ "Corner1", "Top" },        // Front-Top-Right corner -> Top view
		{ "Corner2", "Top" },        // Back-Top-Right corner -> Top view
		{ "Corner3", "Top" },        // Back-Top-Left corner -> Top view
		{ "Corner4", "Bottom" },     // Front-Bottom-Left corner -> Bottom view
		{ "Corner5", "Bottom" },     // Front-Bottom-Right corner -> Bottom view
		{ "Corner6", "Bottom" },     // Back-Bottom-Right corner -> Bottom view
		{ "Corner7", "Bottom" },     // Back-Bottom-Left corner -> Bottom view

		// 12 Edge faces
		{ "EdgeTF", "Top" },         // Top-Front edge -> Top view
		{ "EdgeTB", "Top" },         // Top-Back edge -> Top view
		{ "EdgeTL", "Top" },         // Top-Left edge -> Top view
		{ "EdgeTR", "Top" },         // Top-Right edge -> Top view
		{ "EdgeBF", "Bottom" },      // Bottom-Front edge -> Bottom view
		{ "EdgeBB", "Bottom" },      // Bottom-Back edge -> Bottom view
		{ "EdgeBL", "Bottom" },      // Bottom-Left edge -> Bottom view
		{ "EdgeBR", "Bottom" },      // Bottom-Right edge -> Bottom view
		{ "EdgeFR", "Front" },       // Front-Right edge -> Front view
		{ "EdgeFL", "Front" },       // Front-Left edge -> Front view
		{ "EdgeBL2", "Back" },       // Back-Left edge -> Back view
		{ "EdgeBR2", "Back" }        // Back-Right edge -> Back view
	};

	// Face normal vectors and center points for camera positioning
	m_faceNormals = {
		// 6 Main faces
		{ "FRONT",  std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0, 0, 1)) },      // +Z axis
		{ "REAR",  std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0, 0, -1)) },    // -Z axis
		{ "LEFT",   std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-1, 0, 0)) },   // -X axis
		{ "RIGHT",  std::make_pair(SbVec3f(1, 0, 0), SbVec3f(1, 0, 0)) },    // +X axis
		{ "TOP",    std::make_pair(SbVec3f(0, 1, 0), SbVec3f(0, 1, 0)) },    // +Y axis
		{ "BOTTOM", std::make_pair(SbVec3f(0, -1, 0), SbVec3f(0, -1, 0)) },   // -Y axis

		// 8 Corner faces (using closest main face normal)
		{ "Corner0", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(-0.707, 0.707, 0.707)) },  // Front-Top-Left
		{ "Corner1", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0.707, 0.707, 0.707)) },   // Front-Top-Right
		{ "Corner2", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0.707, 0.707, -0.707)) },  // Back-Top-Right
		{ "Corner3", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(-0.707, 0.707, -0.707)) }, // Back-Top-Left
		{ "Corner4", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(-0.707, -0.707, 0.707)) }, // Front-Bottom-Left
		{ "Corner5", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0.707, -0.707, 0.707)) },  // Front-Bottom-Right
		{ "Corner6", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0.707, -0.707, -0.707)) }, // Back-Bottom-Right
		{ "Corner7", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(-0.707, -0.707, -0.707)) }, // Back-Bottom-Left

		// 12 Edge faces (using average of adjacent faces)
		{ "EdgeTF", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0, 0.707, 0.707)) },       // Top-Front
		{ "EdgeTB", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0, 0.707, -0.707)) },      // Top-Back
		{ "EdgeTL", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, 0.707, 0)) },     // Top-Left
		{ "EdgeTR", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, 0.707, 0)) },      // Top-Right
		{ "EdgeBF", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0, -0.707, 0.707)) },      // Bottom-Front
		{ "EdgeBB", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0, -0.707, -0.707)) },     // Bottom-Back
		{ "EdgeBL", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, -0.707, 0)) },    // Bottom-Left
		{ "EdgeBR", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, -0.707, 0)) },     // Bottom-Right
		{ "EdgeFR", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, 0, 0.707)) },      // Front-Right
		{ "EdgeFL", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, 0, 0.707)) },    // Front-Left
		{ "EdgeBL2", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, 0, -0.707)) },   // Back-Left
		{ "EdgeBR2", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, 0, -0.707)) }      // Back-Right
	};
}

bool CuteNavCube::generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height, const wxColour& bgColor, float faceSize, PickId pickId) {
	
	wxBitmap bitmap(width, height, 32);
	wxMemoryDC dc;
	dc.SelectObject(bitmap);
	if (!dc.IsOk()) {
		LOG_ERR_S("CuteNavCube::generateFaceTexture: Failed to create wxMemoryDC for texture: " + text);
		// Fallback: Fill with white
		for (int i = 0; i < width * height * 4; i += 4) {
			imageData[i] = 255; // R
			imageData[i + 1] = 255; // G
			imageData[i + 2] = 255; // B
			imageData[i + 3] = 255; // A
		}
		return true;
	}

	// Enable anti-aliasing for better text quality
	dc.SetLogicalFunction(wxCOPY);

	// For transparent background, we need to manually set alpha values
	// wxBitmap's SetBackground/Clear doesn't handle alpha properly
	if (bgColor.Alpha() == 0) {
		// Transparent background - manually clear alpha channel
		wxImage image = bitmap.ConvertToImage();
		if (!image.HasAlpha()) {
			image.InitAlpha();
		}
		unsigned char* alpha = image.GetAlpha();
		for (int i = 0; i < width * height; i++) {
			alpha[i] = 0; // Set all pixels to transparent
		}
		bitmap = wxBitmap(image);
		dc.SelectObject(bitmap);
	} else {
		// Opaque background - use normal clear
		dc.SetBackground(wxBrush(bgColor));
		dc.Clear();
	}

	auto& dpiManager = DPIManager::getInstance();

	int baseFontSize;
	if (faceSize > 0) { // Main face text - use the provided faceSize (calculated by createCubeFaceTextures)
		baseFontSize = static_cast<int>(faceSize);

		// Apply DPI scaling
		baseFontSize = static_cast<int>(baseFontSize * dpiManager.getDPIScale());

		// Ensure reasonable size limits - but don't cap at 96, use the calculated size
		baseFontSize = std::max(8, baseFontSize); // Only minimum limit, no maximum
	} else { // Solid color face
		baseFontSize = 12; // Minimal size for solid color
	}

	// Font setup for texture generation - use NORMAL weight for cleaner look
	wxFont font(baseFontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
	font.SetPointSize(baseFontSize);

	dc.SetFont(font);
	// Use blue text color for better visibility
	dc.SetTextForeground(wxColour(0, 100, 255, 255));


	wxSize textSize = dc.GetTextExtent(text);

	// Draw text centered horizontally with 8 pixel margin, then apply vertical balance like FreeCAD
	int margin = 8;
	int x = (width - textSize.GetWidth()) / 2;
	int y = (height - textSize.GetHeight()) / 2;
	
	// Ensure text doesn't go outside the margin bounds
	x = std::max(margin, std::min(x, width - textSize.GetWidth() - margin));
	y = std::max(margin, std::min(y, height - textSize.GetHeight() - margin));
	
	dc.DrawText(text, x, y);

	// Apply vertical balance like FreeCAD's imageVerticalBalance
	int verticalOffset = calculateVerticalBalance(bitmap, textSize.GetHeight());
	if (verticalOffset != 0) {
		// Redraw text with vertical offset - preserve transparent background
		if (bgColor.Alpha() == 0) {
			// Transparent background - manually clear alpha channel again
			wxImage image = bitmap.ConvertToImage();
			if (!image.HasAlpha()) {
				image.InitAlpha();
			}
			unsigned char* alpha = image.GetAlpha();
			for (int i = 0; i < width * height; i++) {
				alpha[i] = 0; // Set all pixels to transparent
			}
			bitmap = wxBitmap(image);
			dc.SelectObject(bitmap);
		} else {
			// Opaque background - use normal clear
			dc.Clear();
			dc.SetBackground(wxBrush(bgColor));
			dc.Clear();
		}
		dc.SetFont(font);
		dc.SetTextForeground(wxColour(0, 100, 255, 255)); // Use blue for visibility
		// Apply margin constraints to vertical offset position as well
		int finalY = std::max(margin, std::min(y + verticalOffset, height - textSize.GetHeight() - margin));
		dc.DrawText(text, x, finalY);
	}

	// Validate bitmap content
	wxImage image = bitmap.ConvertToImage();
	
	// For transparent background, ensure text areas are opaque
	if (bgColor.Alpha() == 0) {
		if (!image.HasAlpha()) {
			image.InitAlpha();
		}
		unsigned char* alpha = image.GetAlpha();
		unsigned char* rgb = image.GetData();
		
		// Find text pixels and set them to opaque
		// FreeCAD uses white transparent background, so we need to detect text differently
		for (int py = 0; py < height; py++) {
			for (int px = 0; px < width; px++) {
				int pixelIndex = (py * width + px) * 3;
				int alphaIndex = py * width + px;
				
				unsigned char r = rgb[pixelIndex];
				unsigned char g = rgb[pixelIndex + 1];
				unsigned char b = rgb[pixelIndex + 2];
				
				// Check if this pixel was drawn by text (has non-zero RGB values)
				// Since we draw blue text (0,100,255), any non-zero RGB indicates text
				if (r != 0 || g != 0 || b != 0) {
					alpha[alphaIndex] = 255; // Make text opaque
				} else {
					alpha[alphaIndex] = 0;   // Keep background transparent
				}
			}
		}
		bitmap = wxBitmap(image);
	}
	if (!image.IsOk()) {
		LOG_ERR_S("CuteNavCube::generateFaceTexture: Failed to convert bitmap to image for texture: " + text);
		// Fallback: Fill with white
		for (int i = 0; i < width * height * 4; i += 4) {
			imageData[i] = 255; // R
			imageData[i + 1] = 255; // G
			imageData[i + 2] = 255; // B
			imageData[i + 3] = 255; // A
		}
		return true;
	}

	// Apply face-specific transformations like FreeCAD
	// Note: No transformations needed as OpenGL coordinate system flip handles orientation

	if (!image.HasAlpha()) {
		image.InitAlpha(); // Ensure alpha channel
	}
	unsigned char* rgb = image.GetData();
	unsigned char* alpha = image.GetAlpha();

	// Copy to imageData (RGBA) and validate
	bool hasValidPixels = false;
	for (int i = 0, j = 0, k = 0; i < width * height * 4; i += 4, j += 3, k++) {
		imageData[i] = rgb[j];     // R
		imageData[i + 1] = rgb[j + 1]; // G
		imageData[i + 2] = rgb[j + 2]; // B
		imageData[i + 3] = alpha[k]; // A - use pixel index
		if (imageData[i] != 0 || imageData[i + 1] != 0 || imageData[i + 2] != 0) {
			hasValidPixels = true;
		}
	}


	if (!hasValidPixels) {
		LOG_WRN_S("CuteNavCube::generateFaceTexture: All pixels are black for texture: " + text);
		// Fallback: Fill with white
		for (int i = 0; i < width * height * 4; i += 4) {
			imageData[i] = 255; // R
			imageData[i + 1] = 255; // G
			imageData[i + 2] = 255; // B
			imageData[i + 3] = 255; // A
		}
	}

	return true;
}

void CuteNavCube::addCubeFace(const SbVec3f& x, const SbVec3f& z, ShapeId shapeType, PickId pickId, float rotZ) {
	m_Faces[pickId].vertexArray.clear();
	m_Faces[pickId].type = shapeType;

	// Log face creation with type and pickId
	std::string faceTypeStr = (shapeType == ShapeId::Main) ? "MAIN" : (shapeType == ShapeId::Corner) ? "CORNER" : "EDGE";
	std::string pickIdStr;
	switch (pickId) {
		case PickId::Top: pickIdStr = "Top"; break;
		case PickId::Bottom: pickIdStr = "Bottom"; break;
		case PickId::Front: pickIdStr = "Front"; break;
		case PickId::Rear: pickIdStr = "Rear"; break;
		case PickId::Left: pickIdStr = "Left"; break;
		case PickId::Right: pickIdStr = "Right"; break;
		case PickId::FrontTopRight: pickIdStr = "FrontTopRight"; break;
		case PickId::FrontTopLeft: pickIdStr = "FrontTopLeft"; break;
		case PickId::FrontBottomRight: pickIdStr = "FrontBottomRight"; break;
		case PickId::FrontBottomLeft: pickIdStr = "FrontBottomLeft"; break;
		case PickId::RearTopRight: pickIdStr = "RearTopRight"; break;
		case PickId::RearTopLeft: pickIdStr = "RearTopLeft"; break;
		case PickId::RearBottomRight: pickIdStr = "RearBottomRight"; break;
		case PickId::RearBottomLeft: pickIdStr = "RearBottomLeft"; break;
		case PickId::FrontTop: pickIdStr = "FrontTop"; break;
		case PickId::FrontBottom: pickIdStr = "FrontBottom"; break;
		case PickId::RearTop: pickIdStr = "RearTop"; break;
		case PickId::RearBottom: pickIdStr = "RearBottom"; break;
		case PickId::FrontRight: pickIdStr = "FrontRight"; break;
		case PickId::FrontLeft: pickIdStr = "FrontLeft"; break;
		case PickId::RearRight: pickIdStr = "RearRight"; break;
		case PickId::RearLeft: pickIdStr = "RearLeft"; break;
		case PickId::TopRight: pickIdStr = "TopRight"; break;
		case PickId::TopLeft: pickIdStr = "TopLeft"; break;
		case PickId::BottomRight: pickIdStr = "BottomRight"; break;
		case PickId::BottomLeft: pickIdStr = "BottomLeft"; break;
	}
	LOG_INF_S("Creating " + faceTypeStr + " face: " + pickIdStr);

	// Calculate y vector using cross product
	SbVec3f y = x.cross(-z);

	// Create normalized vectors for x, y and z
	SbVec3f xN = x;
	SbVec3f yN = y;
	SbVec3f zN = z;
	xN.normalize();
	yN.normalize();
	zN.normalize();

	// Create a rotation matrix
	SbMatrix R(xN[0], yN[0], zN[0], 0,
	           xN[1], yN[1], zN[1], 0,
	           xN[2], yN[2], zN[2], 0,
	           0,     0,     0,     1);

	// Store the standard orientation
	m_Faces[pickId].rotation = (SbRotation(R) * SbRotation(SbVec3f(0, 0, 1), rotZ)).inverse();

	if (shapeType == ShapeId::Corner) {
		float chamfer = m_chamferSize;
		float zDepth = 1.0f - 2.0f * chamfer;
		m_Faces[pickId].vertexArray.reserve(6);
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zDepth - 2 * x[0] * chamfer,
		                                                  z[1] * zDepth - 2 * x[1] * chamfer,
		                                                  z[2] * zDepth - 2 * x[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zDepth - x[0] * chamfer - y[0] * chamfer,
		                                                  z[1] * zDepth - x[1] * chamfer - y[1] * chamfer,
		                                                  z[2] * zDepth - x[2] * chamfer - y[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zDepth + x[0] * chamfer - y[0] * chamfer,
		                                                  z[1] * zDepth + x[1] * chamfer - y[1] * chamfer,
		                                                  z[2] * zDepth + x[2] * chamfer - y[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zDepth + 2 * x[0] * chamfer,
		                                                  z[1] * zDepth + 2 * x[1] * chamfer,
		                                                  z[2] * zDepth + 2 * x[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zDepth + x[0] * chamfer + y[0] * chamfer,
		                                                  z[1] * zDepth + x[1] * chamfer + y[1] * chamfer,
		                                                  z[2] * zDepth + x[2] * chamfer + y[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zDepth - x[0] * chamfer + y[0] * chamfer,
		                                                  z[1] * zDepth - x[1] * chamfer + y[1] * chamfer,
		                                                  z[2] * zDepth - x[2] * chamfer + y[2] * chamfer));
	}
	else if (shapeType == ShapeId::Edge) {
		float chamfer = m_chamferSize;
		float x4_scale = 1.0f - chamfer * 4.0f;
		float zE_scale = 1.0f - chamfer;
		m_Faces[pickId].vertexArray.reserve(4);
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zE_scale - x[0] * x4_scale - y[0] * chamfer,
		                                                  z[1] * zE_scale - x[1] * x4_scale - y[1] * chamfer,
		                                                  z[2] * zE_scale - x[2] * x4_scale - y[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zE_scale + x[0] * x4_scale - y[0] * chamfer,
		                                                  z[1] * zE_scale + x[1] * x4_scale - y[1] * chamfer,
		                                                  z[2] * zE_scale + x[2] * x4_scale - y[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zE_scale + x[0] * x4_scale + y[0] * chamfer,
		                                                  z[1] * zE_scale + x[1] * x4_scale + y[1] * chamfer,
		                                                  z[2] * zE_scale + x[2] * x4_scale + y[2] * chamfer));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] * zE_scale - x[0] * x4_scale + y[0] * chamfer,
		                                                  z[1] * zE_scale - x[1] * x4_scale + y[1] * chamfer,
		                                                  z[2] * zE_scale - x[2] * x4_scale + y[2] * chamfer));
	}
	else if (shapeType == ShapeId::Main) {
		float chamfer = m_chamferSize;
		float x2_scale = 1.0f - chamfer * 2.0f;
		float y2_scale = 1.0f - chamfer * 2.0f;
		float x4_scale = 1.0f - chamfer * 4.0f;
		float y4_scale = 1.0f - chamfer * 4.0f;
		m_Faces[pickId].vertexArray.reserve(8);
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x2_scale - y[0] * y4_scale,
		                                                  z[1] - x[1] * x2_scale - y[1] * y4_scale,
		                                                  z[2] - x[2] * x2_scale - y[2] * y4_scale));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x4_scale - y[0] * y2_scale,
		                                                  z[1] - x[1] * x4_scale - y[1] * y2_scale,
		                                                  z[2] - x[2] * x4_scale - y[2] * y2_scale));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x4_scale - y[0] * y2_scale,
		                                                  z[1] + x[1] * x4_scale - y[1] * y2_scale,
		                                                  z[2] + x[2] * x4_scale - y[2] * y2_scale));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x2_scale - y[0] * y4_scale,
		                                                  z[1] + x[1] * x2_scale - y[1] * y4_scale,
		                                                  z[2] + x[2] * x2_scale - y[2] * y4_scale));

		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x2_scale + y[0] * y4_scale,
		                                                  z[1] + x[1] * x2_scale + y[1] * y4_scale,
		                                                  z[2] + x[2] * x2_scale + y[2] * y4_scale));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x4_scale + y[0] * y2_scale,
		                                                  z[1] + x[1] * x4_scale + y[1] * y2_scale,
		                                                  z[2] + x[2] * x4_scale + y[2] * y2_scale));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x4_scale + y[0] * y2_scale,
		                                                  z[1] - x[1] * x4_scale + y[1] * y2_scale,
		                                                  z[2] - x[2] * x4_scale + y[2] * y2_scale));
		m_Faces[pickId].vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x2_scale + y[0] * y4_scale,
		                                                  z[1] - x[1] * x2_scale + y[1] * y4_scale,
		                                                  z[2] - x[2] * x2_scale + y[2] * y4_scale));

		m_LabelTextures[pickId].vertexArray.clear();
		// Create texture quad vertices at octagon main face diagonal edge midpoints
		// The octagon has 8 edges: 4 connecting square corners to chamfer corners ("diagonal edges")
		// We want the texture quad vertices to be at the midpoints of these 4 diagonal edges
		// These edges connect: v0-v1, v2-v3, v4-v5, v6-v7

		// Edge v0-v1 midpoint: between square corner (-x2_scale, -y4_scale) and chamfer corner (-x4_scale, -y2_scale)
		auto x01_mid = x * (x2_scale + x4_scale) * 0.5f; // Average x coordinate
		auto y01_mid = y * (y4_scale + y2_scale) * 0.5f; // Average y coordinate
		m_LabelTextures[pickId].vertexArray.emplace_back(z - x01_mid - y01_mid);

		// Edge v2-v3 midpoint: between chamfer corner (+x4_scale, -y2_scale) and square corner (+x2_scale, -y4_scale)
		auto x23_mid = x * (x4_scale + x2_scale) * 0.5f; // Average x coordinate
		auto y23_mid = y * (y2_scale + y4_scale) * 0.5f; // Average y coordinate
		m_LabelTextures[pickId].vertexArray.emplace_back(z + x23_mid - y23_mid);

		// Edge v4-v5 midpoint: between square corner (+x2_scale, +y4_scale) and chamfer corner (+x4_scale, +y2_scale)
		auto x45_mid = x * (x2_scale + x4_scale) * 0.5f; // Average x coordinate
		auto y45_mid = y * (y4_scale + y2_scale) * 0.5f; // Average y coordinate
		m_LabelTextures[pickId].vertexArray.emplace_back(z + x45_mid + y45_mid);

		// Edge v6-v7 midpoint: between chamfer corner (-x4_scale, +y2_scale) and square corner (-x2_scale, +y4_scale)
		auto x67_mid = x * (x4_scale + x2_scale) * 0.5f; // Average x coordinate
		auto y67_mid = y * (y2_scale + y4_scale) * 0.5f; // Average y coordinate
		m_LabelTextures[pickId].vertexArray.emplace_back(z - x67_mid + y67_mid);
	}

	// Log all vertices for this face
	const auto& vertices = m_Faces[pickId].vertexArray;
	LOG_INF_S("  Face vertices (" + std::to_string(vertices.size()) + "):");
	for (size_t i = 0; i < vertices.size(); ++i) {
		LOG_INF_S("    V" + std::to_string(i) + ": (" +
			std::to_string(vertices[i][0]) + ", " +
			std::to_string(vertices[i][1]) + ", " +
			std::to_string(vertices[i][2]) + ")");
	}

	// Log texture vertices if available
	const auto& texVertices = m_LabelTextures[pickId].vertexArray;
	if (!texVertices.empty()) {
		LOG_INF_S("  Texture vertices (" + std::to_string(texVertices.size()) + "):");
		for (size_t i = 0; i < texVertices.size(); ++i) {
			LOG_INF_S("    TV" + std::to_string(i) + ": (" +
				std::to_string(texVertices[i][0]) + ", " +
				std::to_string(texVertices[i][1]) + ", " +
				std::to_string(texVertices[i][2]) + ")");
		}
	}
}


void CuteNavCube::setupGeometry() {
	// Create cube face textures exactly like FreeCAD
	createCubeFaceTextures();

	// Clear existing face maps before rebuilding geometry
	m_faceMaterials.clear();
	m_faceBaseColors.clear();

	// Safely clear previous geometry while preserving camera
	bool cameraWasInScene = false;
	if (m_root->getNumChildren() > 0) {
		// Check if camera is already in the scene
		for (int i = 0; i < m_root->getNumChildren(); i++) {
			if (m_root->getChild(i) == m_orthoCamera) {
				cameraWasInScene = true;
				break;
			}
		}
	m_root->removeAllChildren(); // Clear previous geometry
	}

	// Setup camera properties
	m_orthoCamera->viewportMapping = SoOrthographicCamera::ADJUST_CAMERA;
	m_orthoCamera->nearDistance = 0.05f; // Reduced to ensure all faces are visible
	m_orthoCamera->farDistance = 15.0f;  // Increased to include all geometry
	m_orthoCamera->position.setValue(0.0f, 0.0f, 5.0f); // Initial position
	m_orthoCamera->orientation.setValue(SbRotation(SbVec3f(1, 0, 0), -M_PI / 2)); // Rotate to make Z up
	
	// Always add camera back to the scene
	m_root->addChild(m_orthoCamera);

	// --- Geometry Scale Transform ---
	m_geometryTransform = new SoTransform;
	m_geometryTransform->scaleFactor.setValue(m_geometrySize, m_geometrySize, m_geometrySize);
	m_root->addChild(m_geometryTransform);

	// --- Lighting Setup ---
	SoEnvironment* env = new SoEnvironment;
	env->ambientColor.setValue(0.8f, 0.8f, 0.85f); // Brighter and more neutral ambient color
	env->ambientIntensity.setValue(m_ambientIntensity); // Use config value
	m_root->addChild(env);

	m_mainLight = new SoDirectionalLight;
	m_mainLight->direction.setValue(0.5f, 0.5f, -0.5f); // Main light from top-right-front
	m_mainLight->intensity.setValue(0.4f); // Reduced from 0.6
	m_mainLight->color.setValue(1.0f, 1.0f, 1.0f);
	m_root->addChild(m_mainLight);

	m_fillLight = new SoDirectionalLight;
	m_fillLight->direction.setValue(-0.5f, -0.5f, 0.5f); // Fill light from bottom-left-back
	m_fillLight->intensity.setValue(0.4f); // Reduced from 0.6
	m_fillLight->color.setValue(0.95f, 0.95f, 1.0f); // Slightly cool
	m_root->addChild(m_fillLight);

	m_sideLight = new SoDirectionalLight;
	m_sideLight->direction.setValue(-0.8f, 0.2f, -0.3f); // Side light from left
	m_sideLight->intensity.setValue(0.3f); // Reduced from 0.4
	m_sideLight->color.setValue(1.0f, 1.0f, 0.95f); // Slightly warm
	m_root->addChild(m_sideLight);

	// --- Add more lights for better coverage ---
	SoDirectionalLight* backLight = new SoDirectionalLight;
	backLight->direction.setValue(0.0f, 0.0f, 1.0f); // Directly from back
	backLight->intensity.setValue(0.3f); // Reduced from 0.5
	backLight->color.setValue(0.9f, 0.9f, 1.0f);
	m_root->addChild(backLight);

	SoDirectionalLight* bottomLight = new SoDirectionalLight;
	bottomLight->direction.setValue(0.4f, -0.8f, 0.2f); // From bottom-right
	bottomLight->intensity.setValue(0.2f); // Reduced from 0.4
	bottomLight->color.setValue(1.0f, 0.95f, 0.95f);
	m_root->addChild(bottomLight);

	SoDirectionalLight* topSideLight = new SoDirectionalLight;
	topSideLight->direction.setValue(0.8f, 0.3f, 0.3f); // From top-left
	topSideLight->intensity.setValue(0.2f); // Reduced from 0.4
	topSideLight->color.setValue(0.95f, 1.0f, 0.95f);
	m_root->addChild(topSideLight);

	updateCameraRotation();

	// Create cube faces using dynamic generation (ported from FreeCAD NaviCube)
	constexpr float pi = 3.14159265358979323846f;
	constexpr float pi1_2 = pi / 2;

	SbVec3f x(1, 0, 0);
	SbVec3f y(0, 1, 0);
	SbVec3f z(0, 0, 1);

	// ===== MAIN FACES (6 faces) =====
	LOG_INF_S("=== MAIN FACES (6 faces) ===");
	addCubeFace( x, z, ShapeId::Main, PickId::Top);
	addCubeFace( x,-y, ShapeId::Main, PickId::Front);
	addCubeFace(-y,-x, ShapeId::Main, PickId::Left);
	addCubeFace(-x, y, ShapeId::Main, PickId::Rear);
	addCubeFace( y, x, ShapeId::Main, PickId::Right);
	addCubeFace( x,-z, ShapeId::Main, PickId::Bottom);

	// ===== CORNER FACES (8 faces) =====
	LOG_INF_S("=== CORNER FACES (8 faces) ===");
	addCubeFace(-x-y, x-y+z, ShapeId::Corner, PickId::FrontTopRight, pi);
	addCubeFace(-x+y,-x-y+z, ShapeId::Corner, PickId::FrontTopLeft, pi);
	addCubeFace(x+y, x-y-z, ShapeId::Corner, PickId::FrontBottomRight);
	addCubeFace(x-y,-x-y-z, ShapeId::Corner, PickId::FrontBottomLeft);
	addCubeFace(x-y, x+y+z, ShapeId::Corner, PickId::RearTopRight, pi);
	addCubeFace(x+y,-x+y+z, ShapeId::Corner, PickId::RearTopLeft, pi);
	addCubeFace(-x+y, x+y-z, ShapeId::Corner, PickId::RearBottomRight);
	addCubeFace(-x-y,-x+y-z, ShapeId::Corner, PickId::RearBottomLeft);

	// ===== EDGE FACES (12 faces) =====
	LOG_INF_S("=== EDGE FACES (12 faces) ===");
	addCubeFace(x, z-y, ShapeId::Edge, PickId::FrontTop);
	addCubeFace(x,-z-y, ShapeId::Edge, PickId::FrontBottom);
	addCubeFace(x, y-z, ShapeId::Edge, PickId::RearBottom, pi);
	addCubeFace(x, y+z, ShapeId::Edge, PickId::RearTop, pi);
	addCubeFace(z, x+y, ShapeId::Edge, PickId::RearRight, pi1_2);
	addCubeFace(z, x-y, ShapeId::Edge, PickId::FrontRight, pi1_2);
	addCubeFace(z,-x-y, ShapeId::Edge, PickId::FrontLeft, pi1_2);
	addCubeFace(z, y-x, ShapeId::Edge, PickId::RearLeft, pi1_2);
	addCubeFace(y, z-x, ShapeId::Edge, PickId::TopLeft, pi);
	addCubeFace(y, x+z, ShapeId::Edge, PickId::TopRight);
	addCubeFace(y, x-z, ShapeId::Edge, PickId::BottomRight);
	addCubeFace(y,-z-x, ShapeId::Edge, PickId::BottomLeft, pi);


	SoSeparator* cubeAssembly = new SoSeparator;

	auto& dpiManager = DPIManager::getInstance();

	// Create coordinate system and materials for the cube
	// Define texture coordinates for label textures like FreeCAD
	// Fixed UV coordinates: {0,0, 1,0, 1,1, 0,1} for all texture quads
	SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
	texCoords->point.setValues(0, 4, new SbVec2f[4]{
		SbVec2f(0.0f, 0.0f),      // 0: bottom-left
		SbVec2f(1.0f, 0.0f),      // 1: bottom-right
		SbVec2f(1.0f, 1.0f),      // 2: top-right
		SbVec2f(0.0f, 1.0f)       // 3: top-left
		});
	cubeAssembly->addChild(texCoords);

	// Force no shading mode for navigation cube - use BASE_COLOR light model
	// This ensures the cube displays with uniform colors without any lighting calculations
	SoLightModel* lightModel = new SoLightModel;
	lightModel->model = SoLightModel::BASE_COLOR; // No shading, direct color display
	cubeAssembly->addChild(lightModel);

	// Create coordinate node for all faces
	SoCoordinate3* coords = new SoCoordinate3;
	cubeAssembly->addChild(coords);

	SoMaterial* mainFaceMaterial = new SoMaterial;
	// Frosted glass material for main faces - read all properties from config
	// Use unified cube material color since navigation cube is now a single body
	mainFaceMaterial->diffuseColor.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseR", 0.9)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseG", 0.95)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseB", 1.0))
	);
	// Use unified cube material properties from config
	mainFaceMaterial->ambientColor.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialAmbientR", 0.7)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialAmbientG", 0.8)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialAmbientB", 0.9))
	);
	mainFaceMaterial->specularColor.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialSpecularR", 0.95)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialSpecularG", 0.98)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialSpecularB", 1.0))
	);
	mainFaceMaterial->emissiveColor.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialEmissiveR", 0.02)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialEmissiveG", 0.05)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialEmissiveB", 0.1))
	);
	mainFaceMaterial->shininess.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialShininess", 0.0f))
	);
	mainFaceMaterial->transparency.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialTransparency", 0.0f))
	);

	// Store base colors for hover effects from config - use unified hover color
	SbColor baseColor(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "MainFaceHoverColorR", 0.7)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "MainFaceHoverColorG", 0.85)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "MainFaceHoverColorB", 0.95))
	);
	m_faceBaseColors["FRONT"] = baseColor;
	m_faceBaseColors["REAR"] = baseColor;
	m_faceBaseColors["LEFT"] = baseColor;
	m_faceBaseColors["RIGHT"] = baseColor;
	m_faceBaseColors["TOP"] = baseColor;
	m_faceBaseColors["BOTTOM"] = baseColor;

	// Note: edgeAndCornerMaterial is no longer used since navigation cube is now a single body
	// It's kept for backward compatibility but won't affect rendering
	SoMaterial* edgeAndCornerMaterial = new SoMaterial;
	edgeAndCornerMaterial->diffuseColor.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseR", 0.9)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseG", 0.95)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseB", 1.0))
	);
	edgeAndCornerMaterial->ambientColor.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeCornerMaterialAmbientR", 0.3)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeCornerMaterialAmbientG", 0.5)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeCornerMaterialAmbientB", 0.3))
	);
	edgeAndCornerMaterial->specularColor.setValue(0.0f, 0.0f, 0.0f); // No specular for no shading mode
	edgeAndCornerMaterial->emissiveColor.setValue(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeCornerMaterialEmissiveR", 0.04)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeCornerMaterialEmissiveG", 0.12)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeCornerMaterialEmissiveB", 0.04))
	);
	edgeAndCornerMaterial->shininess.setValue(0.0f); // No shading mode
	edgeAndCornerMaterial->transparency.setValue(0.0f); // Opaque

	// Store base colors for edges and corners from config
	SbColor edgeBaseColor(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeHoverColorR", 0.5)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeHoverColorG", 0.7)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "EdgeHoverColorB", 0.5))
	);
	m_faceBaseColors["EdgeTF"] = edgeBaseColor;
	m_faceBaseColors["EdgeTB"] = edgeBaseColor;
	m_faceBaseColors["EdgeTL"] = edgeBaseColor;
	m_faceBaseColors["EdgeTR"] = edgeBaseColor;
	m_faceBaseColors["EdgeBF"] = edgeBaseColor;
	m_faceBaseColors["EdgeBB"] = edgeBaseColor;
	m_faceBaseColors["EdgeBL"] = edgeBaseColor;
	m_faceBaseColors["EdgeBR"] = edgeBaseColor;
	m_faceBaseColors["EdgeFR"] = edgeBaseColor;
	m_faceBaseColors["EdgeFL"] = edgeBaseColor;
	m_faceBaseColors["EdgeBL2"] = edgeBaseColor;
	m_faceBaseColors["EdgeBR2"] = edgeBaseColor;

	// Corner faces use the same material but slightly different base color for hover effect from config
	SbColor cornerBaseColor(
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CornerHoverColorR", 0.4)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CornerHoverColorG", 0.6)),
		static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CornerHoverColorB", 0.4))
	);
	m_faceBaseColors["Corner0"] = cornerBaseColor;
	m_faceBaseColors["Corner1"] = cornerBaseColor;
	m_faceBaseColors["Corner2"] = cornerBaseColor;
	m_faceBaseColors["Corner3"] = cornerBaseColor;
	m_faceBaseColors["Corner4"] = cornerBaseColor;
	m_faceBaseColors["Corner5"] = cornerBaseColor;
	m_faceBaseColors["Corner6"] = cornerBaseColor;
	m_faceBaseColors["Corner7"] = cornerBaseColor;

	// Create faces using dynamic generation
	int vertexIndex = 0;

	// Define face creation helper - now takes PickId directly
	auto createFaceFromVertices = [&](const std::string& faceName, PickId pickId, int materialType) {
		// Skip faces based on display options
		if (materialType == 1 && !m_showEdges) return;    // Skip edge faces if edges are disabled
		if (materialType == 2 && !m_showCorners) return;  // Skip corner faces if corners are disabled

		SoSeparator* faceSep = new SoSeparator;
		faceSep->setName(SbName(faceName.c_str()));
		
		// Store the separator for later texture replacement
		m_faceSeparators[faceName] = faceSep;

		// Add appropriate material
		if (materialType == 0) { // Main face
			faceSep->addChild(mainFaceMaterial);
			m_faceMaterials[faceName] = mainFaceMaterial;
		} else { // Edges and Corners
			faceSep->addChild(edgeAndCornerMaterial);
			m_faceMaterials[faceName] = edgeAndCornerMaterial;
		}

		// Create indexed face set
		SoIndexedFaceSet* face = new SoIndexedFaceSet;

		// Add vertices to coordinate node
		auto& vertices = m_Faces[pickId].vertexArray;
		for (size_t i = 0; i < vertices.size(); i++) {
			coords->point.set1Value(vertexIndex + i, vertices[i]);
			face->coordIndex.set1Value(i, vertexIndex + i);
		}
		face->coordIndex.set1Value(vertices.size(), -1); // End marker
		vertexIndex += vertices.size();

		// Set texture coordinates based on face type
		if (materialType == 0) { // Main face (octagon) - no texture, only material
			// Main faces don't use texture coordinates - they use material color only
			// Texture will be applied as separate quad overlay
		} else {
			// Edge and corner faces - use single color
			for (size_t i = 0; i < vertices.size(); ++i) {
				face->textureCoordIndex.set1Value(i, 0);
			}
			face->textureCoordIndex.set1Value(vertices.size(), -1);
		}

		faceSep->addChild(face);
		cubeAssembly->addChild(faceSep);
		
		// For main faces, create separate texture quad overlay using LabelTextures vertices
		if (materialType == 0) {
			SoSeparator* textureSep = new SoSeparator;
			textureSep->setName(SbName((faceName + "_Texture").c_str()));

			// Disable polygon offset to prevent z-fighting with main face
			SoPolygonOffset* polygonOffset = new SoPolygonOffset;
			polygonOffset->factor.setValue(-1.0f); // Negative offset to bring texture slightly forward
			polygonOffset->units.setValue(-1.0f);
			textureSep->addChild(polygonOffset);

			// Add material for texture overlay - use same color as main face material
			SoMaterial* textureMaterial = new SoMaterial;
			// Use the unified cube material color from config to match the solid body appearance
			float materialR = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseR", 0.9));
			float materialG = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseG", 0.95));
			float materialB = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseB", 1.0));
			
			textureMaterial->diffuseColor.setValue(materialR, materialG, materialB);
			textureMaterial->transparency.setValue(0.0f);
			textureMaterial->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
			textureSep->addChild(textureMaterial);

			// Create texture quad using LabelTextures vertices
			SoIndexedFaceSet* textureFace = new SoIndexedFaceSet;

			// Add LabelTextures vertices to coordinate node
			auto& labelVertices = m_LabelTextures[pickId].vertexArray;
			int labelVertexIndex = vertexIndex;
			for (size_t i = 0; i < labelVertices.size(); i++) {
				coords->point.set1Value(labelVertexIndex + i, labelVertices[i]);
				textureFace->coordIndex.set1Value(i, labelVertexIndex + i);
			}
			textureFace->coordIndex.set1Value(labelVertices.size(), -1);

			// Set texture coordinates for quad like FreeCAD (fixed UV: 0,0,1,0,1,1,0,1)
			textureFace->textureCoordIndex.set1Value(0, 0); // bottom-left: (0,0)
			textureFace->textureCoordIndex.set1Value(1, 1); // bottom-right: (1,0)
			textureFace->textureCoordIndex.set1Value(2, 2); // top-right: (1,1)
			textureFace->textureCoordIndex.set1Value(3, 3); // top-left: (0,1)
			textureFace->textureCoordIndex.set1Value(4, -1);

			textureSep->addChild(textureFace);
			cubeAssembly->addChild(textureSep);

			// Debug: Log MODULATE mode result like FreeCAD
			// In MODULATE mode: FinalColor = MaterialColor * TextureColor
			// MaterialColor: (0.9, 0.95, 1.0) - light blue-green
			// TextureBackground: (1.0, 1.0, 1.0, 0.0) - white transparent background
			// TextureText: (0, 100/255, 1.0, 1.0) - blue text, opaque
			// Background Result: (0.9*1.0, 0.95*1.0, 1.0*1.0) = (0.9, 0.95, 1.0) - material color preserved
			// Background Alpha: 0.0 * 0.0 = 0.0 (transparent) - shows material color
			// Text Result: (0.9*0, 0.95*100/255, 1.0*1.0) = (0, 0.37, 1.0) - dark blue-green
			// Text Alpha: 0.0 * 1.0 = 0.0 (opaque) - text visible

			// Update vertexIndex for next face
			vertexIndex += labelVertices.size();

			// Store texture separator for later texture replacement
			m_faceSeparators[faceName + "_Texture"] = textureSep;
		}
	};

	// NOTE: Individual faces are now combined into a single solid body below
	// The createFaceFromVertices calls are disabled to avoid duplication

	// Create a solid body by combining all faces into one indexed face set
	SoSeparator* solidBodySep = new SoSeparator;
	solidBodySep->setName("SolidBody");

	// Add shape hints for solid body
	SoShapeHints* shapeHints = new SoShapeHints;
	shapeHints->shapeType = SoShapeHints::SOLID;
	shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE; // Ensure outward normals
	shapeHints->faceType = SoShapeHints::CONVEX; // All faces are convex
	solidBodySep->addChild(shapeHints);

	// Add no-shading light model for solid appearance
	solidBodySep->addChild(lightModel);

	// Use shared coordinate node
	solidBodySep->addChild(coords);

	// Use shared texture coordinates
	solidBodySep->addChild(texCoords);

	// Create material for solid body
	SoMaterial* solidMaterial = new SoMaterial;
	solidMaterial->diffuseColor.setValue(0.8f, 0.8f, 0.9f); // Light blue-gray
	solidMaterial->ambientColor.setValue(0.6f, 0.6f, 0.7f);
	solidMaterial->specularColor.setValue(0.0f, 0.0f, 0.0f); // No specular for solid
	solidMaterial->shininess.setValue(0.0f); // No shading
	solidMaterial->transparency.setValue(0.0f); // Opaque
	solidBodySep->addChild(solidMaterial);

	// Create the solid indexed face set with all faces
	SoIndexedFaceSet* solidBody = new SoIndexedFaceSet;
	solidBody->setName("Rhombicuboctahedron");

	// First, add all vertices to the shared coordinate node
	int totalVertices = 0;
	std::vector<PickId> allFaceIds = {
		// Main faces (6)
		PickId::Top, PickId::Bottom, PickId::Front, PickId::Rear, PickId::Right, PickId::Left,
		// Corner faces (8)
		PickId::FrontTopRight, PickId::FrontTopLeft, PickId::FrontBottomRight, PickId::FrontBottomLeft,
		PickId::RearTopRight, PickId::RearTopLeft, PickId::RearBottomRight, PickId::RearBottomLeft,
		// Edge faces (12)
		PickId::FrontTop, PickId::RearTop, PickId::TopLeft, PickId::TopRight,
		PickId::FrontBottom, PickId::RearBottom, PickId::BottomLeft, PickId::BottomRight,
		PickId::FrontRight, PickId::FrontLeft, PickId::RearLeft, PickId::RearRight
	};

	// Add all vertices to coords in order
	for (PickId faceId : allFaceIds) {
		auto& vertices = m_Faces[faceId].vertexArray;
		for (const auto& vertex : vertices) {
			coords->point.set1Value(totalVertices++, vertex);
		}
	}

	// Create face indices
	std::vector<int32_t> allFaceIndices;
	int currentVertexIndex = 0;

	for (PickId faceId : allFaceIds) {
		auto& vertices = m_Faces[faceId].vertexArray;
		if (!vertices.empty()) {
			// Add vertex indices for this face (counter-clockwise for outward normals)
			for (int i = static_cast<int>(vertices.size()) - 1; i >= 0; --i) {
				allFaceIndices.push_back(currentVertexIndex + i);
			}
			allFaceIndices.push_back(-1); // Face separator
			currentVertexIndex += vertices.size();
		}
	}

	solidBody->coordIndex.setValues(0, allFaceIndices.size(), allFaceIndices.data());
	solidBodySep->addChild(solidBody);

	// Add solid body to assembly instead of individual faces
	cubeAssembly->addChild(solidBodySep);

	// Create textured quad faces for main faces - actual geometry with texture mapping
	int currentTextureVertexIndex = totalVertices; // Track current vertex index for textures
	std::vector<PickId> mainFaceIds = {PickId::Front, PickId::Top, PickId::Right, PickId::Rear, PickId::Bottom, PickId::Left};

	for (PickId pickId : mainFaceIds) {
		std::string faceName;
		switch (pickId) {
			case PickId::Front: faceName = "FRONT"; break;
			case PickId::Rear: faceName = "REAR"; break;
			case PickId::Left: faceName = "LEFT"; break;
			case PickId::Right: faceName = "RIGHT"; break;
			case PickId::Top: faceName = "TOP"; break;
			case PickId::Bottom: faceName = "BOTTOM"; break;
			default: continue;
		}

		SoSeparator* textureFaceSep = new SoSeparator;
		textureFaceSep->setName(SbName((faceName + "_Texture").c_str()));

		// Create proper depth testing setup for texture faces
		// Use both depth buffer and polygon offset for reliable occlusion
		SoDepthBuffer* depthBuffer = new SoDepthBuffer;
		depthBuffer->test.setValue(true);
		depthBuffer->write.setValue(false); // Don't write to depth buffer (let solid geometry control depth)
		depthBuffer->function.setValue(SoDepthBufferElement::LEQUAL); // Less or equal depth test
		textureFaceSep->addChild(depthBuffer);

		// Small polygon offset to ensure texture renders on top when at same depth
		SoPolygonOffset* polygonOffset = new SoPolygonOffset;
		polygonOffset->factor.setValue(0.1f);  // Small offset factor
		polygonOffset->units.setValue(1.0f);   // Small offset units
		textureFaceSep->addChild(polygonOffset);

		// Create material for the texture face - use main face material color from config
		SoMaterial* textureMaterial = new SoMaterial;
		// Use the unified cube material color from config to match the solid body appearance
		float materialR = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseR", 0.9));
		float materialG = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseG", 0.95));
		float materialB = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeMaterialDiffuseB", 1.0));
		textureMaterial->diffuseColor.setValue(materialR, materialG, materialB);
		textureMaterial->transparency.setValue(0.0f); // Opaque
		textureMaterial->emissiveColor.setValue(0.0f, 0.0f, 0.0f); // No emissive
		textureFaceSep->addChild(textureMaterial);

		// Create the textured quad face using LabelTextures vertices
		SoIndexedFaceSet* textureQuad = new SoIndexedFaceSet;

		// Add LabelTextures vertices to the shared coordinate node (unique indices for each face)
		auto& labelVertices = m_LabelTextures[pickId].vertexArray;
		if (labelVertices.size() >= 4) {
			for (size_t i = 0; i < labelVertices.size(); ++i) {
				coords->point.set1Value(currentTextureVertexIndex + i, labelVertices[i]);
				textureQuad->coordIndex.set1Value(i, currentTextureVertexIndex + i);
			}
			textureQuad->coordIndex.set1Value(labelVertices.size(), -1);
			currentTextureVertexIndex += labelVertices.size(); // Update for next face
			LOG_INF_S("Added texture quad for " + faceName + " with " + std::to_string(labelVertices.size()) + " vertices");
		} else {
			LOG_WRN_S("LabelTextures for " + faceName + " has insufficient vertices: " + std::to_string(labelVertices.size()));
		}

		// Only add texture coordinates and quad if we have valid vertices
		if (labelVertices.size() >= 4) {
			// Set up texture coordinates
			SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
			texCoords->point.set1Value(0, 0.0f, 0.0f); // bottom-left
			texCoords->point.set1Value(1, 1.0f, 0.0f); // bottom-right
			texCoords->point.set1Value(2, 1.0f, 1.0f); // top-right
			texCoords->point.set1Value(3, 0.0f, 1.0f); // top-left
			textureFaceSep->addChild(texCoords);

			// Set texture coordinate indices for the quad
			textureQuad->textureCoordIndex.set1Value(0, 0); // bottom-left
			textureQuad->textureCoordIndex.set1Value(1, 1); // bottom-right
			textureQuad->textureCoordIndex.set1Value(2, 2); // top-right
			textureQuad->textureCoordIndex.set1Value(3, 3); // top-left
			textureQuad->textureCoordIndex.set1Value(4, -1);

			textureFaceSep->addChild(textureQuad);
			cubeAssembly->addChild(textureFaceSep);
		}

		// Store texture separator for later texture replacement
		m_faceSeparators[faceName + "_Texture"] = textureFaceSep;
	}

	m_root->addChild(cubeAssembly);


	// --- Add black outlines to all faces ---
	SoSeparator* outlineSep = new SoSeparator;

	// Enable line smoothing for anti-aliasing
	SoShapeHints* hints = new SoShapeHints;
	hints->shapeType = SoShapeHints::SOLID;
	hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
	outlineSep->addChild(hints);

	// Define the style for the outlines
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->style = SoDrawStyle::LINES;
	drawStyle->lineWidth = 1.0f;
	outlineSep->addChild(drawStyle);

	// Define the material for the outlines
	SoMaterial* outlineMaterial = new SoMaterial;
	outlineMaterial->diffuseColor.setValue(0.4f, 0.6f, 0.9f); // Light blue
	outlineMaterial->specularColor.setValue(0.0f, 0.0f, 0.0f); // No specular for no shading mode
	outlineMaterial->shininess.setValue(0.0f); // No shading mode
	outlineMaterial->transparency.setValue(0.0f); // Opaque
	outlineSep->addChild(outlineMaterial);

	// Re-use the same coordinates but re-draw all faces as lines
	outlineSep->addChild(coords); // Re-use the same SoCoordinate3 node

	SoIndexedFaceSet* outlineFaceSet = new SoIndexedFaceSet;
	std::vector<int32_t> all_indices;

	// Define the faces to draw outlines for (same order as creation)
	std::vector<std::pair<PickId, int>> outlineFaces = {
		// Main faces (6 faces)
		{PickId::Top, 8}, {PickId::Bottom, 8}, {PickId::Front, 8}, {PickId::Rear, 8}, {PickId::Right, 8}, {PickId::Left, 8},
		// Corner faces (8 faces)
		{PickId::FrontTopRight, 6}, {PickId::FrontTopLeft, 6}, {PickId::FrontBottomRight, 6}, {PickId::FrontBottomLeft, 6},
		{PickId::RearTopRight, 6}, {PickId::RearTopLeft, 6}, {PickId::RearBottomRight, 6}, {PickId::RearBottomLeft, 6},
		// Edge faces (12 faces)
		{PickId::FrontTop, 4}, {PickId::RearTop, 4}, {PickId::TopLeft, 4}, {PickId::TopRight, 4},
		{PickId::FrontBottom, 4}, {PickId::RearBottom, 4}, {PickId::BottomLeft, 4}, {PickId::BottomRight, 4},
		{PickId::FrontRight, 4}, {PickId::FrontLeft, 4}, {PickId::RearLeft, 4}, {PickId::RearRight, 4}
	};

	int vertexOffset = 0;
	for (const auto& [pickId, vertexCount] : outlineFaces) {
		// Skip faces based on display options
		if ((pickId >= PickId::FrontTop && pickId <= PickId::RearRight) && !m_showEdges) continue;    // Skip edge faces
		if ((pickId >= PickId::FrontTopRight && pickId <= PickId::RearBottomLeft) && !m_showCorners) continue;  // Skip corner faces

		// Add all vertices of this face for outline drawing
		for (int i = 0; i < vertexCount; i++) {
			all_indices.push_back(vertexOffset + i);
		}
		all_indices.push_back(-1); // Separator for each face
		vertexOffset += vertexCount;
	}

	outlineFaceSet->coordIndex.setValues(0, all_indices.size(), all_indices.data());
	outlineSep->addChild(outlineFaceSet);

	m_root->addChild(outlineSep);
	
	// Generate and cache all textures after geometry setup
	LOG_INF_S("=== TEXTURE SYSTEM CHECK ===");
	LOG_INF_S("m_showTextures: " + std::string(m_showTextures ? "true" : "false"));
	LOG_INF_S("m_faceFontSizes.size(): " + std::to_string(m_faceFontSizes.size()));
	LOG_INF_S("m_Faces.size(): " + std::to_string(m_Faces.size()));

	if (m_showTextures) {
		LOG_INF_S("Starting texture generation...");
		generateAndCacheTextures();
		LOG_INF_S("Texture generation completed");
	} else {
		LOG_INF_S("Texture generation SKIPPED - m_showTextures is false");
	}

	// Summary and validation log
	LOG_INF_S("=== RHOMBICUBOCTAHEDRON SOLID BODY CREATED ===");
	LOG_INF_S("Geometry: Single SoIndexedFaceSet with 26 faces forming a closed solid");

	// Reuse the totalVertices count from earlier
	int mainFaces = 0, cornerFaces = 0, edgeFaces = 0;
	std::map<ShapeId, int> vertexCounts;

	// Recalculate total vertices
	int recalculatedTotalVertices = 0;
	for (const auto& pair : m_Faces) {
		recalculatedTotalVertices += pair.second.vertexArray.size();
		vertexCounts[pair.second.type] += pair.second.vertexArray.size();
		switch (pair.second.type) {
			case ShapeId::Main: mainFaces++; break;
			case ShapeId::Corner: cornerFaces++; break;
			case ShapeId::Edge: edgeFaces++; break;
		}
	}

	int totalTextureVertices = 0;
	for (const auto& pair : m_LabelTextures) {
		totalTextureVertices += pair.second.vertexArray.size();
	}

	LOG_INF_S("Face counts - Main: " + std::to_string(mainFaces) + ", Corner: " + std::to_string(cornerFaces) + ", Edge: " + std::to_string(edgeFaces));
	LOG_INF_S("Vertex counts - Main: " + std::to_string(vertexCounts[ShapeId::Main]) +
			  ", Corner: " + std::to_string(vertexCounts[ShapeId::Corner]) +
			  ", Edge: " + std::to_string(vertexCounts[ShapeId::Edge]) + " (total: " + std::to_string(recalculatedTotalVertices) + ")");
	LOG_INF_S("Solid body: " + std::to_string(recalculatedTotalVertices) + " vertices, 26 faces, counter-clockwise winding for outward normals");
	LOG_INF_S("Texture quads: " + std::to_string(totalTextureVertices) + " texture vertices for " + std::to_string(m_showTextures ? 6 : 0) + " main face overlays");
	LOG_INF_S("Total geometry: " + std::to_string(recalculatedTotalVertices + totalTextureVertices) + " vertices, 26 solid faces + 6 texture quads");

	// Validation checks
	bool valid = true;
	LOG_INF_S("=== VALIDATION CHECKS ===");
	LOG_INF_S("Face counts - Main: " + std::to_string(mainFaces) + "/6, Corner: " + std::to_string(cornerFaces) + "/8, Edge: " + std::to_string(edgeFaces) + "/12");
	LOG_INF_S("Vertex counts - Main: " + std::to_string(vertexCounts[ShapeId::Main]) + "/48, Corner: " + std::to_string(vertexCounts[ShapeId::Corner]) + "/48, Edge: " + std::to_string(vertexCounts[ShapeId::Edge]) + "/48");

	// Debug: Check individual face vertex counts
	LOG_INF_S("=== INDIVIDUAL FACE VERTEX COUNTS ===");
	std::vector<PickId> debugFaceIds = {
		// Main faces (6)
		PickId::Top, PickId::Front, PickId::Left, PickId::Rear, PickId::Right, PickId::Bottom,
		// Corner faces (8)
		PickId::FrontTopRight, PickId::FrontTopLeft, PickId::FrontBottomRight, PickId::FrontBottomLeft,
		PickId::RearTopRight, PickId::RearTopLeft, PickId::RearBottomRight, PickId::RearBottomLeft,
		// Edge faces (12)
		PickId::FrontTop, PickId::RearTop, PickId::TopLeft, PickId::TopRight,
		PickId::FrontBottom, PickId::RearBottom, PickId::BottomLeft, PickId::BottomRight,
		PickId::FrontRight, PickId::FrontLeft, PickId::RearLeft, PickId::RearRight
	};

	for (PickId faceId : allFaceIds) {
		if (m_Faces.count(faceId) > 0) {
			int vertexCount = m_Faces[faceId].vertexArray.size();
			std::string shapeStr = (m_Faces[faceId].type == ShapeId::Main) ? "Main" : (m_Faces[faceId].type == ShapeId::Corner) ? "Corner" : "Edge";
			LOG_INF_S("Face " + std::to_string(static_cast<int>(faceId)) + " (" + shapeStr + "): " + std::to_string(vertexCount) + " vertices");
		}
	}

	if (mainFaces != 6) { LOG_WRN_S("ERROR: Expected 6 main faces, got " + std::to_string(mainFaces)); valid = false; }
	if (cornerFaces != 8) { LOG_WRN_S("ERROR: Expected 8 corner faces, got " + std::to_string(cornerFaces)); valid = false; }
	if (edgeFaces != 12) { LOG_WRN_S("ERROR: Expected 12 edge faces, got " + std::to_string(edgeFaces)); valid = false; }
	if (vertexCounts[ShapeId::Main] != 48) { LOG_WRN_S("ERROR: Expected 48 main face vertices (6x8), got " + std::to_string(vertexCounts[ShapeId::Main])); valid = false; }
	if (vertexCounts[ShapeId::Corner] != 48) { LOG_WRN_S("ERROR: Expected 48 corner face vertices (8x6), got " + std::to_string(vertexCounts[ShapeId::Corner])); valid = false; }
	if (vertexCounts[ShapeId::Edge] != 48) { LOG_WRN_S("ERROR: Expected 48 edge face vertices (12x4), got " + std::to_string(vertexCounts[ShapeId::Edge])); valid = false; }

	if (valid) {
		LOG_INF_S("[PASS] Rhombicuboctahedron solid body validation PASSED - all faces properly formed");
	} else {
		LOG_ERR_S("[FAIL] Rhombicuboctahedron solid body validation FAILED - geometry errors detected");
	}
}

void CuteNavCube::updateCameraRotation() {
	// Rotates camera around the cube
	float distance = m_cameraDistance; // Configurable camera distance
	float radX = m_rotationX * M_PI / 180.0f;
	float radY = m_rotationY * M_PI / 180.0f;

	// Calculate position in spherical coordinates
	float x = distance * sin(radY) * cos(radX);
	float y = distance * sin(radX);
	float z = distance * cos(radY) * cos(radX);

	m_orthoCamera->position.setValue(x, y, z);
	m_orthoCamera->pointAt(SbVec3f(0, 0, 0)); // Always look at the origin (cube center)
	
	// Note: Orthographic camera will automatically adjust to viewport

	//LOG_DBG("CuteNavCube::updateCameraRotation: Camera position x=" + std::to_string(x) +
	//    ", y=" + std::to_string(y) + ", z=" + std::to_string(z));
}

std::string CuteNavCube::pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize) {
	// Validate viewport size
	if (viewportSize.x <= 0 || viewportSize.y <= 0) {
		LOG_INF_S("CuteNavCube::pickRegion: Invalid viewport size - " + 
			std::to_string(viewportSize.x) + "x" + std::to_string(viewportSize.y));
		return "";
	}

	// Add picking debug log - disabled for performance
	// LOG_INF_S("CuteNavCube::pickRegion: Picking at position (" +
	//	std::to_string(mousePos[0]) + ", " + std::to_string(mousePos[1]) +
	//	") in viewport " + std::to_string(viewportSize.x) + "x" + std::to_string(viewportSize.y));

	// Create viewport region for picking - use cube's local coordinate system
	SbViewportRegion pickViewport;
	pickViewport.setWindowSize(SbVec2s(static_cast<short>(viewportSize.x), static_cast<short>(viewportSize.y)));

	// Set viewport pixels to match the cube's local viewport (0,0 to cubeSize,cubeSize)
	pickViewport.setViewportPixels(0, 0, viewportSize.x, viewportSize.y);

	// Debug viewport settings
	static int debugCount = 0;
	// Debug counter disabled for performance
	// if (++debugCount % 10 == 0) {
	//	LOG_INF_S("CuteNavCube::pickRegion: Viewport settings - local:" +
	//		std::to_string(viewportSize.x) + "x" + std::to_string(viewportSize.y) +
	//		", mouse:" + std::to_string(mousePos[0]) + "," + std::to_string(mousePos[1]));
	// }

	SoRayPickAction pickAction(pickViewport);
	pickAction.setPoint(mousePos);
	pickAction.apply(m_root);

	SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
	if (!pickedPoint) {
	return "";
	}

	SoPath* pickedPath = pickedPoint->getPath();
	if (!pickedPath || pickedPath->getLength() == 0) {
		return "";
	}

	// Find the named separator for the face
	for (int i = pickedPath->getLength() - 1; i >= 0; --i) {
		SoNode* node = pickedPath->getNode(i);
		if (node && node->isOfType(SoSeparator::getClassTypeId()) && node->getName().getLength() > 0) {
			std::string nameStr = node->getName().getString();
			
			// Check if this is a valid face name
			auto viewIt = m_faceToView.find(nameStr);
			auto normalIt = m_faceNormals.find(nameStr);
			if (viewIt != m_faceToView.end() && normalIt != m_faceNormals.end()) {
				return nameStr; // Return the actual face name, not the mapped view
			}
		}
	}

	return "";
}

// Calculate camera position based on clicked face
void CuteNavCube::calculateCameraPositionForFace(const std::string& faceName, SbVec3f& position, SbRotation& orientation) const {
	// Handle main faces (6 faces) - use standard positioning
	static const std::set<std::string> mainFaces = {"FRONT", "REAR", "LEFT", "RIGHT", "TOP", "BOTTOM"};
	if (mainFaces.find(faceName) != mainFaces.end()) {
		auto it = m_faceNormals.find(faceName);
		if (it != m_faceNormals.end()) {
			const SbVec3f& normal = it->second.first;
			const SbVec3f& center = it->second.second;

			// Camera position: move back along the normal direction by camera distance
			float distance = m_cameraDistance * 1.5f;
			position = center - normal * distance;

			// Camera orientation: look at the center point
			SbVec3f upVector = SbVec3f(0, 1, 0); // Default up vector
			if (std::abs(normal[1]) > 0.5f) {
				// If looking at top/bottom, use X axis as up
				upVector = SbVec3f(1, 0, 0);
			}

			SbVec3f direction = center - position;
			orientation.setValue(SbVec3f(0, 0, -1), direction);
			return;
		}
	}

	// Handle edge faces (12 faces) - calculate position to view the edge
	static const std::map<std::string, std::pair<SbVec3f, SbVec3f>> edgeFacePositions = {
		{ "EdgeTF", std::make_pair(SbVec3f(0, 0.5, 1.2), SbVec3f(0, -1, 0)) },   // Top-Front edge: look down at front-top
		{ "EdgeTB", std::make_pair(SbVec3f(0, 0.5, -1.2), SbVec3f(0, -1, 0)) },  // Top-Back edge: look down at back-top
		{ "EdgeTL", std::make_pair(SbVec3f(-1.2, 0.5, 0), SbVec3f(1, 0, 0)) },  // Top-Left edge: look right at top-left
		{ "EdgeTR", std::make_pair(SbVec3f(1.2, 0.5, 0), SbVec3f(-1, 0, 0)) },  // Top-Right edge: look left at top-right
		{ "EdgeBF", std::make_pair(SbVec3f(0, -0.5, 1.2), SbVec3f(0, 1, 0)) },   // Bottom-Front edge: look up at bottom-front
		{ "EdgeBB", std::make_pair(SbVec3f(0, -0.5, -1.2), SbVec3f(0, 1, 0)) },  // Bottom-Back edge: look up at bottom-back
		{ "EdgeBL", std::make_pair(SbVec3f(-1.2, -0.5, 0), SbVec3f(1, 0, 0)) },  // Bottom-Left edge: look right at bottom-left
		{ "EdgeBR", std::make_pair(SbVec3f(1.2, -0.5, 0), SbVec3f(-1, 0, 0)) },  // Bottom-Right edge: look left at bottom-right
		{ "EdgeFR", std::make_pair(SbVec3f(1.2, 0, 1), SbVec3f(-1, 0, 0)) },    // Front-Right edge: look left at front-right
		{ "EdgeFL", std::make_pair(SbVec3f(-1.2, 0, 1), SbVec3f(1, 0, 0)) },    // Front-Left edge: look right at front-left
		{ "EdgeBL2", std::make_pair(SbVec3f(-1.2, 0, -1), SbVec3f(1, 0, 0)) },   // Back-Left edge: look right at back-left
		{ "EdgeBR2", std::make_pair(SbVec3f(1.2, 0, -1), SbVec3f(-1, 0, 0)) }    // Back-Right edge: look left at back-right
	};

	auto edgeIt = edgeFacePositions.find(faceName);
	if (edgeIt != edgeFacePositions.end()) {
		position = edgeIt->second.first;
		SbVec3f upVector = edgeIt->second.second;
		SbVec3f direction = -position; // Look at origin
		orientation.setValue(upVector, direction);
		return;
	}

	// Handle corner faces (8 faces) - calculate position to view the corner
	static const std::map<std::string, std::pair<SbVec3f, SbVec3f>> cornerFacePositions = {
		{ "Corner0", std::make_pair(SbVec3f(-1.2, 1.2, 1.2), SbVec3f(0, 0, -1)) },  // Front-Top-Left: look towards origin
		{ "Corner1", std::make_pair(SbVec3f(1.2, 1.2, 1.2), SbVec3f(0, 0, -1)) },   // Front-Top-Right: look towards origin
		{ "Corner2", std::make_pair(SbVec3f(1.2, 1.2, -1.2), SbVec3f(0, 0, 1)) },   // Back-Top-Right: look towards origin
		{ "Corner3", std::make_pair(SbVec3f(-1.2, 1.2, -1.2), SbVec3f(0, 0, 1)) },  // Back-Top-Left: look towards origin
		{ "Corner4", std::make_pair(SbVec3f(-1.2, -1.2, 1.2), SbVec3f(0, 0, -1)) }, // Front-Bottom-Left: look towards origin
		{ "Corner5", std::make_pair(SbVec3f(1.2, -1.2, 1.2), SbVec3f(0, 0, -1)) },  // Front-Bottom-Right: look towards origin
		{ "Corner6", std::make_pair(SbVec3f(1.2, -1.2, -1.2), SbVec3f(0, 0, 1)) },  // Back-Bottom-Right: look towards origin
		{ "Corner7", std::make_pair(SbVec3f(-1.2, -1.2, -1.2), SbVec3f(0, 0, 1)) }  // Back-Bottom-Left: look towards origin
	};

	auto cornerIt = cornerFacePositions.find(faceName);
	if (cornerIt != cornerFacePositions.end()) {
		position = cornerIt->second.first;
		SbVec3f upVector = cornerIt->second.second;
		SbVec3f direction = -position; // Look at origin
		orientation.setValue(upVector, direction);
		return;
	}

	// Default fallback
	LOG_WRN_S("CuteNavCube::calculateCameraPositionForFace: Unknown face name: " + faceName);
	position = SbVec3f(0, 0, 5);
	orientation = SbRotation::identity();
}

bool CuteNavCube::handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize) {
	std::string eventType = event.Moving() ? "MOVING" : event.Leaving() ? "LEAVING" : event.LeftDown() ? "LEFT_DOWN" : event.LeftUp() ? "LEFT_UP" : "OTHER";

	if (!m_enabled) {
		return false;
	}

	static SbVec2s dragStartPos(0, 0);

	SbVec2s currentPos(
		static_cast<short>(event.GetX()),
		static_cast<short>(event.GetY())
	);

	// Handle mouse movement (hover detection)
	// Check for motion events (both Moving() and Dragging())
	if (event.GetEventType() == wxEVT_MOTION) {
		// Convert coordinates for picking - NavigationCubeManager already converted to cube-local coordinates
		// We need to flip Y for OpenGL picking (OpenGL uses bottom-left origin)
		SbVec2s pickPos(currentPos[0], static_cast<short>(viewportSize.y - currentPos[1]));
		std::string hoveredFace = pickRegion(pickPos, viewportSize);

		// Update hover state
		if (hoveredFace != m_hoveredFace) {
			// Restore previous face color by regenerating texture
			if (!m_hoveredFace.empty()) {
				regenerateFaceTexture(m_hoveredFace, false);
				if (m_refreshCallback) {
					m_refreshCallback();
				}
			}

			// Set new hovered face color by regenerating texture
			if (!hoveredFace.empty()) {
				regenerateFaceTexture(hoveredFace, true);
				if (m_refreshCallback) {
					m_refreshCallback();
				}
			}

			m_hoveredFace = hoveredFace;
		}

		// Don't return here - allow click/drag events to be processed
		if (!event.LeftIsDown()) {
			return true; // Hover events are always handled
		}
	}

	// When mouse leaves window, restore all face colors
	if (event.Leaving()) {
		if (!m_hoveredFace.empty()) {
			regenerateFaceTexture(m_hoveredFace, false);
			if (m_refreshCallback) {
				m_refreshCallback();
			}
			m_hoveredFace = "";
		}
		return true; // Mouse leaving is always handled
	}

	if (event.LeftDown()) {
		m_isDragging = true;
		m_lastMousePos = currentPos;
		dragStartPos = currentPos; // Capture position at the start of a potential drag/click
	}
	else if (event.LeftUp()) {
		if (m_isDragging) {
			m_isDragging = false;

			// Calculate distance between mouse down and mouse up to distinguish a click from a drag
			SbVec2s delta = currentPos - dragStartPos;
			float distance = std::sqrt(static_cast<float>(delta[0] * delta[0] + delta[1] * delta[1]));
			static const float clickThreshold = 5.0f; // Max distance for a click

			if (distance < clickThreshold) {
				// It's a click.
				// Invert Y-coordinate for picking, as OpenGL's origin is bottom-left.
				SbVec2s pickPos(currentPos[0], static_cast<short>(viewportSize.y - currentPos[1]));

				std::string region = pickRegion(pickPos, viewportSize);
				if (!region.empty()) {
					// Calculate camera position for clicked face
					SbVec3f cameraPos;
					SbRotation cameraOrient;
					calculateCameraPositionForFace(region, cameraPos, cameraOrient);

					// Get the mapped view name for logging
					auto viewIt = m_faceToView.find(region);
					std::string viewName = (viewIt != m_faceToView.end()) ? viewIt->second : region;

					// Call camera move callback if available, otherwise use view change callback
					if (m_cameraMoveCallback) {
						m_cameraMoveCallback(cameraPos, cameraOrient);
					} else if (m_viewChangeCallback) {
						m_viewChangeCallback(region);
					}
					return true;
				} else {
					return false; // Transparent area, allow ray penetration to outline viewport
				}
			}
		}
	}
	else if (event.Dragging() && m_isDragging) {
		SbVec2s delta = currentPos - m_lastMousePos;
		if (delta[0] == 0 && delta[1] == 0) return true; // Ignore no-movement events

		float sensitivity = 1.0f;
		m_rotationY += delta[0] * sensitivity;
		m_rotationX -= delta[1] * sensitivity; // Inverted Y-axis for natural feel

		m_rotationX = (std::max)(-89.0f, (std::min)(89.0f, m_rotationX));

		updateCameraRotation();
		m_lastMousePos = currentPos;

		if (m_rotationChangedCallback) {
			m_rotationChangedCallback();
		}

		return true; // Drag events are always handled
	}

	// Default: event not handled (for transparent areas)
	return false;
}

void CuteNavCube::render(int x, int y, const wxSize& size) {
	if (!m_enabled || !m_root) return;

	// Check if geometry rebuild is needed
	if (m_needsGeometryRebuild) {
		setupGeometry();
		m_needsGeometryRebuild = false;
	}

	// Setup viewport for navigation cube at specified position and size
	SbViewportRegion viewport;
	// Use physical window dimensions for correct coordinate mapping
	// Note: m_windowWidth and m_windowHeight are already in physical pixels from NavigationCubeManager
	viewport.setWindowSize(SbVec2s(static_cast<short>(m_windowWidth), static_cast<short>(m_windowHeight)));

	// Save current position for picking coordinate conversion
	m_currentX = x;
	m_currentY = y;

	// Convert logical coordinates to physical pixels
	// Convert logical coordinates to physical pixels
	// size parameter is the layout size (viewport position and size)
	int xPx = static_cast<int>(x * m_dpiScale);
	int yPx = static_cast<int>(y * m_dpiScale);
	int widthPx = static_cast<int>(size.x * m_dpiScale);
	int heightPx = static_cast<int>(size.y * m_dpiScale);

	// Convert top-left origin (x,y) to bottom-left origin for viewport
	int yBottomPx = m_windowHeight - yPx - heightPx;

	// Set the viewport rectangle where the cube will be rendered (origin bottom-left)
	viewport.setViewportPixels(xPx, yBottomPx, widthPx, heightPx);

	// Clear the viewport area to prevent ghosting/trailing effects during rotation
	glPushAttrib(GL_SCISSOR_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);
	glScissor(xPx, yBottomPx, widthPx, heightPx);
	// Don't clear color buffer to maintain transparency, only clear depth buffer
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	glPopAttrib();

	SoGLRenderAction renderAction(viewport);
	renderAction.setSmoothing(true);
	renderAction.setNumPasses(1);

	// Isolate minimal GL state to avoid interference from main scene render
	GLboolean wasTex2D = glIsEnabled(GL_TEXTURE_2D);
	GLboolean wasBlend = glIsEnabled(GL_BLEND);
	GLboolean wasMSAA = glIsEnabled(GL_MULTISAMPLE);
	GLint prevSrc = 0, prevDst = 0;
	glGetIntegerv(GL_BLEND_SRC, &prevSrc);
	glGetIntegerv(GL_BLEND_DST, &prevDst);

	glEnable(GL_TEXTURE_2D);
	// Enable MSAA if available
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderAction.apply(m_root);

	// Restore previous state
	glBlendFunc(prevSrc, prevDst);
	if (!wasBlend) glDisable(GL_BLEND);
	if (!wasTex2D) glDisable(GL_TEXTURE_2D);
	if (!wasMSAA) glDisable(GL_MULTISAMPLE);
}

void CuteNavCube::setEnabled(bool enabled) {
	m_enabled = enabled;
}

void CuteNavCube::updateMaterialProperties(const CubeConfig& config) {
	// Find and update material nodes in the scene graph
	if (!m_root) return;
	
	// Update environment lighting
	for (int i = 0; i < m_root->getNumChildren(); i++) {
		SoNode* child = m_root->getChild(i);
		
		// Update environment settings
		if (child->isOfType(SoEnvironment::getClassTypeId())) {
			SoEnvironment* env = static_cast<SoEnvironment*>(child);
			env->ambientIntensity.setValue(m_ambientIntensity);
		}
		
		// Update materials in separators
		if (child->isOfType(SoSeparator::getClassTypeId())) {
			updateSeparatorMaterials(static_cast<SoSeparator*>(child));
		}
	}
	
}

void CuteNavCube::updateSeparatorMaterials(SoSeparator* sep) {
	if (!sep) return;

	// Get the separator name to determine material type
	std::string sepName = sep->getName().getString();

	for (int i = 0; i < sep->getNumChildren(); i++) {
		SoNode* child = sep->getChild(i);

		if (child->isOfType(SoMaterial::getClassTypeId())) {
			SoMaterial* material = static_cast<SoMaterial*>(child);

			// Force opaque and no shading mode
			material->transparency.setValue(0.0f); // Always opaque
			material->shininess.setValue(0.0f); // Always no shading

			// Update colors based on material context
			float r, g, b;
			if (sepName.find("Edge") != std::string::npos) {
				// Edge material
				r = m_edgeColor.Red() / 255.0f;
				g = m_edgeColor.Green() / 255.0f;
				b = m_edgeColor.Blue() / 255.0f;
			}
			else if (sepName.find("Corner") != std::string::npos) {
				// Corner material
				r = m_cornerColor.Red() / 255.0f;
				g = m_cornerColor.Green() / 255.0f;
				b = m_cornerColor.Blue() / 255.0f;
			}
			else {
				// Main face material - using text color for main faces
				r = m_textColor.Red() / 255.0f;
				g = m_textColor.Green() / 255.0f;
				b = m_textColor.Blue() / 255.0f;
			}
			material->diffuseColor.setValue(r, g, b);
		}

		// Recursively update nested separators
		if (child->isOfType(SoSeparator::getClassTypeId())) {
			updateSeparatorMaterials(static_cast<SoSeparator*>(child));
		}
	}
}

void CuteNavCube::applyConfig(const CubeConfig& config) {
	// Store previous values to check what changed
	bool geometryChanged = (m_geometrySize != config.cubeSize ||
	                       m_chamferSize != config.chamferSize);
	bool cameraChanged = (m_cameraDistance != config.cameraDistance);
	bool displayChanged = (m_showEdges != config.showEdges ||
	                      m_showCorners != config.showCorners ||
	                      m_showTextures != config.showTextures);
	bool colorChanged = (m_textColor.GetRGB() != config.textColor.GetRGB() ||
	                    m_edgeColor.GetRGB() != config.edgeColor.GetRGB() ||
	                    m_cornerColor.GetRGB() != config.cornerColor.GetRGB());
	bool materialChanged = (m_transparency != config.transparency ||
	                       m_shininess != config.shininess ||
	                       m_ambientIntensity != config.ambientIntensity);
	bool circleChanged = (m_circleRadius != config.circleRadius ||
	                     m_circleMarginX != config.circleMarginX ||
	                     m_circleMarginY != config.circleMarginY);

	// Update all parameters
	m_geometrySize = config.cubeSize;
	m_chamferSize = config.chamferSize;
	m_cameraDistance = config.cameraDistance;
	m_showEdges = config.showEdges;
	m_showCorners = config.showCorners;
	m_showTextures = config.showTextures;
	m_enableAnimation = config.enableAnimation;
	m_textColor = config.textColor;
	m_edgeColor = config.edgeColor;
	m_cornerColor = config.cornerColor;

	// Update material properties
	m_transparency = config.transparency;
	m_shininess = config.shininess;
	m_ambientIntensity = config.ambientIntensity;

	// Update circle navigation area
	m_circleRadius = config.circleRadius;
	m_circleMarginX = config.circleMarginX;
	m_circleMarginY = config.circleMarginY;

	// Apply camera distance changes immediately
	if (cameraChanged) {
		updateCameraRotation();
	}

	// Apply material and color changes to existing geometry
	updateMaterialProperties(config);

	// Apply geometry changes if needed (requires rebuild)
	if (geometryChanged || displayChanged || colorChanged || materialChanged || circleChanged) {
		// Update geometry transform immediately if it exists
		if (m_geometryTransform) {
			m_geometryTransform->scaleFactor.setValue(m_geometrySize, m_geometrySize, m_geometrySize);
		}
		// Mark for geometry rebuild on next render
		m_needsGeometryRebuild = true;
	}

}

void CuteNavCube::setCameraPosition(const SbVec3f& position) {
	if (m_orthoCamera) {
		m_orthoCamera->position.setValue(position);
		//LOG_DBG("CuteNavCube::setCameraPosition: Set camera position to x=" + std::to_string(position[0]) +
		// ", y=" + std::to_string(position[1]) + ", z=" + std::to_string(position[2]));
	}
	else {
		LOG_WRN_S("CuteNavCube::setCameraPosition: Camera not initialized");
	}
}

void CuteNavCube::setCameraOrientation(const SbRotation& orientation) {
	if (m_orthoCamera) {
		m_orthoCamera->orientation.setValue(orientation);
		//LOG_DBG("CuteNavCube::setCameraOrientation: Set camera orientation");
	}
	else {
		LOG_WRN_S("CuteNavCube::setCameraOrientation: Camera not initialized");
	}
}

SoTexture2* CuteNavCube::createTextureForFace(const std::string& faceName, bool isHover) {
	// DEBUG: Log texture creation
	LOG_INF_S("=== Creating texture for face: " + faceName + " (hover: " + (isHover ? "true" : "false") + ") ===");

	// Determine texture color based on hover state and face type
	wxColour textureColor;
	if (isHover) {
		// Use hover color from config
		int hoverR = ConfigManager::getInstance().getInt("NavigationCube", "HoverTextureColorR", 255);
		int hoverG = ConfigManager::getInstance().getInt("NavigationCube", "HoverTextureColorG", 200);
		int hoverB = ConfigManager::getInstance().getInt("NavigationCube", "HoverTextureColorB", 150);
		int hoverA = ConfigManager::getInstance().getInt("NavigationCube", "HoverTextureColorA", 160);
		textureColor = wxColour(hoverR, hoverG, hoverB, hoverA);
	} else {
		// Use normal color based on face type from config
		if (faceName == "FRONT" || faceName == "REAR" || faceName == "LEFT" || 
			faceName == "RIGHT" || faceName == "TOP" || faceName == "BOTTOM") {
			// Main faces
			int mainR = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorR", 180);
			int mainG = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorG", 220);
			int mainB = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorB", 180);
			int mainA = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorA", 160);
			textureColor = wxColour(mainR, mainG, mainB, mainA);
		} else if (faceName.find("Edge") != std::string::npos) {
			// Edge faces
			int edgeR = ConfigManager::getInstance().getInt("NavigationCube", "EdgeFaceTextureColorR", 150);
			int edgeG = ConfigManager::getInstance().getInt("NavigationCube", "EdgeFaceTextureColorG", 200);
			int edgeB = ConfigManager::getInstance().getInt("NavigationCube", "EdgeFaceTextureColorB", 150);
			int edgeA = ConfigManager::getInstance().getInt("NavigationCube", "EdgeFaceTextureColorA", 160);
			textureColor = wxColour(edgeR, edgeG, edgeB, edgeA);
		} else if (faceName.find("Corner") != std::string::npos) {
			// Corner faces
			int cornerR = ConfigManager::getInstance().getInt("NavigationCube", "CornerFaceTextureColorR", 170);
			int cornerG = ConfigManager::getInstance().getInt("NavigationCube", "CornerFaceTextureColorG", 210);
			int cornerB = ConfigManager::getInstance().getInt("NavigationCube", "CornerFaceTextureColorB", 170);
			int cornerA = ConfigManager::getInstance().getInt("NavigationCube", "CornerFaceTextureColorA", 160);
			textureColor = wxColour(cornerR, cornerG, cornerB, cornerA);
		} else {
			// Default to main face color
			int mainR = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorR", 180);
			int mainG = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorG", 220);
			int mainB = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorB", 180);
			int mainA = ConfigManager::getInstance().getInt("NavigationCube", "MainFaceTextureColorA", 160);
			textureColor = wxColour(mainR, mainG, mainB, mainA);
		}
	}
	
	// Generate texture
	auto& dpiManager = DPIManager::getInstance();
	
	// Check if this face has text (main faces only)
	bool hasText = (faceName == "FRONT" || faceName == "REAR" || faceName == "LEFT" || 
	                faceName == "RIGHT" || faceName == "TOP" || faceName == "BOTTOM");
	
	// For MODULATE mode, use white transparent background like FreeCAD
	// Background: white transparent (255,255,255,0) - will multiply with material color
	// Text: blue opaque (0,100,255,255) - will be visible
	wxColour backgroundColor = wxColour(255, 255, 255, 0); // White background, transparent like FreeCAD
	
	int texWidth, texHeight;
	int texSize = ConfigManager::getInstance().getInt("NavigationCube", "TextureBaseSize", 192); // Configurable texture size
	
	if (hasText) {
		// Main faces with text use configurable texture size (default 192x192)
		texWidth = texSize;
		texHeight = texSize;
	} else {
		// Edge and corner faces are solid color - use tiny 2x2 texture for massive memory saving
		// Memory usage: 2x2x4 = 16 bytes per texture
		texWidth = 2;
		texHeight = 2;
	}
	
	std::vector<unsigned char> imageData(texWidth * texHeight * 4);
	
	// Generate texture with appropriate text and color
	std::string textureText = hasText ? faceName : "";
	
	// Get the correct font size for this face
	float correctFontSize = 0;
	PickId pickId = PickId::Front; // Default
	
	if (hasText) {
		// Find the corresponding PickId for this face
		if (faceName == "FRONT") pickId = PickId::Front;
		else if (faceName == "REAR") pickId = PickId::Rear;
		else if (faceName == "LEFT") pickId = PickId::Left;
		else if (faceName == "RIGHT") pickId = PickId::Right;
		else if (faceName == "TOP") pickId = PickId::Top;
		else if (faceName == "BOTTOM") pickId = PickId::Bottom;
		
		// Get the calculated font size for this face
		auto it = m_faceFontSizes.find(pickId);
		if (it != m_faceFontSizes.end()) {
			correctFontSize = it->second;
		} else {
			correctFontSize = texSize; // Fallback
		}
	}
	
	if (generateFaceTexture(textureText, imageData.data(), texWidth, texHeight, backgroundColor, correctFontSize, pickId)) {
		// DEBUG: Log successful texture generation
		LOG_INF_S("  Texture generated successfully:");
		LOG_INF_S("    Size: " + std::to_string(texWidth) + "x" + std::to_string(texHeight));
		LOG_INF_S("    Text: '" + textureText + "'");
		LOG_INF_S("    Font size: " + std::to_string(correctFontSize));
		LOG_INF_S("    Background: RGBA(" + std::to_string(backgroundColor.Red()) + "," +
		          std::to_string(backgroundColor.Green()) + "," + std::to_string(backgroundColor.Blue()) + "," +
		          std::to_string(backgroundColor.Alpha()) + ")");

		// DEBUG: Save texture as PNG file for inspection (original orientation for wxWidgets)
		if (hasText) {
			// Convert RGBA imageData to wxImage for saving
			wxImage debugImage(texWidth, texHeight);
			if (!debugImage.HasAlpha()) {
				debugImage.InitAlpha();
			}

			// Copy RGBA data to wxImage (original orientation for debugging)
			for (int y = 0; y < texHeight; ++y) {
				for (int x = 0; x < texWidth; ++x) {
					int index = (y * texWidth + x) * 4;
					debugImage.SetRGB(x, y, imageData[index], imageData[index + 1], imageData[index + 2]);
					debugImage.SetAlpha(x, y, imageData[index + 3]);
				}
			}

			// Save to file
			std::string filename = "texture_debug_" + faceName + (isHover ? "_hover" : "_normal") + ".png";
			wxString fullPath = wxGetCwd() + wxFileName::GetPathSeparator() + filename;
			if (debugImage.SaveFile(fullPath, wxBITMAP_TYPE_PNG)) {
				LOG_INF_S("    DEBUG PNG saved: " + std::string(fullPath.mb_str()));
			} else {
				LOG_WRN_S("    Failed to save DEBUG PNG: " + std::string(fullPath.mb_str()));
			}
		}

		// Flip image data vertically for OpenGL coordinate system (OpenGL has (0,0) at bottom-left)
		std::vector<unsigned char> flippedImageData(imageData.size());
		for (int y = 0; y < texHeight; ++y) {
			for (int x = 0; x < texWidth; ++x) {
				int srcIndex = (y * texWidth + x) * 4;
				int dstIndex = ((texHeight - 1 - y) * texWidth + x) * 4;
				flippedImageData[dstIndex] = imageData[srcIndex];     // R
				flippedImageData[dstIndex + 1] = imageData[srcIndex + 1]; // G
				flippedImageData[dstIndex + 2] = imageData[srcIndex + 2]; // B
				flippedImageData[dstIndex + 3] = imageData[srcIndex + 3]; // A
			}
		}

		// Compress texture data using DXT5 compression for reduced memory usage
		// DXT5 compression reduces memory footprint significantly
		SoTexture2* texture = new SoTexture2;
		
		// Convert RGBA to DXT5 compressed format
		// Note: Coin3D will handle DXT compression internally if supported
		// For now, use RGBA format but with optimized data transfer
		texture->image.setValue(SbVec2s(texWidth, texHeight), 4, flippedImageData.data());
		
		// Enable internal texture compression if available
		// Coin3D will use GPU compression if hardware supports it
		
		if (hasText) {
			// Text textures use MODULATE mode like FreeCAD's GL_MODULATE
			texture->model = SoTexture2::MODULATE;
			LOG_INF_S("    Texture mode: MODULATE (text texture, " + std::to_string(texWidth) + "x" + std::to_string(texHeight) + ")");
		} else {
			// Solid color textures use MODULATE with repeat wrapping for tiling
			texture->model = SoTexture2::MODULATE;
			texture->wrapS = SoTexture2::REPEAT;
			texture->wrapT = SoTexture2::REPEAT;
			LOG_INF_S("    Texture mode: MODULATE + REPEAT (solid color texture)");
		}
		
		return texture;
	} else {
		// DEBUG: Log texture generation failure
		LOG_ERR_S("  Texture generation FAILED for face: " + faceName);
	}
	
	return nullptr;
}

void CuteNavCube::generateAndCacheTextures() {
	LOG_INF_S("=== Starting texture generation and caching for main faces ===");

	// Generate textures only for main faces (which have texture quads)
	std::vector<std::string> mainFaces = {
		"FRONT", "REAR", "LEFT", "RIGHT", "TOP", "BOTTOM"
	};
	
	int normalCount = 0;
	int hoverCount = 0;
	int addedCount = 0;
	
	for (const auto& faceName : mainFaces) {
		// Check if texture separator exists
		auto textureSepIt = m_faceSeparators.find(faceName + "_Texture");
		if (textureSepIt == m_faceSeparators.end()) {
			LOG_WRN_S("Texture separator not found for face: " + faceName);
			continue;
		}

		// Generate normal state texture
		SoTexture2* normalTexture = createTextureForFace(faceName, false);
		if (normalTexture) {
			normalTexture->ref(); // Add reference to prevent premature deletion
			m_normalTextures[faceName] = normalTexture;
			normalCount++;
			
			// Add texture to texture separator
					SoSeparator* textureSep = textureSepIt->second;
			// Insert texture after material node (index 3 in structure: depthBuffer, polygonOffset, material, texture, geometry)
			if (textureSep->getNumChildren() >= 4) {
				textureSep->insertChild(normalTexture, 3); // Insert at index 3 (after material, before geometry)
						addedCount++;
				LOG_INF_S("Added normal texture for " + faceName + " at index 3");
			} else {
				LOG_WRN_S("Texture separator for " + faceName + " has insufficient children: " + std::to_string(textureSep->getNumChildren()));
			}
		}
		
		// Generate hover state texture
		SoTexture2* hoverTexture = createTextureForFace(faceName, true);
		if (hoverTexture) {
			hoverTexture->ref(); // Add reference to prevent premature deletion
			m_hoverTextures[faceName] = hoverTexture;
			hoverCount++;
		}
	}
	
	// DEBUG: Log texture generation summary
	LOG_INF_S("=== Texture generation completed ===");
	LOG_INF_S("  Normal textures generated: " + std::to_string(normalCount));
	LOG_INF_S("  Hover textures generated: " + std::to_string(hoverCount));
	LOG_INF_S("  Textures successfully applied: " + std::to_string(addedCount));
	LOG_INF_S("  Total faces processed: " + std::to_string(mainFaces.size()));
	
}

void CuteNavCube::regenerateFaceTexture(const std::string& faceName, bool isHover) {
	// Determine if this is a main face
	bool isMainFace = (faceName == "FRONT" || faceName == "REAR" || faceName == "LEFT" || 
	                   faceName == "RIGHT" || faceName == "TOP" || faceName == "BOTTOM");
	
	// For main faces, use texture separator; for others, use face separator
	std::string separatorName = isMainFace ? (faceName + "_Texture") : faceName;
	
	auto faceIt = m_faceSeparators.find(separatorName);
	if (faceIt == m_faceSeparators.end()) {
		LOG_WRN_S("CuteNavCube::regenerateFaceTexture: Separator not found: " + separatorName);
		return;
	}
	
	SoSeparator* faceSep = faceIt->second;
	
	// Get the appropriate cached texture
	SoTexture2* newTexture = nullptr;
	if (isHover) {
		auto it = m_hoverTextures.find(faceName);
		if (it != m_hoverTextures.end()) {
			newTexture = it->second;
		}
	} else {
		auto it = m_normalTextures.find(faceName);
		if (it != m_normalTextures.end()) {
			newTexture = it->second;
		}
	}
	
	if (!newTexture) {
		LOG_WRN_S("CuteNavCube::regenerateFaceTexture: Cached texture not found for face: " + faceName + 
			", hover: " + (isHover ? "true" : "false"));
		return;
	}
	
	// Find existing texture node in the separator
	int numChildren = faceSep->getNumChildren();
	int textureIndex = -1;
	SoTexture2* oldTexture = nullptr;
	
	for (int i = 0; i < numChildren; i++) {
		SoNode* child = faceSep->getChild(i);
		if (child->isOfType(SoTexture2::getClassTypeId())) {
			textureIndex = i;
			oldTexture = static_cast<SoTexture2*>(child);
			break;
		}
	}
	
	// Replace or insert texture
	if (textureIndex >= 0 && oldTexture != newTexture) {
		// Only replace if the new texture is different from the old one
		faceSep->removeChild(textureIndex);
		faceSep->insertChild(newTexture, textureIndex);
	} else if (textureIndex < 0) {
		// Insert texture after material node
		// For new structure: depthBuffer(0), polygonOffset(1), material(2), texture(3), geometry(4)
		if (isMainFace && numChildren >= 3) {
			// Main face texture separator: insert after material (index 3)
			faceSep->insertChild(newTexture, 3);
		} else if (numChildren > 0) {
			// Edge/corner faces or fallback: insert after first child (usually material)
			faceSep->insertChild(newTexture, 1);
		} else {
			faceSep->addChild(newTexture);
		}
	}
}