#include "viewer/modes/WireframeMode.h"
#include "edges/ModularEdgeComponent.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <OpenCASCADE/Quantity_Color.hxx>

WireframeMode::WireframeMode() {
}

RenderingConfig::DisplayMode WireframeMode::getModeType() const {
    return RenderingConfig::DisplayMode::Wireframe;
}

int WireframeMode::getSwitchChildIndex() const {
    return 1; // Wireframe mode is child 1
}

bool WireframeMode::requiresFaces() const {
    return false;
}

bool WireframeMode::requiresEdges() const {
    return true;
}

SoSeparator* WireframeMode::buildModeNode(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ModularEdgeComponent* modularEdgeComponent,
    VertexExtractor* vertexExtractor) {
    
    if (shape.IsNull()) {
        return nullptr;
    }

    SoSeparator* modeNode = new SoSeparator();
    modeNode->ref();
    modeNode->renderCaching.setValue(SoSeparator::OFF);
    modeNode->boundingBoxCaching.setValue(SoSeparator::OFF);
    modeNode->pickCulling.setValue(SoSeparator::OFF);

    // DrawStyle for wireframe
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->style.setValue(SoDrawStyle::LINES);
    drawStyle->lineWidth.setValue(static_cast<float>(context.display.wireframeWidth));
    modeNode->addChild(drawStyle);
    LOG_INF_S("WireframeMode::buildModeNode: Added DrawStyle with lineWidth=" + std::to_string(context.display.wireframeWidth));

    // Material for wireframe
    SoMaterial* material = new SoMaterial();
    Standard_Real r, g, b;
    context.display.wireframeColor.Values(r, g, b, Quantity_TOC_RGB);
    
    // CRITICAL FIX: Use emissiveColor to ensure wireframe is visible even on dark backgrounds
    // Wireframe lines should be self-illuminated (emissive) to be visible regardless of lighting
    material->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    material->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    material->transparency.setValue(static_cast<float>(context.material.transparency));
    modeNode->addChild(material);
    LOG_INF_S("WireframeMode::buildModeNode: Added material with color (" + 
        std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + 
        "), using emissiveColor for visibility");

    // Extract original edges and feature edges from CAD geometry
    if (modularEdgeComponent) {
        LOG_INF_S("WireframeMode::buildModeNode: Extracting edges with modularEdgeComponent");
        
        // CRITICAL FIX: Enable edge display flags before extracting edges
        // This ensures that updateEdgeDisplay will actually add the edge nodes
        modularEdgeComponent->setEdgeDisplayType(EdgeType::Original, true);
        modularEdgeComponent->setEdgeDisplayType(EdgeType::Feature, true);
        
        // Extract original edges (all edges from CAD geometry)
        Quantity_Color originalColor(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        modularEdgeComponent->extractOriginalEdges(shape, 80.0, 0.01, false, originalColor, 1.0, false, Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), 3.0);
        
        // Extract feature edges (sharp edges, typically at feature angles)
        Quantity_Color featureColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        modularEdgeComponent->extractFeatureEdges(shape, 15.0, 0.005, false, false, featureColor, 1.0);
        
        // Update edge display to attach edge nodes to mode node
        modularEdgeComponent->updateEdgeDisplay(modeNode);
        LOG_INF_S("WireframeMode::buildModeNode: Edge display updated, modeNode now has " + std::to_string(modeNode->getNumChildren()) + " children");
    } else {
        LOG_WRN_S("WireframeMode::buildModeNode: ModularEdgeComponent not available - cannot extract original edges, modeNode has only " + std::to_string(modeNode->getNumChildren()) + " children (DrawStyle + Material)");
    }

    return modeNode;
}


