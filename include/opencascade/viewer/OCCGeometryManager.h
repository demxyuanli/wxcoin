#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations
class OCCGeometry;
class SceneManager;
class SoSeparator;
struct MeshParameters;

/**
 * @brief OCCGeometryManager - 几何体管理器
 *
 * 负责几何体的添加、删除、查找和管理
 */
class OCCGeometryManager
{
public:
    OCCGeometryManager(SceneManager* sceneManager);
    virtual ~OCCGeometryManager() = default;

    // 几何体管理
    virtual void addGeometry(std::shared_ptr<OCCGeometry> geometry);
    virtual void removeGeometry(std::shared_ptr<OCCGeometry> geometry);
    virtual void removeGeometry(const std::string& name);
    virtual void clearAll();

    virtual std::shared_ptr<OCCGeometry> findGeometry(const std::string& name);
    virtual std::vector<std::shared_ptr<OCCGeometry>> getAllGeometry() const;
    virtual std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometries() const;

    // 几何体属性设置
    virtual void setGeometryVisible(const std::string& name, bool visible);
    virtual void setGeometrySelected(const std::string& name, bool selected);
    virtual void setGeometryColor(const std::string& name, const Quantity_Color& color);
    virtual void setGeometryTransparency(const std::string& name, double transparency);

    // 批量操作
    virtual void hideAll();
    virtual void showAll();
    virtual void selectAll();
    virtual void deselectAll();
    virtual void setAllColor(const Quantity_Color& color);

    // 视图操作
    virtual void fitAll();
    virtual void fitGeometry(const std::string& name);

    // 选择操作
    virtual std::shared_ptr<OCCGeometry> pickGeometry(int x, int y);
    virtual std::shared_ptr<OCCGeometry> pickGeometryAtScreen(const wxPoint& screenPos);

protected:
    SceneManager* m_sceneManager;
    std::vector<std::shared_ptr<OCCGeometry>> m_geometries;
    std::vector<std::shared_ptr<OCCGeometry>> m_selectedGeometries;
    std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>> m_nodeToGeom;
};
