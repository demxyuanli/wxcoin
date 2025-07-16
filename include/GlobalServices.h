#ifndef GLOBAL_SERVICES_H
#define GLOBAL_SERVICES_H

// Forward declarations
class UnifiedRefreshSystem;
class CommandDispatcher;

/**
 * Global services manager - provides access to application-wide services
 * without creating dependencies on MainApplication.h
 */
class GlobalServices {
public:
    // Get global services instances
    static UnifiedRefreshSystem* GetRefreshSystem();
    static CommandDispatcher* GetCommandDispatcher();
    
    // Set global services instances (called by MainApplication)
    static void SetRefreshSystem(UnifiedRefreshSystem* system);
    static void SetCommandDispatcher(CommandDispatcher* dispatcher);
    
    // Cleanup
    static void Clear();

private:
    static UnifiedRefreshSystem* s_refreshSystem;
    static CommandDispatcher* s_commandDispatcher;
};

#endif // GLOBAL_SERVICES_H 