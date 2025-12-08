#include "viewer/ObjectDisplayModeManager.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>

ObjectDisplayModeManager::ObjectDisplayModeManager()
    : m_objectDisplayMode(RenderingConfig::DisplayMode::Solid)
    , m_viewerModManager(std::make_unique<ViewerModManager>())
{
}

ObjectDisplayModeManager::~ObjectDisplayModeManager()
{
}

void ObjectDisplayModeManager::setObjectDisplayMode(SoSwitch* modeSwitch, RenderingConfig::DisplayMode mode)
{
    if (!modeSwitch) {
        LOG_ERR_S("ObjectDisplayModeManager: ModeSwitch is null");
        return;
    }

    m_objectDisplayMode = mode;
    updateDisplayMode(modeSwitch, mode);
}

void ObjectDisplayModeManager::updateDisplayMode(SoSwitch* modeSwitch, RenderingConfig::DisplayMode mode)
{
    if (!modeSwitch || !m_viewerModManager) {
        LOG_WRN_S("ObjectDisplayModeManager::updateDisplayMode: modeSwitch or viewerModManager is null");
        return;
    }

    // CRITICAL FIX: Map DisplayMode to SoSwitch index based on button mappings
    // According to RenderModeListener.cpp:
    // - NoShading -> index 3 (ShadedMode)
    // - Points -> index 0 (PointsMode)
    // - Wireframe -> index 1 (WireframeMode)
    // - FlatLines -> index 2 (FlatLinesMode, SolidWireframe)
    // - Shaded -> index 3 (ShadedMode, Solid)
    // - ShadedWireframe -> index 2 (FlatLinesMode, SolidWireframe)
    // - HiddenLine -> index 2 (FlatLinesMode, HiddenLine)
    
    int modeIndex = -1;
    switch (mode) {
    case RenderingConfig::DisplayMode::Points:
        modeIndex = 0;
        break;
    case RenderingConfig::DisplayMode::Wireframe:
        modeIndex = 1;
        break;
    case RenderingConfig::DisplayMode::SolidWireframe:
    case RenderingConfig::DisplayMode::HiddenLine:
        modeIndex = 2;
        break;
    case RenderingConfig::DisplayMode::Solid:
    case RenderingConfig::DisplayMode::NoShading:
        modeIndex = 3;
        break;
    default:
        modeIndex = getModeIndex(mode); // Fallback to original method
        break;
    }
    
    int numChildren = modeSwitch->getNumChildren();
    
    LOG_INF_S("ObjectDisplayModeManager::updateDisplayMode: mode=" + std::to_string(static_cast<int>(mode)) + 
        ", modeIndex=" + std::to_string(modeIndex) + ", numChildren=" + std::to_string(numChildren));
    
    // CRITICAL: Check if the child at modeIndex is valid (not a placeholder)
    if (modeIndex >= 0 && modeIndex < numChildren) {
        SoNode* childNode = modeSwitch->getChild(modeIndex);
        if (childNode) {
            // Check if it's a placeholder (empty SoSeparator with no children)
            if (childNode->isOfType(SoSeparator::getClassTypeId())) {
                SoSeparator* childSep = static_cast<SoSeparator*>(childNode);
                if (childSep->getNumChildren() == 0) {
                    LOG_WRN_S("ObjectDisplayModeManager::updateDisplayMode: Child " + std::to_string(modeIndex) + 
                        " is a placeholder (empty separator) - mode node was not built");
                }
            }
            modeSwitch->whichChild.setValue(modeIndex);
            m_objectDisplayMode = mode;
            LOG_INF_S("ObjectDisplayModeManager::updateDisplayMode: Successfully set whichChild to " + std::to_string(modeIndex));
        } else {
            LOG_WRN_S("ObjectDisplayModeManager::updateDisplayMode: Child " + std::to_string(modeIndex) + " is null");
        }
    } else {
        LOG_WRN_S("ObjectDisplayModeManager::updateDisplayMode: Invalid mode index " + std::to_string(modeIndex) +
            " for mode " + std::to_string(static_cast<int>(mode)) + 
            " (SoSwitch has " + std::to_string(numChildren) + " children)");
    }
}

SoSwitch* ObjectDisplayModeManager::buildModeSwitch(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ModularEdgeComponent* modularEdgeComponent,
    VertexExtractor* vertexExtractor)
{
    if (!m_viewerModManager) {
        LOG_ERR_S("ObjectDisplayModeManager: ViewerModManager not initialized");
        return nullptr;
    }

    // Create SoSwitch
    SoSwitch* modeSwitch = new SoSwitch();
    modeSwitch->ref();

    // Build all mode nodes using ViewerModManager
    std::vector<SoSeparator*> modeNodes = m_viewerModManager->buildAllModeNodes(
        shape, params, context,
        modularEdgeComponent,
        vertexExtractor
    );

    // CRITICAL FIX: Ensure all 4 mode nodes are added to SoSwitch
    // Even if a mode node is nullptr, add an empty separator to maintain index alignment
    // This ensures that getModeIndex() returns the correct SoSwitch child index
    for (size_t i = 0; i < 4; ++i) {
        SoSeparator* modeNode = (i < modeNodes.size()) ? modeNodes[i] : nullptr;
        if (modeNode) {
            modeNode->ref();
            modeSwitch->addChild(modeNode);
            // Log node info for debugging
            if (modeNode->getNumChildren() > 0) {
                LOG_INF_S("ObjectDisplayModeManager: Added mode node " + std::to_string(i) + 
                    " with " + std::to_string(modeNode->getNumChildren()) + " children");
            } else {
                LOG_WRN_S("ObjectDisplayModeManager: Mode node " + std::to_string(i) + " is empty (no children)");
            }
        } else {
            // Add empty separator as placeholder to maintain index alignment
            SoSeparator* placeholder = new SoSeparator();
            placeholder->ref();
            modeSwitch->addChild(placeholder);
            LOG_WRN_S("ObjectDisplayModeManager: Mode node " + std::to_string(i) + " is nullptr, using placeholder");
        }
    }
    
    LOG_INF_S("ObjectDisplayModeManager: Built SoSwitch with " + std::to_string(modeSwitch->getNumChildren()) + " children");

    // Set initial mode
    int initialModeIndex = getModeIndex(context.display.displayMode);
    modeSwitch->whichChild.setValue(initialModeIndex);
    m_objectDisplayMode = context.display.displayMode;

    return modeSwitch;
}

int ObjectDisplayModeManager::getModeIndex(RenderingConfig::DisplayMode mode) const
{
    if (m_viewerModManager) {
        return m_viewerModManager->getModeIndex(mode);
    }
    return 3; // Default to Shaded mode index
}


