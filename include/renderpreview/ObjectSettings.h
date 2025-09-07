#pragma once

#include <wx/colour.h>
#include <string>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>

enum class ObjectType {
    BOX = 0,
    SPHERE = 1,
    CONE = 2,
    CYLINDER = 3,
    TORUS = 4
};

enum class TextureMode {
    REPLACE = 0,
    MODULATE = 1,
    DECAL = 2,
    BLEND = 3
};

// Object material and texture settings
struct ObjectSettings {
    int objectId;
    ObjectType objectType;
    std::string name;
    bool enabled;
    
    // Material properties
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float transparency;
    wxColour materialColor;
    
    // Texture properties
    bool textureEnabled;
    TextureMode textureMode;
    float textureScale;
    std::string texturePath;
    float textureRotation;
    SbVec3f textureOffset;
    
    // Transform properties
    SbVec3f position;
    SbVec3f rotation;
    SbVec3f scale;
    
    // Visibility properties
    bool visible;
    bool wireframe;
    bool showNormals;
    
    ObjectSettings()
        : objectId(-1)
        , objectType(ObjectType::BOX)
        , name("Default Object")
        , enabled(true)
        , ambient(0.2f)
        , diffuse(0.8f)
        , specular(0.6f)
        , shininess(32.0f)
        , transparency(0.0f)
        , materialColor(128, 128, 128)
        , textureEnabled(false)
        , textureMode(TextureMode::MODULATE)
        , textureScale(1.0f)
        , texturePath("")
        , textureRotation(0.0f)
        , textureOffset(0.0f, 0.0f, 0.0f)
        , position(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f)
        , scale(1.0f, 1.0f, 1.0f)
        , visible(true)
        , wireframe(false)
        , showNormals(false)
    {}
}; 