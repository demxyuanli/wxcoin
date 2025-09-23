#include "WebViewPanel.h"
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/datetime.h>

// Event IDs
enum {
    ID_BACK = 1000,
    ID_FORWARD,
    ID_RELOAD,
    ID_STOP,
    ID_URL_CTRL,
    ID_WEBVIEW
};

wxBEGIN_EVENT_TABLE(WebViewPanel, wxPanel)
    // Only bind essential events to reduce flickering
    EVT_WEBVIEW_NAVIGATED(ID_WEBVIEW, WebViewPanel::OnWebViewLoaded)
    EVT_WEBVIEW_ERROR(ID_WEBVIEW, WebViewPanel::OnWebViewError)
wxEND_EVENT_TABLE()

WebViewPanel::WebViewPanel(wxWindow* parent, wxWindowID id,
                           const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size)
    , m_webView(nullptr)
    , m_currentURL(wxEmptyString)
{
    // Delay control creation until the widget is properly parented
    Bind(wxEVT_SIZE, &WebViewPanel::OnSize, this);
}

WebViewPanel::~WebViewPanel()
{
    // WebView is automatically destroyed as child window
}

void WebViewPanel::CreateControls()
{
    // Create WebView directly without navigation controls
    if (wxWebView::IsBackendAvailable(wxWebViewBackendEdge))
    {
        m_webView = wxWebView::New(this, ID_WEBVIEW, wxWebViewDefaultURLStr,
                                   wxDefaultPosition, wxDefaultSize, wxWebViewBackendEdge);
    }
    else if (wxWebView::IsBackendAvailable(wxWebViewBackendWebKit))
    {
        m_webView = wxWebView::New(this, ID_WEBVIEW, wxWebViewDefaultURLStr,
                                   wxDefaultPosition, wxDefaultSize, wxWebViewBackendWebKit);
    }
    else
    {
        wxMessageBox("WebView is not available on this platform.\n"
                    "Please install Microsoft Edge WebView2 or WebKit.",
                    "WebView Not Available", wxOK | wxICON_WARNING);
        return;
    }

    if (m_webView)
    {
        // Use a simple sizer that fills the entire panel
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(m_webView, 1, wxEXPAND);
        SetSizer(mainSizer);
        
        // Prevent layout recursion and flickering by using Freeze/Thaw
        Freeze();
        Layout();
        Thaw();
        
        // Disable automatic refresh to prevent flickering
        // Note: SetScrollRate is not available for wxWebView
    }

    // Load the stored URL if any
    if (!m_currentURL.IsEmpty())
    {
        LoadURL(m_currentURL);
        m_currentURL.Clear();
    }
}

void WebViewPanel::LoadURL(const wxString& url)
{
    if (!m_webView)
    {
        // If controls haven't been created yet, store the URL for later
        m_currentURL = url;
        return;
    }

    wxString processedURL = url;

    // Add protocol if missing
    if (!processedURL.Contains("://"))
    {
        // Check if it's a search query (contains spaces or no dots)
        if (processedURL.Contains(" ") || !processedURL.Contains("."))
        {
            // It's a search query
            processedURL = "https://www.google.com/search?q=" + processedURL;
        }
        else
        {
            // Assume it's a URL and add http://
            processedURL = "http://" + processedURL;
        }
    }

    m_currentURL = processedURL;
    m_webView->LoadURL(m_currentURL);
}

void WebViewPanel::LoadHTML(const wxString& html)
{
    if (!m_webView) return;
    m_webView->SetPage(html, wxString());
}

void WebViewPanel::Reload()
{
    if (!m_webView) return;
    m_webView->Reload();
}

void WebViewPanel::Stop()
{
    if (!m_webView) return;
    m_webView->Stop();
}

void WebViewPanel::GoBack()
{
    if (!m_webView) return;
    if (m_webView->CanGoBack())
    {
        m_webView->GoBack();
    }
}

void WebViewPanel::GoForward()
{
    if (!m_webView) return;
    if (m_webView->CanGoForward())
    {
        m_webView->GoForward();
    }
}

bool WebViewPanel::CanGoBack() const
{
    return m_webView && m_webView->CanGoBack();
}

bool WebViewPanel::CanGoForward() const
{
    return m_webView && m_webView->CanGoForward();
}

// Navigation methods kept for external use
void WebViewPanel::OnBack(wxCommandEvent& event)
{
    GoBack();
}

void WebViewPanel::OnForward(wxCommandEvent& event)
{
    GoForward();
}

void WebViewPanel::OnReload(wxCommandEvent& event)
{
    Reload();
}

void WebViewPanel::OnStop(wxCommandEvent& event)
{
    Stop();
}

void WebViewPanel::OnNavigate(wxCommandEvent& event)
{
    // No longer used - navigation controls removed
}

void WebViewPanel::OnWebViewLoaded(wxWebViewEvent& event)
{
    // Only handle navigation events, ignore others to reduce flickering
    if (event.GetEventType() == wxEVT_WEBVIEW_NAVIGATED)
    {
        // Throttle URL updates to prevent excessive processing
        static wxDateTime lastUpdateTime;
        wxDateTime now = wxDateTime::Now();
        
        // Only update if at least 200ms have passed since last update
        if (!lastUpdateTime.IsValid() || (now - lastUpdateTime).GetMilliseconds() > 200) {
            m_currentURL = event.GetURL();
            lastUpdateTime = now;
        }
    }
}

void WebViewPanel::OnWebViewError(wxWebViewEvent& event)
{
    wxLogError("WebView error: %s", event.GetString());
}

// Title changed event removed to reduce flickering

void WebViewPanel::UpdateNavigationButtons()
{
    // No longer needed - navigation buttons removed
}

void WebViewPanel::OnSize(wxSizeEvent& event)
{
    event.Skip();

    // Create controls only once, when we have a valid size
    // Use a flag to prevent recursive calls during control creation
    static bool creatingControls = false;
    if (!m_webView && !creatingControls && GetSize().GetWidth() > 0 && GetSize().GetHeight() > 0)
    {
        creatingControls = true;
        CreateControls();
        creatingControls = false;
        
        // Load a default page after controls are created
        LoadURL("https://www.google.com");
    }
    else if (m_webView)
    {
        // Prevent unnecessary refreshes during resize to avoid flickering
        static wxDateTime lastResizeTime;
        wxDateTime now = wxDateTime::Now();
        
        // Only refresh if enough time has passed since last resize
        if (!lastResizeTime.IsValid() || (now - lastResizeTime).GetMilliseconds() > 200) {
            lastResizeTime = now;
            // Use minimal refresh to prevent flickering
            m_webView->Refresh(false);
        }
    }
}