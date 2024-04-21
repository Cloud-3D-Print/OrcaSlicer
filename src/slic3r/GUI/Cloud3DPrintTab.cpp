#include "Cloud3DPrintTab.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI_App.hpp"
#include <curl/curl.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdio.h>
#include "../utils/Http.hpp"
#include <Urlmon.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <codecvt>
#include <locale>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
// Assuming you have included nlohmann/json.hpp and defined the necessary namespaces.

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

Cloud3DPrintTab::Cloud3DPrintTab(wxWindow* parent, Plater* platter, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style)
{
    logWindow              = new wxLogWindow(this, "Log Messages", true, false);
    m_plater               = platter;
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

static std::wstring utf8ToWstring(const std::string& utf8str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(utf8str);
    } catch (const std::exception& e) {
        std::wcerr << "Failed to convert UTF-8 string to wstring: " << e.what() << std::endl;
        return L""; // Return an empty string on failure
    }
}

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
        json     data   = json::parse(message);
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
            std::vector<std::string>  urls;
            std::vector<std::wstring> fileNames; // Using std::wstring for Unicode support
            nlohmann::json            modelList = data["modelList"];
        for (const auto& model : modelList) {
            std::string  url      = model["fileUrl"];
            std::wstring fileName = utf8ToWstring(model["fileName"].get<std::string>());
            fileNames.push_back(fileName);
            urls.push_back(url);
        }

                std::vector<std::wstring> outputPaths;

                for (size_t i = 0; i < urls.size(); i++) {
                    CURL*              curl = curl_easy_init();
                    const std::string& url  = urls[i];
                    FILE*              fp;
                    CURLcode           res;

                    wxLogMessage("Downloading file from url: %s", url.c_str());
                    std::filesystem::path outPath = std::filesystem::current_path() / fileNames[i];
                    if (outPath.extension() != L".stl") {
                        outPath += L".stl";
                    }
                    outputPaths.push_back(outPath.wstring());
                    wxLogMessage("Downloaded file saved to: %ls", outPath.c_str());

                    if (curl) {
                        fp = _wfopen(outPath.c_str(), L"wb");
                        if (fp == nullptr) {
                            wxLogError("Failed to open file: %ls", outPath.c_str());
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

                // Persist project id and org id from JSON
                std::string projectId = data["projectId"];
                std::string orgId     = data["orgId"];

                // Print out the two values extracted
                wxLogMessage("Project ID: %s", projectId.c_str());
                wxLogMessage("Organization ID: %s", orgId.c_str());

                // Create a JSON object
                nlohmann::json jsonOutput;
                jsonOutput["projectId"] = projectId;
                jsonOutput["orgId"]     = orgId;

                // Write JSON to file
                std::filesystem::path jsonOutPath = std::filesystem::current_path() / "orgid.json";
                std::ofstream         outputFile(jsonOutPath);
                if (outputFile.is_open()) {
                    outputFile << jsonOutput.dump(4); // Indent with 4 spaces
                    outputFile.close();
                    wxLogMessage("Project ID and Organization ID written to file: %s", jsonOutPath.string().c_str());
                } else {
                    wxLogError("Failed to open file for writing: %s", jsonOutPath.string().c_str());
                }

                // Check if all downloaded files exist in the specified paths
                bool allFilesExist = true;
                for (const auto& path : outputPaths) {
                    if (!std::filesystem::exists(std::filesystem::path(path))) {
                        wxLogError("Downloaded file does not exist: %ls", path.c_str());
                        allFilesExist = false;
                        break;
                    } else {
                        wxLogMessage("Downloaded file exists: %ls", path.c_str());
                    }
                }

                // Append all downloaded files to the platter
                wxArrayString input_files;
                wxLogMessage("input_files: %d", input_files.GetCount());
                
                for (const auto& path : outputPaths) {
                    if (std::filesystem::exists(std::filesystem::path(path))) {
                        wxString mystring = wxString::FromUTF8(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(path).c_str());
                        wxLogMessage("Opening file path: %s", mystring);
                        input_files.Add(mystring);
                       
                       
                    }
                }
                 //m_plater->delete_plate();
                // post_event(SimpleEvent(EVT_GLTOOLBAR_DELETE_ALL));
                 Model model = m_plater->model();
                 //model.clear_objects();
                 m_plater->new_project();
                 //m_plater->load_project();
                 m_plater->add_file(input_files);
                if (allFilesExist) {
                    wxLogMessage("All downloaded files exist in the specified paths.");
                } else {
                    wxLogError("Not all downloaded files exist in the specified paths.");
                }

            } catch (const std::exception& e) {
                wxLogError("Error parsing JSON 1: %s", e.what());
            }
        }
       
    } catch (const std::exception& e) {
        wxLogError("Error parsing JSON 2: %s", e.what());
    }
}

    bool Cloud3DPrintTab::CheckTokenFileExists()
    {
        std::string filename = "token.json";
        return std::filesystem::exists(filename);
    }

    Cloud3DPrintTab::~Cloud3DPrintTab() {}
}
} // namespace Slic3r::GUI