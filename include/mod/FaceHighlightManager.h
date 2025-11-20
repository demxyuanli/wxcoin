#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

class OCCGeometry;
struct TriangleMesh;
struct MeshParameters;

/**
 * @brief Manages face highlighting in Coin3D scene graph
 * 
 * Improved implementation inspired by FreeCAD's preselection mechanism:
 * - Uses SoSwitch nodes for efficient highlight toggling
 * - Caches highlight nodes to avoid recreation
 * - Supports both hover (preselection) and selection highlights
 */
class FaceHighlightManager
{
public:
	FaceHighlightManager();
	~FaceHighlightManager();

	// Highlight a specific face (preselection/hover)
	bool highlightFace(std::shared_ptr<OCCGeometry> geometry, int faceId, SoSeparator* geometryRootNode);
	
	// Select a face (permanent highlight until cleared)
	bool selectFace(std::shared_ptr<OCCGeometry> geometry, int faceId, SoSeparator* geometryRootNode);
	
	// Clear current highlight (preselection)
	void clearHighlight();
	
	// Clear current selection
	void clearSelection();
	
	// Check if a face is currently highlighted
	bool isHighlighting() const { return m_highlightNode != nullptr; }
	
	// Check if a face is currently selected
	bool isSelecting() const { return m_selectedNode != nullptr; }
	
	// Get currently highlighted geometry and face ID
	std::shared_ptr<OCCGeometry> getHighlightedGeometry() const { return m_highlightedGeometry; }
	int getHighlightedFaceId() const { return m_highlightedFaceId; }
	
	// Get currently selected geometry and face ID
	std::shared_ptr<OCCGeometry> getSelectedGeometry() const { return m_selectedGeometry; }
	int getSelectedFaceId() const { return m_selectedFaceId; }

private:
	// Get or create cached highlight node for a face
	SoSwitch* getOrCreateHighlightNode(std::shared_ptr<OCCGeometry> geometry, int faceId, bool isSelection = false);
	
	// Create highlight geometry node for a specific face
	SoSeparator* createHighlightGeometry(std::shared_ptr<OCCGeometry> geometry, int faceId, bool isSelection = false);
	
	// Extract face mesh data
	bool extractFaceMesh(std::shared_ptr<OCCGeometry> geometry, int faceId, TriangleMesh& faceMesh);

	// Extract mesh data from Coin3D node
	bool extractMeshFromCoinNode(SoSeparator* rootNode, TriangleMesh& mesh);

	// Generate cache key for highlight node
	std::string getCacheKey(std::shared_ptr<OCCGeometry> geometry, int faceId, bool isSelection) const;

	// Preselection (hover) state
	SoSwitch* m_highlightNode;
	SoSeparator* m_geometryRootNode;
	std::shared_ptr<OCCGeometry> m_highlightedGeometry;
	int m_highlightedFaceId;
	
	// Selection (permanent) state
	SoSwitch* m_selectedNode;
	SoSeparator* m_selectedGeometryRootNode;
	std::shared_ptr<OCCGeometry> m_selectedGeometry;
	int m_selectedFaceId;
	
	// Cache of highlight nodes: cacheKey -> SoSwitch node
	std::unordered_map<std::string, SoSwitch*> m_highlightCache;
};


