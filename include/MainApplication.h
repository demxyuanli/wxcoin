#ifndef MAIN_APPLICATION_HPP
#define MAIN_APPLICATION_HPP

#include <wx/wx.h>
#include <memory>

// Forward declarations
class UnifiedRefreshSystem;
class CommandDispatcher;

class MainApplication : public wxApp {
public:
    bool OnInit() override;
    
private:
    // Global application services
    static std::unique_ptr<UnifiedRefreshSystem> s_unifiedRefreshSystem;
    static std::unique_ptr<CommandDispatcher> s_commandDispatcher;
    
    // Initialize and shutdown global services
    bool initializeGlobalServices();
    void shutdownGlobalServices();
};
#endif // MAIN_APPLICATION_HPP