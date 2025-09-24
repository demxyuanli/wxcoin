#include "WebViewPanel.h"
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

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
    EVT_BUTTON(ID_BACK, WebViewPanel::OnBack)
    EVT_BUTTON(ID_FORWARD, WebViewPanel::OnForward)
    EVT_BUTTON(ID_RELOAD, WebViewPanel::OnReload)
    EVT_BUTTON(ID_STOP, WebViewPanel::OnStop)
    EVT_TEXT_ENTER(ID_URL_CTRL, WebViewPanel::OnNavigate)
    EVT_WEBVIEW_NAVIGATING(ID_WEBVIEW, WebViewPanel::OnWebViewLoaded)
    EVT_WEBVIEW_NAVIGATED(ID_WEBVIEW, WebViewPanel::OnWebViewLoaded)
    EVT_WEBVIEW_LOADED(ID_WEBVIEW, WebViewPanel::OnWebViewLoaded)
    EVT_WEBVIEW_ERROR(ID_WEBVIEW, WebViewPanel::OnWebViewError)
    EVT_WEBVIEW_TITLE_CHANGED(ID_WEBVIEW, WebViewPanel::OnWebViewTitleChanged)
wxEND_EVENT_TABLE()

WebViewPanel::WebViewPanel(wxWindow* parent, wxWindowID id,
                           const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size)
    , m_webView(nullptr)
    , m_backBtn(nullptr)
    , m_forwardBtn(nullptr)
    , m_reloadBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_urlCtrl(nullptr)
    , m_statusText(nullptr)
    , m_currentURL(wxEmptyString)
    , m_currentTitle(wxEmptyString)
{
    // Handle WS_EX_COMPOSITED conflict with WebView2
    // Since the parent FlatUI system enables WS_EX_COMPOSITED for performance,
    // we need to handle WebView2 rendering conflicts differently
#ifdef __WXMSW__
    // Store initial window handle for later use
    m_hwnd = GetHandle();
#endif

    // Delay control creation until the widget is properly parented
    Bind(wxEVT_SIZE, &WebViewPanel::OnSize, this);
}

WebViewPanel::~WebViewPanel()
{
    // WebView is automatically destroyed as child window
}

void WebViewPanel::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Navigation toolbar
    wxBoxSizer* navSizer = new wxBoxSizer(wxHORIZONTAL);

    m_backBtn = new wxButton(this, ID_BACK, "<", wxDefaultPosition, wxSize(30, 30));
    m_backBtn->SetToolTip("Go back");
    navSizer->Add(m_backBtn, 0, wxALL, 2);

    m_forwardBtn = new wxButton(this, ID_FORWARD, ">", wxDefaultPosition, wxSize(30, 30));
    m_forwardBtn->SetToolTip("Go forward");
    navSizer->Add(m_forwardBtn, 0, wxALL, 2);

    m_reloadBtn = new wxButton(this, ID_RELOAD, "R", wxDefaultPosition, wxSize(30, 30));
    m_reloadBtn->SetToolTip("Reload");
    navSizer->Add(m_reloadBtn, 0, wxALL, 2);

    m_stopBtn = new wxButton(this, ID_STOP, "S", wxDefaultPosition, wxSize(30, 30));
    m_stopBtn->SetToolTip("Stop");
    navSizer->Add(m_stopBtn, 0, wxALL, 2);

    navSizer->AddSpacer(10);

    m_urlCtrl = new wxTextCtrl(this, ID_URL_CTRL, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_urlCtrl->SetHint("Enter URL or search terms");
    navSizer->Add(m_urlCtrl, 1, wxALL | wxEXPAND, 2);

    mainSizer->Add(navSizer, 0, wxEXPAND | wxALL, 5);

    // Status bar
    m_statusText = new wxStaticText(this, wxID_ANY, "Ready");
    mainSizer->Add(m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    // Create WebView - use IE backend to avoid WS_EX_COMPOSITED conflicts
    bool webViewCreated = false;
    
    if (wxWebView::IsBackendAvailable(wxWebViewBackendIE))
    {
        try {
            m_webView = wxWebView::New(this, ID_WEBVIEW, wxWebViewDefaultURLStr,
                                       wxDefaultPosition, wxDefaultSize, wxWebViewBackendIE);
            if (m_webView) {
                webViewCreated = true;
                m_statusText->SetLabel("Using IE WebView backend");
            }
        }
        catch (const std::exception& e) {
            wxLogError("Failed to create IE WebView: %s", e.what());
        }
    }
    
    if (!webViewCreated && wxWebView::IsBackendAvailable(wxWebViewBackendWebKit))
    {
        try {
            m_webView = wxWebView::New(this, ID_WEBVIEW, wxWebViewDefaultURLStr,
                                       wxDefaultPosition, wxDefaultSize, wxWebViewBackendWebKit);
            if (m_webView) {
                webViewCreated = true;
                m_statusText->SetLabel("Using WebKit backend");
            }
        }
        catch (const std::exception& e) {
            wxLogError("Failed to create WebKit WebView: %s", e.what());
        }
    }
    
    if (!webViewCreated && wxWebView::IsBackendAvailable(wxWebViewBackendEdge))
    {
        try {
            m_webView = wxWebView::New(this, ID_WEBVIEW, wxWebViewDefaultURLStr,
                                       wxDefaultPosition, wxDefaultSize, wxWebViewBackendEdge);
            if (m_webView) {
                webViewCreated = true;
                m_statusText->SetLabel("Using Edge WebView2 backend (fallback)");
            }
        }
        catch (const std::exception& e) {
            wxLogError("Failed to create Edge WebView: %s", e.what());
        }
    }
    
    if (!webViewCreated)
    {
        wxMessageBox("WebView is not available on this platform.\n"
                    "IE WebView backend requires Internet Explorer to be installed.\n"
                    "Please install IE or WebView2 runtime.",
                    "WebView Not Available", wxOK | wxICON_WARNING);
        m_statusText->SetLabel("WebView not available - install IE or WebView2 runtime");
        return;
    }

    if (m_webView)
    {
        // IE WebView backend is more compatible with WS_EX_COMPOSITED
        // Enable double buffering for smoother rendering
        m_webView->SetDoubleBuffered(true);
        
        mainSizer->Add(m_webView, 1, wxEXPAND | wxALL, 5);
    }

    SetSizer(mainSizer);
    Layout();

    UpdateNavigationButtons();

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
    m_urlCtrl->SetValue(m_currentURL);
    m_webView->LoadURL(m_currentURL);
    m_statusText->SetLabel("Loading...");
}

void WebViewPanel::LoadHTML(const wxString& html)
{
    if (!m_webView) return;
    m_webView->SetPage(html, wxString());
    m_statusText->SetLabel("HTML loaded");
}

void WebViewPanel::Reload()
{
    if (!m_webView) return;
    m_webView->Reload();
    m_statusText->SetLabel("Reloading...");
}

void WebViewPanel::Stop()
{
    if (!m_webView) return;
    m_webView->Stop();
    m_statusText->SetLabel("Stopped");
}

void WebViewPanel::GoBack()
{
    if (!m_webView) return;
    if (m_webView->CanGoBack())
    {
        m_webView->GoBack();
        m_statusText->SetLabel("Going back...");
    }
}

void WebViewPanel::GoForward()
{
    if (!m_webView) return;
    if (m_webView->CanGoForward())
    {
        m_webView->GoForward();
        m_statusText->SetLabel("Going forward...");
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

// Event handlers
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
    wxString url = m_urlCtrl->GetValue();
    if (!url.IsEmpty())
    {
        LoadURL(url);
    }
}

void WebViewPanel::OnWebViewLoaded(wxWebViewEvent& event)
{
    UpdateNavigationButtons();

    if (event.GetEventType() == wxEVT_WEBVIEW_LOADED)
    {
        m_statusText->SetLabel("Loaded");
        
        // IE WebView backend handles composited rendering better
        // Just ensure a refresh for proper display
        if (m_webView) {
            m_webView->Refresh(false);
        }
    }
    else if (event.GetEventType() == wxEVT_WEBVIEW_NAVIGATED)
    {
        m_currentURL = event.GetURL();
        m_urlCtrl->SetValue(m_currentURL);
        m_statusText->SetLabel("Navigated");
    }
    else if (event.GetEventType() == wxEVT_WEBVIEW_NAVIGATING)
    {
        m_statusText->SetLabel("Navigating...");
    }
}

void WebViewPanel::OnWebViewError(wxWebViewEvent& event)
{
    m_statusText->SetLabel("Error: " + event.GetString());
    wxLogError("WebView error: %s", event.GetString());
}

void WebViewPanel::OnWebViewTitleChanged(wxWebViewEvent& event)
{
    m_currentTitle = event.GetString();
    // Could update window title here if needed
}

void WebViewPanel::UpdateNavigationButtons()
{
    if (!m_backBtn || !m_forwardBtn) return;

    m_backBtn->Enable(CanGoBack());
    m_forwardBtn->Enable(CanGoForward());
}

void WebViewPanel::OnSize(wxSizeEvent& event)
{
    event.Skip();

    // Create controls only once, when we have a valid size
    if (!m_webView && GetSize().GetWidth() > 0 && GetSize().GetHeight() > 0)
    {
        CreateControls();
        // Load a default page after controls are created
        LoadURL("https://www.google.com");
    }
}