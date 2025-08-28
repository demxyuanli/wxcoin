#include "flatui/UIHierarchyDebugger.h"
#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIHomeSpace.h"
#include "flatui/FlatUIFunctionSpace.h"
#include "flatui/FlatUIProfileSpace.h"
#include "flatui/FlatUISystemButtons.h"
#include "flatui/FlatUIGallery.h"
#include "flatui/FlatUICustomControl.h"

UIHierarchyDebugger::UIHierarchyDebugger()
	: m_logTextCtrl(nullptr), m_oldLog(nullptr), m_usingCustomLog(false)
{
}

UIHierarchyDebugger::~UIHierarchyDebugger()
{
	if (m_usingCustomLog && m_oldLog) {
		wxLog::SetActiveTarget(m_oldLog);
		m_usingCustomLog = false;
	}
}

void UIHierarchyDebugger::SetLogTextCtrl(wxTextCtrl* textCtrl)
{
	m_logTextCtrl = textCtrl;
}

void UIHierarchyDebugger::PrintUIHierarchy(wxWindow* window)
{
	if (!window) {
		return;
	}

	if (m_logTextCtrl) {
		m_oldLog = wxLog::SetActiveTarget(new wxLogTextCtrl(m_logTextCtrl));
		m_usingCustomLog = true;
	}

	wxLogDebug("----- UI HIERARCHY DEBUG -----");
	DebugUIHierarchy(window, 0);
	wxLogDebug("----- END UI HIERARCHY -----");

	if (m_usingCustomLog && m_oldLog) {
		wxLog::SetActiveTarget(m_oldLog);
		m_usingCustomLog = false;
	}
}

void UIHierarchyDebugger::DebugUIHierarchy(wxWindow* window, int depth)
{
	if (!window) {
		return;
	}

	wxString indent;
	for (int i = 0; i < depth; i++) {
		indent += "  ";
	}

	wxString basicClassName = window->GetClassInfo()->GetClassName();

	wxString specificClassName = basicClassName;

	if (FlatUIBar* bar = dynamic_cast<FlatUIBar*>(window)) {
		specificClassName = "FlatUIBar";
	}
	else if (FlatUIPage* page = dynamic_cast<FlatUIPage*>(window)) {
		specificClassName = "FlatUIPage:" + page->GetLabel();
	}
	else if (FlatUIPanel* panel = dynamic_cast<FlatUIPanel*>(window)) {
		specificClassName = "FlatUIPanel:" + panel->GetLabel();
	}
	else if (FlatUIButtonBar* buttonBar = dynamic_cast<FlatUIButtonBar*>(window)) {
		specificClassName = "FlatUIButtonBar";
		size_t buttonCount = buttonBar->GetButtonCount();
		if (buttonCount > 0) {
			specificClassName += " (" + wxString::Format("%zu", buttonCount) + " buttons)";
			wxLogDebug("%s%s Infomation of Button:", indent, specificClassName);
			wxPoint barPos = buttonBar->GetPosition();
			wxSize barSize = buttonBar->GetSize();
			wxLogDebug("%s  ButtonBar Position: (%d,%d) Size: (%d,%d)",
				indent + "  ",
				barPos.x, barPos.y,
				barSize.GetWidth(), barSize.GetHeight());

			wxLogDebug("%s  Note: ButtonBar uses custom drawn buttons instead of wxWindow objects.",
				indent + "  ");
			wxLogDebug("%s  These buttons are stored internally and not as child windows.",
				indent + "  ");

			wxLogDebug("%s  Button information from internal storage:", indent + "  ");

			for (size_t i = 0; i < buttonCount; i++) {
				wxLogDebug("%s  - Button %zu [Custom drawn button, not a wxWindow]",
					indent + "  ", i + 1);
			}
			wxWindowList children = buttonBar->GetChildren();
			for (wxWindowList::compatibility_iterator node = children.GetFirst(); node; node = node->GetNext()) {
				wxWindow* child = node->GetData();
				wxString controlName = child->GetName();
				wxString controlClassName = child->GetClassInfo()->GetClassName();

				if (controlName.IsEmpty()) {
					controlName = wxString::Format("Control_ID_%d", child->GetId());
				}

				wxPoint pos = child->GetPosition();
				wxSize size = child->GetSize();

				wxString extraInfo;

				if (wxBitmapButton* button = dynamic_cast<wxBitmapButton*>(child)) {
					extraInfo = " (BitmapButton)";
				}
				else if (wxButton* button = dynamic_cast<wxButton*>(child)) {
					extraInfo = wxString::Format(" (Button: \"%s\")", button->GetLabel());
				}
				else if (wxStaticText* text = dynamic_cast<wxStaticText*>(child)) {
					wxString label = text->GetLabel();
					if (label.Length() > 20) {
						label = label.Left(20) + "...";
					}
					extraInfo = wxString::Format(" (Text: \"%s\")", label);
				}
				else if (wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>(child)) {
					extraInfo = " (TextCtrl)";
					if (textCtrl->IsMultiLine()) {
						extraInfo += " MultiLine";
					}
				}
				else if (wxCheckBox* checkbox = dynamic_cast<wxCheckBox*>(child)) {
					extraInfo = wxString::Format(" (CheckBox: \"%s\" %s)",
						checkbox->GetLabel(),
						checkbox->GetValue() ? "Checked" : "Unchecked");
				}

				wxString controlDesc = wxString::Format("%s  - %s [%s %p] ID:%d Pos:(%d,%d) Size:(%d,%d)%s %s",
					indent + "  ",
					controlName,
					controlClassName,
					child,
					child->GetId(),
					pos.x, pos.y,
					size.GetWidth(), size.GetHeight(),
					extraInfo,
					child->IsShown() ? "" : "[Hidden]");

				wxLogDebug("%s", controlDesc);
			}
		}
	}
	else if (FlatUIHomeSpace* homeSpace = dynamic_cast<FlatUIHomeSpace*>(window)) {
		specificClassName = "FlatUIHomeSpace";
	}
	else if (FlatUIFunctionSpace* funcSpace = dynamic_cast<FlatUIFunctionSpace*>(window)) {
		specificClassName = "FlatUIFunctionSpace";
	}
	else if (FlatUIProfileSpace* profSpace = dynamic_cast<FlatUIProfileSpace*>(window)) {
		specificClassName = "FlatUIProfileSpace";
	}
	else if (FlatUISystemButtons* sysButtons = dynamic_cast<FlatUISystemButtons*>(window)) {
		specificClassName = "FlatUISystemButtons";
	}
	else if (FlatUIGallery* gallery = dynamic_cast<FlatUIGallery*>(window)) {
		specificClassName = "FlatUIGallery";
		size_t itemCount = gallery->GetItemCount();
		if (itemCount > 0) {
			specificClassName += " (" + wxString::Format("%zu", itemCount) + " items)";
		}
	}
	else if (FlatUICustomControl* custom = dynamic_cast<FlatUICustomControl*>(window)) {
		specificClassName = "FlatUICustomControl";
		if (!custom->GetLabel().IsEmpty()) {
			specificClassName += ":" + custom->GetLabel();
		}
	}
	else if (wxTextCtrl* text = dynamic_cast<wxTextCtrl*>(window)) {
		specificClassName = "wxTextCtrl";
		if (text->GetValue().Length() > 0) {
			specificClassName += " (with text)";
		}
	}
	else if (wxButton* button = dynamic_cast<wxButton*>(window)) {
		specificClassName = "wxButton";
		if (!button->GetLabel().IsEmpty()) {
			specificClassName += ": \"" + button->GetLabel() + "\"";
		}
	}
	else if (wxBitmapButton* bmpButton = dynamic_cast<wxBitmapButton*>(window)) {
		specificClassName = "wxBitmapButton";
	}
	else if (wxStaticText* staticText = dynamic_cast<wxStaticText*>(window)) {
		specificClassName = "wxStaticText";
		wxString label = staticText->GetLabel();
		if (!label.IsEmpty()) {
			if (label.Length() > 30) {
				label = label.Left(30) + "...";
			}
			specificClassName += ": \"" + label + "\"";
		}
	}
	else if (wxCheckBox* checkbox = dynamic_cast<wxCheckBox*>(window)) {
		specificClassName = "wxCheckBox";
		wxString state = checkbox->GetValue() ? " [Checked]" : " [Unchecked]";
		specificClassName += ": \"" + checkbox->GetLabel() + "\"" + state;
	}
	else if (wxRadioButton* radio = dynamic_cast<wxRadioButton*>(window)) {
		specificClassName = "wxRadioButton";
		wxString state = radio->GetValue() ? " [Selected]" : " [Unselected]";
		specificClassName += ": \"" + radio->GetLabel() + "\"" + state;
	}
	else if (wxChoice* choice = dynamic_cast<wxChoice*>(window)) {
		specificClassName = "wxChoice";
		int selection = choice->GetSelection();
		if (selection != wxNOT_FOUND) {
			specificClassName += wxString::Format(" [Selected: %d - \"%s\"]",
				selection, choice->GetString(selection));
		}
		else {
			specificClassName += " [No selection]";
		}
	}
	else if (wxComboBox* combo = dynamic_cast<wxComboBox*>(window)) {
		specificClassName = "wxComboBox";
		specificClassName += wxString::Format(" [Items: %d]", combo->GetCount());
		int selection = combo->GetSelection();
		if (selection != wxNOT_FOUND) {
			specificClassName += wxString::Format(" [Selected: %d - \"%s\"]",
				selection, combo->GetString(selection));
		}
	}
	else if (wxListBox* listbox = dynamic_cast<wxListBox*>(window)) {
		specificClassName = "wxListBox";
		specificClassName += wxString::Format(" [Items: %d]", listbox->GetCount());
	}
	else if (wxPanel* panel = dynamic_cast<wxPanel*>(window)) {
		specificClassName = "wxPanel";
		if (panel->GetSizer()) {
			wxString sizerType;
			if (dynamic_cast<wxBoxSizer*>(panel->GetSizer())) {
				wxBoxSizer* boxSizer = dynamic_cast<wxBoxSizer*>(panel->GetSizer());
				sizerType = boxSizer->GetOrientation() == wxVERTICAL ? "wxBoxSizer(V)" : "wxBoxSizer(H)";
			}
			else if (dynamic_cast<wxGridSizer*>(panel->GetSizer())) {
				sizerType = "wxGridSizer";
			}
			else if (dynamic_cast<wxFlexGridSizer*>(panel->GetSizer())) {
				sizerType = "wxFlexGridSizer";
			}
			else {
				sizerType = "Other Sizer";
			}
			specificClassName += " with " + sizerType;
		}
	}

	wxString additionalInfo;

	if (!window->GetName().IsEmpty()) {
		additionalInfo += " Name:\"" + window->GetName() + "\"";
	}

	additionalInfo += wxString::Format(" ID:%d", window->GetId());

	if (!window->IsShown()) {
		additionalInfo += " [Hidden]";
	}

	wxLogDebug("%s%s [%p]%s - Pos:(%d,%d) Size:(%d,%d) Shown:%d",
		indent,
		specificClassName,
		window,
		additionalInfo,
		window->GetPosition().x, window->GetPosition().y,
		window->GetSize().GetWidth(), window->GetSize().GetHeight(),
		window->IsShown());

	for (wxWindowList::compatibility_iterator node = window->GetChildren().GetFirst();
		node;
		node = node->GetNext()) {
		wxWindow* child = node->GetData();
		DebugUIHierarchy(child, depth + 1);
	}
}