#include "TextureModeReplaceListener.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>

TextureModeReplaceListener::TextureModeReplaceListener(wxFrame* frame, OCCViewer* viewer)
    : m_frame(frame)
    , m_viewer(viewer)
{
}

CommandResult TextureModeReplaceListener::executeCommand(const std::string& commandType,
                                                       const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_viewer) {
        wxMessageBox("OCCViewer not available", "Error", wxOK | wxICON_ERROR);
        return CommandResult(false, "OCCViewer not available", commandType);
    }
    
    // Update RenderingConfig - this will trigger notification to update selected geometries
    RenderingConfig& config = RenderingConfig::getInstance();
    
    // Enable texture and set a visible color texture for Replace mode
    config.setSelectedTextureEnabled(true);
    config.setSelectedTextureColor(Quantity_Color(0.0, 1.0, 1.0, Quantity_TOC_RGB)); // Bright Cyan
    config.setSelectedTextureIntensity(1.0); // Full intensity to completely replace base color
    config.setSelectedTextureMode(RenderingConfig::TextureMode::Replace);
    
    // Set material to a different color that should be replaced
    config.setSelectedMaterialDiffuseColor(Quantity_Color(0.8, 0.3, 0.3, Quantity_TOC_RGB)); // Reddish base
    config.setSelectedMaterialTransparency(0.0); // No transparency
    
    // Force notification to ensure selected geometries are updated
    config.notifySettingsChanged();
    
    // Also directly update geometries as a fallback
    if (m_viewer) {
        auto geometries = m_viewer->getAllGeometry();
        LOG_INF_S("Directly updating " + std::to_string(geometries.size()) + " geometries for Replace mode");
        for (auto& geometry : geometries) {
            geometry->updateFromRenderingConfig();
        }
        
        // Force immediate refresh after direct update
        if (m_frame) {
            m_frame->CallAfter([this]() {
                m_frame->Refresh(true);
                m_frame->Update();
                LOG_INF_S("Forced delayed refresh for Replace mode");
            });
        }
    }
    
    LOG_INF_S("Texture mode set to Replace via RenderingConfig");
    
    return CommandResult(true, "Texture mode changed to Replace", commandType);
}

bool TextureModeReplaceListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::TextureModeReplace);
}

std::string TextureModeReplaceListener::getListenerName() const
{
    return "TextureModeReplaceListener";
} 