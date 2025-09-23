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
    
    // Navigation controls (for external use)
    void OnBack(wxCommandEvent& event);
    void OnForward(wxCommandEvent& event);
    void OnReload(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnNavigate(wxCommandEvent& event);
    
    // WebView events
    void OnWebViewLoaded(wxWebViewEvent& event);
    void OnWebViewError(wxWebViewEvent& event);

    // Window events
    void OnSize(wxSizeEvent& event);

private:
    void CreateControls();
    void UpdateNavigationButtons();
    
    wxWebView* m_webView;
    wxString m_currentURL;

    wxDECLARE_EVENT_TABLE();
};
