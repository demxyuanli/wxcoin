#include "edges/EdgeProcessorFactory.h"
#include "edges/extractors/OriginalEdgeExtractor.h"
#include "edges/extractors/FeatureEdgeExtractor.h"
#include "edges/extractors/MeshEdgeExtractor.h"
#include "edges/extractors/SilhouetteEdgeExtractor.h"
#include "edges/renderers/OriginalEdgeRenderer.h"
#include "edges/renderers/FeatureEdgeRenderer.h"
#include "edges/renderers/MeshEdgeRenderer.h"
#include <stdexcept>

EdgeProcessorFactory* EdgeProcessorFactory::m_instance = nullptr;

EdgeProcessorFactory& EdgeProcessorFactory::getInstance() {
    if (!m_instance) {
        m_instance = new EdgeProcessorFactory();
        m_instance->initializeDefaultProcessors();
    }
    return *m_instance;
}

EdgeProcessorFactory::EdgeProcessorFactory() {}

void EdgeProcessorFactory::initializeDefaultProcessors() {
    // Register default extractors
    m_extractors[EdgeType::Original] = std::make_shared<OriginalEdgeExtractor>();
    m_extractors[EdgeType::Feature] = std::make_shared<FeatureEdgeExtractor>();
    m_extractors[EdgeType::Mesh] = std::make_shared<MeshEdgeExtractor>();
    m_extractors[EdgeType::Silhouette] = std::make_shared<SilhouetteEdgeExtractor>();

    // Register default renderers
    m_renderers[EdgeType::Original] = std::make_shared<OriginalEdgeRenderer>();
    m_renderers[EdgeType::Feature] = std::make_shared<FeatureEdgeRenderer>();
    m_renderers[EdgeType::Mesh] = std::make_shared<MeshEdgeRenderer>();
}

std::shared_ptr<BaseEdgeExtractor> EdgeProcessorFactory::getExtractor(EdgeType type) {
    auto it = m_extractors.find(type);
    if (it == m_extractors.end()) {
        throw std::runtime_error("No extractor registered for edge type");
    }
    return it->second;
}

std::shared_ptr<BaseEdgeRenderer> EdgeProcessorFactory::getRenderer(EdgeType type) {
    auto it = m_renderers.find(type);
    if (it == m_renderers.end()) {
        throw std::runtime_error("No renderer registered for edge type");
    }
    return it->second;
}

void EdgeProcessorFactory::registerExtractor(EdgeType type, std::shared_ptr<BaseEdgeExtractor> extractor) {
    m_extractors[type] = extractor;
}

void EdgeProcessorFactory::registerRenderer(EdgeType type, std::shared_ptr<BaseEdgeRenderer> renderer) {
    m_renderers[type] = renderer;
}

std::vector<EdgeType> EdgeProcessorFactory::getAvailableTypes() const {
    std::vector<EdgeType> types;
    for (const auto& pair : m_extractors) {
        types.push_back(pair.first);
    }
    return types;
}

bool EdgeProcessorFactory::isTypeSupported(EdgeType type) const {
    return m_extractors.find(type) != m_extractors.end();
}

