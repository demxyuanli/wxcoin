#include "docking/DockSplitter.h"
#include "docking/DockContainerWidget.h"
#include "config/ThemeManager.h"
#include <algorithm>

namespace ads {

// Event table
wxBEGIN_EVENT_TABLE(DockSplitter, wxSplitterWindow)
    EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, DockSplitter::OnSplitterSashPosChanging)
    EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, DockSplitter::OnSplitterSashPosChanged)
wxEND_EVENT_TABLE()

// DockSplitter implementation
DockSplitter::DockSplitter(wxWindow* parent)
    : wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                      wxSP_3D)
    , m_orientation(wxHORIZONTAL)
{
    SetSashGravity(0.5);
    SetMinimumPaneSize(50);

    // Theme-driven splitter visuals
    SetBackgroundColour(CFG_COLOUR("DockAreaBgColour"));
    SetSashSize(CFG_INT("SplitterWidth"));

    // React to theme change
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        SetBackgroundColour(CFG_COLOUR("DockAreaBgColour"));
        SetSashSize(CFG_INT("SplitterWidth"));
        Refresh();
        Update();
    });
}

DockSplitter::~DockSplitter() {
}

void DockSplitter::setOrientation(wxOrientation orientation) {
    m_orientation = orientation;
}

void DockSplitter::insertWidget(int index, wxWindow* widget, bool stretch) {
    if (index < 0 || index > widgetCount()) {
        addWidget(widget, stretch);
        return;
    }

    // Ensure widget has this splitter as parent
    if (widget->GetParent() != this) {
        widget->Reparent(this);
    }

    m_widgets.insert(m_widgets.begin() + index, widget);
    updateSplitter();
}

void DockSplitter::addWidget(wxWindow* widget, bool stretch) {
    // Ensure widget has this splitter as parent
    if (widget->GetParent() != this) {
        widget->Reparent(this);
    }

    m_widgets.push_back(widget);
    updateSplitter();
}

wxWindow* DockSplitter::replaceWidget(wxWindow* from, wxWindow* to) {
    // Ensure 'to' widget has this splitter as parent
    if (to && to->GetParent() != this) {
        to->Reparent(this);
    }

    ReplaceWindow(from, to);

    auto it = std::find(m_widgets.begin(), m_widgets.end(), from);
    if (it != m_widgets.end()) {
        *it = to;
    }

    return from;
}

wxWindow* DockSplitter::widget(int index) const {
    if (index >= 0 && index < static_cast<int>(m_widgets.size())) {
        return m_widgets[index];
    }
    return nullptr;
}

int DockSplitter::indexOf(wxWindow* widget) const {
    auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);
    if (it != m_widgets.end()) {
        return std::distance(m_widgets.begin(), it);
    }
    return -1;
}

int DockSplitter::widgetCount() const {
    return m_widgets.size();
}

bool DockSplitter::hasVisibleContent() const {
    for (auto* widget : m_widgets) {
        if (widget && widget->IsShown()) {
            return true;
        }
    }
    return false;
}

void DockSplitter::setSizes(const std::vector<int>& sizes) {
    m_sizes = sizes;

    if (IsSplit() && !sizes.empty()) {
        SetSashPosition(sizes[0]);
    }
}

std::vector<int> DockSplitter::sizes() const {
    std::vector<int> result;

    if (IsSplit()) {
        result.push_back(GetSashPosition());
        result.push_back(GetSize().GetWidth() - GetSashPosition() - GetSashSize());
    }

    return result;
}

void DockSplitter::OnSplitterSashPosChanging(wxSplitterEvent& event) {
    // Allow sash position changes
    event.Skip();
}

void DockSplitter::OnSplitterSashPosChanged(wxSplitterEvent& event) {
    // Update sizes
    m_sizes = sizes();

    // Notify parent DockContainerWidget that user has adjusted layout
    wxWindow* current = GetParent();
    DockContainerWidget* container = nullptr;
    
    while (current && !container) {
        container = dynamic_cast<DockContainerWidget*>(current);
        if (!container) {
            current = current->GetParent();
        }
    }
    
    if (container) {
        container->markUserAdjustedLayout();
    }

    // Force refresh of all child windows after splitter position change
    if (GetWindow1()) {
        GetWindow1()->Refresh();
        GetWindow1()->Update();
    }
    if (GetWindow2()) {
        GetWindow2()->Refresh();
        GetWindow2()->Update();
    }

    // Refresh the splitter itself
    Refresh();
    Update();

    event.Skip();
}

void DockSplitter::updateSplitter() {
    if (m_widgets.size() >= 2 && !IsSplit()) {
        // Ensure both widgets have this splitter as parent
        if (m_widgets[0]->GetParent() != this) {
            m_widgets[0]->Reparent(this);
        }
        if (m_widgets[1]->GetParent() != this) {
            m_widgets[1]->Reparent(this);
        }

        if (m_orientation == wxVERTICAL) {
            SplitVertically(m_widgets[0], m_widgets[1]);
        } else {
            SplitHorizontally(m_widgets[0], m_widgets[1]);
        }
    }
}

} // namespace ads
