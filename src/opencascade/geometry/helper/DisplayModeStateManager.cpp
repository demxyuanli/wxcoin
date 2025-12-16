#include "geometry/helper/DisplayModeStateManager.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"
#include <sstream>
#include <iomanip>

void DisplayModeStateManager::setRenderStateForMode(DisplayModeRenderState& state, 
                                                     RenderingConfig::DisplayMode displayMode,
                                                     const GeometryRenderContext& context) {
    state.showSurface = false;
    state.showOriginalEdges = false;
    state.showMeshEdges = false;
    state.wireframeMode = false;
    state.textureEnabled = false;
    state.lightingEnabled = true;
    state.showPoints = context.display.showPointView;
    state.showSolidWithPoints = context.display.showSolidWithPointView;
    
    state.surfaceAmbientColor = context.material.ambientColor;
    state.surfaceDiffuseColor = context.material.diffuseColor;
    state.surfaceSpecularColor = context.material.specularColor;
    state.surfaceEmissiveColor = context.material.emissiveColor;
    state.shininess = context.material.shininess;
    state.transparency = 0.0;
    state.blendMode = RenderingConfig::BlendMode::None;
    state.surfaceDisplayMode = displayMode;
    
    switch (displayMode) {
    case RenderingConfig::DisplayMode::NoShading: {
        state.showSurface = true;
        state.showOriginalEdges = true;
        state.wireframeMode = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::NoShading;
        state.textureEnabled = false;
        state.lightingEnabled = false;
        // CRITICAL: Keep original diffuse color for visibility, only reset ambient/specular/emissive
        // NoShading mode uses BASE_COLOR lighting, so diffuse color is directly displayed
        state.surfaceAmbientColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        // state.surfaceDiffuseColor is preserved from context (set above)
        state.surfaceSpecularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.surfaceEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.shininess = 0.0;
        state.transparency = 0.0;
        state.blendMode = RenderingConfig::BlendMode::None;
        state.showPoints = false;
        break;
    }

    case RenderingConfig::DisplayMode::Points: {
        state.showPoints = true;
        // CRITICAL: Points mode should show surface if showSolidWithPointView is enabled
        // But even if surface is hidden, points should still be displayed
        state.showSurface = context.display.showSolidWithPointView;
        state.wireframeMode = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::Points;
        state.showOriginalEdges = false;
        state.showMeshEdges = false;
        state.lightingEnabled = false;
        state.textureEnabled = false;
        break;
    }

    case RenderingConfig::DisplayMode::Wireframe: {
        state.showSurface = false;
        state.showOriginalEdges = true;
        state.wireframeMode = true;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::Wireframe;
        state.textureEnabled = false;
        state.lightingEnabled = false;
        state.surfaceAmbientColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.surfaceSpecularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.surfaceEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.shininess = 0.0;
        state.transparency = 0.0;
        state.showPoints = false;
        break;
    }

    case RenderingConfig::DisplayMode::FlatLines: {
        state.showSurface = true;
        state.showOriginalEdges = true;
        state.wireframeMode = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::FlatLines;
        state.lightingEnabled = true;
        state.textureEnabled = false;
        state.shininess = 30.0;
        state.transparency = 0.0;
        state.blendMode = RenderingConfig::BlendMode::None;
        state.showPoints = false;
        break;
    }

    case RenderingConfig::DisplayMode::Solid: {
        state.showSurface = true;
        state.wireframeMode = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::Solid;
        state.lightingEnabled = true;
        state.textureEnabled = false;
        state.transparency = 0.0;
        state.blendMode = RenderingConfig::BlendMode::None;
        state.showPoints = false;
        break;
    }

    case RenderingConfig::DisplayMode::Transparent: {
        state.showSurface = true;
        state.wireframeMode = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::Transparent;
        state.lightingEnabled = true;
        state.textureEnabled = false;
        if (state.transparency <= 0.0) {
            state.transparency = 0.5;
        }
        state.blendMode = RenderingConfig::BlendMode::Alpha;
        state.showPoints = false;
        break;
    }

    case RenderingConfig::DisplayMode::HiddenLine: {
        state.showSurface = true;
        state.showMeshEdges = true;
        state.wireframeMode = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::HiddenLine;
        state.textureEnabled = false;
        state.lightingEnabled = false;
        state.surfaceAmbientColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        state.surfaceDiffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        state.surfaceSpecularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.surfaceEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.shininess = 0.0;
        state.transparency = 0.0;
        state.blendMode = RenderingConfig::BlendMode::None;
        if (state.meshEdgeColor.Red() == 0.0 && state.meshEdgeColor.Green() == 0.0 && state.meshEdgeColor.Blue() == 0.0) {
            state.meshEdgeColor = context.material.diffuseColor;
        }
        state.showPoints = false;
        break;
    }

    default: {
        state.showSurface = true;
        state.wireframeMode = false;
        state.showPoints = false;
        break;
    }
    }
    
    std::ostringstream logMsg;
    logMsg << "DisplayMode [" << RenderingConfig::getDisplayModeName(displayMode) << "] State:\n";
    logMsg << "  Display Components:\n";
    logMsg << "    showSurface: " << (state.showSurface ? "true" : "false") << "\n";
    logMsg << "    showOriginalEdges: " << (state.showOriginalEdges ? "true" : "false") << "\n";
    logMsg << "    showMeshEdges: " << (state.showMeshEdges ? "true" : "false") << "\n";
    logMsg << "    showPoints: " << (state.showPoints ? "true" : "false") << "\n";
    logMsg << "  Surface Properties:\n";
    logMsg << "    wireframeMode: " << (state.wireframeMode ? "true" : "false") << "\n";
    logMsg << "    textureEnabled: " << (state.textureEnabled ? "true" : "false") << "\n";
    logMsg << "    lightingEnabled: " << (state.lightingEnabled ? "true" : "false") << "\n";
    logMsg << "    surfaceDisplayMode: " << RenderingConfig::getDisplayModeName(state.surfaceDisplayMode) << "\n";
    logMsg << "  Material:\n";
    logMsg << "    ambient: (" << std::fixed << std::setprecision(2) 
           << state.surfaceAmbientColor.Red() << ", "
           << state.surfaceAmbientColor.Green() << ", "
           << state.surfaceAmbientColor.Blue() << ")\n";
    logMsg << "    diffuse: (" << state.surfaceDiffuseColor.Red() << ", "
           << state.surfaceDiffuseColor.Green() << ", "
           << state.surfaceDiffuseColor.Blue() << ")\n";
    logMsg << "    specular: (" << state.surfaceSpecularColor.Red() << ", "
           << state.surfaceSpecularColor.Green() << ", "
           << state.surfaceSpecularColor.Blue() << ")\n";
    logMsg << "    shininess: " << state.shininess << "\n";
    logMsg << "    transparency: " << state.transparency << "\n";
    logMsg << "  Edges:\n";
    logMsg << "    originalEdgeColor: (" << state.originalEdgeColor.Red() << ", "
           << state.originalEdgeColor.Green() << ", "
           << state.originalEdgeColor.Blue() << ")\n";
    logMsg << "    originalEdgeWidth: " << state.originalEdgeWidth << "\n";
    logMsg << "    meshEdgeColor: (" << state.meshEdgeColor.Red() << ", "
           << state.meshEdgeColor.Green() << ", "
           << state.meshEdgeColor.Blue() << ")\n";
    logMsg << "    meshEdgeWidth: " << state.meshEdgeWidth << "\n";
    logMsg << "  Blend:\n";
    logMsg << "    blendMode: " << static_cast<int>(state.blendMode) << "\n";
    
    LOG_INF_S(logMsg.str());
}


