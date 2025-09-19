#include "FlatFrame.h"
#include "flatui/FlatUIStatusBar.h"

void FlatFrame::OnThemeChanged(wxCommandEvent& event)
{
	wxString themeName = event.GetString();
	SetStatusText("Theme changed to: " + themeName, 0);
	FlatUIFrame::OnThemeChanged(event);
	if (m_searchPanel) {
		m_searchPanel->SetBackgroundColour(CFG_COLOUR("SearchPanelBgColour"));
	}
	if (m_searchCtrl) {
		m_searchCtrl->SetBackgroundColour(CFG_COLOUR("SearchCtrlBgColour"));
		m_searchCtrl->SetForegroundColour(CFG_COLOUR("SearchCtrlFgColour"));
	}
	if (m_ribbon) {
		m_ribbon->SetTabBorderColour(CFG_COLOUR("BarTabBorderColour"));
		m_ribbon->SetActiveTabBackgroundColour(CFG_COLOUR("BarActiveTabBgColour"));
		m_ribbon->SetActiveTabTextColour(CFG_COLOUR("BarActiveTextColour"));
		m_ribbon->SetInactiveTabTextColour(CFG_COLOUR("BarInactiveTextColour"));
		m_ribbon->SetTabBorderTopColour(CFG_COLOUR("BarTabBorderTopColour"));
		m_ribbon->Refresh(true);
		m_ribbon->Update();
	}
	
	// Refresh SVG icons in bitmap buttons
	if (m_searchButton) {
		m_searchButton->SetBitmap(SvgIconManager::GetInstance().GetIconBitmap("search", wxSize(16, 16)));
		m_searchButton->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	}
	if (m_userButton) {
		m_userButton->SetBitmap(SvgIconManager::GetInstance().GetIconBitmap("user", wxSize(16, 16)));
		m_userButton->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	}
	if (m_settingsButton) {
		m_settingsButton->SetBitmap(SvgIconManager::GetInstance().GetIconBitmap("settings", wxSize(16, 16)));
		m_settingsButton->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	}
	Refresh(true);
	Update();
}

void FlatFrame::appendMessage(const wxString& message) {
	if (m_messageOutput) {
		wxString currentText = m_messageOutput->GetValue();
		if (!currentText.IsEmpty()) currentText += "\n";
		currentText += message;
		m_messageOutput->SetValue(currentText);
		m_messageOutput->ShowPosition(m_messageOutput->GetLastPosition());
	} else {
		// Fallback: use status bar if message output is not available
		SetStatusText(message, 0);
	}
}