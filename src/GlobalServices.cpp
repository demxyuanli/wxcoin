#include "GlobalServices.h"
#include "UnifiedRefreshSystem.h"
#include "CommandDispatcher.h"

// Static member definitions
UnifiedRefreshSystem* GlobalServices::s_refreshSystem = nullptr;
CommandDispatcher* GlobalServices::s_commandDispatcher = nullptr;

UnifiedRefreshSystem* GlobalServices::GetRefreshSystem()
{
    return s_refreshSystem;
}

CommandDispatcher* GlobalServices::GetCommandDispatcher()
{
    return s_commandDispatcher;
}

void GlobalServices::SetRefreshSystem(UnifiedRefreshSystem* system)
{
    s_refreshSystem = system;
}

void GlobalServices::SetCommandDispatcher(CommandDispatcher* dispatcher)
{
    s_commandDispatcher = dispatcher;
}

void GlobalServices::Clear()
{
    s_refreshSystem = nullptr;
    s_commandDispatcher = nullptr;
} 