#include "Cloud3DPrintTab.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI_App.hpp"
#include <curl/curl.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdio.h>
#include "../utils/Http.hpp"
#include <Urlmon.h>
// Assuming you have included nlohmann/json.hpp and defined the necessary namespaces.

using json = nlohmann::json;

namespace Slic3r { namespace GUI {

Cloud3DPrintTab::Cloud3DPrintTab(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style) : wxPanel(parent, id, pos, size, style)
{

    logWindow = new wxLogWindow(this, "Log Messages", true, false);

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    m_c3dp_login_url = wxString::Format("file://%s/web/orcaHtml/login.html", from_u8(resources_dir()));
    wxString strlang = wxGetApp().current_language_code_safe();
    if (strlang != "")
        m_c3dp_login_url = wxString::Format("file://%s/web/orcaHtml/login.html?lang=%s", from_u8(resources_dir()), strlang);

    m_browser = WebView::CreateWebView(this, m_c3dp_login_url);
        main_sizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));
    // Bind the wxEVT_WEBVIEW_NAVIGATED event to a function that will be called when the page changes
    // m_browser->Bind(wxEVT_WEBVIEW_LOADED, &Cloud3DPrintTab::OnPageLoaded, this);
    // m_browser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &WebViewPanel::OnScriptMessage, this);
    m_browser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &Cloud3DPrintTab::OnScriptMessage, this);

    SetSizer(main_sizer);
    Layout();
    Fit();

    wxLogMessage("Cloud3DPrintTab initialized.");
}


void Cloud3DPrintTab::OnPageLoaded(wxWebViewEvent& event) {
}


void Cloud3DPrintTab::OnScriptMessage(wxWebViewEvent& event)
{
    wxString message = event.GetString();
    wxString messageHandler = event.GetMessageHandler();

    // Process the message received from JavaScript
    wxLogMessage("Script message received; (Cloud 3d print tab) value = %s, handler = %s", message, messageHandler);

    // Parse message as JSON
    try {
        json data = json::parse(message.ToStdString());
         wxString strCmd = data["command"];
        // Check if the message contains the token

        /**
         * Register the token
        */
         if (strCmd == "register_token"){
            std::string token = data["token"];
            wxLogMessage("Token received: %s", token);
            // Save the token to a file
            std::ofstream tokenFile("token.json");
            if (tokenFile.is_open()) {
                json tokenJson;
                tokenJson["token"] = token;
                tokenFile << tokenJson.dump(4); // Pretty print with 4 spaces indentation
                tokenFile.close();
               // Get the full path of the file
                std::string fullPath = std::filesystem::current_path().string() + "\\" + "token.json";
                wxLogMessage("Token saved successfully. File location: %s", fullPath.c_str());
            } else {
                wxLogError("Unable to open token file for writing.");
            }
         }

        if (strCmd == "download_file") {
    try {
        wxString downloadUrl = data["fileUrl"];
        // Remove everything after and including the '?' character
        //downloadUrl = downloadUrl.BeforeFirst('?');
        wxLogMessage("Got the download url: %s", downloadUrl);

        // Define the output path
        wxString outputPath = wxT("/code/test.stl");

        // Initialize CURL
        CURL *curl = curl_easy_init();
        if (curl) {
            FILE *fp = fopen(outputPath.utf8_str(), "wb");
            if (fp == nullptr) {
                curl_easy_cleanup(curl);
                wxLogMessage("Download failed: cannot open file\n");
                return; // Can't open the file for writing
            }
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            // Set the URL and callback function
            curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.utf8_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void *ptr, size_t size, size_t nmemb, void *stream) -> size_t {
                return fwrite(ptr, size, nmemb, static_cast<FILE*>(stream));
            });
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            // Perform the request, and check for errors
            CURLcode res = curl_easy_perform(curl);
            fclose(fp);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK) {
                wxLogMessage("Download succeeded\n");
            } else {
                wxLogMessage("Download failed: %s\n", curl_easy_strerror(res));
            }
        }
        else {
            wxLogMessage("Curl failed to initialize");
        }
    } catch (const std::exception& e) {
        wxLogError("Error parsing JSON: %s", e.what());
    }
        }
    } catch (const std::exception& e) {
        wxLogError("Error parsing JSON: %s", e.what());
    }}
    



bool Cloud3DPrintTab::CheckTokenFileExists() {
    std::string filename = "token.json";
    return std::filesystem::exists(filename);
}

Cloud3DPrintTab::~Cloud3DPrintTab() {}

}} // namespace Slic3r::GUI