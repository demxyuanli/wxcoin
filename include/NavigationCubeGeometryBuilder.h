#pragma once

#include <Inventor/SbLinear.h>
#include <Inventor/SbName.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoTransform.h>
#include <map>
#include <string>
#include <vector>

#include "NavigationCubeTypes.h"

class NavigationCubeGeometryBuilder {
public:
	struct FaceData {
		FaceData() : type(ShapeId::Main) {}
		ShapeId type;
		SbRotation rotation;
		std::vector<SbVec3f> vertexArray;
	};

	struct LabelTextureData {
		std::vector<SbVec3f> vertexArray;
	};

	struct BuildParams {
		float chamferSize = 0.12f;
		float geometrySize = 0.55f;
		bool showEdges = true;
		bool showCorners = true;
	};

	struct BuildResult {
		SoSeparator* geometryRoot = nullptr;
		SoTransform* geometryTransform = nullptr;
		std::map<PickId, FaceData> faces;
		std::map<PickId, LabelTextureData> labelTextures;
		std::map<std::string, SoMaterial*> faceMaterials;
		std::map<std::string, SoSeparator*> faceSeparators;
		std::map<std::string, SbColor> faceBaseColors;
		std::map<std::string, SbColor> faceHoverColors;
		std::map<std::string, SoMaterial*> faceTextureMaterials;
	};

	NavigationCubeGeometryBuilder() = default;

	BuildResult build(const BuildParams& params);

private:
	void addCubeFace(const SbVec3f& x, const SbVec3f& z, ShapeId shapeType, PickId pickId, float rotZ);

	float m_chamferSize = 0.12f;
	float m_geometrySize = 0.55f;
	bool m_showEdges = true;
	bool m_showCorners = true;
	std::map<PickId, FaceData> m_faces;
	std::map<PickId, LabelTextureData> m_labelTextures;
};
