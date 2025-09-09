#pragma once

#include <string>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>

/**
 * @brief Result structure for normal validation
 */
struct NormalValidationResult {
    bool success = false;
    std::string errorMessage;
    
    int totalFaces = 0;
    int facesWithNormals = 0;
    int facesWithCorrectNormals = 0;
    int facesWithIncorrectNormals = 0;
    int facesNeedingCorrection = 0;
    
    double correctnessPercentage = 0.0;
    double qualityScore = 0.0;
    double validationTime = 0.0;
    
    std::vector<std::string> issues;
    std::vector<std::string> recommendations;
    
    void calculateMetrics() {
        if (totalFaces > 0) {
            correctnessPercentage = (facesWithCorrectNormals * 100.0) / totalFaces;
            qualityScore = static_cast<double>(facesWithCorrectNormals) / totalFaces;
        }
    }
};

/**
 * @brief Utility class for validating and correcting face normals
 */
class NormalValidator {
public:
    /**
     * @brief Validate normals for a single shape
     * @param shape The shape to validate
     * @param shapeName Name of the shape for logging
     * @return Validation result
     */
    static NormalValidationResult validateNormals(const TopoDS_Shape& shape, const std::string& shapeName = "Unknown");
    
    /**
     * @brief Validate normals for multiple geometries
     * @param geometries Vector of geometries to validate
     * @return Combined validation result
     */
    static NormalValidationResult validateNormals(const std::vector<std::shared_ptr<class OCCGeometry>>& geometries);
    
    /**
     * @brief Automatically correct normals for a shape
     * @param shape The shape to correct
     * @param shapeName Name of the shape for logging
     * @return Corrected shape
     */
    static TopoDS_Shape autoCorrectNormals(const TopoDS_Shape& shape, const std::string& shapeName = "Unknown");
    
    /**
     * @brief Check if a shape has consistent normals
     * @param shape The shape to check
     * @return true if normals are consistent
     */
    static bool hasConsistentNormals(const TopoDS_Shape& shape);
    
    /**
     * @brief Get normal quality score for a shape
     * @param shape The shape to analyze
     * @return Quality score (0.0 to 1.0)
     */
    static double getNormalQualityScore(const TopoDS_Shape& shape);

    /**
     * @brief Calculate the center point of a shape
     * @param shape The shape to analyze
     * @return Center point
     */
    static gp_Pnt calculateShapeCenter(const TopoDS_Shape& shape);
    
    /**
     * @brief Check if a face normal points outward
     * @param face The face to check
     * @param shapeCenter Center of the shape
     * @return true if normal points outward
     */
    static bool isNormalOutward(const TopoDS_Face& face, const gp_Pnt& shapeCenter);

private:
    /**
     * @brief Analyze a single face normal
     * @param face The face to analyze
     * @param shapeCenter Center of the shape
     * @return true if face has valid normal
     */
    static bool analyzeFaceNormal(const TopoDS_Face& face, const gp_Pnt& shapeCenter);
    
    /**
     * @brief Get detailed face normal information
     * @param face The face to analyze
     * @param shapeCenter Center of the shape
     * @return Information string
     */
    static std::string getFaceNormalInfo(const TopoDS_Face& face, const gp_Pnt& shapeCenter);
    
    /**
     * @brief Generate recommendations based on validation results
     * @param result The validation result to update
     */
    static void generateRecommendations(NormalValidationResult& result);
    
    /**
     * @brief Correct face normals for a shape
     * @param shape The shape to correct
     * @param shapeCenter Center of the shape
     * @param shapeName Name for logging
     * @return Corrected shape
     */
    static TopoDS_Shape correctFaceNormals(const TopoDS_Shape& shape, const gp_Pnt& shapeCenter, const std::string& shapeName);
    
    /**
     * @brief Reverse a face orientation
     * @param face The face to reverse
     * @return Reversed face
     */
    static TopoDS_Face reverseFace(const TopoDS_Face& face);
    
    /**
     * @brief Rebuild shape with corrected faces
     * @param originalShape The original shape
     * @param correctedFaces Vector of corrected faces
     * @return Rebuilt shape
     */
    static TopoDS_Shape rebuildShapeWithCorrectedFaces(const TopoDS_Shape& originalShape, const std::vector<TopoDS_Face>& correctedFaces);
};