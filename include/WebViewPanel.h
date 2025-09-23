#pragma once

#include <wx/wx.h>
#include <wx/webview.h>
#include <wx/url.h>

class WebViewPanel : public wxPanel {
public:
    WebViewPanel(wxWindow* parent, wxWindowID id = wxID_ANY, 
                 const wxPoint& pos = wxDefaultPosition, 
                 const wxSize& size = wxDefaultSize);
    virtual ~WebViewPanel();

    // WebView operations
    void LoadURL(const wxString& url);
    void LoadHTML(const wxString& html);
    void Reload();
    void Stop();
    void GoBack();
    void GoForward();
    bool CanGoBack() const;
    bool CanGoForward() const;
    
    // Navigation controls
    void OnBack(wxCommandEvent& event);
    void OnForward(wxCommandEvent& event);
    void OnReload(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnNavigate(wxCommandEvent& event);
    
    // WebView events
    void OnWebViewLoaded(wxWebViewEvent& event);
    void OnWebViewError(wxWebViewEvent& event);
    void OnWebViewTitleChanged(wxWebViewEvent& event);

    // Window events
    void OnSize(wxSizeEvent& event);

private:
    void CreateControls();
    void UpdateNavigationButtons();
    
    wxWebView* m_webView;
    wxButton* m_backBtn;
    wxButton* m_forwardBtn;
    wxButton* m_reloadBtn;
    wxButton* m_stopBtn;
    wxTextCtrl* m_urlCtrl;
    wxStaticText* m_statusText;
    
    wxString m_currentURL;
    wxString m_currentTitle;

    wxDECLARE_EVENT_TABLE();
};
