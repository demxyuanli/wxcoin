// Deprecated: Silhouette edge listener removed. Use Outline toggle via IOutlineApi.
#pragma once
#include "CommandListener.h"
class OCCViewer;
class ShowSilhouetteEdgesListener : public CommandListener {
public:
    ShowSilhouetteEdgesListener(OCCViewer*) {}
    CommandResult executeCommand(const std::string&, const std::unordered_map<std::string, std::string>&) override {
        return CommandResult(false, "Silhouette edges removed. Use Outline toggle.", "SHOW_SILHOUETTE_EDGES");
    }
    bool canHandleCommand(const std::string& commandType) const override { return false; }
    std::string getListenerName() const override { return "ShowSilhouetteEdgesListener(removed)"; }
};