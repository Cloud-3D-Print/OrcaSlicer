#ifndef CLOUD3DPRINTTAB_HPP
#define CLOUD3DPRINTTAB_HPP

#include <wx/wx.h>
#include "wxExtensions.hpp"
#include <slic3r/GUI/Widgets/WebView.hpp>
#include <wx/log.h>

namespace Slic3r { namespace GUI {

class Cloud3DPrintTab : public wxPanel
{
public:
    Cloud3DPrintTab(wxWindow *parent,wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = wxTAB_TRAVERSAL);
    ~Cloud3DPrintTab();

    wxString   m_c3dp_login_url;
    wxWebView* m_browser = {nullptr};

    wxLogWindow* logWindow; 

    void Cloud3DPrintTab::OnPageLoaded(wxWebViewEvent& event);
    void Cloud3DPrintTab::OnScriptMessage(wxWebViewEvent& event);
    bool Cloud3DPrintTab::CheckTokenFileExists();
    bool Cloud3DPrintTab::isTokenValid();


private:
    // Add any private member variables or functions here

};

}} // namespace Slic3r::GUI

#endif // CLOUD3DPRINTTAB_HPP