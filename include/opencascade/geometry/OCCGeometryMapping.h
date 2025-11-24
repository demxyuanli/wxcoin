#pragma once

#include "OCCGeometryCore.h"
#include <vector>
#include <memory>

// Forward declarations
struct MeshParameters;


/**
 * @brief OCCGeometryMapping - 面索引映射功能扩展类
 *
 * 提供几何体的面索引映射功能，用于面选择和交互
 */
class OCCGeometryMapping : public OCCGeometryCore
{
public:
    OCCGeometryMapping(const std::string& name);
    virtual ~OCCGeometryMapping() = default;

    // 面索引映射接口实现（现在使用domain系统）
    bool hasFaceIndexMapping() const override { return hasFaceDomainMapping(); }
    void buildFaceIndexMapping(const MeshParameters& params) override;
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const override;

protected:
    // 面索引映射构建
    virtual void buildFaceIndexMappingInternal(const MeshParameters& params);

private:
};
