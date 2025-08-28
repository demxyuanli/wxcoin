#pragma once

#include "RenderBackend.h"
#include "RenderConfig.h"
#include <memory>

// Forward declarations
class SoSeparator;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoIndexedLineSet;
class SoNormal;
class SoMaterial;
class SoShapeHints;
class SoNormalBinding;
class SoTexture2;

/**
 * @brief Coin3D rendering backend implementation
 *
 * Implements rendering using Coin3D library
 */
class Coin3DBackendImpl : public Coin3DBackend {
public:
	Coin3DBackendImpl();
	virtual ~Coin3DBackendImpl();

	// RenderBackend interface
	bool initialize(const std::string& config = "") override;
	void shutdown() override;

	SoSeparatorPtr createSceneNode(const TriangleMesh& mesh, bool selected,
		const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
		const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
		double shininess, double transparency) override;
	void updateSceneNode(SoSeparator* node, const TriangleMesh& mesh) override;
	void updateSceneNode(SoSeparator* node, const TopoDS_Shape& shape, const MeshParameters& params) override;

	SoSeparatorPtr createSceneNode(const TopoDS_Shape& shape,
		const MeshParameters& params = MeshParameters(),
		bool selected = false) override;
	SoSeparatorPtr createSceneNode(const TopoDS_Shape& shape,
		const MeshParameters& params,
		bool selected,
		const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
		const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
		double shininess, double transparency) override;

	void setEdgeSettings(bool show, double angle = 45.0) override;
	void setSmoothingSettings(bool enabled, double creaseAngle = 30.0, int iterations = 2) override;
	void setSubdivisionSettings(bool enabled, int levels = 2) override;

	std::string getName() const override { return "Coin3D"; }
	bool isAvailable() const override;

	// Coin3DBackend interface

	SoSeparator* createCoinNode(const TriangleMesh& mesh, bool selected,
		const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
		const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
		double shininess, double transparency) override;
	void updateCoinNode(SoSeparator* node, const TriangleMesh& mesh) override;

	SoCoordinate3* createCoordinateNode(const TriangleMesh& mesh) override;
	SoIndexedFaceSet* createFaceSetNode(const TriangleMesh& mesh) override;
	SoNormal* createNormalNode(const TriangleMesh& mesh) override;
	SoIndexedLineSet* createEdgeSetNode(const TriangleMesh& mesh) override;

private:
	// Helper methods

	void buildCoinNodeStructure(SoSeparator* node, const TriangleMesh& mesh, bool selected,
		const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
		const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
		double shininess, double transparency);
	SoShapeHints* createShapeHints();
	SoNormalBinding* createNormalBinding();
	SoMaterial* createEdgeMaterial(bool selected = false);
	SoTexture2* createDisableTexture();

	// Configuration
	RenderConfig& m_config;
	std::unique_ptr<GeometryProcessor> m_geometryProcessor;
};