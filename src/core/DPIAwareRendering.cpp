#include "DPIAwareRendering.h"
#include "logger/Logger.h"
#include <GL/gl.h>
#include <Inventor/nodes/SoDrawStyle.h>

void DPIAwareRendering::setDPIAwareLineWidth(float baseWidth) {
    float scaledWidth = DPIManager::getInstance().getScaledLineWidth(baseWidth);
    glLineWidth(scaledWidth);
    
    LOG_DBG_S("DPIAwareRendering: Set OpenGL line width from " + 
            std::to_string(baseWidth) + " to " + std::to_string(scaledWidth));
}

void DPIAwareRendering::setDPIAwarePointSize(float baseSize) {
    float scaledSize = DPIManager::getInstance().getScaledPointSize(baseSize);
    glPointSize(scaledSize);
    
    LOG_DBG_S("DPIAwareRendering: Set OpenGL point size from " + 
            std::to_string(baseSize) + " to " + std::to_string(scaledSize));
}

void DPIAwareRendering::configureDPIAwareDrawStyle(SoDrawStyle* drawStyle, 
                                                  float baseLineWidth,
                                                  float basePointSize) {
    if (!drawStyle) {
        LOG_WRN_S("DPIAwareRendering::configureDPIAwareDrawStyle: Null drawStyle");
        return;
    }
    
    auto& dpiManager = DPIManager::getInstance();
    
    float scaledLineWidth = dpiManager.getScaledLineWidth(baseLineWidth);
    float scaledPointSize = dpiManager.getScaledPointSize(basePointSize);
    
    drawStyle->lineWidth = scaledLineWidth;
    drawStyle->pointSize = scaledPointSize;

}

void DPIAwareRendering::updateDrawStyleDPI(SoDrawStyle* drawStyle, 
                                          float originalLineWidth,
                                          float originalPointSize) {
    if (!drawStyle) {
        LOG_WRN_S("DPIAwareRendering::updateDrawStyleDPI: Null drawStyle");
        return;
    }
    
    // Reconfigure with original base values to get updated DPI scaling
    configureDPIAwareDrawStyle(drawStyle, originalLineWidth, originalPointSize);
}

SoDrawStyle* DPIAwareRendering::createDPIAwareCoordinateLineStyle(float baseWidth) {
    SoDrawStyle* style = new SoDrawStyle;
    style->style = SoDrawStyle::LINES;
    
    configureDPIAwareDrawStyle(style, baseWidth, 1.0f);
    
    return style;
}

SoDrawStyle* DPIAwareRendering::createDPIAwareGeometryStyle(float baseWidth, bool filled) {
    SoDrawStyle* style = new SoDrawStyle;
    
    if (filled) {
        style->style = SoDrawStyle::FILLED;
    } else {
        style->style = SoDrawStyle::LINES;
    }
    
    configureDPIAwareDrawStyle(style, baseWidth, 1.0f);
   
    return style;
}

float DPIAwareRendering::getCurrentDPIScale() {
    return DPIManager::getInstance().getDPIScale();
} 
