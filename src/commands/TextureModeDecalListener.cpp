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
    
    // Update RenderingConfig - this will trigger notification to update selected geometries
    RenderingConfig& config = RenderingConfig::getInstance();
    
    // Enable texture and set a visible color texture for Decal mode
    config.setSelectedTextureEnabled(true);
    config.setSelectedTextureColor(Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB)); // Bright Red
    config.setSelectedTextureIntensity(1.0); // Full intensity for maximum visibility
    config.setSelectedTextureMode(RenderingConfig::TextureMode::Decal);
    
    // Set material to a contrasting color to show Decal effect
    config.setSelectedMaterialDiffuseColor(Quantity_Color(0.2, 0.8, 0.2, Quantity_TOC_RGB)); // Green base
    config.setSelectedMaterialTransparency(0.0); // No transparency for clear contrast
    
    // Force notification to ensure selected geometries are updated
    LOG_INF_S("About to call notifySettingsChanged() for Decal mode");
    config.notifySettingsChanged();
    LOG_INF_S("notifySettingsChanged() called for Decal mode");
    
    // Also directly update geometries as a fallback
    if (m_viewer) {
        auto geometries = m_viewer->getAllGeometry();
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
    
    return CommandResult(true, "Texture mode changed to Decal", commandType);
}

bool TextureModeDecalListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::TextureModeDecal);
}

std::string TextureModeDecalListener::getListenerName() const
{
    return "TextureModeDecalListener";
} 