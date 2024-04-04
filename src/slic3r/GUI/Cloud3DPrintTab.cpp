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

Cloud3DPrintTab::Cloud3DPrintTab(wxWindow* parent, Plater *platter, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style)
{
    logWindow = new wxLogWindow(this, "Log Messages", true, false);
    m_plater = platter;
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

void Cloud3DPrintTab::OnPageLoaded(wxWebViewEvent& event) {}

static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void Cloud3DPrintTab::OnScriptMessage(wxWebViewEvent& event)
{
    wxString message        = event.GetString();
    wxString messageHandler = event.GetMessageHandler();

    // Process the message received from JavaScript
    wxLogMessage("Script message received; (Cloud 3d print tab) value = %s, handler = %s", message, messageHandler);

    // Parse message as JSON
    try {
        json     data   = json::parse(message.ToStdString());
        wxString strCmd = data["command"];
        // Check if the message contains the token

        /**
         * Register the token
         */
        if (strCmd == "register_token") {
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

        if (strCmd == "download_all_files") {
            try {
                // Extract each url in modelList
                std::vector<std::string> urls;
                std::vector<std::string> fileNames;
                json                     modelList = data["modelList"];
                for (const auto& model : modelList) {
                    std::string url      = model["fileUrl"];
                    std::string fileName = model["fileName"];
                    fileNames.push_back(fileName);
                    urls.push_back(url);
                }

                std::vector<std::string> outputPaths;

                for (size_t i = 0; i < urls.size(); i++) {
                    CURL*              curl = curl_easy_init();
                    const std::string& url  = urls[i]; // Assuming urls[i] contains the correct URL
                    FILE*              fp;
                    CURLcode           res;

                    wxLogMessage("Downloading file from url: %s", url.c_str());
                    std::filesystem::path outPath = std::filesystem::current_path() / fileNames[i];
                    if (outPath.extension() != ".stl") {
                        outPath += ".stl";
                    }
                    outputPaths.push_back(outPath.string());
                    wxLogMessage("Downloaded file saved to: %s", outPath.string().c_str());

                    if (curl) {
                        fp = fopen(outPath.string().c_str(), "wb");
                        if (fp == nullptr) {
                            wxLogError("Failed to open file: %s", outPath.string().c_str());
                            curl_easy_cleanup(curl);
                            continue;
                        }

                        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
                        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // For testing purposes only

                        res = curl_easy_perform(curl);
                        if (res != CURLE_OK) {
                            wxLogError("curl_easy_perform() failed: %s", curl_easy_strerror(res));
                        } else {
                            wxLogMessage("File downloaded successfully.");
                        }

                        curl_easy_cleanup(curl);
                        fclose(fp);
                    } else {
                        wxLogError("Error initializing curl");
                    }
                }

                // Check if all downloaded files exist in the specified paths
                bool allFilesExist = true;
                for (const std::string& path : outputPaths) {
                    if (!std::filesystem::exists(path)) {
                        wxLogError("Downloaded file does not exist: %s", path.c_str());
                        allFilesExist = false;
                        break;
                    }
                }

                // Append all downloaded files to the platter
                wxArrayString input_files;
                for (const std::string& path : outputPaths) {
                    
                    if (std::filesystem::exists(path)) {
                        wxString mystring = wxString::FromUTF8(path.c_str());
                        wxLogMessage("Opening file path: %s", mystring);
                        input_files.Add(mystring);
                        m_plater->add_file(input_files);
                    }
                }
                if (allFilesExist) {
                    wxLogMessage("All downloaded files exist in the specified paths.");
                } else {
                    wxLogError("Not all downloaded files exist in the specified paths.");
                }

            } catch (const std::exception& e) {
                wxLogError("Error parsing JSON: %s", e.what());
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Error parsing JSON: %s", e.what());
    }
}

bool Cloud3DPrintTab::CheckTokenFileExists()
{
    std::string filename = "token.json";
    return std::filesystem::exists(filename);
}

Cloud3DPrintTab::~Cloud3DPrintTab() {}

}} // namespace Slic3r::GUI