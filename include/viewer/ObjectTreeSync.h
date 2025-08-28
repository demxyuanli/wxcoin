#pragma once

#include <memory>
#include <vector>

class SceneManager;
class OCCGeometry;

class ObjectTreeSync {
public:
	ObjectTreeSync(SceneManager* sceneManager,
		std::vector<std::shared_ptr<OCCGeometry>>* pendingQueue);

	void addGeometry(std::shared_ptr<OCCGeometry> geometry, bool batchMode);
	void removeGeometry(std::shared_ptr<OCCGeometry> geometry);
	void processDeferred();

private:
	SceneManager* m_sceneManager;
	std::vector<std::shared_ptr<OCCGeometry>>* m_pendingQueue;
};
