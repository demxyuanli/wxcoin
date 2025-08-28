#pragma once

#include <string>
#include <Inventor/SbColor.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/RenderingConfig.h"
#include "config/EdgeSettingsConfig.h"

struct BasicGeometryParameters {
	std::string geometryType;
	// Box
	double width = 2.0;
	double height = 2.0;
	double depth = 2.0;
	// Sphere
	double radius = 1.0;
	// Cylinder
	double cylinderRadius = 1.0;
	double cylinderHeight = 2.0;
	// Cone
	double bottomRadius = 1.0;
	double topRadius = 0.0;
	double coneHeight = 2.0;
	// Torus
	double majorRadius = 2.0;
	double minorRadius = 0.5;
	// Truncated Cylinder
	double truncatedBottomRadius = 1.0;
	double truncatedTopRadius = 0.5;
	double truncatedHeight = 2.0;
};

struct AdvancedGeometryParameters {
	Quantity_Color materialDiffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
	Quantity_Color materialAmbientColor = Quantity_Color(0.2, 0.2, 0.2, Quantity_TOC_RGB);
	Quantity_Color materialSpecularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
	double materialShininess = 50.0;
	double materialTransparency = 0.0;
	Quantity_Color materialEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);

	// Texture
	std::string texturePath;
	RenderingConfig::TextureMode textureMode = RenderingConfig::TextureMode::Modulate;
	bool textureEnabled = false;

	// Rendering
	RenderingConfig::RenderingQuality renderingQuality = RenderingConfig::RenderingQuality::Normal;
	RenderingConfig::BlendMode blendMode = RenderingConfig::BlendMode::None;
	RenderingConfig::LightingModel lightingModel = RenderingConfig::LightingModel::BlinnPhong;
	bool backfaceCulling = true;
	bool depthTest = true;

	// Display
	bool showNormals = false;
	bool showEdges = false;
	bool showWireframe = false;
	bool showSilhouette = false;
	bool showFeatureEdges = false;
	bool showMeshEdges = false;
	bool showOriginalEdges = false;
	bool showFaceNormals = false;

	// Subdivision
	bool subdivisionEnabled = false;
	int subdivisionLevels = 1;

	// Edge Settings
	int edgeStyle = 0; // 0=Solid, 1=Dashed, 2=Dotted
	double edgeWidth = 1.0;
	Quantity_Color edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
	bool edgeEnabled = true;
};