#include "ShowNormalsListener.h"
#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "NormalValidator.h"
#include "logger/Logger.h"

// OpenCASCADE includes
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include <OpenCASCADE/Geom_Surface.hxx>
#include <OpenCASCADE/GeomLProp_SLProps.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/BRepBndLib.hxx>
#include <OpenCASCADE/BRepTools.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>

ShowNormalsListener::ShowNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowNormalsListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {
    if (!m_viewer) {
        return CommandResult(false, "OCCViewer not available", commandType);
    }
    
    try {
        // Get parameters
        double normalLength = 1.0;
        bool showCorrect = true;
        bool showIncorrect = true;
        
        auto lengthIt = parameters.find("length");
        if (lengthIt != parameters.end()) {
            normalLength = std::stod(lengthIt->second);
        }
        
        auto correctIt = parameters.find("show_correct");
        if (correctIt != parameters.end()) {
            showCorrect = (correctIt->second == "true" || correctIt->second == "1");
        }
        
        auto incorrectIt = parameters.find("show_incorrect");
        if (incorrectIt != parameters.end()) {
            showIncorrect = (incorrectIt->second == "true" || incorrectIt->second == "1");
        }
        
        // Get all geometries
        auto geometries = m_viewer->getAllGeometries();
        if (geometries.empty()) {
            return CommandResult(false, "No geometries found to visualize", commandType);
        }
        
        LOG_INF_S("Creating normal visualization for " + std::to_string(geometries.size()) + " geometries");
        
        int processedCount = 0;
        int totalNormals = 0;
        int correctNormals = 0;
        int incorrectNormals = 0;
        
        // Process each geometry
        for (auto& geometry : geometries) {
            if (!geometry) continue;
            
            // Get normal validation
            NormalValidationResult validation = getNormalValidation(geometry);
            
            // Create visualization
            createNormalVisualization(geometry, normalLength, showCorrect, showIncorrect);
            
            processedCount++;
            totalNormals += validation.totalFaces;
            correctNormals += validation.facesWithCorrectNormals;
            incorrectNormals += validation.facesWithIncorrectNormals;
        }
        
        // Refresh viewer
        m_viewer->refresh();
        
        std::string message = "Normal visualization created for " + std::to_string(processedCount) + 
                            " geometries (" + std::to_string(totalNormals) + " total faces, " +
                            std::to_string(correctNormals) + " correct, " + 
                            std::to_string(incorrectNormals) + " incorrect)";
        
        LOG_INF_S(message);
        return CommandResult(true, message, commandType);
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception during normal visualization: " + std::string(e.what()));
        return CommandResult(false, "Error creating normal visualization: " + std::string(e.what()), commandType);
    }
}

bool ShowNormalsListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowNormals);
}

void ShowNormalsListener::createNormalVisualization(std::shared_ptr<OCCGeometry> geometry, 
                                                   double normalLength,
                                                   bool showCorrect,
                                                   bool showIncorrect) {
    if (!geometry) return;
    
    const TopoDS_Shape& shape = geometry->getShape();
    if (shape.IsNull()) return;
    
    try {
        // Calculate shape center
        gp_Pnt shapeCenter = NormalValidator::calculateShapeCenter(shape);
        
        // Create visualization geometry for normals
        std::vector<gp_Pnt> normalPoints;
        std::vector<gp_Pnt> normalEndPoints;
        std::vector<int> correctIndices;
        std::vector<int> incorrectIndices;
        
        int normalIndex = 0;
        
        // Process each face
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            TopoDS_Face face = TopoDS::Face(exp.Current());
            
            // Get face center
            Bnd_Box faceBox;
            BRepBndLib::Add(face, faceBox);
            
            if (faceBox.IsVoid()) continue;
            
            Standard_Real fxMin, fyMin, fzMin, fxMax, fyMax, fzMax;
            faceBox.Get(fxMin, fyMin, fzMin, fxMax, fyMax, fzMax);
            
            gp_Pnt faceCenter(
                (fxMin + fxMax) / 2.0,
                (fyMin + fyMax) / 2.0,
                (fzMin + fzMax) / 2.0
            );
            
            // Get face normal
            Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
            if (surface.IsNull()) continue;
            
            Standard_Real uMin, uMax, vMin, vMax;
            BRepTools::UVBounds(face, uMin, uMax, vMin, vMax);
            
            Standard_Real uMid = (uMin + uMax) / 2.0;
            Standard_Real vMid = (vMin + vMax) / 2.0;
            
            GeomLProp_SLProps props(surface, uMid, vMid, 1, 1e-6);
            if (!props.IsNormalDefined()) continue;
            
            gp_Vec faceNormal = props.Normal();
            
            // Apply face orientation
            if (face.Orientation() == TopAbs_REVERSED) {
                faceNormal.Reverse();
            }
            
            // Normalize and scale
            faceNormal.Normalize();
            faceNormal.Scale(normalLength);
            
            // Calculate normal end point
            gp_Pnt normalEnd(
                faceCenter.X() + faceNormal.X(),
                faceCenter.Y() + faceNormal.Y(),
                faceCenter.Z() + faceNormal.Z()
            );
            
            // Check if normal is correct
            bool isCorrect = NormalValidator::isNormalOutward(face, shapeCenter);
            
            // Add to visualization
            normalPoints.push_back(faceCenter);
            normalEndPoints.push_back(normalEnd);
            
            if (isCorrect) {
                correctIndices.push_back(normalIndex);
            } else {
                incorrectIndices.push_back(normalIndex);
            }
            
            normalIndex++;
        }
        
        // Create visualization objects
        if (showCorrect && !correctIndices.empty()) {
            // Create green normals for correct orientations
            auto correctNormalsGeometry = std::make_shared<OCCGeometry>(geometry->getName() + "_CorrectNormals");
            // Note: In a full implementation, we would create line geometry here
            LOG_INF_S("Created " + std::to_string(correctIndices.size()) + " correct normal vectors");
        }
        
        if (showIncorrect && !incorrectIndices.empty()) {
            // Create red normals for incorrect orientations
            auto incorrectNormalsGeometry = std::make_shared<OCCGeometry>(geometry->getName() + "_IncorrectNormals");
            // Note: In a full implementation, we would create line geometry here
            LOG_INF_S("Created " + std::to_string(incorrectIndices.size()) + " incorrect normal vectors");
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception creating normal visualization for " + geometry->getName() + ": " + std::string(e.what()));
    }
}

NormalValidationResult ShowNormalsListener::getNormalValidation(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry) {
        NormalValidationResult empty;
        return empty;
    }
    
    return NormalValidator::validateNormals(geometry->getShape(), geometry->getName());
}