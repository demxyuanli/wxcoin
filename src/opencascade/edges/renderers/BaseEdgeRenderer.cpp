#include "edges/renderers/BaseEdgeRenderer.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoPolygonOffset.h>

BaseEdgeRenderer::BaseEdgeRenderer() {}

SoSeparator* BaseEdgeRenderer::createLineNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width,
    int style) {
    
    if (points.empty()) return nullptr;
    
    SoSeparator* separator = new SoSeparator();
    separator->ref();
    
    // Note: Polygon offset is NOT added here for edges
    // FreeCAD approach: Polygon offset is added BEFORE faces in the scene graph
    // This pushes faces slightly back, allowing edges (rendered AFTER faces) to appear on top
    // Edges themselves don't need polygon offset when rendered after faces with offset
    
    // Material
    SoMaterial* material = new SoMaterial();
    applyMaterial(material, color);
    separator->addChild(material);
    
    // Line style
    SoDrawStyle* drawStyle = new SoDrawStyle();
    applyLineStyle(drawStyle, width, style);
    separator->addChild(drawStyle);
    
    // Coordinates
    SoCoordinate3* coords = new SoCoordinate3();
    coords->point.setNum(static_cast<int>(points.size()));
    for (size_t i = 0; i < points.size(); ++i) {
        coords->point.set1Value(static_cast<int>(i), 
            static_cast<float>(points[i].X()),
            static_cast<float>(points[i].Y()),
            static_cast<float>(points[i].Z()));
    }
    separator->addChild(coords);
    
    // Line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet();
    int coordIndex = 0;
    for (size_t i = 0; i + 1 < points.size(); i += 2) {
        lineSet->coordIndex.set1Value(coordIndex++, static_cast<int>(i));
        lineSet->coordIndex.set1Value(coordIndex++, static_cast<int>(i + 1));
        lineSet->coordIndex.set1Value(coordIndex++, -1);
    }
    separator->addChild(lineSet);
    
    return separator;
}

void BaseEdgeRenderer::applyMaterial(SoMaterial* material, const Quantity_Color& color) {
    if (material) {
        material->diffuseColor.setValue(
            static_cast<float>(color.Red()),
            static_cast<float>(color.Green()),
            static_cast<float>(color.Blue())
        );
    }
}

void BaseEdgeRenderer::applyLineStyle(SoDrawStyle* drawStyle, double width, int style) {
    if (drawStyle) {
        drawStyle->lineWidth.setValue(static_cast<float>(width));

        // Apply line pattern based on style
        uint16_t pattern = 0xFFFF;  // Default: solid
        switch (style) {
            case 0:  // Solid
                pattern = 0xFFFF;
                break;
            case 1:  // Dashed
                pattern = 0x0F0F;  // Dash pattern
                break;
            case 2:  // Dotted
                pattern = 0xAAAA;  // Dot pattern
                break;
            case 3:  // Dash-Dot
                pattern = 0x0C0C;  // Dash-dot pattern
                break;
            default:
                pattern = 0xFFFF;
                break;
        }

        drawStyle->linePattern.setValue(pattern);
        drawStyle->style.setValue(SoDrawStyle::LINES);
    }
}

