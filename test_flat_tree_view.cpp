#include <wx/wx.h>
#include "widgets/FlatTreeView.h"

class TestFrame : public wxFrame
{
public:
    TestFrame() : wxFrame(nullptr, wxID_ANY, "FlatTreeView Test", wxDefaultPosition, wxSize(800, 600))
    {
        // Create FlatTreeView
        m_treeView = new FlatTreeView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        
        // Add some test data
        auto root = std::make_shared<FlatTreeItem>("Root", FlatTreeItem::ItemType::ROOT);
        m_treeView->SetRoot(root);
        
        // Add some child items
        for (int i = 0; i < 20; ++i) {
            auto child = std::make_shared<FlatTreeItem>(wxString::Format("[STEP]ATU010%d...", i), FlatTreeItem::ItemType::FILE);
            root->AddChild(child);
        }
        
        // Expand root to show children
        root->SetExpanded(true);
        
        // Set up layout
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_treeView, 1, wxEXPAND | wxALL, 5);
        SetSizer(sizer);
        
        // Bind events
        m_treeView->OnItemClicked([this](std::shared_ptr<FlatTreeItem> item, int column) {
            wxLogMessage("Clicked item: %s, column: %d", item->GetText(), column);
        });
    }
    
private:
    FlatTreeView* m_treeView;
};

class TestApp : public wxApp
{
public:
    bool OnInit() override
    {
        TestFrame* frame = new TestFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);