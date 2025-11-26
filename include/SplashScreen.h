#pragma once

#include <cstddef>
#include <wx/string.h>
#include <vector>

class wxBoxSizer;
class wxFrame;
class wxPanel;
class wxStaticBitmap;
class wxStaticText;
class SplashImagePanel;

class SplashScreen {
public:
	SplashScreen();
	~SplashScreen();

	void ShowMessage(const wxString& message);
	bool ShowConfiguredMessage(const wxString& key);
	bool ShowNextConfiguredMessage();
	void ReloadFromConfig(size_t messagesAlreadyShown);
	void Finish();

	int GetBackgroundHeight() const;

private:
	SplashScreen(const SplashScreen&) = delete;
	SplashScreen& operator=(const SplashScreen&) = delete;

	void initializeFrame(const wxString& title);
	void loadConfiguredMessages();
	void loadBackgroundImage();
	void loadBackgroundImageFromPath(const wxString& imagePath);

	wxFrame* m_frame;
	wxPanel* m_panel;
	wxBoxSizer* m_panelSizer;
	SplashImagePanel* m_backgroundPanel;
	wxStaticText* m_messageLabel;
	wxStaticText* m_titleLabel;
	bool m_finished;

	std::vector<wxString> m_configMessages;
	size_t m_nextMessageIndex;
	bool m_configLoaded;

	wxString m_selectedBackgroundImage;  // Store the selected background image path to avoid re-selection
};
