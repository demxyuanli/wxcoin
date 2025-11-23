#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbVec3f.h>
#include <string>
#include <memory>

class SoSeparator;
class ObjectTreePanel;
class PropertyPanel;
class CommandManager;
class OCCViewer;
class OCCGeometry;
class GeometryCreator; // Forward declaration
struct BasicGeometryParameters; // Forward declaration

enum class GeometryType {
	COIN3D,     // Traditional Coin3D geometry
	OPENCASCADE // OpenCASCADE geometry
};

class GeometryFactory {
public:
	GeometryFactory(SoSeparator* root, ObjectTreePanel* treePanel, PropertyPanel* propPanel,
		CommandManager* cmdManager, OCCViewer* occViewer);
	~GeometryFactory();

	// Create geometry with default parameters
	void createOCCGeometry(const std::string& type, const SbVec3f& position);

	// Create geometry with custom parameters
	std::shared_ptr<OCCGeometry> createOCCGeometryWithParameters(const std::string& type, const SbVec3f& position, const BasicGeometryParameters& params);

	// Create geometry with material parameters
	void createOCCGeometryWithMaterial(const std::string& type, const SbVec3f& position, const BasicGeometryParameters& params);

	// Individual geometry creation methods
	std::shared_ptr<OCCGeometry> createOCCBox(const SbVec3f& position);
	std::shared_ptr<OCCGeometry> createOCCBox(const SbVec3f& position, double width, double height, double depth);

	std::shared_ptr<OCCGeometry> createOCCSphere(const SbVec3f& position);
	std::shared_ptr<OCCGeometry> createOCCSphere(const SbVec3f& position, double radius);

	std::shared_ptr<OCCGeometry> createOCCCylinder(const SbVec3f& position);
	std::shared_ptr<OCCGeometry> createOCCCylinder(const SbVec3f& position, double radius, double height);

	std::shared_ptr<OCCGeometry> createOCCCone(const SbVec3f& position);
	std::shared_ptr<OCCGeometry> createOCCCone(const SbVec3f& position, double bottomRadius, double topRadius, double height);

	std::shared_ptr<OCCGeometry> createOCCTorus(const SbVec3f& position);
	std::shared_ptr<OCCGeometry> createOCCTorus(const SbVec3f& position, double majorRadius, double minorRadius);

	std::shared_ptr<OCCGeometry> createOCCTruncatedCylinder(const SbVec3f& position);
	std::shared_ptr<OCCGeometry> createOCCTruncatedCylinder(const SbVec3f& position, double bottomRadius, double topRadius, double height);

	std::shared_ptr<OCCGeometry> createOCCWrench(const SbVec3f& position);

	std::shared_ptr<OCCGeometry> createOCCNavCube(const SbVec3f& position);
	std::shared_ptr<OCCGeometry> createOCCNavCube(const SbVec3f& position, double size);

	// Set default geometry type
	void setDefaultGeometryType(GeometryType type) { m_defaultGeometryType = type; }
	GeometryType getDefaultGeometryType() const { return m_defaultGeometryType; }

	// Culling system integration
	void addGeometryToCullingSystem(const std::shared_ptr<OCCGeometry>& geometry);

private:
	// Helper function to build high-quality face index mapping for system-created geometries
	void buildFaceIndexMappingForSystemGeometry(std::shared_ptr<OCCGeometry> geometry, const std::string& geometryName);

	SoSeparator* m_root;
	ObjectTreePanel* m_treePanel;
	PropertyPanel* m_propPanel;
	CommandManager* m_cmdManager;
	OCCViewer* m_occViewer;
	GeometryType m_defaultGeometryType;
};