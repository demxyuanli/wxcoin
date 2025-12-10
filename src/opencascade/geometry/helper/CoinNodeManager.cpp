#include "geometry/helper/CoinNodeManager.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

CoinNodeManager::CoinNodeManager() {
}

CoinNodeManager::~CoinNodeManager() {
}

SoSeparator* CoinNodeManager::createOrClearNode(SoSeparator* existingNode) {
    if (!existingNode) {
        try {
            SoSeparator* newNode = new SoSeparator();
            if (newNode) {
                configureNodeCaching(newNode);
                newNode->ref();
                return newNode;
            } else {
                LOG_ERR_S("CoinNodeManager: Failed to create SoSeparator");
                return nullptr;
            }
        } catch (const std::exception& e) {
            LOG_ERR_S("CoinNodeManager: Exception creating SoSeparator: " + std::string(e.what()));
            return nullptr;
        }
    } else {
        try {
            existingNode->removeAllChildren();
            configureNodeCaching(existingNode);
            return existingNode;
        } catch (const std::exception& e) {
            LOG_ERR_S("CoinNodeManager: Exception removing children: " + std::string(e.what()));
            existingNode->unref();
            SoSeparator* newNode = new SoSeparator();
            configureNodeCaching(newNode);
            newNode->ref();
            return newNode;
        }
    }
}

void CoinNodeManager::cleanupTextureNodes(SoSeparator* node) {
    if (!node) {
        return;
    }

    for (int i = node->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = node->getChild(i);
        if (child && (child->isOfType(SoTexture2::getClassTypeId()) ||
            child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
            node->removeChild(i);
        }
    }
}

void CoinNodeManager::configureNodeCaching(SoSeparator* node) {
    if (!node) {
        return;
    }
    disableRenderCaching(node);
}

void CoinNodeManager::disableRenderCaching(SoSeparator* node) {
    if (!node) {
        return;
    }
    node->renderCaching.setValue(SoSeparator::OFF);
    node->boundingBoxCaching.setValue(SoSeparator::OFF);
    node->pickCulling.setValue(SoSeparator::OFF);
}

void CoinNodeManager::removeAllChildren(SoSeparator* node) {
    if (node) {
        node->removeAllChildren();
    }
}

