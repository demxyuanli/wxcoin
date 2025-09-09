#pragma once

#include "CommandListener.h"
#include "OCCViewer.h"
#include "NormalFixDialog.h"
#include <memory>
#include <wx/frame.h>

/**
 * @brief Command listener for normal fix dialog
 */
class NormalFixDialogListener : public CommandListener {
public:
    explicit NormalFixDialogListener(wxFrame* frame, OCCViewer* viewer);
    
    CommandResult executeCommand(const std::string& commandType,
                                const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    
    std::string getListenerName() const override;

private:
    wxFrame* m_frame;
    OCCViewer* m_viewer;
};
