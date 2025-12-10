#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <string>

class SoSeparator;

class CoinNodeManager {
public:
    CoinNodeManager();
    ~CoinNodeManager();

    SoSeparator* createOrClearNode(SoSeparator* existingNode);
    void cleanupTextureNodes(SoSeparator* node);
    void configureNodeCaching(SoSeparator* node);
    void removeAllChildren(SoSeparator* node);

private:
    void disableRenderCaching(SoSeparator* node);
};

