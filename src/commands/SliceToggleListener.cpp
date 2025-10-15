#include "SliceToggleListener.h"
#include "OCCViewer.h"
#include "SliceParamDialog.h"
#include "SceneManager.h"
#include "Canvas.h"
#include <Inventor/SbLinear.h>

SliceToggleListener::SliceToggleListener(OCCViewer* viewer)
	: m_viewer(viewer) {
}

SliceToggleListener::~SliceToggleListener() {
	if (m_paramDialog) {
		m_paramDialog->Destroy();
		m_paramDialog = nullptr;
	}
}

CommandResult SliceToggleListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
	
	// Check if there are any geometries in the scene
	if (m_viewer->getAllGeometry().empty()) {
		return CommandResult(false, "No geometry available for slicing", commandType);
	}
	
	bool wasEnabled = m_viewer->isSliceEnabled();
	
	if (!wasEnabled) {
		// Enable slice and show parameter dialog
		m_viewer->setSliceEnabled(true);
		
		// Create and show parameter dialog if not already shown
		if (!m_paramDialog) {
			auto sceneManager = m_viewer->getSceneManager();
			wxWindow* parentWindow = sceneManager ? dynamic_cast<wxWindow*>(sceneManager->getCanvas()) : nullptr;
			
			if (parentWindow) {
				m_paramDialog = new SliceParamDialog(parentWindow, m_viewer);
				
				// Set current values
				SbVec3f currentColor = m_viewer->getSlicePlaneColor();
				m_paramDialog->setPlaneColor(wxColour(
					static_cast<unsigned char>(currentColor[0] * 255),
					static_cast<unsigned char>(currentColor[1] * 255),
					static_cast<unsigned char>(currentColor[2] * 255)
				));
				m_paramDialog->setPlaneOpacity(0.15); // Default 85% transparency (15% opacity)
				m_paramDialog->setShowSectionContours(m_viewer->isShowSectionContours());
				m_paramDialog->setSliceOffset(static_cast<double>(m_viewer->getSliceOffset()));
				
				// Handle dialog close event
				m_paramDialog->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
					if (m_paramDialog) {
						m_paramDialog->Destroy();
						m_paramDialog = nullptr;
					}
					event.Skip();
				});
				
				m_paramDialog->ShowAtCanvasTopLeft();
			}
		} else {
			m_paramDialog->Show(true);
			m_paramDialog->Raise();
		}
		
		return CommandResult(true, "Slice enabled with parameters", commandType);
	} else {
		// Disable slice and hide dialog
		m_viewer->setSliceEnabled(false);
		if (m_paramDialog) {
			m_paramDialog->Close(true);
		}
		return CommandResult(true, "Slice disabled", commandType);
	}
}

bool SliceToggleListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::SliceToggle);
}

bool SliceToggleListener::isInDragMode() const {
	return m_paramDialog ? m_paramDialog->isDragMode() : false;
}