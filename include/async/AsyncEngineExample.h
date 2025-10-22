#pragma once

#include "async/AsyncEngineIntegration.h"
#include <wx/frame.h>

namespace async {

class AsyncEngineExampleFrame : public wxFrame {
public:
    AsyncEngineExampleFrame();
    ~AsyncEngineExampleFrame();

private:
    void OnLoadModel(wxCommandEvent& event);
    void OnComputeIntersections(wxCommandEvent& event);
    void OnGenerateMesh(wxCommandEvent& event);
    void OnCancelTasks(wxCommandEvent& event);
    void OnShowStatistics(wxCommandEvent& event);
    
    void OnIntersectionResult(AsyncIntersectionResultEvent& event);
    void OnMeshResult(AsyncMeshResultEvent& event);
    void OnTaskProgress(AsyncEngineResultEvent& event);
    
    void setupUI();
    void logMessage(const std::string& message);

private:
    std::unique_ptr<AsyncEngineIntegration> m_asyncEngine;
    TopoDS_Shape m_currentShape;
    
    wxTextCtrl* m_logPanel{nullptr};
    wxStaticText* m_statusLabel{nullptr};
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace async


