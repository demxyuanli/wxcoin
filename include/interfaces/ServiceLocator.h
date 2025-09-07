#pragma once

class ISubsystemFactory;

class ServiceLocator {
public:
    static void setFactory(ISubsystemFactory* factory);
    static ISubsystemFactory* getFactory();
};


