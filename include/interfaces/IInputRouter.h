#pragma once

class IInputRouter {
public:
    enum class Mode { View, Create };
    virtual ~IInputRouter() = default;
    virtual void setMode(Mode mode) = 0;
};


