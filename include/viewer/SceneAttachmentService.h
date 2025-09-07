#pragma once

#include <memory>
#include <unordered_map>

class SoSeparator;
class OCCGeometry;

class SceneAttachmentService {
public:
	SceneAttachmentService(SoSeparator* occRoot,
		std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* nodeToGeom);

	void attach(std::shared_ptr<OCCGeometry> geometry);
	void detach(std::shared_ptr<OCCGeometry> geometry);
	void detachAll();

private:
	SoSeparator* m_occRoot;
	std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* m_nodeToGeom;
};
