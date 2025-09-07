#pragma once

class IOutlineApi {
public:
	virtual ~IOutlineApi() = default;

	// Enable/disable outline rendering for scene geometries
	virtual void setOutlineEnabled(bool enabled) = 0;
	virtual bool isOutlineEnabled() const = 0;

	// Force re-evaluation/attachment for all geometries
	virtual void refreshOutlineAll() = 0;
};
