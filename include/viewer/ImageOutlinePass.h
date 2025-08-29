#pragma once

#include <memory>

class SceneManager;
class SoSeparator;
class SoAnnotation;
class SoShaderProgram;
class SoFragmentShader;
class SoVertexShader;
class SoTexture2;
class SoSceneTexture2;
class SoCamera;
class SoShaderParameter1f;
class SoShaderParameter2f;
class SoShaderParameterMatrix;
class SoShaderParameter1i;

struct ImageOutlineParams {
	float depthWeight = 1.0f;
	float normalWeight = 1.0f;
	float depthThreshold = 0.002f;
	float normalThreshold = 0.1f;
	float edgeIntensity = 1.0f;
	float thickness = 1.0f; // logical thickness factor
};

// Minimal image-space outline pass (skeleton). Currently wires an overlay node,
// ready for future FBO + shader hookup.
class ImageOutlinePass {
public:
	ImageOutlinePass(SceneManager* sceneManager, SoSeparator* captureRoot);
	~ImageOutlinePass();

	void setEnabled(bool enabled);
	bool isEnabled() const { return m_enabled; }

	void setParams(const ImageOutlineParams& p);
	ImageOutlineParams getParams() const { return m_params; }

	enum class DebugOutput : int { Final = 0, ShowColor = 1, ShowEdge = 2 };
	void setDebugOutput(DebugOutput mode);

	void refresh();

private:
	void attachOverlay();
	void detachOverlay();
	void buildShaders();

	SceneManager* m_sceneManager;
	bool m_enabled{ false };
	ImageOutlineParams m_params{};

	SoSeparator* m_overlayRoot{ nullptr };
	SoAnnotation* m_annotation{ nullptr };
	SoSeparator* m_captureRoot{ nullptr };

	// Shader nodes
	SoShaderProgram* m_program{ nullptr };
	SoVertexShader* m_vs{ nullptr };
	SoFragmentShader* m_fs{ nullptr };

	// Render-to-texture nodes
	SoSceneTexture2* m_colorTexture{ nullptr };
	SoSceneTexture2* m_depthTexture{ nullptr };
	SoTexture2* m_colorSampler{ nullptr };
	SoTexture2* m_depthSampler{ nullptr };

	// Geometry
	SoSeparator* m_quadSeparator{ nullptr };

	// Shader parameters for runtime updates
	SoShaderParameter1f* m_uIntensity{ nullptr };
	SoShaderParameter1f* m_uDepthWeight{ nullptr };
	SoShaderParameter1f* m_uNormalWeight{ nullptr };
	SoShaderParameter1f* m_uDepthThreshold{ nullptr };
	SoShaderParameter1f* m_uNormalThreshold{ nullptr };
	SoShaderParameter1f* m_uThickness{ nullptr };
	SoShaderParameter2f* m_uResolution{ nullptr };
	SoShaderParameterMatrix* m_uInvProjection{ nullptr };
	SoShaderParameterMatrix* m_uInvView{ nullptr };
	SoShaderParameter1i* m_uDebugOutput{ nullptr };

	// Texture unit management
	int m_colorUnit{ 0 };
	int m_depthUnit{ 1 };

	// Helpers
	bool chooseTextureUnits();
	void updateCameraMatrices();

	DebugOutput m_debugOutput{ DebugOutput::Final };
	SoSeparator* m_tempSceneRoot{ nullptr }; // Temporary scene root for RTT
};
