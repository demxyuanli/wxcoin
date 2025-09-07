#pragma once

#include <OpenCASCADE/gp_Pnt.hxx>

class IViewApi {
public:
	virtual ~IViewApi() = default;

	virtual void fitAll() = 0;
	virtual void fitGeometry(const std::string& name) = 0;
	virtual void requestViewRefresh() = 0;
	virtual gp_Pnt getCameraPosition() const = 0;
	virtual void setPreserveViewOnAdd(bool preserve) = 0;
	virtual bool isPreserveViewOnAdd() const = 0;
};
