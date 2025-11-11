#include "NavigationCubeGeometryBuilder.h"

#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>

#include "config/ConfigManager.h"
#include "logger/Logger.h"

namespace {
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kHalfPi = kPi / 2.0f;
}

NavigationCubeGeometryBuilder::BuildResult NavigationCubeGeometryBuilder::build(const BuildParams& params) {
	m_chamferSize = params.chamferSize;
	m_geometrySize = params.geometrySize;
	m_showEdges = params.showEdges;
	m_showCorners = params.showCorners;

	m_faces.clear();
	m_labelTextures.clear();

	BuildResult result;

	result.geometryRoot = new SoSeparator;
	result.geometryRoot->ref();

	result.geometryTransform = new SoTransform;
	result.geometryTransform->scaleFactor.setValue(m_geometrySize, m_geometrySize, m_geometrySize);
	result.geometryRoot->addChild(result.geometryTransform);

	SoSeparator* cubeAssembly = new SoSeparator;
	cubeAssembly->setName("RhombicuboctahedronAssembly");
	result.geometryRoot->addChild(cubeAssembly);

	SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
	texCoords->point.setValues(0, 4, new SbVec2f[4]{
		SbVec2f(0.0f, 0.0f),
		SbVec2f(1.0f, 0.0f),
		SbVec2f(1.0f, 1.0f),
		SbVec2f(0.0f, 1.0f)
	});
	cubeAssembly->addChild(texCoords);

	SoLightModel* lightModel = new SoLightModel;
	lightModel->model = SoLightModel::BASE_COLOR;
	cubeAssembly->addChild(lightModel);

	SoCoordinate3* coords = new SoCoordinate3;
	cubeAssembly->addChild(coords);

	auto& config = ConfigManager::getInstance();

	SoMaterial* mainFaceMaterial = new SoMaterial;
	mainFaceMaterial->diffuseColor.setValue(
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseR", 0.9)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseG", 0.95)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseB", 1.0))
	);
	mainFaceMaterial->ambientColor.setValue(
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyAmbientR", 0.7)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyAmbientG", 0.8)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyAmbientB", 0.9))
	);
	mainFaceMaterial->specularColor.setValue(
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodySpecularR", 0.95)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodySpecularG", 0.98)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodySpecularB", 1.0))
	);
	mainFaceMaterial->emissiveColor.setValue(
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyEmissiveR", 0.02)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyEmissiveG", 0.05)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyEmissiveB", 0.1))
	);
	mainFaceMaterial->shininess.setValue(static_cast<float>(config.getDouble("NavigationCube", "CubeBodyShininess", 0.0f)));
	mainFaceMaterial->transparency.setValue(static_cast<float>(config.getDouble("NavigationCube", "CubeBodyTransparency", 0.0f)));

	SoMaterial* edgeAndCornerMaterial = new SoMaterial;
	edgeAndCornerMaterial->diffuseColor.setValue(
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseR", 0.9)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseG", 0.95)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseB", 1.0))
	);
	edgeAndCornerMaterial->ambientColor.setValue(
		static_cast<float>(config.getDouble("NavigationCube", "EdgeCornerMaterialAmbientR", 0.3)),
		static_cast<float>(config.getDouble("NavigationCube", "EdgeCornerMaterialAmbientG", 0.5)),
		static_cast<float>(config.getDouble("NavigationCube", "EdgeCornerMaterialAmbientB", 0.3))
	);
	edgeAndCornerMaterial->specularColor.setValue(0.0f, 0.0f, 0.0f);
	edgeAndCornerMaterial->emissiveColor.setValue(
		static_cast<float>(config.getDouble("NavigationCube", "EdgeCornerMaterialEmissiveR", 0.04)),
		static_cast<float>(config.getDouble("NavigationCube", "EdgeCornerMaterialEmissiveG", 0.12)),
		static_cast<float>(config.getDouble("NavigationCube", "EdgeCornerMaterialEmissiveB", 0.04))
	);
	edgeAndCornerMaterial->shininess.setValue(0.0f);
	edgeAndCornerMaterial->transparency.setValue(0.0f);

	SbColor baseColor(
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseR", 0.9)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseG", 0.95)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseB", 1.0))
	);

	SbColor hoverColor(
		static_cast<float>(config.getDouble("NavigationCube", "CubeHoverColorR", 0.7)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeHoverColorG", 0.85)),
		static_cast<float>(config.getDouble("NavigationCube", "CubeHoverColorB", 0.95))
	);

	result.faceBaseColors["FRONT"] = baseColor;
	result.faceBaseColors["REAR"] = baseColor;
	result.faceBaseColors["LEFT"] = baseColor;
	result.faceBaseColors["RIGHT"] = baseColor;
	result.faceBaseColors["TOP"] = baseColor;
	result.faceBaseColors["BOTTOM"] = baseColor;

	result.faceHoverColors["FRONT"] = hoverColor;
	result.faceHoverColors["REAR"] = hoverColor;
	result.faceHoverColors["LEFT"] = hoverColor;
	result.faceHoverColors["RIGHT"] = hoverColor;
	result.faceHoverColors["TOP"] = hoverColor;
	result.faceHoverColors["BOTTOM"] = hoverColor;

	const std::vector<std::string> edgeNames = {
		"EdgeTF", "EdgeTB", "EdgeTL", "EdgeTR",
		"EdgeBF", "EdgeBB", "EdgeBL", "EdgeBR",
		"EdgeFR", "EdgeFL", "EdgeBL2", "EdgeBR2"
	};

	for (const auto& name : edgeNames) {
		result.faceBaseColors[name] = baseColor;
		result.faceHoverColors[name] = hoverColor;
	}

	const std::vector<std::string> cornerNames = {
		"Corner0", "Corner1", "Corner2", "Corner3",
		"Corner4", "Corner5", "Corner6", "Corner7"
	};

	for (const auto& name : cornerNames) {
		result.faceBaseColors[name] = baseColor;
		result.faceHoverColors[name] = hoverColor;
	}

	SbVec3f x(1, 0, 0);
	SbVec3f y(0, 1, 0);
	SbVec3f z(0, 0, 1);

	addCubeFace(x, z, ShapeId::Main, PickId::Top, 0.0f);
	addCubeFace(x, -y, ShapeId::Main, PickId::Front, 0.0f);
	addCubeFace(-y, -x, ShapeId::Main, PickId::Left, 0.0f);
	addCubeFace(-x, y, ShapeId::Main, PickId::Rear, 0.0f);
	addCubeFace(y, x, ShapeId::Main, PickId::Right, 0.0f);
	addCubeFace(x, -z, ShapeId::Main, PickId::Bottom, 0.0f);

	addCubeFace(-x - y, x - y + z, ShapeId::Corner, PickId::FrontTopRight, kPi);
	addCubeFace(-x + y, -x - y + z, ShapeId::Corner, PickId::FrontTopLeft, kPi);
	addCubeFace(x + y, x - y - z, ShapeId::Corner, PickId::FrontBottomRight, 0.0f);
	addCubeFace(x - y, -x - y - z, ShapeId::Corner, PickId::FrontBottomLeft, 0.0f);
	addCubeFace(x - y, x + y + z, ShapeId::Corner, PickId::RearTopRight, kPi);
	addCubeFace(x + y, -x + y + z, ShapeId::Corner, PickId::RearTopLeft, kPi);
	addCubeFace(-x + y, x + y - z, ShapeId::Corner, PickId::RearBottomRight, 0.0f);
	addCubeFace(-x - y, -x + y - z, ShapeId::Corner, PickId::RearBottomLeft, 0.0f);

	addCubeFace(x, z - y, ShapeId::Edge, PickId::FrontTop, 0.0f);
	addCubeFace(x, -z - y, ShapeId::Edge, PickId::FrontBottom, 0.0f);
	addCubeFace(x, y - z, ShapeId::Edge, PickId::RearBottom, kPi);
	addCubeFace(x, y + z, ShapeId::Edge, PickId::RearTop, kPi);
	addCubeFace(z, x + y, ShapeId::Edge, PickId::RearRight, kHalfPi);
	addCubeFace(z, x - y, ShapeId::Edge, PickId::FrontRight, kHalfPi);
	addCubeFace(z, -x - y, ShapeId::Edge, PickId::FrontLeft, kHalfPi);
	addCubeFace(z, y - x, ShapeId::Edge, PickId::RearLeft, kHalfPi);
	addCubeFace(y, z - x, ShapeId::Edge, PickId::TopLeft, kPi);
	addCubeFace(y, x + z, ShapeId::Edge, PickId::TopRight, 0.0f);
	addCubeFace(y, x - z, ShapeId::Edge, PickId::BottomRight, 0.0f);
	addCubeFace(y, -z - x, ShapeId::Edge, PickId::BottomLeft, kPi);

	int vertexIndex = 0;

	auto insertFace = [&](const std::string& faceName, PickId pickId, int materialType) {
		if ((materialType == 1 && !m_showEdges) || (materialType == 2 && !m_showCorners)) {
			return;
		}

		SoMaterial* templateMaterial = materialType == 0 ? mainFaceMaterial : edgeAndCornerMaterial;
		SoSeparator* faceSep = new SoSeparator;
		faceSep->setName(SbName(faceName.c_str()));

		SoMaterial* faceMaterial = new SoMaterial;
		faceMaterial->diffuseColor.setValue(templateMaterial->diffuseColor[0]);
		faceMaterial->ambientColor.setValue(templateMaterial->ambientColor[0]);
		faceMaterial->specularColor.setValue(templateMaterial->specularColor[0]);
		faceMaterial->emissiveColor.setValue(templateMaterial->emissiveColor[0]);
		faceMaterial->shininess.setValue(templateMaterial->shininess[0]);
		faceMaterial->transparency.setValue(templateMaterial->transparency[0]);
		faceSep->addChild(faceMaterial);

		result.faceMaterials[faceName] = faceMaterial;
		result.faceSeparators[faceName] = faceSep;

		SoIndexedFaceSet* faceSet = new SoIndexedFaceSet;

		auto verticesIt = m_faces.find(pickId);
		if (verticesIt == m_faces.end()) {
			LOG_WRN_S("Missing vertex data for face " + faceName);
			return;
		}

		const auto& vertices = verticesIt->second.vertexArray;

		for (size_t i = 0; i < vertices.size(); ++i) {
			coords->point.set1Value(vertexIndex + static_cast<int>(i), vertices[i]);
			faceSet->coordIndex.set1Value(static_cast<int>(i), vertexIndex + static_cast<int>(i));
		}
		faceSet->coordIndex.set1Value(static_cast<int>(vertices.size()), -1);
		vertexIndex += static_cast<int>(vertices.size());

		if (materialType != 0) {
			for (size_t i = 0; i < vertices.size(); ++i) {
				faceSet->textureCoordIndex.set1Value(static_cast<int>(i), 0);
			}
			faceSet->textureCoordIndex.set1Value(static_cast<int>(vertices.size()), -1);
		}

		faceSep->addChild(faceSet);

		SoSeparator* outlineSep = new SoSeparator;
		outlineSep->setName(SbName((faceName + "_Outline").c_str()));

		SoDrawStyle* lineStyle = new SoDrawStyle;
		lineStyle->style = SoDrawStyle::LINES;
		lineStyle->lineWidth = 1.0f;
		outlineSep->addChild(lineStyle);

		SoMaterial* outlineMaterial = new SoMaterial;
		float outlineR = static_cast<float>(config.getDouble("NavigationCube", "CubeOutlineColorR", 0.4));
		float outlineG = static_cast<float>(config.getDouble("NavigationCube", "CubeOutlineColorG", 0.6));
		float outlineB = static_cast<float>(config.getDouble("NavigationCube", "CubeOutlineColorB", 0.9));
		outlineMaterial->diffuseColor.setValue(outlineR, outlineG, outlineB);
		outlineMaterial->transparency.setValue(0.0f);
		outlineSep->addChild(outlineMaterial);

		SoIndexedLineSet* faceOutline = new SoIndexedLineSet;
		int startIndex = vertexIndex - static_cast<int>(vertices.size());
		std::vector<int32_t> outlineIndices;
		outlineIndices.reserve(vertices.size() + 2);
		for (size_t i = 0; i < vertices.size(); ++i) {
			outlineIndices.push_back(startIndex + static_cast<int>(i));
		}
		outlineIndices.push_back(startIndex);
		outlineIndices.push_back(-1);

		faceOutline->coordIndex.setValues(0, static_cast<int>(outlineIndices.size()), outlineIndices.data());
		outlineSep->addChild(faceOutline);

		faceSep->addChild(outlineSep);

		cubeAssembly->addChild(faceSep);

		if (materialType == 0) {
			auto labelIt = m_labelTextures.find(pickId);
			if (labelIt != m_labelTextures.end() && !labelIt->second.vertexArray.empty()) {
				SoSeparator* textureSep = new SoSeparator;
				textureSep->setName(SbName((faceName + "_Texture").c_str()));

				SoPolygonOffset* polygonOffset = new SoPolygonOffset;
				polygonOffset->factor.setValue(-1.0f);
				polygonOffset->units.setValue(-1.0f);
				textureSep->addChild(polygonOffset);

				SoMaterial* textureMaterial = new SoMaterial;
				float materialR = static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseR", 0.9));
				float materialG = static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseG", 0.95));
				float materialB = static_cast<float>(config.getDouble("NavigationCube", "CubeBodyDiffuseB", 1.0));
				textureMaterial->diffuseColor.setValue(materialR, materialG, materialB);
				textureMaterial->ambientColor.setValue(materialR, materialG, materialB);
				textureMaterial->specularColor.setValue(materialR, materialG, materialB);
				textureMaterial->transparency.setValue(0.0f);
				textureMaterial->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
				textureSep->addChild(textureMaterial);
				result.faceTextureMaterials[faceName] = textureMaterial;

				SoIndexedFaceSet* textureQuad = new SoIndexedFaceSet;

				const auto& labelVertices = labelIt->second.vertexArray;
				int labelStartIndex = vertexIndex;
				for (size_t i = 0; i < labelVertices.size(); ++i) {
					coords->point.set1Value(labelStartIndex + static_cast<int>(i), labelVertices[i]);
					textureQuad->coordIndex.set1Value(static_cast<int>(i), labelStartIndex + static_cast<int>(i));
				}
				textureQuad->coordIndex.set1Value(static_cast<int>(labelVertices.size()), -1);
				vertexIndex += static_cast<int>(labelVertices.size());

				textureQuad->textureCoordIndex.set1Value(0, 0);
				textureQuad->textureCoordIndex.set1Value(1, 1);
				textureQuad->textureCoordIndex.set1Value(2, 2);
				textureQuad->textureCoordIndex.set1Value(3, 3);
				textureQuad->textureCoordIndex.set1Value(4, -1);

				textureSep->addChild(textureQuad);
				cubeAssembly->addChild(textureSep);
				result.faceSeparators[faceName + "_Texture"] = textureSep;
			}
		}
	};

	insertFace("FRONT", PickId::Front, 0);
	insertFace("REAR", PickId::Rear, 0);
	insertFace("LEFT", PickId::Left, 0);
	insertFace("RIGHT", PickId::Right, 0);
	insertFace("TOP", PickId::Top, 0);
	insertFace("BOTTOM", PickId::Bottom, 0);

	insertFace("Corner0", PickId::FrontTopRight, 2);
	insertFace("Corner1", PickId::FrontTopLeft, 2);
	insertFace("Corner2", PickId::FrontBottomRight, 2);
	insertFace("Corner3", PickId::FrontBottomLeft, 2);
	insertFace("Corner4", PickId::RearTopRight, 2);
	insertFace("Corner5", PickId::RearTopLeft, 2);
	insertFace("Corner6", PickId::RearBottomRight, 2);
	insertFace("Corner7", PickId::RearBottomLeft, 2);

	insertFace("EdgeTF", PickId::FrontTop, 1);
	insertFace("EdgeTB", PickId::RearTop, 1);
	insertFace("EdgeTL", PickId::TopLeft, 1);
	insertFace("EdgeTR", PickId::TopRight, 1);
	insertFace("EdgeBF", PickId::FrontBottom, 1);
	insertFace("EdgeBB", PickId::RearBottom, 1);
	insertFace("EdgeBL", PickId::BottomLeft, 1);
	insertFace("EdgeBR", PickId::BottomRight, 1);
	insertFace("EdgeFR", PickId::FrontRight, 1);
	insertFace("EdgeFL", PickId::FrontLeft, 1);
	insertFace("EdgeBL2", PickId::RearLeft, 1);
	insertFace("EdgeBR2", PickId::RearRight, 1);

	result.faces = m_faces;
	result.labelTextures = m_labelTextures;

	result.geometryRoot->unrefNoDelete();

	return result;
}

void NavigationCubeGeometryBuilder::addCubeFace(const SbVec3f& x, const SbVec3f& z, ShapeId shapeType, PickId pickId, float rotZ) {
	FaceData& face = m_faces[pickId];
	face.vertexArray.clear();
	face.type = shapeType;

	SbVec3f y = x.cross(-z);

	SbVec3f xN = x;
	SbVec3f yN = y;
	SbVec3f zN = z;
	xN.normalize();
	yN.normalize();
	zN.normalize();

	SbMatrix R(
		xN[0], yN[0], zN[0], 0,
		xN[1], yN[1], zN[1], 0,
		xN[2], yN[2], zN[2], 0,
		0, 0, 0, 1);

	face.rotation = (SbRotation(R) * SbRotation(SbVec3f(0, 0, 1), rotZ)).inverse();

	if (shapeType == ShapeId::Corner) {
		float chamfer = m_chamferSize;
		float zDepth = 1.0f - 2.0f * chamfer;
		face.vertexArray.reserve(6);
		face.vertexArray.emplace_back(SbVec3f(z[0] * zDepth - 2 * x[0] * chamfer,
			z[1] * zDepth - 2 * x[1] * chamfer,
			z[2] * zDepth - 2 * x[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zDepth - x[0] * chamfer - y[0] * chamfer,
			z[1] * zDepth - x[1] * chamfer - y[1] * chamfer,
			z[2] * zDepth - x[2] * chamfer - y[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zDepth + x[0] * chamfer - y[0] * chamfer,
			z[1] * zDepth + x[1] * chamfer - y[1] * chamfer,
			z[2] * zDepth + x[2] * chamfer - y[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zDepth + 2 * x[0] * chamfer,
			z[1] * zDepth + 2 * x[1] * chamfer,
			z[2] * zDepth + 2 * x[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zDepth + x[0] * chamfer + y[0] * chamfer,
			z[1] * zDepth + x[1] * chamfer + y[1] * chamfer,
			z[2] * zDepth + x[2] * chamfer + y[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zDepth - x[0] * chamfer + y[0] * chamfer,
			z[1] * zDepth - x[1] * chamfer + y[1] * chamfer,
			z[2] * zDepth - x[2] * chamfer + y[2] * chamfer));
	} else if (shapeType == ShapeId::Edge) {
		float chamfer = m_chamferSize;
		float x4Scale = 1.0f - chamfer * 4.0f;
		float zEScale = 1.0f - chamfer;
		face.vertexArray.reserve(4);
		face.vertexArray.emplace_back(SbVec3f(z[0] * zEScale - x[0] * x4Scale - y[0] * chamfer,
			z[1] * zEScale - x[1] * x4Scale - y[1] * chamfer,
			z[2] * zEScale - x[2] * x4Scale - y[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zEScale + x[0] * x4Scale - y[0] * chamfer,
			z[1] * zEScale + x[1] * x4Scale - y[1] * chamfer,
			z[2] * zEScale + x[2] * x4Scale - y[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zEScale + x[0] * x4Scale + y[0] * chamfer,
			z[1] * zEScale + x[1] * x4Scale + y[1] * chamfer,
			z[2] * zEScale + x[2] * x4Scale + y[2] * chamfer));
		face.vertexArray.emplace_back(SbVec3f(z[0] * zEScale - x[0] * x4Scale + y[0] * chamfer,
			z[1] * zEScale - x[1] * x4Scale + y[1] * chamfer,
			z[2] * zEScale - x[2] * x4Scale + y[2] * chamfer));
	} else {
		float chamfer = m_chamferSize;
		float x2Scale = 1.0f - chamfer * 2.0f;
		float y2Scale = 1.0f - chamfer * 2.0f;
		float x4Scale = 1.0f - chamfer * 4.0f;
		float y4Scale = 1.0f - chamfer * 4.0f;
		face.vertexArray.reserve(8);
		face.vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x2Scale - y[0] * y4Scale,
			z[1] - x[1] * x2Scale - y[1] * y4Scale,
			z[2] - x[2] * x2Scale - y[2] * y4Scale));
		face.vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x4Scale - y[0] * y2Scale,
			z[1] - x[1] * x4Scale - y[1] * y2Scale,
			z[2] - x[2] * x4Scale - y[2] * y2Scale));
		face.vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x4Scale - y[0] * y2Scale,
			z[1] + x[1] * x4Scale - y[1] * y2Scale,
			z[2] + x[2] * x4Scale - y[2] * y2Scale));
		face.vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x2Scale - y[0] * y4Scale,
			z[1] + x[1] * x2Scale - y[1] * y4Scale,
			z[2] + x[2] * x2Scale - y[2] * y4Scale));
		face.vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x2Scale + y[0] * y4Scale,
			z[1] + x[1] * x2Scale + y[1] * y4Scale,
			z[2] + x[2] * x2Scale + y[2] * y4Scale));
		face.vertexArray.emplace_back(SbVec3f(z[0] + x[0] * x4Scale + y[0] * y2Scale,
			z[1] + x[1] * x4Scale + y[1] * y2Scale,
			z[2] + x[2] * x4Scale + y[2] * y2Scale));
		face.vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x4Scale + y[0] * y2Scale,
			z[1] - x[1] * x4Scale + y[1] * y2Scale,
			z[2] - x[2] * x4Scale + y[2] * y2Scale));
		face.vertexArray.emplace_back(SbVec3f(z[0] - x[0] * x2Scale + y[0] * y4Scale,
			z[1] - x[1] * x2Scale + y[1] * y4Scale,
			z[2] - x[2] * x2Scale + y[2] * y4Scale));

		LabelTextureData& labelData = m_labelTextures[pickId];
		labelData.vertexArray.clear();

		auto x01Mid = x * (x2Scale + x4Scale) * 0.5f;
		auto y01Mid = y * (y4Scale + y2Scale) * 0.5f;
		labelData.vertexArray.emplace_back(z - x01Mid - y01Mid);

		auto x23Mid = x * (x4Scale + x2Scale) * 0.5f;
		auto y23Mid = y * (y2Scale + y4Scale) * 0.5f;
		labelData.vertexArray.emplace_back(z + x23Mid - y23Mid);

		auto x45Mid = x * (x2Scale + x4Scale) * 0.5f;
		auto y45Mid = y * (y4Scale + y2Scale) * 0.5f;
		labelData.vertexArray.emplace_back(z + x45Mid + y45Mid);

		auto x67Mid = x * (x4Scale + x2Scale) * 0.5f;
		auto y67Mid = y * (y2Scale + y4Scale) * 0.5f;
		labelData.vertexArray.emplace_back(z - x67Mid + y67Mid);
	}
}
