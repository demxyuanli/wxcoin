#include "TextureModeDecalListener.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>

TextureModeDecalListener::TextureModeDecalListener(wxFrame* frame, OCCViewer* viewer)
    : m_frame(frame)
    , m_viewer(viewer)
{
}

CommandResult TextureModeDecalListener::executeCommand(const std::string& commandType,
                                                     const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_viewer) {
        wxMessageBox("OCCViewer not available", "Error", wxOK | wxICON_ERROR);
        return CommandResult(false, "OCCViewer not available", commandType);
    }
    
    // Check if any objects are selected
    auto selectedGeometries = m_viewer->getSelectedGeometries();
    bool hasSelection = !selectedGeometries.empty();
    
    LOG_INF_S("TextureModeDecalListener: " + std::to_string(selectedGeometries.size()) + " objects selected");
    
    // Update RenderingConfig - this will trigger notification to update geometries
    RenderingConfig& config = RenderingConfig::getInstance();
    
    if (hasSelection) {
        // Apply settings to selected objects only
        LOG_INF_S("Applying Decal texture mode to " + std::to_string(selectedGeometries.size()) + " selected objects");
        
        // Enable texture and set a visible color texture for Decal mode
        config.setSelectedTextureEnabled(true);
        config.setSelectedTextureColor(Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB)); // Bright Red
        config.setSelectedTextureIntensity(1.0); // Full intensity for maximum visibility
        config.setSelectedTextureMode(RenderingConfig::TextureMode::Decal);
        
        // Set material to a contrasting color to show Decal effect
        config.setSelectedMaterialDiffuseColor(Quantity_Color(0.2, 0.8, 0.2, Quantity_TOC_RGB)); // Green base
        config.setSelectedMaterialTransparency(0.0); // No transparency for clear contrast
    } else {
        // Apply settings to all objects
        LOG_INF_S("No objects selected, applying Decal texture mode to all objects");
        
        // Enable texture and set a visible color texture for Decal mode
        config.setTextureEnabled(true);
        config.setTextureColor(Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB)); // Bright Red
        config.setTextureIntensity(1.0); // Full intensity for maximum visibility
        config.setTextureMode(RenderingConfig::TextureMode::Decal);
        
        // Set material to a contrasting color to show Decal effect
        config.setMaterialDiffuseColor(Quantity_Color(0.2, 0.8, 0.2, Quantity_TOC_RGB)); // Green base
        config.setMaterialTransparency(0.0); // No transparency for clear contrast
    }
    
    // Force notification to ensure geometries are updated
    LOG_INF_S("About to call notifySettingsChanged() for Decal mode");
    config.notifySettingsChanged();
    LOG_INF_S("notifySettingsChanged() called for Decal mode");
    
    // Also directly update geometries as a fallback
    if (m_viewer) {
        auto geometries = hasSelection ? selectedGeometries : m_viewer->getAllGeometry();
        LOG_INF_S("Directly updating " + std::to_string(geometries.size()) + " geometries for Decal mode");
        for (auto& geometry : geometries) {
            geometry->updateFromRenderingConfig();
        }
        
        // Force immediate refresh after direct update
        if (m_frame) {
            // Use CallAfter to ensure the update happens after the current event
            m_frame->CallAfter([this]() {
                // Force a complete refresh cycle
                m_frame->Refresh(true);
                m_frame->Update();
                LOG_INF_S("Forced delayed refresh for Decal mode");
            });
        }
    }
    
    LOG_INF_S("Texture mode set to Decal via RenderingConfig");
    
    // Add test feedback
    std::string feedbackMessage = "Decal texture mode applied to " + 
        (hasSelection ? std::to_string(selectedGeometries.size()) + " selected objects" : "all objects") +
        " (Red texture on Green base)";
    
    // Show feedback to user
    if (m_frame) {
        wxMessageBox(feedbackMessage, "Texture Mode Applied", wxOK | wxICON_INFORMATION);
    }
    
    // Show detailed test feedback in logs
    config.showTestFeedback();
    
    return CommandResult(true, feedbackMessage, commandType);
}

bool TextureModeDecalListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::TextureModeDecal);
}

std::string TextureModeDecalListener::getListenerName() const
{
    return "TextureModeDecalListener";
} 