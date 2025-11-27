#include <wx/wx.h>
#include <wx/display.h>

#include "MainApplication.h"
#include "config/ConfigManager.h"
#include "config/ConstantsConfig.h"
#include "config/FontManager.h"
#include "interfaces/DefaultSubsystemFactory.h"
#include "interfaces/ServiceLocator.h"
#include "logger/Logger.h"
#include "rendering/RenderingToolkitAPI.h"
#include "FlatFrameDocking.h"
#include "SplashScreen.h"

#include <Inventor/SoDB.h>

// Define a new app class that uses docking
class MainApplicationDocking : public MainApplication {
public:
	virtual bool OnInit() override;
};

wxIMPLEMENT_APP(MainApplicationDocking);

bool MainApplicationDocking::OnInit()
{
	ConfigManager& cm = ConfigManager::getInstance();
	bool initialConfigLoaded = cm.initialize("");

	SplashScreen splash;
	size_t stageProgress = 0;

	if (initialConfigLoaded) {
		splash.ReloadFromConfig(stageProgress);
	}

	auto showStageMessage = [&](const char* fallback) {
		if (!splash.ShowNextConfiguredMessage()) {
			splash.ShowMessage(wxString::FromUTF8(fallback));
		}
		++stageProgress;
	};

	// Initialize Coin3D first - this must happen before any SoBase-derived objects are created
	showStageMessage("Initializing Coin3D...");
	try {
		SoDB::init();
		LOG_INF("Coin3D initialized successfully", "MainApplicationDocking");
	}
	catch (const std::exception& e) {
		splash.Finish();
		wxMessageBox("Failed to initialize Coin3D library: " + wxString(e.what()), "Initialization Error", wxOK | wxICON_ERROR);
		return false;
	}

	// Initialize rendering toolkit
	showStageMessage("Initializing rendering toolkit...");
	try {
		if (!RenderingToolkitAPI::initialize()) {
			splash.Finish();
			wxMessageBox("Failed to initialize rendering toolkit", "Initialization Error", wxOK | wxICON_ERROR);
			return false;
		}
		LOG_INF("Rendering toolkit initialized successfully", "MainApplicationDocking");
	}
	catch (const std::exception& e) {
		splash.Finish();
		wxMessageBox("Failed to initialize rendering toolkit: " + wxString(e.what()), "Initialization Error", wxOK | wxICON_ERROR);
		return false;
	}

	// Set default subsystem factory (can be replaced by tests or other compositions)
	showStageMessage("Configuring subsystem factory...");
	static DefaultSubsystemFactory s_factory;
	ServiceLocator::setFactory(&s_factory);

	showStageMessage("Loading configuration...");
	if (!cm.isInitialized()) {
		bool configInitialized = cm.initialize("");
		if (!configInitialized) {
			configInitialized = cm.initialize("./config.ini");
		}
		if (!configInitialized) {
			splash.Finish();
			wxMessageBox("Cannot find config.ini in config/ or current directory",
				"Configuration Error", wxOK | wxICON_ERROR);
			return false;
		}
		else {
			splash.ReloadFromConfig(stageProgress);
		}
	}

	ConstantsConfig::getInstance().initialize(cm);

	// Initialize FontManager after ConfigManager
	showStageMessage("Initializing font subsystem...");
	FontManager& fontManager = FontManager::getInstance();
	if (!fontManager.initialize("config/config.ini")) { // Pass the path to config.ini
		splash.Finish();
		wxMessageBox("Failed to initialize font manager", "Initialization Error", wxOK | wxICON_ERROR);
		return false;
	}

	showStageMessage("Preparing user interface...");

	// Create main frame with docking system
	// Get screen client area (work area excluding taskbar)
	wxDisplay display;
	wxRect clientRect = display.GetClientArea();
	wxSize clientSize = clientRect.GetSize();

	FlatFrameDocking* frame = new FlatFrameDocking("CAD VisBird - Docking Edition",
		wxPoint(clientRect.GetLeft(), clientRect.GetTop()),
		clientSize);

	showStageMessage("Starting application...");
	frame->Show(true);
	SetTopWindow(frame);

	splash.Finish();

	LOG_INF("Application started with docking system", "MainApplicationDocking");

	return true;
}