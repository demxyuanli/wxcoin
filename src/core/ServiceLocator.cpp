#include "interfaces/ServiceLocator.h"
#include "interfaces/ISubsystemFactory.h"

namespace {
	ISubsystemFactory* g_factory = nullptr;
}

void ServiceLocator::setFactory(ISubsystemFactory* factory) { g_factory = factory; }
ISubsystemFactory* ServiceLocator::getFactory() { return g_factory; }