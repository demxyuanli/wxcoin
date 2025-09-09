#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include "NormalValidator.h"

class OCCViewer;

/**
 * @brief Command listener for visualizing face normals
 */
class ShowNormalsListener : public CommandListener {
public:
    explicit ShowNormalsListener(OCCViewer* viewer);
    
    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    
    std::string getListenerName() const override { return "ShowNormalsListener"; }

private:
    OCCViewer* m_viewer;
    
    /**
     * @brief Create normal visualization for a geometry
     * @param geometry The geometry to visualize normals for
     * @param normalLength Length of normal vectors to display
     * @param showCorrect Show normals that are correctly oriented
     * @param showIncorrect Show normals that are incorrectly oriented
     */
    void createNormalVisualization(std::shared_ptr<class OCCGeometry> geometry, 
                                  double normalLength = 1.0,
                                  bool showCorrect = true,
                                  bool showIncorrect = true);
    
    /**
     * @brief Get normal validation result for a geometry
     * @param geometry The geometry to analyze
     * @return Validation result
     */
    NormalValidationResult getNormalValidation(std::shared_ptr<class OCCGeometry> geometry);
};