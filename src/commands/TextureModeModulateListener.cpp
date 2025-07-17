#include "TextureModeModulateListener.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>

TextureModeModulateListener::TextureModeModulateListener(wxFrame* frame, OCCViewer* viewer)
    : m_frame(frame)
    , m_viewer(viewer)
{
}

CommandResult TextureModeModulateListener::executeCommand(const std::string& commandType,
                                                        const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_viewer) {
        wxMessageBox("OCCViewer not available", "Error", wxOK | wxICON_ERROR);
        return CommandResult(false, "OCCViewer not available", commandType);
    }
    
    // Update RenderingConfig - this will trigger notification to update selected geometries
    RenderingConfig& config = RenderingConfig::getInstance();
    
    // Enable texture and set a visible color texture for Modulate mode
    config.setSelectedTextureEnabled(true);
    config.setSelectedTextureColor(Quantity_Color(1.0, 1.0, 0.0, Quantity_TOC_RGB)); // Bright Yellow
    config.setSelectedTextureIntensity(0.7); // Moderate intensity for modulation effect
    config.setSelectedTextureMode(RenderingConfig::TextureMode::Modulate);
    
    // Set material to a base color that will be modulated
    config.setSelectedMaterialDiffuseColor(Quantity_Color(0.5, 0.2, 0.8, Quantity_TOC_RGB)); // Purple base
    config.setSelectedMaterialTransparency(0.0); // No transparency
    
    // Force notification to ensure selected geometries are updated
    config.notifySettingsChanged();
    
    // Also directly update geometries as a fallback
    if (m_viewer) {
        auto geometries = m_viewer->getAllGeometry();
        LOG_INF_S("Directly updating " + std::to_string(geometries.size()) + " geometries for Modulate mode");
        for (auto& geometry : geometries) {
            geometry->updateFromRenderingConfig();
        }
        
        // Force immediate refresh after direct update
        if (m_frame) {
            m_frame->CallAfter([this]() {
                m_frame->Refresh(true);
                m_frame->Update();
                LOG_INF_S("Forced delayed refresh for Modulate mode");
            });
        }
    }
    
    LOG_INF_S("Texture mode set to Modulate via RenderingConfig");
    
    return CommandResult(true, "Texture mode changed to Modulate", commandType);
}

bool TextureModeModulateListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::TextureModeModulate);
}

std::string TextureModeModulateListener::getListenerName() const
{
    return "TextureModeModulateListener";
} 