#include "WebViewPanel.h"
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

#ifdef __WXMSW__
#include <windows.h>
#endif

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
                           const wxPoint& pos, const wxSize& size, bool disableWebView)
    : wxPanel(parent, id, pos, size)
    , m_webView(nullptr)
    , m_backBtn(nullptr)
    , m_forwardBtn(nullptr)
    , m_reloadBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_urlCtrl(nullptr)
    , m_statusText(nullptr)
    , m_placeholderText(nullptr)
    , m_webViewDisabled(disableWebView)
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

    // If WebView is disabled, create static placeholder immediately
    if (m_webViewDisabled) {
        CreateStaticPlaceholder();
        CreateControls();
        return;
    }

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

    // Check if parent window has compositing enabled (conflicts with WebView2)
    bool hasCompositing = false;
#ifdef __WXMSW__
    if (GetParent()) {
        // Check for WS_EX_COMPOSITED extended style
        HWND hwndParent = static_cast<HWND>(GetParent()->GetHandle());
        if (hwndParent) {
            LONG exStyle = GetWindowLongPtr(hwndParent, GWL_EXSTYLE);
            hasCompositing = (exStyle & WS_EX_COMPOSITED) != 0;
        }
    }
#endif

    // Create WebView - prefer IE backend, avoid WebView2 if compositing is enabled
    bool webViewCreated = false;

    // Try IE backend first (most compatible)
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

    // Try WebKit backend if IE not available
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

    // Only try Edge WebView2 as last resort and only if no compositing
    if (!webViewCreated && !hasCompositing && wxWebView::IsBackendAvailable(wxWebViewBackendEdge))
    {
        try {
            m_webView = wxWebView::New(this, ID_WEBVIEW, wxWebViewDefaultURLStr,
                                       wxDefaultPosition, wxDefaultSize, wxWebViewBackendEdge);
            if (m_webView) {
                webViewCreated = true;
                m_statusText->SetLabel("Using Edge WebView2 backend");
            }
        }
        catch (const std::exception& e) {
            wxLogError("Failed to create Edge WebView: %s", e.what());
        }
    }
    
    if (!webViewCreated)
    {
        wxString message = "WebView is not available on this platform.\n"
                          "IE WebView backend requires Internet Explorer to be installed.";
        if (hasCompositing) {
            message += "\nNote: WebView2 (Edge) backend is disabled when window compositing is enabled.";
        }
        message += "\nPlease install IE or WebView2 runtime.";

        wxMessageBox(message, "WebView Not Available", wxOK | wxICON_WARNING);
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
    if (m_webViewDisabled || !m_webView)
    {
        // If WebView is disabled or controls haven't been created yet, store the URL for later
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
    if (m_webViewDisabled || !m_webView) return;
    try {
        m_webView->SetPage(html, wxString());
        m_statusText->SetLabel("HTML loaded");
    }
    catch (const std::exception& e) {
        wxLogError("Failed to load HTML: %s", e.what());
        m_statusText->SetLabel("Failed to load HTML");
    }
}

void WebViewPanel::Reload()
{
    if (m_webViewDisabled || !m_webView) return;
    m_webView->Reload();
    m_statusText->SetLabel("Reloading...");
}

void WebViewPanel::Stop()
{
    if (m_webViewDisabled || !m_webView) return;
    m_webView->Stop();
    m_statusText->SetLabel("Stopped");
}

void WebViewPanel::GoBack()
{
    if (m_webViewDisabled || !m_webView) return;
    if (m_webView->CanGoBack())
    {
        m_webView->GoBack();
        m_statusText->SetLabel("Going back...");
    }
}

void WebViewPanel::GoForward()
{
    if (m_webViewDisabled || !m_webView) return;
    if (m_webView->CanGoForward())
    {
        m_webView->GoForward();
        m_statusText->SetLabel("Going forward...");
    }
}

bool WebViewPanel::CanGoBack() const
{
    return !m_webViewDisabled && m_webView && m_webView->CanGoBack();
}

bool WebViewPanel::CanGoForward() const
{
    return !m_webViewDisabled && m_webView && m_webView->CanGoForward();
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
    wxString errorMsg = event.GetString();
    m_statusText->SetLabel("Error: " + errorMsg);

    // Log JavaScript errors with more detail
    if (errorMsg.Contains("JavaScript") || errorMsg.Contains("script") || errorMsg.Contains("Js::")) {
        wxLogError("WebView JavaScript error: %s (URL: %s)", errorMsg, m_currentURL);
        // Disable WebView completely on JavaScript exceptions to prevent crashes
        DisableWebView("JavaScript compatibility issues detected");
    } else {
        wxLogError("WebView error: %s", errorMsg);
    }

    // For JavaScript errors, try to load a simple fallback page (but only if not disabled)
    if ((errorMsg.Contains("JavaScript") || errorMsg.Contains("Js::")) && m_webView && !m_webViewDisabled) {
        wxLogWarning("JavaScript error detected, loading fallback page");
        LoadFallbackPage();
    }
}

void WebViewPanel::LoadFallbackPage()
{
    wxString fallbackHtml = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>WebView Fallback</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        p {
            color: #666;
            line-height: 1.6;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WebView Compatibility Mode</h1>
        <p>This page is displayed because the original web content encountered JavaScript compatibility issues.</p>
        <p>The embedded browser is running in compatibility mode to ensure stable operation.</p>
        <p>You can try navigating to a different URL using the address bar above.</p>
    </div>
</body>
</html>
)HTML";

    LoadHTML(fallbackHtml);
}

void WebViewPanel::DisableWebView(const wxString& reason)
{
    if (m_webViewDisabled) return; // Already disabled

    wxLogWarning("Disabling WebView due to: %s", reason);
    m_webViewDisabled = true;

    // Hide WebView if it exists
    if (m_webView) {
        m_webView->Hide();
        // Remove from sizer to free space
        if (GetSizer()) {
            GetSizer()->Detach(m_webView);
        }
    }

    // Create static placeholder
    CreateStaticPlaceholder();

    // Update navigation buttons state
    if (m_backBtn) m_backBtn->Disable();
    if (m_forwardBtn) m_forwardBtn->Disable();
    if (m_reloadBtn) m_reloadBtn->Disable();
    if (m_stopBtn) m_stopBtn->Disable();
    if (m_urlCtrl) m_urlCtrl->Disable();

    m_statusText->SetLabel("WebView disabled: " + reason);

    Layout();
}

void WebViewPanel::CreateStaticPlaceholder()
{
    if (m_placeholderText) return; // Already created

    wxString placeholderMsg;
    if (m_webViewDisabled) {
        placeholderMsg = wxString::Format(
            "Embedded Browser Disabled\n\n"
            "The embedded browser component has been disabled to prevent\n"
            "JavaScript compatibility issues and ensure application stability.\n\n"
            "This prevents crashes caused by browser engine conflicts.\n\n"
            "You can continue using all other CAD features normally."
        );
    } else {
        placeholderMsg = wxString::Format(
            "WebView has been disabled for stability reasons.\n\n"
            "Reason: JavaScript compatibility issues detected\n\n"
            "The embedded browser component encountered critical errors that could\n"
            "cause application instability. WebView functionality has been disabled\n"
            "to ensure the application remains stable.\n\n"
            "You can continue using other CAD features normally."
        );
    }

    m_placeholderText = new wxStaticText(this, wxID_ANY, placeholderMsg,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE);

    wxFont placeholderFont = m_placeholderText->GetFont();
    placeholderFont.SetPointSize(10);
    m_placeholderText->SetFont(placeholderFont);
    m_placeholderText->SetForegroundColour(wxColour(128, 128, 128));

    // Add to sizer in place of WebView
    if (GetSizer()) {
        GetSizer()->Add(m_placeholderText, 1, wxEXPAND | wxALL, 20);
    }

    Layout();
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
    if (!m_webView && !m_webViewDisabled && GetSize().GetWidth() > 0 && GetSize().GetHeight() > 0)
    {
        CreateControls();
        // Load a default page after controls are created
        LoadURL("https://www.google.com");
    }
}