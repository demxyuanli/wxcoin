#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

class SceneManager;
class SoSeparator;
class OCCGeometry;
class SelectionManager;
class DynamicSilhouetteRenderer;

struct SelectionOutlineStyle {
    float lineWidth{2.0f};
    float r{1.0f}, g{0.6f}, b{0.0f};
};

class SelectionOutlineManager {
public:
    SelectionOutlineManager(SceneManager* sceneManager,
                            SoSeparator* occRoot,
                            std::vector<std::shared_ptr<OCCGeometry>>* selectedGeometries);
    ~SelectionOutlineManager();

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setStyle(const SelectionOutlineStyle& style);
    SelectionOutlineStyle getStyle() const { return m_style; }

    // Call when selection list changes
    void syncToSelection();
    void clearAll();

private:
    SceneManager* m_sceneManager;
    SoSeparator* m_occRoot;
    std::vector<std::shared_ptr<OCCGeometry>>* m_selectedGeometries;
    bool m_enabled{false};
    SelectionOutlineStyle m_style{};

    std::map<std::string, std::unique_ptr<DynamicSilhouetteRenderer>> m_renderersByName;
};


