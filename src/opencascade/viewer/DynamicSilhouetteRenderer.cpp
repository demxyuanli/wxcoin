#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/DynamicSilhouetteRenderer.h"
#include "logger/Logger.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoBaseColor.h>

#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include <OpenCASCADE/Geom_Curve.hxx>
#include <OpenCASCADE/GeomAdaptor_Curve.hxx>
#include <OpenCASCADE/GCPnts_UniformDeflection.hxx>

DynamicSilhouetteRenderer::DynamicSilhouetteRenderer(SoSeparator* parent)
    : m_parent(parent), m_silhouetteNode(nullptr) {
    m_silhouetteNode = new SoSeparator;
    m_silhouetteNode->ref();
    LOG_DBG("DynamicSilhouetteRenderer created", "DynamicSilhouetteRenderer");
}

DynamicSilhouetteRenderer::~DynamicSilhouetteRenderer() {
    clearSilhouette();
    if (m_silhouetteNode) {
        m_silhouetteNode->unref();
        m_silhouetteNode = nullptr;
    }
    LOG_DBG("DynamicSilhouetteRenderer destroyed", "DynamicSilhouetteRenderer");
}

void DynamicSilhouetteRenderer::setShape(const TopoDS_Shape& shape) {
    m_shape = shape;
    if (m_enabled) {
        buildSilhouette();
    }
}

void DynamicSilhouetteRenderer::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    
    if (m_enabled) {
        buildSilhouette();
    } else {
        clearSilhouette();
    }
    
    LOG_DBG((std::string("Silhouette ") + (enabled ? "enabled" : "disabled")).c_str(), "DynamicSilhouetteRenderer");
}

void DynamicSilhouetteRenderer::setFastMode(bool fastMode) {
    m_fastMode = fastMode;
    if (m_enabled) {
        buildSilhouette(); // Rebuild with different quality settings
    }
}

void DynamicSilhouetteRenderer::setColor(float r, float g, float b) {
    m_colorR = r;
    m_colorG = g;
    m_colorB = b;
    
    if (m_enabled && m_silhouetteNode && m_silhouetteNode->getNumChildren() > 0) {
        // Update existing material if present
        for (int i = 0; i < m_silhouetteNode->getNumChildren(); ++i) {
            if (auto* material = dynamic_cast<SoMaterial*>(m_silhouetteNode->getChild(i))) {
                material->diffuseColor.setValue(r, g, b);
                material->emissiveColor.setValue(r * 0.3f, g * 0.3f, b * 0.3f);
                break;
            }
        }
    }
}

void DynamicSilhouetteRenderer::setLineWidth(float width) {
    m_lineWidth = width;
    
    if (m_enabled && m_silhouetteNode && m_silhouetteNode->getNumChildren() > 0) {
        // Update existing draw style if present
        for (int i = 0; i < m_silhouetteNode->getNumChildren(); ++i) {
            if (auto* drawStyle = dynamic_cast<SoDrawStyle*>(m_silhouetteNode->getChild(i))) {
                drawStyle->lineWidth.setValue(width);
                break;
            }
        }
    }
}

void DynamicSilhouetteRenderer::buildSilhouette() {
    clearSilhouette();
    
    if (m_shape.IsNull()) {
        LOG_WRN("Cannot build silhouette for null shape", "DynamicSilhouetteRenderer");
        return;
    }
    
    try {
        // Create material for silhouette lines
        auto* material = new SoMaterial;
        material->diffuseColor.setValue(m_colorR, m_colorG, m_colorB);
        material->emissiveColor.setValue(m_colorR * 0.3f, m_colorG * 0.3f, m_colorB * 0.3f);
        material->transparency.setValue(0.0f);
        m_silhouetteNode->addChild(material);
        
        // Create draw style for line rendering
        auto* drawStyle = new SoDrawStyle;
        drawStyle->style.setValue(SoDrawStyle::LINES);
        drawStyle->lineWidth.setValue(m_lineWidth);
        drawStyle->linePattern.setValue(0xFFFF); // Solid line
        m_silhouetteNode->addChild(drawStyle);
        
        // Extract and render silhouette edges
        extractSilhouetteEdges(m_shape);
        
        LOG_DBG("Silhouette built successfully", "DynamicSilhouetteRenderer");
        
    } catch (const std::exception& e) {
        LOG_ERR((std::string("Failed to build silhouette: ") + e.what()).c_str(), "DynamicSilhouetteRenderer");
        clearSilhouette();
    }
}

void DynamicSilhouetteRenderer::clearSilhouette() {
    if (m_silhouetteNode) {
        m_silhouetteNode->removeAllChildren();
    }
}

void DynamicSilhouetteRenderer::extractSilhouetteEdges(const TopoDS_Shape& shape) {
    std::vector<SbVec3f> points;
    std::vector<int> lineIndices;
    
    // Traverse all edges in the shape
    TopExp_Explorer edgeExplorer(shape, TopAbs_EDGE);
    int pointIndex = 0;
    
    for (; edgeExplorer.More(); edgeExplorer.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeExplorer.Current());
        
        if (BRep_Tool::Degenerated(edge)) {
            continue; // Skip degenerated edges
        }
        
        try {
            // Get curve from edge
            Standard_Real first, last;
            Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
            
            if (curve.IsNull()) {
                continue;
            }
            
            // Set deflection based on fast mode
            Standard_Real deflection = m_fastMode ? 0.1 : 0.01;
            
            // Discretize curve
            GeomAdaptor_Curve adaptor(curve, first, last);
            GCPnts_UniformDeflection discretizer(adaptor, deflection);
            
            if (!discretizer.IsDone() || discretizer.NbPoints() < 2) {
                continue;
            }
            
            // Add points and line indices
            int startIndex = pointIndex;
            for (int i = 1; i <= discretizer.NbPoints(); ++i) {
                gp_Pnt pnt = discretizer.Value(i);
                points.emplace_back(static_cast<float>(pnt.X()), 
                                   static_cast<float>(pnt.Y()), 
                                   static_cast<float>(pnt.Z()));
                
                if (i > 1) {
                    // Add line segment
                    lineIndices.push_back(pointIndex - 1);
                    lineIndices.push_back(pointIndex);
                    lineIndices.push_back(-1); // End of line marker
                }
                pointIndex++;
            }
            
        } catch (const std::exception& e) {
            LOG_WRN((std::string("Failed to process edge: ") + e.what()).c_str(), "DynamicSilhouetteRenderer");
            continue;
        }
    }
    
    if (points.empty() || lineIndices.empty()) {
        LOG_WRN("No silhouette edges extracted", "DynamicSilhouetteRenderer");
        return;
    }
    
    // Create Coin3D geometry
    auto* coords = new SoCoordinate3;
    coords->point.setValues(0, static_cast<int>(points.size()), points.data());
    m_silhouetteNode->addChild(coords);
    
    auto* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, static_cast<int>(lineIndices.size()), lineIndices.data());
    m_silhouetteNode->addChild(lineSet);
    
    LOG_DBG((std::string("Extracted ") + std::to_string(points.size()) + " points, " + 
             std::to_string(lineIndices.size() / 3) + " line segments").c_str(), "DynamicSilhouetteRenderer");
}