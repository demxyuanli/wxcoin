#pragma once

#include <vector>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "rendering/GeometryProcessor.h"

/**
 * @brief Base class for edge extractors
 * 
 * Defines the common interface for all edge extraction algorithms
 */
class BaseEdgeExtractor {
public:
    virtual ~BaseEdgeExtractor() = default;
    
    /**
     * @brief Extract edge points from shape
     * @param shape Input shape
     * @param params Extraction parameters (specific to each extractor type)
     * @return Vector of edge points
     */
    virtual std::vector<gp_Pnt> extract(const TopoDS_Shape& shape, const void* params = nullptr) = 0;
    
    /**
     * @brief Check if this extractor can handle the given shape
     * @param shape Input shape
     * @return True if extractor is applicable
     */
    virtual bool canExtract(const TopoDS_Shape& shape) const = 0;
    
    /**
     * @brief Get extractor name for debugging
     * @return Extractor type name
     */
    virtual const char* getName() const = 0;
};

/**
 * @brief Template base for typed edge extractors
 */
template<typename ParamsType>
class TypedEdgeExtractor : public BaseEdgeExtractor {
public:
    virtual std::vector<gp_Pnt> extract(const TopoDS_Shape& shape, const void* params = nullptr) override {
        const ParamsType* typedParams = params ? static_cast<const ParamsType*>(params) : nullptr;
        return extractTyped(shape, typedParams);
    }
    
protected:
    virtual std::vector<gp_Pnt> extractTyped(const TopoDS_Shape& shape, const ParamsType* params) = 0;
};

