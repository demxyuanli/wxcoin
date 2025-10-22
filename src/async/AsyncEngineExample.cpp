#include "async/AsyncEngineExample.h"
#include "logger/Logger.h"
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/filedlg.h>
#include <OpenCASCADE/BRepPrimAPI_MakeBox.hxx>

namespace async {

wxBEGIN_EVENT_TABLE(AsyncEngineExampleFrame, wxFrame)
wxEND_EVENT_TABLE()

AsyncEngineExampleFrame::AsyncEngineExampleFrame()
    : wxFrame(nullptr, wxID_ANY, "Async Compute Engine Example", 
              wxDefaultPosition, wxSize(800, 600))
{
    m_asyncEngine = std::make_unique<AsyncEngineIntegration>(this);
    
    setupUI();
    
    Bind(wxEVT_ASYNC_INTERSECTION_RESULT, &AsyncEngineExampleFrame::OnIntersectionResult, this);
    Bind(wxEVT_ASYNC_MESH_RESULT, &AsyncEngineExampleFrame::OnMeshResult, this);
    Bind(wxEVT_ASYNC_TASK_PROGRESS, &AsyncEngineExampleFrame::OnTaskProgress, this);
    
    logMessage("Async Compute Engine initialized");
    logMessage("Workers: " + std::to_string(std::thread::hardware_concurrency()));
}

AsyncEngineExampleFrame::~AsyncEngineExampleFrame() {
    if (m_asyncEngine) {
        m_asyncEngine->cancelAllTasks();
    }
}

void AsyncEngineExampleFrame::setupUI() {
    wxPanel* mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* loadBtn = new wxButton(mainPanel, wxID_ANY, "Load Test Model");
    wxButton* intersectionBtn = new wxButton(mainPanel, wxID_ANY, "Compute Intersections");
    wxButton* meshBtn = new wxButton(mainPanel, wxID_ANY, "Generate Mesh");
    wxButton* cancelBtn = new wxButton(mainPanel, wxID_ANY, "Cancel All");
    wxButton* statsBtn = new wxButton(mainPanel, wxID_ANY, "Show Stats");
    
    buttonSizer->Add(loadBtn, 0, wxALL, 5);
    buttonSizer->Add(intersectionBtn, 0, wxALL, 5);
    buttonSizer->Add(meshBtn, 0, wxALL, 5);
    buttonSizer->Add(cancelBtn, 0, wxALL, 5);
    buttonSizer->Add(statsBtn, 0, wxALL, 5);
    
    m_statusLabel = new wxStaticText(mainPanel, wxID_ANY, "Status: Ready");
    
    m_logPanel = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(m_statusLabel, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(m_logPanel, 1, wxEXPAND | wxALL, 5);
    
    mainPanel->SetSizer(mainSizer);
    
    loadBtn->Bind(wxEVT_BUTTON, &AsyncEngineExampleFrame::OnLoadModel, this);
    intersectionBtn->Bind(wxEVT_BUTTON, &AsyncEngineExampleFrame::OnComputeIntersections, this);
    meshBtn->Bind(wxEVT_BUTTON, &AsyncEngineExampleFrame::OnGenerateMesh, this);
    cancelBtn->Bind(wxEVT_BUTTON, &AsyncEngineExampleFrame::OnCancelTasks, this);
    statsBtn->Bind(wxEVT_BUTTON, &AsyncEngineExampleFrame::OnShowStatistics, this);
}

void AsyncEngineExampleFrame::OnLoadModel(wxCommandEvent& event) {
    logMessage("Loading test model (box)...");
    m_currentShape = BRepPrimAPI_MakeBox(100, 100, 100).Shape();
    logMessage("Test model loaded");
}

void AsyncEngineExampleFrame::OnComputeIntersections(wxCommandEvent& event) {
    if (m_currentShape.IsNull()) {
        logMessage("ERROR: No model loaded");
        return;
    }
    
    std::string taskId = "intersection_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    logMessage("Submitting intersection task: " + taskId);
    
    m_asyncEngine->computeIntersectionsAsync(taskId, m_currentShape, 1e-6);
    
    if (m_statusLabel) {
        m_statusLabel->SetLabelText("Status: Computing intersections...");
    }
}

void AsyncEngineExampleFrame::OnGenerateMesh(wxCommandEvent& event) {
    if (m_currentShape.IsNull()) {
        logMessage("ERROR: No model loaded");
        return;
    }
    
    std::string taskId = "mesh_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    logMessage("Submitting mesh generation task: " + taskId);
    
    m_asyncEngine->generateMeshAsync(taskId, m_currentShape, 0.1, 0.5);
    
    if (m_statusLabel) {
        m_statusLabel->SetLabelText("Status: Generating mesh...");
    }
}

void AsyncEngineExampleFrame::OnCancelTasks(wxCommandEvent& event) {
    logMessage("Cancelling all tasks...");
    m_asyncEngine->cancelAllTasks();
    
    if (m_statusLabel) {
        m_statusLabel->SetLabelText("Status: Cancelled");
    }
}

void AsyncEngineExampleFrame::OnShowStatistics(wxCommandEvent& event) {
    auto stats = m_asyncEngine->getStatistics();
    
    std::string statsMsg = "\n=== Engine Statistics ===\n";
    statsMsg += "Queued: " + std::to_string(stats.queuedTasks) + "\n";
    statsMsg += "Running: " + std::to_string(stats.runningTasks) + "\n";
    statsMsg += "Completed: " + std::to_string(stats.completedTasks) + "\n";
    statsMsg += "Failed: " + std::to_string(stats.failedTasks) + "\n";
    statsMsg += "Avg Execution Time: " + std::to_string(stats.avgExecutionTimeMs) + "ms\n";
    statsMsg += "Total Processed: " + std::to_string(stats.totalProcessedTasks) + "\n";
    
    logMessage(statsMsg);
}

void AsyncEngineExampleFrame::OnIntersectionResult(AsyncIntersectionResultEvent& event) {
    const auto& result = event.GetResult();
    
    std::string msg = "Intersection task " + event.GetTaskId() + " completed:\n";
    msg += "  Points: " + std::to_string(result.points.size()) + "\n";
    msg += "  Edges: " + std::to_string(result.edgeCount) + "\n";
    msg += "  Time: " + std::to_string(result.computeTime.count()) + "ms\n";
    
    logMessage(msg);
    
    if (m_statusLabel) {
        m_statusLabel->SetLabelText("Status: Intersection computation completed");
    }
}

void AsyncEngineExampleFrame::OnMeshResult(AsyncMeshResultEvent& event) {
    auto meshData = event.GetMeshData();
    
    if (meshData) {
        std::string msg = "Mesh generation task " + event.GetTaskId() + " completed:\n";
        msg += "  Vertices: " + std::to_string(meshData->vertexCount) + "\n";
        msg += "  Triangles: " + std::to_string(meshData->triangleCount) + "\n";
        msg += "  Memory: " + std::to_string(meshData->getMemoryUsage() / 1024) + " KB\n";
        
        logMessage(msg);
        
        std::string cacheKey = event.GetTaskId() + "_mesh";
        auto sharedMesh = m_asyncEngine->getSharedData<MeshData>(cacheKey);
        if (sharedMesh && sharedMesh->ready) {
            logMessage("  Mesh data available in shared cache");
        }
    }
    
    if (m_statusLabel) {
        m_statusLabel->SetLabelText("Status: Mesh generation completed");
    }
}

void AsyncEngineExampleFrame::OnTaskProgress(AsyncEngineResultEvent& event) {
    logMessage("Progress update for task: " + event.GetTaskId());
}

void AsyncEngineExampleFrame::logMessage(const std::string& message) {
    if (m_logPanel) {
        wxString currentText = m_logPanel->GetValue();
        wxString newText = wxString::Format("[%s] %s\n", 
            wxDateTime::Now().Format("%H:%M:%S"),
            message);
        m_logPanel->AppendText(newText);
    }
    
    LOG_INF_S("AsyncEngineExample: " + message);
}

} // namespace async


