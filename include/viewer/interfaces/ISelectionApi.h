#pragma once

#include <string>

class Quantity_Color;

class ISelectionApi {
public:
    virtual ~ISelectionApi() = default;

    virtual void setGeometryVisible(const std::string& name, bool visible) = 0;
    virtual void setGeometrySelected(const std::string& name, bool selected) = 0;
    virtual void setGeometryColor(const std::string& name, const Quantity_Color& color) = 0;
    virtual void setGeometryTransparency(const std::string& name, double transparency) = 0;

    virtual void hideAll() = 0;
    virtual void showAll() = 0;
    virtual void selectAll() = 0;
    virtual void deselectAll() = 0;
};


