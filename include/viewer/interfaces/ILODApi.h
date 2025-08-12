#pragma once

class ILODApi {
public:
    virtual ~ILODApi() = default;

    virtual void setLODEnabled(bool enabled) = 0;
    virtual bool isLODEnabled() const = 0;
    virtual void setLODRoughDeflection(double deflection) = 0;
    virtual double getLODRoughDeflection() const = 0;
    virtual void setLODFineDeflection(double deflection) = 0;
    virtual double getLODFineDeflection() const = 0;
    virtual void setLODTransitionTime(int milliseconds) = 0;
    virtual int getLODTransitionTime() const = 0;
    virtual void setLODMode(bool roughMode) = 0;
    virtual bool isLODRoughMode() const = 0;
    virtual void startLODInteraction() = 0;
};



