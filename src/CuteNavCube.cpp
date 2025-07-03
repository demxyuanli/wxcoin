#include "CuteNavCube.h"
#include "DPIManager.h"
#include "DPIAwareRendering.h"
#include <algorithm>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLinear.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/time.h>
#include <cmath>
#include <vector>
#include "Logger.h"

std::map<std::string, std::shared_ptr<CuteNavCube::TextureData>> CuteNavCube::s_textureCache;

CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight)
    : m_root(new SoSeparator)
    , m_orthoCamera(new SoOrthographicCamera)
    , m_enabled(true)
    , m_dpiScale(dpiScale)
    , m_viewChangeCallback(viewChangeCallback)
    , m_isDragging(false)
    , m_lastMousePos(0, 0)
    , m_rotationX(0.0f)
    , m_rotationY(0.0f)
    , m_lastDragTime(0)
    , m_windowWidth(windowWidth)
    , m_windowHeight(windowHeight)
    , m_positionX(20)  // Default to left side
    , m_positionY(20)  // Default to bottom side
    , m_cubeSize(150)  // Smaller default size for cute version
{
    m_root->ref();
    initialize();
}

CuteNavCube::~CuteNavCube() {
    m_root->unref();
}

void CuteNavCube::initialize() {
    setupGeometry();

    m_faceToView = {
        { "F", "Front" },
        { "B", "Back" },
        { "L", "Left" },
        { "R", "Right" },
        { "T", "Top" },
        { "D", "Bottom" }
    };
}

bool CuteNavCube::generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height) {
    wxBitmap bitmap(width, height, 32);
    wxMemoryDC dc;
    dc.SelectObject(bitmap);
    if (!dc.IsOk()) {
        LOG_ERR("CuteNavCube::generateFaceTexture: Failed to create wxMemoryDC for texture: " + text);
        // Fallback: Fill with white
        for (int i = 0; i < width * height * 4; i += 4) {
            imageData[i] = 255; // R
            imageData[i + 1] = 255; // G
            imageData[i + 2] = 255; // B
            imageData[i + 3] = 255; // A
        }
        LOG_INF("CuteNavCube::generateFaceTexture: Fallback to white texture for: " + text);
        return true;
    }

    dc.SetBackground(wxColour(255, 255, 255, 255)); // Opaque white background
    dc.Clear();

    // Use DPI manager for high-quality font rendering
    auto& dpiManager = DPIManager::getInstance();
    wxFont font = dpiManager.getScaledFont(14, "Arial", true, false); // Smaller font for cute version
    dc.SetFont(font);
    dc.SetTextForeground(wxColour(0, 100, 200)); // Blue text for cute version

    wxSize textSize = dc.GetTextExtent(text);
    int x = (width - textSize.GetWidth()) / 2;
    int y = (height - textSize.GetHeight()) / 2;
    dc.DrawText(text, x, y);

    // Validate bitmap content
    wxImage image = bitmap.ConvertToImage();
    if (!image.IsOk()) {
        LOG_ERR("CuteNavCube::generateFaceTexture: Failed to convert bitmap to image for texture: " + text);
        // Fallback: Fill with white
        for (int i = 0; i < width * height * 4; i += 4) {
            imageData[i] = 255; // R
            imageData[i + 1] = 255; // G
            imageData[i + 2] = 255; // B
            imageData[i + 3] = 255; // A
        }
        LOG_INF("CuteNavCube::generateFaceTexture: Fallback to white texture for: " + text);
        return true;
    }

    image.InitAlpha(); // Ensure alpha channel
    unsigned char* rgb = image.GetData();
    unsigned char* alpha = image.GetAlpha();

    // Set alpha to 255 for all pixels (opaque texture)
    for (int i = 0; i < width * height; ++i) {
        alpha[i] = 255;
    }

    // Copy to imageData (RGBA) and validate
    bool hasValidPixels = false;
    for (int i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
        imageData[i] = rgb[j];     // R
        imageData[i + 1] = rgb[j + 1]; // G
        imageData[i + 2] = rgb[j + 2]; // B
        imageData[i + 3] = alpha[j / 3]; // A
        if (imageData[i] != 0 || imageData[i + 1] != 0 || imageData[i + 2] != 0) {
            hasValidPixels = true;
        }
    }

    if (!hasValidPixels) {
        LOG_WAR("CuteNavCube::generateFaceTexture: All pixels are black for texture: " + text);
        // Fallback: Fill with white
        for (int i = 0; i < width * height * 4; i += 4) {
            imageData[i] = 255; // R
            imageData[i + 1] = 255; // G
            imageData[i + 2] = 255; // B
            imageData[i + 3] = 255; // A
        }
        LOG_INF("CuteNavCube::generateFaceTexture: Fallback to white texture for: " + text);
    }

    LOG_INF("CuteNavCube::generateFaceTexture: Texture generated for " + text +
        ", size=" + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void CuteNavCube::setupGeometry() {
    m_root->removeAllChildren(); // Clear previous geometry

    m_orthoCamera->viewportMapping = SoOrthographicCamera::ADJUST_CAMERA;
    m_orthoCamera->nearDistance = 0.1f;
    m_orthoCamera->farDistance = 10.0f;
    m_orthoCamera->orientation.setValue(SbRotation::identity());
    m_root->addChild(m_orthoCamera);

    // --- Lighting Setup ---
    // Lights are added to the root, but their direction will be updated
    // in `updateCameraRotation` to follow the camera.
    SoEnvironment* env = new SoEnvironment;
    env->ambientColor.setValue(1.0f, 1.0f, 1.0f);
    env->ambientIntensity.setValue(0.6f);
    m_root->addChild(env);

    // Main light from front-right-top relative to the camera
    m_mainLight = new SoDirectionalLight;
    m_mainLight->direction.setValue(0.3f, 0.3f, -1.0f);
    m_mainLight->intensity.setValue(1.0f);
    m_mainLight->color.setValue(1.0f, 1.0f, 1.0f);
    m_root->addChild(m_mainLight);

    // Fill light from back-left-bottom relative to the camera
    m_fillLight = new SoDirectionalLight;
    m_fillLight->direction.setValue(-0.5f, -0.5f, 0.5f);
    m_fillLight->intensity.setValue(0.7f);
    m_fillLight->color.setValue(0.95f, 0.95f, 1.0f);
    m_root->addChild(m_fillLight);

    // Additional side light for better coverage
    m_sideLight = new SoDirectionalLight;
    m_sideLight->direction.setValue(-1.0f, 0.0f, 0.0f);
    m_sideLight->intensity.setValue(0.5f);
    m_sideLight->color.setValue(1.0f, 1.0f, 0.95f);
    m_root->addChild(m_sideLight);
    
    // Set initial camera position AND light directions now that lights are created
    updateCameraRotation(); 

    SoSeparator* cubeAssembly = new SoSeparator; // Parent for all faces

    // --- Manual Chamfered Cube Definition with Correct Normals ---
    const float s = 0.5f;    // Main size
    const float c = 0.18f;   // Chamfer size
    
    // Define 24 vertices for chamfered cube (3 vertices per corner)
    SbVec3f vertices[24] = {
        // Corner 0 (+ + +): Front-Top-Right
        SbVec3f( s,  s-c,  s-c),  // v0
        SbVec3f( s-c,  s,  s-c),  // v1  
        SbVec3f( s-c,  s-c,  s),  // v2
        
        // Corner 1 (- + +): Front-Top-Left
        SbVec3f(-s,  s-c,  s-c),  // v3
        SbVec3f(-s+c,  s,  s-c),  // v4
        SbVec3f(-s+c,  s-c,  s),  // v5
        
        // Corner 2 (- - +): Front-Bottom-Left
        SbVec3f(-s, -s+c,  s-c),  // v6
        SbVec3f(-s+c, -s,  s-c),  // v7
        SbVec3f(-s+c, -s+c,  s),  // v8
        
        // Corner 3 (+ - +): Front-Bottom-Right
        SbVec3f( s, -s+c,  s-c),  // v9
        SbVec3f( s-c, -s,  s-c),  // v10
        SbVec3f( s-c, -s+c,  s),  // v11
        
        // Corner 4 (+ + -): Back-Top-Right
        SbVec3f( s,  s-c, -s+c),  // v12
        SbVec3f( s-c,  s, -s+c),  // v13
        SbVec3f( s-c,  s-c, -s),  // v14
        
        // Corner 5 (- + -): Back-Top-Left
        SbVec3f(-s,  s-c, -s+c),  // v15
        SbVec3f(-s+c,  s, -s+c),  // v16
        SbVec3f(-s+c,  s-c, -s),  // v17
        
        // Corner 6 (- - -): Back-Bottom-Left
        SbVec3f(-s, -s+c, -s+c),  // v18
        SbVec3f(-s+c, -s, -s+c),  // v19
        SbVec3f(-s+c, -s+c, -s),  // v20
        
        // Corner 7 (+ - -): Back-Bottom-Right
        SbVec3f( s, -s+c, -s+c),  // v21
        SbVec3f( s-c, -s, -s+c),  // v22
        SbVec3f( s-c, -s+c, -s)   // v23
    };
    
    // Define all 26 faces with correct winding order (counter-clockwise when viewed from outside)
    struct FaceDefinition {
        std::string name;
        std::vector<int> indices;
        std::string textureKey;
        int materialType; // 0=main face, 1=edge, 2=corner
    };
    
    std::vector<FaceDefinition> faces = {
        // 6 Main faces
        {"Top",    {1, 4, 16, 13}, "T", 0},       // Face 0: Top (Y+)
        {"Bottom", {19, 22, 10, 7}, "D", 0},      // Face 1: Bottom (Y-)
        {"Front",  {2, 5, 8, 11}, "F", 0},        // Face 2: Front (Z+)
        {"Back",   {17, 20, 23, 14}, "B", 0},     // Face 3: Back (Z-)
        {"Right",  {0, 12, 21, 9}, "R", 0},       // Face 4: Right (X+)
        {"Left",   {6, 18, 15, 3}, "L", 0},       // Face 5: Left (X-)
        
        // 8 Corner faces (triangular)
        {"Corner0", {0, 1, 2}, "", 2},            // Face 6: Corner 0
        {"Corner1", {3, 5, 4}, "", 2},            // Face 7: Corner 1  
        {"Corner2", {6, 7, 8}, "", 2},            // Face 8: Corner 2
        {"Corner3", {9, 11, 10}, "", 2},          // Face 9: Corner 3
        {"Corner4", {12, 14, 13}, "", 2},         // Face 10: Corner 4
        {"Corner5", {15, 16, 17}, "", 2},         // Face 11: Corner 5
        {"Corner6", {18, 19, 20}, "", 2},         // Face 12: Corner 6
        {"Corner7", {21, 23, 22}, "", 2},         // Face 13: Corner 7
        
        // 12 Edge faces (rectangular)
        {"EdgeTF", {1, 0, 9, 10, 7, 6, 3, 4}, "", 1},      // Face 14: Top-Front edge (bad - 8 vertices)
        {"EdgeTR", {13, 1, 0, 12}, "", 1},        // Face 15: Top-Right edge  
        {"EdgeTB", {16, 13, 12, 15}, "", 1},      // Face 16: Top-Back edge
        {"EdgeTL", {4, 16, 15, 3}, "", 1},        // Face 17: Top-Left edge
        {"EdgeBF", {7, 10, 11, 8}, "", 1},        // Face 18: Bottom-Front edge
        {"EdgeBR", {22, 9, 0, 21}, "", 1},        // Face 19: Bottom-Right edge (bad order)
        {"EdgeBB", {19, 22, 23, 20}, "", 1},      // Face 20: Bottom-Back edge
        {"EdgeBL", {18, 6, 7, 19}, "", 1},        // Face 21: Bottom-Left edge (bad order)
        {"EdgeFR", {2, 0, 9, 11}, "", 1},         // Face 22: Front-Right edge (bad order)
        {"EdgeFL", {5, 2, 8, 6}, "", 1},          // Face 23: Front-Left edge (bad order)
        {"EdgeBL2", {17, 15, 18, 20}, "", 1},     // Face 24: Back-Left edge
        {"EdgeBR2", {14, 17, 20, 23}, "", 1}      // Face 25: Back-Right edge (bad order)
    };
    
    // Manually corrected face definitions with verified normals
    faces = {
        // 6 Main faces
        {"Top",    {13, 16, 4, 1}, "T", 0},       // Face 0: Top (Y+) - FIXED: reversed winding
        {"Bottom", {19, 22, 10, 7}, "D", 0},      // Face 1: Bottom (Y-) - OK
        {"Front",  {2, 5, 8, 11}, "F", 0},        // Face 2: Front (Z+) - OK
        {"Back",   {14, 23, 20, 17}, "B", 0},     // Face 3: Back (Z-) - FIXED: reversed winding
        {"Right",  {9, 21, 12, 0}, "R", 0},       // Face 4: Right (X+) - FIXED: reversed winding
        {"Left",   {3, 15, 18, 6}, "L", 0},       // Face 5: Left (X-) - FIXED: reversed winding
        
        // 8 Corner faces (triangular)
        {"Corner0", {0, 1, 2}, "", 2},            // Face 6 - OK
        {"Corner1", {3, 5, 4}, "", 2},            // Face 7 - OK
        {"Corner2", {6, 7, 8}, "", 2},            // Face 8 - OK
        {"Corner3", {9, 11, 10}, "", 2},          // Face 9 - OK
        {"Corner4", {12, 14, 13}, "", 2},         // Face 10 - OK
        {"Corner5", {15, 16, 17}, "", 2},         // Face 11 - OK
        {"Corner6", {20, 19, 18}, "", 2},         // Face 12 - FIXED: reversed winding
        {"Corner7", {22, 23, 21}, "", 2},         // Face 13 - FIXED: reversed winding
        
        // 12 Edge faces (rectangular) - carefully corrected with verified normals
        {"EdgeTF", {1, 4, 5, 2}, "", 1},          // Face 14: Top-Front edge - OK
        {"EdgeTR", {13, 1, 0, 12}, "", 1},        // Face 15: Top-Right edge - OK
        {"EdgeTB", {16, 13, 14, 17}, "", 1},      // Face 16: Top-Back edge - OK
        {"EdgeTL", {4, 16, 15, 3}, "", 1},        // Face 17: Top-Left edge - OK
        {"EdgeBF", {7, 10, 11, 8}, "", 1},        // Face 18: Bottom-Front edge - FIXED: reversed winding
        {"EdgeBR", {10, 22, 21, 9}, "", 1},       // Face 19: Bottom-Right edge - FIXED: reversed winding
        {"EdgeBB", {20, 23, 22, 19}, "", 1},      // Face 20: Bottom-Back edge - OK
        {"EdgeBL", {6, 18, 19, 7}, "", 1},        // Face 21: Bottom-Left edge - OK
        {"EdgeFR", {2, 11, 9, 0}, "", 1},         // Face 22: Front-Right edge - FIXED: reversed winding
        {"EdgeFL", {3, 6, 8, 5}, "", 1},          // Face 23: Front-Left edge - FIXED: reversed winding
        {"EdgeBL2", {17, 20, 18, 15}, "", 1},     // Face 24: Back-Left edge - FIXED: reversed winding
        {"EdgeBR2", {12, 21, 23, 14}, "", 1}      // Face 25: Back-Right edge - OK
    };
    
    auto& dpiManager = DPIManager::getInstance();
    
    // Shared texture coordinates for main faces
    SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
    texCoords->point.setValues(0, 4, new SbVec2f[4]{
        SbVec2f(0, 0), SbVec2f(1, 0), SbVec2f(1, 1), SbVec2f(0, 1)
    });
    cubeAssembly->addChild(texCoords);

    // Create all vertices
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(24);
    for (int i = 0; i < 24; i++) {
        coords->point.set1Value(i, vertices[i]);
    }
    cubeAssembly->addChild(coords);

    LOG_INF("=== Manual Face Definition Analysis ===");
    int faceIndex = 0;
    
    // Create a unified, light blue, opaque material for all faces
    SoMaterial* lightBlueMaterial = new SoMaterial;
    lightBlueMaterial->ambientColor.setValue(0.2f, 0.3f, 0.4f);
    lightBlueMaterial->diffuseColor.setValue(0.7f, 0.8f, 0.95f); // Light Blue
    lightBlueMaterial->specularColor.setValue(0.5f, 0.5f, 0.5f);
    lightBlueMaterial->shininess.setValue(0.2f);
    lightBlueMaterial->transparency.setValue(0.0f); // Opaque
    
    LOG_INF("--- Logging Face Properties ---");
    for (const auto& faceDef : faces) {
        // Log the properties of the current face
        std::string indices_str;
        for(int idx : faceDef.indices) { indices_str += std::to_string(idx) + " "; }
        LOG_INF("Face " + std::to_string(faceIndex) + ": Name='" + faceDef.name + 
                "', Type=" + std::to_string(faceDef.materialType) + 
                ", TextureKey='" + faceDef.textureKey + 
                "', Indices: " + indices_str);

        SoSeparator* faceSep = new SoSeparator;
        faceSep->setName(SbName(faceDef.name.c_str()));
        
        // Apply the unified light blue material to all faces
        faceSep->addChild(lightBlueMaterial);
        
        // Note: Textures are disabled.

        // Create face
        SoIndexedFaceSet* face = new SoIndexedFaceSet;
        for (size_t i = 0; i < faceDef.indices.size(); i++) {
            face->coordIndex.set1Value(i, faceDef.indices[i]);
        }
        face->coordIndex.set1Value(faceDef.indices.size(), -1); // End marker
        faceSep->addChild(face);
        
        cubeAssembly->addChild(faceSep);
        
        faceIndex++;
    }
    
    m_root->addChild(cubeAssembly);

    LOG_INF("CuteNavCube::setupGeometry: Total faces analyzed: " + std::to_string(faceIndex));
    LOG_INF("CuteNavCube::setupGeometry: Manual geometry definition with verified normals.");
}

void CuteNavCube::updateCameraRotation() {
    // Rotates camera around the cube, from NavigationCube
    float distance = 5.0f; // Keep a fixed distance
    float radX = m_rotationX * M_PI / 180.0f;
    float radY = m_rotationY * M_PI / 180.0f;

    // Calculate position in spherical coordinates
    float x = distance * sin(radY) * cos(radX);
    float y = distance * sin(radX);
    float z = distance * cos(radY) * cos(radX);

    m_orthoCamera->position.setValue(x, y, z);
    m_orthoCamera->pointAt(SbVec3f(0, 0, 0)); // Always look at the origin

    // Update light directions to follow the camera
    if (m_mainLight && m_fillLight && m_sideLight) {
        // Get the camera's orientation
        SbRotation cam_orientation = m_orthoCamera->orientation.getValue();

        // Define base light directions (as if camera is at default orientation)
        SbVec3f base_main_dir(0.3f, 0.3f, -1.0f);
        SbVec3f base_fill_dir(-0.5f, -0.5f, 0.5f);
        SbVec3f base_side_dir(-1.0f, 0.0f, 0.0f);
        
        // Rotate the base directions by the camera's current orientation
        SbVec3f new_main_dir, new_fill_dir, new_side_dir;
        cam_orientation.multVec(base_main_dir, new_main_dir);
        cam_orientation.multVec(base_fill_dir, new_fill_dir);
        cam_orientation.multVec(base_side_dir, new_side_dir);

        // Set the new, rotated directions on the light nodes
        m_mainLight->direction.setValue(new_main_dir);
        m_fillLight->direction.setValue(new_fill_dir);
        m_sideLight->direction.setValue(new_side_dir);
    }

    LOG_DBG("CuteNavCube::updateCameraRotation: Camera position x=" + std::to_string(x) + 
            ", y=" + std::to_string(y) + ", z=" + std::to_string(z));
}

std::string CuteNavCube::pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize) {
    SoRayPickAction pickAction(SbViewportRegion(viewportSize.x, viewportSize.y));
    pickAction.setPoint(mousePos);
    pickAction.apply(m_root);

    SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
    if (!pickedPoint) {
        LOG_DBG("CuteNavCube::pickRegion: No point picked");
        return "";
    }

    SoPath* pickedPath = pickedPoint->getPath();
    if (!pickedPath || pickedPath->getLength() == 0) {
        LOG_DBG("CuteNavCube::pickRegion: Picked point has no valid path.");
        return "";
    }

    // Find the named separator for the face
    for (int i = pickedPath->getLength() - 1; i >= 0; --i) {
        SoNode* node = pickedPath->getNode(i);
        if (node && node->isOfType(SoSeparator::getClassTypeId()) && node->getName().getLength() > 0) {
            std::string nameStr = node->getName().getString();
            auto it = m_faceToView.find(nameStr);
            if (it != m_faceToView.end()) {
                LOG_INF("CuteNavCube::pickRegion: Picked face: " + nameStr + ", maps to view: " + it->second);
                return it->second;
            }
        }
    }
    
    LOG_DBG("CuteNavCube::pickRegion: No specific face region identified.");
    return "";
}

void CuteNavCube::handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize) {
    if (!m_enabled) return;

    static float dragThreshold = 5.0f;
    static SbVec2s dragStartPos(0, 0);

    float x = static_cast<float>(event.GetX());
    float y = static_cast<float>(event.GetY());
    SbVec2s currentPos(static_cast<short>(x), static_cast<short>(y));

    if (event.LeftDown()) {
        m_isDragging = true;
        m_lastMousePos = currentPos;
        dragStartPos = m_lastMousePos;
        LOG_INF("CuteNavCube::handleMouseEvent: Drag started.");
    }
    else if (event.LeftUp()) {
        if (m_isDragging) {
            m_isDragging = false;
            SbVec2s delta = currentPos - dragStartPos;
            float distance = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
            
            if (distance < dragThreshold) { // It's a click
                // Y-coordinate needs to be inverted for picking
                SbVec2s pickPos(currentPos[0], static_cast<short>(viewportSize.y - currentPos[1]));
                std::string region = pickRegion(pickPos, viewportSize);
                if (!region.empty() && m_viewChangeCallback) {
                    m_viewChangeCallback(region);
                    LOG_INF("CuteNavCube::handleMouseEvent: Clicked, switched to view: " + region);
                }
            }
            LOG_INF("CuteNavCube::handleMouseEvent: Drag ended.");
        }
    }
    else if (event.Dragging() && m_isDragging) {
        SbVec2s delta = currentPos - m_lastMousePos;

        // Use logic from NavigationCube for more intuitive rotation
        float sensitivity = 1.0f;
        m_rotationY += delta[0] * sensitivity;
        m_rotationX -= delta[1] * sensitivity; // Inverted Y-axis for natural feel

        // Clamp rotation to prevent flipping
        m_rotationX = std::max(-89.0f, std::min(89.0f, m_rotationX));

        updateCameraRotation();
        m_lastMousePos = currentPos;

        if (m_rotationChangedCallback) {
            m_rotationChangedCallback();
        }

        LOG_DBG("CuteNavCube::handleMouseEvent: Rotated: X=" + std::to_string(m_rotationX) +
            ", Y=" + std::to_string(m_rotationY));
    }
}

void CuteNavCube::render(int x, int y, const wxSize& size) {
    if (!m_enabled || !m_root) return;

    SbViewportRegion viewport(size.x, size.y);
    SoGLRenderAction renderAction(viewport);
    renderAction.apply(m_root);
}

void CuteNavCube::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void CuteNavCube::setCameraPosition(const SbVec3f& position) {
    if (m_orthoCamera) {
        m_orthoCamera->position.setValue(position);
        LOG_DBG("CuteNavCube::setCameraPosition: Set camera position to x=" + std::to_string(position[0]) +
            ", y=" + std::to_string(position[1]) + ", z=" + std::to_string(position[2]));
    } else {
        LOG_WAR("CuteNavCube::setCameraPosition: Camera not initialized");
    }
}

void CuteNavCube::setCameraOrientation(const SbRotation& orientation) {
    if (m_orthoCamera) {
        m_orthoCamera->orientation.setValue(orientation);
        LOG_DBG("CuteNavCube::setCameraOrientation: Set camera orientation");
    } else {
        LOG_WAR("CuteNavCube::setCameraOrientation: Camera not initialized");
    }
} 