#pragma once

#include "OCCGeometryCore.h"
#include <vector>
#include <memory>

// Forward declarations
struct MeshParameters;

/**
 * @brief FaceIndexMapping - 面索引映射结构
 */
struct FaceIndexMapping {
    int geometryFaceId;
    std::vector<int> triangleIndices;

    FaceIndexMapping(int faceId) : geometryFaceId(faceId) {}
};

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

    // 面索引映射接口实现
    bool hasFaceIndexMapping() const override;
    void buildFaceIndexMapping(const MeshParameters& params) override;
    int getGeometryFaceIdForTriangle(int triangleIndex) const override;
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const override;

protected:
    // 面索引映射构建
    virtual void buildFaceIndexMappingInternal(const MeshParameters& params);

private:
    std::vector<FaceIndexMapping> m_faceIndexMappings;
    bool m_hasFaceIndexMapping;
};
